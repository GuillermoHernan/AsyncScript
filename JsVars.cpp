/* 
 * File:   JsVars.cpp
 * Author: ghernan
 * 
 * Javascript variables and values implementation code
 *
 * Created on November 21, 2016, 7:24 PM
 */

#include "JsVars.h"
#include "OS_support.h"
#include "utils.h"

#include <cstdlib>

using namespace std;

//Default prototypes
Ref<JSObject>   JSObject::DefaultPrototype;
Ref<JSObject>   JSString::DefaultPrototype;
Ref<JSObject>   JSArray::DefaultPrototype;
Ref<JSObject>   JSFunction::DefaultPrototype;
//Ref<JSObject>   JSObject::DefaultPrototype;

/**
 * Gives an string representation of the type name
 * @return 
 */
std::string getTypeName(JSValueTypes vType)
{
    typedef map<JSValueTypes, string> TypesMap;
    static TypesMap types;

    if (types.empty())
    {
        types[VT_UNDEFINED] = "undefined";
        types[VT_NULL] = "null";
        types[VT_NUMBER] = "Number";
        types[VT_BOOL] = "Boolean";
        types[VT_STRING] = "String";
        types[VT_OBJECT] = "Object";
        types[VT_ARRAY] = "Array";
        types[VT_FUNCTION] = "Function";
    }

    ASSERT(types.find(vType) != types.end());
    return types[vType];
}

/**
 * Class for 'undefined' values.
 */
class JSUndefined: public JSValueBase<VT_UNDEFINED>
{
public:
    virtual std::string toString()const
    {
        return "undefined";
    }
};

/**
 * Class for 'null' values.
 */
class JSNull: public JSValueBase<VT_NULL>
{
public:
    virtual std::string toString()const
    {
        return "null";
    }
};

/**
 * Gets the 'undefined value.
 * @return 
 */
Ref<JSValue> undefined()
{
    static Ref<JSValue> value = refFromNew(new JSUndefined);
    return value;
}

Ref<JSValue> jsNull()
{
    static Ref<JSValue> value = refFromNew(new JSNull);
    return value;
}

Ref<JSBool> jsTrue()
{
    static Ref<JSBool> value = refFromNew(new JSBool(true));
    return value;
}

Ref<JSBool> jsFalse()
{
    static Ref<JSBool> value = refFromNew(new JSBool(false));
    return value;
}

Ref<JSValue> jsBool(bool value)
{
    if (value)
        return jsTrue();
    else
        return jsFalse();
}

Ref<JSValue> jsInt(int value)
{
    return JSNumber::create(value);
}

Ref<JSValue> jsDouble(double value)
{
    return JSNumber::create(value);
}

Ref<JSValue> jsString(const std::string& value)
{
    return JSString::create(value);
}

/**
 * Class for numeric constants. 
 * It also stores the original string representation, to have an accurate string 
 * representation
 */
class JSNumberConstant : public JSNumber
{
public:

    static Ref<JSNumberConstant> create(const std::string& text)
    {
        if (text.size() > 0 && text[0] == '0' && isOctal(text))
        {
            const unsigned value = strtoul(text.c_str()+1, NULL, 8);
            
            return refFromNew(new JSNumberConstant(value, text));
        }
        else
        {
            const double value = strtod(text.c_str(), NULL);

            return refFromNew(new JSNumberConstant(value, text));
        }
    }

    virtual std::string toString()const
    {
        return m_text;
    }

private:

    JSNumberConstant(double value, const std::string& text)
    : JSNumber(value), m_text(text)
    {
    }

    string m_text;
};

Ref<JSValue> createConstant(CScriptToken token)
{
    if (token.type() == LEX_STR)
        return JSString::create(token.strValue());
    else
        return JSNumberConstant::create(token.text());
}

/**
 * Gets a member form a scope, and ensures it is an object.
 * If not an object, returns NULL
 * @param pScope
 * @param name
 * @return 
 */
Ref<JSObject> getObject(Ref<IScope> pScope, const std::string& name)
{
    Ref<JSValue> value = pScope->get(name);
    
    if (!value.isNull())
    {
        if (value->isObject())
            return value.staticCast<JSObject>();
    }

    return Ref<JSObject>();
}

/**
 * Transforms a NULL pointer into an undefined value. If not undefined,
 * just returns the input value.
 * @param value input value to check
 * @return 
 */
