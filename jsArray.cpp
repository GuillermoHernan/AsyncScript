/* 
 * File:   jsArray.cpp
 * Author: ghernan
 * 
 * Asyncscript arrays runtime support code.
 *
 * Created on January 29, 2017, 2:24 PM
 */

#include "ascript_pch.hpp"
#include "jsArray.h"
#include "utils.h"
#include "scriptMain.h"
#include "mvmFunctions.h"
#include "ScriptException.h"

#include <math.h>

using namespace std;

Ref<JSClass> createArrayClass();
Ref<JSClass> JSArray::ArrayClass = createArrayClass();


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
 * Generates a 'JSArray' from a vector of values.
 * @param values
 * @return 
 */
Ref<JSArray> JSArray::fromVector(const ValueVector& values)
{
    auto    newArr = JSArray::create(values.size());
    size_t  i;
    
    for (i=0; i <values.size(); ++i)
        newArr->push(values[i]);
    
    return newArr;
}



/**
 * Adds a value to the end of the array
 * @param value
 * @return Returns new array size
 */
size_t JSArray::push(ASValue value)
{
    if (getMutability() == MT_MUTABLE)
        m_content.push_back(value);
    
    return m_content.size();
}

/**
 * Gets an element located at an array position
 * @param index
 * @return 
 */
ASValue JSArray::getAt(size_t index)const
{
    if (index >= m_content.size())
        return jsNull();
    else
        return m_content[index];
}

/**
 * Changes an array element at a given position.
 * @param index
 * @param value
 * @return 
 */
ASValue JSArray::setAt(size_t index, ASValue value)
{
    if (index >= m_content.size())
        return jsNull();
    else
    {
        m_content[index] = value;
        return value;
    }
}



/**
 * JSArray 'get' override, to implement 'length' property reading.
 * @param name
 * @return 
 */
