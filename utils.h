/*
 * TinyJS
 *
 * Miscellaneous functions.
 */

#pragma once

#include <string>
#include <stdexcept>

bool isWhitespace(char ch);

bool isNumeric(char ch);
bool isNumber(const std::string &str);
bool isHexadecimal(char ch);
bool isAlpha(char ch);
bool isAlphaNum(const std::string &str);
bool isIDString(const char *s);
void replace(std::string &str, char textFrom, const char *textTo);

/// convert the given string into a quoted string suitable for javascript
std::string getJSString(const std::string &str);

/**
 * Exceptions throw in script execution / parsing
 */
class CScriptException : public std::logic_error {
public:
    CScriptException(const std::string &text): logic_error(text) {}
    CScriptException(const char *text): logic_error(text) {}
};