Ref<JSValue> null2undef (Ref<JSValue> value)
{
    if (value.isNull())
        return undefined();
    else
        return value;
}

/**
 * Checks if the object has any kind of 'null' states: internal NULL pointer,
 * Javascript null, Javascript undefined.
 * @param value
 * @return 
 */
bool nullCheck (Ref<JSValue> value)
{
    if (value.isNull())
        return true;
    else
        return value->isNull();
}





// JSNumber
//
//////////////////////////////////////////////////

/**
 * Construction function
 * @param value
 * @return 
 */
Ref<JSNumber> JSNumber::create(double value)
{
    return refFromNew(new JSNumber(value));
}

std::string JSNumber::toString()const
{
    //TODO: Review the standard. Find about number to string conversion formats.
    return to_string(m_value);
}

// JSString
//
//////////////////////////////////////////////////

/**
 * Construction function
 * @param value
 * @return 
 */
Ref<JSString> JSString::create(const std::string & value)
{
    return refFromNew(new JSString(value));
}

/**
 * Tries to transform the string into a int32
 * @return 
 */
int JSString::toInt32()const
{
    return strtol(m_text.c_str(), NULL, 0);
}

/**
 * Tries to transform a string into a double value
 * @return 
 */
double JSString::toDouble()const
{
    const double result = strtod(m_text.c_str(), NULL);
    
    if (result == 0 && !isNumber(m_text))
        return getNaN();
    else
        return result;
}

/**
 * Gets the JSON representation of a string
 * @param indent
 * @return 
 */
std::string JSString::getJSON(int indent)
{
    return escapeString(m_text, true);
}


/**
 * Member access function overridden to have access to 'length' property.
 * @param name
 * @return 
 */
Ref<JSValue> JSString::get(const std::string& name)const
{
    if (name == "length")
        return jsDouble (m_text.size());
    else
        return JSObject::get(name);
}


// JSObject
//
//////////////////////////////////////////////////

JSObject::~JSObject()
{
    //printf ("Destroying object: %s\n", this->getJSON(0).c_str());
}

/**
 * Creates an empty JSON object, with default prototype
 * @return 
 */
Ref<JSObject> JSObject::create()
{
    return refFromNew(new JSObject(DefaultPrototype));
}

/**
 * Creates an empty JSON object
 * @return 
 */
Ref<JSObject> JSObject::create(Ref<JSObject> prototype)
{
    return refFromNew(new JSObject(prototype));
}

/**
 * Gets the value of a member.
 * @return 
 */
Ref<JSValue> JSObject::get(const std::string& name)const
{
    MembersMap::const_iterator it = m_members.find(name);

    if (it != m_members.end())
        return it->second;
    else if (!nullCheck(m_prototype))
        return m_prototype->get (name);
    else
        return undefined();
}

/**
 * Sets the value of a member, or creates it if not already present.
 * @param name
 * @param value
 * @return 
 */
Ref<JSValue> JSObject::set(const std::string& name, Ref<JSValue> value)
{
    if (m_frozen)
        return get(name);
    
    if (value->isUndefined())
    {
        //'undefined' means not present in the object. So it is deleted.
        m_members.erase(name);
    }
    else
        m_members[name] = value;

    return value;

}

/**
 * Member access. Returns a reference, in order to be able to modify the object.
 * @param name  member name
 * @return 
 */
Ref<JSValue> JSObject::memberAccess(const std::string& name)
{
    return get(name);
}

std::string indentText(int indent)
{
    std::string result;
    
    result.reserve(indent * 2);
    
    for (int i=0; i < indent; ++i)
        result += "  ";
    
    return result;
}

/**
 * Generates a JSON representation of the object
 * @return JSON string
 */
std::string JSObject::getJSON(int indent)
{
    ostringstream output;
    MembersMap::const_iterator it;
    bool first = true;

    //{"x":2}
    output << "{";

    for (it = m_members.begin(); it != m_members.end(); ++it)
    {
        string childJSON = it->second->getJSON(indent+1);

        if (!childJSON.empty())
        {
            if (!first)
                output << ",";
            else
                first = false;

            output << "\n" << indentText(indent+1) << "\"" << it->first << "\":";
            output << childJSON;
        }
    }

    if (!first)
        output << "\n" << indentText(indent) << "}";
    else
        output << "}";

    return output.str();
}


