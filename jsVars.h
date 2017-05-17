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

    VT_CLASS,
    VT_OBJECT,
    VT_STRING,
    VT_FUNCTION,
    VT_CLOSURE,
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

/**
 * Stores values and properties of variables.
 */
class VarMap
{
public:
    typedef const std::string   CSTR;
    
    bool    isConst (CSTR& name)const;
    ASValue getValue (CSTR& name)const;
    bool    tryGetValue (CSTR& name, ASValue* val)const;

    void    checkedVarWrite (CSTR& name, ASValue value, bool isConst);
    ASValue varWrite (CSTR& name, ASValue value, bool isConst);
    ASValue varDelete (CSTR& name);
    
    ASValue getProperty (CSTR& name, CSTR& propName)const;
    ASValue setProperty (CSTR& name, CSTR& propName, ASValue propValue);
    
    typedef std::function<ASValue (CSTR&, ASValue)>    ItemFn;
    typedef std::function<void (CSTR&, ASValue)>       VoidItemFn;
    typedef std::function<bool (CSTR&, ASValue)>       BoolItemFn;
    
    VarMap  map (ItemFn fn)const;
    void    forEach (VoidItemFn fn)const;
    bool    any (BoolItemFn fn)const;

    void    forEachProperty (CSTR& varName, VoidItemFn fn)const;
    
private:
    typedef std::map<std::string, ASValue>  ContentMap;
    ContentMap    m_content;
};


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

    virtual std::string toString()const;
    
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