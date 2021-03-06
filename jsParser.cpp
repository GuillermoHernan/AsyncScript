/* 
 * File:   jsParser.cpp
 * Author: ghernan
 * 
 * Created on November 28, 2016, 10:36 PM
 */

#include "ascript_pch.hpp"
#include "jsParser.h"
#include "utils.h"
#include "jsVars.h"
#include "ScriptException.h"

using namespace std;

Ref<AstNode>   emptyStatement(ScriptPosition pos);

ParseResult parseSimpleStatement (CScriptToken token);
ExprResult parseBodyStatement (CScriptToken token);
ExprResult parseBlock (CScriptToken token);
ExprResult  parseVar (CScriptToken token);
ParseResult parseIf (CScriptToken token);
ParseResult parseWhile (CScriptToken token);
ParseResult parseFor (CScriptToken token);
ExprResult parseForEach (CScriptToken token);
ParseResult parseReturn (CScriptToken token);
ExprResult parseArgumentList(CScriptToken token, Ref<AstNode> function);

ExprResult parseExpression (CScriptToken token);
ExprResult parseAssignment (CScriptToken token);
ExprResult parseLeftExpr (CScriptToken token);
ExprResult parseConditional (CScriptToken token);
ExprResult parseLogicalOrExpr (CScriptToken token);
ExprResult parseLogicalAndExpr (CScriptToken token);
ExprResult parseBitwiseOrExpr (CScriptToken token);
ExprResult parseBitwiseXorExpr (CScriptToken token);
ExprResult parseBitwiseAndExpr (CScriptToken token);
ExprResult parseEqualityExpr (CScriptToken token);
ExprResult parseRelationalExpr (CScriptToken token);
ExprResult parseShiftExpr (CScriptToken token);
ExprResult parseAddExpr (CScriptToken token);
ExprResult parseMultiplyExpr (CScriptToken token);
ExprResult parsePowerExpr (CScriptToken token);
ExprResult parseUnaryExpr (CScriptToken token);
ExprResult parsePostFixExpr (CScriptToken token);
ExprResult parseIdentifier (CScriptToken token);

//ExprResult parseNewExpr (CScriptToken token);
ExprResult parseCallExpr (CScriptToken token);
ExprResult parseMemberExpr (CScriptToken token);
ExprResult parsePrimaryExpr (CScriptToken token);
ExprResult parseFunctionExpr (CScriptToken token);
ExprResult parseArrayLiteral (CScriptToken token);
ExprResult parseObjectLiteral (CScriptToken token);
ExprResult parseCallArguments (CScriptToken token, Ref<AstNode> fnExpr);
ExprResult parseArrayAccess (CScriptToken token, Ref<AstNode> arrayExpr);
ExprResult parseMemberAccess (CScriptToken token, Ref<AstNode> objExpr);
ExprResult parseObjectProperty (CScriptToken token, Ref<AstNode> objExpr);

ExprResult parseBinaryLROp (CScriptToken token, LEX_TYPES opType, ExprResult::ParseFunction childParser);
ExprResult parseBinaryLROp (CScriptToken token, const int *types, ExprResult::ParseFunction childParser);
ExprResult parseBinaryRLOp (CScriptToken token, LEX_TYPES opType, ExprResult::ParseFunction childParser);
ExprResult parseBinaryRLOp (CScriptToken token, const int *types, ExprResult::ParseFunction childParser);

ExprResult parseActorExpr (CScriptToken token);
ExprResult parseActorMember (CScriptToken token);
ExprResult parseInputMessage (CScriptToken token);
ExprResult parseOutputMessage (CScriptToken token);
ExprResult parseConnectExpr (CScriptToken token);

ExprResult parseClassExpr (CScriptToken token);
ExprResult parseExtends (CScriptToken token);
ExprResult parseClassMember (CScriptToken token);

ExprResult parseExport (CScriptToken token);
ExprResult parseImport (CScriptToken token);

    
bool isAssignment(CScriptToken token)
{
    const LEX_TYPES t = token.type();
    return t == '=' || (t > LEX_ASSIGN_BASE && t < LEX_ASSIGN_MAX);
}

bool oneOf (CScriptToken token, const char* chars)
{
    const LEX_TYPES t = token.type();
    
    if ( t >= LEX_ASSIGN_BASE )
        return false;
    
    for (; *chars != 0; ++chars)
        if (*chars == (char)t)
            return true;
    
    return false;
}

bool oneOf (CScriptToken token, const int* ids)
{
    const LEX_TYPES t = token.type();
    
    for (; *ids != 0; ++ids)
        if (*ids == t)
            return true;
    
    return false;
}

/**
 * Crates an empty statement
 * @return 
 */
Ref<AstNode> emptyStatement(ScriptPosition pos)
{
    return AstLiteral::createNull(pos);
}

