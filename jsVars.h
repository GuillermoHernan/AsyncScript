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
class ExecutionContext;

/**
 * Enumeration of basic Javascript value types.
 */
enum JSValueTypes
{
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
    VT_CLOSURE,
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

    typedef std::map< Ref<JSValue>, Ref<JSValue> >  JSValuesMap;
    
    virtual JSMutability    getMutability()const=0;
    virtual Ref<JSValue>    freeze()=0;
    virtual Ref<JSValue>    deepFreeze(JSValuesMap& transformed)=0;
    virtual Ref<JSValue>    unFreeze(bool forceClone=false)=0;

    virtual std::string toString()const = 0;
    virtual bool toBoolean()const = 0;
    virtual double toDouble()const = 0;

    virtual Ref<JSValue> toFunction() = 0;

    virtual Ref<JSValue> readField(const std::string& key)const;
    virtual Ref<JSValue> writeField(const std::string& key, Ref<JSValue> value, bool isConst) = 0;
    virtual Ref<JSValue> deleteField(const std::string& key) = 0;
    virtual StringSet    getFields(bool inherited = true)const=0;

    virtual Ref<JSValue> getAt(Ref<JSValue> index) = 0;
    virtual Ref<JSValue> setAt(Ref<JSValue> index, Ref<JSValue> value) = 0;
    
    virtual Ref<JSValue> iterator() = 0;
    
    //virtual Ref<JSValue> call (Ref<FunctionScope> scope);

    virtual std::string getJSON(int indent) = 0;
    
    virtual const StringVector& getParams()const=0;
    virtual const std::string& getName()const=0;
    
    Ref<JSValue> deepFreeze();

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
        //TODO: Review this check
        return getType() >= VT_OBJECT;
    }

    bool isNull()const
    {
        return getType() == VT_NULL;
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

Ref<JSValue>    jsNull();
Ref<JSBool>     jsTrue();
Ref<JSBool>     jsFalse();
Ref<JSValue>    jsBool(bool value);
Ref<JSValue>    jsInt(int value);
Ref<JSValue>    jsSizeT(size_t value);
Ref<JSValue>    jsDouble(double value);
Ref<JSValue>    jsString(const std::string& value);

Ref<JSValue>    createConstant(CScriptToken token);

double          jsValuesCompare (Ref<JSValue> a, Ref<JSValue> b);

int             toInt32 (Ref<JSValue> a);
unsigned long long toUint64 (Ref<JSValue> a);
size_t          toSizeT (Ref<JSValue> a);
bool            isInteger (Ref<JSValue> a);
bool            isUint (Ref<JSValue> a);

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
    
    virtual Ref<JSValue> deepFreeze(JSValuesMap& transformed)
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
    
    virtual Ref<JSValue> toFunction()override
    {
        return jsNull();
    }

    virtual Ref<JSValue> writeField(const std::string& key, Ref<JSValue> value, bool isConst)
    {
        return jsNull();
    }
    
    virtual Ref<JSValue> getAt(Ref<JSValue> index)
    {
        return jsNull();
    }
    virtual Ref<JSValue> setAt(Ref<JSValue> index, Ref<JSValue> value)
    {
        return jsNull();
    }
    
    virtual Ref<JSValue> iterator()override
    {
        //TODO: Return list of one element.
        return jsNull();
    }
    
    virtual Ref<JSValue> deleteField(const std::string& key)
    {
        return jsNull();
    }
    
    virtual StringSet getFields(bool inherited = true)const
    {
        static StringSet empty;
        return empty;
    }
    
    virtual std::string getJSON(int indent)
    {
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
    
};//class JSValueBase

/**
 * Class for 'null' values.
 */
class JSNull : public JSValueBase<VT_NULL>
{
public:

    virtual std::string toString()const
    {
        return "null";
    }
    
    virtual std::string getJSON(int indent)
    {
        return "null";
    }

    virtual Ref<JSValue> iterator()override
    {
        return jsNull();
    }
};

/**
 * Javascript number class.
 * Both integer and floating point, as all numbers in Javascript are stored
 * as a 64 floating point value.
 * Javascript numbers are immutable. Once created, they cannot be modified.
 */
class JSNumber : public JSValueBase<VT_NUMBER>
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
    virtual std::string getJSON(int indent);

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
class JSBool : public JSValueBase<VT_BOOL>
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
    
    virtual std::string getJSON(int indent);
   

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
    : m_value(jsNull()), m_isConst(false)
    {        
    }
    
private:
    Ref<JSValue>    m_value;
    bool            m_isConst;
};

typedef std::map<std::string, VarProperties> VarMap;

void            checkedVarWrite (VarMap& map, const std::string& name, Ref<JSValue> value, bool isConst);
Ref<JSValue>    checkedVarDelete (VarMap& map, const std::string& name);

class FunctionScope;

/// Pointer to native function type. 
/// Native functions must have this signature
typedef Ref<JSValue> (*JSNativeFn)(ExecutionContext* var);

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
    
    virtual Ref<JSValue> toFunction()override
    {
        return ref(this);
    }


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
    
    //virtual Ref<JSValue> call (Ref<FunctionScope> scope);
    
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

/**
 * A closure contains a function, and a reference to environment on
 * which it has been created.
 */
class JSClosure : public JSValueBase<VT_CLOSURE>
{
public:
    static Ref<JSClosure> create (Ref<JSFunction> fn, Ref<JSValue> env);
    
    Ref<JSFunction> getFunction()const 
    {
        return m_fn;
    }
    
    Ref<JSValue> getEnv()const
    {
        return m_env;
    }
    
    virtual Ref<JSValue> toFunction()override
    {
        return ref(this);
    }
    
private:
    JSClosure (Ref<JSFunction> fn, Ref<JSValue> env);

    Ref<JSFunction> m_fn;
    Ref<JSValue> m_env;
};