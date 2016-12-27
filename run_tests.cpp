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
 * This is a program to run all the tests in the tests folder...
 */

#include "OS_support.h"
#include "utils.h"
#include "scriptMain.h"
#include "mvmCodegen.h"
#include "jsParser.h"
#include "semanticCheck.h"
#include "TinyJS_Functions.h"
#include "actorRuntime.h"

#include <assert.h>
#include <sys/stat.h>
#include <string>
#include <sstream>
#include <stdio.h>

using namespace std;

/**
 * Generic JSON format logger.
 * It is used to generate call log.
 */
class JsonLogger
{
public:
    JsonLogger (const string& filePath) : m_path (filePath)
    {
        FILE*   pf = fopen (m_path.c_str(), "w");
        if (pf)
        {
            fclose(pf);
            log ("[", false);
            m_first = true;
        }
    }
    
    ~JsonLogger()
    {
        log ("]", false);
    }
    
    void log (const string& text, bool comma = true)
    {
        FILE*   pf = fopen (m_path.c_str(), "a+");
        
        if (pf)
        {
            if (comma && !m_first)
                fprintf (pf, ",%s\n", text.c_str());
            else
                fprintf (pf, "%s\n", text.c_str());
            m_first = false;
            fclose(pf);
        }
    }
    
private:
    string  m_path;
    bool    m_first;
};

JsonLogger*  s_curFunctionLogger = NULL;


/**
 * Assertion function exported to tests
 * @param pScope
 * @return 
 */
Ref<JSValue> assertFunction(FunctionScope* pScope)
{
    auto    value =  pScope->getParam("value");
    
    if (!value->toBoolean())
    {
        auto    text =  pScope->getParam("text")->toString();
        
        error("Assertion failed: %s", text.c_str());
    }
    
    return undefined();
}

/**
 * Executes some code using eval, and expects that it throws a 'CScriptException'.
 * It catches the exception, and returns 'true'. If no exception is throw, it 
 * throws an exception to indicate a test failure.
 * @param pScope
 * @return 
 */
Ref<JSValue> expectError(FunctionScope* pScope)
{
    string  code =  pScope->getParam("code")->toString();
    
    try
    {
        evaluate (code.c_str(), createDefaultGlobals());
    }
    catch (CScriptException& error)
    {
        return jsTrue();
    }
    
    error ("No exception thrown: %s", code.c_str());
    
    return jsFalse();
}


/**
 * Function to write on standard output
 * @param pScope
 * @return 
 */
Ref<JSValue> printLn(FunctionScope* pScope)
{
    auto    text =  pScope->getParam("text");
    
    printf ("%s\n", text->toString().c_str());
    
    return undefined();
}

/**
 * Gives access to the parser to the tested code.
 * Useful for tests which target the parser.
 * @param pScope
 * @return 
 */
Ref<JSValue> asParse(FunctionScope* pScope)
{
    string          code =  pScope->getParam("code")->toString();
    CScriptToken    token (code.c_str());
    auto            result = JSArray::create();

    //Parsing loop
    token = token.next();
    while (!token.eof())
    {
        const ParseResult   parseRes = parseStatement (token);

        result->push(parseRes.ast->toJS());
        token = parseRes.nextToken;
    }
    
    return result;
}

/**
 * Funs a test script loaded from a file.
 * @param szFile        Path to the test script.
 * @param testDir       Directory in which the test script is located.
 * @param resultsDir    Directory in which tests results are written
 * @return 
 */
bool run_test(const std::string& szFile, const string &testDir, const string& resultsDir)
{
    printf("TEST %s ", szFile.c_str());
    
    string script = readTextFile(szFile);
    if (script.empty())
    {
        printf("Cannot read file: '%s'\n", szFile.c_str());
        return false;
    }
    
    const string relPath = szFile.substr (testDir.size());
    const string testName = removeExt( fileFromPath(relPath));
    string testResultsDir = resultsDir + removeExt(relPath) + '/';

    auto globals = createDefaultGlobals();
    
    globals->newVar("result", jsInt(0));
    addNative("function assert(value, text)", assertFunction, globals);
    addNative("function printLn(text)", printLn, globals);
    addNative("function expectError(code)", expectError, globals);
    addNative("function asParse(code)", asParse, globals);
    try
    {
        //This code is copied from 'evaluate', to log the intermediate results 
        //generated from each state
        CScriptToken    token (script.c_str());
        AstNodeList   statements;

        //Parsing loop
        token = token.next();
        while (!token.eof())
        {
            const ParseResult   parseRes = parseStatement (token);

            statements.push_back(parseRes.ast);
            token = parseRes.nextToken;
        }

        //Write Abstract Syntax Tree
        const string astJSON = toJSON (statements);
        writeTextFile(testResultsDir + testName + ".ast.json", astJSON);
        
        //Semantic analysis
        semanticCheck(statements);

        //Code generation.
        const Ref<MvmRoutine>    code = scriptCodegen(statements);

        //Write disassembly
        writeTextFile(testResultsDir + testName + ".asm.json", mvmDisassembly(code));
        
        //Call logger setup
        JsonLogger  callLogger (testResultsDir + testName + ".calls.json");
        s_curFunctionLogger = &callLogger;
        auto logFn = [](FunctionScope* pScope) -> Ref<JSValue>
        {
            auto entry = pScope->getParam("x");
            
            s_curFunctionLogger->log(entry->getJSON(0));
            return undefined();
        };
        //addNative("function callLogger(x)", logFn, globals);

        //Execution
        //mvmExecute(code, globals);
        asBlockingExec(code, globals);
    }
    catch (const CScriptException &e)
    {
        printf("ERROR: %s\n", e.what());
    }

    //Write globals
    writeTextFile(testResultsDir + testName + ".globals.json", globals->toObject()->getJSON(0));

    bool pass = null2undef( globals->get("result") )->toBoolean();

    if (pass)
        printf("PASS\n");
    else
        printf("FAIL\n");

    return pass;
}

int main(int argc, char **argv)
{
    const string testsDir = "./tests/";
    const string resultsDir = "./tests/results/";
    
    printf("TinyJS test runner\n");
    printf("USAGE:\n");
    printf("   ./run_tests test.js       : run just one test\n");
    printf("   ./run_tests               : run all tests\n");
    if (argc == 2)
    {
        printf("Running test: %s\n", argv[1]);
        
        return !run_test(testsDir + argv[1], testsDir, resultsDir);
    }
    else
        printf("Running all tests!\n");

    int test_num = 1;
    int count = 0;
    int passed = 0;
    
    //TODO: Run all tests in the directory (or even in subdirectories). Do not depend
    //on test numbers.

    while (test_num < 1000)
    {
        char name[32];
        sprintf_s(name, "test%03d.js", test_num);
        
        const string    szPath = testsDir + name;
        // check if the file exists - if not, assume we're at the end of our tests
        FILE *f = fopen(szPath.c_str(), "r");
        if (!f) break;
        fclose(f);

        if (run_test(szPath, testsDir, resultsDir))
            passed++;
        count++;
        test_num++;
    }

    printf("Done. %d tests, %d pass, %d fail\n", count, passed, count - passed);

#ifdef _DEBUG
#ifdef _WIN32
    _CrtDumpMemoryLeaks();
#endif
#endif

    return 0;
}
