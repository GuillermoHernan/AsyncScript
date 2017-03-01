/* 
 * File:   jsLexer.cpp
 * Author: ghernan
 * 
 * Javascript lexical analysis code
 *
 */

#include "ascript_pch.hpp"
#include "jsLexer.h"
#include "utils.h"

#include "ScriptException.h"

#include <map>
#include <string>
#include <string.h>
#include <sstream>
#include <cstdlib>
#include <stdio.h>

using namespace std;

string getTokenStr(int token)
{
    if (token > 32 && token < 128)
    {
        char buf[2] = "x";
        buf[0] = (char)token;
        return buf;
    }
    else if (token > LEX_ASSIGN_BASE && token < LEX_R_WORDS_BASE)
    {
        return getTokenStr(token - LEX_ASSIGN_BASE) + "=";
    }
    switch (token)
    {
    case LEX_EOF: return "EOF";
    case LEX_ID: return "ID";
    case LEX_INT: return "INT";
    case LEX_FLOAT: return "FLOAT";
    case LEX_STR: return "STRING";
    case LEX_EQUAL: return "==";
    case LEX_TYPEEQUAL: return "===";
    case LEX_NEQUAL: return "!=";
    case LEX_NTYPEEQUAL: return "!==";
    case LEX_LEQUAL: return "<=";
    case LEX_LSHIFT: return "<<";
    case LEX_GEQUAL: return ">=";
    case LEX_RSHIFT: return ">>";
    case LEX_RSHIFTUNSIGNED: return ">>";
    case LEX_PLUSPLUS: return "++";
    case LEX_MINUSMINUS: return "--";
    case LEX_ANDAND: return "&&";
    case LEX_OROR: return "||";
    case LEX_POWER: return "**";
    case LEX_CONNECT: return "<-";
    case LEX_SEND: return "->";
        // reserved words
    case LEX_R_IF: return "if";
    case LEX_R_ELSE: return "else";
    case LEX_R_DO: return "do";
    case LEX_R_WHILE: return "while";
    case LEX_R_FOR: return "for";
    case LEX_R_BREAK: return "break";
    case LEX_R_CONTINUE: return "continue";
    case LEX_R_FUNCTION: return "function";
    case LEX_R_RETURN: return "return";
    case LEX_R_VAR: return "var";
    case LEX_R_CONST: return "const";
    case LEX_R_TRUE: return "true";
    case LEX_R_FALSE: return "false";
    case LEX_R_NULL: return "null";
    case LEX_R_NEW: return "new";
    
    case LEX_R_ACTOR:   return "actor";
    case LEX_R_INPUT:   return "input";
    case LEX_R_OUTPUT:  return "output";
    case LEX_R_PROTOCOL:return "protocol";
    case LEX_R_SOCKET:  return "socket";

    case LEX_R_CLASS:  return "class";

    }

    ostringstream msg;
    msg << "?[" << token << "]";
    return msg.str();
}

/**
 * The constructor doesn't make a copy of the input string, so it is important
 * not to delete input string while there are still live 'CSriptTokens' using it.
 * 
 * The token created with the constructor is not parsed from input string. It is
 * just the 'initial' token. To parse the first real token, call 'next'.
 */
CScriptToken::CScriptToken(const char* code) 
: m_code(code)
, m_type(LEX_INITIAL)
, m_position(1, 1)
, m_length(0)
{
}

CScriptToken::CScriptToken(LEX_TYPES lexType, const char* code, const ScriptPosition& position, int length)
: m_code(code)
, m_type(lexType)
, m_position(position)
, m_length(length)
{
}

/**
 * Returns full token text
 * @return 
 */
std::string CScriptToken::text()const
{
    return string(m_code, m_code + m_length);
}

/**
 * Gets the value of a string constant. Replaces escape sequences, and removes
 * initial and final quotes.
 * @return 
 */
std::string CScriptToken::strValue()const
{
    ASSERT(type() == LEX_STR);

    string result;
    int i;

    result.reserve(m_length);

    for (i = 1; i < m_length - 1; ++i)
    {
        const char c = m_code[i];
        char buf[8];

        if (c != '\\')
            result.push_back(c);
        else
        {
            //TODO: Support for Unicode escape sequences.
            ++i;
            switch (m_code[i])
            {
            case 'b': result.push_back('\b');
                break;
            case 'f': result.push_back('\f');
                break;
            case 'n': result.push_back('\n');
                break;
            case 'r': result.push_back('\r');
                break;
            case 't': result.push_back('\t');
                break;
            case 'v': result.push_back('\v');
                break;
            case '\'': result.push_back('\'');
                break;
            case '\"': result.push_back('\"');
                break;
            case 'x':
                copyWhile(buf, m_code + 1, isHexadecimal, 2);
                result.push_back((char) strtol(buf, 0, 16));
                break;
            default:
                copyWhile(buf, m_code + 1, isOctal, 3);
                if (buf[0] != 0)
                    result.push_back((char) strtol(buf, 0, 8));
                else
                    result.push_back(m_code[i]);
            }//switch
        }
    }//for 

    return result;
}

