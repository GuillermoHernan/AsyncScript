/* 
 * File:   jsLexer.h
 * Author: ghernan
 * 
 * Javascript lexical analysis code
 *
 */

#pragma once

#include "ScriptPosition.h"

enum LEX_TYPES
{
    LEX_EOF = 0,
    LEX_INITIAL,
    LEX_COMMENT,
    
    LEX_ID = 256,
    LEX_INT,
    LEX_FLOAT,
    LEX_STR,

    LEX_EQUAL,
    LEX_TYPEEQUAL,
    LEX_NEQUAL,
    LEX_NTYPEEQUAL,
    LEX_LEQUAL,
    LEX_LSHIFT,
    LEX_GEQUAL,
    LEX_RSHIFT,
    LEX_RSHIFTUNSIGNED,
    LEX_PLUSPLUS,
    LEX_MINUSMINUS,
    LEX_ANDAND,
    LEX_OROR,
    LEX_POWER,
    LEX_CONNECT,
    LEX_SEND,

    LEX_ASSIGN_BASE = 512,

//    LEX_PLUSEQUAL = LEX_ASSIGN_BASE + '+',
//    LEX_MINUSEQUAL = LEX_ASSIGN_BASE + '-',
//    LEX_ANDEQUAL = LEX_ASSIGN_BASE + '&',
//    LEX_OREQUAL = LEX_ASSIGN_BASE + '|',
//    LEX_XOREQUAL = LEX_ASSIGN_BASE + '^',
//    LEX_LSHIFTEQUAL,
//    LEX_RSHIFTEQUAL,
    LEX_ASSIGN_MAX = 1023,
    
    // reserved words
    LEX_R_WORDS_BASE = 1024,
    LEX_R_IF,
    LEX_R_ELSE,
    LEX_R_DO,
    LEX_R_WHILE,
    LEX_R_FOR,
    LEX_R_BREAK,
    LEX_R_CONTINUE,
    LEX_R_FUNCTION,
    LEX_R_RETURN,
    LEX_R_VAR,
    LEX_R_CONST,
    LEX_R_TRUE,
    LEX_R_FALSE,
    LEX_R_NULL,
    LEX_R_NEW,
    
    //Actor system keywords
    LEX_R_ACTOR,
    LEX_R_INPUT,
    LEX_R_OUTPUT,
    LEX_R_PROTOCOL,
    LEX_R_SOCKET,
    
    //Classes and objects keywords.
    LEX_R_CLASS,
    
    //Modules keywords
    LEX_R_EXPORT,
    LEX_R_IMPORT,

    LEX_R_LIST_END /* always the last entry */
};

/// To get the string representation of a token type
std::string getTokenStr(int token);


/**
 * Javascript token. Tokens are the fragments in which input source is divided
 * and classified before being parsed.
 * 
 * The lexical analysis process is implemented taking a functional approach. There
 * is no 'lexer' object. There are functions which return the current state of the
 * lex process as immutable 'CScriptToken' objects. 
 * These objects are not strictly 'immutable', as they have assignment operator. But none
 * of their public methods modify its internal state.
 */
class CScriptToken
{
public:

    /**
     * The constructor doesn't make a copy of the input string, so it is important
     * not to delete input string while there are still live 'CSriptTokens' using it.
     * 
     * The token created with the constructor is not parsed from input string. It is
     * just the 'initial' token. To parse the first real token, call 'next'.
     */
    CScriptToken(const char* code);

    CScriptToken(LEX_TYPES lexType, const char* code, const ScriptPosition& position, int length);

    /// Reads next token from input, and returns it.
    CScriptToken next(bool skipComments = true)const;

    /// Checks that the current token matches the expected, and returns next
    CScriptToken match(int expected_tk)const;

    ///Return a string representing the position in lines and columns of the token

    const ScriptPosition& getPosition()const
    {
        return m_position;
    }

    LEX_TYPES type()const
    {
        return m_type;
    }
    
    bool eof()const
    {
        return m_type == LEX_EOF;
    }
    std::string text()const;

    const char* code()const
    {
        return m_code;
    }

    std::string strValue()const;

private:
    const char* m_code;
    LEX_TYPES m_type; ///<Token type.
    ScriptPosition m_position;
    int m_length;

    CScriptToken nextDispatch()const;

    CScriptToken buildNextToken(LEX_TYPES lexType, const char* code, int length)const;
    CScriptToken parseComment(const char * code)const;
    CScriptToken parseId(const char * code)const;
    CScriptToken parseNumber(const char * code)const;
    CScriptToken parseString(const char * code)const;
    CScriptToken parseOperator(const char * code)const;

    ScriptPosition calcPosition(const char* code)const;
    CScriptToken errorAt(const char* charPos, const char* msgFormat, ...)const;
};