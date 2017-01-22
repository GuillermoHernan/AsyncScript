/* 
 * File:   JsVars.h
 * Author: ghernan
 * 
 * Javascript variables and values implementation code
 *
 * Created on November 21, 2016, 7:24 PM
 */

#pragma once

#include <string>

#include "jsLexer.h"
#include "utils.h"
#include "RefCountObj.h"
#include <map>
#include <set>
#include <sstream>
#include <vector>

class CScriptToken;
struct IScope;
class FunctionScope;

/**
 * Enumeration of basic Javascript value types.
 */
enum JSValueTypes
{
    VT_UNDEFINED,
    VT_NULL,
    VT_NUMBER,
    VT_BOOL,
    VT_ACTOR_REF,
    VT_INPUT_EP_REF,
    VT_OUTPUT_EP_REF,
    VT_CLASS,
    VT_OBJECT,
    VT_STRING,
    VT_ARRAY,
    VT_ACTOR,
    VT_FUNCTION,
    VT_ACTOR_CLASS,
    VT_INPUT_EP,
    VT_OUTPUT_EP,
};
std::string getTypeName(JSValueTypes vType);

typedef std::vector<std::string> StringVector;
typedef std::set<std::string> StringSet;

/**
 * The possible mutability states of a JSValue.
 */
enum JSMutability
{
    MT_MUTABLE,
    MT_FROZEN,
    MT_DEEPFROZEN   //Deep frozen means that it is frozen and contains no references
                    //which may lead to a mutable object.
};

/**
 * Root class for all Javascript types.
 * Defines several operations which must be implemented by the derived types.
 * The defined operations fall in these categories:
 *  - Conversion to basic types.
 *  - Standard operations
 *  - Type checking
 *  - JSON generation
 */
class JSValue : public RefCountObj
{
public:
    virtual JSValueTypes getType()const = 0;
    
    virtual JSMutability    getMutability()const=0;
    virtual Ref<JSValue>    freeze()=0;
    virtual Ref<JSValue>    unFreeze(bool forceClone=false)=0;

    virtual std::string toString()const = 0;
    virtual bool toBoolean()const = 0;
    virtual double toDouble()const = 0;

    virtual Ref<JSValue> readField(Ref<JSValue> key)const = 0;
    virtual Ref<JSValue> writeField(Ref<JSValue> key, Ref<JSValue> value) = 0;
    virtual Ref<JSValue> newConstField(Ref<JSValue> key, Ref<JSValue> value) = 0;
    virtual Ref<JSValue> deleteField(Ref<JSValue> key) = 0;
    virtual StringSet    getFields(bool inherited = true)const=0;
    
    virtual Ref<JSValue> call (Ref<FunctionScope> scope);

    virtual std::string getJSON(int indent) = 0;
    
    virtual const StringVector& getParams()const=0;
    virtual const std::string& getName()const=0;
    
    Ref<JSValue> readFieldStr(const std::string& strKey)const;
    Ref<JSValue> writeFieldStr(const std::string& strKey, Ref<JSValue> value);
    Ref<JSValue> newConstFieldStr(const std::string& strKey, Ref<JSValue> value);

    virtual std::string getTypeName()const
    {
        return ::getTypeName(this->getType());
    }

    bool isFunction()const
    {
        return getType() >= VT_FUNCTION;
    }

    bool isArray()const
    {
        return getType() == VT_ARRAY;
    }

    bool isObject()const
    {
        return getType() >= VT_OBJECT;
    }

    bool isUndefined()const
    {
        return getType() == VT_UNDEFINED;
    }

    bool isNull()const
    {
        return getType() <= VT_NULL;
    }

    bool isPrimitive()const
    {
        const JSValueTypes t = this->getType();

        return t == VT_STRING || (t > VT_NULL && t < VT_OBJECT);
    }
    
    bool isMutable()const
    {
        return getMutability() == MT_MUTABLE;
    }
};

// JSValue helper functions
//////////////////////////////////////////

