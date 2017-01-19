/* 
 * File:   parserResults.cpp
 * Author: ghernan
 * 
 * Created on November 30, 2016, 7:56 PM
 */

#include "OS_support.h"
#include "parserResults.h"
#include "utils.h"

#include <stdarg.h>

/**
 * If the current result is an error, executes the parse function.
 * If not, it doesn't call the parse function.
 * @param parseFn
 * @return 
 */
ExprResult ExprResult::orElse(ParseFunction parseFn)
{
    if (ok())
        return *this;
    
    ExprResult r = parseFn(token);
    r.m_initialToken = m_initialToken;
    
    if (r.ok() || r.errorDesc.position > this->errorDesc.position)
        return r;
    else
        return *this;
}

/**
 * If the current result is a success, executes the parse function and returns
 * its result.
 * Version for regular parse functions.
 * @param parseFn
 * @return 
 */
ExprResult ExprResult::then(ParseFunction parseFn)
{
    if (error())
        return *this;

    ExprResult r = parseFn(token);
    r.m_initialToken = m_initialToken;
    
    return r;    
}

/**
 * If the current result is a success, executes the parse function and returns
 * its result.
 * Version for chained parse functions. Chained parse functions receive the current
 * result as parameter.
 * @param parseFn
 * @return 
 */
ExprResult ExprResult::then(ChainParseFunction parseFn)
{
    if (error())
        return *this;

    ExprResult r = parseFn(token, result);
    r.m_initialToken = m_initialToken;
    
    return r;    
}

/**
 * Requires that the current token complies with certain condition.
 * The 'condition' is a function which receives a token, and return a boolean
 * value.
 * If the check is successful, advances to the next token in the result.
 * @param checkFn   Token check function.
 * @return 
 */
ExprResult ExprResult::require(TokenCheck checkFn)
{
    if (error())
        return *this;
    
    if (checkFn(token))
    {
        ExprResult r (token.next(), result);
        r.m_initialToken = m_initialToken;
        return r;
    }
    else
        return getError("Unexpected token: '%s'", token.text().c_str());
}

/**
 * Requires that the current token is of the specified type.
 * If the check is successful, advances to the next token in the result.
 * @param tokenType     Token type
 * @return 
 */
ExprResult ExprResult::require(int tokenType)
{
    if (error())
        return *this;
    
    if (token.type() == tokenType)
    {
        ExprResult r (token.next(), result);
        r.m_initialToken = m_initialToken;
        return r;
    }
    else
        return getError("Unexpected token: '%s'", token.text().c_str());
}

/**
 * Requires that the current token is an identifier with the given text.
 * @param text
 * @return 
 */
ExprResult ExprResult::requireId(const char* text)
{
    if (error())
        return *this;
    
    ExprResult  r = require(LEX_ID);
    
    if (r.ok() && token.text() == text)
        return r;
    else
        return getError("Unexpected id. '%s' expected, '%s' found", text, token.text().c_str());
}

/**
 * Skips next token. Forwards the previous result
 * @return 
 */
ExprResult ExprResult::skip()
{
    if (error())
        return *this;
    
    ExprResult  r(token.next(), result);
    r.m_initialToken = m_initialToken;
    
    return r;    
}


/**
 * Gets an error result located at the current token.
 * @param format
 * @param ...
 * @return 
 */
ExprResult ExprResult::getError(const char* format, ...)
{
    va_list args;
    char buffer[2048];

    va_start(args, format);
    vsprintf_s(buffer, format, args);
    va_end(args);
    
    ExprResult result (token, ParseError(token.getPosition(), buffer));
    result.m_initialToken = m_initialToken;

    return result;
    
}

/**
 * Throws a script exception if it is an error result.
 */
void ExprResult::throwIfError()const
{
    if (error())
        errorAt(errorDesc.position, "%s", errorDesc.text.c_str());
}
    
/**
 * Transforms a 'ExprResult' into a 'ParseResult'
 * If it is an error result, throws an exception, because the ParseResult 
 * structure doesn't contain error information
 * @return 
 */
ParseResult ExprResult::toParseResult()const
{
    throwIfError();
    
    return ParseResult(token, result);
}

/**
 * If the result is an error, relocates it to the initial token
 * @return 
 */
ExprResult ExprResult::final()const
{
    if (ok())
        return *this;
    else
        return ExprResult(m_initialToken, errorDesc);
}
