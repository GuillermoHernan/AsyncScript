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
#include "executionScope.h"
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
        newArr->setAt(i, values[i]);
    
    return newArr;
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
 * Changes an array element at a given position.
 * @param index
 * @param value
 * @return 
 */
Ref<JSValue> JSArray::setAt(size_t index, Ref<JSValue> value)
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
 * Returns an iterator which skips the first element
 * @return 
 */
//Ref<JSValue> JSArray::iterator()
//{
//    return JSArrayIterator::create(ref(this), 0);
//}

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
        rtError ("Invalid array index: %s", value->toString().c_str());
    
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
//
//Ref<JSValue> JSArrayIterator::head()
//{
//    return m_array->getAt(m_index);
//}
//
//Ref<JSValue> JSArrayIterator::tail()
//{
//    return create (m_array, m_index+1);
//}

Ref<JSValue> scArrayPush(ExecutionContext* ec)
{
    auto    arr =  ec->getLastParam().staticCast<JSArray>();
    auto    val =  ec->getParam(0);
    
    arr->push(val);
    
    return arr;
}

Ref<JSValue> scArrayIndexOf(ExecutionContext* ec)
{
    auto    arrVal =  ec->getLastParam();
    auto    arr =  arrVal.staticCast<JSArray>();
    auto    searchElement =  ec->getParam(0);
    auto    fromIndex =  ec->getParam(1);

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

Ref<JSValue>scArrayJoin(ExecutionContext* ec)
{
    auto    arr = ec->getLastParam().staticCast<JSArray>();
    auto    sep = ec->getParam(0);

    return jsString( JSArray::join(arr, sep) );
}

/**
 * Creates a 'slice' of the array. A contiguous subset of array elements
 * defined by a initial index (included) and a final index (not included)
 * @param pScope
 * @return 
 */
Ref<JSValue>scArraySlice(ExecutionContext* ec)
{
    Ref<JSArray>    arr = ec->getLastParam().staticCast<JSArray>();
    auto            begin = ec->getParam(0);
    auto            end = ec->getParam(1);
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

Ref<JSValue> scArrayConstructor(ExecutionContext* ec)
{
    return JSArray::create();
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
