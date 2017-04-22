/* 
 * File:   asVars.h
 * Author: ghernan
 * 
 * Async script basic data types. Contains the data types related to actor system.
 * The rest are in 'jsVars.*'
 *
 * Created on December 25, 2016, 12:53 PM
 */

#ifndef ASVARS_H
#define	ASVARS_H

#pragma once

#include "asObjects.h"
#include "microVM.h"

class AsEndPoint;
class MvmRoutine;

/**
 * Actor class runtime object
 */
class AsActorClass : public JSValueBase<VT_ACTOR_CLASS>
{
public:
    static Ref<AsActorClass> create (const std::string& name, 
                                     const VarMap& members, 
                                     const StringVector& params);

    //virtual ASValue call (Ref<FunctionScope> scope);
    
    virtual StringSet getFields(bool inherited = true)const;
    virtual ASValue readField(const std::string& key)const;
    
    Ref<AsEndPoint> getEndPoint (const std::string& name);

    Ref<AsEndPoint> getConstructor ()
    {
        return getEndPoint("@start");
    }
    
    virtual const StringVector& getParams()const
    {
        return m_params;
    }

    virtual const std::string& getName()const
    {
        return m_name;
    }

protected:
    AsActorClass(const std::string& name,
                 const VarMap& members,
                 const StringVector& params);

    static VarMap createDefaultEndPoints (const VarMap& members);
    
private:
    std::string     m_name;
    VarMap          m_members;
    StringVector    m_params;
};

class AsActorRef;
class AsEndPointRef;
typedef std::vector < Ref<AsActorRef> > AsActorList;

/**
 * Actor runtime object
 */
class AsActor : public JSValueBase<VT_ACTOR>
{
public:
    
    static Ref<AsActor> create (Ref<AsActorClass> cls, 
                                Ref<GlobalScope> globals,
                                Ref<AsActorRef> parent)
    {
        return refFromNew(new AsActor(cls, globals, parent));
    }

    virtual JSValueTypes getType()const
    {
        return VT_ACTOR;
    }
    
    virtual ASValue readField(const std::string& key)const;    
    virtual ASValue writeField(const std::string& key, ASValue value, bool isConst);
    
    void setOutputConnection (const std::string& msgName, Ref<AsEndPointRef> dst)
    {
        m_outputConections[msgName] = dst;
    }
    
    Ref<AsEndPointRef> getConnectedEp (const std::string& msgName)const;
    
    bool isRunning()const
    {
        return !m_finished;
    }
    
    void stop(ASValue result, ASValue error);
        
    ASValue getResult()
    {
        return m_result;
    }
        
    ASValue getError()
    {
        return m_error;
    }
    
    Ref<AsEndPoint> getEndPoint (const std::string& name)const
    {
        return m_cls->getEndPoint(name);
    }

    Ref<GlobalScope> getGlobals()const
    {
        return m_globals;
    }
    
    Ref<AsActorRef> getParent()const
    {
        return m_parent;
    }
    
protected:
    AsActor (Ref<AsActorClass> cls, Ref<GlobalScope> globals, Ref<AsActorRef> parent) :
        m_cls (cls)
        , m_globals (globals)
        , m_parent (parent)
        , m_result(jsNull())
        , m_error(jsNull())
        , m_finished(false)
    {
    }
        
private:
    const Ref<AsActorClass> m_cls;
    Ref<GlobalScope>    m_globals;
    Ref<AsActorRef>     m_parent;

    VarMap              m_members;
    
    AsActorList         m_childActors;
    
    typedef std::map<std::string, Ref<AsEndPointRef> > ConnectionMap;
    ConnectionMap       m_outputConections;
    ASValue        m_result;
    ASValue        m_error;
    
    bool                m_finished;
};


/**
 * Actor reference object
 * @param cls
 * @return 
 */
class AsActorRef : public JSValueBase<VT_ACTOR_REF>
{
public:
    static Ref<AsActorRef> create (Ref<AsActor> actor)
    {
        return refFromNew(new AsActorRef(actor));
    }
    
    virtual ASValue readField(const std::string& key)const
    {
        return getEndPoint(key);
    }

    bool isRunning()
    {
        return m_ref->isRunning();
    }
        
    Ref<AsActor> getActor()const
    {
        return m_ref;
    }
    
    ASValue getResult()
    {
        if (isRunning())
            return jsNull();
        else
            return m_ref->getResult();
    }
    
    ASValue getError()
    {
        if (isRunning())
            return jsNull();
        else
            return m_ref->getError();
    }
    
    Ref<AsEndPointRef> getEndPoint (const std::string& name)const;
    
protected:
    AsActorRef (Ref<AsActor> actor) : m_ref (actor)
    {
    }
    
private:
    const Ref<AsActor> m_ref;
    
};

/**
 * Message input or output end point object
 */
class AsEndPoint : public JSFunction
{
public:
    static Ref<AsEndPoint> create (const std::string& name, 
                                   const StringVector& params,  
                                   bool input)
    {
        return refFromNew (new AsEndPoint (name, params, input));
    }

    static Ref<AsEndPoint> createInput (const std::string& name, 
                                        const StringVector& params, 
                                        Ref<MvmRoutine> code)
    {
        return refFromNew (new AsEndPoint (name, params, code));
    }

    static Ref<AsEndPoint> createNative(const std::string& name,
                                        const StringVector& params,
                                        JSNativeFn fnPtr
                                        )
    {
        return refFromNew (new AsEndPoint (name, params, fnPtr));
    }
    
    
    bool isInput()const
    {
        return m_isInput;
    }

    virtual JSValueTypes getType()const
    {
        return isInput() ? VT_INPUT_EP : VT_OUTPUT_EP;
    }
    
    virtual std::string toString()const;
    
protected:
    AsEndPoint (const std::string& name, const StringVector& params, bool input) :
    JSFunction(name, params, NULL), m_isInput (input)
    {
    }

    AsEndPoint (const std::string& name, const StringVector& params, Ref<MvmRoutine> code);
    AsEndPoint (const std::string& name, const StringVector& params, JSNativeFn pNative);
   
private:
    const bool m_isInput;
};

//TODO: May be, I am abusing of subclassing from 'JSFunction'. There should
//be something like a 'JSCallable' base class.

/**
 * Async Script message reference.
 * Holds a reference to the message, and to the actor.
 */
class AsEndPointRef : public JSValueBase<VT_INPUT_EP_REF>
{
public:
    static Ref<AsEndPointRef> create (Ref<AsEndPoint> endPoint, Ref<AsActorRef> actor)
    {
        return refFromNew (new AsEndPointRef (endPoint, actor));
    }
    
    bool isInput()const
    {
        return m_endPoint->isInput();
    }
    
    virtual JSValueTypes getType()const
    {
        return (m_endPoint->getType() == VT_OUTPUT_EP) ? VT_OUTPUT_EP_REF : VT_INPUT_EP_REF;
    }
    
    ASValue call (Ref<FunctionScope> scope);
    
    Ref<AsActorRef> getActor()const
    {
        return m_actor;
    }
    
    Ref<AsEndPoint> getEndPoint()const
    {
        return m_endPoint;
    }
    
protected:
    AsEndPointRef (Ref<AsEndPoint> endPoint, Ref<AsActorRef> actor)
        : m_endPoint (endPoint), m_actor(actor)
    {
    }
    
private:
    const Ref<AsEndPoint>   m_endPoint;
    const Ref<AsActorRef>   m_actor;
};

#endif	/* ASVARS_H */

