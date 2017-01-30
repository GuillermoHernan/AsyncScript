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
#include "jsArray.h"

using namespace std;

//Empty children list constant
const AstNodeList AstNode::ms_noChildren;

//  Constructor functions.
//
////////////////////////////////
Ref<AstNode> astCreateScript(ScriptPosition pos)
{
    return refFromNew( new AstBranchNode(AST_SCRIPT, pos));    
}

Ref<AstNode> astCreateBlock(CScriptToken token)
{
    return refFromNew( new AstBranchNode(AST_BLOCK, token.getPosition()));    
}

Ref<AstNode> astCreateIf (ScriptPosition pos, 
                          Ref<AstNode> condition,
                          Ref<AstNode> thenSt,
                          Ref<AstNode> elseSt)
{
    
    auto result = refFromNew( new AstBranchNode(AST_IF, pos));
    
    result->addChild(condition);
    result->addChild(thenSt);
    result->addChild(elseSt);
    
    return result;
}

Ref<AstNode> astCreateConditional ( ScriptPosition pos, 
                                    Ref<AstNode> condition,
                                    Ref<AstNode> thenExpr,
                                    Ref<AstNode> elseExpr)
{    
    auto result = refFromNew( new AstBranchNode(AST_CONDITIONAL, pos));
    
    result->addChild(condition);
    result->addChild(thenExpr);
    result->addChild(elseExpr);

    return result;
}

Ref<AstNode> astCreateFor (ScriptPosition pos, 
                          Ref<AstNode> initSt,
                          Ref<AstNode> condition,
                          Ref<AstNode> incrementSt,
                          Ref<AstNode> body)
{    
    auto result = refFromNew( new AstBranchNode(AST_FOR, pos));
    
    result->addChild(initSt);
    result->addChild(condition);
    result->addChild(incrementSt);
    result->addChild(body);

    return result;
}

Ref<AstNode> astCreateForEach (ScriptPosition pos, 
                          Ref<AstNode> itemDeclaration,
                          Ref<AstNode> sequenceExpr,
                          Ref<AstNode> body)
{
    auto result = refFromNew( new AstBranchNode(AST_FOR_EACH, pos));
    
    result->addChild(itemDeclaration);
    result->addChild(sequenceExpr);
    result->addChild(body);

    return result;
}


Ref<AstNode> astCreateReturn (ScriptPosition pos, Ref<AstNode> expr)
{
    auto result = refFromNew( new AstBranchNode(AST_RETURN, pos));
    
    result->addChild(expr);
    return result;
}

Ref<AstNode> astCreateAssignment(ScriptPosition pos, 
                                 int opCode, 
                                 Ref<AstNode> lexpr, 
                                 Ref<AstNode> rexpr)
{
    auto result = refFromNew( new AstOperator(AST_ASSIGNMENT, pos, opCode));
    
    result->addChild(lexpr);
    result->addChild(rexpr);
    return result;
}

Ref<AstNode> astCreatePrefixOp(CScriptToken token, Ref<AstNode> rexpr)
{
    auto result = refFromNew( new AstOperator(AST_PREFIXOP, 
                                              token.getPosition(),
                                              token.type()));
    
    result->addChild(rexpr);
    return result;
}

Ref<AstNode> astCreatePostfixOp(CScriptToken token, Ref<AstNode> lexpr)
{
    auto result = refFromNew( new AstOperator(AST_POSTFIXOP, 
                                              token.getPosition(),
                                              token.type()));
    
    result->addChild(lexpr);
    return result;
}

Ref<AstNode> astCreateBinaryOp(CScriptToken token, 
                                 Ref<AstNode> lexpr, 
                                 Ref<AstNode> rexpr)
{
    auto result = refFromNew( new AstOperator(AST_BINARYOP, 
                                              token.getPosition(),
                                              token.type()));
    
    result->addChild(lexpr);
    result->addChild(rexpr);
    return result;
}

Ref<AstNode> astCreateFnCall(ScriptPosition pos, Ref<AstNode> fnExpr)
{
    auto result = refFromNew( new AstBranchNode(AST_FNCALL, pos));
    
    result->addChild(fnExpr);
    return result;
}

/**
 * Transforms a regular function call into a 'new' operator call.
 * @param callExpr
 * @return 
 */
//Ref<AstNode> astToNewCall(Ref<AstNode> callExpr)
//{
//    auto result = astCreateFnCall(callExpr->position(), 
//                                  callExpr->children()[0], true);
//    
//    auto children = callExpr->children();
//    
//    for (size_t i = 1; i < children.size(); ++i)
//        result->addChild(children[i]);
//    
//    return result;
//}

