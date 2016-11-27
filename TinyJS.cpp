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

#include "TinyJS.h"
#include "TinyJS_Lexer.h"
#include "jsOperators.h"
#include "utils.h"
#include <assert.h>

#include "OS_support.h"

#include <string>
#include <string.h>
#include <sstream>
#include <cstdlib>
#include <stdio.h>

using namespace std;

static const char *TINYJS_TEMP_NAME = "";


// ----------------------------------------------------------------------------------- Memory Debug

#define DEBUG_MEMORY 0

#if DEBUG_MEMORY

vector<CScriptVar*> allocatedVars;
vector<CScriptVarLink*> allocatedLinks;

void mark_allocated(CScriptVar *v)
{
    allocatedVars.push_back(v);
}

void mark_deallocated(CScriptVar *v)
{
    for (size_t i = 0; i < allocatedVars.size(); i++)
    {
        if (allocatedVars[i] == v)
        {
            allocatedVars.erase(allocatedVars.begin() + i);
            break;
        }
    }
}

void mark_allocated(CScriptVarLink *v)
{
    allocatedLinks.push_back(v);
}

void mark_deallocated(CScriptVarLink *v)
{
    for (size_t i = 0; i < allocatedLinks.size(); i++)
    {
        if (allocatedLinks[i] == v)
        {
            allocatedLinks.erase(allocatedLinks.begin() + i);
            break;
        }
    }
}

void show_allocated()
{
    for (size_t i = 0; i < allocatedVars.size(); i++)
    {
        printf("ALLOCATED, %d refs\n", allocatedVars[i]->getRefs());
        allocatedVars[i]->trace("  ");
    }
    for (size_t i = 0; i < allocatedLinks.size(); i++)
    {
        printf("ALLOCATED LINK %s, allocated[%d] to \n", allocatedLinks[i]->name.c_str(), allocatedLinks[i]->var->getRefs());
        allocatedLinks[i]->var->trace("  ");
    }
    allocatedVars.clear();
    allocatedLinks.clear();
}
#endif

/**
 * To return the result of a parse / execute operation
 */
struct SResult
{
    CScriptToken token;
    Ref<JSValue> value;

    SResult(CScriptToken _token, Ref<JSValue> _value)
    : token(_token)
    , value(_value)
    {
    }
};

CTinyJS::CTinyJS() : m_globals(JSObject::create(Ref<JSObject>()))
{
}

CTinyJS::~CTinyJS()
{
#if DEBUG_MEMORY
    show_allocated();
#endif
}

void CTinyJS::execute(const string &code)
{
#ifdef TINYJS_CALL_STACK
    call_stack.clear();
#endif

    try
    {
        bool execute = true;
        CScriptToken token(code.c_str());

        token = token.next();
        while (!token.eof())
            token = statement(execute, token, m_globals.getPointer());
    }
    catch (const CScriptException &e)
    {
        ostringstream msg;
        msg << "Error " << e.what();
#ifdef TINYJS_CALL_STACK
        for (int i = (int) call_stack.size() - 1; i >= 0; i--)
            msg << "\n" << i << ": " << call_stack.at(i);
#endif
        throw CScriptException(msg.str());
    }
}

Ref<JSValue> CTinyJS::evaluateComplex(const string &code)
{
#ifdef TINYJS_CALL_STACK
    call_stack.clear();
#endif
    Ref<JSValue> v;

    try
    {
        CScriptToken token(code.c_str());
        bool execute = true;

        token = token.next();
        do
        {
            SResult r = base(execute, token, m_globals.getPointer());
            token = r.token;
            v = r.value;
            if (!token.eof())
                token = token.match(';');
        }
        while (!token.eof());
    }
    catch (const CScriptException &e)
    {
        ostringstream msg;
        msg << "Error " << e.what();
#ifdef TINYJS_CALL_STACK
        for (int i = (int) call_stack.size() - 1; i >= 0; i--)
            msg << "\n" << i << ": " << call_stack.at(i);
#endif

        throw CScriptException(msg.str());
    }

    if (!v.isNull())
        return v;
    else
        return undefined();
}

string CTinyJS::evaluate(const string &code)
{
    return evaluateComplex(code)->toString();
}

