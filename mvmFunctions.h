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

#include "JsVars.h"

void registerMvmFunctions(Ref<IScope> scope);

#endif	/* MVMFUNCTIONS_H */