class JSBool;
class JSObject;

Ref<JSValue> undefined();
Ref<JSValue> jsNull();
Ref<JSBool> jsTrue();
Ref<JSBool> jsFalse();
Ref<JSValue> jsBool(bool value);
Ref<JSValue> jsInt(int value);
Ref<JSValue> jsDouble(double value);
Ref<JSValue> jsString(const std::string& value);

std::string  key2Str(Ref<JSValue> key);

Ref<JSValue> createConstant(CScriptToken token);

Ref<JSObject> getObject(Ref<IScope> pScope, const std::string& name);

Ref<JSValue> null2undef (Ref<JSValue> value);
bool nullCheck (Ref<JSValue> value);

template <class DestType, class SrcType> 
Ref<DestType> castTo (Ref<SrcType> value)
{
    if (nullCheck(value))
        return Ref<DestType>();
    else
        return Ref<DestType>( static_cast<DestType*> ( value.getPointer() ) );
}

double jsValuesCompare (Ref<JSValue> a, Ref<JSValue> b);

int toInt32 (Ref<JSValue> a);
unsigned long long toUint64 (Ref<JSValue> a);
size_t toSizeT (Ref<JSValue> a);
bool isInteger (Ref<JSValue> a);
bool isUint (Ref<JSValue> a);

typedef std::map< Ref<JSValue>, Ref<JSValue> >  JSValuesMap;
Ref<JSValue>    deepFreeze(Ref<JSValue> obj, JSValuesMap& transformed);
Ref<JSValue>    deepFreeze(Ref<JSValue> obj);


//////////////////////////////////////////

/**
 * Helper class to implement 'JSValue' derived classes.
 * Provides a default implementation for every virtual method.
 */
template <JSValueTypes V_TYPE>
class JSValueBase : public JSValue
{
public:
    
    virtual JSMutability getMutability()const
    {
        return MT_DEEPFROZEN;
    }
    
    virtual Ref<JSValue> freeze()
    {
        return Ref<JSValue>(this);
    }
    
    virtual Ref<JSValue> unFreeze(bool forceClone=false)
    {
        return Ref<JSValue>(this);
    }

    virtual std::string toString()const
    {
        return "";
    }

    virtual bool toBoolean()const
    {
        return false;
    }

    virtual double toDouble()const
    {
        return getNaN();
    }

    virtual Ref<JSValue> readField(Ref<JSValue> key)const
    {
        return undefined();
    }
    
    virtual Ref<JSValue> writeField(Ref<JSValue> key, Ref<JSValue> value)
    {
        return undefined();
    }
    
    virtual Ref<JSValue> newConstField(Ref<JSValue> key, Ref<JSValue> value)
    {
        return undefined();
    }
    
    virtual Ref<JSValue> deleteField(Ref<JSValue> key)
    {
        return undefined();
    }
    
    virtual StringSet getFields(bool inherited = true)const
    {
        static StringSet empty;
        return empty;
    }
    
    virtual std::string getJSON(int indent)
    {
        if (V_TYPE == VT_NULL)
            return "null";
        else
            return "";
    }

    virtual JSValueTypes getType()const
    {
        return V_TYPE;
    }
    
    virtual const StringVector& getParams()const
    {
        static const StringVector    empty;
        return empty;
    }

    virtual const std::string& getName()const
    {
        static std::string empty;
        return empty;
    }
    
};

/**
 * Base class for Javascript primitive types.
 */
template <JSValueTypes V_TYPE>
class JSPrimitive : public JSValueBase<V_TYPE>
{
public:

    virtual std::string getJSON(int indent)
    {
        return this->toString();
    }
};

/**
 * Javascript number class.
 * Both integer and floating point, as all numbers in Javascript are stored
 * as a 64 floating point value.
 * Javascript numbers are immutable. Once created, they cannot be modified.
 */
class JSNumber : public JSPrimitive<VT_NUMBER>
{
public:
    static Ref<JSNumber> create(double value);