CScriptToken CTinyJS::parseFunctionArguments(JSFunction *function, CScriptToken token)
{
    token = token.match('(');

    while (token.type() != ')')
    {
        function->addParameter(token.text());
        //funcVar->addChildNoDup(token.text());
        token = token.match(LEX_ID);
        if (token.type() != ')')
            token = token.match(',');
    }
    return token.match(')');
}

Ref<JSFunction> CTinyJS::addNative(const string &funcDesc, JSNativeFn ptr)
{
    CScriptToken token(funcDesc.c_str());
    token = token.next();

    IScope *scope = m_globals.getPointer();

    token = token.match(LEX_R_FUNCTION);
    string funcName = token.text();
    token = token.match(LEX_ID);

    /* Check for dots, we might want to do something like function String.substring ... */
    while (token.type() == '.')
    {
        token = token.match('.');
        Ref<JSObject> child = getObject(scope, funcName);

        // if it doesn't exist, make an object class
        if (child.isNull())
        {
            child = JSObject::create( JSObject::DefaultPrototype );
            scope->set(funcName, child);
        }

        scope = child.getPointer();
        funcName = token.text();
        token = token.match(LEX_ID);
    }

    Ref<JSFunction> function = JSFunction::createNative(funcName, ptr);
    parseFunctionArguments(function.getPointer(), token);

    scope->set(funcName, function);
    
    return function;
}

/**
 * Gets the value of a global symbol
 * @param name
 * @return The symbol value or a NULL pointer is not found
 */
Ref<JSValue> CTinyJS::getGlobal(const std::string& name)const
{
    return m_globals->get(name);
}

/**
 * Sets the value of a global symbol, or creates it if not present.
 * @param name
 * @param value
 * @return 
 */
Ref<JSValue> CTinyJS::setGlobal(const std::string& name, Ref<JSValue> value)
{
    return m_globals->set(name, value);
}

/**
 * Dumps JSON symbols to a stream
 * @param output
 */
string CTinyJS::dumpJSONSymbols()
{
    return m_globals->getJSON(0);
}

SResult CTinyJS::parseFunctionDefinition(CScriptToken token, IScope* pScope)
{
    // actually parse a function...
    token = token.match(LEX_R_FUNCTION);
    string funcName = TINYJS_TEMP_NAME;

    /* we can have functions without names */
    if (token.type() == LEX_ID)
    {
        funcName = token.text();
        token = token.match(LEX_ID);
    }
    Ref<JSFunction> funcVar = JSFunction::createJS(funcName);
    token = parseFunctionArguments(funcVar.getPointer(), token);

    funcVar->setCode(token);

    bool noexecute = false;
    token = block(noexecute, token, pScope);

    return SResult(token, funcVar);
}

/** Handle a function call (assumes we've parsed the function name and we're
 * on the start bracket). 'parent' is the object that contains this method,
 * if there was one (otherwise it's just a normal function).
 */
SResult CTinyJS::functionCall(bool &execute, Ref<JSValue> fnValue, Ref<JSValue> parent, CScriptToken token, IScope* pScope)
{
    if (execute)
    {
        fnValue = dereference (fnValue);
        
        if (!fnValue->isFunction())
        {
            errorAt(token.getPosition(), "Function expected, found '%s'", fnValue->getTypeName().c_str());
        }

        Ref<JSFunction> function = fnValue.staticCast<JSFunction>();

        //Create function scope
        FunctionScope fnScope(m_globals.getPointer(), function);

        fnScope.setThis(parent);

        token = token.match('(');

        while (token.type() != ')')
        {
            SResult r = base(execute, token, pScope);
            token = r.token;

            if (execute)
                fnScope.addParam(dereference(r.value));

            if (token.type() != ')')
                token = token.match(',');
        }//while

        token = token.match(')');

#ifdef TINYJS_CALL_STACK
        call_stack.push_back(function->getName() + " from " + token.getPosition().toString());
#endif
        SResult r(token, Ref<JSValue>());

        if (function->isNative())
            r.value = function->nativePtr()(&fnScope);
        else
        {
            block(execute, function->codeBlock(), &fnScope);
            execute = true;
            r.value = fnScope.getResult();
        }

#ifdef TINYJS_CALL_STACK
        if (!call_stack.empty()) call_stack.pop_back();
#endif
        return r;
    }
    else
    {
        // function, but not executing - just parse args and be done
        token = token.match('(');
        while (token.type() != ')')
        {
            token = base(execute, token, pScope).token;

            if (token.type() != ')')
                token = token.match(',');
        }
        token = token.match(')');
        if (token.type() == '{')
        { // TODO: why is this here? --> Si no lo sabes tú... Aunque tampoco me extraña mucho.
            token = block(execute, token, pScope);
        }
        /* function will be a blank scriptvarlink if we're not executing,
         * so just return it rather than an alloc/free */
        return SResult(token, fnValue);
    }
}

