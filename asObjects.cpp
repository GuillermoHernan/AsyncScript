/* 
 * File:   asObjects.cpp
 * Author: ghernan
 * 
 * Support for object oriented programming in AsyncScript. 
 * Contains the runtime objects for AsyncScript classes and objects
 * 
 * Created on January 3, 2017, 11:16 AM
 */

#include "ascript_pch.hpp"
#include "asObjects.h"
#include "scriptMain.h"
#include "microVM.h"
#include "jsArray.h"
#include "ScriptException.h"

using namespace std;

Ref<JSClass> createObjectClass();

Ref<JSClass> JSObject::DefaultClass = createObjectClass();


// JSClass
//
//////////////////////////////////////////////////

/**
 * Constructor.
 * @param name
 * @param parent
 * @param members
 * @param constructorFn
 */
JSClass::JSClass(const std::string& name,
                 Ref<JSClass> parent,
                 const VarMap& members,
                 Ref<JSFunction> constructorFn) :
m_name(name), m_members(members), m_parent(parent), m_constructor(constructorFn)
{
}

Ref<JSClass> JSClass::create (const std::string& name, 
                            Ref<JSClass> parent, 
                            const VarMap& members, 
                            Ref<JSFunction> constructorFn)
{
    return refFromNew(new JSClass(name, parent, members, constructorFn));
}


Ref<JSClass> JSClass::createNative (const std::string& name, 
                                            Ref<JSClass> parent, 
                                            const VarMap& members, 
                                            const StringVector& params,
                                            JSNativeFn nativeFn)
{
    auto native = JSFunction::createNative("@constructor", params, nativeFn);
    
    return refFromNew(new JSClass(name, parent, members, native));
}


/**
 * Gets all fields of the class
 * @param inherited     Include inherited fields
 * @return 
 */
StringSet JSClass::getFields(bool inherited)const
{
    StringSet    result;
    
    if (inherited && m_parent.notNull())
        result = m_parent->getFields();
    
    for (auto it = m_members.begin(); it != m_members.end(); ++it)
        result.insert(it->first);
    
    return result;
}


/**
 * Reads a field from a class
 * @param key
 * @return 
 */
ASValue JSClass::readField(const std::string& key)const
{
    auto it = m_members.find(key);

    if (it != m_members.end())
        return it->second.value();
    else if (m_parent.notNull())
        return m_parent->readField (key);
    else
        return jsNull();
}

/**
 * Call to a class object. Creates a new instance, and calls the constructor.
 * @param scope
 * @return 
 */
//ASValue JSClass::call (Ref<FunctionScope> scope)
//{
//    auto result = m_constructor->call(scope);
//    
//    if (result.isNull() || !result->isObject())
//        rtError ("Constructor must return an object");
//    
//    auto newObj = result.staticCast<JSObject>();
//    
//    newObj->setClass (ref(this));
//    
//    return result;
//}

// JSObject
//
//////////////////////////////////////////////////


JSObject::JSObject(Ref<JSClass> cls, JSMutability mutability) : 
m_cls (cls), m_mutability(mutability)
{
    ASSERT(cls.notNull());
}


JSObject::~JSObject()
{
    //printf ("Destroying object: %s\n", this->getJSON(0).c_str());
}

/**
 * Creates an empty object, with default class
 * @return 
 */
Ref<JSObject> JSObject::create()
{
    return refFromNew(new JSObject(DefaultClass, MT_MUTABLE));
}

/**
 * Creates an empty JSON object
 * @return 
 */
Ref<JSObject> JSObject::create(Ref<JSClass> cls)
{
    return refFromNew(new JSObject(cls, MT_MUTABLE));
}

/**
 * Creates a frozen copy of an object
 */
ASValue JSObject::freeze()
{
    if (getMutability() == MT_MUTABLE)
        return clone (false)->value();
    else
        return value();
}

/**
 * Creates a 'deep-frozen' copy of the object, by making a frozen copy of the object
 * and all referenced objects which are not 'deep-frozen' recursively.
 * @param transformed
 * @return 
 */
ASValue JSObject::deepFreeze(ASValue::ValuesMap& transformed)
{
    auto me = value();
    
    if (m_mutability == MT_DEEPFROZEN)
        return me;

    auto it = transformed.find(me);
    if (it != transformed.end())
        return it->second;

    //Clone object
    auto newObject = JSObject::create(m_cls);
    transformed[me] = newObject->value();
    
    for (auto it = m_members.begin(); it != m_members.end(); ++it)
    {
        auto value = it->second.value().deepFreeze(transformed);
        newObject->writeField(it->first, value, it->second.isConst());
    }

    newObject->m_mutability = MT_DEEPFROZEN;

    return newObject->value();
}


/**
 * Creates a mutable copy of an object
 * @param forceClone
 * @return 
 */
ASValue JSObject::unFreeze(bool forceClone)
{
    if (forceClone || getMutability() == MT_MUTABLE)
        return clone (true)->value();
    else
        return value();
}

/**
 * Transforms the object into an immutable object. The transformation is made in
 * place, no copy is performed.
 * Watch-out for side-effects.
 */
