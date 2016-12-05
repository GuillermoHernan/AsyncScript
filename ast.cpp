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

using namespace std;

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
 * Creates a literal form an integer
 * @param pos
 * @param value
 * @return 
 */
Ref<AstLiteral> AstLiteral::create(ScriptPosition pos, int value)
{
    return refFromNew(new AstLiteral(pos, jsInt(value)));
}

/**
 * Transforms an AST statement into a Javascript object. 
 * This particular version creates an object containing all its children
 * @return 
 */
Ref<JSValue> AstStatement::toJSValue()const
{
    Ref<JSObject>   obj = JSObject::create();
    
    obj->set("a_type", jsString(astTypeToString(getType())));
    obj->set("z_children", toJSArray(m_children));
    
    return obj;
}

/**
 * Var declaration to JSValue
 * @return 
 */
Ref<JSValue> AstVar::toJSValue()const
{
    Ref<JSObject>   obj = AstStatement::toJSValue().staticCast<JSObject>();
    
    obj->set("b_name", jsString(name));
    
    return obj;
}

/**
 * Function declaration to JSValue
 * @return 
 */
Ref<JSValue> AstFunction::toJSValue()const
{
    Ref<JSObject>   obj = AstExpression::toJSValue().staticCast<JSObject>();
    
    obj->set("b_name", jsString(m_name));
    obj->set("c_parameters", JSArray::createStrArray(m_params));
    obj->set("d_code", m_code->toJSValue());
    
    return obj;
}

/**
 * Operator to JSValue
 * @return 
 */
Ref<JSValue> AstOperator::toJSValue()const
{
    Ref<JSObject>   obj = AstExpression::toJSValue().staticCast<JSObject>();

    obj->set("d_operator", jsString(getTokenStr(code)));
    return obj;
}

/**
 * Function call to JSValue
 * @return 
 */
Ref<JSValue> AstFunctionCall::toJSValue()const
{
    Ref<JSObject>   obj = AstExpression::toJSValue().staticCast<JSObject>();

    obj->set("d_isNew", jsBool(m_bNew));
    return obj;
}

/**
 * Literal to JSValue
 * @return 
 */
Ref<JSValue> AstLiteral::toJSValue()const
{
    Ref<JSObject>   obj = JSObject::create();
    
    obj->set("a_type", jsString(astTypeToString(getType())));
    obj->set("v_value", value);
    return obj;
}

/**
 * Identifier to JSValue
 * @return 
 */
Ref<JSValue> AstIdentifier::toJSValue()const
{
    Ref<JSObject>   obj = JSObject::create();
    
    obj->set("a_type", jsString(astTypeToString(getType())));
    obj->set("b_name", jsString(name));
    return obj;
}

/**
 * Object literal to JSValue
 * @return 
 */
Ref<JSValue> AstObject::toJSValue()const
{
    Ref<JSObject>   obj = JSObject::create();
    Ref<JSObject>   props = JSObject::create();
    
    obj->set("a_type", jsString(astTypeToString(getType())));
    obj->set("b_properties", props);
    
    PropsMap::const_iterator it;
    for (it = m_properties.begin(); it != m_properties.end(); ++it)
        props->set(it->first, it->second->toJSValue());
    
    return obj;
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

/**
 * Transforms a list of AstStatements into a Javascript Array.
 * @param statements
 * @return 
 */
Ref<JSArray> toJSArray (const StatementList& statements)
{
    Ref<JSArray>    result = JSArray::create();
    
    for (size_t i = 0; i < statements.size(); ++i)
        result->push( statements[i]->toJSValue() );
    
    return result;
}

/**
 * Generates a JSON file from a statements list
 * @param statements
 * @return 
 */
std::string toJSON (const StatementList& statements)
{
    return toJSArray(statements)->getJSON(0);
}

/**
 * Gets the string representation of an AST type
 * @param type
 * @return 
 */
std::string astTypeToString(AstNodeTypes type)
{
    typedef map<AstNodeTypes, string>   TypesMap;
    static TypesMap types;
    
    if (types.empty())
    {
        types[AST_BLOCK] = "AST_BLOCK";
        types[AST_VAR] = "AST_VAR";
        types[AST_IF] = "AST_IF";
        types[AST_FOR] = "AST_FOR";
        types[AST_RETURN] = "AST_RETURN";
        types[AST_FUNCTION] = "AST_FUNCTION";
        types[AST_ASSIGNMENT] = "AST_ASSIGNMENT";
        types[AST_FNCALL] = "AST_FNCALL";
        types[AST_LITERAL] = "AST_LITERAL";
        types[AST_IDENTIFIER] = "AST_IDENTIFIER";
        types[AST_ARRAY] = "AST_ARRAY";
        types[AST_OBJECT] = "AST_OBJECT";
        types[AST_ARRAY_ACCESS] = "AST_ARRAY_ACCESS";
        types[AST_MEMBER_ACCESS] = "AST_MEMBER_ACCESS";
        types[AST_CONDITIONAL] = "AST_CONDITIONAL";
        types[AST_BINARYOP] = "AST_BINARYOP";
        types[AST_PREFIXOP] = "AST_PREFIXOP";
        types[AST_POSTFIXOP] = "AST_POSTFIXOP";
        //types[AST_TYPES_COUNT] = "AST_TYPES_COUNT";
    }
    
    TypesMap::const_iterator it = types.find(type);
    
    if (it != types.end())
        return it->second;
    else
        return "BAD_AST_TYPE";
}