/* 
 * File:   mvmFunctions.h
 * Author: ghernan
 *
 * Functions which implement Micro virtual machine primitive operations
 * 
 * Created on December 4, 2016, 11:48 PM
 */

#ifndef MVMFUNCTIONS_H
#define	MVMFUNCTIONS_H

#pragma once

#include "jsVars.h"
#include "asObjects.h"

void registerMvmFunctions(Ref<JSObject> scope);

//bool mvmAreEqual (ASValue opA, ASValue opB);
//bool mvmAreTypeEqual (ASValue opA, ASValue opB);



#endif	/* MVMFUNCTIONS_H */

