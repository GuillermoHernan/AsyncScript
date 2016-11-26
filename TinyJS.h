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

#ifndef TINYJS_H
#define TINYJS_H

// If defined, this keeps a note of all calls and where from in memory. This is slower, but good for debugging
#define TINYJS_CALL_STACK

#ifdef _WIN32
#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif
#endif
#include <string>
#include <vector>

#include "TinyJS_Lexer.h"
#include "JsVars.h"

class CScriptToken;
struct SResult;

class CTinyJS
{
public:
    CTinyJS();
    ~CTinyJS();

    void execute(const std::string &code);

    /** Evaluate the given code and return a link to a javascript object,
     * useful for (dangerous) JSON parsing. If nothing to return, will return
     * 'undefined' variable type. CScriptVarLink is returned as this will
     * automatically unref the result as it goes out of scope. If you want to
     * keep it, you must use ref() and unref() */
    Ref<JSValue> evaluateComplex(const std::string &code);

    /** Evaluate the given code and return a string. If nothing to return, will return
     * 'undefined' */
    std::string evaluate(const std::string &code);

    /// add a native function to be called from TinyJS
    void addNative(const std::string &funcDesc, JSNativeFn ptr, void *userdata);

    Ref<JSValue> getGlobal(const std::string& name)const;
    Ref<JSValue> setGlobal(const std::string& name, Ref<JSValue> value);

    std::string dumpJSONSymbols();

private:
    Ref<JSObject> m_globals;

#ifdef TINYJS_CALL_STACK
    std::vector<std::string> call_stack; /// Names of places called so we can show when erroring
#endif

    // parsing - in order of precedence
    SResult functionCall(bool &execute, Ref<JSValue> function, Ref<JSValue> parent, CScriptToken token, IScope* pScope);
    SResult factor(bool &execute, CScriptToken token, IScope* pScope);
    SResult unary(bool &execute, CScriptToken token, IScope* pScope);
    SResult term(bool &execute, CScriptToken token, IScope* pScope);
    SResult expression(bool &execute, CScriptToken token, IScope* pScope);
    SResult shift(bool &execute, CScriptToken token, IScope* pScope);
    SResult condition(bool &execute, CScriptToken token, IScope* pScope);
    SResult logic(bool &execute, CScriptToken token, IScope* pScope);
    SResult ternary(bool &execute, CScriptToken token, IScope* pScope);
    SResult base(bool &execute, CScriptToken token, IScope* pScope);
    CScriptToken block(bool &execute, CScriptToken token, IScope* pScope);
    CScriptToken statement(bool &execute, CScriptToken token, IScope* pScope);
    CScriptToken whileLoop(bool &execute, CScriptToken token, IScope* pScope);
    CScriptToken forLoop(bool &execute, CScriptToken token, IScope* pScope);
    // parsing utility functions
    SResult parseFunctionDefinition(CScriptToken token, IScope* pScope);
    CScriptToken parseFunctionArguments(JSFunction *function, CScriptToken token);

    Ref<JSValue> findInParentClasses(Ref<JSValue> object, const std::string &name);
};

#endif
