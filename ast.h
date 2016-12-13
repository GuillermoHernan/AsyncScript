/* 
 * File:   ast.h
 * Author: ghernan
 * 
 * Abstract Syntax Tree classes
 *
 * Created on November 30, 2016, 7:56 PM
 */

#ifndef AST_H
#define	AST_H

#pragma once

#include "RefCountObj.h"
#include "jsLexer.h"
#include "JsVars.h"
#include <vector>
#include <map>

/**
 * AST node types enumeration
 */
enum AstNodeTypes
{
    AST_BLOCK
    ,AST_VAR
    ,AST_IF
    ,AST_FOR
    ,AST_RETURN
    ,AST_FUNCTION
    ,AST_ASSIGNMENT
    ,AST_FNCALL
    ,AST_LITERAL
    ,AST_IDENTIFIER
    ,AST_ARRAY
    ,AST_OBJECT
    ,AST_ARRAY_ACCESS
    ,AST_MEMBER_ACCESS
    ,AST_CONDITIONAL
    ,AST_BINARYOP
    ,AST_PREFIXOP
    ,AST_POSTFIXOP
    ,AST_TYPES_COUNT
};

class AstStatement;
typedef std::vector <Ref<AstStatement> > StatementList;

//AST conversion functions (mainly for AST /parser debugging)
Ref<JSArray> toJSArray (const StatementList& statements);
std::string toJSON (const StatementList& statements);

std::string astTypeToString(AstNodeTypes type);


/**
 * Base class for statements
 */
class AstStatement : public RefCountObj
{
public:
    const StatementList& children()const
    {
        return m_children;
    }
    
    bool childExists (size_t index)const
    {
        if (index < m_children.size())
            return m_children[index].notNull();
        else
            return false;
    }

    const ScriptPosition& position()const
    {
        return m_position;
    }
    
    virtual Ref<JSValue> toJSValue()const;
    
    virtual AstNodeTypes getType()const =0;

protected:
    StatementList m_children;
    ScriptPosition m_position;

    AstStatement(const ScriptPosition& pos) : m_position(pos)
    {
    }
};

/**
 * Base class for expressions.
 */
class AstExpression : public AstStatement
{
public:
    typedef std::vector< Ref<AstExpression> > ExprList;

    const ExprList& subExprs()const
    {
        return (const ExprList&) m_children;
    }

protected:

    AstExpression(const ScriptPosition& pos) : AstStatement(pos)
    {
    }

};

/**
 * Code block ast node.
 */
class AstBlock : public AstStatement
{
public:

    static Ref<AstBlock> create(ScriptPosition position)
    {
        return refFromNew(new AstBlock(position));
    }

    void add(Ref<AstStatement> child)
    {
        m_children.push_back(child);
    }
    
    virtual AstNodeTypes getType()const
    {
        return AST_BLOCK;
    }


protected:

    AstBlock(const ScriptPosition& pos) : AstStatement(pos)
    {
    }
};

/**
 * AST node for variable declarations
 */
class AstVar : public AstStatement
{
public:

    static Ref<AstVar> create(ScriptPosition position, const std::string& name, Ref<AstExpression> expr)
    {
        return refFromNew(new AstVar(position, name, expr));
    }
    
    virtual AstNodeTypes getType()const
    {
        return AST_VAR;
    }
    
    virtual Ref<JSValue> toJSValue()const;

    const std::string name;
protected:

    AstVar(const ScriptPosition& pos, const std::string& _name, Ref<AstExpression> expr)
    : AstStatement(pos), name(_name)
    {
        m_children.push_back(expr);
    }
};

/**
 * AST node for 'if' statements.
 */
class AstIf : public AstStatement
{
public:
    static Ref<AstIf> create(ScriptPosition position,
                             Ref<AstExpression> condition,
                             Ref<AstStatement> thenSt,
                             Ref<AstStatement> elseSt)
    {
        return refFromNew(new AstIf(position, condition, thenSt, elseSt));
    }
    