SResult CTinyJS::factor(bool &execute, CScriptToken token, IScope* pScope)
{
    if (token.type() == '(')
    {
        token = token.match('(');
        SResult a = base(execute, token, pScope);
        a.token = a.token.match(')');
        return a;
    }
    if (token.type() == LEX_R_TRUE)
    {
        token = token.match(LEX_R_TRUE);
        return SResult(token, jsTrue());
    }
    if (token.type() == LEX_R_FALSE)
    {
        token = token.match(LEX_R_FALSE);
        return SResult(token, jsFalse());
    }
    if (token.type() == LEX_R_NULL)
    {
        token = token.match(LEX_R_NULL);
        return SResult(token, jsNull());
    }
    if (token.type() == LEX_R_UNDEFINED)
    {
        token = token.match(LEX_R_UNDEFINED);
        return SResult(token, undefined());
    }
    if (token.type() == LEX_ID)
    {
        const string    name = token.text();
        Ref<JSValue>    a = undefined();

        if (execute && !pScope->get(name).isNull())
            a = JSReference::create(pScope, name);

        /* The parent if we're executing a method call */
        Ref<JSValue> parent;

        if (execute && a->isUndefined())
        {
            /* Variable doesn't exist! JavaScript says we should create it
             * (we won't add it here. This is done in the assignment operator)*/
            //TODO: I think it is not necessary to add it to the scope at this point.
            //a = pScope->set(token.text(), jsNull()); //TODO: null or undefined? Is this necessary
        }
        token = token.match(LEX_ID);
        while (token.type() == '(' || token.type() == '.' || token.type() == '[')
        {
            if (token.type() == '(')
            { // ------------------------------------- Function Call
                SResult r = functionCall(execute, a, parent, token, pScope);
                a = r.value;
                token = r.token;
            }
            else if (token.type() == '.')
            { // ------------------------------------- Record Access
                token = token.match('.');
                if (execute)
                {
                    string name = token.text();
                    
                    if (!a->isObject())
                        errorAt (token.getPosition(), "Illegal left hand side of member access operator");
                    
                    parent = a;
                    a = JSReference::create(castTo<JSObject>(a).getPointer(), name);
                }
                token = token.match(LEX_ID);
            }
            else if (token.type() == '[')
            { // ------------------------------------- Array Access
                token = token.match('[');
                SResult indexRes = base(execute, token, pScope);
                token = indexRes.token.match(']');

                //TODO: resolve better left references for array, because it can be a read or write, which
                //must be handled differently
                if (execute)
                {
                    Ref<JSValue> child = a->arrayAccess(indexRes.value);
                    parent = a;
                    a = child;
                }
            }
            else
                ASSERT(0);
        }
        return SResult(token, a);
    }
    if (token.type() == LEX_INT || token.type() == LEX_FLOAT || token.type() == LEX_STR)
    {
        return SResult(token.next(), createConstant(token));
    }
    if (token.type() == '{')
    {
        Ref<JSObject> contents = JSObject::create( JSObject::DefaultPrototype );

        /* JSON-style object definition */
        token = token.match('{');
        while (token.type() != '}')
        {
            string id;
            // we only allow strings or IDs on the left hand side of an initialisation
            if (token.type() == LEX_STR)
            {
                id = token.strValue();
                token = token.match(LEX_STR);
            }
            else
            {
                id = token.text();
                token = token.match(LEX_ID);
            }
            token = token.match(':');
            if (execute)
            {
                SResult r = base(execute, token, pScope);
                //contents->addChild(id, r.varLink->var);
                contents->set(id, r.value);
                token = r.token;
                //CLEAN(r.varLink);
            }
            // no need to clean here, as it will definitely be used
            if (token.type() != '}')
                token = token.match(',');
        }

        token = token.match('}');
        return SResult(token, contents);
    }
    if (token.type() == '[')
    {
        Ref<JSArray> contents = JSArray::create();

        /* JSON-style array */
        token = token.match('[');
        //int idx = 0;
        while (token.type() != ']')
        {
            if (execute)
            {
                SResult r = base(execute, token, pScope);

                contents->push(r.value);
                token = r.token;
            }

            if (token.type() != ']')
                token = token.match(',');
        }
        token = token.match(']');
        return SResult(token, contents);
    }
    if (token.type() == LEX_R_FUNCTION)
    {
        return parseFunctionDefinition(token, pScope);
    }
    if (token.type() == LEX_R_NEW)
    {
        // new -> create a new object
        token = token.match(LEX_R_NEW);
        string className = token.text();

        Ref<JSValue> constructor = pScope->get(className);
        if (constructor.isNull())
        {
            errorAt(token.getPosition(), "%s is not a valid class name", className.c_str());
            return SResult(token, undefined());
        }
        //TODO: Doesn't support a constructor member of an object. IE: 'a = new Text.Parser'
        token = token.match(LEX_ID);

        if (constructor->isFunction())
        {
            const Ref<JSFunction>   fn = castTo<JSFunction> (constructor);
            const Ref<JSObject>     obj = JSObject::create( castTo<JSObject> (fn->get("prototype"))  );
            
            //TODO: Support 'new' invoke without neither parameters, nor parenthesis.
            SResult r = functionCall(execute, constructor, obj, token, pScope);
            
            return SResult(r.token, obj);
        }
        else
        {
            errorAt(token.getPosition(), "'%s' is not a constructor function", token.text().c_str());
            return SResult(token, undefined());
        }
    }
    // Nothing we can do here... just hope it's the end...
    token = token.match(LEX_EOF);
    return SResult(token, NULL);
}

