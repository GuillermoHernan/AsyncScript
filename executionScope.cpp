/* 
 * File:   executionScope.cpp
 * Author: ghernan
 * 
 * Code which manages the AsyncScript execution scopes
 * 
 * Created on January 2, 2017, 10:15 PM
 */

#include "OS_support.h"
#include "executionScope.h"



// BlockScope
//
//////////////////////////////////////////////////

BlockScope::BlockScope(Ref<IScope> parent) : m_pParent(parent)
{
    ASSERT (parent.notNull());
}

/**
 * Checks if a symbol is defined at current scope.
 * It also checks parent scope
 * @param name
 * @return 
 */
bool BlockScope::isDefined(const std::string& name)const
{
    auto it = m_symbols.find(name);

    if (it != m_symbols.end())
        return true;
    else
        return m_pParent->isDefined(name);
}

/**
 * Looks for a symbol in current scope.
 * If not found, it look in the parent scope. 
 * 
 * @param name  Symbol name
 * @return Value for the symbol
 */
Ref<JSValue> BlockScope::get(const std::string& name)const
{
    auto it = m_symbols.find(name);

    if (it != m_symbols.end())
        return it->second.value();
    else
        return m_pParent->get(name);
}

/**
 * Sets a symbol value in current block scope.
 * 
 * If the symbol is already defined in this scope, it is overwritten. If not, it
 * tries to set it in parent scope.
 * 
 * @param name      Symbol name
 * @param value     Symbol value.
 * @return Symbol value.
 */
Ref<JSValue> BlockScope::set(const std::string& name, Ref<JSValue> value)
{
    if (m_symbols.find(name) != m_symbols.end())
        checkedVarWrite(m_symbols, name, value, false);
    else if (m_pParent.notNull())
        m_pParent->set(name, value);

    return value;
}

/**
 * Creates a new variable at current scope.
 * @param name
 * @param value
 * @return 
 */
Ref<JSValue> BlockScope::newVar(const std::string& name, Ref<JSValue> value, bool isConst)
{
    checkedVarWrite(m_symbols, name, value, isConst);

    return value;
}


// FunctionScope
//
//////////////////////////////////////////////////

/**
 * Constructor
 * @param globals
 * @param targetFn
 */
FunctionScope::FunctionScope(Ref<IScope> globals, Ref<JSValue> targetFn) :
m_function(targetFn),
m_globals(globals)
{
    m_this = undefined();
    m_arguments = JSArray::create();
}

/**
 * Adds a new parameter to the parameter list
 * @param value
 * @return Actual number of parameters
 */
int FunctionScope::addParam(Ref<JSValue> value)
{
    const StringVector& paramsDef = m_function->getParams();
    const size_t index = m_arguments->length();

    if (index < paramsDef.size())
    {
        const std::string &name = paramsDef[index];

        checkedVarWrite(m_params, name, value, false);
        m_arguments->push(value);
    }
    else
        m_arguments->push(value);

    return m_arguments->length();
}

/**
 * Gets a parameter by name.
 * Just looks into the parameter list. Doesn't look into any other scope.
 * For a broader scope search, use 'get'
 * 
 * @param name parameter name
 * @return Parameter value or 'undefined' if not found
 */
Ref<JSValue> FunctionScope::getParam(const std::string& name)const
{
    auto it = m_params.find(name);
    
    if (it != m_params.end())
        return it->second.value();
    else
        return undefined();            
}

/**
 * Checks if a symbols is defined
 * @param name
 * @return 
 */
bool FunctionScope::isDefined(const std::string& name)const
{
    if (name == "this" || name == "arguments")
        return true;
    else
        return (m_params.find(name) != m_params.end());
}

/**
 * Gets a value from function scope. It looks for:
 * - 'this' pointer
 * - 'arguments' array
 * - Function arguments
 * @param name Symbol name
 * @return The symbol value or undefined.
 */
Ref<JSValue> FunctionScope::get(const std::string& name)const
{
    if (name == "this")
        return m_this;
    else if (name == "arguments")
        return m_arguments;
    else
    {
        auto it = m_params.find(name);

        if (it != m_params.end())
            return it->second.value();
        else
            error ("'%s' is undefined", name.c_str());
    }
    
    return undefined();
}

/**
 * Sets a value in function scope.
 * It only lets change arguments values
 * @param name
 * @param value
 * @return 
 */
Ref<JSValue> FunctionScope::set(const std::string& name, Ref<JSValue> value)
{
    if (name == "this" || name == "arguments")
        error("'%s' cannot be written", name.c_str());
    else if (m_params.find(name) != m_params.end())
        checkedVarWrite(m_params, name, value, false);
    else 
        error ("'%s' is undefined", name.c_str());

    return value;
}

/**
 * 'newVar' implementation for 'FunctionScope' should never be called, because
 * variables are created at global scope or at block level.
 * It just asserts.
 * @param name
 * @param value
 * @return 
 */
Ref<JSValue> FunctionScope::newVar(const std::string& name, Ref<JSValue> value, bool isConst)
{
    ASSERT(!"Variables cannot be created at 'FunctionScope");

    return undefined();
}

// GlobalScope
//
//////////////////////////////////////////////////

/**
 * Checks if a symbol is defined at current scope.
 * @param name
 * @return 
 */
bool GlobalScope::isDefined(const std::string& name)const
{
    return m_symbols.find(name) != m_symbols.end();
}

/**
 * Looks for a symbol in current scope.
 * 
 * @param name  Symbol name
 * @return Value for the symbol
 */
Ref<JSValue> GlobalScope::get(const std::string& name)const
{
    auto it = m_symbols.find(name);

    if (it != m_symbols.end())
        return it->second.value();
    else
    {
        error ("'%s' is not defined", name.c_str());
        return undefined();
    }
}

/**
 * Sets a symbol value in current block scope.
 * It must have been previously defined with a call to 'newVar'
 * 
 * @param name      Symbol name
 * @param value     Symbol value.
 * @return Symbol value.
 */
Ref<JSValue> GlobalScope::set(const std::string& name, Ref<JSValue> value)
{
    if (!isDefined(name))
        error ("'%s' is not defined", name.c_str());

    checkedVarWrite(m_symbols, name, value, false);

    return value;
}

/**
 * Creates a new variable at global scope.
 * @param name
 * @param value
 * @return 
 */
Ref<JSValue> GlobalScope::newVar(const std::string& name, Ref<JSValue> value, bool isConst)
{
    checkedVarWrite(m_symbols, name, value, isConst);

    return value;
}

/**
 * Generates an object which contains all symbols defined at global scope.
 * @return 
 */
Ref<JSObject> GlobalScope::toObject()
{
    auto result = JSObject::create();
    
    for (auto it = m_symbols.begin(); it != m_symbols.end(); ++it)
        result->writeField(jsString(it->first), it->second.value());
    
    return result;
}