/**
 * Parses an script, which is a list of statements
 * @param token
 * @return 
 */
ParseResult parseScript(CScriptToken token)
{
    auto script = astCreateScript(token.getPosition());
    
    while (!token.eof())
    {
        const ParseResult   parseRes = parseStatement (token);

        script->addChild(parseRes.ast);
        token = parseRes.nextToken;
    }

    return ParseResult(token, script);
}

/**
 * Parses one statement
 * @param token
 * @return 'ParseResult' structure, with the generated AST tree, and a pointer to
 * the next token.
 */
ParseResult parseStatement (CScriptToken token)
{
    switch ((int)token.type())
    {
    case '{':
    {
        ExprResult  r = parseObjectLiteral(token);
        
        if (r.ok())
            return r.toParseResult();
        else
            return parseBlock(token).toParseResult();
    }
    case ';':           return ParseResult (token.next(), emptyStatement(token.getPosition()));
    case LEX_R_VAR:
    case LEX_R_CONST:
        return parseVar (token).toParseResult();
    case LEX_R_IF:      return parseIf (token);
    case LEX_R_WHILE:   return parseWhile (token);
    case LEX_R_FOR:     return parseFor (token);
    case LEX_R_RETURN:  return parseReturn (token);
    case LEX_R_FUNCTION:return parseFunctionExpr (token).toParseResult();
    case LEX_R_ACTOR:   return parseActorExpr (token).toParseResult();
    case LEX_R_CLASS:   return parseClassExpr (token).toParseResult();
    case LEX_R_EXPORT:  return parseExport (token).toParseResult();
    case LEX_R_IMPORT:  return parseImport (token).toParseResult();
    
    default:
        return parseSimpleStatement(token);
    }//switch
}

/**
 * Parses a simple statement (no blocks, no reserved words)
 * @param token
 * @return 
 */
ParseResult parseSimpleStatement (CScriptToken token)
{
    switch ((int)token.type())
    {
    case LEX_ID:
    case LEX_INT:
    case LEX_FLOAT:
    case LEX_STR:
    case '-':
    case '+':
    case '!':
    case '~':
    case LEX_PLUSPLUS:
    case LEX_MINUSMINUS:
    {
        ExprResult r = parseExpression (token);
        return r.toParseResult();
    }
    case LEX_R_VAR:
    case LEX_R_CONST:
        return parseVar (token).toParseResult();
    default:
        errorAt(token.getPosition(), "Unexpected token: '%s'", token.text().c_str());
        return ParseResult (token.next(), emptyStatement(token.getPosition()));
    }//switch
}

/**
 * Parses an statement which is part of the body of an 'if' or loop
 * @param token
 * @return 
 */
ExprResult parseBodyStatement (CScriptToken token)
{
    ExprResult  r(token);
    
    if (token.type() == ';')
    {
        r = r.skip();
        r.result = emptyStatement(token.getPosition());
    }
    else
    {
        ParseResult     result = parseStatement(token);
        
        switch ((int)token.type())
        {
        case '{':
        case LEX_R_IF:
        case LEX_R_WHILE:
        case LEX_R_FOR:
        case LEX_R_FUNCTION:
            break;
            
        default:
            //with all other statements, it may follow a semicolon.
            if (result.nextToken.type() == ';')
                result.nextToken = result.nextToken.next();
        }
        
        r.token = result.nextToken;
        r.result = result.ast;
    }//else.
    
    return r.final();
}


/**
 * Parses a code block
 * @param token
 * @return Code block object
 */
ExprResult parseBlock (CScriptToken token)
{
    //TODO: Better handling of ';'
    Ref<AstNode>    block = astCreateBlock(token);
    ExprResult      r(token);
    
    r = r.require('{');
    
    while (r.ok() && r.token.type() != '}')
    {
        //TODO: Migrate all parsing functions to 'ExprResult'
        ParseResult pr = parseStatement(r.token);
        
        block->addChild (pr.ast);
        r.token = pr.nextToken;
    }
    
    r = r.require('}');
    
    if (r.ok())
        r.result = block;
    
    return r.final();
}

/**
 * Parses a 'var' declaration
 * @param token
 * @return 
 */
ExprResult parseVar (CScriptToken token)
{
    ScriptPosition  pos = token.getPosition();
    ExprResult      r(token);
    bool            isConst = false;
    
    if (r.token.type() == LEX_R_CONST)
    {
        isConst = true;
        r = r.skip();
    }
    else
        r = r.require(LEX_R_VAR);
    
    if (!r.ok())
        return r.final();    
    
    const string name = r.token.text();
    
    r = r.require(LEX_ID);
    
    Ref<AstNode> initExp;
    
    if (r.ok() && r.token.type() == '=')
    {
        //Initialization is optional.
        r = r.skip().then(parseExpression);
        
        initExp = r.result;
    }
    
    if (r.ok())
        r.result = astCreateVar (pos, name, initExp, isConst);
    
    return r;
}

