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
#include "TinyJS_Functions.h"
#include "asObjects.h"
#include "executionScope.h"
#include "scriptMain.h"
#include "mvmFunctions.h"

#include <math.h>

using namespace std;

Ref<JSClass> createObjectClass();
Ref<JSClass> createArrayClass();

Ref<JSClass> JSObject::DefaultClass = createObjectClass();
Ref<JSClass> JSArray::ArrayClass = createArrayClass();


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
        return jsNull();    
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
        scope->setThis(ref(this));
        return fn->call(scope);
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
    auto    fnScope = FunctionScope::create(function);
    fnScope->setThis(ref(const_cast<JSObject*>(this)));
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
    auto    fnScope = FunctionScope::create(function);
    fnScope->setThis(ref(const_cast<JSObject*>(this)));
    fnScope->addParam(p1);
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
    auto    fnScope = FunctionScope::create(function);
    fnScope->setThis(ref(const_cast<JSObject*>(this)));
    fnScope->addParam(p1);
    fnScope->addParam(p2);
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

std::string indentText(int indent)
{
    std::string result;
    
    result.reserve(indent * 2);
    
    for (int i=0; i < indent; ++i)
        result += "  ";
    
    return result;
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


// JSArray
//
//////////////////////////////////////////////////

/**
 * Creates an array object in the heap.
 * @return 
 */
Ref<JSArray> JSArray::create()
{
    return refFromNew(new JSArray);
}

/**
 * Creates an array of a given initial length.
 * All elements in the array will be set to 'null'
 * @param size
 * @return 
 */
Ref<JSArray> JSArray::create(size_t size)
{
    Ref<JSArray>    a = refFromNew(new JSArray);

    a->m_content.resize(size, jsNull());
    
    return a;
}

/**
 * Translates a C++ string vector into a Javascript string array
 * @param strList
 * @return 
 */
Ref<JSArray> JSArray::createStrArray(const std::vector<std::string>& strList)
{
    Ref<JSArray>    arr = JSArray::create();
    
    for (size_t i=0; i < strList.size(); ++i)
        arr->push(jsString(strList[i]));
    
    return arr;
}


/**
 * Adds a value to the end of the array
 * @param value
 * @return Returns new array size
 */
size_t JSArray::push(Ref<JSValue> value)
{
    if (isMutable())
        m_content.push_back(value);
    
    return m_content.size();
}

/**
 * Gets an element located at an array position
 * @param index
 * @return 
 */
Ref<JSValue> JSArray::getAt(size_t index)const
{
    if (index >= m_content.size())
        return jsNull();
    else
        return m_content[index];
}


/**
 * JSArray 'get' override, to implement 'length' property reading.
 * @param name
 * @return 
 */
Ref<JSValue> JSArray::readField(const std::string& key)const
{
    if (key == "length")
        return jsSizeT(m_content.size());
    else
        return ArrayClass->readField(key);
}

/**
 * Array 'writeField' method override. Handles:
 * - Length overwrite, which changes array length
 * - Write an element past the last one, which enlarges the array.
 * @param key
 * @param value
 * @param isConst
 * @return 
 */
Ref<JSValue> JSArray::writeField(const std::string& key, Ref<JSValue> value, bool isConst)
{
    if (!isMutable())
        return readField(key);
    
    if (key == "length")
    {
        setLength(value);
        return jsSizeT(m_content.size());
    }
    else
        return jsNull();
}

/**
 * Reads an element of the array
 * @param index
 * @return 
 */
Ref<JSValue> JSArray::indexedRead(Ref<JSValue> index)
{
    if (!isUint(index))
        return jsNull();
    else
        return getAt( toSizeT (index) );
}

/**
 * Returns the first element of the array
 * @return 
 */
Ref<JSValue> JSArray::head()
{
    return getAt(0);
}

/**
 * Returns an iterator which skips the first element
 * @return 
 */
Ref<JSValue> JSArray::tail()
{
    return JSArrayIterator::create(ref(this), 1);
}

/**
 * Writes an array element
 * @param index
 * @param value
 * @return 
 */
Ref<JSValue> JSArray::indexedWrite(Ref<JSValue> index, Ref<JSValue> value)
{
    if (!isUint(index))
        return jsNull();
    else
    {
        size_t  i = toSizeT (index);
        
        if (m_content.size() <= i)
        {
            //Grow the backing vector if necessary
            m_content.resize(i+1, jsNull());
        }

        return m_content[i] = value;
    }
}


/**
 * String representation of the array
 * @return 
 */
std::string JSArray::toString()const
{
    return join(Ref<JSArray>(const_cast<JSArray*>(this)), jsString(","));
}

/**
 * Writes a JSON representation of the array to the output
 * @param output
 */
std::string JSArray::getJSON(int indent)
{
    std::ostringstream output;
    const size_t    n = m_content.size();
    const bool      multiLine = n > 4;

    output << '[';

    for (size_t i = 0; i < n; ++i)
    {
        if (multiLine)
            output << "\n" << indentText(indent + 1);
        
        if (i > 0)
            output << ',';

        const std::string childJSON = this->getAt(i)->getJSON(indent);

        if (childJSON.empty())
            output << "null";
        else
            output << childJSON;
    }
    
    if (multiLine)
            output << "\n" << indentText(indent);
    output << ']';

    return output.str();
}

/**
 * Creates an immutable copy of the array
 * @return 
 */
Ref<JSValue> JSArray::freeze()
{
    if (m_mutability != MT_MUTABLE)
        return ref(this);
    else
    {
        auto newArray = JSArray::create();
        
        newArray->m_content = m_content;
        newArray->m_mutability = MT_FROZEN;
        return newArray;
    }
}

/**
 * Creates an immutable copy of the array which contains no references to
 * any mutable object
 * @return 
 */
Ref<JSValue> JSArray::deepFreeze(JSValuesMap& transformed)
{
    auto me = ref(this);
    
    if (m_mutability == MT_DEEPFROZEN)
        return me;

    auto it = transformed.find(me);
    if (it != transformed.end())
        return it->second;

    //Clone array
    auto newArray = JSArray::create();
    transformed[me] = newArray;
    
    for (size_t i = 0; i < m_content.size(); ++i )
    {
        auto value = m_content[i]->deepFreeze(transformed);
        newArray->m_content.push_back(value);
    }

    newArray->m_mutability = MT_DEEPFROZEN;

    return newArray;
}        

/**
 * Creates a mutable copy of the array
 * @param forceClone
 * @return 
 */
Ref<JSValue> JSArray::unFreeze(bool forceClone)
{
    if (m_mutability == MT_MUTABLE && !forceClone)
        return ref(this);
    else
    {
        auto newArray = JSArray::create();
        
        newArray->m_content = m_content;
        newArray->m_mutability = MT_FROZEN;
        return newArray;
    }
}

/**
 * Modifies array length
 * @param value
 */
void JSArray::setLength(Ref<JSValue> value)
{
    if (!isUint(value))
        error ("Invalid array index: %s", value->toString().c_str());
    
    const size_t length = toSizeT(value);
    
    m_content.resize(length, jsNull());
}

/**
 * Creates an array iterator. If the iterator is beyond the end, returns 'null'
 * @param arr
 * @param index
 * @return 
 */
Ref<JSValue> JSArrayIterator::create(Ref<JSArray> arr, size_t index)
{
    if (index < arr->length())
        return refFromNew(new JSArrayIterator(arr, index));
    else
        return jsNull();
}

Ref<JSValue> JSArrayIterator::head()const
{
    return m_array->getAt(m_index);
}

Ref<JSValue> JSArrayIterator::tail()const
{
    return create (m_array, m_index+1);
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


Ref<JSValue> scArrayPush(FunctionScope* pScope)
{
    auto    arr =  pScope->getThis().staticCast<JSArray>();
    auto    val =  pScope->getParam("x");
    
    arr->push(val);
    
    return arr;
}

Ref<JSValue> scArrayIndexOf(FunctionScope* pScope)
{
    auto    arrVal =  pScope->getThis();
    auto    arr =  arrVal.staticCast<JSArray>();
    auto    searchElement =  pScope->getParam("searchElement");
    auto    fromIndex =  pScope->getParam("fromIndex");

    if (arrVal->isNull()) 
        return jsInt(-1);

    const size_t len = arr->length();
    
    if (len <= 0)
        return jsInt(-1);

    size_t n = 0;
    
    if (!fromIndex->isNull())
        n = (size_t)floor(fromIndex->toDouble());

    if (n >= len)
        return jsInt(-1);

    for (; n < len; n++) {
        auto item = arr->getAt (n);
        
        if (mvmAreTypeEqual (item, searchElement))
            return jsInt(n);
    }
    return jsInt(-1);
}

std::string JSArray::join(Ref<JSArray> arr, Ref<JSValue> sep)
{
    string          sepStr = ",";

    if (!sep->isNull())
        sepStr = sep->toString();

    ostringstream output;
    const size_t n = arr->length();
    for (size_t i = 0; i < n; i++)
    {
        if (i > 0) 
            output << sepStr;
        output << arr->getAt(i)->toString();
    }

    return output.str();
}

Ref<JSValue>scArrayJoin(FunctionScope* pScope)
{
    auto    arr = pScope->getThis().staticCast<JSArray>();
    auto    sep = pScope->getParam("separator");

    return jsString( JSArray::join(arr, sep) );
}

/**
 * Creates a 'slice' of the array. A contiguous subset of array elements
 * defined by a initial index (included) and a final index (not included)
 * @param pScope
 * @return 
 */
Ref<JSValue>scArraySlice(FunctionScope* pScope)
{
    Ref<JSArray>    arr = pScope->getThis().staticCast<JSArray>();
    auto            begin = pScope->getParam("begin");
    auto            end = pScope->getParam("end");
    const size_t    iBegin = toSizeT( begin );
    size_t          iEnd = arr->length();
    
    if (isUint(end))
        iEnd = toSizeT( end );
    
    iEnd = max (iEnd, iBegin);
    
    auto result = JSArray::create();
    
    for (size_t i = iBegin; i < iEnd; ++i)
        result->push(arr->getAt(i));

    return result;
}


Ref<JSValue> scObjectConstructor(FunctionScope* pScope)
{
    return JSObject::create();
}

Ref<JSValue> scArrayConstructor(FunctionScope* pScope)
{
    return JSArray::create();
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


Ref<JSClass> createArrayClass()
{
    VarMap  members;

    addNative("function slice(begin, end)", scArraySlice, members);
    addNative("function join(separator)", scArrayJoin, members);
    addNative("function push(x)", scArrayPush, members);
    addNative("function indexOf(searchElement, fromIndex)", scArrayIndexOf, members);

    return JSClass::createNative("Array", 
                                 JSObject::DefaultClass, 
                                 members, 
                                 StringVector(),
                                 scArrayConstructor);
}
