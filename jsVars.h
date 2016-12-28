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
    VT_OBJECT,  //All below are objects
    VT_STRING,
    VT_ARRAY,
    VT_ACTOR,
    VT_FUNCTION,//All below are functions
    VT_ACTOR_CLASS,
    VT_INPUT_EP,
    VT_OUTPUT_EP,
};
std::string getTypeName(JSValueTypes vType);

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
    virtual Ref<JSValue> deleteField(Ref<JSValue> key) = 0;
    virtual std::string getJSON(int indent) = 0;
    
    Ref<JSValue> readFieldStr(const std::string& strKey)const;
    Ref<JSValue> writeFieldStr(const std::string& strKey, Ref<JSValue> value);

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
    
    virtual Ref<JSValue> deleteField(Ref<JSValue> key)
    {
        return undefined();
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
};

/**
 * Interface for variable scopes. Allows to define custom logic for each kind of scope.
 */
struct IScope : public RefCountObj
{
    /**
     * Looks for a symbol. 
     * It always looks for it at the current scope, at may look for it at
     * higher level scopes. This depends on the kind of scope.
     * @param name Symbol name
     * @return The requested value or a NULL pointer if not found.
     */
    virtual bool isDefined(const std::string& name)const = 0;
    virtual Ref<JSValue> get(const std::string& name)const = 0;
    virtual Ref<JSValue> set(const std::string& name, Ref<JSValue> value) = 0;
    virtual Ref<JSValue> newVar(const std::string& name, Ref<JSValue> value) = 0;
    virtual bool isBlockScope()const = 0;
    
protected:
    virtual ~IScope(){};
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
 * Javascript object class
 */
class JSObject : public JSValue
{
public:
    static Ref<JSObject> create();
    static Ref<JSObject> create(Ref<JSObject> prototype);
    
    virtual JSMutability getMutability()const
    {
        return m_mutability;
    }
    
    virtual Ref<JSValue>    freeze();
    virtual Ref<JSValue>    unFreeze(bool forceClone=false);
    
    Ref<JSObject> getPrototype()const
    {
        return m_prototype;
    }
    
    Ref<JSObject> setPrototype(Ref<JSObject> value)
    {
        if (isMutable())
            return m_prototype = value;
        else
            return m_prototype;
    }
    
    void setFrozen();
    
    std::vector <Ref<JSValue> >   getKeys()const;

    // JSValue
    /////////////////////////////////////////

    virtual std::string toString()const
    {
        return "[object Object]";
    }

    virtual bool toBoolean()const
    {
        return true;
    } //TODO: Should be 'undefined'. Check the spec.

    virtual double toDouble()const
    {
        return 0;
    }

    virtual Ref<JSValue> readField(Ref<JSValue> key)const;
    virtual Ref<JSValue> writeField(Ref<JSValue> key, Ref<JSValue> value);
    virtual Ref<JSValue> deleteField(Ref<JSValue> key);

    virtual std::string getJSON(int indent);

    virtual JSValueTypes getType()const
    {
        return VT_OBJECT;
    }
    /////////////////////////////////////////

    ///'JSObject' default prototype
    static Ref<JSObject> DefaultPrototype;

protected:

    JSObject(Ref<JSObject> prototype, JSMutability mutability) : 
    m_prototype (prototype), m_mutability(mutability)
    {
    }

    ~JSObject();

    JSObject(const JSObject& src, bool _mutable);
    virtual Ref<JSObject>   clone (bool _mutable);
    
    static JSMutability     selectMutability(const JSObject& src, bool _mutable);
    static std::string      key2Str(Ref<JSValue> key);

private:
    typedef std::map <std::string, Ref<JSValue> > MembersMap;

    MembersMap      m_members;
    Ref<JSObject>   m_prototype;
    JSMutability    m_mutability;
    
    friend Ref<JSValue> deepFreeze(Ref<JSValue> obj, JSValuesMap& transformed);
};

/**
 * Javascript string class.
 * Javascript strings are immutable. Once created, they cannot be modified.
 */
class JSString : public JSObject
{
public:
    static Ref<JSString> create(const std::string& value);

    virtual Ref<JSValue> unFreeze(bool forceClone=false);

    virtual bool toBoolean()const
    {
        return !m_text.empty();
    }
    virtual double toDouble()const;

    virtual std::string toString()const
    {
        return m_text;
    }

    virtual Ref<JSValue> readField(Ref<JSValue> key)const;

    virtual std::string getJSON(int indent);

    virtual JSValueTypes getType()const
    {
        return VT_STRING;
    }
    
    /// 'JSString' default prototype.
    static Ref<JSObject> DefaultPrototype;

protected:

    JSString(const std::string& text) 
    : JSObject(DefaultPrototype, MT_DEEPFROZEN), m_text(text)
    {
    }

private:
    const std::string m_text;

};

/**
 * Javascript array implementation
 */
class JSArray : public JSObject
{
public:
    static Ref<JSArray> create();
    static Ref<JSArray> create(size_t size);
    static Ref<JSArray> createStrArray(const std::vector<std::string>& strList);

    size_t push(Ref<JSValue> value);

    size_t length()const
    {
        return m_length;
    }

    Ref<JSValue> getAt(size_t index)const;

    // JSValue
    /////////////////////////////////////////
    virtual std::string toString()const;

    virtual std::string getJSON(int indent);