/**
 * Creates a token, next to the current one, with the specified type
 * @param lexType
 * @param code
 * @param length
 * @return 
 */
CScriptToken CScriptToken::buildNextToken(LEX_TYPES lexType, const char* code, int length)const
{
    return CScriptToken(lexType, code, calcPosition(code), length);
}

/**
 * Reads next token from input, and returns it.
 * @param skipComments  To indicate if 'comment' tokens must be skipped
 * @return Next token
 */
CScriptToken CScriptToken::next(bool skipComments)const
{
    CScriptToken result = nextDispatch();

    if (skipComments && result.type() == LEX_COMMENT)
        return result.next(true);
    else
        return result;
}

/// Reads next token from input, and returns it.

CScriptToken CScriptToken::nextDispatch()const
{
    const char* currCh = skipWhitespace(m_code + m_length);

    if (*currCh == '/')
    {
        CScriptToken comment = parseComment(currCh);
        if (comment.type() != LEX_COMMENT)
            return parseOperator(currCh);
        else
            return comment;
    }
    else if (isAlpha(*currCh))
        return parseId(currCh);
    else if (isNumeric(*currCh))
        return parseNumber(currCh);
    else if (*currCh == '\"' || *currCh == '\'')
        return parseString(currCh);
    else if (*currCh != 0)
        return parseOperator(currCh);
    else
        return buildNextToken(LEX_EOF, currCh, 0);
}

CScriptToken CScriptToken::match(int expected_tk)const
{
    if (type() != expected_tk)
    {
        ostringstream errorString;
        errorString << "Got '" << text() << "' expected " << getTokenStr(expected_tk);
        return errorAt(m_code, "%s", errorString.str().c_str());
    }
    else
        return next();
}

/**
 * Parses commentaries. Both single line and multi-line
 * @param code Pointer to comment code start.
 * @return 
 */
CScriptToken CScriptToken::parseComment(const char * const code)const
{
    const char* end = code + 2;

    if (code[1] == '/')
    {
        while (*end && *end != '\n')
            ++end;
    }
    else if (code[1] == '*')
    {
        while (*end && (end[0] != '*' || end[1] != '/'))
            ++end;

        if (*end == 0)
            errorAt(code, "Unclosed multi-line comment");
        else
            end += 2;
    }
    else
        return CScriptToken(""); //Not a commentary

    return buildNextToken(LEX_COMMENT, code, end - code);
}

/**
 * Parses an identifier (keywords included)
 * @param code Pointer to identifier start.
 * @return An identifier token or a keyword token
 */
CScriptToken CScriptToken::parseId(const char * code)const
{
    typedef map<string, LEX_TYPES> KEYWORD_MAP;
    static KEYWORD_MAP keywords;

    if (keywords.size() == 0)
    {
        keywords["if"] = LEX_R_IF;
        keywords["else"] = LEX_R_ELSE;
        keywords["do"] = LEX_R_DO;
        keywords["while"] = LEX_R_WHILE;
        keywords["for"] = LEX_R_FOR;
        keywords["break"] = LEX_R_BREAK;
        keywords["continue"] = LEX_R_CONTINUE;
        keywords["function"] = LEX_R_FUNCTION;
        keywords["return"] = LEX_R_RETURN;
        keywords["var"] = LEX_R_VAR;
        keywords["const"] = LEX_R_CONST;
        keywords["true"] = LEX_R_TRUE;
        keywords["false"] = LEX_R_FALSE;
        keywords["null"] = LEX_R_NULL;
        keywords["new"] = LEX_R_NEW;

        keywords["actor"] = LEX_R_ACTOR;
        keywords["input"] = LEX_R_INPUT;
        keywords["output"] = LEX_R_OUTPUT;
        keywords["protocol"] = LEX_R_PROTOCOL;
        keywords["socket"] = LEX_R_SOCKET;

        keywords["class"] = LEX_R_CLASS;
    }

    const char* end = code + 1;
    while (isAlpha(*end) || isNumeric(*end))
        ++end;

    KEYWORD_MAP::const_iterator itKw = keywords.find(string(code, end));
    LEX_TYPES type = LEX_ID;

    if (itKw != keywords.end())
        type = itKw->second;

    return buildNextToken(type, code, end - code);
}

/**
 * Parses a number
 * @param code Pointer to number text start.
 * @return An integer or float token.
 */
CScriptToken CScriptToken::parseNumber(const char * code)const
{
    const char * end = code;
    LEX_TYPES type = LEX_INT;

    if (code[0] == '0' && tolower(code[1]) == 'x')
    {
        //Hexadecimal
        end = skipHexadecimal(code + 2);
    }
    else
    {
        //Decimal integers or floats.
        end = skipNumeric(code);

        if (*end == '.')
        {
            type = LEX_FLOAT;

            end = skipNumeric(end + 1);
        }

        // do fancy e-style floating point
        if (tolower(*end) == 'e')
        {
            type = LEX_FLOAT;

            if (end[1] == '+' || end[1] == '-')
                ++end;

            end = skipNumeric(end + 1);
        }
    }

    return buildNextToken(type, code, end - code);
}

