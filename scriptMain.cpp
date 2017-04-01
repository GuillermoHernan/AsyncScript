/* 
 * File:   scriptMain.cpp
 * Author: ghernan
 * 
 * Script engine main file. Contains 'evaluate' function, which runs an script
 * contained in a string.
 * 
 * Created on December 4, 2016, 10:31 PM
 */

#include "ascript_pch.hpp"
#include "scriptMain.h"
#include "jsParser.h"
#include "mvmCodegen.h"
#include "TinyJS_Functions.h"
#include "TinyJS_MathFunctions.h"
#include "mvmFunctions.h"
#include "semanticCheck.h"
#include "asObjects.h"
#include "ScriptException.h"

using namespace std;

// Functions forward declarations.
//////////////////////////////////////////
StringVector parseArgumentList(CScriptToken token);

/**
 * Script evaluation function. Runs a script, and returns its result.
 *
 * @param script    Script code, in a C string.
 * @param globals   Global symbols
 * @return 
 */
Ref<JSValue> evaluate (const char* script, Ref<JSObject> globals)
{
    CScriptToken    token (script);
    

    //Parse
    auto parseResult = parseScript(token.next());
    auto ast = parseResult.ast;
    
    //Semantic check
    semanticCheck(ast);
    
    //Code generation.
    CodeMap                 cMap;
    const Ref<MvmRoutine>   code = scriptCodegen(ast, &cMap);
    
    //Execution
    return evaluate (code, &cMap, globals);
}

/**
 * Evaluates a compiled script.
 * @param code
 * @param codeMap
 * @param globals
 * @return 
 */
Ref<JSValue> evaluate (Ref<MvmRoutine> code, const CodeMap* codeMap, Ref<JSObject> globals)
{
    try
    {
        ExecutionContext    ec;
        
        ec.stack.push_back(globals);
        return mvmExecRoutine(code, &ec, 1);
        //return mvmExecute(code, globals, Ref<IScope>());
    }
    catch (const RuntimeError& e)
    {
        ScriptPosition pos;
        
        if (codeMap != NULL)
            pos = codeMap->get(e.Position);
        errorAt(pos, "%s", e.what());
        return jsNull();        //Not executed
    }
}


/**
 * Creates the default global scope
 * @return 
 */
Ref<JSObject> createDefaultGlobals()
{
    auto    globals = JSObject::create();
    
    registerMvmFunctions(globals);
    registerFunctions(globals);
    registerMathFunctions(globals);
    
    return globals;
}

/**
 * Adds a new native function. This version uses a javascript declaration of
 * the function name to get its name and parameters.
 * @param szFunctionHeader  Javascript function header definition, which includes the
 * function name and parameters.
 * @param pFn               Pointer to native function.
 * @param scope             Scope object to which the function will be added.
 * @param isConst           To specify if the function shall be added as a constant or as
 *                          a variable.
 * @return A new Javascript function object
 */
Ref<JSFunction> addNative (const std::string& szFunctionHeader, 
                           JSNativeFn pFn, 
                           Ref<JSObject> container,
                           bool isConst)
{
    CScriptToken token(szFunctionHeader.c_str());
    token = token.next();

    token = token.match(LEX_R_FUNCTION);
    string funcName = token.text();
    token = token.match(LEX_ID);
    
    //Next dots, read in previous objects
    while (token.type() == '.')
    {
        token = token.match('.');
        Ref<JSValue> child = container->readField(funcName);
        
        // if it doesn't exist or it is not an object, make an object class
        if (!child->isObject())
        {
            child = JSObject::create();
            container->writeField(funcName, child, isConst);
        }
        container = child.staticCast<JSObject>();

        funcName = token.text();
        token = token.match(LEX_ID);
    }

    Ref<JSFunction> function = JSFunction::createNative(funcName,
                                                        parseArgumentList(token),
                                                        pFn);

    container->writeField(funcName, function, isConst);
    
    return function;
}

/**
 * Adds a new native function to a variable map.
 * @param szFunctionHeader
 * @param pFn
 * @param varMap
 * @return 
 */
Ref<JSFunction> addNative (const std::string& szFunctionHeader, 
                           JSNativeFn pFn, 
                           VarMap& varMap)
{
    CScriptToken token(szFunctionHeader.c_str());
    token = token.next();

    token = token.match(LEX_R_FUNCTION);
    string funcName = token.text();
    token = token.match(LEX_ID);
    
    Ref<JSObject>   container;

    Ref<JSFunction> function = JSFunction::createNative(funcName, 
                                                        parseArgumentList(token),
                                                        pFn);

    varMap[funcName] = VarProperties(function, true);
    
    return function;
}


/**
 * Parses a function argument list.
 * @note Used by 'addNative' function
 * @param token 
 * @return A vector with argument names.
 */
StringVector parseArgumentList(CScriptToken token)
{
    //TODO: It is a copy & paste from the function of the same name in 'jsParser.cpp'
    //But the other version uses a 'AstFunction', not a 'StringVector'
    token = token.match('(');
    
    StringVector arguments;

    while (token.type() != ')')
    {
        const string name = token.text();

        token = token.match(LEX_ID);
        arguments.push_back(name);

        if (token.type() != ')')
            token = token.match(',');
    }
    token.match(')');
    
    return arguments;
}


/**
 * Adds a native function with no arguments
 * @param szName    Function name
 * @param pFn       Pointer to native function
 * @param scope     Scope object to which the function will be added.
 * @return A new Javascript function object
 */
Ref<JSFunction> addNative0 (const std::string& szName, 
                           JSNativeFn pFn, 
                           Ref<JSObject> container)
{
    Ref<JSFunction> function = JSFunction::createNative(szName, StringVector(), pFn);

    container->writeField(szName, function, true);
    
    return function;
    
}

/**
 * Adds a native function with one argument
 * @param szName    Function name
 * @param p1        First parameter name
 * @param pFn       Pointer to native function
 * @param scope     Scope object to which the function will be added.
 * @return A new Javascript function object
 */
Ref<JSFunction> addNative1 (const std::string& szName, 
                            const std::string& p1, 
                           JSNativeFn pFn, 
                           Ref<JSObject> container)
{
    StringVector    arguments;
    
    arguments.push_back(p1);
    Ref<JSFunction> function = JSFunction::createNative(szName, arguments, pFn);

    container->writeField(szName, function, true);
    
    return function;
}


/**
 * Adds a native function with two arguments
 * @param szName    Function name
 * @param p1        First parameter name
 * @param p1        Second parameter name
 * @param pFn       Pointer to native function
 * @param scope     Scope object to which the function will be added.
 * @return A new Javascript function object
 */
Ref<JSFunction> addNative2 (const std::string& szName, 
                            const std::string& p1, 
                            const std::string& p2, 
                           JSNativeFn pFn, 
                           Ref<JSObject> container)
{
    StringVector    arguments;
    
    arguments.push_back(p1);
    arguments.push_back(p2);
    Ref<JSFunction> function = JSFunction::createNative(szName, arguments, pFn);

    container->writeField(szName, function, true);
    
    return function;
}