/**
 * Creates an array literal AST node.
 * @param pos
 * @return 
 */
Ref<AstNode> astCreateArray(ScriptPosition pos)
{
    return refFromNew( new AstBranchNode(AST_ARRAY, pos));
}

Ref<AstNode> astCreateArrayAccess(ScriptPosition pos,
                                  Ref<AstNode> arrayExpr, 
                                  Ref<AstNode> indexExpr)
{    
    auto result = refFromNew( new AstBranchNode(AST_ARRAY_ACCESS, pos));
    
    result->addChild(arrayExpr);
    result->addChild(indexExpr);

    return result;
}

Ref<AstNode> astCreateMemberAccess(ScriptPosition pos,
                                  Ref<AstNode> objExpr, 
                                  Ref<AstNode> identifier)
{    
    auto result = refFromNew( new AstBranchNode(AST_MEMBER_ACCESS, pos));
    
    result->addChild(objExpr);
    result->addChild(identifier);

    return result;
}

Ref<AstNode> astCreateVar (const ScriptPosition& pos, 
                           const std::string& name, 
                           Ref<AstNode> expr,
                           bool isConst)
{
    auto result = refFromNew (new AstNamedBranch(isConst ? AST_CONST : AST_VAR, pos, name));
    
    result->addChild(expr);
    return result;
}

Ref<AstNode> astCreateActor(ScriptPosition pos, const std::string& name)
{
    return refFromNew(new AstNamedBranch(AST_ACTOR, pos, name));
}

Ref<AstFunction> astCreateInputMessage(ScriptPosition pos, const std::string& name)
{
    return refFromNew(new AstFunction(AST_INPUT, pos, name));
}

Ref<AstFunction> astCreateOutputMessage(ScriptPosition pos, const std::string& name)
{
    return refFromNew(new AstFunction(AST_OUTPUT, pos, name));
}

Ref<AstNode> astCreateConnect(ScriptPosition pos,
                                 Ref<AstNode> lexpr, 
                                 Ref<AstNode> rexpr)
{
    auto result = refFromNew (new AstOperator(AST_CONNECT, pos, LEX_CONNECT));
    
    result->addChild(lexpr);
    result->addChild(rexpr);
    return result;
}

Ref<AstNode> astCreateSend(ScriptPosition pos,
                            Ref<AstNode> lexpr, 
                            Ref<AstNode> rexpr)
{
    auto result = refFromNew (new AstOperator(AST_BINARYOP, pos, LEX_SEND));
    
    result->addChild(lexpr);
    result->addChild(rexpr);
    return result;
}

Ref<AstNode> astCreateExtends (ScriptPosition pos,
                                const std::string& parentName)
{
    return refFromNew (new AstNamedBranch(AST_EXTENDS, pos, parentName));
}

/**
 * Gets the 'extends' node of a class node.
 * @param node
 * @return 
 */
Ref<AstNode> astGetExtends(Ref<AstNode> node)
{
    ASSERT (node->getType() == AST_CLASS);
    if (!node->childExists(0))
        return Ref<AstNode>();
    
    auto child = node->children().front();
    if (child->getType() != AST_EXTENDS)
        return Ref<AstNode>();
    else
        return child;    
}

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
 * Creates a literal from an integer
 * @param pos
 * @param value
 * @return 
 */
Ref<AstLiteral> AstLiteral::create(ScriptPosition pos, int value)
{
    return refFromNew(new AstLiteral(pos, jsInt(value)));
}

/**
 * Creates a 'null' literal
 * @param pos
 * @return 
 */
Ref<AstLiteral> AstLiteral::createNull(ScriptPosition pos)
{
    return refFromNew(new AstLiteral(pos, jsNull()));
}

/**
 * Gets the 'extends' node of a class. The 'extends' node contains inheritance 
 * information.
 * @return The node or a NULL pointer if not present.
 */
Ref<AstNamedBranch> AstClassNode::getExtendsNode()const
{
    if (this->childExists(0))
    {
        auto child = this->children().front();
        
        if (child->getType() == AST_EXTENDS)
            return child.staticCast<AstNamedBranch>();
    }
    
    return Ref<AstNamedBranch>();
}

/**
 * Transforms an AST statement into a Javascript object. 
 * This particular version creates an object containing all its children
 * @return 
 */
