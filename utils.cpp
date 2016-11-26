/*
 * TinyJS
 * 
 * Miscellaneous functions implementation file.
 */

#include "utils.h"
#include "OS_support.h"
#include "TinyJS_Lexer.h"

#include <string>
#include <string.h>
#include <sstream>
#include <cstdlib>
#include <stdio.h>

using namespace std;

// ----------------------------------------------------------------------------------- Utils

bool isWhitespace(char ch)
{
    return (ch == ' ') || (ch == '\t') || (ch == '\n') || (ch == '\r');
}

bool isNumeric(char ch)
{
    return (ch >= '0') && (ch <= '9');
}

bool isNumber(const string &str)
{
    for (size_t i = 0; i < str.size(); i++)
        if (!isNumeric(str[i])) return false;
    return true;
}

bool isHexadecimal(char ch)
{
    return ((ch >= '0') && (ch <= '9')) ||
            ((ch >= 'a') && (ch <= 'f')) ||
            ((ch >= 'A') && (ch <= 'F'));
}

bool isOctal(char ch)
{
    return (ch >= '0') && (ch <= '7');
}

bool isAlpha(char ch)
{
    return ((ch >= 'a') && (ch <= 'z')) || ((ch >= 'A') && (ch <= 'Z')) || ch == '_';
}

bool isIDString(const char *s)
{
    if (!isAlpha(*s))
        return false;
    while (*s)
    {
        if (!(isAlpha(*s) || isNumeric(*s)))
            return false;
        s++;
    }
    return true;
}

void replace(string &str, char textFrom, const char *textTo)
{
    int sLen = strlen(textTo);
    size_t p = str.find(textFrom);
    while (p != string::npos)
    {
        str = str.substr(0, p) + textTo + str.substr(p + 1);
        p = str.find(textFrom, p + sLen);
    }
}

int copyWhile(char* dest, const char* src, bool (*conditionFN)(char), int maxLen)
{
    int i = 0;
    for (i = 0; src[i] && i < maxLen && conditionFN(src[i]); ++i)
        dest[i] = src[i];

    dest[i] = 0;

    return i;
}

const char* skipWhitespace(const char* input)
{
    while (isWhitespace(*input))
        ++input;

    return input;
}

const char* skipNumeric(const char* input)
{
    while (isNumeric(*input))
        ++input;

    return input;
}

const char* skipHexadecimal(const char* input)
{
    while (isHexadecimal(*input))
        ++input;

    return input;
}



/// convert the given string into a quoted string suitable for javascript

std::string getJSString(const std::string &str)
{
    std::string nStr = str;
    for (size_t i = 0; i < nStr.size(); i++)
    {
        const char *replaceWith = "";
        bool replace = true;

        switch (nStr[i])
        {
        case '\\': replaceWith = "\\\\";
            break;
        case '\n': replaceWith = "\\n";
            break;
        case '\r': replaceWith = "\\r";
            break;
        case '\a': replaceWith = "\\a";
            break;
        case '"': replaceWith = "\\\"";
            break;
        default:
        {
            int nCh = ((int) nStr[i]) &0xFF;
            if (nCh < 32 || nCh > 127)
            {
                char buffer[16];
                sprintf_s(buffer, "\\x%02X", nCh);
                replaceWith = buffer;
            }
            else replace = false;
        }
        }

        if (replace)
        {
            nStr = nStr.substr(0, i) + replaceWith + nStr.substr(i + 1);
            i += strlen(replaceWith) - 1;
        }
    }
    return "\"" + nStr + "\"";
}

/** Is the string alphanumeric */
bool isAlphaNum(const std::string &str)
{
    if (str.size() == 0) return true;
    if (!isAlpha(str[0])) return false;
    for (size_t i = 0; i < str.size(); i++)
        if (!(isAlpha(str[i]) || isNumeric(str[i])))
            return false;
    return true;
}

std::string generateErrorMessage(const ScriptPosition* pPos, const char* msgFormat, va_list args)
{
    char buffer [2048];
    string message;

    if (pPos)
    {
        sprintf_s(buffer, "(line: %d, col: %d): ", pPos->line, pPos->column);
        message = buffer;
    }

    vsprintf_s(buffer, msgFormat, args);
    message += buffer;

    return message;
}

/**
 * Generates an exception, with the error message
 * @param msgFormat 'printf-like' format string
 * @param ...       Optional message parameters
 */
void error(const char* msgFormat, ...)
{
    va_list aptr;

    va_start(aptr, msgFormat);
    const std::string message = generateErrorMessage(NULL, msgFormat, aptr);
    va_end(aptr);

    throw CScriptException(message);
}

/**
 * Generates an error message located at the given position
 * @param code      Pointer to the code location where the error occurs. 
 * It is used to calculate line and column for the error message
 * @param msgFormat 'printf-like' format string
 * @param ...       Optional message parameters
 */
void errorAt(const ScriptPosition& position, const char* msgFormat, ...)
{
    va_list aptr;

    va_start(aptr, msgFormat);
    const std::string message = generateErrorMessage(&position, msgFormat, aptr);
    va_end(aptr);

    throw CScriptException(message);
}

void errorAt_v(const ScriptPosition& position, const char* msgFormat, va_list args)
{
    const std::string message = generateErrorMessage(&position, msgFormat, args);

    throw CScriptException(message);
}
