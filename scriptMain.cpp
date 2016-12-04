/* 
 * File:   scriptMain.cpp
 * Author: ghernan
 * 
 * Script engine main file. Contains 'evaluate' function, which runs an script
 * contained in a string.
 * 
 * Created on December 4, 2016, 10:31 PM
 */

#include "OS_support.h"
#include "scriptMain.h"
#include "jsParser.h"
#include "mvmCodegen.h"

using namespace std;

/**
 * Script evaluation function. Runs a script, and returns its result.
 *
 * @param script    Script code, in a C string.
 * @param globals   Global symbols
 * @return 
 */
Ref<JSValue> evaluate (const char* script, Ref<IScope> globals)
{
    CScriptToken    token (script);
    StatementList   statements;

    //Parsing loop
    token = token.next();
    while (!token.eof())
    {
        const ParseResult   parseRes = parseStatement (token);
        
        statements.push_back(parseRes.ast);
        token = parseRes.nextToken;
    }
    
    //Code generation.
    const Ref<MvmScript>    code = scriptCodegen(statements);
    
    //Execution
    return mvmExecute(code, globals);
}