    virtual AstNodeTypes getType()const
    {
        return AST_IF;
    }


protected:
    AstIf (ScriptPosition position,
                             Ref<AstExpression> condition,
                             Ref<AstStatement> thenSt,
                             Ref<AstStatement> elseSt):
    AstStatement(position)
    {
        m_children.push_back(condition);
        m_children.push_back(thenSt);
        m_children.push_back(elseSt);
    }

};

/**
 * AST node for 'for' statements.
 */
class AstFor : public AstStatement
{
public:
    static Ref<AstFor> create(ScriptPosition position,
                              Ref<AstStatement> initSt,
                              Ref<AstExpression> condition,
                              Ref<AstStatement> incrementSt,
                              Ref<AstStatement> body)
    {
        return refFromNew (new AstFor(position, initSt, condition, incrementSt, body));
    }
    
    AstNodeTypes getType()const
    {
        return AST_FOR;
    }


protected:
    AstFor (ScriptPosition position,
                              Ref<AstStatement> initSt,
                              Ref<AstExpression> condition,
                              Ref<AstStatement> incrementSt,
                              Ref<AstStatement> body):
    AstStatement(position)
    {
        m_children.push_back(initSt);
        m_children.push_back(condition);
        m_children.push_back(incrementSt);
        m_children.push_back(body);
    }
        

};

/**
 * AST node for 'return' statements.
 */
class AstReturn : public AstStatement
{
public:
    static Ref<AstReturn> create(ScriptPosition position,
                                 Ref<AstExpression> expr)
    {
        return refFromNew (new AstReturn(position, expr));
    }
    
    virtual AstNodeTypes getType()const
    {
        return AST_RETURN;
    }

    
protected:
    AstReturn(ScriptPosition position, Ref<AstExpression> expr) : AstStatement(position)
    {
        m_children.push_back(expr);
    }
};

/**
 * AST node for function definitions
 */
class AstFunction : public AstExpression
{
public:
    static Ref<AstFunction> create(ScriptPosition position,
                                   const std::string& name)
    {
        return refFromNew (new AstFunction(position, name));
    }

    void setCode(Ref<AstStatement> code)
    {
        m_code = code;
    }

    Ref<AstStatement> getCode()const
    {
        return m_code;
    }
    
    const std::string& getName()const
    {
        return m_name;
    }

    void addParam(const std::string& paramName)
    {
        m_params.push_back(paramName);
    }

    typedef std::vector<std::string> Params;
    const Params& getParams()const
    {
        return m_params;
    }
    
    virtual AstNodeTypes getType()const
    {
        return AST_FUNCTION;
    }
    
    virtual Ref<JSValue> toJSValue()const;

protected:

    AstFunction(ScriptPosition position, const std::string& name) :
    AstExpression(position),
    m_name(name)
    {
    }

    const std::string m_name;
    Params m_params;
    Ref<AstStatement> m_code;
};

/**
 * Base class for all AST nodes which represent operators.
 */
class AstOperator : public AstExpression
{
public:
    const int code;
    
    virtual Ref<JSValue> toJSValue()const;
    
protected:
    AstOperator (ScriptPosition position, int opCode) : 
    AstExpression (position),
        code (opCode)
    {
    }
};

/**
 * AST node for assignment expressions
 */
class AstAssignment : public AstOperator
{
public:
    static Ref<AstAssignment> create(ScriptPosition position,
                                     int opCode,
                                     Ref<AstExpression> left,
                                     Ref<AstExpression> right)
    {
        return refFromNew (new AstAssignment(position, opCode, left, right));
    }
    
    virtual AstNodeTypes getType()const
    {
        return AST_ASSIGNMENT;
    }

protected:
    AstAssignment(ScriptPosition position,
                    int opCode,
                    Ref<AstExpression> left,
                    Ref<AstExpression> right):
    AstOperator (position, opCode)
    {
        m_children.push_back(left);
        m_children.push_back(right);
    }

};