ASValue JSArray::readField(const std::string& key)const
{
    if (key == "length")
        return jsSizeT(m_content.size());
    else
        return JSObject::readField(key);
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
ASValue JSArray::writeField(const std::string& key, ASValue value, bool isConst)
{
    if (getMutability() != MT_MUTABLE)
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
ASValue JSArray::getAt(ASValue index, ExecutionContext* ec)
{
    if (!index.isUint())
        return jsNull();
    else
        return getAt( index.toSizeT () );
}


/**
 * Writes an array element
 * @param index
 * @param value
 * @return 
 */
ASValue JSArray::setAt(ASValue index, ASValue value, ExecutionContext* ec)
{
    if (!index.isUint())
        return jsNull();
    else
    {
        size_t  i = index.toSizeT ();
        
        if (m_content.size() <= i)
        {
            //Grow the backing vector if necessary
            m_content.resize(i+1, jsNull());
        }

        return m_content[i] = value;
    }
}

/**
 * Returns an iterator to walk the array
 * @param ec
 * @return 
 */
ASValue JSArray::iterator(ExecutionContext* ec)const
{
    return JSArrayIterator::create(ref(const_cast<JSArray*>(this)), 0);
}

/**
 * Gets the fields of the 'Array' object.
 * @param inherited
 * @return 
 */
StringSet JSArray::getFields(bool inherited)const
{
    StringSet result = JSObject::getFields(inherited);
    
    result.insert("length");
    return result;
}

/**
 * String representation of the array
 * @return 
 */
std::string JSArray::toString(ExecutionContext* ec)const
{
    return join(ref(const_cast<JSArray*>(this)), jsString(","), ec);
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

        const std::string childJSON = this->getAt(i).getJSON(indent);

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
ASValue JSArray::freeze()
{
    if (getMutability() != MT_MUTABLE)
        return value();
    else
    {
        auto newArray = JSArray::create();
        
        newArray->m_content = m_content;
        newArray->m_mutability = MT_FROZEN;
        return newArray->value();
    }
}

/**
 * Creates an immutable copy of the array which contains no references to
 * any mutable object
 * @return 
 */
ASValue JSArray::deepFreeze(ASValue::ValuesMap& transformed)
{
    auto me = value();
    
    if (getMutability() == MT_DEEPFROZEN)
        return me;

    auto it = transformed.find(me);
    if (it != transformed.end())
        return it->second;

    //Clone array
    auto newArray = JSArray::create();
    transformed[me] = newArray->value();
    
    for (size_t i = 0; i < m_content.size(); ++i )
    {
        auto value = m_content[i].deepFreeze(transformed);
        newArray->m_content.push_back(value);
    }

    newArray->m_mutability = MT_DEEPFROZEN;

    return newArray->value();
}        

/**
 * Creates a mutable copy of the array
 * @param forceClone
 * @return 
 */
ASValue JSArray::unFreeze(bool forceClone)
{
    if (m_mutability == MT_MUTABLE && !forceClone)
        return value();
    else
    {
        auto newArray = JSArray::create();
        
        newArray->m_content = m_content;
        newArray->m_mutability = MT_FROZEN;
        return newArray->value();
    }
}

/**
 * Modifies array length
 * @param value
 */
void JSArray::setLength(ASValue value)
{
    if (!value.isUint())
        rtError ("Invalid array index: %s", value.toString().c_str());
    
    m_content.resize(value.toSizeT(), jsNull());
}

// JSArrayIterator
//
////////////////////////////////////

/**
 * Creates an array iterator. If the iterator is beyond the end, returns 'null'
 * @param arr
 * @param index
 * @return 
 */
ASValue JSArrayIterator::create(Ref<JSArray> arr, size_t index)
{
    if (index < arr->length())
        return refFromNew(new JSArrayIterator(arr, index))->value();
    else
        return jsNull();
}

/**
 * Creates the class for the iterator
 * @return 
 */
Ref<JSClass> JSArrayIterator::getClass()
{
    static Ref<JSClass>     cls;
    
    if (cls.isNull())
    {
        VarMap  members;

        addNative("function head()", head, members);
        addNative("function tail()", tail, members);

        cls = JSClass::createNative("ArrayIterator", 
                                     JSObject::DefaultClass, 
                                     members, 
                                     StringVector(),
                                     scConstructor);
    }

    return cls;
}

/**
 * Javascript constructor of the array iterator
 * @param ec
 * @return 
 */
ASValue JSArrayIterator::scConstructor(ExecutionContext* ec)
{
    //TODO: Missing mechanism to ensure that the first parameter is an array
    auto arr = ec->getParam(0);
    auto index = ec->getParam(1);
    
    if (arr.getType() != VT_OBJECT)
        return jsNull();
    
    if (!index.isUint())
        index = jsSizeT(0);
    
    return JSArrayIterator::create(arr.staticCast<JSArray>(), index.toSizeT());
}

ASValue JSArrayIterator::head(ExecutionContext* ec)
{
    ASValue thisPtr = ec->getThis();
    
    if (thisPtr.getType() != VT_OBJECT)
        return jsNull();
    
    auto obj = thisPtr.staticCast<JSArrayIterator>();
    
    return obj->m_array->getAt(obj->m_index);
}

ASValue JSArrayIterator::tail(ExecutionContext* ec)
{
    ASValue thisPtr = ec->getThis();
    
    if (thisPtr.getType() != VT_OBJECT)
        return jsNull();
    
    auto obj = thisPtr.staticCast<JSArrayIterator>();
    
    return create (obj->m_array, obj->m_index+1);
}

ASValue scArrayPush(ExecutionContext* ec)
{
    auto    arr =  ec->getThis().staticCast<JSArray>();
    auto    val =  ec->getParam(0);
    
    arr->push(val);
    
    return arr->value();
}

ASValue scArrayIndexOf(ExecutionContext* ec)
{
    auto    arrVal =  ec->getThis();
    auto    arr =  arrVal.staticCast<JSArray>();
    auto    searchElement =  ec->getParam(0);
    auto    fromIndex =  ec->getParam(1);

    if (arrVal.isNull()) 
        return jsInt(-1);

    const size_t len = arr->length();
    
    if (len <= 0)
        return jsInt(-1);

    size_t n = 0;
    
    if (!fromIndex.isNull())
        n = fromIndex.toSizeT();

    if (n >= len)
        return jsInt(-1);

    for (; n < len; n++) {
        auto item = arr->getAt (n);
        
        if (item.compare(searchElement, ec) == 0)
            return jsInt(n);
    }
    return jsInt(-1);
}

std::string JSArray::join(Ref<JSArray> arr, ASValue sep, ExecutionContext* ec)
{
    string          sepStr = ",";

    if (!sep.isNull())
        sepStr = sep.toString(ec);

    ostringstream output;
    const size_t n = arr->length();
    for (size_t i = 0; i < n; i++)
    {
        if (i > 0) 
            output << sepStr;
        output << arr->getAt(i).toString(ec);
    }

    return output.str();
}

ASValue scArrayJoin(ExecutionContext* ec)
{
    auto    arr = ec->getThis().staticCast<JSArray>();
    auto    sep = ec->getParam(0);

    return jsString( JSArray::join(arr, sep, ec) );
}

/**
 * Creates a 'slice' of the array. A contiguous subset of array elements
 * defined by a initial index (included) and a final index (not included)
 * @param pScope
 * @return 
 */
ASValue scArraySlice(ExecutionContext* ec)
{
    Ref<JSArray>    arr = ec->getThis().staticCast<JSArray>();
    auto            begin = ec->getParam(0);
    auto            end = ec->getParam(1);
    const size_t    iBegin = begin.toSizeT();
    size_t          iEnd = arr->length();
    
    if (end.isUint())
        iEnd = end.toSizeT();
    
    iEnd = max (iEnd, iBegin);
    
    auto result = JSArray::create();
    
    for (size_t i = iBegin; i < iEnd; ++i)
        result->push(arr->getAt(i));

    return result->value();
}

ASValue scArrayConstructor(ExecutionContext* ec)
{
    return JSArray::create()->value();
}

Ref<JSClass> createArrayClass()
{
    VarMap  members;
    
//    addNative("getAt(index)", scArrayGetAt, members);
//    addNative("setAt(index, value)", scArraySetAt, members);
//    addNative("iterator()", scArrayIterator, members);

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
