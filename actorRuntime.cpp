/* 
 * File:   actorRuntime.cpp
 * Author: ghernan
 * 
 * Actor system runtime support.
 * 
 * Created on December 24, 2016, 11:50 AM... Merry Christmas!!!
 */

#include "ascript_pch.hpp"
#include "actorRuntime.h"
#include "microVM.h"
#include "scriptMain.h"
#include "jsArray.h"
#include "ScriptException.h"

using namespace std;

void execMessageLoop (Ref<ActorRuntime> runtime, CodeMap* pMap);

ASValue    connectOperator (ExecutionContext* ec);


/**
 * Actor class which executes a piece of MVM code at startup and then finishes.
 */
class RoutineActor : public AsActor
{
public:
    /**
     * Creates a routine actor.
     * @param code
     * @param globals
     * @param parent
     * @return 
     */
    static Ref<AsActorRef> create(Ref<MvmRoutine> code, 
                                  Ref<GlobalScope> globals, 
                                  Ref<AsActorRef> parent)
    {
        auto            constructor = AsEndPoint::createNative("@start", 
                                                               StringVector(),
                                                               routineActorExec);
        VarMap          members;

        members.varWrite(constructor->getName(), constructor, true);

        auto            ownClass = AsActorClass::create("", members, StringVector());
        RoutineActor*   newActor = new RoutineActor (ownClass, globals, parent);

        newActor->m_code = code;
        return AsActorRef::create(refFromNew(newActor));
        
    }

    /**
     * Executes the routine actor.
     * @param pScope
     * @return 
     */
    static ASValue routineActorExec (ExecutionContext* ec)
    {
        auto actor = pScope->getThis().staticCast<RoutineActor>();
        auto actorRef = AsActorRef::create(actor);
        auto globals = ::getGlobals().staticCast<GlobalScope>();
        
        globals->newNotSharedVar("@curActor", actorRef, true);
        
        return mvmExecute(actor->m_code, globals, Ref<IScope>());
    }
    
private:
    RoutineActor (Ref<AsActorClass> cls, Ref<GlobalScope> globals, Ref<AsActorRef> parent) : 
    AsActor (cls, globals, parent)
    {        
    }
    
    Ref<MvmRoutine>     m_code;
};


/**
 * Executes a compiled actor script, and blocks the current thread until all created
 * actors have been stopped.
 * @param code
 * @param globals
 * @return pair of 'JSValue'. The first value is success, the second is error.
 */
pair<ASValue, ASValue > asBlockingExec(Ref<MvmRoutine> code, 
                                                      Ref<GlobalScope> globals, 
                                                      CodeMap* pMap)
{
    auto    rootActor = RoutineActor::create (code, globals, Ref<AsActorRef>());
    auto    runtime = ActorRuntime::create(rootActor);
    
    globals->newVar("@actorRT", runtime, true);
    addNative1("@connect", "src", connectOperator, globals);
    
    execMessageLoop (runtime, pMap);
    
    auto errVal = rootActor->getError();
    
    if (!errVal->isNull())
        return make_pair (jsNull(), errVal);
    else   
        return make_pair (rootActor->getResult(), jsNull());
}

/**
 * Executes the message loop, until there is no message left.
 * @param runtime
 */
void execMessageLoop (Ref<ActorRuntime> runtime, CodeMap* pMap)
{
    //TODO: This function must become more complex when taking I/O into account.
    while (runtime->dispatchMessage(pMap));
}

/**
 * The connect operator ('<-') establishes a connection between and output message
 * and an input message.
 * @param pScope
 * @return 
 */
ASValue connectOperator (ExecutionContext* ec)
{
    auto runtime = ActorRuntime::getRuntime(pScope);
    auto src = ec->getParam("src");
    auto dst = pScope->getThis();
    
    if (src->getType() != VT_OUTPUT_EP_REF)
        rtError ("Source is not an output message");
    
    if (dst->getType() != VT_INPUT_EP_REF)
        rtError ("Destination is not an input message");
    
//    runtime.staticCast<ActorRuntime>()->connectMessages(
//        dst.staticCast<AsMessageRef>(), src.staticCast<AsMessageRef>());
    
    const auto srcActorRef = src.staticCast<AsEndPointRef>()->getActor();
    
    if (!srcActorRef->isRunning())
        return jsNull();
    
    const auto srcActor = srcActorRef->getActor();
    const auto msgName = src.staticCast<AsEndPointRef>()->getEndPoint()->getName();
    
    srcActor->setOutputConnection (msgName, dst.staticCast<AsEndPointRef>());
    
    return jsNull();    
}