/**
 * AST node for function call expressions
 * @param position
 * @param fnExpression
 * @return 
 */
class AstFunctionCall : public AstExpression
{
public:
    static Ref<AstFunctionCall> create(ScriptPosition position,
                                       Ref<AstExpression> fnExpression)
    {
        return refFromNew(new AstFunctionCall(position, fnExpression));
    }

    void addParam(Ref<AstExpression> paramExpression)
    {
        m_children.push_back(paramExpression);
    }
    
    void setNewFlag()
    {
        m_bNew = true;
    }
    
    bool getNewFlag()const
    {
        return m_bNew;
    }
    
    virtual AstNodeTypes getType()const
    {
        return AST_FNCALL;
    }

    virtual Ref<JSValue> toJSValue()const;
    
protected:
    AstFunctionCall(ScriptPosition position, Ref<AstExpression> fnExpression):
        AstExpression(position),
        m_bNew(false)
    {
        m_children.push_back(fnExpression);
    }
        
    bool    m_bNew;
};

/**
 * AST node for primitive types literals (Number, String, Boolean)
 */
class AstLiteral : public AstExpression
{
public:
    static Ref<AstLiteral> create(CScriptToken token);
    static Ref<AstLiteral> create(ScriptPosition pos, int value);
    static Ref<AstLiteral> undefined(ScriptPosition pos);
    
    const Ref<JSValue>  value;
    
    virtual AstNodeTypes getType()const
    {
        return AST_LITERAL;
    }

    virtual Ref<JSValue> toJSValue()const;
    
protected:
    AstLiteral (ScriptPosition position, Ref<JSValue> val) : 
    AstExpression(position),
        value (val)
    {        
    }
};

/**
 * AST node for identifiers (variable names, function names, members...)
 */
class AstIdentifier : public AstExpression
{
public:
    static Ref<AstIdentifier> create(CScriptToken token)
    {
        return refFromNew(new AstIdentifier(token));
    }
    
    const std::string   name;
    
    virtual AstNodeTypes getType()const
    {
        return AST_IDENTIFIER;
    }

    virtual Ref<JSValue> toJSValue()const;
    
protected:
    AstIdentifier (CScriptToken token) : 
    AstExpression(token.getPosition()),
        name (token.text())
    {        
    }
};

/**
 * AST node for array literals.
 */
class AstArray : public AstExpression
{
public:
    static Ref<AstArray> create(ScriptPosition pos)
    {
        return refFromNew (new AstArray(pos));
    }

    void addItem(Ref<AstExpression> expr)
    {
        m_children.push_back(expr);
    }
    
    virtual AstNodeTypes getType()const
    {
        return AST_ARRAY;
    }


protected:
    AstArray(ScriptPosition pos) : AstExpression(pos)
    {
    }
};

/**
 * AST node for object literals.
 */
class AstObject : public AstExpression
{
public:
    static Ref<AstObject> create(ScriptPosition pos)
    {
        return refFromNew (new AstObject(pos));
    }
    
    void addProperty (const std::string name, Ref<AstExpression> value)
    {
        m_properties[name] = value;
    }
    
    typedef std::map< std::string, Ref<AstExpression> >     PropsMap;
    
    const PropsMap & getProperties()const
    {
        return m_properties;
    }
    
    virtual AstNodeTypes getType()const
    {
        return AST_OBJECT;
    }
    
    virtual Ref<JSValue> toJSValue()const;

protected:
    AstObject(ScriptPosition pos) : AstExpression(pos)
    {
    }
    
    PropsMap m_properties;
};

/**
 * AST node for array access operator
 */
class AstArrayAccess : public AstExpression
{
public:
    static Ref<AstArrayAccess> create(ScriptPosition pos, Ref<AstExpression> array, Ref<AstExpression> index)
    {
        return refFromNew (new AstArrayAccess(pos, array, index));
    }
    
