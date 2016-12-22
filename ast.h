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
#include "jsVars.h"
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
    ,AST_NEWCALL
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
    ,AST_ACTOR
    ,AST_CONNECT
    ,AST_INPUT
    ,AST_OUTPUT
    ,AST_TYPES_COUNT
};

class AstNode;
class AstFunction;
typedef std::vector <Ref<AstNode> > AstNodeList;

//AST conversion functions (mainly for AST /parser debugging)
Ref<JSArray> toJSArray (const AstNodeList& statements);
std::string toJSON (const AstNodeList& statements);

std::string astTypeToString(AstNodeTypes type);

//Constructor functions
Ref<AstNode> astCreateBlock(CScriptToken token);
Ref<AstNode> astCreateIf (ScriptPosition pos, 
                          Ref<AstNode> condition,
                          Ref<AstNode> thenSt,
                          Ref<AstNode> elseSt);
Ref<AstNode> astCreateConditional ( ScriptPosition pos, 
                                    Ref<AstNode> condition,
                                    Ref<AstNode> thenExpr,
                                    Ref<AstNode> elseExpr);
Ref<AstNode> astCreateFor (ScriptPosition pos, 
                          Ref<AstNode> initSt,
                          Ref<AstNode> condition,
                          Ref<AstNode> incrementSt,
                          Ref<AstNode> body);
Ref<AstNode> astCreateReturn (ScriptPosition pos, Ref<AstNode> expr);
Ref<AstNode> astCreateAssignment(ScriptPosition pos, 
                                 int opCode, 
                                 Ref<AstNode> lexpr, 
                                 Ref<AstNode> rexpr);
Ref<AstNode> astCreatePrefixOp(CScriptToken token, Ref<AstNode> rexpr);
Ref<AstNode> astCreatePostfixOp(CScriptToken token, Ref<AstNode> lexpr);
Ref<AstNode> astCreateBinaryOp(CScriptToken token, 
                                 Ref<AstNode> lexpr, 
                                 Ref<AstNode> rexpr);
Ref<AstNode> astCreateFnCall(ScriptPosition pos, Ref<AstNode> fnExpr, bool newCall);
Ref<AstNode> astToNewCall(Ref<AstNode> callExpr);
Ref<AstNode> astCreateArray(ScriptPosition pos);
Ref<AstNode> astCreateArrayAccess(ScriptPosition pos,
                                  Ref<AstNode> arrayExpr, 
                                  Ref<AstNode> indexExpr);
Ref<AstNode> astCreateMemberAccess(ScriptPosition pos,
                                  Ref<AstNode> objExpr, 
                                  Ref<AstNode> identifier);
Ref<AstNode> astCreateVar (const ScriptPosition& pos, 
                              const std::string& name, 
                              Ref<AstNode> expr);
Ref<AstFunction> astCreateInputMessage(ScriptPosition pos, const std::string& name);
Ref<AstFunction> astCreateOutputMessage(ScriptPosition pos, const std::string& name);
Ref<AstNode> astCreateConnect(ScriptPosition pos,
                                 Ref<AstNode> lexpr, 
                                 Ref<AstNode> rexpr);
Ref<AstNode> astCreateSend (ScriptPosition pos,
                                 Ref<AstNode> lexpr, 
                                 Ref<AstNode> rexpr);


/**
 * Base class for AST nodes
 */
class AstNode : public RefCountObj
{
public:

    virtual const AstNodeList& children()const
    {
        return ms_noChildren;
    }
    
    virtual const std::string getName()const
    {
        return "";
    }
    
    virtual Ref<JSValue> getValue()const
    {
        return undefined();
    }
    
    virtual void addChild(Ref<AstNode> child)
    {
        ASSERT(!"addChildren unsupported");
    }
    
    virtual void addParam(const std::string& paramName)
    {
        ASSERT(!"addParam unsupported");
    }


    bool childExists(size_t index)const
    {
        const AstNodeList&  c = children();
        
        if (index < c.size())
            return c[index].notNull();
        else
            return false;
    }

    const ScriptPosition& position()const
    {
        return m_position;
    }

    virtual Ref<JSValue> toJS()const;

    AstNodeTypes getType()const
    {
        return m_type;
    }

protected:
    static const AstNodeList    ms_noChildren;
    
    const ScriptPosition m_position;
    const AstNodeTypes m_type;

    AstNode(AstNodeTypes type, const ScriptPosition& pos) :
    m_position(pos), m_type(type)
    {
    }

    virtual ~AstNode()
    {
    }
};

/**
 * Base class for AST nodes which contain children nodes.
 */
class AstBranchNode : public AstNode
{
public:

    virtual const AstNodeList& children()const
    {
        return m_children;
    }
    
