/* 
 * File:   jsParser.cpp
 * Author: ghernan
 * 
 * Created on November 28, 2016, 10:36 PM
 */

#include "OS_support.h"
#include "jsParser.h"
#include "utils.h"
#include "JsVars.h"

using namespace std;

Ref<AstStatement>   emptyStatement(ScriptPosition pos);

ParseResult parseSimpleStatement (CScriptToken token);
ParseResult parseBodyStatement (CScriptToken token);
ParseResult parseBlock (CScriptToken token);
ParseResult parseVar (CScriptToken token);
ParseResult parseIf (CScriptToken token);
ParseResult parseWhile (CScriptToken token);
ParseResult parseFor (CScriptToken token);
ParseResult parseReturn (CScriptToken token);
CScriptToken parseFunctionArguments(Ref<AstFunction>(function), CScriptToken token);

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
ExprResult parseUnaryExpr (CScriptToken token);
ExprResult parsePostFixExpr (CScriptToken token);
ExprResult parseIdentifier (CScriptToken token);

ExprResult parseNewExpr (CScriptToken token);
ExprResult parseCallExpr (CScriptToken token);
ExprResult parseMemberExpr (CScriptToken token);
ExprResult parsePrimaryExpr (CScriptToken token);
ExprResult parseFunctionExpr (CScriptToken token);
ExprResult parseArrayLiteral (CScriptToken token);
ExprResult parseObjectLiteral (CScriptToken token);
ExprResult parseCallArguments (CScriptToken token, Ref<AstExpression> fnExpr);
ExprResult parseArrayAccess (CScriptToken token, Ref<AstExpression> arrayExpr);
ExprResult parseMemberAccess (CScriptToken token, Ref<AstExpression> objExpr);
ExprResult parseObjectProperty (CScriptToken token, Ref<AstExpression> objExpr);

ExprResult parseBinaryLROp (CScriptToken token, LEX_TYPES opType, ExprResult::ParseFunction childParser);
ExprResult parseBinaryLROp (CScriptToken token, const int *types, ExprResult::ParseFunction childParser);

    
bool isAssignment(CScriptToken token)
{
    const LEX_TYPES t = token.type();
    return t == '=' || (t > LEX_ASSIGN_BASE && t < LEX_ID);
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
Ref<AstStatement> emptyStatement(ScriptPosition pos)
{
    return AstLiteral::undefined(pos);
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
            return parseBlock(token);
    }
    case ';':           return ParseResult (token.next(), emptyStatement(token.getPosition()));
    case LEX_R_VAR:     return parseVar (token);
    case LEX_R_IF:      return parseIf (token);
    case LEX_R_WHILE:   return parseWhile (token);
    case LEX_R_FOR:     return parseFor (token);
    case LEX_R_RETURN:  return parseReturn (token);
    case LEX_R_FUNCTION:return parseFunctionExpr (token).toParseResult();
    
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
    case LEX_R_VAR:     return parseVar (token);
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
ParseResult parseBodyStatement (CScriptToken token)
{
    if (token.type() == ';')
        return ParseResult (token.next(), emptyStatement(token.getPosition()));
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
        
        return result;        
    }//else.
}


/**
 * Parses a code block
 * @param token
 * @return Code block object
 */
ParseResult parseBlock (CScriptToken token)
{
    //TODO: Better handling of ';'
    Ref<AstBlock>   block = AstBlock::create(token.getPosition());
    
    token = token.match('{');
    
    while (token.type() != '}')
    {
        ParseResult r = parseStatement(token);
        
        block->add (r.ast);
        token = r.nextToken;
    }
    
    token = token.match('}');
    
    return ParseResult (token, block);
}

/**
 * Parses a 'var' declaration
 * @param token
 * @return 
 */
