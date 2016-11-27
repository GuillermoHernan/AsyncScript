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

Ref<JSValue> scMathRand(FunctionScope* pScope)
{
    return jsDouble(double(rand()) / RAND_MAX);
}

Ref<JSValue> scMathRandInt(FunctionScope* pScope)
{
    const int min = pScope->getParam("min")->toInt32();
    const int max = pScope->getParam("max")->toInt32();
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
    int lo = pScope->getParam("lo")->toInt32();
    int hi = pScope->getParam("hi")->toInt32();

    int l = hi - lo;
    if (l > 0 && lo >= 0 && lo + l <= (int) str.length())
        return jsString(str.substr(lo, l));
    else
        return jsString("");
}

Ref<JSValue> scStringCharAt(FunctionScope* pScope)
{
    string str = pScope->getThis()->toString();
    int p = pScope->getParam("pos")->toInt32();
    if (p >= 0 && p < (int) str.length())
        return jsString(str.substr(p, 1));
    else
        return jsString("");
}

Ref<JSValue> scStringCharCodeAt(FunctionScope* pScope)
{
    string str = pScope->getThis()->toString();
    int p = pScope->getParam("pos")->toInt32();
    if (p >= 0 && p < (int) str.length())
        return jsInt(str.at(p));
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
    str[0] = pScope->getParam("char")->toInt32();
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

Ref<JSValue> scExec(FunctionScope* pScope)
{
    //TODO: Is it meant to share globals?
    CTinyJS tinyJS;
    std::string str = pScope->getParam("jsCode")->toString();
    tinyJS.execute(str);

    return undefined();
}

Ref<JSValue> scEval(FunctionScope* pScope)
{
    //TODO: Is it meant to share globals?
    CTinyJS tinyJS;
    std::string str = pScope->getParam("jsCode")->toString();

    return tinyJS.evaluateComplex(str);
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

//TODO: Reactivate, but replacing them by standard javascript functions

/*Ref<JSValue> scArrayContains(FunctionScope* pScope) {
  Ref<JSValue>  obj = pScope->getParam("obj");
  Ref<JSValue>  arr = pScope->getThis();
  
  if (!arr->isArray())
      return jsFalse();

  Ref<JSArray>  v = arr.staticCast();

  while (v) {
      if (v->var->equals(obj)) {
        contains = true;
        break;
      }
      v = v->nextSibling;
  }

  return jsFalse();
}

Ref<JSValue> scArrayRemove(FunctionScope* pScope) {
  CScriptVar *obj = pScope->getParam("obj");
  vector<int> removedIndices;
  CScriptVarLink *v;
  // remove
  v = pScope->getThis()->firstChild;
  while (v) {
      if (v->var->equals(obj)) {
        removedIndices.push_back(v->getIntName());
      }
      v = v->nextSibling;
  }
  // renumber
  v = pScope->getThis()->firstChild;
  while (v) {
      int n = v->getIntName();
      int newn = n;
      for (size_t i=0;i<removedIndices.size();i++)
        if (n>=removedIndices[i])
          newn--;
      if (newn!=n)
        v->setIntName(newn);
      v = v->nextSibling;
  }
}

Ref<JSValue>scArrayJoin(FunctionScope* pScope) {
  string sep = pScope->getParam("separator")->toString();
  CScriptVar *arr = pScope->getThis();

  ostringstream sstr;
  int l = arr->getArrayLength();
  for (int i=0;i<l;i++) {
    if (i>0) sstr << sep;
    sstr << arr->getArrayIndex(i)->toString();
  }

  return jsString(sstr.str());
}*/

// ----------------------------------------------- Register Functions

Ref<JSObject> createClass(const char* className,
                          Ref<JSObject> parentPrototype,
                          JSNativeFn constructorFn,
                          CTinyJS *tinyJS)
{
    static const std::string fnHeader = "function ";
    Ref<JSObject>   prototype = JSObject::create(parentPrototype);
    Ref<JSFunction> constructor = tinyJS->addNative(fnHeader + className, constructorFn);
    
    constructor->set("prototype", prototype);
    return prototype;
}

void fixPrototype (const char* objName, Ref<JSObject> prototype, CTinyJS *tinyJS)
{
    Ref<JSValue>    obj = tinyJS->getGlobal(objName);
    
    castTo<JSObject>(obj)->setPrototype(prototype);
}


void createDefaultPrototypes (CTinyJS *tinyJS)
{
    Ref<JSObject>   objProto = createClass("Object(obj)", Ref<JSObject>(), objectConstructor, tinyJS);
    
    JSObject::DefaultPrototype = objProto;
    JSFunction::DefaultPrototype = createClass("Function()", objProto, functionConstructor, tinyJS);
    
    fixPrototype ("Object", JSFunction::DefaultPrototype, tinyJS);
    fixPrototype ("Function", JSFunction::DefaultPrototype, tinyJS);
    
    JSArray::DefaultPrototype = createClass("Array()", objProto, arrayConstructor, tinyJS);
    JSString::DefaultPrototype = createClass("String(obj)", objProto, stringConstructor, tinyJS);
    
    
    
}

void registerFunctions(CTinyJS *tinyJS)
{
    tinyJS->addNative("function exec(jsCode)", scExec); // execute the given code
    tinyJS->addNative("function eval(jsCode)", scEval); // execute the given string (an expression) and return the result
    //    tinyJS->addNative("function trace()", scTrace);
    //    tinyJS->addNative("function Object.dump()", scObjectDump);
    //    tinyJS->addNative("function Object.clone()", scObjectClone);
    tinyJS->addNative("function Math.rand()", scMathRand);
    tinyJS->addNative("function Math.randInt(min, max)", scMathRandInt);
    tinyJS->addNative("function charToInt(ch)", scCharToInt); //  convert a character to an int - get its value

    tinyJS->addNative("function String.prototype.indexOf(search)", scStringIndexOf); // find the position of a string in a string, -1 if not
    tinyJS->addNative("function String.prototype.substring(lo,hi)", scStringSubstring);
    tinyJS->addNative("function String.prototype.charAt(pos)", scStringCharAt);
    tinyJS->addNative("function String.prototype.charCodeAt(pos)", scStringCharCodeAt);
    tinyJS->addNative("function String.prototype.split(separator)", scStringSplit);
    tinyJS->addNative("function String.fromCharCode(char)", scStringFromCharCode);

    tinyJS->addNative("function parseInt(str)", scIntegerParseInt); // string to int
    tinyJS->addNative("function Integer.valueOf(str)", scIntegerValueOf); // value of a single character
    tinyJS->addNative("function JSON.stringify(obj, replacer)", scJSONStringify); // convert to JSON. replacer is ignored at the moment
    // JSON.parse is left out as you can (unsafely!) use eval instead
    //    tinyJS->addNative("function Array.contains(obj)", scArrayContains);
    //    tinyJS->addNative("function Array.remove(obj)", scArrayRemove);
    //    tinyJS->addNative("function Array.join(separator)", scArrayJoin);
}

