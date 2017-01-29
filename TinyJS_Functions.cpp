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
#include "asObjects.h"
#include "asString.h"

#include <math.h>
#include <cstdlib>
#include <sstream>

using namespace std;
// ----------------------------------------------- Actual Functions

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

void registerDefaultClasses(Ref<IScope> scope)
{
    scope->newVar("Object", JSObject::DefaultClass, true);
    scope->newVar("String", JSString::StringClass, true);
    scope->newVar("Array", JSArray::ArrayClass, true);
}

// ----------------------------------------------- Register Functions
/**
 * Register default functions into the given scope.
 * @param scope
 */
void registerFunctions(Ref<IScope> scope)
{
    registerDefaultClasses(scope);
    
    addNative("function eval(jsCode)", scEval, scope); // execute the given string (an expression) and return the result
    
    addNative("function Math.rand()", scMathRand, scope);
    addNative("function Math.randInt(min, max)", scMathRandInt, scope);
    addNative("function charToInt(ch)", scCharToInt, scope); //  convert a character to an int - get its value

    addNative("function parseInt(str)", scIntegerParseInt, scope); // string to int
    addNative("function Integer.valueOf(str)", scIntegerValueOf, scope); // value of a single character
    addNative("function JSON.stringify(obj, replacer)", scJSONStringify, scope); // convert to JSON. replacer is ignored at the moment
    //TODO: Add JSON.parse()
}

