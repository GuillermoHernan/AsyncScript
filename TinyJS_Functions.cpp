/*
 * TinyJS
 *
 * A single-file Javascript-alike engine
 *
 * - Useful language functions
 *
 * Authored By Gordon Williams <gw@pur3.co.uk>
 *
 * Copyright (C) 2009 Pur3 Ltd
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "TinyJS_Functions.h"
#include "scriptMain.h"
#include "mvmFunctions.h"

#include <math.h>
#include <cstdlib>
#include <sstream>

using namespace std;
// ----------------------------------------------- Actual Functions

//TODO: Reactivate

/*void scTrace(CScriptVar *c, void *userdata) {
    CTinyJS *js = (CTinyJS*)userdata;
    js->root->trace();
}

void scObjectDump(FunctionScope* pScope) {
    pScope->getThis()->trace("> ");
}

void scObjectClone(FunctionScope* pScope) {
    CScriptVar *obj = pScope->getThis();
    c->getReturnVar()->copyValue(obj);
}*/

Ref<JSValue> scObjectFreeze(FunctionScope* pScope)
{
    auto obj = pScope->getThis();
    
    return obj->freeze();
}

Ref<JSValue> scObjectUnfreeze(FunctionScope* pScope)
{
    auto obj = pScope->getThis();
    auto forceClone = pScope->getParam("forceClone");
    
    return obj->unFreeze(forceClone->toBoolean());
}

Ref<JSValue> scMathRand(FunctionScope* pScope)
{
    return jsDouble(double(rand()) / RAND_MAX);
}

Ref<JSValue> scMathRandInt(FunctionScope* pScope)
{
    const int min = toInt32(pScope->getParam("min"));
    const int max = toInt32(pScope->getParam("max"));
    const int val = min + (int) (rand() % (1 + max - min));
    return jsInt(val);
}

Ref<JSValue> scCharToInt(FunctionScope* pScope)
{
    string str = pScope->getParam("ch")->toString();
    int val = 0;
    if (str.length() > 0)
        val = (int) str.c_str()[0];
    return jsInt(val);
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

Ref<JSValue> scIntegerParseInt(FunctionScope* pScope)
{
    //TODO: Make it more standard compliant (octal support, return NaN if fails...)
    //We can reuse the code which parses numeric constants.
    string str = pScope->getParam("str")->toString();
    int val = strtol(str.c_str(), 0, 0);
    return jsInt(val);
}

Ref<JSValue> scIntegerValueOf(FunctionScope* pScope)
{
    string str = pScope->getParam("str")->toString();

    int val = 0;
    if (str.length() == 1)
        val = str[0];
    return jsInt(val);
}

Ref<JSValue> scJSONStringify(FunctionScope* pScope)
{
    std::string result;
    result = pScope->getParam("obj")->getJSON(0);
    return jsString(result);
}

Ref<JSValue> scEval(FunctionScope* pScope)
{
    std::string str = pScope->getParam("jsCode")->toString();

    return evaluate (str.c_str(), createDefaultGlobals());
}

Ref<JSValue> objectConstructor(FunctionScope* pScope)
{
    //TODO: Not implemented
    return undefined();
}

Ref<JSValue> arrayConstructor(FunctionScope* pScope)
{
    //TODO: Not implemented
    return undefined();
}

Ref<JSValue> functionConstructor(FunctionScope* pScope)
{
    //TODO: Not implemented
    return undefined();
}

Ref<JSValue> stringConstructor(FunctionScope* pScope)
{
    Ref<JSValue>    obj =  pScope->getParam("obj");
    
    return jsString (obj->toString());
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

std::string scArrayJoin(Ref<JSArray> arr, Ref<JSValue> sep)
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

    return jsString( scArrayJoin(arr, sep) );
}

/**
 * Creates a 'alice' of the array. A contiguous subset of array elements
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

// ----------------------------------------------- Register Functions

Ref<JSObject> createClass(const char* className,
                          Ref<JSObject> parentPrototype,
                          JSNativeFn constructorFn,
                          Ref<IScope> scope)
{
    static const std::string fnHeader = "function ";
    Ref<JSObject>   prototype = JSObject::create(parentPrototype);
    Ref<JSFunction> constructor = addNative(fnHeader + className, constructorFn, scope);
    
    constructor->writeField(jsString("prototype"), prototype);
    return prototype;
}

void fixPrototype (const char* objName, Ref<JSObject> prototype, Ref<IScope> scope)
{
    Ref<JSValue>    obj = scope->get(objName);
    
    castTo<JSObject>(obj)->setPrototype(prototype);
}


void createDefaultPrototypes (Ref<IScope> scope)
{
    Ref<JSObject>   objProto = createClass("Object(obj)", Ref<JSObject>(), objectConstructor, scope);
    
    JSObject::DefaultPrototype = objProto;
    JSFunction::DefaultPrototype = createClass("Function()", objProto, functionConstructor, scope);
    
    fixPrototype ("Object", JSFunction::DefaultPrototype, scope);
    fixPrototype ("Function", JSFunction::DefaultPrototype, scope);
    
    JSArray::DefaultPrototype = createClass("Array()", objProto, arrayConstructor, scope);
    JSString::DefaultPrototype = createClass("String(obj)", objProto, stringConstructor, scope);
}

void registerFunctions(Ref<IScope> scope)
{
    createDefaultPrototypes (scope);
    
    addNative("function eval(jsCode)", scEval, scope); // execute the given string (an expression) and return the result
    //    addNative("function trace()", scTrace, scope);
    //    addNative("function Object.dump()", scObjectDump, scope);
    //    addNative("function Object.clone()", scObjectClone, scope);
    
    addNative("function Object.prototype.freeze()", scObjectFreeze, scope);
    addNative("function Object.prototype.unfreeze(forceClone)", scObjectUnfreeze, scope);
    
    addNative("function Math.rand()", scMathRand, scope);
    addNative("function Math.randInt(min, max)", scMathRandInt, scope);
    addNative("function charToInt(ch)", scCharToInt, scope); //  convert a character to an int - get its value

    addNative("function String.prototype.indexOf(search)", scStringIndexOf, scope); // find the position of a string in a string, -1 if not
    addNative("function String.prototype.substring(lo,hi)", scStringSubstring, scope);
    addNative("function String.prototype.charAt(pos)", scStringCharAt, scope);
    addNative("function String.prototype.charCodeAt(pos)", scStringCharCodeAt, scope);
    addNative("function String.prototype.split(separator)", scStringSplit, scope);
    addNative("function String.fromCharCode(char)", scStringFromCharCode, scope);

    addNative("function parseInt(str)", scIntegerParseInt, scope); // string to int
    addNative("function Integer.valueOf(str)", scIntegerValueOf, scope); // value of a single character
    addNative("function JSON.stringify(obj, replacer)", scJSONStringify, scope); // convert to JSON. replacer is ignored at the moment
    //TODO: Add JSON.parse()
    addNative("function Array.prototype.slice(begin, end)", scArraySlice, scope);
    addNative("function Array.prototype.join(separator)", scArrayJoin, scope);
    addNative("function Array.prototype.push(x)", scArrayPush, scope);
    addNative("function Array.prototype.indexOf(searchElement, fromIndex)", scArrayIndexOf, scope);
}

