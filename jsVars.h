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
class ExecutionContext;

/**
 * Enumeration of basic Javascript value types.
 */
enum JSValueTypes
{
    VT_NULL,
    VT_BOOL,
    VT_NUMBER,
    //VT_ACTOR_REF,
    //VT_INPUT_EP_REF,
    //VT_OUTPUT_EP_REF,
    VT_CLASS,
    VT_OBJECT,
    VT_STRING,
    //VT_ARRAY,
    //VT_ACTOR,
    VT_FUNCTION,
    VT_CLOSURE,
    //VT_ACTOR_CLASS,
    //VT_INPUT_EP,
    //VT_OUTPUT_EP,
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

struct ExecutionContext;

/**
 * Class which contains an AsyncScript value.
 */
class ASValue
{
public:
    ASValue ();
    ASValue (double number);
    ASValue (bool value);
    ASValue (RefCountObj* ptr, JSValueTypes type);
    ~ASValue();
    
    ASValue (const ASValue& src);
    ASValue& operator=(const ASValue& src);
    
    JSValueTypes getType()const
    {
        return m_type;
    }
    bool isNull()const
    {
        return m_type == VT_NULL;
    }

    typedef std::map< ASValue, ASValue >    ValuesMap;

    JSMutability    getMutability()const;
    bool            isMutable()const;
    ASValue         freeze()const;
    ASValue         deepFreeze()const;
    ASValue         deepFreeze(ValuesMap& transformed)const;
    ASValue         unFreeze(bool forceClone=false)const;

    std::string     toString(ExecutionContext* ec = NULL)const;
    bool            toBoolean(ExecutionContext* ec = NULL)const;
    double          toDouble(ExecutionContext* ec = NULL)const;

    ASValue         readField(const std::string& key)const;
    ASValue         writeField(const std::string& key, ASValue value, bool isConst);
    ASValue         deleteField(const std::string& key);
    StringSet       getFields(bool inherited = true)const;

    ASValue         getAt(ASValue index, ExecutionContext* ctx)const;
    ASValue         setAt(ASValue index, ASValue value, ExecutionContext* ctx)const;
    
    ASValue         iterator(ExecutionContext* ctx)const;
    
    std::string     getJSON(int indent)const;
    
    bool            operator < (const ASValue& b)const;
    double          typedCompare (const ASValue& b, ExecutionContext* ec)const;
    double          compare (const ASValue& b, ExecutionContext* ec)const;

    int             toInt32 ()const;
    unsigned long long toUint64 ()const;
    size_t          toSizeT ()const;
    bool            isInteger ()const;
    bool            isUint ()const;
    
    template <class T>
    Ref<T> staticCast()const
    {
        ASSERT(m_type >= VT_CLASS);
        return ref(static_cast<T*>(m_content.ptr));
    }

private:
    void        setNull();

private:
    //The different data types which an 'ASValue' can hold.
    union Content
    {
        bool            boolean;
        double          number;
        RefCountObj*    ptr;
    };
    