/**
 * Parses a string constant
 * @param code Pointer to string constant start
 * @return A string token
 */
CScriptToken CScriptToken::parseString(const char * code)const
{
    const char openChar = *code;
    const char* end;

    for (end = code + 1; *end != openChar; ++end)
    {
        const char c = end[0];
        if (c == '\\' && end[1] != 0)
            ++end;
        else if (c == '\n' || c == '\r')
            errorAt(end, "New line in string constant");
        else if (c == 0)
            errorAt(end, "End of file in string constant");
    }

    return buildNextToken(LEX_STR, code, (end - code) + 1);
}

/**
 * Structure to define available operators
 */
struct SOperatorDef
{
    const char * text;
    const int code;
    const int len;
};

/**
 * Operator definition table. It is important that longer operators are added
 * before sorter ones.
 */
const SOperatorDef s_operators [] = {
    {">>>=", LEX_ASSIGN_BASE + LEX_RSHIFTUNSIGNED, 4},
    {"===", LEX_TYPEEQUAL, 3},
    {"!==", LEX_NTYPEEQUAL, 3},
    {">>>", LEX_RSHIFTUNSIGNED, 3},
    {"<<=", LEX_ASSIGN_BASE + LEX_LSHIFT, 3},
    {">>=", LEX_ASSIGN_BASE + LEX_RSHIFT, 3},
    {"**=", LEX_ASSIGN_BASE + LEX_POWER, 3},
    {"==", LEX_EQUAL, 2},
    {"!=", LEX_NEQUAL, 2},
    {"<=", LEX_LEQUAL, 2},
    {">=", LEX_GEQUAL, 2},
    {"<<", LEX_LSHIFT, 2},
    {">>", LEX_RSHIFT, 2},
    {"**", LEX_POWER, 2},
    {"+=", LEX_ASSIGN_BASE + '+', 2},
    {"-=", LEX_ASSIGN_BASE + '-', 2},
    {"*=", LEX_ASSIGN_BASE + '*', 2},
    {"/=", LEX_ASSIGN_BASE + '/', 2},
    {"%=", LEX_ASSIGN_BASE + '%', 2},
    {"&=", LEX_ASSIGN_BASE + '&', 2},
    {"|=", LEX_ASSIGN_BASE + '|', 2},
    {"^=", LEX_ASSIGN_BASE + '^', 2},
    {"||", LEX_OROR, 2},
    {"&&", LEX_ANDAND, 2},
    {"++", LEX_PLUSPLUS, 2},
    {"--", LEX_MINUSMINUS, 2},
    {"<-", LEX_CONNECT, 2},
    {"->", LEX_SEND, 2},
    {"", LEX_EOF, 0}, //End record must have zero length
};

/**
 * Matches a Javascript operator token
 * @param code Pointer to the operator text
 * @return The appropriate token type for the operator
 */
CScriptToken CScriptToken::parseOperator(const char * code)const
{
    //First, try multi-char operators
    for (int i = 0; s_operators[i].len > 0; ++i)
    {
        if (strncmp(code, s_operators[i].text, s_operators[i].len) == 0)
            return buildNextToken((LEX_TYPES)s_operators[i].code, code, s_operators[i].len);
    }

    //Take it as a single char operator
    return buildNextToken((LEX_TYPES) * code, code, 1);
}

/**
 * Generates an error message located at the given position
 * @param code      Pointer to the code location where the error occurs. 
 * It is used to calculate line and column for the error message
 * @param msgFormat 'printf-like' format string
 * @param ...       Optional message parameters
 */
CScriptToken CScriptToken::errorAt(const char* code, const char* msgFormat, ...)const
{
    va_list aptr;

    va_start(aptr, msgFormat);
    ::errorAt_v(getPosition(), msgFormat, aptr);
    va_end(aptr);

    return *this;
}

/**
 * Calculates a line and column position, from a code pointer.
 * Uses the 'CScriptToken' information as a base from which to perform the 
 * calculations
 * @param codePos  Pointer to the text for which the position is going to be 
 * calculated. It MUST BE a pointer to the same text as 'm_code' pointer, and be
 * greater than it.
 */
ScriptPosition CScriptToken::calcPosition(const char* codePos)const
{
    ASSERT(codePos >= m_code);

    int line = m_position.line, col = m_position.column;
    const char* code;

    for (code = m_code; *code != 0 && code < codePos; ++code, ++col)
    {
        if (*code == '\n')
        {
            ++line;
            col = 0;
        }
    }

    ASSERT(codePos == code);

    ScriptPosition pos;
    pos.line = line;
    pos.column = col;

    return pos;
}
