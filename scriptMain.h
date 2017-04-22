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
#include "asObjects.h"
#include <string>

class MvmRoutine;

ASValue    evaluate (const char* script, Ref<JSObject> globals);
ASValue    evaluate (Ref<MvmRoutine> code, const CodeMap* codeMap, Ref<JSObject> globals);

Ref<JSObject> createDefaultGlobals();

Ref<JSFunction> addNative (const std::string& szFunctionHeader, 
                           JSNativeFn pFn,
                           VarMap& varMap);

Ref<JSFunction> addNative (const std::string& szFunctionHeader, 
                           JSNativeFn pFn, 
                           Ref<JSObject> obj,
                           bool isConst=true);

Ref<JSFunction> addNative0 (const std::string& szName, 
                           JSNativeFn pFn, 
                           Ref<JSObject> container);

Ref<JSFunction> addNative1 (const std::string& szName, 
                            const std::string& p1, 
                           JSNativeFn pFn, 
                           Ref<JSObject> container);

Ref<JSFunction> addNative2 (const std::string& szName, 
                            const std::string& p1, 
                            const std::string& p2, 
                           JSNativeFn pFn, 
                           Ref<JSObject> container);


#endif	/* SCRIPTMAIN_H */

