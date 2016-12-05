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

#include <assert.h>
#include <sys/stat.h>
#include <string>
#include <sstream>
#include <stdio.h>

using namespace std;

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

    Ref<JSObject> globalsObj = createDefaultGlobalsObj();
    Ref<IScope> globals = ObjectScope::create(globalsObj);
    
    globals->set("result", jsInt(0));
    try
    {
        //This code is copied from 'evalute', to log the intermediate results 
        //generated from each state
        CScriptToken    token (script.c_str());
        StatementList   statements;

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

        //Code generation.
        const Ref<MvmScript>    code = scriptCodegen(statements);

        //Write disassembly
        writeTextFile(testResultsDir + testName + ".asm.json", mvmDisassembly(code));

        //Execution
        mvmExecute(code, globals);
    }
    catch (const CScriptException &e)
    {
        printf("ERROR: %s\n", e.what());
    }

    //Write globals
    writeTextFile(testResultsDir + testName + ".globals.json", globalsObj->getJSON(0));

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