/**
 * Handles calls to input endpoints, which are executed as message sends
 * @param endPoint
 * @param scope
 * @return 
 */
ASValue inputEpCall(Ref<AsEndPointRef> endPoint, Ref<FunctionScope> scope)
{
    auto runtime = ActorRuntime::getRuntime(scope.getPointer());
    auto params = scope->get("arguments").staticCast<JSArray>();
    
    runtime->sendMessage (endPoint, params);
    return jsNull();
}

/**
 * Handles calls to output endpoints. It looks for the connected input end point,
 * and sends a message to it.
 * 
 * @param endPoint
 * @param scope
 * @return 
 */
ASValue outputEpCall(Ref<AsEndPointRef> endPoint, Ref<FunctionScope> scope)
{
    //TODO: Check that it is invoked from the owning actor.
    auto actor = endPoint->getActor()->getActor();
    auto destination = actor->getConnectedEp(endPoint->getEndPoint()->getName());
    
    if (destination.isNull())
        return jsNull();
    else
        return inputEpCall(destination, scope);    
}

/**
 * Handles actor constructors calls
 * @param actorClass
 * @param scope
 * @return 
 */
ASValue actorConstructor(Ref<AsActorClass> actorClass, Ref<FunctionScope> scope)
{
    auto curGlobals = ::getGlobals().staticCast<GlobalScope>();
    auto newGlobals = curGlobals->share();
    auto runtime = ActorRuntime::getRuntime(scope.getPointer());
    auto curActor = curGlobals->get("@curActor").staticCast<AsActorRef>();
    auto newActor = AsActor::create(actorClass, newGlobals, curActor);
    auto actorRef = AsActorRef::create(newActor);
    auto constructor = actorClass->getConstructor();
    auto params = constructor->getParams();
    auto msgParams = JSArray::create();
    
    //Set the new current actor at the new global scope.
    newGlobals->newNotSharedVar("@curActor", actorRef, true);
    
    //Parameters become actor fields, and parameters for 'start' message
    for (size_t i=0; i<params.size(); ++i)
    {
        auto value = scope->getParam(params[i]);
        newActor->writeField(params[i], value, false);
        msgParams->push(value);
    }
    
    //Send start message
    runtime->sendMessage(actorRef, "@start", msgParams);
    
    return actorRef;
}

/**
 * Default handler for 'childStopped' message.
 * @param pScope
 * @return 
 */
ASValue actorChildStoppedDefaultHandler(ExecutionContext* ec)
{
    auto actor = pScope->getThis().staticCast<AsActor>();
    auto actorRef = AsActorRef::create(actor);
    auto runtime = ActorRuntime::getRuntime(pScope);
    
    auto result = ec->getParam("result");
    auto errVal = ec->getParam("error");
    
    runtime->stopActor(actorRef, result, errVal);

    return jsNull();
}

/**
 * Creates the actor runtime.
 * @param rootActor
 * @return 
 */
Ref<ActorRuntime> ActorRuntime::create(Ref<AsActorRef> rootActor)
{
    auto result = refFromNew(new ActorRuntime(rootActor));
    
//    result = deepFreeze(result).staticCast<ActorRuntime>();
//    
    result->sendMessage0 (rootActor, "@start");    
    
    return result;
}

/**
 * Obtains the actor runtime from a function scope
 * @param fnScope
 * @return 
 */
Ref<ActorRuntime> ActorRuntime::getRuntime(FunctionScope* fnScope)
{
    auto runtime = ::getGlobals()->get("@actorRT");
    
    if (!runtime->isObject())
        rtError ("Missing actor runtime");

    return runtime.staticCast<ActorRuntime>();
}

/**
 * Sends a message with no parameters.
 * @param dstActor
 * @param epName
 */
void ActorRuntime::sendMessage0 (Ref<AsActorRef> dstActor, const std::string& epName)
{
    auto params = JSArray::create();

    sendMessage(dstActor, epName, params);
}

/**
 * Sends a message with one parameter.
 * @param dstActor
 * @param epName
 * @param p1
 */
void ActorRuntime::sendMessage1 (Ref<AsActorRef> dstActor, 
                                 const std::string& epName, 
                                 ASValue p1)
{
    auto params = JSArray::create();
    
    params->push(p1);

    sendMessage(dstActor, epName, params);
}

