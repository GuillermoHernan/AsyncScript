/* 
 * File:   actorRuntime.h
 * Author: ghernan
 *
 * Actor system runtime support.
 *
 * Created on December 24, 2016, 11:50 AM
 */

#ifndef ACTORRUNTIME_H
#define	ACTORRUNTIME_H

#pragma once

#include "asActors.h"
#include "jsArray.h"
#include <deque>

struct MvmRoutine;
class FunctionScope;
class CScriptException;

std::pair<Ref<JSValue>, Ref<JSValue> > asBlockingExec(Ref<MvmRoutine> code, 
                                                      Ref<GlobalScope> globals, 
                                                      CodeMap* pMap);

Ref<JSValue> actorChildStoppedDefaultHandler(FunctionScope* pScope);

Ref<JSValue>    inputEpCall(Ref<AsEndPointRef> endPoint, Ref<FunctionScope> scope);
Ref<JSValue>    outputEpCall(Ref<AsEndPointRef> endPoint, Ref<FunctionScope> scope);
Ref<JSValue>    actorConstructor(Ref<AsActorClass> actorClass, Ref<FunctionScope> scope);


/**
 * Keeps shared state of the actor system.
 * It contains the message queue. It only supports single-threaded dispatching
 * at this time.
 * 
 * @param rootActor
 * @return 
 */
class ActorRuntime : public JSObject
{
public:

    static Ref<ActorRuntime> create(Ref<AsActorRef> rootActor);
    static Ref<ActorRuntime> getRuntime(FunctionScope* fnScope);

    //void connectMessages (Ref<AsMessageRef> dst, Ref<AsMessageRef> src);

    void sendMessage0(Ref<AsActorRef> dstActor, const std::string& msgName);
    void sendMessage1(Ref<AsActorRef> dstActor,
                      const std::string& msgName,
                      Ref<JSValue> p1);
    void sendMessage2(Ref<AsActorRef> dstActor,
                      const std::string& msgName,
                      Ref<JSValue> p1,
                      Ref<JSValue> p2);
    void sendMessage3(Ref<AsActorRef> dstActor,
                      const std::string& msgName,
                      Ref<JSValue> p1,
                      Ref<JSValue> p2,
                      Ref<JSValue> p3);
    void sendMessage(Ref<AsActorRef> dstActor, 
                     const std::string& msgName, 
                     Ref<JSArray> params);
    void sendMessage(Ref<AsEndPointRef> dstMessage, Ref<JSArray> params);
    
    void stopActor (Ref<AsActorRef> actorRef, Ref<JSValue> value, Ref<JSValue> error);

    bool dispatchMessage(CodeMap* pMap);
    
protected:
    void actorCrashed(Ref<AsActorRef> actor, const char* msg);

    virtual Ref<JSObject>   clone (bool _mutable);

private:
    struct SMessage
    {
        Ref<AsEndPointRef> destination;
        Ref<JSArray> params;

        SMessage(Ref<AsEndPointRef> _dest, Ref<JSArray> _params) :
        destination(_dest), params(_params)
        {
        }
    };

    typedef std::deque<SMessage> MessageQueue;

    ActorRuntime(Ref<AsActorRef> rootActor)
    : JSObject(DefaultClass, MT_DEEPFROZEN)
    , m_rootActor(rootActor)
    {
    }

    Ref<AsActorRef> m_rootActor;
    MessageQueue m_messageQueue;
};


#endif	/* ACTORRUNTIME_H */

