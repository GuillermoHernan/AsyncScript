/* 
 * File:   parserResults.h
 * Author: ghernan
 * 
 * Classes to handle results from parsing functions
 *
 * Created on November 30, 2016, 7:56 PM
 */

#ifndef PARSERRESULTS_H
#define	PARSERRESULTS_H

#pragma once

#include "RefCountObj.h"
#include "jsLexer.h"
#include "ast.h"

class JSValue;


/**
 * Result of a 'parse' operation
 */
struct ParseResult
{
    CScriptToken nextToken;
    Ref<AstNode> ast;

    ParseResult(const CScriptToken &token, const Ref<AstNode> _ast)
    : nextToken(token), ast(_ast)
    {
    }

    ParseResult() : nextToken("")
    {
    }
};

/**
 * Represents an error detected during parsing
 */
struct ParseError
{
    ScriptPosition position;
    std::string text;

    bool error()const
    {
        return !text.empty();
    }

    ParseError(ScriptPosition pos, const std::string txt) :
    position(pos),
    text(txt)
    {
    }

    //No error constructor
    ParseError()
    {
    }
};

/**
 * Result of parsing an expression
 */
class ExprResult
{
public:
    CScriptToken token;
    Ref<AstNode> result;
    ParseError errorDesc;

    ExprResult(const CScriptToken &nextToken,
                    const Ref<AstNode> expr)
    : token(nextToken), result(expr), m_initialToken(token)
    {
    }

    ExprResult(const CScriptToken &initialToken,
                    const ParseError& err)
    : token(initialToken), errorDesc(err), m_initialToken(initialToken)
    {
    }

    ExprResult(const CScriptToken &initialToken)
    : token(initialToken), m_initialToken(token)
    {
    }

    typedef ExprResult(*ParseFunction)(CScriptToken token);
    typedef ExprResult(*ChainParseFunction)(CScriptToken token, Ref<AstNode> prev);
    typedef bool (*TokenCheck)(CScriptToken token);

    ExprResult orElse(ParseFunction parseFn);
    ExprResult then(ParseFunction parseFn);
    ExprResult then(ChainParseFunction parseFn);
    ExprResult require(TokenCheck checkFn);
    ExprResult require(int tokenType);
    ExprResult requireId(const char* text);
    ExprResult skip();
    ExprResult getError(const char* format, ...);

    bool error()const
    {
        return errorDesc.error();
    }
    
    bool ok()const
    {
        return !error();
    }

    void throwIfError()const;
    
    ParseResult toParseResult()const;
    
    ExprResult final()const;
    
private:
    CScriptToken    m_initialToken;
};

#endif	/* PARSERRESULTS_H */

