/* 
 * File:   mvmCodegen.cpp
 * Author: ghernan
 * 
 * Code generator for micro VM,
 * 
 * Created on December 2, 2016, 9:59 PM
 */

#include "OS_support.h"
#include "mvmCodegen.h"
#include "asVars.h"

#include <set>
#include <map>
#include <string>

using namespace std;

/**
 * Code generation scope
 */
class CodegenScope 
{
public:
    void declare (const std::string& name)
    {
        m_symbols.insert(name);
    }

    bool isDeclared (const std::string& name)const
    {
        return m_symbols.count(name) > 0;
    }
    
    const Ref<AstNode>          ownerNode;
    
    CodegenScope (const Ref<AstNode> _ownerNode) : ownerNode(_ownerNode)
    {
    }
    
private:
    std::set < std::string>     m_symbols;
};

/**
 * Value comparer function class, to be able to use 'JSValue' objects as map keys.
 */
struct JsValueLessComp
{
    bool operator ()(Ref<JSValue> a, Ref<JSValue> b)const
    {
        return jsValuesCompare(a, b) < 0;
    }
};

typedef map<Ref<JSValue>, int, JsValueLessComp> ConstantsMap;

/**
 * State of a codegen operation
 */
struct CodegenState
{
    Ref<MvmRoutine>         curRoutine;
    Ref<AsActorClass>       curActor;
    vector<CodegenScope>    scopes;
    ConstantsMap            constants;
};

//Forward declarations

typedef void (*CodegenFN)(Ref<AstNode> statement, CodegenState* pState);

void codegen (Ref<AstNode> statement, CodegenState* pState);
int  childrenCodegen (Ref<AstNode> statement, CodegenState* pState);
bool childCodegen (Ref<AstNode> statement, int index, CodegenState* pState);

void invalidNodeCodegen (Ref<AstNode> node, CodegenState* pState);
void blockCodegen (Ref<AstNode> statement, CodegenState* pState);
void varCodegen (Ref<AstNode> statement, CodegenState* pState);
void ifCodegen (Ref<AstNode> statement, CodegenState* pState);
void forCodegen (Ref<AstNode> statement, CodegenState* pState);
void returnCodegen (Ref<AstNode> statement, CodegenState* pState);
void functionCodegen (Ref<AstNode> statement, CodegenState* pState);
void assignmentCodegen (Ref<AstNode> statement, CodegenState* pState);
void fncallCodegen (Ref<AstNode> statement, CodegenState* pState);
void thisCallCodegen (Ref<AstNode> statement, CodegenState* pState);
void constructorCodegen (Ref<AstNode> statement, CodegenState* pState);
void literalCodegen (Ref<AstNode> statement, CodegenState* pState);
void identifierCodegen (Ref<AstNode> statement, CodegenState* pState);
void arrayCodegen (Ref<AstNode> statement, CodegenState* pState);
void objectCodegen (Ref<AstNode> statement, CodegenState* pState);
void arrayAccessCodegen (Ref<AstNode> statement, CodegenState* pState);
void memberAccessCodegen (Ref<AstNode> statement, CodegenState* pState);
void conditionalCodegen (Ref<AstNode> statement, CodegenState* pState);
void binaryOpCodegen (Ref<AstNode> statement, CodegenState* pState);
void prefixOpCodegen (Ref<AstNode> statement, CodegenState* pState);
void postfixOpCodegen (Ref<AstNode> statement, CodegenState* pState);
void logicalOpCodegen (const int opCode, Ref<AstNode> statement, CodegenState* pState);

void actorCodegen (Ref<AstNode> node, CodegenState* pState);
void connectCodegen (Ref<AstNode> node, CodegenState* pState);
void messageCodegen (Ref<AstNode> node, CodegenState* pState);

void pushConstant (Ref<JSValue> value, CodegenState* pState);
void pushConstant (const char* str, CodegenState* pState);
void pushConstant (const std::string& str, CodegenState* pState);
void pushConstant (int value, CodegenState* pState);
void pushConstant (bool value, CodegenState* pState);
void pushUndefined (CodegenState* pState);

void callCodegen (const std::string& fnName, int nParams, CodegenState* pState);
void callInstruction (int nParams, CodegenState* pState);

void instruction8 (int opCode, CodegenState* pState);
void instruction16 (int opCode, CodegenState* pState);
int  getLastInstruction (CodegenState* pState);
int  removeLastInstruction (CodegenState* pState);
void binaryOperatorCode (int tokenCode, CodegenState* pState);

void endBlock (int trueJump, int falseJump, CodegenState* pState);
void setTrueJump (int blockId, int destinationId, CodegenState* pState);
void setFalseJump (int blockId, int destinationId, CodegenState* pState);
int  curBlockId (CodegenState* pState);