/**
 * Parses an 'if' statement
 * @param token
 * @return 
 */
ParseResult parseIf (CScriptToken token)
{
    ScriptPosition  pos = token.getPosition();

    token = token.match(LEX_R_IF);
    token = token.match('(');
    
    ExprResult rCondition = parseExpression(token);
    rCondition.throwIfError();

    token = rCondition.token;
    
    ParseResult r = parseBodyStatement(token.match(')')).toParseResult();
    Ref<AstNode>   thenSt = r.ast;
    Ref<AstNode>   elseSt;
    
    token = r.nextToken;
    
    if (token.type() == LEX_R_ELSE)
    {
        r = parseBodyStatement(token.next()).toParseResult();
        elseSt = r.ast;
        token = r.nextToken;
    }
    
    auto result = astCreateIf (pos, rCondition.result, thenSt, elseSt);
    
    return ParseResult (token, result);
}

/**
 * Parses a 'while' statement
 * @param token
 * @return 
 */
ParseResult parseWhile (CScriptToken token)
{
    ScriptPosition  pos = token.getPosition();

    token = token.match(LEX_R_WHILE);
    token = token.match('(');
    
    ExprResult rCondition = parseExpression(token);
    rCondition.throwIfError();

    token = rCondition.token;
    
    ParseResult r = parseBodyStatement(token.match(')')).toParseResult();
    
    auto result = astCreateFor (pos, 
                                Ref<AstNode>(), 
                                rCondition.result, 
                                Ref<AstNode>(), 
                                r.ast);
    
    return ParseResult (r.nextToken, result);
}

/**
 * Parses a 'for' statement
 * @param token
 * @return 
 */
ParseResult parseFor (CScriptToken token)
{
    ExprResult  res = parseForEach(token);
    
    if (res.ok())
        return res.toParseResult();
    
    //TODO: Better handling of ';'
    ScriptPosition      pos = token.getPosition();
    Ref<AstNode>   init;
    Ref<AstNode>   increment;
    Ref<AstNode>   body;
    Ref<AstNode>  condition;

    token = token.match(LEX_R_FOR);
    token = token.match('(');
    
    ParseResult r;
    
    //Initialization
    if (token.type() != ';')
    {
        r = parseSimpleStatement(token);
        init = r.ast;
        token = r.nextToken.match(';');
    }
    else
        token = token.next();
    
    ExprResult rCondition = parseExpression(token);
    rCondition.throwIfError();
    condition = rCondition.result;
    token = rCondition.token.match(';');
    
    if (token.type() != ')')
    {
        r = parseSimpleStatement(token);
        increment = r.ast;
        token = r.nextToken;        
    }
    
    token = token.match(')');
    r = parseBodyStatement(token).toParseResult();
    body = r.ast;
    token = r.nextToken;

    auto result = astCreateFor (pos, init, condition, increment, body);
    
    return ParseResult (token, result);
}

/**
 * Parses a for loop which iterates over the elements of a sequence. 
 * 'for (x in z)...'
 * @param token
 * @return 
 */
ExprResult parseForEach (CScriptToken token)
{
    ExprResult  r(token);
    
    r = r.require(LEX_R_FOR).require('(');
    if (r.error())
        return r.final();

    r = r.then(parseIdentifier).requireId("in");
    if (r.error())
        return r.final();
    
    auto itemDecl = r.result;
    r = r.then(parseExpression).require(')');
    
    if (r.error())
        return r.final();
    
    auto seqExpression = r.result;
    
    r = r.then (parseBodyStatement);
    
    if (r.ok())
        r.result = astCreateForEach (token.getPosition(), itemDecl, seqExpression, r.result);
    
    return r.final();
}


/**
 * Parses a return statement
 * @param token
 * @return 
 */
ParseResult parseReturn (CScriptToken token)
{
    ScriptPosition      pos = token.getPosition();
    Ref<AstNode>  result;

    token = token.match(LEX_R_RETURN);
    
    if (token.type() == ';')
        token = token.next();
    else
    {
        ExprResult r = parseExpression(token);
        r.throwIfError();
        
        token = r.token;
        result = r.result;
        if (token.type() == ';')
            token = token.next();
    }
    
    auto  ret = astCreateReturn(pos, result);
    return ParseResult(token, ret);    
}

/**
 * Parses a function argument list.
 * @param token 
 * @param fnNode  Function into which add the arguments
 * @return  ExprResult containing the same function AST node received, with
 * the parameters added.
 */