ParseResult parseVar (CScriptToken token)
{
    ScriptPosition  pos = token.getPosition();
    token = token.match(LEX_R_VAR);
    
    const string name = token.text();
    
    token = token.match(LEX_ID);
    
    Ref<AstExpression> initExp;
    
    if (token.type() == '=')
    {
        //Initialization is optional.
        ExprResult r = parseExpression(token.next());
        r.throwIfError();
        
        initExp = r.result;
        token = r.token;
    }
    
    Ref<AstVar> var = AstVar::create (pos, name, initExp);
    
    return ParseResult (token, var);
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
    
    ParseResult r = parseBodyStatement(token.match(')'));
    Ref<AstStatement>   thenSt = r.ast;
    Ref<AstStatement>   elseSt;
    
    token = r.nextToken;
    
    if (token.type() == LEX_R_ELSE)
    {
        r = parseBodyStatement(token.next());
        elseSt = r.ast;
        token = r.nextToken;
    }
    
    Ref<AstIf> result = AstIf::create (pos, rCondition.result, thenSt, elseSt);
    
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
    
    ParseResult r = parseBodyStatement(token.match(')'));
    
    Ref<AstFor> result = AstFor::create (pos, 
                                         Ref<AstStatement>(),
                                         rCondition.result, 
                                         Ref<AstStatement>(),
                                         r.ast);
    
    return ParseResult (token, result);
}

/**
 * Parses a 'for' statement
 * @param token
 * @return 
 */
ParseResult parseFor (CScriptToken token)
{
    //TODO: Better handling of ';'
    ScriptPosition      pos = token.getPosition();
    Ref<AstStatement>   init;
    Ref<AstStatement>   increment;
    Ref<AstStatement>   body;
    Ref<AstExpression>  condition;

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
    r = parseBodyStatement(token);
    body = r.ast;
    token = r.nextToken;

    Ref<AstFor> result = AstFor::create (pos, 
                                         init,
                                         condition, 
                                         increment,
                                         body);
    
    return ParseResult (token, result);
}

/**
 * Parses a return statement
 * @param token
 * @return 
 */
ParseResult parseReturn (CScriptToken token)
{
    ScriptPosition      pos = token.getPosition();
    Ref<AstExpression>  result;

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
    
    Ref<AstReturn>  ret = AstReturn::create(pos, result);
    return ParseResult(token, ret);    
}

/**
 * Parses a function argument list.
 * @param function  Function into which add the arguments
 * @param token 
 * @return Next token
 */
