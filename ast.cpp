/* 
 * File:   ast.cpp
 * Author: ghernan
 * 
 * Abstract Syntax Tree classes
 * 
 * Created on November 30, 2016, 7:56 PM
 */

#include "OS_support.h"
#include "ast.h"

/**
 * Creates an 'AstLiteral' object from a source token.
 * @param token
 * @return 
 */
Ref<AstLiteral> AstLiteral::create(CScriptToken token)
{
    Ref<JSValue>    value;
    
    switch (token.type())
    {
    case LEX_R_TRUE:        value = jsTrue(); break;
    case LEX_R_FALSE:       value = jsFalse(); break;
    case LEX_R_NULL:        value = jsNull(); break;
    case LEX_R_UNDEFINED:   value = ::undefined(); break;
    case LEX_STR:
    case LEX_INT:        
    case LEX_FLOAT:
        value = createConstant(token); 
        break;
        
    default:
        ASSERT(!"Invalid token for a literal");
    }
    
    return refFromNew(new AstLiteral(token.getPosition(), value));
}

/**
 * Creates a undefined literal
 * @param pos
 * @return 
 */
Ref<AstLiteral> AstLiteral::undefined(ScriptPosition pos)
{
    return refFromNew(new AstLiteral(pos, ::undefined()));
}
