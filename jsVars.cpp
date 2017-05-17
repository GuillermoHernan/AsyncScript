/* 
 * File:   JsVars.cpp
 * Author: ghernan
 * 
 * Javascript variables and values implementation code
 *
 * Created on November 21, 2016, 7:24 PM
 */

#include "ascript_pch.hpp"
#include "jsVars.h"
#include "utils.h"
#include "asString.h"
#include "microVM.h"
#include "ScriptException.h"

#include <cstdlib>
#include <math.h>

using namespace std;

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
        types[VT_NULL] = "null";
        types[VT_NUMBER] = "Number";
        types[VT_BOOL] = "Boolean";
        types[VT_CLASS] = "Class";
        types[VT_OBJECT] = "Object";
        types[VT_STRING] = "String";
        types[VT_FUNCTION] = "Function";
        types[VT_CLOSURE] = "Closure";
    }

    ASSERT(types.find(vType) != types.end());
    return types[vType];
}

ASValue jsNull()
{
    static ASValue value;
    return value;
}

ASValue jsTrue()
{
    static ASValue value(true);
    return value;
}

ASValue jsFalse()
{
    static ASValue value(false);
    return value;
}

ASValue jsBool(bool value)
{
    if (value)
        return jsTrue();
    else
        return jsFalse();
}

ASValue jsInt(int value)
{
    return ASValue((double)value);
}

ASValue jsSizeT(size_t value)
{
    return ASValue((double)value);
}


ASValue jsDouble(double value)
{
    return ASValue(value);
}

ASValue jsString(const std::string& value)
{
    auto str = JSString::create(value);
    
    return ASValue(str.getPointer(), VT_STRING);
}

ASValue createConstant(CScriptToken token)
{
    if (token.type() == LEX_STR)
        return jsString(token.strValue());
    else
    {
        string text = token.text();
        
        if (text.size() > 0 && text[0] == '0' && isOctal(text))
        {
            const unsigned value = strtoul(text.c_str() + 1, NULL, 8);

            return jsSizeT(value);
        }
        else
        {
            const double value = strtod(text.c_str(), NULL);

            return jsDouble(value);
        }
        
    }
}

/**
 * Returns an iterator which points to a list which just contains
 * the given item
 * @param v
 * @return 
 */
ASValue singleItemIterator(ASValue v)
{
    //TODO: Implement as specified.
    return jsNull();    
}

/**
 * Compares two values, taking into account its types.
 * @param b
 * @param ec
 * @return 
 */
double ASValue::typedCompare(const ASValue& b, ExecutionContext* ec)const
{
    auto typeA = this->getType();
    auto typeB = b.getType();

    if (typeA != typeB)
        return typeA - typeB;
    else
        return compare(b, ec);
}


/**
 * Compares two values.
 * @param b
 * @return 
 */
double ASValue::compare(const ASValue& b, ExecutionContext* ec)const
{
    auto typeA = this->getType();
    auto typeB = b.getType();
    auto minType = min (typeA, typeB);
    
    switch (minType)
    {
    case VT_NULL:
        return typeA - typeB;
        
    case VT_BOOL:
        return int(this->toBoolean(ec)) - int(b.toBoolean(ec));
        
    case VT_NUMBER:
        return this->toDouble(ec) - b.toDouble(ec);
        
    default:
        break;
    }

    if (typeA == VT_OBJECT || typeB == VT_STRING)
        return this->staticCast<JSObject>()->compare(b, ec);
    else
        return double(this->m_content.ptr - b.m_content.ptr);
}

/**
 * Converts a 'JSValue' into a 32 bit signed integer.
 * If the conversion is not posible, it returns zero. Therefore, the use
 * of 'isInteger' function is advised.
 * @param a
 * @return 
 */
int ASValue::toInt32()const
{
    if (m_type != VT_NUMBER)
        return 0;
    else
        return (int) m_content.number;
}

/**
 * Converts a value into a 64 bit integer.
 * @param a
 * @return The integer value. In case of failure, it returns the largest 
 * number representable by a 64 bit integer (0xFFFFFFFFFFFFFFFF), which cannot
 * be represented by a double.
 */
unsigned long long ASValue::toUint64()const
{
    if (m_type != VT_NUMBER)
        return 0xFFFFFFFFFFFFFFFF;
    else
        return (unsigned long long) m_content.number;
}

size_t ASValue::toSizeT()const
{
    return (size_t) toUint64();
}

/**
 * Checks if a value is an integer number
 * @param a
 * @return 
 */
