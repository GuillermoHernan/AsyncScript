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
#include "jsArray.h"


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
 * @param targetFn
 */
FunctionScope::FunctionScope(Ref<JSValue> targetFn,
                             Ref<JSValue> thisObj,
                             Ref<JSArray> params) :
m_function(targetFn),
m_arguments(params),
m_this(thisObj)
{
}

Ref<FunctionScope> FunctionScope::create(Ref<JSValue> targetFn, 
                                 Ref<JSValue> thisObj, 
                                 Ref<JSArray> params)
{
    return refFromNew(new FunctionScope(targetFn, thisObj, params));
}

Ref<FunctionScope> FunctionScope::create(Ref<JSValue> targetFn, 
                                 Ref<JSValue> thisObj, 
                                 const ValueVector& params)
{
    return refFromNew(new FunctionScope(targetFn, thisObj, JSArray::fromVector(params)));
}


/**
 * Retrieves parameter index given its name
 * @param name
 * @return The parameter index or -1 if not found
 */
int FunctionScope::paramIndex (const std::string& name)const
{
    const StringVector& params = m_function->getParams();
    size_t i;
    
    for (i=0; i<params.size(); ++i)
        if (params[i] == name)
            return (int)i;
    
    return -1;
    
//    ASSERT (m_function->isFunction());
//    
//    auto fn = m_function.staticCast<JSFunction>();
//    return fn->paramIndex(name);
}

/**
 * Adds a new parameter to the parameter list
 * @param value
 * @return Actual number of parameters
 */
//int FunctionScope::addParam(Ref<JSValue> value)
//{
//    const StringVector& paramsDef = m_function->getParams();
//    const size_t index = m_arguments->length();
//
//    if (index < paramsDef.size())
//    {
//        const std::string &name = paramsDef[index];
//
//        checkedVarWrite(m_params, name, value, false);
//        m_arguments->push(value);
//    }
//    else
//        m_arguments->push(value);
//
//    return m_arguments->length();
//}

/**
 * Gets a parameter by name.
 * Just looks into the parameter list. Doesn't look into any other scope.
 * For a broader scope search, use 'get'
 * 
 * @param name parameter name
 * @return Parameter value or 'null' if not found
 */
Ref<JSValue> FunctionScope::getParam(const std::string& name)const
{
    const int index = paramIndex(name);
    
    if (index >= 0)
        return m_arguments->getAt(index);
    else
        return jsNull();            
}

/**
 * Returns function call parameters
 * @return 
 */
Ref<JSArray> FunctionScope::getParams()const
{
    return m_arguments;
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
        return paramIndex(name) >= 0;
}

/**
 * Gets a value from function scope. It looks for:
 * - 'this' pointer
 * - 'arguments' array
 * - Function arguments
 * @param name Symbol name
 * @return The symbol value or 'null'.
 */
Ref<JSValue> FunctionScope::get(const std::string& name)const
{
    if (name == "this")
        return m_this;
    else if (name == "arguments")
        return m_arguments;
    else
    {
        const int index = paramIndex(name);

        if (index >= 0)
            return m_arguments->getAt(index);
        else
            error ("'%s' is undefined", name.c_str());
    }
    
    return jsNull();
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
    else
    {
        const int index = paramIndex(name);

        if (index >= 0)
            m_arguments->setAt(index, value);
        else
            error ("'%s' is undefined", name.c_str());
    }

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

    return jsNull();
}

// GlobalScope
//
//////////////////////////////////////////////////

/**
 * Default constructor.
 */
GlobalScope::GlobalScope() : m_shared (refFromNew(new SharedVars)), m_sharing(false)
{
}

/**
 * Constructor used in 'share' operation
 * @param shared
 */
GlobalScope::GlobalScope(Ref<SharedVars> shared) : m_shared (shared), m_sharing(true)
{
}

/**
 * Checks if a symbol is defined at current scope.
 * @param name
 * @return 
 */
bool GlobalScope::isDefined(const std::string& name)const
{
    return m_notShared.find(name) != m_notShared.end() || 
            m_shared->vars.find(name) != m_shared->vars.end();
}

/**
 * Looks for a symbol in current scope.
 * 
 * @param name  Symbol name
 * @return Value for the symbol
 */
Ref<JSValue> GlobalScope::get(const std::string& name)const
{
    auto it = m_notShared.find(name);

    if (it != m_notShared.end())
        return it->second.value();
    else
    {
        it = m_shared->vars.find(name);
        if (it != m_shared->vars.end())
            return it->second.value();
        else
        {
            error ("'%s' is not defined", name.c_str());
            return jsNull();
        }
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
    deleteVar(name);

    return newVar(name, value, false);
}

/**
 * Deletes a variable from global scope.
 * @param name
 * @return 
 */
Ref<JSValue> GlobalScope::deleteVar(const std::string& name)
{
    auto            it = m_shared->vars.find(name);
    Ref<JSValue>    value;
    
    if (it != m_shared->vars.end())
    {
        copyOnWrite();
        value = checkedVarDelete(m_shared->vars, name);
    }
    else 
        value = checkedVarDelete(m_shared->vars, name);
    
    return value;
}

/**
 * Creates a new variable at global scope.
 * @param name
 * @param value
 * @param isConst
 * @return 
 */
Ref<JSValue> GlobalScope::newVar(const std::string& name, Ref<JSValue> value, bool isConst)
{
    if (isDefined(name))
        deleteVar (name);
    
    if (value->getMutability() == MT_DEEPFROZEN)
    {
        copyOnWrite();
        checkedVarWrite(m_shared->vars, name, value, isConst);
    }
    else
        checkedVarWrite(m_notShared, name, value, isConst);

    return value;
}

/**
 * Creates a new variable at global scope, which cannot be shared when sharing
 * global scope with other actors.
 * @param name
 * @param value
 * @param isConst
 * @return 
 */
void GlobalScope::newNotSharedVar(const std::string& name, Ref<JSValue> value, bool isConst)
{
    checkedVarWrite(m_notShared, name, value, isConst);
}

/**
 * Generates an object which contains all symbols defined at global scope.
 * @return 
 */
Ref<JSObject> GlobalScope::toObject()
{
    auto result = JSObject::create();
    
    for (auto it = m_notShared.begin(); it != m_notShared.end(); ++it)
        result->writeField(it->first, it->second.value(), false);
    
    for (auto it = m_shared->vars.begin(); it != m_shared->vars.end(); ++it)
        result->writeField(it->first, it->second.value(), false);
    
    return result;
}

/**
 * Creates a new global scope which shares 'deep-frozen' objects with this
 * scope. 
 * 
 * It only shares objects created prior to the call to 'share'
 * 
 * @return 
 */
Ref<GlobalScope> GlobalScope::share()
{
    auto newScope = new GlobalScope(this->m_shared);
    
    this->m_sharing = true;
    return refFromNew(newScope);
}

/**
 * Makes a copy of the shareable variables if they are being shared, because
 * shareable variable set is going to be modified.
 */
void GlobalScope::copyOnWrite()
{
    if (m_sharing)
    {
        auto copy = refFromNew(new SharedVars);
        copy->vars = m_shared->vars;
        m_shared = copy;
        m_sharing = false;
    }
}

//Global symbols global variable.
static Ref<IScope>     s_globals;

Ref<IScope> getGlobals()
{
    return s_globals;
}

GlobalsSetter::GlobalsSetter (Ref<IScope> newGlobals) : m_oldGlobals(s_globals)
{
    s_globals = newGlobals;
}

GlobalsSetter::~GlobalsSetter()
{
    s_globals = m_oldGlobals;
}