    virtual void addChild(Ref<AstNode> child)
    {
        m_children.push_back(child);
    }
    
    AstBranchNode(AstNodeTypes type, const ScriptPosition& pos) : AstNode(type, pos)
    {
    }
protected:
    
    AstNodeList     m_children;
    
};

/**
 * Class for branch nodes which are also named
 */
class AstNamedBranch : public AstBranchNode
{
public:

    virtual const std::string getName()const
    {
        return m_name;
    }

    AstNamedBranch(AstNodeTypes type, const ScriptPosition& pos, const std::string& _name)
    : AstBranchNode(type, pos), m_name(_name)
    {
    }
    
protected:

    const std::string m_name;
};

/**
 * AST node for function definitions
 */
class AstFunction : public AstNode
{
public:
    static Ref<AstFunction> create(ScriptPosition position,
                                   const std::string& name)
    {
        return refFromNew (new AstFunction(AST_FUNCTION, position, name));
    }

    void setCode(Ref<AstNode> code)
    {
        m_code = code;
    }

    Ref<AstNode> getCode()const
    {
        return m_code;
    }
    
    virtual const std::string getName()const
    {
        return m_name;
    }

    virtual void addParam(const std::string& paramName)
    {
        m_params.push_back(paramName);
    }

    typedef std::vector<std::string> Params;
    const Params& getParams()const
    {
        return m_params;
    }
    
    virtual Ref<JSValue> toJS()const;

    AstFunction(AstNodeTypes type, ScriptPosition position, const std::string& name) :
    AstNode(type, position),
    m_name(name)
    {
    }
    
protected:

    const std::string   m_name;
    Params              m_params;
    Ref<AstNode>        m_code;
};

/**
 * AST node for actor definitions
 */
class AstActor : public AstNamedBranch
{
public:
    static Ref<AstActor> create(ScriptPosition position,
                                   const std::string& name)
    {
        return refFromNew (new AstActor(position, name));
    }

    virtual void addParam(const std::string& paramName)
    {
        m_params.push_back(paramName);
    }

    typedef std::vector<std::string> Params;
    const Params& getParams()const
    {
        return m_params;
    }
    
    virtual Ref<JSValue> toJS()const;

protected:
    AstActor(ScriptPosition position, const std::string& name) :
    AstNamedBranch(AST_ACTOR, position, name)
    {
    }
    
    Params              m_params;
};


/**
 * Base class for all AST nodes which represent operators.
 */
class AstOperator : public AstBranchNode
{
public:
    const int code;
    
    virtual Ref<JSValue> toJS()const;
    
    AstOperator (AstNodeTypes type, ScriptPosition position, int opCode) : 
    AstBranchNode (type, position), code (opCode)
    {
    }
};

/**
 * AST node for primitive types literals (Number, String, Boolean)
 */
class AstLiteral : public AstNode
{
public:
    static Ref<AstLiteral> create(CScriptToken token);
    static Ref<AstLiteral> create(ScriptPosition pos, int value);
    static Ref<AstLiteral> undefined(ScriptPosition pos);
    
    virtual Ref<JSValue> getValue()const
    {
        return m_value;
    }
    
protected:
    AstLiteral (ScriptPosition position, Ref<JSValue> val) : 
    AstNode(AST_LITERAL, position),
        m_value (val)
    {        
    }

    const Ref<JSValue>  m_value;
};

/**
 * AST node for identifiers (variable names, function names, members...)
 */
class AstIdentifier : public AstNode
{
public:
    static Ref<AstIdentifier> create(CScriptToken token)
    {
        return refFromNew(new AstIdentifier(token));
    }
    
    virtual const std::string getName()const
    {
        return m_name;
    }

protected:
    AstIdentifier (CScriptToken token) : 
    AstNode(AST_IDENTIFIER, token.getPosition()),
        m_name (token.text())
    {        
    }

    const std::string   m_name;
};

/**
 * AST node for object literals.
 */
class AstObject : public AstNode
{
public:
    /**
     * Object property structure
     */
    struct Property
    {
        std::string     name;
        Ref<AstNode>    expr;
    };
    typedef std::vector<Property>   PropertyList;
    
    static Ref<AstObject> create(ScriptPosition pos)
    {
        return refFromNew (new AstObject(pos));
    }
    
    void addProperty (const std::string name, Ref<AstNode> expr)
    {
        Property prop;
        prop.name = name;
        prop.expr = expr;
        
        m_properties.push_back(prop);
    }
    
    const PropertyList & getProperties()const
    {
        return m_properties;
    }
    
    virtual Ref<JSValue> toJS()const;

protected:
    AstObject(ScriptPosition pos) : AstNode(AST_OBJECT, pos)
    {
    }
    
    PropertyList m_properties;
};

#endif	/* AST_H */