CodegenState initFunctionState (Ref<AstNode> node);

/**
 * Generates MVM code for a script.
 * @param statements    The script is received as a sequence (vector) of compiled statements.
 * @return 
 */
Ref<MvmRoutine> scriptCodegen (Ref<AstNode> script)
{
    CodegenState    state;
    
    ASSERT (script->getType() == AST_SCRIPT);
    
    state.curRoutine = MvmRoutine::create(ScriptPosition(1,1));
    state.scopes.push_back(CodegenScope(script));
    
    auto statements = script->children();
    
    for (size_t i = 0; i < statements.size(); ++i)
    {
        //Remove previous result
        if (i > 0)
            instruction8 (OC_POP, &state);
        codegen (statements[i], &state);
    }
    
    return state.curRoutine;
}


void codegen (Ref<AstNode> statement, CodegenState* pState)
{
    static CodegenFN types[AST_TYPES_COUNT] = {NULL, NULL};
    
    if (types [0] == NULL)
    {
        types [AST_SCRIPT] = invalidNodeCodegen;
        types [AST_BLOCK] = blockCodegen;
        types [AST_VAR] = varCodegen;
        types [AST_CONST] = varCodegen;
        types [AST_IF] = ifCodegen;
        types [AST_FOR] = forCodegen;
        types [AST_RETURN] = returnCodegen;
        types [AST_FUNCTION] = functionCodegen;
        types [AST_ASSIGNMENT] = assignmentCodegen;
        types [AST_FNCALL] = fncallCodegen;
        types [AST_NEWCALL] = fncallCodegen;
        types [AST_LITERAL] = literalCodegen;
        types [AST_IDENTIFIER] = identifierCodegen;
        types [AST_ARRAY] = arrayCodegen;
        types [AST_OBJECT] = objectCodegen;
        types [AST_ARRAY_ACCESS] = arrayAccessCodegen;
        types [AST_MEMBER_ACCESS] = memberAccessCodegen;
        types [AST_CONDITIONAL] = conditionalCodegen;
        types [AST_BINARYOP] = binaryOpCodegen;
        types [AST_PREFIXOP] = prefixOpCodegen;
        types [AST_POSTFIXOP] = postfixOpCodegen;
        types [AST_ACTOR] = actorCodegen;
        types [AST_CONNECT] = connectCodegen;
        types [AST_INPUT] = messageCodegen;
        types [AST_OUTPUT] = messageCodegen;
    }
    
    types[statement->getType()](statement, pState);
}

/**
 * Generates code for the children of a given statement
 * @param statement
 * @param pState
 */
int childrenCodegen (Ref<AstNode> statement, CodegenState* pState)
{
    const AstNodeList&  children = statement->children();
    int count = 0;
    
    for (size_t i=0; i < children.size(); ++i)
    {
        if (children[i].notNull())
        {
            codegen (children[i], pState);
            ++count;
        }
    }
    
    return count;
}

/**
 * Generates code for a child node of an AST node.
 * @param statement
 * @param index
 * @param pState
 * @return 
 */
bool childCodegen (Ref<AstNode> statement, int index, CodegenState* pState)
{
    const AstNodeList&  children = statement->children();
    
    if (index >= (int)children.size() || index < 0)
        return false;
    else if (children[index].notNull())
    {
        codegen (children[index], pState);
        return true;
    }
    else
        return false;
}

/**
 * Called if an invalid node is found in the AST
 * @param node
 * @param pState
 */
void invalidNodeCodegen(Ref<AstNode> node, CodegenState* pState)
{
    auto typeString = astTypeToString(node->getType());
    errorAt (node->position(), "Invalid AST node found: ", typeString.c_str());
}

/**
 * Generates the code for a block of statements.
 * @param statement
 * @param pState
 */
void blockCodegen (Ref<AstNode> statement, CodegenState* pState)
{
    const AstNodeList&  children = statement->children();
    
    //It does not create a code generation scope, because it just needs to know
    //if a variable is local or global. Therefore, code generation scopes are
    //only created at function-level.
    instruction8 (OC_PUSH_SCOPE, pState);
    
    for (size_t i=0; i < children.size(); ++i)
    {
        if (children[i].notNull())
        {
            codegen (children[i], pState);
            instruction8(OC_POP, pState);   //Discard result
        }
    }

    instruction8 (OC_POP_SCOPE, pState);
        
    //Non expression stataments leave an 'undefined' on the stack.
    pushUndefined(pState);
}

/**
 * Generates code for a 'var' declaration
 * @param statement
 * @param pState
 */