ExprResult parseArgumentList(CScriptToken token, Ref<AstNode> fnNode)
{
    ExprResult          r(token);
    
    r = r.require('(');

    while (r.ok() && r.token.type() != ')')
    {
        const string name = r.token.text();

        r = r.require (LEX_ID);
        if (r.ok())
            fnNode->addParam(name);

        if (r.token.type() != ')')
            r = r.require (',');
    }
    
    r = r.require(')');
    
    if (r.ok())
        r.result = fnNode;
    
    return r.final();
}

/**
 * Parses a Javascript expression
 * @param token     Startup token
 * @param exception Controls if the function shall throw an exception on an error.
 * @return 
 */
ExprResult parseExpression (CScriptToken token)
{
    //TODO: Support comma operator (¡qué pereza!)
    return parseAssignment (token);
}

/**
 * Parses an assignment expression.
 * @param token
 * @param exception
 * @return 
 */
ExprResult parseAssignment (CScriptToken token)
{
    ExprResult r = parseLeftExpr(token);

    const Ref<AstNode>    lexpr = r.result;
    const int                   op = r.token.type();
    const ScriptPosition        pos = r.token.getPosition();

    r = r.require(isAssignment).then (parseAssignment);
    if (r.ok())
    {
        auto result = astCreateAssignment(pos, op, lexpr, r.result);
        r.result = result;
        return r.final();
    }
    else
        return parseConditional(token);
}

/**
 * Parses an expression which can be at the left side of an assignment
 * @param token
 * @param exception
 * @return 
 */
ExprResult parseLeftExpr (CScriptToken token)
{
    ExprResult     r = parseCallExpr (token);
    
    return r.orElse (parseMemberExpr).final();
}

/**
 * Parses a conditional expression ('?' operator)
 * @param token
 * @return 
 */
ExprResult parseConditional (CScriptToken token)
{
    ExprResult      r = parseLogicalOrExpr (token);
    
    if (r.ok() && r.token.type() == '?')
    {
        const Ref<AstNode>  condition =r.result;
        
        r = r.skip();
        r = r.then (parseAssignment).require(':');
        const Ref<AstNode>  thenExpr =r.result;
        
        r = r.then (parseAssignment);
        const Ref<AstNode>  elseExpr =r.result;
        
        if (r.ok())
            r.result = astCreateConditional (token.getPosition(),
                                               condition,
                                               thenExpr,
                                               elseExpr);
        
    }
    
    return r.final();
}

/**
 * Parses a 'logical OR' expression ('||' operator)
 * @param token
 * @return 
 */
ExprResult parseLogicalOrExpr (CScriptToken token)
{
    return parseBinaryLROp(token, LEX_OROR, parseLogicalAndExpr);
}

/**
 * Parses a 'logical AND' expression ('&&' operator)
 * @param token
 * @return 
 */
ExprResult parseLogicalAndExpr (CScriptToken token)
{
    return parseBinaryLROp(token, LEX_ANDAND, parseBitwiseOrExpr);
}

/**
 * Parses a 'bitwise OR' expression ('|' operator)
 * @param token
 * @return 
 */
ExprResult parseBitwiseOrExpr(CScriptToken token)
{
    return parseBinaryLROp(token, LEX_TYPES('|'), parseBitwiseXorExpr);
}

/**
 * Parses a 'bitwise XOR' expression ('^' operator)
 * @param token
 * @return 
 */
ExprResult parseBitwiseXorExpr(CScriptToken token)
{
    return parseBinaryLROp(token, LEX_TYPES('^'), parseBitwiseAndExpr);
}

/**
 * Parses a 'bitwise XOR' expression ('&' operator)
 * @param token
 * @return 
 */
ExprResult parseBitwiseAndExpr(CScriptToken token)
{
    return parseBinaryLROp(token, LEX_TYPES('&'), parseEqualityExpr);
}

/**
 * Parses a equality expression (see operators in code)
 * @param token
 * @return 
 */
ExprResult parseEqualityExpr(CScriptToken token)
{
    const int operators[] = {LEX_EQUAL, LEX_TYPEEQUAL, LEX_NEQUAL, LEX_NTYPEEQUAL, 0 };
    return parseBinaryLROp(token, operators, parseRelationalExpr);
}

/**
 * Parses a relational expression (<, >, <=, >=, instanceof, in) operators
 * @param token
 * @return 
 */
ExprResult parseRelationalExpr(CScriptToken token)
{
    //TODO: Missing 'instanceof' and 'in' operators
    const int operators[] = {'<', '>', LEX_LEQUAL, LEX_GEQUAL,  0 };
    return parseBinaryLROp(token, operators, parseShiftExpr);
}

/**
 * Parses a bit shift expression.
 * @param token
 * @return 
 */
ExprResult parseShiftExpr(CScriptToken token)
{
    const int operators[] = {LEX_LSHIFT, LEX_RSHIFT, LEX_RSHIFTUNSIGNED,  0 };
    return parseBinaryLROp(token, operators, parseAddExpr);
}