SResult CTinyJS::unary(bool &execute, CScriptToken token, IScope* pScope)
{
    if (token.type() == '!')
    {
        token = token.match('!'); // binary not
        SResult r = factor(execute, token, pScope);
        if (execute)
        {
            const bool res = !r.value->toBoolean();
            r.value = jsBool(res);
        }

        return r;
    }
    else
        return factor(execute, token, pScope);
}

SResult CTinyJS::term(bool &execute, CScriptToken token, IScope* pScope)
{
    SResult ra = unary(execute, token, pScope);
    Ref<JSValue> result = ra.value;

    token = ra.token;
    while (token.type() == '*' || token.type() == '/' || token.type() == '%')
    {
        const int op = token.type();
        token = token.next();
        SResult rb = unary(execute, token, pScope);
        token = rb.token;
        if (execute)
            result = jsOperator(op, result, rb.value);
    }

    return SResult(token, result);
}

SResult CTinyJS::expression(bool &execute, CScriptToken token, IScope* pScope)
{
    bool negate = false;
    if (token.type() == '-')
    {
        token = token.match('-');
        negate = true;
    }
    const CScriptToken    firstTerm = token;
    SResult r = term(execute, token, pScope);
    token = r.token;
    if (negate)
    {
        r.value = jsOperator('-', jsInt(0), r.value);
    }

    while (token.type() == '+' || token.type() == '-' ||
           token.type() == LEX_PLUSPLUS || token.type() == LEX_MINUSMINUS)
    {
        const int op = token.type();
        token = token.next();
        if (op == LEX_PLUSPLUS || op == LEX_MINUSMINUS)
        {
            //TODO: Missing implementation of prefix operators
            if (execute)
            {
                if (!r.value->isReference())
                    errorAt(firstTerm.getPosition(), "Invalid left-hand side expression in postfix operation");
                
                Ref<JSReference>    ref = r.value.staticCast<JSReference>();
                
                const double prevValue = ref->toDouble();
                
                if (op == LEX_PLUSPLUS)
                    ref->set(jsDouble( prevValue + 1));
                else
                    ref->set(jsDouble( prevValue - 1));
                
                r.value = jsDouble(prevValue);
            }
        }
        else
        {
            SResult rb = term(execute, token, pScope);
            token = rb.token;
            if (execute)
            {
                // not in-place, so just replace
                r.value = jsOperator(op, r.value, rb.value);
            }
        }
    }//while

    r.token = token;
    return r;
}