Ref<JSValue> AstNode::toJS()const
{
    Ref<JSObject>   obj = JSObject::create();
    
    obj->writeField("a_type", jsString(astTypeToString(getType())), false);

    const string name = getName();
    if (!name.empty())
        obj->writeField("b_name", jsString(name), false);
    
    const AstNodeList&  c = children();
    if (!c.empty())
        obj->writeField("z_children", toJSArray(c), false);

    const auto value = getValue();
    if (!value->isNull())
        obj->writeField("v_value", value, false);

    return obj;
}


/**
 * Function declaration to JSValue
 * @return 
 */
Ref<JSValue> AstFunction::toJS()const
{
    Ref<JSObject>   obj = AstNode::toJS().staticCast<JSObject>();
    
    obj->writeField("c_parameters", JSArray::createStrArray(m_params), false);
    if (m_code.notNull())
        obj->writeField("d_code", m_code->toJS(), false);
    
    return obj;
}

/**
 * Class node to javascript object
 * @return 
 */
Ref<JSValue> AstClassNode::toJS()const
{
    Ref<JSObject>   obj = AstNamedBranch::toJS().staticCast<JSObject>();
    
    obj->writeField("c_parameters", JSArray::createStrArray(m_params), false);
    
    return obj;
}

/**
 * Actor node to javascript object
 * @return 
 */
Ref<JSValue> AstActor::toJS()const
{
    Ref<JSObject>   obj = AstNamedBranch::toJS().staticCast<JSObject>();
    
    obj->writeField("c_parameters", JSArray::createStrArray(m_params), false);
    
    return obj;
}

/**
 * Operator to JSValue
 * @return 
 */
Ref<JSValue> AstOperator::toJS()const
{
    Ref<JSObject>   obj = AstBranchNode::toJS().staticCast<JSObject>();

    obj->writeField("d_operator", jsString(getTokenStr(code)), false);
    return obj;
}

/**
 * Object literal to JSValue
 * @return 
 */
Ref<JSValue> AstObject::toJS()const
{
    Ref<JSObject>   obj = JSObject::create();
    Ref<JSObject>   props = JSObject::create();
    
    obj->writeField("a_type", jsString(astTypeToString(getType())), false);
    obj->writeField("b_properties", props, false);
    
    PropertyList::const_iterator it;
    for (it = m_properties.begin(); it != m_properties.end(); ++it)
        props->writeField(it->name, it->expr->toJS(), false);
    
    return obj;
}

/**
 * Transforms a list of AstNodes into a Javascript Array.
 * @param statements
 * @return 
 */
Ref<JSArray> toJSArray (const AstNodeList& statements)
{
    Ref<JSArray>    result = JSArray::create();
    
    for (size_t i = 0; i < statements.size(); ++i)
    {
        if (statements[i].notNull())
            result->push( statements[i]->toJS() );
        else
            result->push(jsNull());
    }
    
    return result;
}

/**
 * Generates a JSON file from a statements list
 * @param statements
 * @return 
 */
//std::string toJSON (const AstNodeList& statements)
//{
//    return toJSArray(statements)->getJSON(0);
//}

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
        types[AST_SCRIPT] = "AST_SCRIPT";
        types[AST_BLOCK] = "AST_BLOCK";
        types[AST_VAR] = "AST_VAR";
        types[AST_CONST] = "AST_CONST";
        types[AST_IF] = "AST_IF";
        types[AST_FOR] = "AST_FOR";
        types[AST_FOR_EACH] = "AST_FOR_EACH";
        types[AST_RETURN] = "AST_RETURN";
        types[AST_FUNCTION] = "AST_FUNCTION";
        types[AST_ASSIGNMENT] = "AST_ASSIGNMENT";
        types[AST_FNCALL] = "AST_FNCALL";
//        types[AST_NEWCALL] = "AST_NEWCALL";
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
        types[AST_ACTOR] = "AST_ACTOR";
        types[AST_CONNECT] = "AST_CONNECT";
        types[AST_INPUT] = "AST_INPUT";
        types[AST_OUTPUT] = "AST_OUTPUT";
        types[AST_CLASS] = "AST_CLASS";
        types[AST_EXTENDS] = "AST_EXTENDS";
        //types[AST_TYPES_COUNT] = "AST_TYPES_COUNT";
    }
    
    TypesMap::const_iterator it = types.find(type);
    
    if (it != types.end())
        return it->second;
    else
        return "BAD_AST_TYPE";
}