/**
 * Parses an add or substract expression
 * @param token
 * @return 
 */
ExprResult parseAddExpr (CScriptToken token)
{
    const int operators[] = {'+', '-', 0 };
    return parseBinaryLROp(token, operators, parseMultiplyExpr);
}

/**
 * Parses a multiply, divide or modulus expression.
 * @param token
 * @return 
 */
ExprResult parseMultiplyExpr (CScriptToken token)
{
    const int operators[] = {'*', '/', '%', 0 };
    return parseBinaryLROp(token, operators, parsePowerExpr);
}

/**
 * Parses a exponenciation operation (operator '**')
 * @param token
 * @return 
 */
ExprResult parsePowerExpr(CScriptToken token)
{
    return parseBinaryRLOp(token, LEX_POWER, parseUnaryExpr);
}

/**
 * Parses an unary expression
 * @param token
 * @return 
 */
ExprResult parseUnaryExpr (CScriptToken token)
{
    //TODO: Missing: delete, void & typeof
    const int operators[] = {'+', '-', '~', '!', LEX_PLUSPLUS, LEX_MINUSMINUS , 0};
    
    if (oneOf (token, operators))
    {
        ExprResult r = parseUnaryExpr(token.next());
        
        if (r.ok())
        {
            r.result = astCreatePrefixOp(token, r.result);
            return r.final();
        }
        else
            return ExprResult(token, r.errorDesc);
    }
    else
        return parsePostFixExpr(token);
}

/**
 * Parses a postfix expression
 * @param token
 * @return 
 */
ExprResult parsePostFixExpr (CScriptToken token)
{
    const int operators[] = {LEX_PLUSPLUS, LEX_MINUSMINUS , 0};
    ExprResult r = parseLeftExpr(token);
    
    if (r.ok() && oneOf(r.token, operators))
    {
        r.result = astCreatePostfixOp(r.token, r.result);
        r = r.skip();
    }
    
    return r.final();
}

/**
 * Parses an identifier
 * @param token
 * @return 
 */
ExprResult parseIdentifier (CScriptToken token)
{
    ExprResult  r = ExprResult(token).require(LEX_ID);
    
    if (r.ok())
        r.result = AstIdentifier::create(token);
    
    return r.final();
}

/**
 * Parses a function call expression
 * @param token
 * @return 
 */
ExprResult parseCallExpr (CScriptToken token)
{
    ExprResult r = ExprResult(token).then(parseMemberExpr).then(parseCallArguments);
        
    while (r.ok() && oneOf (r.token, "([."))
    {
        const LEX_TYPES t = r.token.type();
        
        if (t == '(')
            r = r.then(parseCallArguments);
        else if (t == '[')
            r = r.then(parseArrayAccess);
        else
        {
            ASSERT (t == '.');
            r = r.then (parseMemberAccess);
        }
    }//while
    
    return r.final();
}

/**
 * Parses a member access expression
 * @note The name is taken from the production of the standard Javascript grammar.
 * But in my opinion, the production has too broad scope to be called just 'member access'
 * @param token
 * @return 
 */
ExprResult parseMemberExpr (CScriptToken token)
{
    ExprResult r = parsePrimaryExpr(token).orElse(parseFunctionExpr);

    while (!r.error() && oneOf (r.token, "[."))
    {
        const LEX_TYPES t = r.token.type();

        if (t == '[')
            r = r.then(parseArrayAccess);
        else
        {
            ASSERT (t == '.');
            r = r.then (parseMemberAccess);
        }
    }//while

    return r.final();
}

/**
 * Parses a primary expression.  A primary expression is an identifier name,
 * a constant, an array literal, an object literal or an expression between parenthesis
 * @param token
 * @return 
 */
ExprResult parsePrimaryExpr (CScriptToken token)
{
    switch ((int)token.type())
    {
    case LEX_R_TRUE:    //valueNode = AstLiteral::create(jsTrue()); break;
    case LEX_R_FALSE:   //valueNode = AstLiteral::create(jsFalse()); break;
    case LEX_R_NULL:
    case LEX_FLOAT:
    case LEX_INT:
    case LEX_STR:       //valueNode = AstLiteral::create(createConstant(token)); break;
        return ExprResult (token.next(), AstLiteral::create(token));
        
    case LEX_ID:        return parseIdentifier(token);
    case '[':           return parseArrayLiteral(token);
    case '{':           return parseObjectLiteral(token);
    case '(':
        return ExprResult(token)
                .require('(')
                .then(parseExpression)
                .require(')').final();
        
    default:
        return ExprResult(token)
                .getError("Unexpected token: '%s'", token.text().c_str());
    }//switch
}