SResult CTinyJS::shift(bool &execute, CScriptToken token, IScope* pScope)
{
    SResult ra = expression(execute, token, pScope);
    token = ra.token;

    if (token.type() == LEX_LSHIFT || token.type() == LEX_RSHIFT || token.type() == LEX_RSHIFTUNSIGNED)
    {
        int op = token.type();
        token = token.match(op);
        SResult rb = base(execute, token, pScope);
        token = rb.token;
        ra.value = jsOperator(op, ra.value, rb.value);
    }

    ra.token = token;
    return ra;
}

SResult CTinyJS::condition(bool &execute, CScriptToken token, IScope* pScope)
{
    SResult ra = shift(execute, token, pScope);
    token = ra.token;

    while (token.type() == LEX_EQUAL || token.type() == LEX_NEQUAL ||
           token.type() == LEX_TYPEEQUAL || token.type() == LEX_NTYPEEQUAL ||
           token.type() == LEX_LEQUAL || token.type() == LEX_GEQUAL ||
           token.type() == '<' || token.type() == '>')
    {
        int op = token.type();
        token = token.next();

        SResult rb = shift(execute, token, pScope);
        token = rb.token;

        if (execute)
        {
            ra.value = jsOperator(op, ra.value, rb.value);
        }
    }//while

    ra.token = token;
    return ra;
}

SResult CTinyJS::logic(bool &execute, CScriptToken token, IScope* pScope)
{
    SResult ra = condition(execute, token, pScope);
    token = ra.token;

    //TODO: Not sure if this code is correct. Mixes bitwise and logical operations

    while (token.type() == '&' || token.type() == '|' || token.type() == '^' || token.type() == LEX_ANDAND || token.type() == LEX_OROR)
    {
        int op = token.type();
        token = token.match(op);
        bool shortCircuit = false;

        // if we have short-circuit ops, then if we know the outcome
        // we don't bother to execute the other op. Even if not
        // we need to tell mathsOp it's an & or |
        if (op == LEX_ANDAND)
        {
            shortCircuit = !ra.value->toBoolean();
        }
        else if (op == LEX_OROR)
        {
            shortCircuit = ra.value->toBoolean();
        }
        bool    executeB = execute && !shortCircuit;
        SResult rb = condition(executeB, token, pScope);
        token = rb.token;

        if (execute && !shortCircuit)
        {
            ra.value = jsOperator(op, ra.value, rb.value);
        }
    }

    ra.token = token;
    return ra;
}

SResult CTinyJS::ternary(bool &execute, CScriptToken token, IScope* pScope)
{
    SResult lhsRes = logic(execute, token, pScope);
    token = lhsRes.token;

    bool noexec = false;
    if (token.type() == '?')
    {
        token = token.match('?');
        if (!execute)
        {
            SResult rhsRes = base(noexec, token, pScope);
            token = rhsRes.token;
            token = token.match(':');

            rhsRes = base(noexec, token, pScope);
            token = rhsRes.token;
        }
        else
        {
            bool first = lhsRes.value->toBoolean();

            if (first)
            {
                lhsRes = base(execute, token, pScope);
                token = lhsRes.token;
                token = token.match(':');

                token = base(noexec, token, pScope).token;
            }
            else
            {
                token = base(noexec, token, pScope).token;
                token = token.match(':');
                lhsRes = base(execute, token, pScope);
                token = lhsRes.token;
            }
        }
    }

    lhsRes.token = token;
    return lhsRes;
}