void varCodegen (Ref<AstNode> node, CodegenState* pState)
{
    const string name = node->getName();
    const bool inActor = pState->scopes.back().ownerNode->getType() == AST_ACTOR;
    const bool isConst = node->getType() == AST_CONST;

    if (!inActor)
    {
        //Not inside an actor
        pState->scopes.back().declare(name);

        pushConstant (name, pState);
        if (!childCodegen(node, 0, pState))
            pushUndefined(pState);
        instruction8 (isConst ? OC_NEW_CONST : OC_NEW_VAR, pState);

        //Non-expression statements leave an 'undefined' on the stack.
        pushUndefined(pState);
    }
    else
    {
//        pState->curActor->writeFieldStr(name, jsNull());
        
        pushConstant ("this", pState);
        instruction8 (OC_RD_LOCAL, pState);
        pushConstant (name, pState);
        if (!childCodegen(node, 0, pState))
            pushConstant (jsNull(), pState);
        instruction8 (isConst ? OC_NEW_CONST_FIELD : OC_WR_FIELD, pState);
    }
}

/**
 * Generates code for an 'if' statement
 * @param statement
 * @param pState
 */
void ifCodegen (Ref<AstNode> statement, CodegenState* pState)
{
    const int conditionBlock = curBlockId(pState)+1;
    const bool conditional = statement->getType() == AST_CONDITIONAL;
    
    //Generate code for condition
    endBlock (conditionBlock, conditionBlock, pState);
    childCodegen(statement, 0, pState);
    
    const int thenInitialBlock = curBlockId(pState)+1;
    endBlock (thenInitialBlock, -1, pState);
    
    //Generate code for 'then' block
    childCodegen(statement, 1, pState);
    if (!conditional)
        instruction8(OC_POP, pState);
    const int thenFinalBlock = curBlockId(pState);    
    endBlock (thenFinalBlock+1, thenFinalBlock+1, pState);
    
    //Try to generate 'else'
    if (childCodegen(statement, 2, pState))
    {
        if (!conditional)
            instruction8(OC_POP, pState);
        
        //'else' block index
        const int nextBlock = curBlockId(pState)+1;
        endBlock (nextBlock, nextBlock, pState);
        
        //Fix 'then' jump destination
        setTrueJump (thenFinalBlock, nextBlock, pState);
        setFalseJump (thenFinalBlock, nextBlock, pState);
    }

    //Fix else jump
    setFalseJump (thenInitialBlock-1, thenFinalBlock+1, pState);
    
    //Non expression statements leave an 'undefined' on the stack.
    if (!conditional)
        pushUndefined(pState);
}

/**
 * Generates code for a 'for' loop.
 * It also implements 'while' loops, as they are just a for without initialization 
 * and increment statements. 
 * @param statement
 * @param pState
 */
void forCodegen (Ref<AstNode> statement, CodegenState* pState)
{
    //For loops define its own scope
    instruction8(OC_PUSH_SCOPE, pState);
    
    //Loop initialization
    if (childCodegen (statement, 0, pState))
        instruction8(OC_POP, pState);
    
    const int conditionBlock = curBlockId(pState)+1;
    //Generate code for condition
    endBlock (conditionBlock, conditionBlock, pState);
    if (!childCodegen(statement, 1, pState))
    {
        //If there is no condition, we replace it by an 'always true' condition
        pushConstant(true, pState);
    }
    const int bodyBegin = curBlockId(pState)+1;
    endBlock (bodyBegin, -1, pState);

    //loop body & increment
    if (childCodegen (statement, 3, pState))
        instruction8(OC_POP, pState);

    if (childCodegen (statement, 2, pState))
        instruction8(OC_POP, pState);
    
    const int nextBlock = curBlockId(pState)+1;
    endBlock(conditionBlock, conditionBlock, pState);
    
    //Fix condition jump destination
    setFalseJump(bodyBegin-1, nextBlock, pState);    
    
    //Remove loop scope
    instruction8(OC_POP_SCOPE, pState);
    
    //Non expression statements leave an 'undefined' on the stack.
    pushUndefined(pState);
}

/**
 * Generates code for a 'return' statement
 * @param statement
 * @param pState
 */
void returnCodegen (Ref<AstNode> statement, CodegenState* pState)
{
    //It just pushes return expression value on the stack, and sets next block 
    //indexes to (-1), which means that the current function shall end.
    if (!childCodegen(statement, 0, pState))
    {
        //If it is an empty return statement, push a 'undefined' value on the stack
        pushUndefined(pState);
    }
    
    //instruction8(OC_RETURN, pState);
    endBlock(-1, -1, pState);
}

/**
 * Generates code for a function declaration.
 * @param node
 * @param pState
 */
