/* 
 * File:   scriptMain.h
 * Author: ghernan
 * 
 * Script engine main file. Contains 'evaluate' function, which runs an script
 * contained in a string.
 * 
 * Created on December 4, 2016, 10:31 PM
 */

#ifndef SCRIPTMAIN_H
#define	SCRIPTMAIN_H
#pragma once

#include "jsVars.h"
#include "executionScope.h"
#include <string>

Ref<JSValue>    evaluate (const char* script, Ref<IScope> globals);

Ref<GlobalScope> createDefaultGlobals();

Ref<JSFunction> addNative (const std::string& szFunctionHeader, 
                           JSNativeFn pFn, 
                           Ref<IScope> scope);

Ref<JSFunction> addNative0 (const std::string& szName, 
                           JSNativeFn pFn, 
                           Ref<IScope> scope);

Ref<JSFunction> addNative1 (const std::string& szName, 
                            const std::string& p1, 
                           JSNativeFn pFn, 
                           Ref<IScope> scope);

Ref<JSFunction> addNative2 (const std::string& szName, 
                            const std::string& p1, 
                            const std::string& p2, 
                           JSNativeFn pFn, 
                           Ref<IScope> scope);


#endif	/* SCRIPTMAIN_H */