SResult CTinyJS::base(bool &execute, CScriptToken token, IScope* pScope)
{
    //TODO: Ternary operator is an invalid left hand side...
    const CScriptToken startToken = token;
    SResult lres = ternary(execute, token, pScope);
    token = lres.token;

    if (token.type() == '=' || token.type() == LEX_PLUSEQUAL || token.type() == LEX_MINUSEQUAL)
    {
        if (!lres.value->isReference() && lres.value->isNull() && execute)
            lres.value = createGlobal (startToken, pScope);
        
        const CScriptToken opToken = token;
        const int op = opToken.type();
        token = token.next();
        SResult rres = base(execute, token, pScope);
        token = rres.token;

        if (execute)
        {
            if (!lres.value->isReference())
                errorAt(opToken.getPosition(), "Invalid left hand side in assignment");

            const Ref<JSReference> lhsRef = lres.value.staticCast<JSReference>();
            Ref<JSValue> r = rres.value;

            if (op == '=')
                r = rres.value;
            else if (op == LEX_PLUSEQUAL)
                r = jsOperator('+', lhsRef, rres.value);
            else if (op == LEX_MINUSEQUAL)
                r = jsOperator('-', lhsRef, rres.value);
            else
                ASSERT(0);

            lhsRef->set(r);
        }
    }

    lres.token = token;
    return lres;
}

CScriptToken CTinyJS::block(bool &execute, CScriptToken token, IScope* pScope)
{
    token = token.match('{');

    if (execute)
    {
        BlockScope blScope(pScope);
        while (token.type() && token.type() != '}')
            token = statement(execute, token, &blScope);
        token = token.match('}');
    }
    else
    {
        // fast skip of blocks
        int brackets = 1;
        while (token.type() && brackets)
        {
            if (token.type() == '{') brackets++;
            if (token.type() == '}') brackets--;
            token = token.next();
        }
    }

    return token;
}

CScriptToken CTinyJS::statement(bool &execute, CScriptToken token, IScope* pScope)
{
    if (token.type() == LEX_ID ||
        token.type() == LEX_INT ||
        token.type() == LEX_FLOAT ||
        token.type() == LEX_STR ||
        token.type() == '-')
    {
        /* Execute a simple statement that only contains basic arithmetic... */
        token = base(execute, token, pScope).token;
    }
    else if (token.type() == '{')
    {
        /* A block of code */
        token = block(execute, token, pScope);
    }
    else if (token.type() == ';')
    {
        /* Empty statement - to allow things like ;;; */
        return token.match(';');
    }
    else if (token.type() == LEX_R_VAR)
    {
        /* variable creation. TODO - we need a better way of parsing the left
         * hand side. Maybe just have a flag called can_create_var that we
         * set and then we parse as if we're doing a normal equals.*/
        token = token.match(LEX_R_VAR);
        while (token.type() != ';')
        {
            const string varName = token.text();
            Ref<JSValue> value = undefined();

            token = token.match(LEX_ID);

            // sort out initialiser
            if (token.type() == '=')
            {
                token = token.match('=');
                SResult r = base(execute, token, pScope);
                token = r.token;
                if (execute)
                    value = r.value;
            }

            pScope->set(varName, value, true);

            if (token.type() != ';')
                token = token.match(',');
        }
    }
    else if (token.type() == LEX_R_IF)
    {
        token = token.match(LEX_R_IF);
        token = token.match('(');
        SResult r = base(execute, token, pScope);
        token = r.token;
        token = token.match(')');
        bool cond = execute && r.value->toBoolean();
        //CLEAN(r.varLink);
        bool noexecute = false; // because we need to be abl;e to write to it
        token = statement(cond ? execute : noexecute, token, pScope);
        if (token.type() == LEX_R_ELSE)
        {
            token = token.match(LEX_R_ELSE);
            token = statement(cond ? noexecute : execute, token, pScope);
        }
    }
    else if (token.type() == LEX_R_WHILE)
    {
        token = whileLoop(execute, token, pScope);
    }
    else if (token.type() == LEX_R_FOR)
    {
        token = forLoop(execute, token, pScope);
    }
    else if (token.type() == LEX_R_RETURN)
    {
        CScriptToken retToken = token;
        token = token.match(LEX_R_RETURN);
        Ref<JSValue> result;
        if (token.type() != ';')
        {
            SResult r = base(execute, token, pScope);
            token = r.token;
            result = dereference(r.value);
        }
        if (execute)
        {
            IScope* fnScope = pScope->getFunctionScope();
            //CScriptVarLink *resultVar = scopes.back()->findChild(TINYJS_RETURN_VAR);
            if (fnScope)
                ((FunctionScope*) fnScope)->setResult(result);
                //resultVar->replaceWith(result);
            else
                errorAt(retToken.getPosition(), "Illegal return statement. Not inside a function");
            //TRACE("RETURN statement, but not in a function.\n");
            execute = false;
        }
        token = token.match(';');
    }
    else if (token.type() == LEX_R_FUNCTION)
    {
        const CScriptToken fnToken = token;
        SResult r = parseFunctionDefinition(token, pScope);
        token = r.token;

        if (execute)
        {
            const string name = r.value.staticCast<JSFunction>()->getName();
            //TODO: Handle error in a different way (having unnamed / named function productions)
            if (name == TINYJS_TEMP_NAME)
                errorAt(fnToken.getPosition(), "Functions defined at statement-level are meant to have a name\n");
            else
                pScope->set(name, r.value);
        }
    }
    else 
        token = token.match(LEX_EOF);

    if (token.type() == ';')
        return statement(execute, token, pScope);
    else
        return token;
}