void functionCodegen (Ref<AstNode> node, CodegenState* pState)
{
    Ref<AstFunction>    fnNode = node.staticCast<AstFunction>();
    Ref<JSFunction>     function = JSFunction::createJS(fnNode->getName());
    const AstFunction::Params& params = fnNode->getParams();
    
    CodegenState    fnState = initFunctionState(node);

    function->setParams (params);
    function->setCodeMVM (fnState.curRoutine);
    
    codegen (fnNode->getCode(), &fnState);
    
    //Push constant to return it.
    pushConstant(function, pState);
    if (!fnNode->getName().empty())
    {
        //If it is a named function, create a constant at current scope.
        pushConstant(fnNode->getName(), pState);
        
        //Copy function reference.
        instruction8(OC_CP+1, pState);
        instruction8(OC_NEW_CONST, pState);
    }
}

/**
 * Generates code for assignments
 * @param statement
 * @param pState
 */
void assignmentCodegen (Ref<AstNode> statement, CodegenState* pState)
{
    Ref<AstOperator>  assign = statement.staticCast<AstOperator>();
    const int         op = assign->code;
    
    childCodegen(statement, 0, pState);
    const int           rdInst = removeLastInstruction (pState);
    
    ASSERT (rdInst == OC_RD_LOCAL || rdInst == OC_RD_GLOBAL || rdInst == OC_RD_FIELD);
    const int wrInst = rdInst +1;
    
    if (op == '=')
    {
        //Simple assignment
        childCodegen(statement, 1, pState);

        instruction8(OC_CP_AUX, pState);
        instruction8(wrInst, pState);
        instruction8(OC_PUSH_AUX, pState);
    }
    else
    {
        //Duplicate the values used to read the variable, to write it later.
        if (rdInst != OC_RD_FIELD)
            instruction8 (OC_CP, pState);
        else
        {
            instruction8 (OC_CP+1, pState);
            instruction8 (OC_CP+1, pState);
        }
        
        //Add again the read instruction.
        instruction8 (rdInst, pState);
        
        //execute right side
        childCodegen(statement, 1, pState);
        
        //execute operation
        binaryOperatorCode (op - LEX_ASSIGN_BASE, pState);
        instruction8 (OC_CP_AUX, pState);

        //Execute write.
        instruction8(wrInst, pState);

        //The assigned value was stored in 'AUX' register.
        instruction8 (OC_PUSH_AUX, pState);
    }
}

/**
 * Generates code for a function call
 * @param statement
 * @param pState
 */
void fncallCodegen (Ref<AstNode> statement, CodegenState* pState)
{
    if (statement->getType() == AST_NEWCALL)
        constructorCodegen(statement, pState);
    else
    {
        const AstNodeTypes fnExprType = statement->children()[0]->getType();
        
        //If the expression to get the function reference is an object member access,
        //then use generate a 'this' call.
        if (fnExprType == AST_MEMBER_ACCESS || fnExprType == AST_ARRAY_ACCESS)
            thisCallCodegen (statement, pState);
        else
        {
            //Regular function call (no this pointer)
            pushUndefined(pState);      //No 'this' pointer.
            
            //Parameters evaluation
            const int nChilds = (int)statement->children().size();
            for (int i = 1; i < nChilds; ++i)
                childCodegen(statement, i, pState);

            //Evaluate function reference expression
            childCodegen(statement, 0, pState);

            callInstruction (nChilds, pState);
        }
    }
}

/**
 * Generates code for function call which receives a 'this' reference.
 * @param statement
 * @param pState
 */
void thisCallCodegen (Ref<AstNode> statement, CodegenState* pState)
{
    childCodegen(statement, 0, pState);
    
    const int rdInstruction = removeLastInstruction(pState);
    instruction8(OC_CP+1, pState);
    instruction8(OC_SWAP, pState);
    instruction8(rdInstruction, pState);
    
    //Stack state at this point: [this, function]
    
    //Parameters evaluation
    const int nChilds = (int)statement->children().size();
    for (int i = 1; i < nChilds; ++i)
    {
        childCodegen(statement, i, pState);     //Parameter evaluation
        instruction8(OC_SWAP, pState);          //Put function back on top        
    }
    
    //Write call instruction
    callInstruction (nChilds, pState);
}

/**
 * Generates code for a operator 'new' function call
 * @param statement
 * @param pState
 */
