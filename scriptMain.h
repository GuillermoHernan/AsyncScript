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

#include "JsVars.h"
#include <string>

Ref<JSValue>    evaluate (const char* script, Ref<IScope> globals);

Ref<IScope>     createDefaultGlobals();
Ref<JSObject>   createDefaultGlobalsObj();

Ref<JSFunction> addNative (const std::string& szFunctionHeader, 
                           JSNativeFn pFn, 
                           Ref<IScope> scope);


#endif	/* SCRIPTMAIN_H */