/**
 * Parses a function expression
 * @param token
 * @return 
 */
ExprResult parseFunctionExpr (CScriptToken token)
{
    ScriptPosition  pos = token.getPosition();
    string          name;
    ExprResult      r(token);

    r = r.require(LEX_R_FUNCTION);
    if (r.error())
        return r.final();

    //unnamed functions are legal.
    if (r.token.type() == LEX_ID)
    {
        name = r.token.text();
        r = r.skip();
    }
    
    Ref<AstFunction>    function = AstFunction::create (pos, name);
    r.result = function;
    
    r = r.then(parseArgumentList).then(parseBlock);

    if (r.ok())
    {
        function->setCode (r.result);
        r.result = function;
    }
    
    return r.final();
}

/**
 * Parses an array literal.
 * @param token
 * @return 
 */
ExprResult parseArrayLiteral (CScriptToken token)
{
    ExprResult     r(token);
    
    r = r.require('[');
    
    auto    array = astCreateArray(token.getPosition());
    
    while (r.ok() && r.token.type() != ']')
    {
        //Skip empty entries
        while (r.token.type() == ',')
        {
            array->addChild( AstLiteral::createNull(r.token.getPosition()));
            r = r.require(',');
        }
        
        if (r.token.type() != ']')
        {
            r = r.then(parseAssignment);
            if (r.ok())
            {
                array->addChild(r.result);
                if (r.token.type() != ']')
                    r = r.require(',');
            }
        }
    }//while
    
    r = r.require(']');
    if (r.ok())
        r.result = array;
    
    return r.final();
}

/**
 * Parses an object literal (JSON)
 * @param token
 * @return 
 */
ExprResult parseObjectLiteral (CScriptToken token)
{
    ExprResult     r(token);
    
    r = r.require('{');
    
    Ref<AstObject>   object = AstObject::create(token.getPosition());
    r.result = object;
    
    while (r.ok() && r.token.type() != '}')
    {
        r = r.then(parseObjectProperty);
        if (r.token.type() != '}')
            r = r.require(',');
    }//while
    
    r = r.require('}');
    
    return r.final();
}

/**
 * Parses function call arguments.
 * @note This function creates the function call AST node.
 * @param token     next token
 * @param fnExpr    Expression from which the function reference is obtained
 * @return 
 */
ExprResult parseCallArguments (CScriptToken token, Ref<AstNode> fnExpr)
{
    ExprResult      r(token);

    r = r.require('(');
    auto    call = astCreateFnCall(token.getPosition(), fnExpr/*, false*/);
    
    while (r.ok() && r.token.type() != ')')
    {
        r = r.then(parseAssignment);
        
        if (r.ok())
        {
            call->addChild(r.result);
            if (r.token.type() != ')')
            {
                r = r.require(',');
                if (r.token.type() == ')')
                    r = r.getError("Empty parameter");
            }
        }
    }//while
    
    r = r.require(')');
    if (r.ok())
        r.result = call;
    
    return r.final();
}

/**
 * Parses an array access expression ('[expression]')
 * @param token
 * @param arrayExpr
 * @return 
 */
ExprResult parseArrayAccess (CScriptToken token, Ref<AstNode> arrayExpr)
{
    ExprResult     r(token);
    
    r = r.require('[').then(parseExpression).require(']');
    
    if (r.ok())
        r.result = astCreateArrayAccess(token.getPosition(), arrayExpr, r.result);
    
    return r.final();
}

/**
 * Parses a member access expression ('.' operator)
 * @param token
 * @param arrayExpr
 * @return 
 */
ExprResult parseMemberAccess (CScriptToken token, Ref<AstNode> arrayExpr)
{
    ExprResult     r(token);
    
    r = r.require('.').then(parseIdentifier);
    
    if (r.ok())
        r.result = astCreateMemberAccess(token.getPosition(), arrayExpr, r.result);
    
    return r.final();
}

/**
 * Parses a property of an object.
 * @param token
 * @param objExpr
 * @return 
 */
ExprResult parseObjectProperty (CScriptToken token, Ref<AstNode> objExpr)
{
    string      name;
    ExprResult  r(token);
    bool        isConst = false;
    
    if (token.type() == LEX_R_CONST)
    {
        isConst = true;
        r = r.skip();
    }
    
    switch (r.token.type())
    {
    case LEX_INT:
    case LEX_FLOAT:
    case LEX_ID:
        name = r.token.text();
        break;
        
    case LEX_STR:
        name = r.token.strValue();
        break;
        
    default:
        return r.getError("Invalid object property name: %s", r.token.text().c_str()).final();
    }//switch
    
    r = r.skip().require(':').then(parseAssignment);
    
    if (r.ok())
    {
        objExpr.staticCast<AstObject>()->addProperty (name, r.result, isConst);
        r.result = objExpr;
    }
    
    return r.final();
}