void constructorCodegen (Ref<AstNode> statement, CodegenState* pState)
{
    childCodegen(statement, 0, pState); //[function]
    instruction8(OC_CP, pState);        //[function, function]

    //Create a new object, to pass it as 'this' reference. '@newObj' receives as
    //parameter the constructor function, from which it will read the object prototype.
    callCodegen("@newObj", 1, pState);  //[function, this]
    
    instruction8(OC_SWAP, pState);      //[this, function]
    instruction8(OC_CP+1, pState);      //[this, function, this]
    instruction8(OC_SWAP, pState);      //[this, this, function]
    
    //Parameters evaluation
    const int nChilds = (int)statement->children().size();
    for (int i = 1; i < nChilds; ++i)
    {
        childCodegen(statement, i, pState);     //Parameter evaluation
        instruction8(OC_SWAP, pState);          //Put function back on top        
    }
    
    //stack: [this, this, param1, param2,... paramN, function]
    
    //Write call instruction
    callInstruction (nChilds, pState);  //[this, result]
    
    //Discard function result, keep 'this' (the new object)
    instruction8(OC_POP, pState);
}

/**
 * Generates code for a literal expression.
 * @param statement
 * @param pState
 */
void literalCodegen (Ref<AstNode> statement, CodegenState* pState)
{
    pushConstant(statement->getValue(), pState);
}

/**
 * Code generation for identifier reading.
 * It generates code for reading it. The assignment operator replaces the instruction,
 * if necessary, for a write instruction
 * @param statement
 * @param pState
 */
void identifierCodegen (Ref<AstNode> statement, CodegenState* pState)
{
    const string name = statement->getName();
    
    ASSERT (!name.empty());
    
    pushConstant(name, pState);
    if (pState->scopes.back().isDeclared(name))
        instruction8 (OC_RD_LOCAL, pState);
    else
        instruction8 (OC_RD_GLOBAL, pState);
}

/**
 * Generates code for an array literal.
 * @param statement
 * @param pState
 */
void arrayCodegen (Ref<AstNode> statement, CodegenState* pState)
{
    const AstNodeList children = statement->children();
    
    pushConstant((int)children.size(), pState);
    callCodegen("@newArray", 1, pState);
    
    for (int i=0; i < (int)children.size(); ++i)
    {
        instruction8(OC_CP, pState);        //Copy array reference
        pushConstant(i, pState);            //Array index
        childCodegen(statement, i, pState); //Value expression
        instruction8(OC_WR_FIELD, pState);
    }
    
    //After the loop, the array reference is on the top of the stack
}

/**
 * Generates code for an object literal.
 * @param statement
 * @param pState
 */
void objectCodegen (Ref<AstNode> statement, CodegenState* pState)
{
    Ref<AstObject>                  obj = statement.staticCast<AstObject>();
    const AstObject::PropertyList   properties= obj->getProperties();
    
    callCodegen("@newObj", 0, pState);

    //TODO: Properties are not evaluated in definition order, but in 
    //alphabetical order, as they are defined in a map.
    AstObject::PropertyList::const_iterator     it;
    for (it = properties.begin(); it != properties.end(); ++it)
    {
        instruction8(OC_CP, pState);        //Copy object reference
        pushConstant(it->name, pState);    //Property name
        codegen(it->expr, pState);        //Value expression
        
        const int opCode = it->isConst ? OC_NEW_CONST_FIELD : OC_WR_FIELD;
        instruction8(opCode, pState);
    }
    
    //After the loop, the object reference is on the top of the stack
}

/**
 * Generates code for array access operator ('[index]')
 * @param statement
 * @param pState
 */
void arrayAccessCodegen (Ref<AstNode> statement, CodegenState* pState)
{
    childrenCodegen(statement, pState);
    instruction8 (OC_RD_FIELD, pState);
}

/**
 * Generates code for member access operator ('object.field')
 * @param statement
 * @param pState
 */
void memberAccessCodegen(Ref<AstNode> statement, CodegenState* pState)
{
    childCodegen(statement, 0, pState);
    
    const string  fieldId = statement->children()[1]->getName();
    pushConstant(fieldId, pState);
    instruction8 (OC_RD_FIELD, pState);
}

/**
 * Generates code for the conditional operator.
 * @param statement
 * @param pState
 */
void conditionalCodegen (Ref<AstNode> statement, CodegenState* pState)
{
    //The conditional operator is handled by 'ifCodegen', as it is very similar.
    ifCodegen(statement, pState);    
}

/**
 * Generates code for a binary operators
 * @param statement
 * @param pState
 */
void binaryOpCodegen (Ref<AstNode> statement, CodegenState* pState)
{
    Ref<AstOperator>    op = statement.staticCast<AstOperator>();
    const int           opCode = op->code;
    
    if (opCode == LEX_OROR || opCode == LEX_ANDAND)
    {
        //Logical operators are special, because they have short-circuited evaluation
        //In all other cases, both sides of operation are evaluated.
        logicalOpCodegen (opCode, statement, pState);
    }
    else
    {
        childrenCodegen(statement, pState);
        binaryOperatorCode(opCode, pState);
    }    
}

