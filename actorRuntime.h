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

#include "asVars.h"
#include <deque>

struct MvmRoutine;

Ref<JSValue> asBlockingExec(Ref<MvmRoutine> code, Ref<GlobalScope> globals);

Ref<JSValue> actorChildStoppedDefaultHandler(FunctionScope* pScope);

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

    //void connectMessages (Ref<AsMessageRef> dst, Ref<AsMessageRef> src);

    void sendMessage0(Ref<AsActorRef> dstActor, const std::string& msgName);
    void sendMessage1(Ref<AsActorRef> dstActor,
                      const std::string& msgName,
                      Ref<JSValue> p1);
    void sendMessage2(Ref<AsActorRef> dstActor,
                      const std::string& msgName,
                      Ref<JSValue> p1,
                      Ref<JSValue> p2);
    void sendMessage(Ref<AsActorRef> dstActor, 
                     const std::string& msgName, 
                     Ref<JSArray> params);
    void sendMessage(Ref<AsEndPointRef> dstMessage, Ref<JSArray> params);

    bool dispatchMessage();

protected:
    void actorCrashed(Ref<AsActorRef> actor, const CScriptException& error);

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
    : JSObject(JSObject::DefaultPrototype, MT_MUTABLE)
    , m_rootActor(rootActor)
    {
    }

    Ref<AsActorRef> m_rootActor;
    MessageQueue m_messageQueue;
};


#endif	/* ACTORRUNTIME_H */

