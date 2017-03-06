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

    CScriptException(const std::string &text, const ScriptPosition& pos) : 
    logic_error(text), Position(pos)
    {
    }

    const ScriptPosition    Position;
};

/**
 * Class for exceptions which may occur executing the script.
 */
class RuntimeError : public std::logic_error
{
public:
    RuntimeError (const std::string& text, const VmPosition& pos) 
    : logic_error(text), Position(pos)
    {
    }
    
    const VmPosition    Position;
};

//Exception helper functions
void rtError(const char* msgFormat, ...);

void errorAt(const ScriptPosition& position, const char* msgFormat, ...);
void errorAt_v(const ScriptPosition& position, const char* msgFormat, va_list args);

#endif	/* SCRIPTEXCEPTION_H */