/**
 * Generates code for a prefix operator ('++' or '--')
 * @param statement
 * @param pState
 */
void prefixOpCodegen(Ref<AstNode> statement, CodegenState* pState)
{
    Ref<AstOperator>    op = statement.staticCast<AstOperator>();
    const int           opCode = op->code;
    
    if (opCode == LEX_PLUSPLUS || opCode == LEX_MINUSMINUS)
    {
        //It is translated into a '+=' or '-=' equivalent operation
        Ref<AstLiteral> l = AstLiteral::create(op->position(), 1);
        const int       newOpCode = LEX_ASSIGN_BASE + (op->code == LEX_PLUSPLUS ? '+' : '-');
        Ref<AstNode>    newOp = astCreateAssignment(op->position(),
                                                    newOpCode,
                                                    op->children()[0], 
                                                    l);
        codegen(newOp, pState);
    }
    else if (opCode != '+')     //Plus unary operator does nothing, so no code is generated
    {
        childrenCodegen(statement, pState);
        const char* function;
        
        switch (opCode)
        {
        case '-':       function = "@negate"; break;
        case '~':       function = "@binNot"; break;
        case '!':       function = "@logicNot"; break;
        default:
            ASSERT (!"Unexpected operator");
        }
        
        //Call function
        callCodegen(function, 1, pState);
    }
}

/**
 * Generates code for a prefix operator ('++' or '--')
 * @param statement
 * @param pState
 */
void postfixOpCodegen (Ref<AstNode> statement, CodegenState* pState)
{
    Ref<AstOperator>    op = statement.staticCast<AstOperator>();
 
    childrenCodegen(statement, pState);
    const int rdInst = removeLastInstruction(pState);
    const int wrInst = rdInst + 1;
    
    if (rdInst != OC_RD_FIELD)
        instruction8 (OC_CP, pState);
    else
    {
        instruction8 (OC_CP+1, pState);
        instruction8 (OC_CP+1, pState);
    }
    
    instruction8(rdInst, pState);
    instruction8(OC_CP, pState);        //Save previous
    callCodegen(op->code == LEX_PLUSPLUS ? "@inc": "@dec", 1, pState);
    
    //Move previous value to aux
    instruction8(OC_SWAP, pState);
    instruction8(OC_CP_AUX, pState);
    instruction8(OC_POP, pState);
    
    //Write result.
    instruction8(wrInst, pState);
    
    //Recover previous value from aux.
    instruction8 (OC_PUSH_AUX, pState);
}

/**
 * Generates code for a logical operator
 * @param opCode
 * @param statement
 * @param pState
 */
void logicalOpCodegen (const int opCode, Ref<AstNode> statement, CodegenState* pState)
{
    childCodegen(statement, 0, pState);
    instruction8(OC_CP, pState);
    const int firstBlock = curBlockId(pState);
    
    endBlock(-1, -1, pState);
    instruction8(OC_POP, pState);
    childCodegen(statement, 1, pState);
    const int secondBlock = curBlockId(pState);
    
    endBlock (secondBlock+1, secondBlock+1, pState);
    
    if (opCode == LEX_OROR)
    {
        setTrueJump(firstBlock, secondBlock+1, pState);
        setFalseJump(firstBlock, firstBlock+1, pState);
    }
    else
    {
        setTrueJump(firstBlock, firstBlock+1, pState);
        setFalseJump(firstBlock, secondBlock+1, pState);
    }
}

/**
 * Generates code for an actor declaration
 * @param node
 * @param pState
 */
void actorCodegen (Ref<AstNode> node, CodegenState* pState)
{
    auto actor = AsActorClass::create(node->getName());
    
    const AstNode::Params&  params = node->getParams();
    CodegenState            actorState = initFunctionState(node);
    auto                    constructor = actor->getConstructor();
    
    actorState.curActor = actor;
    constructor->setParams (params);
    constructor->setCodeMVM (actorState.curRoutine);
    
    childrenCodegen(node, &actorState);
    
    actor->createDefaultEndPoints ();
    
    //Create a new constant, and yield actor class reference
    pushConstant( deepFreeze(actor), pState);
    pushConstant(node->getName(), pState);
    instruction8(OC_CP+1, pState);
    instruction8(OC_NEW_CONST, pState);
}

/**
 * Generates code for 'connect' operator
 * @param node
 * @param pState
 */
void connectCodegen (Ref<AstNode> node, CodegenState* pState)
{
    pushConstant("this", pState);
    instruction8(OC_RD_LOCAL, pState);
    pushConstant(node->children().front()->getName(), pState);
    instruction8(OC_RD_FIELD, pState);
    childCodegen(node, 1, pState);
    callCodegen("@connect", 2, pState);
}