void JSObject::setFrozen()
{
    m_mutability = selectMutability(*this, false);
}

/**
 * Gets a vector with all object keys
 * @return 
 */
std::vector <ASValue > JSObject::getKeys()const
{
    std::vector <ASValue >     result;
    
    result.reserve(m_members.size());
    for (auto it = m_members.begin(); it != m_members.end(); ++it)
        result.push_back(jsString(it->first));
    
    return result;
}

/**
 * Gets a vector with all object keys (string format)
 * @return 
 */
StringSet JSObject::getFields(bool inherited)const
{
    StringSet result;
    
    if (inherited)
        result = m_cls->getFields(true);

    for (auto it = m_members.begin(); it != m_members.end(); ++it)
        result.insert(it->first);
    
    return result;
}

/**
 * Handles array access operator reads
 * @param index
 * @return 
 */
ASValue JSObject::getAt(ASValue index, ExecutionContext* ec)
{
    ASValue fn = jsNull();
    
    if (ec != NULL)
        fn = readField("getAt");
    
    if (!fn.isNull())
        return callMemberFn(fn, index, ec);
    else        
        return readField(index.toString(ec));
}

/**
 * Handles array access operator writes
 * @param index
 * @return 
 */
ASValue JSObject::setAt(ASValue index, ASValue value, ExecutionContext* ec)
{
    ASValue fn = jsNull();
    
    if (ec != NULL)
        fn = readField("setAt");
    
    if (!fn.isNull())
        return callMemberFn(fn, index, value, ec);
    else        
        return writeField(index.toString(ec), value, false);
}

/**
 * Returns an iterator to walk object contents.
 * It is script-overridable. The default implementation returns a
 * single item iterator.
 * @return 
 */
ASValue JSObject::iterator(ExecutionContext* ec)const
{
    ASValue fn = jsNull();
    
    if (ec != NULL)
        fn = readField("iterator");
    
    if (!fn.isNull())
        return callMemberFn(fn, ec);
    else        
        return singleItemIterator(const_cast<JSObject*>(this)->value());
}

/**
 * Handles function calls, which allows to use objects as functions.
 * Default implementation just returns 'null'
 * @param scope
 * @return 
 */
//ASValue JSObject::call (Ref<FunctionScope> scope)
//{
//    auto fn = readField("call");
//    
//    if (!fn->isNull())
//    {
//        //Create a new scope, to map argument names to the new function.
//        auto newScope = FunctionScope::create(fn, ref(this), scope->getParams());
//        
//        return fn->call(newScope);
//    }
//    else
//        return jsNull();
//}

/**
 * Copy constructor.
 * @param src       Reference to the source object
 * @param _mutable  
 */
JSObject::JSObject(const JSObject& src, bool _mutable)
: m_members (src.m_members)
, m_cls (src.m_cls)
, m_mutability (selectMutability(src, _mutable))
{
}


/**
 * Creates a copy of the object
 * @param _mutable   Controls if the copy will be mutable or not. In case of not 
 * being mutable, it will be 'frozen' or 'deepFrozen' depending on the mutability 
 * state of the object members.
 * @return 
 */
Ref<JSObject> JSObject::clone (bool _mutable)
{
    return refFromNew(new JSObject(*this, _mutable));
}

/**
 * Chooses the appropriate mutability state for the new object on a clone operation
 * @param src
 * @param _mutable
 * @return 
 */
JSMutability JSObject::selectMutability(const JSObject& src, bool _mutable)
{
    if (_mutable)
        return MT_MUTABLE;
    else
    {
        //Check if all children are 'deepfrozen'
        auto items = src.m_members;
        for (auto it = items.begin(); it != items.end(); ++it)
        {
            if (it->second.value().getMutability() != MT_DEEPFROZEN)
                return MT_FROZEN;
        }
        return MT_DEEPFROZEN;
    }
}

/**
 * Calls a member function defined in the script code.
 * @param function
 * @return 
 */
ASValue JSObject::callMemberFn (ASValue function, ExecutionContext* ec)const
{
    ASValue me = const_cast<JSObject*>(this)->value();
    
    ec->push(me);
    ec->push(function);    
    mvmExecCall(1, ec);
    
    return ec->pop();
}

/**
 * Calls a member function defined in the script code.
 * @param function
 * @return 
 */
ASValue JSObject::callMemberFn (ASValue function, ASValue p1, ExecutionContext* ec)const
{
    ASValue me = const_cast<JSObject*>(this)->value();
    
    ec->push(p1);
    ec->push(me);
    ec->push(function);    
    mvmExecCall(1, ec);
    
    return ec->pop();
}

/**
 * Calls a member function defined in the script code.
 * @param function
 * @return 
 */
ASValue JSObject::callMemberFn (ASValue function, ASValue p1, ASValue p2, ExecutionContext* ec)const
{
    ASValue me = const_cast<JSObject*>(this)->value();
    
    ec->push(p1);
    ec->push(p2);
    ec->push(me);
    ec->push(function);    
    mvmExecCall(1, ec);
    
    return ec->pop();
}

