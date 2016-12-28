/* 
 * File:   actorRuntime.cpp
 * Author: ghernan
 * 
 * Actor system runtime support.
 * 
 * Created on December 24, 2016, 11:50 AM... Merry Christmas!!!
 */

#include "OS_support.h"
#include "actorRuntime.h"
#include "microVM.h"
#include "scriptMain.h"

void execMessageLoop (Ref<ActorRuntime> runtime);

Ref<JSValue>    connectOperator (FunctionScope* pScope);
Ref<JSValue>    asCallHook( Ref<JSValue> function, 
                            Ref<FunctionScope> scope, 
                            ExecutionContext* ec, 
                            void* prevHook);
Ref<JSValue>    inputEpCall(Ref<AsEndPointRef> endPoint, Ref<FunctionScope> scope);
Ref<JSValue>    outputEpCall(Ref<AsEndPointRef> endPoint, Ref<FunctionScope> scope);
Ref<JSValue>    actorConstructor(Ref<AsActorClass> actorClass, Ref<FunctionScope> scope);


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
        auto            ownClass = AsActorClass::create("");
        RoutineActor*   newActor = new RoutineActor (ownClass, globals, parent);

        ownClass->getConstructor()->setNativePtr(routineActorExec);
        newActor->m_code = code;
        return AsActorRef::create(refFromNew(newActor));
        
    }

    /**
     * Executes the routine actor.
     * @param pScope
     * @return 
     */
    static Ref<JSValue> routineActorExec (FunctionScope* pScope)
    {
        auto actor = pScope->getThis().staticCast<RoutineActor>();
        
        return mvmExecute(actor->m_code, pScope->getGlobals(), Ref<IScope>(), asCallHook);
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
 * @return 
 */
Ref<JSValue> asBlockingExec (Ref<MvmRoutine> code, Ref<GlobalScope> globals)
{
    auto    rootActor = RoutineActor::create (code, globals, Ref<AsActorRef>());
    auto    runtime = ActorRuntime::create(rootActor);
    
    globals->newVar("@actorRT", runtime);
    addNative2("@connect", "dst", "src", connectOperator, globals);
    
    execMessageLoop (runtime);
    
    return rootActor->getResult();
}

/**
 * Executes the message loop, until there is no message left.
 * @param runtime
 */
void execMessageLoop (Ref<ActorRuntime> runtime)
{
    //TODO: This function must become more complex when taking I/O into account.
    while (runtime->dispatchMessage());
}

/**
 * The connect operator ('<-') establishes a connection between and output message
 * and an input message.
 * @param pScope
 * @return 
 */
Ref<JSValue> connectOperator (FunctionScope* pScope)
{
    auto runtime = pScope->getGlobals()->get("@actorRT");
    
    if (!runtime->isObject())
        error ("Missing actor runtime");
    
    auto src = pScope->getParam("src");
    auto dst = pScope->getParam("dst");
    
    if (src->getType() != VT_OUTPUT_EP_REF)
        error ("Source is not an output message");
    
    if (dst->getType() != VT_INPUT_EP_REF)
        error ("Destination is not an input message");
    
//    runtime.staticCast<ActorRuntime>()->connectMessages(
//        dst.staticCast<AsMessageRef>(), src.staticCast<AsMessageRef>());
    
    const auto srcActorRef = src.staticCast<AsEndPointRef>()->getActor();
    
    if (!srcActorRef->isRunning())
        return undefined();
    
    const auto srcActor = srcActorRef->getActor();
    const auto msgName = src.staticCast<AsEndPointRef>()->getEndPoint()->getName();
    
    srcActor->setOutputConnection (msgName, dst.staticCast<AsEndPointRef>());
    
    return undefined();    
}

/**
 * This function manages ALL MVM function calls when using actor runtime.
 * @param function
 * @param scope
 * @param ec
 * @param prevHook
 * @return 
 */
Ref<JSValue> asCallHook( Ref<JSValue> function, 
                            Ref<FunctionScope> scope, 
                            ExecutionContext* ec, 
                            void* prevHook)
{
    auto type = function->getType();
    
    if (type == VT_FUNCTION)
    {
        auto hook = reinterpret_cast<MvmCallHook>(prevHook);
        return hook(function, scope, ec, NULL);
    }
    
    switch (type)
    {
    case VT_INPUT_EP_REF:
        return inputEpCall(function.staticCast<AsEndPointRef>(), scope);
    case VT_OUTPUT_EP_REF:
        return outputEpCall(function.staticCast<AsEndPointRef>(), scope);
    case VT_ACTOR_CLASS:
        return actorConstructor (function.staticCast<AsActorClass>(), scope);
        
    default:
        error ("Invalid function type: %s", function->getTypeName().c_str());
        return undefined();        
    }
}

/**
 * Handles calls to input endpoints, which are executed as message sends
 * @param endPoint
 * @param scope
 * @return 
 */
Ref<JSValue> inputEpCall(Ref<AsEndPointRef> endPoint, Ref<FunctionScope> scope)
{
    auto globals = scope->getGlobals().staticCast<GlobalScope>();
    auto runtime = globals->get("@actorRT").staticCast<ActorRuntime>();
    auto params = globals->get("arguments").staticCast<JSArray>();
    
    runtime->sendMessage (endPoint, params);
    return undefined();
}

/**
 * Handles calls to output endpoints. It looks for the connected input end point,
 * and sends a message to it.
 * 
 * @param endPoint
 * @param scope
 * @return 
 */
Ref<JSValue> outputEpCall(Ref<AsEndPointRef> endPoint, Ref<FunctionScope> scope)
{
    //TODO: Check that it is invoked from the owning actor.
    auto actor = endPoint->getActor()->getActor();
    auto destination = actor->getConnectedEp(endPoint->getEndPoint()->getName());
    
    if (destination.isNull())
        return undefined();
    else
        return inputEpCall(destination, scope);    
}

/**
 * Handles actor constructors calls
 * @param actorClass
 * @param scope
 * @return 
 */
Ref<JSValue> actorConstructor(Ref<AsActorClass> actorClass, Ref<FunctionScope> scope)
{
    auto globals = scope->getGlobals().staticCast<GlobalScope>();
    auto runtime = globals->get("@actorRT").staticCast<ActorRuntime>();
    auto curActor = globals->get("@curActor").staticCast<AsActorRef>();
    auto newActor = AsActor::create(actorClass, globals, curActor);
    auto constructor = actorClass->getConstructor();
    auto params = constructor->getParams();
    auto msgParams = JSArray::create();
    
    //Parameters become actor fields, and parameters for 'start' message
    for (size_t i=0; i<params.size(); ++i)
    {
        auto value = scope->getParam(params[i]);
        newActor->writeFieldStr(params[i], value);
        msgParams->push(value);
    }
    
    auto actorRef = AsActorRef::create(newActor);
    
    //Send start message
    runtime->sendMessage(actorRef, "@start", msgParams);
    
    return actorRef;
}

/**
 * Creates the actor runtime.
 * @param rootActor
 * @return 
 */
Ref<ActorRuntime> ActorRuntime::create(Ref<AsActorRef> rootActor)
{
    auto result = refFromNew(new ActorRuntime(rootActor));
    
    result->sendMessage0 (rootActor, "@start");    
    
    return result;
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
                                 Ref<JSValue> p1)
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
                                 Ref<JSValue> p1,
                                 Ref<JSValue> p2)
{
    auto params = JSArray::create();
    
    params->push(p1);
    params->push(p2);

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
        error ("End point '%s' does not exist", epName.c_str());
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
        error ("Destination is not an input end point");
    
    m_messageQueue.push_back(SMessage(dstEndPoint, params));
}