/**
 * Parses a binary operator which associates from left to right.
 * @param token     startup Token
 * @param opType    Token identifier (from lexer) of the operator
 * @param childParser   Parser for the child expressions.
 * @return 
 */
ExprResult parseBinaryLROp (CScriptToken token, LEX_TYPES opType, ExprResult::ParseFunction childParser)
{
    const int ids[] = {opType, 0};
    
    return parseBinaryLROp (token, ids, childParser);
}

/**
 * Parses a binary operator which associates from left to right.
 * @param token         startup Token
 * @param ids           array of operator ids. The last element must be zero.
 * @param childParser   Parser for the child expressions.
 * @return 
 */
ExprResult parseBinaryLROp (CScriptToken token, const int* ids, ExprResult::ParseFunction childParser)
{
    ExprResult      r = childParser (token);
    
    while (r.ok() && oneOf (r.token, ids))
    {
        const Ref<AstNode>  left = r.result;
        CScriptToken        opToken = r.token;
        
        r = r.skip();
        if (r.error())
            return r.final();
        
        r = r.then(childParser);
            
        if (r.ok())
            r.result = astCreateBinaryOp (opToken, left, r.result);
    }

    return r.final();
}

/**
 * Parses a binary operator which associates from right to left.
 * @param token     startup Token
 * @param opType    Token identifier (from lexer) of the operator
 * @param childParser   Parser for the child expressions.
 * @return 
 */
ExprResult parseBinaryRLOp (CScriptToken token, LEX_TYPES opType, ExprResult::ParseFunction childParser)
{
    const int ids[] = {opType, 0};
    
    return parseBinaryRLOp (token, ids, childParser);
}

/**
 * Parses a binary operator which associates from right to left.
 * @param token         startup Token
 * @param ids           array of operator ids. The last element must be zero.
 * @param childParser   Parser for the child expressions.
 * @return 
 */
ExprResult parseBinaryRLOp (CScriptToken token, const int* ids, ExprResult::ParseFunction childParser)
{
    ExprResult      r = childParser (token);
    
    if (r.ok() && oneOf (r.token, ids))
    {
        const Ref<AstNode>  left = r.result;
        CScriptToken        opToken = r.token;
        
        r = r.skip();
        if (r.error())
            return r.final();
        
        r = parseBinaryLROp(r.token, ids, childParser);
            
        if (r.ok())
            r.result = astCreateBinaryOp (opToken, left, r.result);
        
        return r.final();
    }
    else    
        return r.final();
}

/**
 * Parses an actor definition.
 * @param token
 * @return 
 */
ExprResult parseActorExpr (CScriptToken token)
{
    ScriptPosition  pos = token.getPosition();
    ExprResult      r(token);
    Ref<AstActor>   actorNode;

    r = r.require(LEX_R_ACTOR);
    if (r.error())
        return r.final();

    const string name = r.token.text();
    
    r = r.require(LEX_ID);
    
    if (r.ok())
    {
        actorNode = AstActor::create(pos, name);
        r.result = actorNode;
        r = r.then(parseArgumentList).require('{');

        while (r.ok() && r.token.type() != '}')
        {
            //Skip ';', which may (optionally) act as separators.
            while (r.token.type() == ';')
                r = r.skip();
            
            if (r.token.type() == '}')
                break;
            
            r = parseActorMember(r.token);
            if (r.ok())
                actorNode->addChild(r.result);
        }
        
        r = r.require('}');
    }//if
    
    if (r.ok())
        r.result = actorNode;
    
    return r.final();
}

/**
 * Parses one of the possible members of an actor.
 * @param token
 * @return 
 */
ExprResult parseActorMember (CScriptToken token)
{
    switch (token.type())
    {
    case LEX_R_VAR:
    case LEX_R_CONST:
        return parseVar(token);
    case LEX_R_INPUT:   return parseInputMessage(token);
    case LEX_R_OUTPUT:  return parseOutputMessage(token);
    default:            return parseConnectExpr(token);
    }
}

/**
 * Parses an input message definition
 * @param token
 * @return 
 */
ExprResult parseInputMessage (CScriptToken token)
{
    ScriptPosition      pos = token.getPosition();
    ExprResult          r(token);
    Ref<AstFunction>    function;

    r = r.require(LEX_R_INPUT);
    if (r.error())
        return r.final();

    const string name = r.token.text();
    r = r.require(LEX_ID);

    if (r.ok())
    {
        function = astCreateInputMessage (pos, name);
        r.result = function;
    }
    
    r = r.then(parseArgumentList).then(parseBlock);
    
    if (r.ok())
    {
        function->setCode(r.result);
        r.result = function;
    }
    
    return r.final();
}

/**
 * Parses an output message declaration
 * @param token
 * @return 
 */
