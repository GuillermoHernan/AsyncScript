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

#include "utils.h"
#include "scriptMain.h"

#include <assert.h>
#include <sys/stat.h>
#include <string>
#include <sstream>
#include <stdio.h>

#ifdef MTRACE
#include <mcheck.h>
#endif

//#define INSANE_MEMORY_DEBUG

#ifdef INSANE_MEMORY_DEBUG
// needs -rdynamic when compiling/linking
#include <execinfo.h>
#include <malloc.h>
#include <map>
#include <vector>
using namespace std;

void **get_stackframe()
{
    void **trace = (void**) malloc(sizeof (void*)*17);
    int trace_size;

    for (int i = 0; i < 17; i++) trace[i] = (void*) 0;
    trace_size = backtrace(trace, 16);
    return trace;
}

void print_stackframe(char *header, void **trace)
{
    char **messages = (char **) NULL;
    int trace_size = 0;

    trace_size = 0;
    while (trace[trace_size]) trace_size++;
    messages = backtrace_symbols(trace, trace_size);

    printf("%s\n", header);
    for (int i = 0; i < trace_size; ++i)
    {
        printf("%s\n", messages[i]);
    }
    //free(messages);
}

/* Prototypes for our hooks.  */
static void *my_malloc_hook(size_t, const void *);
static void my_free_hook(void*, const void *);
static void *(*old_malloc_hook) (size_t, const void *);
static void (*old_free_hook) (void*, const void *);

map<void *, void **> malloced;

static void *my_malloc_hook(size_t size, const void *caller)
{
    /* Restore all old hooks */
    __malloc_hook = old_malloc_hook;
    __free_hook = old_free_hook;
    /* Call recursively */
    printf ("malloc (%u)... ", (unsigned int) size);
    void *result = malloc(size);
    /* we call malloc here, so protect it too. */
    printf ("returns %p\n", result);
    //malloced[result] = get_stackframe();

    /* Restore our own hooks */
    __malloc_hook = my_malloc_hook;
    __free_hook = my_free_hook;
    return result;
}

static void my_free_hook(void *ptr, const void *caller)
{
    /* Restore all old hooks */
    __malloc_hook = old_malloc_hook;
    __free_hook = old_free_hook;
    /* Call recursively */
    printf ("freeing pointer %p\n", ptr);
    free(ptr);
    /* we call malloc here, so protect it too. */
    if (malloced.find(ptr) == malloced.end())
    {
        /*fprintf(stderr, "INVALID FREE\n");
        void *trace[16];
        int trace_size = 0;
        trace_size = backtrace(trace, 16);
        backtrace_symbols_fd(trace, trace_size, STDERR_FILENO);*/
    }
    else
        malloced.erase(ptr);
    /* Restore our own hooks */
    __malloc_hook = my_malloc_hook;
    __free_hook = my_free_hook;
}

void memtracing_init()
{
    old_malloc_hook = __malloc_hook;
    old_free_hook = __free_hook;
    __malloc_hook = my_malloc_hook;
    __free_hook = my_free_hook;
}

long gethash(void **trace)
{
    unsigned long hash = 0;
    while (*trace)
    {
        hash = (hash << 1) ^ (hash >> 63) ^ (unsigned long) *trace;
        trace++;
    }
    return hash;
}

void memtracing_kill()
{
    /* Restore all old hooks */
    __malloc_hook = old_malloc_hook;
    __free_hook = old_free_hook;

    map<long, void**> hashToReal;
    map<long, int> counts;
    map<void *, void **>::iterator it = malloced.begin();
    while (it != malloced.end())
    {
        long hash = gethash(it->second);
        hashToReal[hash] = it->second;

        if (counts.find(hash) == counts.end())
            counts[hash] = 1;
        else
            counts[hash]++;

        it++;
    }

    vector<pair<int, long> > sorting;
    map<long, int>::iterator countit = counts.begin();
    while (countit != counts.end())
    {
        sorting.push_back(pair<int, long>(countit->second, countit->first));
        countit++;
    }

    // sort
    bool done = false;
    while (!done)
    {
        done = true;
        for (size_t i = 0; i < sorting.size() - 1; i++)
        {
            if (sorting[i].first < sorting[i + 1].first)
            {
                pair<int, long> t = sorting[i];
                sorting[i] = sorting[i + 1];
                sorting[i + 1] = t;
                done = false;
            }
        }
    }


    for (size_t i = 0; i < sorting.size(); i++)
    {
        long hash = sorting[i].second;
        int count = sorting[i].first;
        char header[256];
        sprintf(header, "--------------------------- LEAKED %d", count);
        print_stackframe(header, hashToReal[hash]);
    }
}
#endif // INSANE_MEMORY_DEBUG

bool run_test(const char *filename)
{
    printf("TEST %s ", filename);
    struct stat results;
    if (!stat(filename, &results) == 0)
    {
        printf("Cannot stat file! '%s'\n", filename);
        return false;
    }
    int size = results.st_size;
    FILE *file = fopen(filename, "rb");
    /* if we open as text, the number of bytes read may be > the size we read */
    if (!file)
    {
        printf("Unable to open file! '%s'\n", filename);
        return false;
    }
    char *buffer = new char[size + 1];
    long actualRead = fread(buffer, 1, size, file);
    buffer[actualRead] = 0;
    buffer[size] = 0;
    fclose(file);

    Ref<JSObject> globalsObj = createDefaultGlobalsObj();
    Ref<IScope> globals = ObjectScope::create(globalsObj);
    
//    registerFunctions(&s);
//    registerMathFunctions(&s);
    globals->set("result", jsInt(0));
    try
    {
        evaluate(buffer, globals);
        //s.execute(buffer);
    }
    catch (const CScriptException &e)
    {
        printf("ERROR: %s\n", e.what());
    }
    bool pass = null2undef( globals->get("result") )->toBoolean();

    if (pass)
        printf("PASS\n");
    else
    {
        char fn[64];
        sprintf(fn, "%s.fail.js", filename);
        FILE *f = fopen(fn, "wt");
        if (f)
        {
            std::string symbols = globalsObj->getJSON(0);
            fprintf(f, "%s", symbols.c_str());
            fclose(f);
        }

        printf("FAIL - symbols written to %s\n", fn);
    }

    delete[] buffer;
    return pass;
}

int main(int argc, char **argv)
{
#ifdef MTRACE
    mtrace();
#endif
#ifdef INSANE_MEMORY_DEBUG
    memtracing_init();
#endif
    printf("TinyJS test runner\n");
    printf("USAGE:\n");
    printf("   ./run_tests test.js       : run just one test\n");
    printf("   ./run_tests               : run all tests\n");
    if (argc == 2)
    {
        printf("Running test: %s\n", argv[1]);
        
        return !run_test(argv[1]);
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
        char fn[32];
        sprintf(fn, "tests/test%03d.js", test_num);
        // check if the file exists - if not, assume we're at the end of our tests
        FILE *f = fopen(fn, "r");
        if (!f) break;
        fclose(f);

        if (run_test(fn))
            passed++;
        count++;
        test_num++;
    }

    printf("Done. %d tests, %d pass, %d fail\n", count, passed, count - passed);
#ifdef INSANE_MEMORY_DEBUG
    memtracing_kill();
#endif

#ifdef _DEBUG
#ifdef _WIN32
    _CrtDumpMemoryLeaks();
#endif
#endif
#ifdef MTRACE
    muntrace();
#endif

    return 0;
}