/**
 * Sends a message with two parameters.
 * @param dstActor
 * @param epName
 * @param p1
 * @param p2
 */
void ActorRuntime::sendMessage2 (Ref<AsActorRef> dstActor, 
                                 const std::string& epName, 
                                 ASValue p1,
                                 ASValue p2)
{
    auto params = JSArray::create();
    
    params->push(p1);
    params->push(p2);

    sendMessage(dstActor, epName, params);
}

/**
 * Sends a message with three parameters.
 * @param dstActor
 * @param epName
 * @param p1
 * @param p2
 * @param p3
 */
void ActorRuntime::sendMessage3 (Ref<AsActorRef> dstActor, 
                                 const std::string& epName, 
                                 ASValue p1,
                                 ASValue p2,
                                 ASValue p3)
{
    auto params = JSArray::create();
    
    params->push(p1);
    params->push(p2);
    params->push(p3);

    sendMessage(dstActor, epName, params);
}

/**
 * 'sendMessage' version which looks up for the message name
 * @param dstActor
 * @param epName
 * @param params
 */
void ActorRuntime::sendMessage (Ref<AsActorRef> dstActor, 
                                const std::string& epName, 
                                Ref<JSArray> params)
{
    auto ep = dstActor->getEndPoint(epName);
    
    if (ep.isNull())
        rtError ("End point '%s' does not exist", epName.c_str());
    else
        sendMessage (ep, params);
}

/**
 * Sends a message to an actor, through one of its input endpoints
 * @param dstEndPoint
 * @param params
 */
void ActorRuntime::sendMessage (Ref<AsEndPointRef> dstEndPoint, Ref<JSArray> params)
{
    if (dstEndPoint->getType() != VT_INPUT_EP_REF)
        rtError ("Destination is not an input end point");
    
    m_messageQueue.push_back(SMessage(dstEndPoint, params));
}

/**
 * Stops an actor execution
 * @param actorRef
 * @param value
 * @param error
 */
void ActorRuntime::stopActor (Ref<AsActorRef> actorRef, ASValue value, ASValue error)
{
    auto actor = actorRef->getActor();
    
    actor->stop(value, error);

    auto parent = actor->getParent();
    
    if (parent.notNull())
        sendMessage3 (parent, "childStopped", actorRef, value, error);
}

/**
 * Dispatch next message in the queue.
 * @return true if a message has been processed, false if the message queue is 
 * empty.
 */
bool ActorRuntime::dispatchMessage(CodeMap* pMap)
{
    if (m_messageQueue.empty())
        return false;
    
    SMessage    msg = m_messageQueue.front();
    auto        actorRef = msg.destination->getActor();

    m_messageQueue.pop_front();
    
    if (actorRef->isRunning())
    {
        auto actor = actorRef->getActor();
        auto globals = actor->getGlobals();
        GlobalsSetter   g(globals);
        
        try
        {
            try
            {
                auto endPoint = msg.destination->getEndPoint();
                auto scope = FunctionScope::create(endPoint, actor, msg.params);

                if (endPoint->isNative())
                    endPoint->nativePtr()(scope.getPointer());
                else
                {
                    auto routine = endPoint->getCodeMVM().staticCast<MvmRoutine>();
                    mvmExecute (routine, globals, scope);
                }
            }
            catch (RuntimeError& ex)
            {
                ScriptPosition scPos;

                if (pMap != NULL)
                    scPos = pMap->get(ex.Position);
                errorAt(scPos, "%s", ex.what());
            }
        }
        catch (std::exception& ex)
        {
            actorCrashed (actorRef, ex.what());
        }        
    }
    
    return true;
}

/**
 * Handles exceptions inside actors.
 * @param actorRef
 * @param ex
 */
void ActorRuntime::actorCrashed(Ref<AsActorRef> actorRef, const char* msg)
{
    auto actor = actorRef->getActor();
    
    this->stopActor(actorRef, jsNull(), jsString(msg));

    //TODO: Better error logging
    if (actor->getParent().notNull())
        fprintf (stderr, "Actor crashed: %s\n", msg);
    else
        fprintf (stderr, "Root actor crashed: %s\n", msg);
}

Ref<JSObject> ActorRuntime::clone (bool _mutable)
{
    ASSERT (!"clone operation not allowed on 'ActorRuntime'");
    return Ref<JSObject>(this);
}