bool ASValue::isInteger()const
{
    if (m_type != VT_NUMBER)
        return false;
    else
    {
        double v = m_content.number;
        return floor(v) == v;
    }
}

/**
 * Checks if a value is a unsigned integer number.
 * @param a
 * @return 
 */
bool ASValue::isUint()const
{
    return isInteger() && m_content.number >= 0;
}

// JSFunction
//
//////////////////////////////////////////////////

/**
 * Creates a Javascript function 
 * @param name Function name
 * @return A new function object
 */
Ref<JSFunction> JSFunction::createJS(const std::string& name,
                                     const StringVector& params,
                                     Ref<RefCountObj> code)
{
    return refFromNew(new JSFunction(name, params, code));
}

/**
 * Creates an object which represents a native function
 * @param name  Function name
 * @param fnPtr Pointer to the native function.
 * @return A new function object
 */
Ref<JSFunction> JSFunction::createNative(const std::string& name,
                                         const StringVector& params,
                                         JSNativeFn fnPtr)
{
    return refFromNew(new JSFunction(name, params, fnPtr));
}

JSFunction::JSFunction(const std::string& name,
                       const StringVector& params,
                       JSNativeFn pNative) :
m_name(name),
m_params(params),
m_pNative(pNative)
{
}

JSFunction::JSFunction(const std::string& name,
                       const StringVector& params,
                       Ref<RefCountObj> code) :