CScriptToken parseFunctionArguments(Ref<AstFunction>(function), CScriptToken token)
{
    token = token.match('(');

    while (token.type() != ')')
    {
        const string name = token.text();

        token = token.match(LEX_ID);
        function->addParam(name);

        if (token.type() != ')')
            token = token.match(',');
    }
    return token.match(')');
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

    const Ref<AstExpression>    lexpr = r.result;
    const int                   op = r.token.type();
    const ScriptPosition        pos = r.token.getPosition();

    r = r.require(isAssignment).then (parseAssignment);
    if (r.ok())
    {
        Ref<AstAssignment> result = AstAssignment::create(pos, op, lexpr, r.result);
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
    
    return r.orElse (parseNewExpr).final();
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
        const Ref<AstExpression>  condition =r.result;
        
        r = r.then (parseAssignment).require(':');
        const Ref<AstExpression>  thenExpr =r.result;
        
        r = r.then (parseAssignment);
        const Ref<AstExpression>  elseExpr =r.result;
        
        if (r.ok())
            r.result = AstConditional::create (token.getPosition(),
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
    return parseBinaryLROp(token, operators, parseUnaryExpr);
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
            r.result = AstPrefixOp::create(token.getPosition(), token.type(), r.result);
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
        const int               op = r.token.type();
        
        r.result = AstPostfixOp::create(r.token.getPosition(), op, r.result);
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
 * Parses an operator 'new' expression
 * @param token
 * @return 
 */
ExprResult parseNewExpr (CScriptToken token)
{
    ExprResult     r = parseMemberExpr (token);
    
    if (r.ok())
        return r;
    else
    {
        ScriptPosition  newPos = r.token.getPosition();
        
        r = r.require(LEX_R_NEW).then(parseNewExpr);
        
        if (r.ok())
        {
            Ref<AstFunctionCall>    call = AstFunctionCall::create(newPos, r.result);
            call->setNewFlag();
            
            r.result = call;
        }

        return r.final();
    }
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
    if (token.type() == LEX_R_NEW)
    {
        ExprResult r(token);
        r = r.require(LEX_R_NEW).then(parseMemberExpr).then(parseCallArguments);
        
        if (!r.error())
            r.result.staticCast<AstFunctionCall>()->setNewFlag();
        return r.final();
    }
    else
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
    case LEX_R_UNDEFINED:
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

    token = r.token;

    //unnamed functions are legal.
    if (token.type() == LEX_ID)
    {
        name = token.text();
        token = token.next();
    }
    
    Ref<AstFunction>    function = AstFunction::create (pos, name);
    
    token = parseFunctionArguments(function, token);
    
    ParseResult rBlock = parseBlock(token);
    
    function->setCode (rBlock.ast);
    
    return ExprResult(rBlock.nextToken, function);    
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
    
    Ref<AstArray>   array = AstArray::create(token.getPosition());
    
    while (r.ok() && r.token.type() != ']')
    {
        //Skip empty entries
        while (r.token.type() == ',')
        {
            array->addItem( AstLiteral::undefined(r.token.getPosition()));
            r = r.require(',');
        }
        
        if (r.token.type() != ']')
        {
            r = r.then(parseAssignment);
            if (r.ok())
            {
                array->addItem(r.result);
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
ExprResult parseCallArguments (CScriptToken token, Ref<AstExpression> fnExpr)
{
    ExprResult      r(token);

    r = r.require('(');
    Ref<AstFunctionCall>    call = AstFunctionCall::create(token.getPosition(), fnExpr);
    
    while (r.ok() && r.token.type() != ')')
    {
        r = r.then(parseAssignment);
        
        if (r.ok())
        {
            call->addParam(r.result);
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
ExprResult parseArrayAccess (CScriptToken token, Ref<AstExpression> arrayExpr)
{
    ExprResult     r(token);
    
    r = r.require('[').then(parseExpression).require(']');
    
    if (r.ok())
        r.result = AstArrayAccess::create(token.getPosition(), arrayExpr, r.result);
    
    return r.final();
}

/**
 * Parses a member access expression ('.' operator)
 * @param token
 * @param arrayExpr
 * @return 
 */
ExprResult parseMemberAccess (CScriptToken token, Ref<AstExpression> arrayExpr)
{
    ExprResult     r(token);
    
    r = r.require('.').then(parseIdentifier);
    
    if (r.ok())
        r.result = AstMemberAccess::create(token.getPosition(), arrayExpr, r.result);
    
    return r.final();
}

/**
 * Parses a property of an object.
 * @param token
 * @param objExpr
 * @return 
 */
ExprResult parseObjectProperty (CScriptToken token, Ref<AstExpression> objExpr)
{
    string name;
    ExprResult  r(token);
    
    switch (token.type())
    {
    case LEX_INT:
    case LEX_FLOAT:
    case LEX_ID:
        name = token.text();
        break;
        
    case LEX_STR:
        name = token.strValue();
        break;
        
    default:
        return r.getError("Invalid object property name: %s", token.text().c_str()).final();
    }//switch
    
    r = r.skip().require(':').then(parseAssignment);
    
    if (r.ok())
    {
        objExpr.staticCast<AstObject>()->addProperty (name, r.result);
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
    //TODO: All binary operations can be parsed with a very similar code
    ExprResult      r = childParser (token);
    
    if (r.ok() && oneOf (r.token, ids))
    {
        const Ref<AstExpression>    left = r.result;
        const ScriptPosition        pos = r.token.getPosition();
        const int                   op = r.token.type();
        
        r = r.skip();
        if (r.error())
            return r.final();
        
        r = parseBinaryLROp(r.token, ids, childParser);
            
        if (r.ok())
            r.result = AstBinaryOp::create (pos, op, left, r.result);
        
        return r.final();
    }
    else    
        return r.final();
}