/**
 * Dispatch next message in the queue.
 * @return true if a message has been processed, false if the message queue is 
 * empty.
 */
bool ActorRuntime::dispatchMessage()
{
    if (m_messageQueue.empty())
        return false;
    
    SMessage    msg = m_messageQueue.front();
    auto        actorRef = msg.destination->getActor();

    m_messageQueue.pop_front();
    
    if (actorRef->isRunning())
    {
        try
        {
            auto actor = actorRef->getActor();
            auto globals = actor->getGlobals();
            auto endPoint = msg.destination->getEndPoint();
            auto scope = FunctionScope::create(globals, endPoint);

            globals->newVar("@curActor", actorRef);
            scope->setThis(actor);
            
            for (size_t i = 0; i < msg.params->length(); ++i)
                scope->addParam(msg.params->getAt(i));
            
            if (endPoint->isNative())
                endPoint->nativePtr()(scope.getPointer());
            else
            {
                auto routine = endPoint->getCodeMVM().staticCast<MvmRoutine>();
                mvmExecute (routine, globals, scope, asCallHook);
            }
        }
        catch (CScriptException& ex)
        {
            actorCrashed (actorRef, ex);
        }        
    }
    
    return true;
}

/**
 * Handles exceptions inside actors.
 * @param actorRef
 * @param ex
 */
void ActorRuntime::actorCrashed(Ref<AsActorRef> actorRef, const CScriptException& ex)
{
    auto actor = actorRef->getActor();
    actor->forceStop();

    auto parent = actor->getParent();
    
    if (parent.notNull())
    {
        sendMessage2 (parent, "childStopped", jsTrue(), jsString(ex.what()));
    }
    else
        fprintf (stderr, "Root actor crashed: %s\n", ex.what());
}

Ref<JSObject> ActorRuntime::clone (bool _mutable)
{
    ASSERT (!"clone operation not allowed on 'ActorRuntime'");
    return Ref<JSObject>(this);
}