    virtual JSValueTypes getType()const
    {
        return VT_ARRAY;
    }

    virtual Ref<JSValue> readField(Ref<JSValue> key)const;
    virtual Ref<JSValue> writeField(Ref<JSValue> key, Ref<JSValue> value);
    /////////////////////////////////////////


    /// 'JSArray' default prototype.
    static Ref<JSObject> DefaultPrototype;
    
private:

    JSArray() : JSObject(DefaultPrototype, MT_MUTABLE), m_length(0)
    {
    }
    
    JSArray(const JSArray& src, bool _mutable);
    virtual Ref<JSObject>   clone (bool _mutable);

    void setLength(Ref<JSValue> value);

    size_t m_length;
};

class FunctionScope;

/// Pointer to native function type. 
/// Native functions must have this signature
typedef Ref<JSValue> (*JSNativeFn)(FunctionScope* var);

/**
 * Javascript function class.
 * Extends 'JSObject', as functions are objects.
 */
class JSFunction : public JSObject
{
public:
    static Ref<JSFunction> createJS(const std::string& name);
    static Ref<JSFunction> createNative(const std::string& name, JSNativeFn fnPtr);

    typedef std::vector<std::string> ParametersList;

    int addParam(const std::string& name)
    {
        m_params.push_back(name);
        return (int) m_params.size();
    }

    void setParams(const ParametersList& params)
    {
        m_params = params;
    }

    const ParametersList& getParams()const
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
    
    void setNativePtr(JSNativeFn pNative)
    {
        m_pNative = pNative;
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
    /////////////////////////////////////////

    
    /// 'JSFunction' default prototype.
    static Ref<JSObject> DefaultPrototype;

protected:

    JSFunction(const std::string& name, JSNativeFn pNative);
    ~JSFunction();

    JSFunction(const JSFunction& src, bool _mutable);
    virtual Ref<JSObject>   clone (bool _mutable);
    
private:
    const std::string m_name;
    Ref<RefCountObj> m_codeMVM;
    JSNativeFn m_pNative;
    ParametersList m_params;
};

/**
 * Name scope for a block of code.
 * It is also used for the global scope.
 * 
 * @param parent Parent scope. If a symbol is not found in the current scope,
 * it looks in the parent scope.
 */
class BlockScope : public IScope
{
public:
    static Ref<BlockScope> create(Ref<IScope> parent)
    {
        return refFromNew(new BlockScope(parent));
    }

    virtual bool isDefined(const std::string& name)const;
    virtual Ref<JSValue> get(const std::string& name)const;
    virtual Ref<JSValue> set(const std::string& name, Ref<JSValue> value);
    virtual Ref<JSValue> newVar(const std::string& name, Ref<JSValue> value);
    virtual bool isBlockScope()const
    {
        return true;
    }

private:
    BlockScope(Ref<IScope> parent);
    
    Ref<IScope> m_pParent;

    typedef std::map<std::string, Ref<JSValue> > SymbolMap;
    SymbolMap   m_symbols;
};

/**
 * Local scope of a function
 * @param globals
 * @param targetFn
 */
class FunctionScope : public IScope
{
public:
    static Ref<FunctionScope> create(Ref<IScope> globals, Ref<JSFunction> targetFn)
    {
        return refFromNew(new FunctionScope(globals, targetFn));
    }

    void setThis(Ref<JSValue> value)
    {
        m_this = value;
    }

    Ref<JSValue> getThis()const
    {
        return m_this;
    }

    int addParam(Ref<JSValue> value);
    
    Ref<JSValue> getParam(const std::string& name)const;

    // IScope
    ////////////////////////////////////
    virtual bool isDefined(const std::string& name)const;
    virtual Ref<JSValue> get(const std::string& name)const;
    virtual Ref<JSValue> set(const std::string& name, Ref<JSValue> value);
    virtual Ref<JSValue> newVar(const std::string& name, Ref<JSValue> value);
    virtual bool isBlockScope()const
    {
        return false;
    }
    ////////////////////////////////////

    Ref<IScope> getGlobals()const
    {
        return m_globals;
    }
    
    Ref<JSFunction> getFunction()const
    {
        return m_function;
    }

private:
    typedef std::map <std::string, Ref<JSValue> > SymbolsMap;

    SymbolsMap m_params;
    SymbolsMap m_locals;
    Ref<JSFunction> m_function;
    Ref<JSArray> m_arguments;
    Ref<JSValue> m_this;
    Ref<IScope> m_globals;
    
    FunctionScope(Ref<IScope> globals, Ref<JSFunction> targetFn);
};

/**
 * Class to manage the global scope.
 * @param name
 * @return 
 */
class GlobalScope : public IScope
{
public:
    static Ref<GlobalScope> create()
    {
        return refFromNew(new GlobalScope);
    }
    
    // IScope
    /////////////////////////////////
    virtual bool isDefined(const std::string& name)const;
    virtual Ref<JSValue> get(const std::string& name)const;
    virtual Ref<JSValue> set(const std::string& name, Ref<JSValue> value);
    virtual Ref<JSValue> newVar(const std::string& name, Ref<JSValue> value);
    virtual bool isBlockScope()const
    {
        return false;
    }
    /////////////////////////////////
    
    Ref<JSObject>   toObject();
    
private:
    GlobalScope()
    {        
    }

    typedef std::map<std::string, Ref<JSValue> > SymbolMap;
    SymbolMap   m_symbols;
};