ExprResult parseOutputMessage (CScriptToken token)
{
    ScriptPosition      pos = token.getPosition();
    ExprResult          r(token);

    r = r.require(LEX_R_OUTPUT);
    if (r.error())
        return r.final();

    const string name = r.token.text();
    r = r.require(LEX_ID);

    if (r.error())
        return r.final();
    
    Ref<AstFunction>    function = astCreateOutputMessage(pos, name);
    r.result = function;
    
    r = r.then(parseArgumentList);
    
    if (r.ok())
        r.result = function;
    
    return r.final();
}

/**
 * Parses message connection operator ('<-')
 * @param token
 * @return 
 */
ExprResult parseConnectExpr (CScriptToken token)
{
    ExprResult  r(token);
    
    r = r.then(parseIdentifier);
    auto lexpr = r.result;
    
    ScriptPosition  pos = r.token.getPosition();
    r = r.require(LEX_CONNECT).then(parseLeftExpr);
    
    if (r.ok())
        r.result = astCreateConnect (pos, lexpr, r.result);    
    
    return r.final();
}


/**
 * Parses a class definition.
 * @param token
 * @return 
 */
ExprResult parseClassExpr (CScriptToken token)
{
    ScriptPosition      pos = token.getPosition();
    ExprResult          r(token);
    Ref<AstClassNode>   classNode;

    r = r.require(LEX_R_CLASS);
    if (r.error())
        return r.final();

    const string name = r.token.text();
    
    r = r.require(LEX_ID);
    
    if (r.ok())
    {
        classNode = AstClassNode::create(pos, name);
        r.result = classNode;
        
        if (r.token.type() == '(')
            r = r.then(parseArgumentList);
        
        if (r.ok() && r.token.type() == LEX_ID && r.token.text() == "extends")
        {
            r = r.then(parseExtends);
            if (r.ok())
                classNode->addChild(r.result);
        }
        
        r = r.require('{');

        while (r.ok() && r.token.type() != '}')
        {
            //Skip ';', which may (optionally) act as separators.
            while (r.token.type() == ';')
                r = r.skip();
            
            if (r.token.type() == '}')
                break;
            
            r = parseClassMember(r.token);
            if (r.ok())
                classNode->addChild(r.result);
        }
        
        r = r.require('}');
    }//if
    
    if (r.ok())
        r.result = classNode;
    
    return r.final();
}

/**
 * Parses a 'extends' clause, used to define class inheritance.
 * @param token
 * @return 
 */
ExprResult parseExtends (CScriptToken token)
{
    ExprResult  r(token);

    r = r.requireId("extends");
    if (r.error())
        return r.final();
    
    const string        parentName = r.token.text();
    r = r.require(LEX_ID);
    
    if (r.ok())
    {
        auto    extNode = astCreateExtends(token.getPosition(), parentName);
        
        if (r.token.type() == '(')
        {
            r.result = extNode;
            r = r.then(parseCallArguments);
            if (r.ok())
                extNode->addChild (r.result);
        }
        
        if (r.ok())
            r.result = extNode;    
    }
    
    
    return r.final();
}

/**
 * Parses a class member.
 * @param token
 * @return 
 */
ExprResult parseClassMember (CScriptToken token)
{
    switch (token.type())
    {
    case LEX_R_VAR:
    case LEX_R_CONST:
        return parseVar(token);

    default:
        return parseFunctionExpr(token);
    }
}

/**
 * Parses an 'export' keyword.
 * @param token
 * @return 
 */
ExprResult parseExport (CScriptToken token)
{
    ExprResult  r(token);

    r = r.require(LEX_R_EXPORT);
    if (r.error())
        return r.final();
    
    switch ((int)r.token.type())
    {
    case LEX_R_VAR:
    case LEX_R_CONST:
        r = r.then (parseVar); break;

    case LEX_R_FUNCTION:    r = r.then (parseFunctionExpr); break;
    case LEX_R_ACTOR:       r = r.then (parseActorExpr); break;
    case LEX_R_CLASS:       r = r.then (parseClassExpr); break;
    
    default:
        r = r.getError("Unexpected token after 'export': '%s'", token.text().c_str());;
        break;
    }//switch
    
    if (r.ok())
        r.result = astCreateExport(token.getPosition(), r.result);
    
    return r.final();    
}

/**
 * Parses an import statement
 * @param token
 * @return 
 */
ExprResult parseImport (CScriptToken token)
{
    ExprResult  r(token);

    r = r.require(LEX_R_IMPORT);
    if (r.error())
        return r.final();
    
    auto paramToken = r.token;
    r = r.require(LEX_STR);
    
    if (r.ok())
        r.result = astCreateImport (token.getPosition(), AstLiteral::create(paramToken));
    
    return r.final();
}
