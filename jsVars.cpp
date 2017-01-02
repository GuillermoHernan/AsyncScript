/* 
 * File:   JsVars.cpp
 * Author: ghernan
 * 
 * Javascript variables and values implementation code
 *
 * Created on November 21, 2016, 7:24 PM
 */

#include "jsVars.h"
#include "OS_support.h"
#include "utils.h"
#include "TinyJS_Functions.h"

#include <cstdlib>
#include <math.h>

using namespace std;

//Default prototypes
Ref<JSObject>   JSObject::DefaultPrototype;
Ref<JSObject>   JSString::DefaultPrototype;
Ref<JSObject>   JSArray::DefaultPrototype;
Ref<JSObject>   JSFunction::DefaultPrototype;

Ref<JSValue> JSValue::readFieldStr(const std::string& strKey)const
{
    return readField (jsString(strKey));
}    

Ref<JSValue> JSValue::writeFieldStr(const std::string& strKey, Ref<JSValue> value)
{
    return writeField (jsString(strKey), value);
}


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

/**
 * Compares two javascript values.
 * @param a
 * @param b
 * @return 
 */
double jsValuesCompare (Ref<JSValue> a, Ref<JSValue> b)
{
    auto typeA = a->getType();
    auto typeB = b->getType();
    
    if (typeA != typeB)
        return typeA - typeB;
    else
    {
        if (typeA <= VT_NULL)
            return 0;
        else if (typeA <= VT_BOOL)
            return a->toDouble() - b->toDouble();
        else if (typeA == VT_STRING)
            return a->toString().compare( b->toString() );
        else
            return a.getPointer() - b.getPointer();
    }
}

/**
 * Converts a 'JSValue' into a 32 bit signed integer.
 * If the conversion is not posible, it returns zero. Therefore, the use
 * of 'isInteger' function is advised.
 * @param a
 * @return 
 */
int toInt32 (Ref<JSValue> a)
{
    const double v = a->toDouble();
    
    if (isnan(v))
        return 0;
    else
        return (int)v;
}

/**
 * Converts a value into a 64 bit integer.
 * @param a
 * @return The integer value. In case of failure, it returns the largest 
 * number representable by a 64 bit integer (0xFFFFFFFFFFFFFFFF), which cannot
 * be represented by a double.
 */
unsigned long long toUint64 (Ref<JSValue> a)
{
    const double v = a->toDouble();
    
    if (isnan(v))
        return 0xFFFFFFFFFFFFFFFF;
    else
        return (unsigned long long)v;
}

size_t toSizeT (Ref<JSValue> a)
{
    return (size_t)toUint64(a);
}


/**
 * Checks if a value is an integer number
 * @param a
 * @return 
 */
bool isInteger (Ref<JSValue> a)
{
    const double v = a->toDouble();
    
    return floor(v) == v;
}

/**
 * Checks if a value is a unsigned integer number.
 * @param a
 * @return 
 */
bool isUint (Ref<JSValue> a)
{
    const double v = a->toDouble();
    
    if ( v < 0)
        return false;
    else
        return floor(v) == v;
}

/**
 * Creates a 'deep frozen' copy of an object, making deep frozen copies of 
 * all descendants which are necessary,
 * @param obj
 * @param transformed
 * @return 
 */
Ref<JSValue> deepFreeze(Ref<JSValue> obj, JSValuesMap& transformed)
{
    if (obj.isNull())
        return obj;
    
    if (obj->getMutability() == MT_DEEPFROZEN)
        return obj;
    
    auto it = transformed.find(obj);
    if (it != transformed.end())
        return it->second;

    ASSERT (obj->isObject());
    
    //Clone object
    auto newObject = obj->unFreeze(true).staticCast<JSObject>();
    transformed[obj] = newObject;    
    
    auto    object = obj.staticCast<JSObject>();
    auto    keys = object->getKeys();
    auto    prototype = deepFreeze(object->getPrototype(), transformed);
    newObject->setPrototype ( prototype.staticCast<JSObject>() );
    
    for (size_t i = 0; i < keys.size(); ++i)
    {
        auto key = keys[i];
        auto value = deepFreeze (object->readField (key), transformed);
        newObject->writeField(key, value);
    }

    newObject->m_mutability = MT_DEEPFROZEN;
    
    return newObject;
}

/**
 * Writes to a variable in a variable map. If the variable already exist, and
 * is a constant, it throw an exception.
 * @param map
 * @param name
 * @param value
 * @param isConst   If true, it creates a new constant
 */
