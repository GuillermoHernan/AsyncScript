/* 
 * File:   ScriptException.h
 * Author: ghernan
 *
 * Exception class to manage script errors.
 * 
 * Created on March 1, 2017, 8:27 PM
 */

#ifndef SCRIPTEXCEPTION_H
#define	SCRIPTEXCEPTION_H

#pragma once

#include "ScriptPosition.h"

#include <stdexcept>
#include <string>


/**
 * Exceptions throw in script execution / parsing
 */
class CScriptException : public std::logic_error
{
public:

    CScriptException(const std::string &text) : logic_error(text)
    {
    }

    CScriptException(const char *text) : logic_error(text)
    {
    }
};

//Exception helper functions
void error(const char* msgFormat, ...);
void errorAt(const ScriptPosition& position, const char* msgFormat, ...);
void errorAt_v(const ScriptPosition& position, const char* msgFormat, va_list args);

#endif	/* SCRIPTEXCEPTION_H */