// JSArray
//
//////////////////////////////////////////////////

/**
 * Creates an array object in the heap.
 * @return 
 */
Ref<JSArray> JSArray::create()
{
    return refFromNew(new JSArray);
}

/**
 * Creates an array of a given initial length.
 * All elements in the array will be set to 'undefined'
 * @param size
 * @return 
 */
Ref<JSArray> JSArray::create(size_t size)
{
    Ref<JSArray>    a = refFromNew(new JSArray);
    
    a->m_length = size;
    
    return a;
}

/**
 * Translates a C++ string vector into a Javascript string array
 * @param strList
 * @return 
 */
Ref<JSArray> JSArray::createStrArray(const std::vector<std::string>& strList)
{
    Ref<JSArray>    arr = JSArray::create();
    
    for (size_t i=0; i < strList.size(); ++i)
        arr->push(jsString(strList[i]));
    
    return arr;
}


/**
 * Adds a value to the end of the array
 * @param value
 * @return Returns new array size
 */
size_t JSArray::push(Ref<JSValue> value)
{
    //TODO: String conversion may be more efficient.
    this->set(jsInt(m_length++)->toString(), value);
    return m_length;
}

/**
 * JSArray 'get' override, to implement 'length' property read.
 * @param name
 * @param exception
 * @return 
 */
Ref<JSValue> JSArray::get(const std::string& name)const
{
    if (name == "length")
        return jsInt(m_length);
    else
        return JSObject::get(name);
}

/**
 * Array 'set' method override. Handles:
 * - Length overwrite, which changes array length
 * - Write an element past the last one, which enlarges the array.
 * @param name
 * @param value
 * @return 
 */
Ref<JSValue> JSArray::set(const std::string& name, Ref<JSValue> value, bool forceLocal)
{
    if (name == "length")
    {
        setLength(value);
        return value;
    }
    else
    {
        value = JSObject::set(name, value);
        if (isNumber(name))
        {
            const size_t index = strtoul(name.c_str(), NULL, 10);

            m_length = max(m_length, index + 1);
        }

        return value;
    }
}

/**
 * String representation of the array
 * @return 
 */
std::string JSArray::toString()const
{
    //TODO: This is basically 'String.join()' implementation. We should share code.

    ostringstream output;
    for (size_t i = 0; i < m_length; ++i)
    {
        if (i > 0)
            output << ',';

        const Ref<JSValue>  val = this->get(to_string(i));
        
        if (!val.isNull())
            output << val->toString();
    }

    return output.str();
}

/**
 * Writes a JSON representation of the array to the output
 * @param output
 */
std::string JSArray::getJSON(int indent)
{
    std::ostringstream output;
    const bool multiLine = m_length > 4;

    output << '[';

    for (size_t i = 0; i < m_length; ++i)
    {
        if (multiLine)
            output << "\n" << indentText(indent + 1);
        
        if (i > 0)
            output << ',';

        const std::string childJSON = this->arrayAccess(jsInt(i))->getJSON(indent);

        if (childJSON.empty())
            output << "null";
        else
            output << childJSON;
    }
    
    if (multiLine)
            output << "\n" << indentText(indent);
    output << ']';

    return output.str();
}

/**
 * Modifies array length
 * @param value
 */
void JSArray::setLength(Ref<JSValue> value)
{
    //TODO: Not fully standard compliant
    const size_t length = (size_t) value->toInt32();

    for (size_t i = length; i < m_length; ++i)
        this->set(to_string(i), undefined());

    m_length = length;
}



// JSFunction
//
//////////////////////////////////////////////////

/**
 * Creates a Javascript function 
 * @param name Function name
 * @return A new function object
 */
Ref<JSFunction> JSFunction::createJS(const std::string& name)
{
    return refFromNew(new JSFunction(name, NULL));
}

/**
 * Creates an object which represents a native function
 * @param name  Function name
 * @param fnPtr Pointer to the native function.
 * @return A new function object
 */
Ref<JSFunction> JSFunction::createNative(const std::string& name, JSNativeFn fnPtr)
{
    return refFromNew(new JSFunction(name, fnPtr));
}

JSFunction::~JSFunction()
{
//    printf ("Destroying function: %s\n", m_name.c_str());
}


/**
 * String representation of the function.
 * Just the function and the parameter list, no code.
 * @return 
 */
