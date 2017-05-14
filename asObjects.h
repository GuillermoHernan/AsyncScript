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
#include "microVM.h"


/**
 * Runtime object for classes
 * @return 
 */
class JSClass : public RefCountObj
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
//    virtual bool toBoolean()const
//    {
//        return true;
//    }
//    
//    virtual ASValue toFunction()override
//    {
//        return m_constructor;
//    }
    
    
    virtual StringSet getFields(bool inherited = true)const;

    virtual ASValue readField(const std::string& key)const;
    
    virtual const StringVector& getParams()const;
    
    virtual const std::string& getName()const
    {
        return m_name;
    }
    
    Ref<JSClass> getParent()const
    {
        return m_parent;
    }
    
    ASValue getConstructor()const
    {
        return m_constructor;
    }
    
    ASValue value()
    {
        return ASValue(this, VT_CLASS);
    }
    
    static ASValue scSetEnv(ExecutionContext* ec);
    
protected:
    JSClass(const std::string& name,
            Ref<JSClass> parent,
            const VarMap& members,
            Ref<JSFunction> constructorFn);

private:
    const std::string   m_name;
    VarMap              m_members;
    Ref<JSClass>        m_parent;
    ASValue             m_constructor;
};


/**
 * AsynScript object class
 */
class JSObject : public RefCountObj
{
public:
    static Ref<JSObject> create();
    static Ref<JSObject> create(Ref<JSClass> cls);
    
    virtual JSMutability getMutability()const
    {
        return m_mutability;
    }
    
    virtual ASValue    freeze();
    virtual ASValue    deepFreeze(ASValue::ValuesMap& transformed);
    virtual ASValue    unFreeze(bool forceClone=false);
    
    void setFrozen();
    
    std::vector <ASValue > getKeys()const;
    
    bool isWritable(const std::string& key)const;

    // JSValue
    /////////////////////////////////////////

    virtual std::string toString(ExecutionContext* ec)const;
    bool                toBoolean(ExecutionContext* ec)const;
    double              toDouble(ExecutionContext* ec)const;
    
//    virtual std::string toString()const;
//    virtual bool        toBoolean()const;
//    virtual double      toDouble()const;
//    
//    virtual ASValue toFunction()override;

    virtual ASValue readField(const std::string& key)const;
    virtual ASValue writeField(const std::string& key, ASValue value, bool isConst);
    virtual ASValue deleteField(const std::string& key);
    virtual StringSet    getFields(bool inherited = true)const;
    
    virtual ASValue getAt(ASValue index, ExecutionContext* ec);
    virtual ASValue setAt(ASValue index, ASValue value, ExecutionContext* ec);
    
    virtual ASValue iterator(ExecutionContext* ec)const;

    virtual double compare (const ASValue& b, ExecutionContext* ec)const;
    
    //virtual ASValue call (Ref<FunctionScope> scope);
    
    virtual std::string getJSON(int indent);


//    virtual JSValueTypes getType()const
//    {
//        return VT_OBJECT;
//    }
//    
//    virtual const std::string& getName()const
//    {
//        static std::string empty;
//        return empty;
//    }
    /////////////////////////////////////////
    
    Ref<JSClass> getClass()const
    {
        return m_cls;
    }
    
    ASValue value()
    {
        return ASValue(this, VT_OBJECT);
    }
    
//    void setClass(Ref<JSClass> cls)
//    {
//        m_cls = cls;
//    }
    
    ///'JSObject' default class
    static Ref<JSClass> DefaultClass;

    static ASValue scSetObjClass(ExecutionContext* ec);

protected:

    JSObject(Ref<JSClass> cls, JSMutability mutability);

    ~JSObject();

    JSObject(const JSObject& src, bool _mutable);
    virtual Ref<JSObject>   clone (bool _mutable);
    
    static JSMutability     selectMutability(const JSObject& src, bool _mutable);

    ASValue    callMemberFn (ASValue function, ExecutionContext* ec)const;
    ASValue    callMemberFn (ASValue function, ASValue p1, ExecutionContext* ec)const;
    ASValue    callMemberFn (ASValue function, ASValue p1, ASValue p2, ExecutionContext* ec)const;
private:
    VarMap          m_members;
    Ref<JSClass>    m_cls;
    
protected:
    JSMutability    m_mutability;
};

#endif	/* ASOBJECTS_H */