m_name(name),
m_params(params),
m_codeMVM(code),
m_pNative(NULL)
{
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


// JSClosure
//
//////////////////////////////////////////////////


Ref<JSClosure> JSClosure::create (Ref<JSFunction> fn, const ASValue* first, size_t count)
{
    return refFromNew (new JSClosure(fn, first, count));
}

JSClosure::JSClosure (Ref<JSFunction> fn, const ASValue* first, size_t count)
    : m_fn(fn), 
    m_params(first+1, first + count),
    m_env(*first), 
    m_mutability (selectMutability(first+1, count -1))  
{
    ASSERT (count > 0);
}

/**
 * Constructor used during 'deepFreeze' operation. Is private.
 * @param fn
 * @param env
 */
JSClosure::JSClosure (Ref<JSFunction> fn, ASValue env)
    : m_fn(fn), 
    m_env(env), 
    m_mutability (MT_DEEPFROZEN)  
{
}

std::string JSClosure::toString()const
{
    return "(clos) " + getFunction()->toString();
}

/**
 * Creates a deep-frozen copy of a closure, if it is not already deep-frozen
 * @param transformed
 * @return 
 */
ASValue JSClosure::deepFreeze(ValuesMap& transformed)const
{
    if (m_mutability == MT_DEEPFROZEN)
        return const_cast<JSClosure*>(this)->value();
    else
    {
        Ref<JSClosure>  newCl = refFromNew(new JSClosure(m_fn, m_env));
        
        for (const auto& param : m_params)
            newCl->m_params.push_back(param.deepFreeze(transformed));
        
        return newCl->value();
    }
}

/**
 * Field reads on closures are redirected to environment attribute.
 */
ASValue JSClosure::readField(const std::string& key)
{
    return m_env.readField(key);
}

/**
 * Field writes are redirected to environment attribute.
 * @param key
 * @param value
 * @param isConst
 * @return 
 */
ASValue JSClosure::writeField(const std::string& key, ASValue value, bool isConst)
{
    return m_env.writeField(key, value, isConst);
}

/**
 * Reads one of the parameters
 * @param index
 * @return 
 */
ASValue JSClosure::getAt(ASValue index)
{
    if (!index.isUint())
        return jsNull();
    
    size_t stIndex = index.toSizeT();
    
    if (stIndex >= m_params.size())
        return jsNull();
    else
        return m_params.at(stIndex);    
}

/**
 * Selects which mutability will have a closure, depending on its contents
 * @param first
 * @param count
 * @return 
 */
JSMutability JSClosure::selectMutability (const ASValue* first, size_t count)
{
    for (size_t i = 0; i < count; ++i){
        if (first[i].getMutability() != MT_DEEPFROZEN)
            return MT_FROZEN;
    }
    
    return MT_DEEPFROZEN;
}

// ASValue
//
//////////////////////////////////////////////////

ASValue::ASValue () : m_type(VT_NULL)
{
    m_content.ptr = NULL;
}

ASValue::ASValue (double number) : m_type(VT_NUMBER)
{
    m_content.number = number;
}

ASValue::ASValue (bool value) : m_type(VT_BOOL)
{
    m_content.boolean = value;
}

ASValue::ASValue (RefCountObj* ptr, JSValueTypes type) : m_type(type)
{
    ASSERT (type >= VT_CLASS);
    
    m_content.ptr = ptr;
    ptr->addref();
}

ASValue::~ASValue()
{
    setNull();
}

void ASValue::setNull()
{
    if (m_type >= VT_CLASS)
        m_content.ptr->release();
    
    m_type = VT_NULL;
    m_content.ptr = NULL;
}


ASValue::ASValue (const ASValue& src) : m_content (src.m_content), m_type(src.m_type)
{
    if (src.getType() >= VT_CLASS)
        src.m_content.ptr->addref();
}

ASValue& ASValue::operator=(const ASValue& src)
{
    if (src.getType() >= VT_CLASS)
        src.m_content.ptr->addref();
    
    if (m_type >= VT_CLASS)
        m_content.ptr->release();

    m_type = src.getType();
    m_content = src.m_content;
    
    return *this;
}

/**
 * Gets the mutability of a value
 * @return 
 */
JSMutability ASValue::getMutability()const
{
    switch (m_type)
    {
    case VT_OBJECT:     return staticCast<JSObject>()->getMutability();
    case VT_CLOSURE:    return staticCast<JSClosure>()->getMutability();
    default:            return MT_DEEPFROZEN;
    }
}

bool ASValue::isMutable()const
{
    return getMutability() == MT_MUTABLE;
}

ASValue ASValue::freeze()const
{
    switch (m_type)
    {
    case VT_OBJECT:     return staticCast<JSObject>()->freeze();
    default:            return *this;
    }
}

/**
 * Creates a 'deep frozen' copy of an object, making deep frozen copies of 
 * all descendants which are necessary,
 * @return 
 */
ASValue ASValue::deepFreeze()const
{
    ValuesMap   tmpMap;
    return deepFreeze (tmpMap);
}

ASValue ASValue::deepFreeze(ValuesMap& transformed)const
{
    switch (m_type)
    {
    case VT_OBJECT:     return staticCast<JSObject>()->deepFreeze(transformed);
    case VT_CLOSURE:    return staticCast<JSClosure>()->deepFreeze(transformed);
    default:            return *this;
    }
}

ASValue ASValue::unFreeze(bool forceClone)const
{
    if (m_type != VT_OBJECT)
        return *this;
    else
        return staticCast<JSObject>()->unFreeze(forceClone);    
}

/**
 * Casts a value into a string.
 * @param ec
 * @return 
 */
string ASValue::toString(ExecutionContext* ec)const
{
    switch (m_type)
    {
    case VT_NULL:   return "null";
    case VT_NUMBER: return double_to_string (m_content.number);
    case VT_BOOL:   return m_content.boolean ? "true" : "false";
    case VT_CLASS: 
        return staticCast<JSClass>()->toString();
    case VT_OBJECT: 
        return staticCast<JSObject>()->toString(ec);
    case VT_STRING: 
        return staticCast<JSString>()->toString();
    case VT_FUNCTION: 
        return staticCast<JSFunction>()->toString();
    case VT_CLOSURE: 
        return staticCast<JSClosure>()->toString();
    
    default:
        return "[Unknown value type]";
    }
}

/**
 * Casts a value into a boolean.
 * @param ec
 * @return 
 */
bool ASValue::toBoolean(ExecutionContext* ec)const
{
    switch (m_type)
    {
    case VT_NULL:   return false;
    case VT_NUMBER: return m_content.number != 0;
    case VT_BOOL:   return m_content.boolean;
    case VT_STRING: return !toString(ec).empty();
    case VT_OBJECT: 
        return staticCast<JSObject>()->toBoolean(ec);
    default:        return true;
    }
}


/**
 * Casts a value into a double precision number.
 * @param ec
 * @return 
 */
double ASValue::toDouble(ExecutionContext* ec)const
{
    switch (m_type)
    {
    case VT_NUMBER: return m_content.number;
    case VT_BOOL:   return m_content.boolean ? 1 : 0;
    case VT_STRING: return staticCast<JSString>()->toDouble();
    case VT_OBJECT:
        return staticCast<JSObject>()->toDouble(ec);
    default:        
        return getNaN();
    }
}

/**
 * Reads a field ('.' operator)
 * @param key
 * @return 
 */
ASValue ASValue::readField(const std::string& key)const
{
    switch (m_type)
    {
    case VT_CLASS:
        return staticCast<JSClass>()->readField(key);
        
    case VT_STRING:
    case VT_OBJECT: 
        return staticCast<JSObject>()->readField(key);
    case VT_CLOSURE:
        return staticCast<JSClosure>()->readField(key);
    default:        
        return jsNull();
    }    
}

/**
 * Writes a field ('.' operator)
 * @param key
 * @param value
 * @param isConst
 * @return 
 */
ASValue ASValue::writeField(const std::string& key, ASValue value, bool isConst)
{
    switch (m_type)
    {
    case VT_OBJECT:     return staticCast<JSObject>()->writeField(key, value, isConst);
    case VT_CLOSURE:    return staticCast<JSClosure>()->writeField(key, value, isConst);
    default:            return jsNull();
    }
}

/**
 * Deletes a field.
 * @param key
 * @return 
 */
ASValue ASValue::deleteField(const std::string& key)
{
    if (m_type == VT_OBJECT)
        return staticCast<JSObject>()->deleteField(key);
    else
        return jsNull();
}

/**
 * Gets the list of fields of an object
 * @param inherited
 * @return 
 */
StringSet ASValue::getFields(bool inherited)const
{
    static StringSet empty;

    switch (m_type)
    {
    case VT_CLASS:
        return staticCast<JSClass>()->getFields(inherited);
        
    case VT_STRING:
    case VT_OBJECT: 
        return staticCast<JSObject>()->getFields(inherited);
    default:        
        return empty;
    }    
}

/**
 * Implements array access operator, for writes.
 * @param index
 * @param ctx
 * @return 
 */
ASValue ASValue::getAt(ASValue index, ExecutionContext* ec)const
{
    switch (m_type)
    {
    case VT_STRING:
        return staticCast<JSString>()->getAt(index);
    case VT_OBJECT: 
        return staticCast<JSObject>()->getAt(index, ec);
    case VT_CLOSURE: 
        return staticCast<JSClosure>()->getAt(index);
    default:        
        return jsNull();
    }    
}

/**
 * Implements array access operator, for writes.
 * @param index
 * @param ctx
 * @return 
 */
ASValue ASValue::setAt(ASValue index, ASValue value, ExecutionContext* ec)const
{
    if (m_type == VT_OBJECT)
        return staticCast<JSObject>()->setAt(index, value, ec);
    else        
        return jsNull();
}

/**
 * Returns an iterator to walk the items contained inside the value.
 * @param ec
 * @return 
 */
ASValue ASValue::iterator(ExecutionContext* ec)const
{
    switch (m_type)
    {
    case VT_NULL:   return *this;
    case VT_OBJECT: return staticCast<JSObject>()->iterator(ec);
    default:
        return singleItemIterator(*this);
    }
}

/**
 * Gets the JSON representation of the value.
 * @param indent
 * @return 
 */
std::string ASValue::getJSON(int indent)const
{
    switch (m_type)
    {
    case VT_NULL:   
        return "null";
    case VT_NUMBER:
    case VT_BOOL:
        return toString();
    case VT_STRING:
    case VT_OBJECT:
        return staticCast<JSObject>()->getJSON(indent);
        
    default:
        return "";
    }
    
}

bool ASValue::operator < (const ASValue& b)const
{
    return typedCompare (b, NULL) < 0;
}

// VarMap
//
//////////////////////////////////////////////////


/**
 * Checks if a given variable is constant by checking its 'const' property.
 * If the map does not contain a variable with the given name, it returns 'false'
 * (the variable can be written)
 * @param name
 * @return 
 */
bool VarMap::isConst (CSTR& name)const
{
    auto it = m_content.find(name + ".const");
    
    if (it == m_content.end())
        return false;
    else
        return it->second.toBoolean(NULL);
}

/**
 * Gets a value from the map.
 * Returns 'null' if the variable does not exist.
 * @param name
 * @return 
 */
ASValue VarMap::getValue (CSTR& name)const
{
    ASValue val;
    
    if (tryGetValue(name, &val))
        return val;
    else
        return jsNull();
}

/**
 * Tries to get a value from the map, which is returned by the second
 * (output) parameter.
 * 
 * @param name  Variable name
 * @param val   Pointer to an 'ASValue' which will receive the value. Cannot be null.
 * @return  'true' if the variable has been found, 'false' if it does not exist.
 */
bool VarMap::tryGetValue (CSTR& name, ASValue* val)const
{
    auto it = m_content.find(name);
    
    if (it != m_content.end())
    {
        *val = it->second;
        return true;
    }
    else
        return false;
}

/**
 * Writes to a variable in a variable map. If the variable already exist, and
 * is a constant, it throws a runtime exception.
 * @param name
 * @param value
 * @param isConst   If true, it creates a new constant
 */
void VarMap::checkedVarWrite(const std::string& name, ASValue value, bool isConst)
{
    if (this->isConst(name))
        rtError("Trying to write to constant '%s'", name.c_str());

    m_content[name] = value;
    
    if (isConst)
        m_content[name + ".const"] = jsTrue();
}

/**
 * Writes to a variable, and returns back the value.
 * 
 * If the variable is constant, it does not modify it and returns the
 * constant value.
 * 
 * @param name
 * @param value
 * @param isConst
 * @return 
 */
ASValue VarMap::varWrite(const std::string& name, ASValue value, bool isConst)
{
    if (this->isConst(name))
        return getValue (name);
    else
    {
        m_content[name] = value;

        if (isConst)
            m_content[name + ".const"] = jsTrue();

        return value;
    }
}

/**
 * Tries to delete a variable. 
 * 
 * If the variable is constant, the operation will have no effect.
 * It will also delete any properties associated to the variable.
 * 
 * @param name
 * @return Value of the deleted (or not deleted) variable. If the variable
 * does not exist, it returns null.
 */
ASValue VarMap::varDelete (CSTR& name)
{
    if (isConst(name))
        return getValue(name);
    
    auto itBegin = m_content.find(name);
    if (itBegin == m_content.end())
        return jsNull();
    
    ASValue result = itBegin->second;
    auto    itEnd = itBegin;
    string  propPrefix = name + ".";
    
    //Find all properties.
    for (++itEnd; itEnd != m_content.end() && startsWith(itEnd->first, propPrefix); ++itEnd);
    
    //delete variable and its properties.
    m_content.erase(itBegin, itEnd);
    
    return result;
}

/**
 * Gets a field property. 
 * If the field has not this property, or does not exist at all, it returns null.
 * 
 * @param name
 * @param propName
 * @return 
 */
ASValue VarMap::getProperty (CSTR& name, CSTR& propName)const
{
    string  key = name + "." + propName;
    auto    it = m_content.find(key);
    
    if (it != m_content.end())
        return it->second;
    else
        return jsNull();
}

/**
 * Sets a property for a variable.
 * Properties can only be written once. The next attempts to write them
 * will be ignored.
 * 
 * @param name      Variable name
 * @param propName  Property name
 * @param propValue Property value
 * @return new property value ('propValue' parameter) or the previous value if the
 *      property already existed.
 */
ASValue VarMap::setProperty (CSTR& name, CSTR& propName, ASValue propValue)
{
    string  key = name + "." + propName;
    auto    it = m_content.find(key);
    
    if (it != m_content.end())
        return it->second;
    else
        return m_content[key] = propValue;
}

/**
 * Applies a transformation to each variable value in the map, an generates a new map
 * with the new values.
 * It also copies associated properties (but does not transform them)
 * @param fn    Variable transform function
 * @return A new map with the transformed variables.
 */
VarMap VarMap::map (ItemFn fn)const
{
    VarMap  result;
    
    forEach ([&](CSTR& name, ASValue val){
        ASValue newVal = fn(name, val);
        result.varWrite(name, newVal, false);
        
        this->forEachProperty(name, [&result, &name](CSTR& propName, ASValue propVal){
           result.setProperty(name, propName, propVal); 
        });
    });
    
    return result;    
}

/**
 * Calls the parameter function for each variable - value pair in the map.
 * @param fn
 */
void VarMap::forEach (VoidItemFn fn)const
{
    for (auto it = m_content.begin(); it != m_content.end(); ++it)
    {
        if (it->first.find('.') == string::npos)
            fn (it->first, it->second);
    }
}

/**
 * Checks if any given variable within the map fulfills the given predicate
 * (the function passed as parameter which returns a boolean value for each 
 * variable - value pair)
 * @param fn
 */
bool VarMap::any (BoolItemFn fn)const
{
    for (auto it = m_content.begin(); it != m_content.end(); ++it)
    {
        if (it->first.find('.') == string::npos)
        {
            if (fn (it->first, it->second))
                return true;
        }
    }
    
    return false;
}

/**
 * Calls the 'fn' function for each property of the given variable.
 * @param varName
 * @param fn
 */
void VarMap::forEachProperty (CSTR& varName, VoidItemFn fn)const
{
    auto it = m_content.find(varName);
    
    if (it == m_content.end())
        return;
    
    string prefix = varName + ".";
    
    for (++it; it != m_content.end() && startsWith (it->first, prefix); ++it)
        fn (it->first.substr(prefix.length()), it->second);
}
