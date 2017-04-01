/*
 * TinyJS
 *
 * A single-file Javascript-alike engine
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

/*
 * This is a simple program showing how to use TinyJS
 */

#include "ascript_pch.hpp"
#include "utils.h"
#include "scriptMain.h"
#include "ScriptException.h"
#include "microVM.h"
#include <assert.h>
#include <stdio.h>


//const char *code = "var a = 5; if (a==5) a=4; else a=3;";
//const char *code = "{ var a = 4; var b = 1; while (a>0) { b = b * 2; a = a - 1; } var c = 5; }";
//const char *code = "{ var b = 1; for (var i=0;i<4;i=i+1) b = b * 2; }";
const char *code = "function myfunc(x, y) { return x + y; } var a = myfunc(1,2); print(a);";

Ref<JSValue> js_print(ExecutionContext* ec)
{
    printf("> %s\n", ec->getParam(0)->toString().c_str());
    return jsNull();
}

Ref<JSValue> js_dump(ExecutionContext* ec)
{
    printf ("Temporarily out of order!\n");
    //TODO: Make it work again.
//    Ref<JSObject> globals = pScope->getGlobals().staticCast<JSObject>();
//
//    printf("> %s\n", globals->getJSON(0).c_str());
    return jsNull();
}

int main(int argc, char **argv)
{
    //Create default global functions
    auto        globals = createDefaultGlobals();

    // Add custom native functions
    addNative("function print(text)", js_print, globals);
    addNative("function dump()", js_dump, globals);
    
    /* Execute out bit of code */
    try
    {
        evaluate("var lets_quit = 0; function quit() { lets_quit = 1; }", globals);
        evaluate("print(\"Interactive mode... Type quit(); to exit, or print(...); to print something, or dump() to dump the symbol table!\");", globals);
    }
    catch (const CScriptException& e)
    {
        printf("ERROR: %s\n", e.what());
    }

    while (evaluate("lets_quit", globals)->toBoolean() == false)
    {
        char buffer[2048];
        fgets(buffer, sizeof (buffer), stdin);
        try
        {
            auto result = evaluate(buffer, globals);
            
            printf ("> %s\n", result->toString().c_str());
        }
        catch (const CScriptException &e)
        {
            printf("ERROR: %s\n", e.what());
        }
    }
#ifdef _WIN32
#ifdef _DEBUG
    _CrtDumpMemoryLeaks();
#endif
#endif
    return 0;
}
