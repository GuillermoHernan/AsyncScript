/* 
 * File:   asObjects.h
 * Author: ghernan
 * 
 * Support for object oriented programming in AsyncScript. 
 * Contains the runtime objects for AsyncScript classes and objects
 * 
 * Created on January 3, 2017, 11:16 AM
 */

#ifndef ASOBJECTS_H
#define	ASOBJECTS_H

#pragma once

#include "jsVars.h"


/**
 * Runtime object for classes
 * @return 
 */
class JSClass : public JSValueBase<VT_CLASS>
{
public:
    static Ref<JSClass> create (const std::string& name, 
                                Ref<JSClass> parent, 
                                const VarMap& members, 
                                Ref<JSFunction> constructorFn);

    static Ref<JSClass> createNative (const std::string& name, 
                                Ref<JSClass> parent, 
                                const VarMap& members, 
                                const StringVector& params,
                                JSNativeFn nativeFn);
    
    virtual std::string toString()const
    {
        return std::string("class ") + getName();
    }
    virtual bool toBoolean()const
    {
        return true;
    }
    
    virtual StringSet getFields(bool inherited = true)const;

    virtual Ref<JSValue> readField(const std::string& key)const;
    
    virtual const StringVector& getParams()const
    {
        return m_constructor->getParams();
    }
    
    virtual const std::string& getName()const
    {
        return m_name;
    }
    
    Ref<JSValue> call (Ref<FunctionScope> scope);
    
    Ref<JSClass> getParent()const
    {
        return m_parent;
    }
    
    Ref<JSFunction> getConstructor()const
    {
        return m_constructor;
    }
    
protected:
    JSClass(const std::string& name,
            Ref<JSClass> parent,
            const VarMap& members,
            Ref<JSFunction> constructorFn);

private:
    const std::string   m_name;
    VarMap              m_members;
    Ref<JSClass>        m_parent;
    Ref<JSFunction>     m_constructor;
};


/**
 * AsynScript object class
 */
class JSObject : public JSValue
{
public:
    static Ref<JSObject> create();
    static Ref<JSObject> create(Ref<JSClass> cls);
    
    virtual JSMutability getMutability()const
    {
        return m_mutability;
    }
    
    virtual Ref<JSValue>    freeze();
    virtual Ref<JSValue>    deepFreeze(JSValuesMap& transformed);
    virtual Ref<JSValue>    unFreeze(bool forceClone=false);
    
    void setFrozen();
    
    std::vector <Ref<JSValue> > getKeys()const;
    
    bool isWritable(const std::string& key)const;

    // JSValue
    /////////////////////////////////////////

    virtual std::string toString()const;
    virtual bool        toBoolean()const;
    virtual double      toDouble()const;

    virtual Ref<JSValue> readField(const std::string& key)const;
    virtual Ref<JSValue> writeField(const std::string& key, Ref<JSValue> value, bool isConst);
    virtual Ref<JSValue> deleteField(const std::string& key);
    virtual StringSet    getFields(bool inherited = true)const;
    
    virtual Ref<JSValue> indexedRead(Ref<JSValue> index);
    virtual Ref<JSValue> indexedWrite(Ref<JSValue> index, Ref<JSValue> value);
    
    virtual Ref<JSValue> head();
    virtual Ref<JSValue> tail();
    
    virtual Ref<JSValue> call (Ref<FunctionScope> scope);
    
    virtual std::string getJSON(int indent);

    virtual JSValueTypes getType()const
    {
        return VT_OBJECT;
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
    /////////////////////////////////////////
    
    Ref<JSClass> getClass()const
    {
        return m_cls;
    }
    
    void setClass(Ref<JSClass> cls)
    {
        m_cls = cls;
    }
    
    ///'JSObject' default class
    static Ref<JSClass> DefaultClass;

protected:

    JSObject(Ref<JSClass> cls, JSMutability mutability);

    ~JSObject();

    JSObject(const JSObject& src, bool _mutable);
    virtual Ref<JSObject>   clone (bool _mutable);
    
    static JSMutability     selectMutability(const JSObject& src, bool _mutable);

    Ref<JSValue>    callMemberFn (Ref<JSValue> function)const;
    Ref<JSValue>    callMemberFn (Ref<JSValue> function, Ref<JSValue> p1)const;
    Ref<JSValue>    callMemberFn (Ref<JSValue> function, Ref<JSValue> p1, Ref<JSValue> p2)const;
private:
    VarMap          m_members;
    Ref<JSClass>    m_cls;
    JSMutability    m_mutability;
};

/**
 * Javascript array implementation
 */
class JSArray : public JSValueBase<VT_ARRAY>
{
public:
    static Ref<JSArray> create();
    static Ref<JSArray> create(size_t size);
    static Ref<JSArray> createStrArray(const StringVector& strList);

    size_t push(Ref<JSValue> value);

    size_t length()const
    {
        return m_content.size();
    }

    Ref<JSValue> getAt(size_t index)const;

    // JSValue
    /////////////////////////////////////////
    virtual std::string toString()const;

    virtual std::string getJSON(int indent);
    
    virtual JSMutability getMutability()const
    {
        return m_mutability;
    }

    virtual Ref<JSValue> freeze();
    virtual Ref<JSValue> deepFreeze(JSValuesMap& transformed);
    virtual Ref<JSValue> unFreeze(bool forceClone=false);

    virtual Ref<JSValue> readField(const std::string& key)const;
    virtual Ref<JSValue> writeField(const std::string& key, Ref<JSValue> value, bool isConst);
    
    virtual Ref<JSValue> indexedRead(Ref<JSValue> index);
    virtual Ref<JSValue> indexedWrite(Ref<JSValue> index, Ref<JSValue> value);

    virtual Ref<JSValue> head();
    virtual Ref<JSValue> tail();
    
    /////////////////////////////////////////

    static Ref<JSClass> ArrayClass;
    
    static std::string join(Ref<JSArray> arr, Ref<JSValue> sep);
private:

    JSArray() : m_mutability(MT_MUTABLE)
    {
    }
    
//    JSArray(const JSArray& src, bool _mutable);
//    virtual Ref<JSObject>   clone (bool _mutable);

    void setLength(Ref<JSValue> value);

    std::vector< Ref<JSValue> >     m_content;
    JSMutability                    m_mutability;
};

/**
 * Object to iterate over an array
 * @return 
 */
class JSArrayIterator : public JSObject
{
public:
    static Ref<JSValue> create(Ref<JSArray> arr, size_t index);

    virtual Ref<JSValue> head()const;
    virtual Ref<JSValue> tail()const;

private:
    //TODO: Should it has its own class?

    JSArrayIterator(Ref<JSArray> arr, size_t index) :
    JSObject(DefaultClass, arr->getMutability() == MT_DEEPFROZEN ? MT_DEEPFROZEN : MT_FROZEN),
    m_array(arr),
    m_index(index)
    {
    }

    Ref<JSArray> m_array;
    size_t m_index;
};


#endif	/* ASOBJECTS_H */