/**
 * Generates code for a message declared inside an actor.
 * @param node
 * @param pState
 */
void messageCodegen (Ref<AstNode> node, CodegenState* pState)
{
    ASSERT(pState->curActor.notNull());
    
    Ref<AsEndPoint>  msg = AsEndPoint::create(node->getName(), 
                                            node->getType() == AST_INPUT);

    msg->setParams(node->getParams());
    
    if (node->getType() == AST_INPUT)
    {
        //TODO: This code is very simular in function, actor, and message code generation
        Ref<MvmRoutine>         code = MvmRoutine::create(node->position());

        CodegenState    fnState = initFunctionState(node);
        msg->setCodeMVM (code);
        fnState.curRoutine = code;

        codegen (node.staticCast<AstFunction>()->getCode(), &fnState);
    }

    pState->curActor->writeFieldStr (msg->getName(), msg);
}


/**
 * Generates the instruction which pushes a constant form the constant table into
 * the stack
 * @param value
 * @param pState
 */
void pushConstant (Ref<JSValue> value, CodegenState* pState)
{
    //TODO: It can be further optimized, by deduplicating constants at script level.
    int                             id = (int)pState->curRoutine->constants.size();
    ConstantsMap::const_iterator    it = pState->constants.find(value);
    
    if (it != pState->constants.end())
        id = it->second;
    else
    {
        pState->constants[value] = id;
        pState->curRoutine->constants.push_back(value);
    }
    
    if (id < OC_PUSHC)
        instruction8(OC_PUSHC + id, pState);
    else
    {
        const int id16 = id - OC_PUSHC;

        //TODO: 32 bit instructions, to have more constants.
        if (id16 >= OC16_PUSHC)
            errorAt (pState->curRoutine->position,  "Too much constants. Maximum is 8256 per function");
        else
            instruction16(id16 + OC16_PUSHC, pState);
    }
}

/**
 * Push string constant
 * @param str
 * @param pState
 */
void pushConstant (const char* str, CodegenState* pState)
{
    pushConstant(jsString(str), pState);
}

/**
 * Push string constant
 * @param str
 * @param pState
 */
void pushConstant (const std::string& str, CodegenState* pState)
{
    pushConstant(jsString(str), pState);
}

/**
 * Push integer constant
 * @param value
 * @param pState
 */
void pushConstant (int value, CodegenState* pState)
{
    pushConstant(jsInt(value), pState);
}

/**
 * Push boolean constant
 * @param value
 * @param pState
 */
void pushConstant (bool value, CodegenState* pState)
{
    pushConstant(jsBool(value), pState);
}

/**
 * Pushes an 'undefined' value into the stack
 * @param pState
 */
void pushUndefined (CodegenState* pState)
{
    pushConstant(undefined(), pState);
}

/**
 * Adds the instructions necessary to perform a call to a function, given its name.
 * @param fnName
 * @param nParams
 * @param pState
 */
void callCodegen (const std::string& fnName, int nParams, CodegenState* pState)
{
    pushConstant(fnName, pState);
    
    if (pState->scopes.back().isDeclared(fnName))
        instruction8(OC_RD_LOCAL, pState);
    else
        instruction8(OC_RD_GLOBAL, pState);
    callInstruction(nParams, pState);    
}


/**
 * Writes a call instruction. Takes into account the number of parameters to 
 * create the 8 bit or 16 version.
 * @param nParams Number of parameters, including this pointer (which is the first one)
 * @param pState
 */
void callInstruction (int nParams, CodegenState* pState)
{
    if (nParams <= OC_CALL_MAX)
        instruction8(OC_CALL + nParams, pState);
    else {
        //I think that 1031 arguments are enough.
        if (nParams > (OC_CALL_MAX + OC16_CALL_MAX + 1))
            error("Too much arguments in function call: %d", nParams);
        
        //TODO: Provide location for the error.
        
        instruction16(OC16_CALL + nParams - (OC_CALL_MAX+1), pState);
    }
}

/**
 * Writes a 8 bit instruction to the current block
 * @param opCode
 * @param pState
 */
void instruction8 (int opCode, CodegenState* pState)
{
    ASSERT (opCode < 128 && opCode >=0);
    
    pState->curRoutine->blocks.rbegin()->instructions.push_back(opCode);
}

/**
 * Writes a 16 bit instruction to the current block
 * @param opCode
 * @param pState
 */
void instruction16 (int opCode, CodegenState* pState)
{
    ASSERT (opCode >=0 && opCode < 0x8000);
    
    opCode |= 0x8000;   //16 bits indicator
    
    ByteVector&     block = pState->curRoutine->blocks.rbegin()->instructions;
    
    block.push_back((unsigned char)(opCode >> 8));
    block.push_back((unsigned char)(opCode & 0xff));
}

