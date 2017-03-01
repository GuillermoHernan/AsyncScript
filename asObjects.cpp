/* 
 * File:   asObjects.cpp
 * Author: ghernan
 * 
 * Support for object oriented programming in AsyncScript. 
 * Contains the runtime objects for AsyncScript classes and objects
 * 
 * Created on January 3, 2017, 11:16 AM
 */

#include "OS_support.h"
#include "asObjects.h"
#include "executionScope.h"
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
Ref<JSValue> JSClass::readField(const std::string& key)const
{
    auto it = m_members.find(key);

    if (it != m_members.end())
        return it->second.value();
    else if (m_parent.notNull())
        return m_parent->readField (key);
    else
        return JSValue::readField(key);
}

/**
 * Call to a class object. Creates a new instance, and calls the constructor.
 * @param scope
 * @return 
 */
Ref<JSValue> JSClass::call (Ref<FunctionScope> scope)
{
    auto result = m_constructor->call(scope);
    
    if (result.isNull() || !result->isObject())
        error ("Constructor must return an object");
    
    auto newObj = result.staticCast<JSObject>();
    
    newObj->setClass (ref(this));
    
    return result;
}

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
Ref<JSValue> JSObject::freeze()
{
    if (isMutable())
        return clone (false);
    else
        return Ref<JSValue>(this);
}

/**
 * Creates a 'deep-frozen' copy of the object, by making a frozen copy of the object
 * and all referenced objects which are not 'deep-frozen' recursively.
 * @param transformed
 * @return 
 */
Ref<JSValue> JSObject::deepFreeze(JSValuesMap& transformed)
{
    auto me = ref(this);
    
    if (m_mutability == MT_DEEPFROZEN)
        return me;

    auto it = transformed.find(me);
    if (it != transformed.end())
        return it->second;

    //Clone object
    auto newObject = JSObject::create(m_cls);
    transformed[me] = newObject;
    
    for (auto it = m_members.begin(); it != m_members.end(); ++it)
    {
        auto value = it->second.value()->deepFreeze(transformed);
        newObject->writeField(it->first, value, it->second.isConst());
    }

    newObject->m_mutability = MT_DEEPFROZEN;

    return newObject;
}


/**
 * Creates a mutable copy of an object
 * @param forceClone
 * @return 
 */
