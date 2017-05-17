/* 
 * File:   modules.h
 * Author: ghernan
 * 
 * Module system for AsyncScript
 *
 * Created on May 15, 2017, 11:09 PM
 */

#pragma once
#ifndef MODULES_H
#define	MODULES_H

struct ExecutionContext;
#include "jsVars.h"


std::string normalizeModulePath (const std::string& modulePath, ExecutionContext *ec);
ASValue     findModule (const std::string& modulePath, ExecutionContext *ec);
ASValue     loadModule (const std::string& modulePath, ExecutionContext *ec);
void        mixModule (ASValue target, ASValue source, ExecutionContext *ec);


/**
 * Contains module information for a AsyncScript program.
 */
struct Modules
{
    typedef std::map<std::string, ASValue>    ModMap;
    
    ModMap  modules;
};


#endif	/* MODULES_H */