void checkedVarWrite (VarMap& map, const std::string& name, Ref<JSValue> value, bool isConst)
{
    auto it = map.find(name);
    
    if (it != map.end() && it->second.isConst())
        error ("Trying to write to constant '%s'", name.c_str());
    
    map[name] = VarProperties(value, isConst);        
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
    return double_to_string(m_value);
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
 * Strings are never mutable. Therefore 'unFreeze' operation returns a reference
 * to the same object.
 * @param forceClone
 * @return 
 */
Ref<JSValue> JSString::unFreeze(bool forceClone)
{
    return Ref<JSValue>(this);
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
 * Member access function overridden to have access to 'length' property, and to
 * have access to individual characters.
 * @param name
 * @return 
 */
Ref<JSValue> JSString::readField(Ref<JSValue> key)const
{
    if (isUint(key))
    {
        const size_t    index = toSizeT(key);
        
        if (index >= m_text.length())
            return undefined();
        else
            return jsString(m_text.substr(index, 1));
    }
    else if (key->toString() == "length")
        return jsDouble (m_text.size());
    else
        return JSObject::readField(key);
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
    return refFromNew(new JSObject(DefaultPrototype, MT_MUTABLE));
}

/**
 * Creates an empty JSON object
 * @return 
 */
Ref<JSObject> JSObject::create(Ref<JSObject> prototype)
{
    return refFromNew(new JSObject(prototype, MT_MUTABLE));
}

/**
 * Creates a frozen copy of an object
 */
Ref<JSValue> JSObject::freeze()
{
    if (isMutable())
        return clone (false);
    else
        return Ref<JSValue>(this);
}

/**
 * Creates a mutable copy of an object
 * @param forceClone
 * @return 
 */
Ref<JSValue> JSObject::unFreeze(bool forceClone)
{
    if (forceClone || !isMutable())
        return clone (true);
    else
        return Ref<JSValue>(this);
}

/**
 * Transforms the object into an immutable object. The transformation is made in
 * place, no copy is performed.
 * Watch-out for side-effects.
 */
void JSObject::setFrozen()
{
    m_mutability = selectMutability(*this, false);
}

/**
 * Gets an with all object keys
 * @return 
 */
std::vector <Ref<JSValue> > JSObject::getKeys()const
{
    std::vector <Ref<JSValue> >     result;
    
    result.reserve(m_members.size());
    for (auto it = m_members.begin(); it != m_members.end(); ++it)
        result.push_back(jsString(it->first));
    
    return result;
}

/**
 * Copy constructor.
 * @param src       Reference to the source object
 * @param _mutable  
 */
JSObject::JSObject(const JSObject& src, bool _mutable)
: m_members (src.m_members)
, m_prototype (src.m_prototype)
, m_mutability (selectMutability(src, _mutable))
{
}


/**
 * Creates a copy of the object
 * @param _mutable   Controls if the copy will be mutable or not. In case of not 
 * being mutable, it will be 'frozen' or 'deepFrozen' depending on the mutability 
 * state of the object members.
 * @return 
 */
Ref<JSObject> JSObject::clone (bool _mutable)
{
    return refFromNew(new JSObject(*this, _mutable));
}

/**
 * Chooses the appropriate mutability state for the new object on a clone operation
 * @param src
 * @param _mutable
 * @return 
 */
JSMutability JSObject::selectMutability(const JSObject& src, bool _mutable)
{
    if (_mutable)
        return MT_MUTABLE;
    else
    {
        if (src.m_prototype.notNull() && src.m_prototype->getMutability() != MT_DEEPFROZEN)
            return MT_FROZEN;
        
        //Check if all children are 'deepfrozen'
        auto items = src.m_members;
        for (auto it = items.begin(); it != items.end(); ++it)
        {
            if (it->second.value()->getMutability() != MT_DEEPFROZEN)
                return MT_FROZEN;
        }
        return MT_DEEPFROZEN;
    }
}

bool JSObject::isWritable(const std::string& key)const
{
    if (!isMutable())
        return false;
    
    auto it = m_members.find(key);
    if (it == m_members.end())
        return true;
    else
        return !it->second.isConst();
}

/**
 * Reads a field of the object. If it does not exist, it returns 'undefined'
 * @param key
 * @return 
 */
Ref<JSValue> JSObject::readField(Ref<JSValue> key)const
{
    auto it = m_members.find(key2Str(key));

    if (it != m_members.end())
        return it->second.value();
    else if (!nullCheck(m_prototype))
        return m_prototype->readField (key);
    else
        return undefined();
}

/**
 * Sets the value of a member, or creates it if not already present.
 * @param name
 * @param value
 * @return 
 */
Ref<JSValue> JSObject::writeField(Ref<JSValue> key, Ref<JSValue> value)
{
    const string keyStr = key2Str(key);
    
    if (!isWritable(keyStr))
        return readField(key);
    
    m_members[keyStr] = VarProperties(value, false);

    return value;
}

/**
 * Creates a new constant field
 * @param name
 * @param value
 * @return 
 */
Ref<JSValue> JSObject::newConstField(Ref<JSValue> key, Ref<JSValue> value)
{
    const string keyStr = key2Str(key);
    
    if (!isWritable(keyStr))
        return readField(key);
    
    m_members[keyStr] = VarProperties(value, true);

    return value;
}

/**
 * Deletes a field from the object
 * @param key
 * @return 
 */
Ref<JSValue> JSObject::deleteField(Ref<JSValue> key)
{
    const string keyStr = key2Str(key);
    
    if (!isWritable(keyStr))
        return readField(key);

    auto it = m_members.find(keyStr);
    
    if (it == m_members.end())
        return undefined();
    else
    {
        auto result = it->second.value();
        m_members.erase(it);
        return result;
    }
}


/**
 * Transforms a 'JSValue' into a string which can be used to search the store.
 * @param key
 * @return 
 */
std::string JSObject::key2Str(Ref<JSValue> key)
{
    if (!key->isPrimitive())
        error ("Invalid array index: %s", key->toString().c_str());
    else if (key->getType() == VT_NUMBER)
        return double_to_string(key->toDouble());
    else
        return key->toString();
    
    return "";
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
    bool first = true;

    //{"x":2}
    output << "{";

    for (auto it = m_members.begin(); it != m_members.end(); ++it)
    {
        string childJSON = it->second.value()->getJSON(indent+1);

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
    if (!isMutable())
        return m_length;
    
    //TODO: String conversion may be more efficient.
    this->writeField(jsInt(m_length++), value);
    return m_length;
}

/**
 * Gets an element located at an array position
 * @param index
 * @return 
 */
Ref<JSValue> JSArray::getAt(size_t index)const
{
    return readField (jsDouble(index));
}


/**
 * JSArray 'get' override, to implement 'length' property reading.
 * @param name
 * @param exception
 * @return 
 */
Ref<JSValue> JSArray::readField(Ref<JSValue> key)const
{
    if (key->toString() == "length")
        return jsInt(m_length);
    else
        return JSObject::readField(key);
}

/**
 * Array 'writeField' method override. Handles:
 * - Length overwrite, which changes array length
 * - Write an element past the last one, which enlarges the array.
 * @param key
 * @param value
 * @return 
 */
Ref<JSValue> JSArray::writeField(Ref<JSValue> key, Ref<JSValue> value)
{
    if (!isMutable())
        return readField(key);
    
    if (key->toString() == "length")
    {
        setLength(value);
        return key;
    }
    else
    {
        value = JSObject::writeField(key, value);
        if (isUint(key))
        {
            const size_t index = toSizeT(key);

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
    return scArrayJoin(Ref<JSArray>(const_cast<JSArray*>(this)), jsString(","));
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

        const std::string childJSON = this->readField(jsInt(i))->getJSON(indent);

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

JSArray::JSArray(const JSArray& src, bool _mutable)
: JSObject(src, _mutable)
, m_length (src.m_length)
{    
}

/**
 * 'JSArray' clone operation.
 * @param _mutable
 * @return 
 */
Ref<JSObject> JSArray::clone (bool _mutable)
{
    return refFromNew (new JSArray(*this, _mutable));
}

/**
 * Modifies array length
 * @param value
 */
void JSArray::setLength(Ref<JSValue> value)
{
    if (!isUint(value))
        error ("Invalid array index: %s", value->toString().c_str());
    
    //TODO: Not fully standard compliant
    const size_t length = toSizeT(value);

    for (size_t i = length; i < m_length; ++i)
        this->deleteField(jsDouble(i));

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


JSFunction::JSFunction(const std::string& name, JSNativeFn pNative) :
    JSObject(DefaultPrototype, MT_MUTABLE),
    m_name(name),
    m_pNative(pNative)
{
    //Prototype object, used when the function acts as a constructor.
    writeField(jsString("prototype"), JSObject::create());
}


JSFunction::~JSFunction()
{
//    printf ("Destroying function: %s\n", m_name.c_str());
}

JSFunction::JSFunction(const JSFunction& src, bool _mutable)
: JSObject(src, _mutable)
, m_name (src.m_name)
, m_codeMVM (src.m_codeMVM)
, m_pNative (src.m_pNative)
, m_params (src.m_params)
{    
}

Ref<JSObject> JSFunction::clone (bool _mutable)
{
    return refFromNew (new JSFunction(*this, _mutable));
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