/**
 * Returns last instruction of the current block.
 * @param pState
 * @return 
 */
int  getLastInstruction (CodegenState* pState)
{
    const ByteVector& block = pState->curRoutine->blocks.rbegin()->instructions;
    
    if (block.size() > 1)
    {
        const unsigned char     last = *block.rbegin();
        const unsigned char     prev = *(block.rbegin()+1);
        
        if (prev & OC_EXT_FLAG)
        {
            const int instruction = (int(prev)<< 8) + int(last);
            return instruction;
        }
        else
        {
            ASSERT(!(last & OC_EXT_FLAG));
            return last;
        }
    }
    else if (block.size() == 1)
        return block[0];
    else
        return -1;
        
}

/**
 * Removes last instruction from the current block.
 * @param pState
 * @return 
 */
int removeLastInstruction (CodegenState* pState)
{
    ByteVector& block = pState->curRoutine->blocks.rbegin()->instructions;
    const int   lastInstruction = getLastInstruction(pState);
    
    if (lastInstruction & 0x8000)
        block.resize(block.size()-2);
    else if (lastInstruction >= 0)
        block.resize(block.size()-1);
    
    return lastInstruction;
}

/**
 * Generates the instruction or call for a binary operator
 * @param tokenCode
 * @param pState
 */
void binaryOperatorCode (int tokenCode, CodegenState* pState)
{
    typedef map <int, string> OpMap;
    static OpMap operators;
    
    if (operators.empty())
    {
        operators['+'] =                "@add";
        operators['-'] =                "@sub";
        operators['*'] =                "@multiply";
        operators['/'] =                "@divide";
        operators['%'] =                "@modulus";
        operators[LEX_POWER] =          "@power";
        operators['&'] =                "@binAnd";
        operators['|'] =                "@binOr";
        operators['^'] =                "@binXor";
        operators[LEX_LSHIFT] =         "@lshift";
        operators[LEX_RSHIFT] =         "@rshift";
        operators[LEX_RSHIFTUNSIGNED] = "@rshiftu";
        operators['<'] =                "@less";
        operators['>'] =                "@greater";
        operators[LEX_EQUAL] =          "@areEqual";
        operators[LEX_TYPEEQUAL] =      "@areTypeEqual";
        operators[LEX_NEQUAL] =         "@notEqual";
        operators[LEX_NTYPEEQUAL] =     "@notTypeEqual";
        operators[LEX_LEQUAL] =         "@lequal";
        operators[LEX_GEQUAL] =         "@gequal";
    }
    
    OpMap::const_iterator it = operators.find(tokenCode);
    
    ASSERT (it != operators.end());
    callCodegen(it->second, 2, pState);
}

/**
 * Ends current block, and creates a new one.
 * @param trueJump      Jump target when the value on the top of the stack is 'true'
 * @param falseJump     Jump target when the value on the top of the stack is 'false'
 * @param pState
 */
void endBlock (int trueJump, int falseJump, CodegenState* pState)
{
    MvmBlock    &curBlock = pState->curRoutine->blocks.back();
    
    curBlock.nextBlocks[1] = trueJump;
    curBlock.nextBlocks[0] = falseJump;

    pState->curRoutine->blocks.push_back(MvmBlock());
}

/**
 * Sets the jump target for the specified block, in 'true' case
 * @param blockId
 * @param destinationId
 * @param pState
 */
void setTrueJump (int blockId, int destinationId, CodegenState* pState)
{
    pState->curRoutine->blocks[blockId].nextBlocks[1] = destinationId;
}

/**
 * Sets the jump target for the specified block, in 'false' case
 * @param blockId
 * @param destinationId
 * @param pState
 */
void setFalseJump (int blockId, int destinationId, CodegenState* pState)
{
    pState->curRoutine->blocks[blockId].nextBlocks[0] = destinationId;
}

/**
 * Returns current block id.
 */
int curBlockId (CodegenState* pState)
{
    return (int)pState->curRoutine->blocks.size() - 1;
}


/**
 * Initializes a 'CodegenState' object for a function
 * @param node
 * @return 
 */
CodegenState initFunctionState (Ref<AstNode> node)
{
    Ref<MvmRoutine>         code = MvmRoutine::create(node->position());
    const AstNode::Params&  params = node->getParams();
    CodegenState            fnState;
    
    fnState.curRoutine = code;
    fnState.scopes.push_back(CodegenScope(node));

    //Declare function reserved symbols.
    fnState.scopes.back().declare("this");
    fnState.scopes.back().declare("arguments");

    for (size_t i = 0; i < params.size(); ++i)
        fnState.scopes.back().declare(params[i]);

    return fnState;
}
