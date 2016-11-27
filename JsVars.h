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

#include "TinyJS_Lexer.h"
#include "utils.h"
#include <map>
#include <sstream>
#include <vector>

class CScriptToken;
struct IScope;

/**
 * Smart reference class, for reference-counted objects
 */
template <class ObjType>
class Ref
{
public:

    Ref() : m_ptr(NULL)
    {
    }

    Ref(ObjType* ptr) : m_ptr(ptr)
    {
        if (ptr != NULL)
            ptr->addref();
    }

    Ref(const Ref<ObjType>& src)
    {
        m_ptr = src.getPointer();

        if (m_ptr != NULL)
            m_ptr->addref();
    }

    template <class SrcType>
    Ref(const Ref<SrcType>& src)
    {
        m_ptr = src.getPointer();

        if (m_ptr != NULL)
            m_ptr->addref();
    }

    ~Ref()
    {
        if (m_ptr != NULL)
            m_ptr->release();
    }

    Ref<ObjType> & operator=(const Ref<ObjType>& src)
    {
        ObjType*  srcPtr = src.getPointer();

        if (srcPtr != NULL)
            srcPtr->addref();
        
        if (m_ptr != NULL)
            m_ptr->release();

        m_ptr = srcPtr;
        
        return *this;
    }

    template <class SrcType>
    Ref<ObjType> & operator=(const Ref<SrcType>& src)
    {
        return this->operator =(src.staticCast<ObjType>());
    }

    bool isNull()const
    {
        return m_ptr == NULL;
    }

    ObjType* operator->()const
    {
        return m_ptr;
    }

    ObjType* getPointer()const
    {
        return m_ptr;
    }

    template <class DestType>
    Ref<DestType> staticCast()const
    {
        return Ref<DestType> (static_cast<DestType*> (m_ptr));
    }

private:
    ObjType* m_ptr;
};

/**
 * Enumeration of basic Javascript value types.
 */
enum JSValueTypes
{
    VT_UNDEFINED,
    VT_NULL,
    VT_NUMBER,
    VT_BOOL,
    VT_STRING,
    VT_OBJECT,
    VT_ARRAY,
    VT_FUNCTION
};
std::string getTypeName(JSValueTypes vType);

/**
 * Root class for all Javascript types.
 * Defines several operations which must be implemented by the derived types.
 * The defined operations fall in these categories:
 *  - Conversion to basic types.
 *  - Standard operations
 *  - Type checking
 *  - JSON generation
 */
class JSValue
{
public:

    JSValue() : m_refCount(1)
    {
    }

    virtual ~JSValue()
    {
    }

    virtual std::string toString()const = 0;
    virtual bool toBoolean()const = 0;
    virtual int toInt32()const = 0;
    virtual double toDouble()const = 0;

    virtual Ref<JSValue> memberAccess(const std::string& name) = 0;
    virtual Ref<JSValue> arrayAccess(Ref<JSValue> index) = 0;

    virtual std::string getJSON() = 0;


    virtual bool isReference()const = 0;
    virtual JSValueTypes getType()const = 0;

    virtual std::string getTypeName()const
    {
        return ::getTypeName(this->getType());
    }

    bool isFunction()const
    {
        return getType() == VT_FUNCTION;
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

        return (t > VT_NULL && t < VT_OBJECT);
    }

    int addref()
    {
        return ++m_refCount;
    }

    void release()
    {
        if (--m_refCount == 0)
            delete this;
    }

private:
    int m_refCount;
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

Ref<JSObject> getObject(IScope* pScope, const std::string& name);

Ref<JSValue> dereference (Ref<JSValue> value);
Ref<JSValue> null2undef (Ref<JSValue> value);
bool nullCheck (Ref<JSValue> value);

template <class DestType, class SrcType> 
Ref<DestType> castTo (Ref<SrcType> value)
{
    if (nullCheck(value))
        return Ref<DestType>();
    else
        return Ref<DestType>( static_cast<DestType*> ( dereference(value).getPointer() ) );
}



//////////////////////////////////////////

/**
 * Helper class to implement 'JSValue' derived classes.
 * Provides a default implementation for every virtual method.
 */
template <JSValueTypes V_TYPE>
class JSValueBase : public JSValue
{
public:

    virtual std::string toString()const
    {
        return "";
    }

    virtual bool toBoolean()const
    {
        return false;
    }