Ref<JSValue> JSObject::unFreeze(bool forceClone)
{
    if (forceClone || !isMutable())
        return clone (true);
    else
        return Ref<JSValue>(this);
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
std::vector <Ref<JSValue> > JSObject::getKeys()const
{
    std::vector <Ref<JSValue> >     result;
    
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
Ref<JSValue> JSObject::indexedRead(Ref<JSValue> index)
{
    auto fn = readField("indexedRead");
    
    if (!fn->isNull())
        return callMemberFn(fn, index);
    else
        return readField(index->toString());
}

/**
 * Handles array access operator writes
 * @param index
 * @return 
 */
Ref<JSValue> JSObject::indexedWrite(Ref<JSValue> index, Ref<JSValue> value)
{
    auto fn = readField("indexedWrite");
    
    if (!fn->isNull())
        return callMemberFn(fn, index, value);
    else
        return writeField(index->toString(), value, false);
}

/**
 * Gets the head element of a sequence. The default implementation just returns
 * a reference to the object.
 * @return 
 */
Ref<JSValue> JSObject::head()
{
    auto fn = readField("head");
    
    if (!fn->isNull())
        return callMemberFn(fn);
    else
        return ref(this);
}

/**
 * Gets the next elements of a sequence. The default implementation just returns null.
 * @return 
 */
Ref<JSValue> JSObject::tail()
{
    auto fn = readField("tail");
    
    if (!fn->isNull())
        return callMemberFn(fn);
    else
        return jsNull();
}

/**
 * Handles function calls, which allows to use objects as functions.
 * Default implementation just returns 'null'
 * @param scope
 * @return 
 */
Ref<JSValue> JSObject::call (Ref<FunctionScope> scope)
{
    auto fn = readField("call");
    
    if (!fn->isNull())
    {
        //Create a new scope, to map argument names to the new function.
        auto newScope = FunctionScope::create(fn, ref(this), scope->getParams());
        
        return fn->call(newScope);
    }
    else
        return jsNull();
}

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
            if (it->second.value()->getMutability() != MT_DEEPFROZEN)
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
Ref<JSValue> JSObject::callMemberFn (Ref<JSValue> function)const
{
    auto    me = ref(const_cast<JSObject*>(this));
    auto    fnScope = FunctionScope::create(function, me, ValueVector());
    return function->call(fnScope);
}

/**
 * Calls a member function defined in the script code.
 * One parameter version
 * @param function
 * @return 
 */
Ref<JSValue> JSObject::callMemberFn (Ref<JSValue> function, Ref<JSValue> p1)const
{
    auto    me = ref(const_cast<JSObject*>(this));
    ValueVector params;
    params.push_back(p1);
    
    auto    fnScope = FunctionScope::create(function, me, params);
    return function->call(fnScope);
}

/**
 * Calls a member function defined in the script code.
 * One parameter version
 * @param function
 * @return 
 */
Ref<JSValue> JSObject::callMemberFn (Ref<JSValue> function, 
                                     Ref<JSValue> p1,
                                     Ref<JSValue> p2)const
{
    auto    me = ref(const_cast<JSObject*>(this));
    ValueVector params;
    params.push_back(p1);
    params.push_back(p2);
    
    auto    fnScope = FunctionScope::create(function, me, params);
    return function->call(fnScope);
}


bool JSObject::isWritable(const std::string& key)const
{
    //TODO: Check class fields
    if (!isMutable())
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
std::string JSObject::toString()const
{
    auto fn = readField ("toString");
    
    if (!fn->isNull())
        return callMemberFn(fn)->toString();
    else        
        return std::string("[Object of ") + m_cls->toString() + "]";
}

/**
 * Transforms the object into a boolean value. Calls 'toBoolean' script function, if defined.
 * @return 
 */
bool JSObject::toBoolean()const
{
    auto fn = readField ("toBoolean");
    
    if (!fn->isNull())
        return callMemberFn(fn)->toBoolean();
    else        
        return true;
}

/**
 * Transforms the object into a 'double' value. Calls 'toNumber' script function, if defined.
 * @return 
 */
double JSObject::toDouble()const
{
    auto fn = readField ("toNumber");
    
    if (!fn->isNull())
        return callMemberFn(fn)->toDouble();
    else        
        return getNaN();
}


/**
 * Reads a field of the object. If it does not exist, it returns 'null'
 * @param key
 * @return 
 */
Ref<JSValue> JSObject::readField(const std::string& key)const
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
Ref<JSValue> JSObject::writeField(const std::string& key, 
                                  Ref<JSValue> value, 
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
Ref<JSValue> JSObject::deleteField(const std::string& key)
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
        string childJSON = it->second.value()->getJSON(indent+1);

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

Ref<JSValue> scObjectFreeze(FunctionScope* pScope)
{
    auto obj = pScope->getThis();
    
    return obj->freeze();
}

Ref<JSValue> scObjectDeepFreeze(FunctionScope* pScope)
{
    auto obj = pScope->getThis();
    
    return obj->deepFreeze();
}

Ref<JSValue> scObjectUnfreeze(FunctionScope* pScope)
{
    auto obj = pScope->getThis();
    auto forceClone = pScope->getParam("forceClone");
    
    return obj->unFreeze(forceClone->toBoolean());
}

Ref<JSValue> scObjectIsFrozen(FunctionScope* pScope)
{
    auto obj = pScope->getThis();
    
    return jsBool (!obj->isMutable());
}

Ref<JSValue> scObjectIsDeepFrozen(FunctionScope* pScope)
{
    auto obj = pScope->getThis();
    
    return jsBool (obj->getMutability() == MT_DEEPFROZEN);
}

Ref<JSValue> scObjectConstructor(FunctionScope* pScope)
{
    return JSObject::create();
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