bool JSObject::isWritable(const std::string& key)const
{
    //TODO: Check class fields
    if (getMutability() != MT_MUTABLE)
        return false;
    
    auto it = m_members.find(key);
    if (it == m_members.end())
        return true;
    else
        return !it->second.isConst();
}

/**
 * String representation of an object. Calls 'toString' script function, if defined.
 * @return 
 */
std::string JSObject::toString(ExecutionContext* ec)const
{
    ASValue fn = jsNull();
    
    if (ec != NULL)
        fn = readField("toString");
    
    if (!fn.isNull())
        return callMemberFn(fn, ec).toString(ec);
    else        
        return std::string("[Object of ") + m_cls->toString() + "]";
}

/**
 * Transforms the object into a boolean value. Calls 'toBoolean' script function, if defined.
 * @return 
 */
bool JSObject::toBoolean(ExecutionContext* ec)const
{
    ASValue fn = jsNull();
    
    if (ec != NULL)
        fn = readField("toBoolean");
    
    if (!fn.isNull())
        return callMemberFn(fn, ec).toBoolean(ec);
    else        
        return true;
}

/**
 * Transforms the object into a 'double' value. Calls 'toNumber' script function, if defined.
 * @return 
 */
double JSObject::toDouble(ExecutionContext* ec)const
{
    ASValue fn = jsNull();
    
    if (ec != NULL)
        fn = readField("toNumber");
    
    if (!fn.isNull())
        return callMemberFn(fn, ec).toDouble(ec);
    else        
        return getNaN();
}

//ASValue JSObject::toFunction()
//{
//    return jsNull();
//}

/**
 * Reads a field of the object. If it does not exist, it returns 'null'
 * @param key
 * @return 
 */
ASValue JSObject::readField(const std::string& key)const
{
    auto it = m_members.find(key);

    if (it != m_members.end())
        return it->second.value();
    else
        return m_cls->readField (key);
}

/**
 * Sets the value of a member, or creates it if not already present.
 * @param name
 * @param value
 * @param isConst
 * @return 
 */
ASValue JSObject::writeField(const std::string& key, 
                                  ASValue value, 
                                  bool isConst)
{
    if (!isWritable(key))
        return readField(key);
    
    m_members[key] = VarProperties(value, isConst);

    return value;
}

/**
 * Deletes a field from the object
 * @param key
 * @return 
 */
ASValue JSObject::deleteField(const std::string& key)
{
    if (!isWritable(key))
        return readField(key);

    auto it = m_members.find(key);
    
    if (it == m_members.end())
        return jsNull();
    else
    {
        auto result = it->second.value();
        m_members.erase(it);
        return result;
    }
}

/**
 * Generates a JSON representation of the object
 * @return JSON string
 */
std::string JSObject::getJSON(int indent)
{
    ostringstream output;
    bool first = true;

    //{"x":2}
    output << "{";

    for (auto it = m_members.begin(); it != m_members.end(); ++it)
    {
        string childJSON = it->second.value().getJSON(indent+1);

        if (!childJSON.empty())
        {
            if (!first)
                output << ",";
            else
                first = false;

            output << "\n" << indentText(indent+1) << "\"" << it->first << "\":";
            output << childJSON;
        }
    }

    if (!first)
        output << "\n" << indentText(indent) << "}";
    else
        output << "}";

    return output.str();
}

ASValue scObjectFreeze(ExecutionContext* ec)
{
    auto obj = ec->getLastParam();
    
    return obj.freeze();
}

ASValue scObjectDeepFreeze(ExecutionContext* ec)
{
    auto obj = ec->getLastParam();
    
    return obj.deepFreeze();
}

ASValue scObjectUnfreeze(ExecutionContext* ec)
{
    auto obj = ec->getLastParam();
    auto forceClone = ec->getParam(0);
    
    return obj.unFreeze(forceClone.toBoolean(ec));
}

ASValue scObjectIsFrozen(ExecutionContext* ec)
{
    auto obj = ec->getLastParam();
    
    return jsBool (!obj.isMutable());
}

ASValue scObjectIsDeepFrozen(ExecutionContext* ec)
{
    auto obj = ec->getLastParam();
    
    return jsBool (obj.getMutability() == MT_DEEPFROZEN);
}

ASValue scObjectConstructor(ExecutionContext* ec)
{
    return JSObject::create()->value();
}

/**
 * Creates the class object for the 'Object' class
 * @return 
 */
Ref<JSClass> createObjectClass()
{
    VarMap  members;

    addNative("function freeze()", scObjectFreeze, members);
    addNative("function deepFreeze()", scObjectDeepFreeze, members);
    addNative("function unfreeze(forceClone)", scObjectUnfreeze, members);
    addNative("function isFrozen(forceClone)", scObjectIsFrozen, members);
    addNative("function isDeepFrozen(forceClone)", scObjectIsDeepFrozen, members);
    
    return JSClass::createNative("Object", 
                                 JSObject::DefaultClass, 
                                 members, 
                                 StringVector(),
                                 scObjectConstructor);
}