    virtual int toInt32()const
    {
        return 0;
    }

    virtual double toDouble()const
    {
        return getNaN();
    }

    virtual Ref<JSValue> memberAccess(const std::string& name)
    {
        return undefined();
    }

    virtual Ref<JSValue> arrayAccess(Ref<JSValue> index)
    {
        return memberAccess(index->toString());
    }

    virtual std::string getJSON()
    {
        return "";
    }

    virtual bool isReference()const
    {
        return false;
    }

    virtual JSValueTypes getType()const
    {
        return V_TYPE;
    }
};

/**
 * Interface for variable scopes. Allows to define custom logic for each kind of scope.
 */
struct IScope
{
    /**
     * Looks for a symbol. 
     * It always look for it at the current scope, at may look for it at
     * higher level scopes. This depends on the kind of scope.
     * @param name Symbol name
     * @return The requested value or a NULL pointer if not found.
     */
    virtual Ref<JSValue> get(const std::string& name)const = 0;
    virtual Ref<JSValue> set(const std::string& name, Ref<JSValue> value, bool forceLocal=false) = 0;
    virtual IScope* getFunctionScope() = 0;
    
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

    virtual std::string getJSON()
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

    virtual int toInt32()const
    {
        return (int) m_value;
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

    virtual int toInt32()const
    {
        return m_value ? 1 : 0;
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
 * Holds a reference to a Javascript variable.
 * @note The reference must be strictly temporary, as it depends on the pointer to 
 * the scope, which will be invalid when it goes out of scope.
 */
//TODO: The current references system is problematic. Re-think it.
class JSReference : public JSValue
{
public:
    static Ref<JSReference> create(IScope* pScope, const std::string& name);

    Ref<JSValue> set(Ref<JSValue> value);
    
    Ref<JSValue> get()const
    {
        return target();
    }

    virtual std::string toString()const
    {
        return target()->toString();
    }

    virtual bool toBoolean()const
    {
        return target()->toBoolean();
    }

    virtual int toInt32()const
    {
        return target()->toInt32();
    }

    virtual double toDouble()const
    {
        return target()->toDouble();
    }

    virtual Ref<JSValue> memberAccess(const std::string& name)
    {
        return target()->memberAccess(name);
    }

    virtual Ref<JSValue> arrayAccess(Ref<JSValue> index)
    {
        return target()->arrayAccess(index);

    }

    virtual std::string getJSON()
    {
        return target()->getJSON();
    }

    virtual bool isReference()const
    {
        return true;
    }

    virtual JSValueTypes getType()const
    {
        return target()->getType();
    }

private:
    JSReference(IScope* pScope, const std::string& name);

    const std::string m_name;
    IScope * const m_pScope;

    Ref<JSValue> target()const
    {
        return null2undef(m_pScope->get(m_name));
    }
};

/**
 * Javascript object class
 */
class JSObject : public JSValue, public IScope
{
public:
    static Ref<JSObject> create(Ref<JSObject> prototype);
    
    Ref<JSObject> getPrototype()const
    {
        return m_prototype;
    }
    
    Ref<JSObject> setPrototype(Ref<JSObject> value)
    {
        return m_prototype = value;
    }
    
    void freeze()
    {
        m_frozen = true;
    }

    // IScope
    /////////////////////////////////////////
    virtual Ref<JSValue> get(const std::string& name)const;
    virtual Ref<JSValue> set(const std::string& name, Ref<JSValue> value, bool forceLocal=false);

    virtual IScope* getFunctionScope()
    {
        return NULL;
    }
    /////////////////////////////////////////

    // JSValue
    /////////////////////////////////////////

    virtual std::string toString()const
    {
        return "[object Object]";
    }

    virtual bool toBoolean()const
    {
        return true;
    } //TODO: Should 'undefined'. Check the spec.

    virtual int toInt32()const
    {
        return 0;
    }

    virtual double toDouble()const
    {
        return 0;
    }

    virtual Ref<JSValue> memberAccess(const std::string& name);

    virtual Ref<JSValue> arrayAccess(Ref<JSValue> index)
    {
        if (index->isPrimitive())
            return memberAccess(index->toString());
        else
            throw CScriptException("Invalid array index");
    }

    virtual std::string getJSON();

    virtual bool isReference()const
    {
        return false;
    }

    virtual JSValueTypes getType()const
    {
        return VT_OBJECT;
    }
    /////////////////////////////////////////

    ///'JSObject' default prototype
    static Ref<JSObject> DefaultPrototype;

protected:

    JSObject(Ref<JSObject> prototype) : m_prototype (prototype), m_frozen(false)
    {
    }

private:
    typedef std::map <std::string, Ref<JSValue> > MembersMap;

    MembersMap      m_members;
    Ref<JSObject>   m_prototype;
    bool            m_frozen;
};

/**
 * Javascript string class.
 * Javascript strings are immutable. Once created, they cannot be modified.
 */
class JSString : public JSObject
{
public:
    static Ref<JSString> create(const std::string& value);

    virtual bool toBoolean()const
    {
        return !m_text.empty();
    }
    virtual int toInt32()const;
    virtual double toDouble()const;

    virtual std::string toString()const
    {
        return m_text;
    }

    virtual std::string getJSON()
    {
        return std::string("\"") + m_text + "\"";
    }

    virtual JSValueTypes getType()const
    {
        return VT_STRING;
    }

    /// 'JSString' default prototype.
    static Ref<JSObject> DefaultPrototype;

protected:

    JSString(const std::string& text) : JSObject(DefaultPrototype), m_text(text)
    {
        freeze();
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

    size_t push(Ref<JSValue> value);

    size_t length()const
    {
        return m_length;
    }

    // IScope
    /////////////////////////////////////////
    virtual Ref<JSValue> get(const std::string& name)const;
    virtual Ref<JSValue> set(const std::string& name, Ref<JSValue> value, bool forceLocal=false);

    // JSValue
    /////////////////////////////////////////
    virtual std::string toString()const;

    virtual std::string getJSON();

    virtual JSValueTypes getType()const
    {
        return VT_ARRAY;
    }
    /////////////////////////////////////////


    /// 'JSArray' default prototype.
    static Ref<JSObject> DefaultPrototype;
    
private:

    JSArray() : JSObject(DefaultPrototype), m_length(0)
    {
    }

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

    int addParameter(const std::string& name)
    {
        m_params.push_back(name);
        return (int) m_params.size();
    }

    const ParametersList getParams()const
    {
        return m_params;
    }

    const std::string& getName()const
    {
        return m_name;
    }

    void setCode(CScriptToken token)
    {
        m_code = token;
    }

    bool isNative()const
    {
        return m_pNative != NULL;
    }

    JSNativeFn nativePtr()const
    {
        return m_pNative;
    }

    CScriptToken codeBlock()const
    {
        return m_code;
    }

    // JSValue
    /////////////////////////////////////////
    virtual std::string toString()const;

    virtual std::string getJSON()
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

private:

    JSFunction(const std::string& name, JSNativeFn pNative) :
    JSObject(DefaultPrototype),
    m_name(name),
    m_code(""),
    m_pNative(pNative)
    {
    }

    const std::string m_name;
    CScriptToken m_code;
    const JSNativeFn m_pNative;
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

    BlockScope(IScope* parent) : m_pParent(parent)
    {
    }

    virtual Ref<JSValue> get(const std::string& name)const;
    virtual Ref<JSValue> set(const std::string& name, Ref<JSValue> value, bool forceLocal=false);
    virtual IScope* getFunctionScope();

private:
    IScope* m_pParent;

    typedef std::map<std::string, Ref<JSValue> > SymbolMap;
    SymbolMap m_symbols;
};

/**
 * Local scope of a function
 * @param globals
 * @param targetFn
 */
class FunctionScope : public IScope
{
public:
    FunctionScope(IScope* globals, Ref<JSFunction> targetFn);

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

    Ref<JSValue> getResult()const
    {
        return m_result;
    }

    void setResult(Ref<JSValue> value)
    {
        m_result = value;
    }

    virtual Ref<JSValue> get(const std::string& name)const;
    virtual Ref<JSValue> set(const std::string& name, Ref<JSValue> value, bool forceLocal=false);

    virtual IScope* getFunctionScope()
    {
        return this;
    }

    IScope* getGlobals()const
    {
        return m_globals;
    }

private:
    typedef std::map <std::string, Ref<JSValue> > SymbolsMap;

    SymbolsMap m_params;
    Ref<JSFunction> m_function;
    Ref<JSArray> m_arguments;
    Ref<JSValue> m_this;
    Ref<JSValue> m_result;
    IScope* m_globals;
};
