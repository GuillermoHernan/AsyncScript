/* 
 * File:   asString.cpp
 * Author: ghernan
 * 
 * AsyncScript string class implementation
 * 
 * Created on January 3, 2017, 2:19 PM
 */

#include "OS_support.h"
#include "asString.h"
#include "executionScope.h"
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
 * @param name
 * @return 
 */
Ref<JSValue> JSString::readField(Ref<JSValue> key)const
{
    if (isUint(key))
    {
        const size_t    index = toSizeT(key);
        
        if (index >= m_text.length())
            return undefined();
        else
            return jsString(m_text.substr(index, 1));
    }
    else if (key->toString() == "length")
        return jsDouble (m_text.size());
    else
        return JSObject::readField(key);
}


Ref<JSValue> scStringIndexOf(FunctionScope* pScope)
{
    string str = pScope->getThis()->toString();
    string search = pScope->getParam("search")->toString();
    size_t p = str.find(search);
    int val = (p == string::npos) ? -1 : p;
    return jsInt(val);
}

Ref<JSValue> scStringSubstring(FunctionScope* pScope)
{
    string str = pScope->getThis()->toString();
    const size_t lo = toSizeT(pScope->getParam("lo"));
    const size_t hi = toSizeT(pScope->getParam("hi"));

    size_t l = hi - lo;
    if (l > 0 && lo >= 0 && lo + l <= str.length())
        return jsString(str.substr(lo, l));
    else
        return jsString("");
}

Ref<JSValue> scStringCharAt(FunctionScope* pScope)
{
    Ref<JSString> str = pScope->getThis().staticCast<JSString>();
    
    size_t pos = toSizeT( pScope->getParam("pos") );
    
    return str->readField (jsDouble(pos));
}

Ref<JSValue> scStringCharCodeAt(FunctionScope* pScope)
{
    string str = scStringCharAt(pScope)->toString();
    if (!str.empty())
        return jsInt(str[0]);
    else
        return jsInt(0);
}

Ref<JSValue> scStringSplit(FunctionScope* pScope)
{
    string str = pScope->getThis()->toString();
    string sep = pScope->getParam("separator")->toString();
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

Ref<JSValue> scStringFromCharCode(FunctionScope* pScope)
{
    char str[2];
    str[0] = (char)toInt32( pScope->getParam("char"));
    str[1] = 0;
    return jsString(str);
}

Ref<JSValue> scStringConstructor(FunctionScope* pScope)
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