    virtual bool toBoolean()const
    {
        return m_value != 0.0;
    }

    virtual double toDouble()const
    {
        return m_value;
    }

    virtual std::string toString()const;

protected:

    JSNumber(double value) : m_value(value)
    {
    }

private:
    const double m_value;
};


/**
 * Javascript booleans class.
 * Javascript booleans are immutable. Once created, they cannot be modified.
 */
class JSBool : public JSPrimitive<VT_BOOL>
{
public:
    friend Ref<JSBool> jsTrue();
    friend Ref<JSBool> jsFalse();

    virtual bool toBoolean()const
    {
        return m_value;
    }

    virtual double toDouble()const
    {
        return m_value ? 1 : 0;
    }

    virtual std::string toString()const
    {
        return m_value ? "true" : "false";
    }

private:

    JSBool(bool b) : m_value(b)
    {
    }

    const bool m_value;
};

/**
 * Stores a variable properties
 */
class VarProperties
{
public:
    Ref<JSValue> value()const
    {
        return m_value;
    }
    
    bool isConst()const
    {
        return m_isConst;
    }
    
    VarProperties (Ref<JSValue> value, bool isConst) 
    : m_value(value), m_isConst(isConst)
    {        
    }
    
    VarProperties () 
    : m_value(undefined()), m_isConst(false)
    {        
    }
    
private:
    Ref<JSValue>    m_value;
    bool            m_isConst;
};

typedef std::map<std::string, VarProperties> VarMap;

void checkedVarWrite (VarMap& map, const std::string& name, Ref<JSValue> value, bool isConst);
Ref<JSValue> checkedVarDelete (VarMap& map, const std::string& name);

class FunctionScope;

/// Pointer to native function type. 
/// Native functions must have this signature
typedef Ref<JSValue> (*JSNativeFn)(FunctionScope* var);

/**
 * Javascript function class.
 * Extends 'JSObject', as functions are objects.
 */
class JSFunction : public JSValueBase<VT_FUNCTION>
{
public:
    static Ref<JSFunction> createJS(const std::string& name, 
                                    const StringVector& params,
                                    Ref<RefCountObj> code);
    static Ref<JSFunction> createNative(const std::string& name, 
                                        const StringVector& params, 
                                        JSNativeFn fnPtr);

//    int addParam(const std::string& name)
//    {
//        m_params.push_back(name);
//        return (int) m_params.size();
//    }
//
//    void setParams(const StringVector& params)
//    {
//        m_params = params;
//    }

    const StringVector& getParams()const
    {
        return m_params;
    }

    const std::string& getName()const
    {
        return m_name;
    }

    void setCodeMVM(Ref<RefCountObj> code)
    {
        m_codeMVM = code;
    }
    
//    void setNativePtr(JSNativeFn pNative)
//    {
//        m_pNative = pNative;
//    }

    Ref<RefCountObj> getCodeMVM()const
    {
        return m_codeMVM;
    }

    bool isNative()const
    {
        return m_pNative != NULL;
    }

    JSNativeFn nativePtr()const
    {
        return m_pNative;
    }

    // JSValue
    /////////////////////////////////////////
    virtual std::string toString()const;

    //TODO: adding functions to the JSON yields invalid JSON. But it is a valuable
    //debug information. Add some kind of flag to enable / disable it.
    virtual std::string getJSON(int indent)
    {
        return "";
    }

    virtual JSValueTypes getType()const
    {
        return VT_FUNCTION;
    }
    
    virtual Ref<JSValue> call (Ref<FunctionScope> scope);
    
    /////////////////////////////////////////

protected:

    JSFunction(const std::string& name, const StringVector& params, JSNativeFn pNative);
    JSFunction(const std::string& name, const StringVector& params, Ref<RefCountObj> code);
    ~JSFunction();

private:
    const std::string m_name;
    StringVector m_params;
    Ref<RefCountObj> m_codeMVM;
    JSNativeFn m_pNative;
};
