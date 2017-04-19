/* 
 * File:   asString.cpp
 * Author: ghernan
 * 
 * AsyncScript string class implementation
 * 
 * Created on January 3, 2017, 2:19 PM
 */

#include "ascript_pch.hpp"
#include "asString.h"
#include "jsArray.h"
#include "scriptMain.h"
#include <string>

using namespace std;

Ref<JSClass> createStringClass();

Ref<JSClass> JSString::StringClass = createStringClass();

/**
 * Construction function
 * @param value
 * @return 
 */
Ref<JSString> JSString::create(const std::string & value)
{
    return refFromNew(new JSString(value));
}

/**
 * Strings are never mutable. Therefore 'unFreeze' operation returns a reference
 * to the same object.
 * @param forceClone
 * @return 
 */
Ref<JSValue> JSString::unFreeze(bool forceClone)
{
    return Ref<JSValue>(this);
}

/**
 * Tries to transform a string into a double value
 * @return 
 */
double JSString::toDouble()const
{
    const double result = strtod(m_text.c_str(), NULL);
    
    if (result == 0 && !isNumber(m_text))
        return getNaN();
    else
        return result;
}

/**
 * Gets the JSON representation of a string
 * @param indent
 * @return 
 */
std::string JSString::getJSON(int indent)
{
    return escapeString(m_text, true);
}


/**
 * Member access function overridden to have access to 'length' property, and to
 * have access to individual characters.
 * @param key
 * @return 
 */
Ref<JSValue> JSString::readField(const string& key)const
{
    if (key == "length")
        return jsDouble (m_text.size());
    else
        return JSObject::readField(key);
}

/**
 * It overrides 'getAt' to have access to individual characters.
 * @param index
 * @return 
 */
Ref<JSValue> JSString::getAt(Ref<JSValue> index)
{
    if (isUint(index))
    {
        const size_t    uIndex = toSizeT(index);
        
        if (uIndex >= m_text.length())
            return jsNull();
        else
            return jsString(m_text.substr(uIndex, 1));
    }
    else
        return jsNull();
}


Ref<JSValue> scStringIndexOf(ExecutionContext* ec)
{
    string str = ec->getLastParam()->toString();
    string search = ec->getParam(0)->toString();
    size_t p = str.find(search);
    int val = (p == string::npos) ? -1 : p;
    return jsInt(val);
}

Ref<JSValue> scStringSubstring(ExecutionContext* ec)
{
    string str = ec->getLastParam()->toString();
    const size_t lo = toSizeT(ec->getParam(0));
    const size_t hi = toSizeT(ec->getParam(1));

    size_t l = hi - lo;
    if (l > 0 && lo >= 0 && lo + l <= str.length())
        return jsString(str.substr(lo, l));
    else
        return jsString("");
}

Ref<JSValue> scStringCharAt(ExecutionContext* ec)
{
    Ref<JSString> str = ec->getLastParam().staticCast<JSString>();
    
    return str->getAt( ec->getParam(0) );
}

Ref<JSValue> scStringCharCodeAt(ExecutionContext* ec)
{
    string str = scStringCharAt(ec)->toString();
    if (!str.empty())
        return jsInt(str[0]);
    else
        return jsInt(0);
}

Ref<JSValue> scStringSplit(ExecutionContext* ec)
{
    string str = ec->getLastParam()->toString();
    string sep = ec->getParam(0)->toString();
    Ref<JSArray> result = JSArray::create();

    size_t pos = str.find(sep);
    while (pos != string::npos)
    {
        result->push(jsString(str.substr(0, pos)));
        str = str.substr(pos + 1);
        pos = str.find(sep);
    }

    if (str.size() > 0)
        result->push(jsString(str));

    return result;
}

Ref<JSValue> scStringFromCharCode(ExecutionContext* ec)
{
    char str[2];
    str[0] = (char)toInt32( ec->getParam(0));
    str[1] = 0;
    return jsString(str);
}

Ref<JSValue> scStringConstructor(ExecutionContext* ec)
{
    return jsString("");
}

/**
 * Creates the class object for the 'String' class
 * @return 
 */
Ref<JSClass> createStringClass()
{
    VarMap  members;

    addNative("function indexOf(search)", scStringIndexOf, members);
    addNative("function substring(lo,hi)", scStringSubstring, members);
    addNative("function charAt(pos)", scStringCharAt, members);
    addNative("function charCodeAt(pos)", scStringCharCodeAt, members);
    addNative("function split(separator)", scStringSplit, members);
    addNative("function fromCharCode(char)", scStringFromCharCode, members);
    
    return JSClass::createNative("String", 
                                 JSObject::DefaultClass, 
                                 members, 
                                 StringVector(),
                                 scStringConstructor);
}
