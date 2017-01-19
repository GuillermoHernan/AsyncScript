/* 
 * File:   asVars.cpp
 * Author: ghernan
 * 
 * Async script basic data types. Contains the data types related to actor system.
 * 
 * Created on December 25, 2016, 12:53 PM
 */

#include "OS_support.h"
#include "asVars.h"
#include "actorRuntime.h"

using namespace std;

//Ref<JSClass> createActorBaseClass();
//
//Ref<JSClass> AsActorClass::ActorBaseClass = createActorBaseClass();

/**
 * Actor class construction function
 * @param name
 * @param members
 * @return 
 */
Ref<AsActorClass> AsActorClass::create (const std::string& name, 
                                        const VarMap& members,
                                        const StringVector& params)
{
    auto newMembers = createDefaultEndPoints(members);
//    auto constructor = AsEndPoint::create("@start", true);
//    
//    writeFieldStr(constructor->getName(), constructor);
    return refFromNew (new AsActorClass(name, newMembers, params));
}

/**
 * Actor class constructor.
 * @param name
 */
AsActorClass::AsActorClass(const std::string& name,
                           const VarMap& members,
                           const StringVector& params)
: m_name(name),
m_members(members),
m_params(params)
{
}

/**
 * Gets the function used to create actor objects (but not the one used to initialize them)
 * @return 
 */
//Ref<JSFunction> AsActorClass::getActorObjConstructor()
//{
//    static Ref<JSFunction> constructor = JSFunction::createNative("@constructor", StringVector(), actorObjConstructor);
//    
//    return constructor;
//}
//
//static Ref<JSValue> AsActorClass::actorObjConstructor (FunctionScope* pScope)
//{
//    
//}


/**
 * Creates default endPoints of an actor class, if they are missing.
 */
VarMap AsActorClass::createDefaultEndPoints (const VarMap& members)
{
    const char* childStopped = "childStopped";
    if (members.find(childStopped) == members.end())
    {
        StringVector    params;
        auto            newMembers = members;
        
        params.push_back("child");
        params.push_back("result");
        params.push_back("error");
        auto            endPoint = AsEndPoint::createNative(childStopped, 
                                                            params,
                                                            actorChildStoppedDefaultHandler);
        
        newMembers[childStopped] = VarProperties(endPoint, true);
        
        return newMembers;
    }
    else
        return members;
}

/**
 * Call to an actor class (invokes the actor constructor).
 * @param scope
 * @param ec
 * @return 
 */
Ref<JSValue> AsActorClass::call (Ref<FunctionScope> scope)
{
    return actorConstructor(Ref<AsActorClass>(this), scope);
}


/**
 * Gets one of the endpoints defined in the class.
 * @param name
 * @return 
 */
Ref<AsEndPoint> AsActorClass::getEndPoint (const std::string& name)
{
    auto it = m_members.find(name);
    
    if (it == m_members.end())
        return Ref<AsEndPoint>();
    
    auto item = it->second.value();
    auto type = item->getType();
    
    if (type == VT_INPUT_EP || type == VT_OUTPUT_EP)
        return item.staticCast<AsEndPoint>();
    else
        return Ref<AsEndPoint>();
}

/**
 * Gets one of the referenced actor end points
 * @param name
 * @return 
 */
Ref<AsEndPointRef> AsActorRef::getEndPoint (const std::string& name)const
{
    auto ep = m_ref->getEndPoint(name);

    if (ep.isNull())
        return Ref<AsEndPointRef>();
    else
        return AsEndPointRef::create(ep, Ref<AsActorRef>(const_cast<AsActorRef*>(this)));
}

/**
 * Stops an actor execution
 * @param result
 * @param error
 */
void AsActor::stop(Ref<JSValue> result, Ref<JSValue> error)
{
    m_result = result;
    m_error = error;
    m_finished = true;
}


//Ref<JSObject> AsActor::clone (bool _mutable)
//{
//    ASSERT (!"Clone operation not allowed on actors.");
//    return Ref<AsActor>(this);
//}

AsEndPoint::AsEndPoint (const std::string& name, 
                        const StringVector& params, 
                        Ref<MvmRoutine> code) :
                        JSFunction(name, params, code), m_isInput(true)
{    
}

AsEndPoint::AsEndPoint (const std::string& name, 
                        const StringVector& params, 
                        JSNativeFn pNative):
                        JSFunction(name, params, pNative), m_isInput(true)
{    
}

/**
 * String representation of an end point.
 * @return 
 */
std::string AsEndPoint::toString()const
{
    const string header = isInput() ? "input" : "output";
    
    return header + JSFunction::toString().substr(8);
}

/**
 * Call made to a end point reference (will send a message)
 * @param scope
 * @return 
 */
Ref<JSValue> AsEndPointRef::call (Ref<FunctionScope> scope)
{
    if (isInput())
        return inputEpCall(Ref<AsEndPointRef>(this), scope);
    else
        return outputEpCall(Ref<AsEndPointRef>(this), scope);
}

/**
 * Reads a field from an actor.
 * @param key
 * @return 
 */
Ref<JSValue> AsActor::readField(Ref<JSValue> key)const
{
    string keyStr = key2Str(key);
    auto it = m_members.find(keyStr);
    
    // if not found at object fields, it may be an endpoint
    if (it == m_members.end())
    {
        auto ep = getEndPoint(key->toString());
        
        if (ep.notNull())
            return AsEndPointRef::create(ep, AsActorRef::create(Ref<AsActor>(const_cast<AsActor*>(this))));
        else
            return undefined();
    }
    else
        return it->second.value();
}

/**
 * Writes a field
 * @param key
 * @param value
 * @return 
 */
Ref<JSValue> AsActor::writeField(Ref<JSValue> key, Ref<JSValue> value)
{
    //TODO: Check class fields
    checkedVarWrite(m_members, key2Str(key), value, false);
    return value;
}

/**
 * Creates a new constant field
 * @param key
 * @param value
 * @return 
 */
Ref<JSValue> AsActor::newConstField(Ref<JSValue> key, Ref<JSValue> value)
{
    //TODO: Check class fields
    checkedVarWrite(m_members, key2Str(key), value, true);
    return value;
}

/**
 * Looks for the connected input end point of an output end point
 * @param msgName
 * @return 
 */
Ref<AsEndPointRef> AsActor::getConnectedEp (const std::string& msgName)const
{
    auto    it = m_outputConections.find(msgName);
    
    if (it != m_outputConections.end())
        return it->second;
    else
        return Ref<AsEndPointRef>();
}


//Ref<JSValue> scActorConstructor(FunctionScope* pScope)
//{
//    return AsActor::c ::create();
//}
//
//
//Ref<JSClass> createActorBaseClass()
//{
//    VarMap  members;
//    
//    
//
//    return JSClass::create("Actor", Ref<JSClass>(), members, StringVector());
//}