std::string JSFunction::toString()const
{
    ostringstream output;

    //TODO: another time a code very like to 'String.Join'. But sadly, each time
    //read from a different container.

    output << "function " << getName() << " (";

    for (size_t i = 0; i < m_params.size(); ++i)
    {
        if (i > 0)
            output << ',';

        output << m_params[i];
    }

    output << ")";
    return output.str();
}


// BlockScope
//
//////////////////////////////////////////////////

/**
 * Looks for a symbol in current scope.
 * If not found, it look in the parent scope. If is not found in any scope,
 * it returns 'undefined'
 * 
 * @param name  Symbol name
 * @return Value for the symbol or 'undefined'
 */
Ref<JSValue> BlockScope::get(const std::string& name)const
{
    SymbolMap::const_iterator it = m_symbols.find(name);

    if (it != m_symbols.end())
        return it->second;
    else if (!m_pParent.isNull())
        return m_pParent->get(name);
    else
        return Ref<JSValue>();
}

/**
 * Sets a symbol value in current scope.
 * 
 * - If the symbol is already defined in this scope, it is overwritten.
 * - If the symbol is defined in a parent scope, it is not overwritten in its scope,
 * but is also defined in this scope, so it shadows the old value while this scope
 * is valid.
 * - If the symbol is not defined, it is created.
 * 
 * @param name      Symbol name
 * @param value     Symbol value.
 * @return Symbol value.
 */
Ref<JSValue> BlockScope::set(const std::string& name, Ref<JSValue> value, bool forceLocal)
{
    if (value->isUndefined())
    {
        if (m_pParent.notNull())
            m_symbols[name] = value; //The symbol will be 'undefined' while this scope is active.
        else
            m_symbols.erase(name); //At global scope, we can delete it.
    }
    else if (m_symbols.find(name) != m_symbols.end())
        m_symbols[name] = value; //Present at current scope
    else if (get(name).isNull())
        m_symbols[name] = value; //Symbol does not exist, create it at this scope.
    else if (forceLocal)
        m_symbols[name] = value;
    else
        return m_pParent->set(name, value); //Set at parent scope

    return value;
}

/**
 * Gets the first function scope down the scope chain.
 * @return Function scope or null if there is no function scope.
 */
Ref<IScope> BlockScope::getFunctionScope()
{
    if (m_pParent.isNull())
        return Ref<IScope>();
    else
        return m_pParent->getFunctionScope();
}


// FunctionScope
//
//////////////////////////////////////////////////

/**
 * Constructor
 * @param globals
 * @param targetFn
 */
FunctionScope::FunctionScope(Ref<IScope> globals, Ref<JSFunction> targetFn) :
m_function(targetFn),
m_globals(globals)
{
    m_this = undefined();
    m_result = undefined();
    m_arguments = JSArray::create();
}

/**
 * Adds a new parameter to the parameter list
 * @param value
 * @return Actual number of parameters
 */
int FunctionScope::addParam(Ref<JSValue> value)
{
    const JSFunction::ParametersList paramsDef = m_function->getParams();
    const size_t index = m_arguments->length();

    if (index < paramsDef.size())
    {
        const std::string &name = paramsDef[index];

        m_params[name] = value;
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
    SymbolsMap::const_iterator it = m_params.find(name);
    
    if (it != m_params.end())
        return it->second;
    else
        return undefined();            
}

/**
 * Gets a value from function scope. It looks for:
 * - this pointer
 * - Function arguments
 * - global variables
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
        SymbolsMap::const_iterator it = m_params.find(name);

        if (it != m_params.end())
            return it->second;
        else
        {
            it = m_locals.find(name);
            if (it != m_locals.end())
                return it->second;
            else if (!m_globals.isNull())
                return m_globals->get(name);
            else
                return undefined();
        }
    }
}

/**
 * Sets a value in function scope.
 * It only lets change values of arguments, and globals
 * @param name
 * @param value
 * @return 
 */
Ref<JSValue> FunctionScope::set(const std::string& name, Ref<JSValue> value, bool forceLocal)
{
    if (name == "this" || name == "arguments")
        throw CScriptException("Invalid left hand side in assignment");
    else if (m_params.find(name) != m_params.end())
        m_params[name] = value;
    else if (forceLocal || m_locals.find(name) != m_locals.end())
        m_locals[name] = value;
    else if (!get(name).isNull())
        return m_globals->set(name, value);

    return value;
}