    virtual AstNodeTypes getType()const
    {
        return AST_ARRAY_ACCESS;
    }


protected:
    AstArrayAccess(ScriptPosition pos, Ref<AstExpression> array, Ref<AstExpression> index) :
    AstExpression(pos)
    {
        m_children.push_back(array);
        m_children.push_back(index);
    }
};

/**
 * AST node for member access operator.
 */
class AstMemberAccess : public AstExpression
{
public:
    static Ref<AstMemberAccess> create(ScriptPosition pos, Ref<AstExpression> obj, Ref<AstExpression> field)
    {
        return refFromNew (new AstMemberAccess(pos, obj, field));
    }
    
    virtual AstNodeTypes getType()const
    {
        return AST_MEMBER_ACCESS;
    }
    

protected:
    AstMemberAccess(ScriptPosition pos, Ref<AstExpression> obj, Ref<AstExpression> field) :
    AstExpression(pos)
    {
        m_children.push_back(obj);
        m_children.push_back(field);
    }
};

/**
 * AST node for conditional expressions.
 */
class AstConditional : public AstExpression
{
public:
    static Ref<AstConditional> create(ScriptPosition position,
                                      Ref<AstExpression> condition,
                                      Ref<AstExpression> thenExpr,
                                      Ref<AstExpression> elseExpr)
    {
        return refFromNew (new AstConditional(position, condition, thenExpr, elseExpr));
    }
    
    virtual AstNodeTypes getType()const
    {
        return AST_CONDITIONAL;
    }
    
    
protected:
    AstConditional(ScriptPosition position,
                                      Ref<AstExpression> condition,
                                      Ref<AstExpression> thenExpr,
                                      Ref<AstExpression> elseExpr):
    AstExpression(position)
    {        
        m_children.push_back(condition);
        m_children.push_back(thenExpr);
        m_children.push_back(elseExpr);
    }

};

/**
 * AST node for binary operations.
 */
class AstBinaryOp : public AstOperator
{
public:
    static Ref<AstBinaryOp> create(ScriptPosition position,
                                   int opType,
                                   Ref<AstExpression> left,
                                   Ref<AstExpression> right)
    {
        return refFromNew (new AstBinaryOp(position, opType, left, right));
    }
    
    virtual AstNodeTypes getType()const
    {
        return AST_BINARYOP;
    }
    

protected:
    AstBinaryOp(ScriptPosition position,
                int opType,
                Ref<AstExpression> left,
                Ref<AstExpression> right): AstOperator (position, opType)
    {
        m_children.push_back(left);
        m_children.push_back(right);
    }

};

/**
 * AST node for unary prefix operators.
 */
class AstPrefixOp : public AstOperator
{
public:
    static Ref<AstPrefixOp> create(ScriptPosition position,
                                   int opType,
                                   Ref<AstExpression> child)
    {
        return refFromNew (new AstPrefixOp(position, opType, child));
    }
    
    virtual AstNodeTypes getType()const
    {
        return AST_PREFIXOP;
    }


protected:
    AstPrefixOp(ScriptPosition position,
                int opType,
                Ref<AstExpression> child): AstOperator (position, opType)
    {
        m_children.push_back(child);
    }
};

/**
 * AST node for unary postfix operators.
 */
class AstPostfixOp : public AstOperator
{
public:
    static Ref<AstPostfixOp> create(ScriptPosition position,
                                    int opType,
                                    Ref<AstExpression> child)
    {
        return refFromNew (new AstPostfixOp(position, opType, child));
    }
    
    virtual AstNodeTypes getType()const
    {
        return AST_POSTFIXOP;
    }


protected:
    AstPostfixOp(ScriptPosition position,
                int opType,
                Ref<AstExpression> child): AstOperator (position, opType)
    {
        m_children.push_back(child);
    }
};

#endif	/* AST_H */

