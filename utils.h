/*
 * TinyJS
 *
 * Miscellaneous functions.
 */

#pragma once

#include <string>
#include <stdexcept>
#include <sstream>
#include <stdarg.h>

#include "TinyJS_Lexer.h"

struct ScriptPosition;

bool isWhitespace(char ch);

bool isNumeric(char ch);
bool isNumber(const std::string &str);
bool isHexadecimal(char ch);
bool isOctal(char ch);
bool isOctal(const std::string& str);
bool isAlpha(char ch);
bool isAlphaNum(const std::string &str);
bool isIDString(const char *s);
void replace(std::string &str, char textFrom, const char *textTo);

int copyWhile(char* dest, const char* src, bool (*conditionFN)(char), int maxLen);

const char* skipWhitespace(const char* input);
const char* skipNumeric(const char* input);
const char* skipHexadecimal(const char* input);

/// convert the given string into a quoted string suitable for javascript
std::string escapeString(const std::string &str, bool quote = true);

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

//Remove if compiling with c++ 2011 or later

template <class T>
std::string to_string(const T &x)
{
    std::ostringstream output;

    output << x;
    return output.str();
}

double getNaN();

std::string readTextFile (const std::string& szPath);
bool writeTextFile (const std::string& szPath, const std::string& szContent);
bool createDirIfNotExist (const std::string& szPath);

std::string parentPath (const std::string& szPath);
std::string removeExt (const std::string& szPath);
std::string fileFromPath (const std::string& szPath);