/**
 * While loop parsing / execution
 * @param execute
 * @param token
 * @return 
 */
CScriptToken CTinyJS::whileLoop(bool &execute, CScriptToken token, IScope* pScope)
{
    token = token.match(LEX_R_WHILE);
    token = token.match('(');

    CScriptToken condition = token;
    bool conditionValue = true;

    while (conditionValue)
    {
        token = condition; //Go back to evaluate condition

        SResult c = base(execute, token, pScope);
        token = c.token;

        conditionValue = execute && c.value->toBoolean();

        token = token.match(')');

        bool bodyExec = conditionValue;
        token = statement(bodyExec, token, pScope);
    }

    return token;
}

/**
 * 'for' loop parsing / execution
 * @param execute
 * @param token
 * @return 
 */
CScriptToken CTinyJS::forLoop(bool &execute, CScriptToken token, IScope* pScope)
{
    bool noExec = false;

    //First part: find for loop parts
    token = token.match(LEX_R_FOR);

    const CScriptToken init = token.match('(');

    const CScriptToken condition = statement(noExec, init, pScope);
    token = base(noExec, condition, pScope).token;

    const CScriptToken increment = token.match(';');
    token = statement(noExec, increment, pScope);

    const CScriptToken body = token.match(')');
    const CScriptToken nextToken = statement(noExec, body, pScope);

    //Second part: execute if requested
    if (execute)
    {
        bool conditionValue = true;

        statement(execute, init, pScope);

        while (conditionValue)
        {
            SResult r = base(execute, condition, pScope);

            conditionValue = r.value->toBoolean();

            if (conditionValue)
            {
                statement(execute, body, pScope);
                statement(execute, increment, pScope);
            }
        }
    }//if (execute)

    return nextToken;
}

/// Look up in any parent classes of the given object

Ref<JSValue> CTinyJS::findInParentClasses(Ref<JSValue> object, const std::string &name)
{
    //TODO: Refactor
    return undefined();

    /*// Look for links to actual parent classes
    CScriptVarLink *parentClass = object->findChild(TINYJS_PROTOTYPE_CLASS);
    while (parentClass) {
      CScriptVarLink *implementation = parentClass->var->findChild(name);
      if (implementation) return implementation;
      parentClass = parentClass->var->findChild(TINYJS_PROTOTYPE_CLASS);
    }
    // else fake it for strings and finally objects
    if (object->isString()) {
      CScriptVarLink *implementation = stringClass->findChild(name);
      if (implementation) return implementation;
    }
    if (object->isArray()) {
      CScriptVarLink *implementation = arrayClass->findChild(name);
      if (implementation) return implementation;
    }
    CScriptVarLink *implementation = objectClass->findChild(name);
    if (implementation) return implementation;

    return 0;*/
}

/**
 * Creates a global variable with the name currently at token position
 * @param token
 * @param pScope
 * @return 
 */
Ref<JSValue> CTinyJS::createGlobal (CScriptToken token, IScope* pScope)
{
    if (token.type() != LEX_ID)
        errorAt(token.getPosition(), "Invalid left hand side in assignment");
    
    const string name = token.text();
    
    m_globals->set(name, jsNull());
    return JSReference::create(m_globals.getPointer(), name);    
}