    Content         m_content;
    JSValueTypes    m_type;
};

typedef std::vector<ASValue >   ValueVector;
typedef ASValue::ValuesMap      ValuesMap;


/**
 * Root class for all Javascript types.
 * Defines several operations which must be implemented by the derived types.
 * The defined operations fall in these categories:
 *  - Conversion to basic types.
 *  - Standard operations
 *  - Type checking
 *  - JSON generation
 */
/*class JSValue : public RefCountObj
{
public:
    virtual JSValueTypes getType()const = 0;

    typedef std::map< Ref<JSValue>, Ref<JSValue> >  JSValuesMap;
    
    virtual JSMutability    getMutability()const=0;
    virtual Ref<JSValue>    freeze()=0;
    virtual Ref<JSValue>    deepFreeze(JSValuesMap& transformed)=0;
    virtual Ref<JSValue>    unFreeze(bool forceClone=false)=0;

    //TODO: [Sobreescribir] Lo mejor en estas funciones es no usar
    //funciones virtuales (creo)
    //Aún así, si hay que ejecutar código MVM para resolver la llamada,
    //habría que pasarle el puntero al 'ExecutionContext'.
    //Se usen o no funciones virtuales. A no ser que el 'ExecutionContext'
    //se ponga en una variable global.
    virtual std::string toString()const = 0;
    virtual bool toBoolean()const = 0;
    virtual double toDouble()const = 0;

    //TODO: [Sobreescribir]: Se podría resolver sin llamar a código MVM
    //Es simplemente buscar la funciṕn sobreescrita y devolverla.
    //Si todas fuesen como esta sería todo más fácil... ¿Y si hago que sea así?    
    virtual Ref<JSValue> toFunction() = 0;

    //TODO: [Sobreescribir]: Estas no.
    virtual Ref<JSValue> readField(const std::string& key)const;
    virtual Ref<JSValue> writeField(const std::string& key, Ref<JSValue> value, bool isConst) = 0;
    virtual Ref<JSValue> deleteField(const std::string& key) = 0;
    virtual StringSet    getFields(bool inherited = true)const=0;

    //TODO: [Sobreescribir]: Más o menos la misma problemática que todas.
    virtual Ref<JSValue> getAt(Ref<JSValue> index) = 0;
    virtual Ref<JSValue> setAt(Ref<JSValue> index, Ref<JSValue> value) = 0;
    
    //TODO: [Sobreescribir]: Más o menos la misma problemática que todas.
    virtual Ref<JSValue> iterator() = 0;
    
    //virtual Ref<JSValue> call (Ref<FunctionScope> scope);

    //TODO: [Sobreescribir]: Más o menos la misma problemática que todas.
    virtual std::string getJSON(int indent) = 0;
    
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
};*/

// JSValue helper functions
//////////////////////////////////////////

//class JSBool;
class JSObject;

ASValue    jsNull();
ASValue    jsTrue();
ASValue    jsFalse();
ASValue    jsBool(bool value);
ASValue    jsInt(int value);
ASValue    jsSizeT(size_t value);
ASValue    jsDouble(double value);
ASValue    jsString(const std::string& value);

ASValue    createConstant(CScriptToken token);
ASValue    singleItemIterator(ASValue v);


//////////////////////////////////////////

/**
 * Helper class to implement 'JSValue' derived classes.
 * Provides a default implementation for every virtual method.
 */
/*template <JSValueTypes V_TYPE>
class JSValueBase : public JSValue
{
public:
    
    virtual JSMutability getMutability()const
    {
        return MT_DEEPFROZEN;
    }
    
    virtual ASValue freeze()
    {
        return ASValue(this);
    }
    
    virtual ASValue deepFreeze(JSValuesMap& transformed)
    {
        return ASValue(this);
    }
    
    virtual ASValue unFreeze(bool forceClone=false)
    {
        return ASValue(this);
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
    
    virtual ASValue toFunction()override
    {
        return jsNull();
    }

    virtual ASValue writeField(const std::string& key, ASValue value, bool isConst)
    {
        return jsNull();
    }
    
    virtual ASValue getAt(ASValue index)
    {
        return jsNull();
    }
    virtual ASValue setAt(ASValue index, ASValue value)
    {
        return jsNull();
    }
    
    virtual ASValue iterator()override
    {
        //TODO: Return list of one element.
        return jsNull();
    }
    
    virtual ASValue deleteField(const std::string& key)
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
    
    virtual const std::string& getName()const
    {
        static std::string empty;
        return empty;
    }
    
};//class JSValueBase*/

/**
 * Class for 'null' values.
 */
/*class JSNull : public JSValueBase<VT_NULL>
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

    virtual ASValue iterator()override
    {
        return jsNull();
    }
};*/

/**
 * Javascript number class.
 * Both integer and floating point, as all numbers in Javascript are stored
 * as a 64 floating point value.
 * Javascript numbers are immutable. Once created, they cannot be modified.
 */
/*class JSNumber : public JSValueBase<VT_NUMBER>
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
};*/


/**
 * Javascript booleans class.
 * Javascript booleans are immutable. Once created, they cannot be modified.
 */
/*class JSBool : public JSValueBase<VT_BOOL>
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
};*/

/**
 * Stores a variable properties
 */
class VarProperties
{
public:
    ASValue value()const
    {
        return m_value;
    }
    
    bool isConst()const
    {
        return m_isConst;
    }
    
    VarProperties (ASValue value, bool isConst) 
    : m_value(value), m_isConst(isConst)
    {        
    }
    
    VarProperties () 
    : m_value(jsNull()), m_isConst(false)
    {        
    }
    
private:
    ASValue    m_value;
    bool            m_isConst;
};

typedef std::map<std::string, VarProperties> VarMap;

void       checkedVarWrite (VarMap& map, const std::string& name, ASValue value, bool isConst);
ASValue    checkedVarDelete (VarMap& map, const std::string& name);

/// Pointer to native function type. 
/// Native functions must have this signature
typedef ASValue (*JSNativeFn)(ExecutionContext* var);

/**
 * Javascript function class.
 */
class JSFunction : public RefCountObj
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
    
//    virtual ASValue toFunction()override
//    {
//        return ref(this);
//    }


    //TODO: adding functions to the JSON yields invalid JSON. But it is a valuable
    //debug information. Add some kind of flag to enable / disable it.
//    virtual std::string getJSON(int indent)
//    {
//        return "";
//    }

//    virtual JSValueTypes getType()const
//    {
//        return VT_FUNCTION;
//    }
    
    //virtual ASValue call (Ref<FunctionScope> scope);
    
    /////////////////////////////////////////
    
    ASValue value()
    {
        return ASValue(this, VT_FUNCTION);
    }

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
class JSClosure : public RefCountObj
{
public:
    static Ref<JSClosure> create (Ref<JSFunction> fn, const ASValue* first, size_t count);
    
    Ref<JSFunction> getFunction()const 
    {
        return m_fn;
    }
    
    std::string toString()const;
    
    ASValue value()
    {
        return ASValue(this, VT_CLOSURE);
    }
    
    JSMutability getMutability()const
    {
        return m_mutability;
    }
    
    ASValue deepFreeze(ValuesMap& transformed)const;
    ASValue readField(const std::string& key);
    ASValue writeField(const std::string& key, ASValue value, bool isConst);
    ASValue getAt(ASValue index);
   
    
private:
    JSClosure (Ref<JSFunction> fn, const ASValue* first, size_t count);
    JSClosure (Ref<JSFunction> fn, ASValue env);
    
    static JSMutability selectMutability (const ASValue* first, size_t count);

    Ref<JSFunction> m_fn;
    ValueVector     m_params;
    ASValue         m_env;
    JSMutability    m_mutability;
};