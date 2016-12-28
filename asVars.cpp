/* 
 * File:   asVars.cpp
 * Author: ghernan
 * 
 * Async script basic data types. Contains the data types related to actor system.
 * The rest are in 'jsVars.*'
 * 
 * Created on December 25, 2016, 12:53 PM
 */

#include "OS_support.h"
#include "asVars.h"

/**
 * Actor class constructor.
 * @param name
 */
AsActorClass::AsActorClass (const std::string& name) 
: JSObject(DefaultPrototype, MT_MUTABLE)
, m_name (name)
{
    auto constructor = AsEndPoint::create("@start", true);
    
    writeFieldStr(constructor->getName(), constructor);
}

AsActorClass::AsActorClass(const AsActorClass& src, bool _mutable)
: JSObject(src, _mutable)
, m_name (src.m_name)
{    
}

Ref<JSObject> AsActorClass::clone (bool _mutable)
{
    return refFromNew (new AsActorClass(*this, _mutable));
}

/**
 * Gets one of the endpoints defined in the class.
 * @param name
 * @return 
 */
Ref<AsEndPoint> AsActorClass::getEndPoint (const std::string& name)
{
    auto item = readFieldStr(name);
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
Ref<AsEndPointRef> AsActorRef::getEndPoint (const std::string& name)
{
    auto ep = m_ref->getEndPoint(name);

    if (ep.isNull())
        return Ref<AsEndPointRef>();
    else
        return AsEndPointRef::create(ep, Ref<AsActorRef>(this));
}

Ref<JSObject> AsActor::clone (bool _mutable)
{
    ASSERT (!"Clone operation not allowed on actors.");
    return Ref<AsActor>(this);
}

   
AsEndPoint::AsEndPoint(const AsEndPoint& src, bool _mutable)
: JSFunction(src, _mutable)
, m_isInput(src.m_isInput)
{
}

Ref<JSObject> AsEndPoint::clone (bool _mutable)
{
    return refFromNew (new AsEndPoint(*this, _mutable));
}

/**
 * Looks for the connected input end point of an output end point
 * @param msgName
 * @return 
 */
Ref<AsEndPointRef> AsActor:: getConnectedEp (const std::string& msgName)const
{
    auto    it = m_outputConections.find(msgName);
    
    if (it != m_outputConections.end())
        return it->second;
    else
        return Ref<AsEndPointRef>();
}
