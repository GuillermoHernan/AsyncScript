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

#include <set>
#include <map>
#include <string>

using namespace std;

typedef AstStatement::ChildList StatementList;

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
    
private:
    std::set < std::string>     m_symbols;
};

/**
 * State of a codegen operation
 */
struct CodegenState
{
    Ref<MvmScript>              curScript;
    std::vector<CodegenScope>   scopes;    
};

//Forward declarations

typedef void (*CodegenFN)(Ref<AstStatement> statement, CodegenState* pState);

void codegen (Ref<AstStatement> statement, CodegenState* pState);
int  childrenCodegen (Ref<AstStatement> statement, CodegenState* pState);
bool childCodegen (Ref<AstStatement> statement, int index, CodegenState* pState);

void blockCodegen (Ref<AstStatement> statement, CodegenState* pState);
void varCodegen (Ref<AstStatement> statement, CodegenState* pState);
void ifCodegen (Ref<AstStatement> statement, CodegenState* pState);
void forCodegen (Ref<AstStatement> statement, CodegenState* pState);
void returnCodegen (Ref<AstStatement> statement, CodegenState* pState);
void functionCodegen (Ref<AstStatement> statement, CodegenState* pState);
void assignmentCodegen (Ref<AstStatement> statement, CodegenState* pState);
void fncallCodegen (Ref<AstStatement> statement, CodegenState* pState);
void thisCallCodegen (Ref<AstStatement> statement, CodegenState* pState);
void literalCodegen (Ref<AstStatement> statement, CodegenState* pState);
void identifierCodegen (Ref<AstStatement> statement, CodegenState* pState);
void arrayCodegen (Ref<AstStatement> statement, CodegenState* pState);
void objectCodegen (Ref<AstStatement> statement, CodegenState* pState);
void arrayAccessCodegen (Ref<AstStatement> statement, CodegenState* pState);
void memberAccessCodegen (Ref<AstStatement> statement, CodegenState* pState);
void conditionalCodegen (Ref<AstStatement> statement, CodegenState* pState);
void binaryOpCodegen (Ref<AstStatement> statement, CodegenState* pState);
void prefixOpCodegen (Ref<AstStatement> statement, CodegenState* pState);
void postfixOpCodegen (Ref<AstStatement> statement, CodegenState* pState);
void logicalOpCodegen (const int opCode, Ref<AstStatement> statement, CodegenState* pState);

void pushConstant (Ref<JSValue> value, CodegenState* pState);
void pushConstant (const std::string& str, CodegenState* pState);
void pushConstant (int value, CodegenState* pState);
void pushConstant (bool value, CodegenState* pState);
void pushUndefined (CodegenState* pState);

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



/**
 * Generates MVM code for a script.
 * @param statements    The script is received as a sequence (vector) of compiled statements.
 * @return 
 */
Ref<MvmScript> scriptCodegen (const StatementList& statements)
{
    CodegenState    state;
    
    ASSERT (!statements.empty());
    
    state.curScript = MvmScript::create(ScriptPosition(1,1));
    state.scopes.push_back(CodegenScope());
    
    for (size_t i = 0; i < statements.size(); ++i)
        codegen (statements[i], &state);
    
    return state.curScript;    
}


void codegen (Ref<AstStatement> statement, CodegenState* pState)
{
    static CodegenFN types[AST_TYPES_COUNT] = {NULL, NULL};
    
    if (types [0] == NULL)
    {
        types [AST_BLOCK] = blockCodegen;
        types [AST_VAR] = varCodegen;
        types [AST_IF] = ifCodegen;
        types [AST_FOR] = forCodegen;
        types [AST_RETURN] = returnCodegen;
        types [AST_FUNCTION] = functionCodegen;
        types [AST_ASSIGNMENT] = assignmentCodegen;
        types [AST_FNCALL] = fncallCodegen;
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
    }
    
    types[statement->getType()](statement, pState);
}

/**
 * Generates code for the children of a given statement
 * @param statement
 * @param pState
 */
int childrenCodegen (Ref<AstStatement> statement, CodegenState* pState)
{
    const StatementList&  children = statement->children();
    int count = 0;
    
    for (size_t i; i < children.size(); ++i)
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
bool childCodegen (Ref<AstStatement> statement, int index, CodegenState* pState)
{
    const StatementList&  children = statement->children();
    
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
 * Generates the code for a block of statements.
 * @param statement
 * @param pState
 */
void blockCodegen (Ref<AstStatement> statement, CodegenState* pState)
{
    const StatementList&  children = statement->children();
    
    for (size_t i; i < children.size(); ++i)
    {
        if (children[i].notNull())
        {
            codegen (children[i], pState);
            instruction8(OC_POP, pState);   //Discard result
        }
    }
        
    //Non expression stataments leave an 'undefined' on the stack.
    pushUndefined(pState);
}

/**
 * Generates code for a 'var' declaration
 * @param statement
 * @param pState
 */
void varCodegen (Ref<AstStatement> statement, CodegenState* pState)
{
    const std::string& name = statement.staticCast<AstVar>()->name;
    pState->scopes.rbegin()->declare(name);
    
    if (statement->childExists(0))
    {
        pushConstant (name, pState);
        childrenCodegen(statement, pState);
        instruction8 (OC_WR_LOCAL, pState);
    }
    
    //Non expression stataments leave an 'undefined' on the stack.
    pushUndefined(pState);
}

/**
 * Generates code for an 'if' statement
 * @param statement
 * @param pState
 */
void ifCodegen (Ref<AstStatement> statement, CodegenState* pState)
{
    const int conditionBlock = curBlockId(pState)+1;
    
    //Generate code for condition
    endBlock (conditionBlock, conditionBlock, pState);
    childCodegen(statement, 0, pState);
    
    const int thenInitialBlock = curBlockId(pState)+1;
    endBlock (thenInitialBlock, -1, pState);
    
    //Generate code for 'then' block
    childCodegen(statement, 1, pState);
    const int thenFinalBlock = curBlockId(pState);
    endBlock (thenFinalBlock+1, thenFinalBlock+1, pState);
    
    //Try to generate 'else'
    if (childCodegen(statement, 2, pState))
    {
        //'else' block code
        const int nextBlock = curBlockId(pState)+1;
        endBlock (nextBlock, nextBlock, pState);
        
        //Fix 'then' jump destination
        setTrueJump (thenFinalBlock+1, nextBlock, pState);
        setFalseJump (thenFinalBlock+1, nextBlock, pState);
    }

    //Fix else jump
    setFalseJump (thenInitialBlock-1, thenFinalBlock+1, pState);
    
    //Non expression stataments leave an 'undefined' on the stack.
    pushUndefined(pState);
}

/**
 * Generates code for a 'for' loop.
 * It also implements 'while' loops, as they are just a for without initialization 
 * and increment statements. 
 * @param statement
 * @param pState
 */
void forCodegen (Ref<AstStatement> statement, CodegenState* pState)
{
    //Loop initialization
    childCodegen (statement, 0, pState);
    
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
    childCodegen (statement, 2, pState);
    childCodegen (statement, 3, pState);
    
    const int nextBlock = curBlockId(pState)+1;
    endBlock(conditionBlock, conditionBlock, pState);
    
    //Fix condition jump destination
    setFalseJump(bodyBegin-1, nextBlock, pState);    
    
    //Non expression stataments leave an 'undefined' on the stack.
    pushUndefined(pState);
}

/**
 * Generates code for a 'return' statement
 * @param statement
 * @param pState
 */
void returnCodegen (Ref<AstStatement> statement, CodegenState* pState)
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
 * @param statement
 * @param pState
 */
void functionCodegen (Ref<AstStatement> statement, CodegenState* pState)
{
    Ref<AstFunction>    fnNode = statement.staticCast<AstFunction>();
    Ref<JSFunction>     function = JSFunction::createJS(fnNode->getName());
    Ref<MvmScript>      code = MvmScript::create(fnNode->position());
    const AstFunction::Params& params = fnNode->getParams();
    
    function->setParams (params);
    function->setCodeMVM (code);
    
    //TODO: review state & scope structures.
    CodegenState    fnState = *pState;
    fnState.curScript = code;
    fnState.scopes.push_back(CodegenScope());
    
    for (size_t i = 0; i < params.size(); ++i)
        fnState.scopes.rbegin()->declare(params[i]);
    
    codegen (fnNode->getCode(), &fnState);
    
    //Push constant to return it.
    pushConstant(function, pState);
    if (!fnNode->getName().empty())
    {
        //If it is a named function, create a variable at current scope.
        pushConstant(fnNode->getName(), pState);
        
        //Copy function reference.
        instruction8(OC_CP+1, pState);
        instruction8(OC_WR_LOCAL, pState);
    }
}

/**
 * Generates code for assignments
 * @param statement
 * @param pState
 */
void assignmentCodegen (Ref<AstStatement> statement, CodegenState* pState)
{
    Ref<AstAssignment>  assign = statement.staticCast<AstAssignment>();
    const int           op = assign->code;
    
    childCodegen(statement, 0, pState);
    const int           rdInst = removeLastInstruction (pState);
    
    ASSERT (rdInst == OC_RD_LOCAL || rdInst == OC_RD_GLOBAL || rdInst == OC_RD_FIELD);
    const int wrInst = rdInst +1;
    
    if (op == '=')
    {
        //Simple assignment
        childCodegen(statement, 1, pState);
        instruction8(wrInst, pState);
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

        //Execute write.
        instruction8(wrInst, pState);
    }
}

/**
 * Generates code for a function call
 * @param statement
 * @param pState
 */
void fncallCodegen (Ref<AstStatement> statement, CodegenState* pState)
{
    Ref<AstFunctionCall>    callNode = statement.staticCast<AstFunctionCall>();
    
    if (callNode->getNewFlag())
    {
        //Create a new object, and pass it as 'this' reference.
        pushConstant("@newObj", pState);
        instruction8(OC_CALL, pState);
    }
    else
    {
        const AstNodeTypes fnExprType = statement->children()[0]->getType();
        
        //If the expression to get the function reference is an object member access,
        //then use generate a 'this' call.
        if (fnExprType == AST_MEMBER_ACCESS || fnExprType == AST_ARRAY_ACCESS)
        {
            thisCallCodegen (statement, pState);
            return;
        }
        else
            pushUndefined(pState);      //No 'this' pointer.
    }
    
    //Parameters evaluation
    const int nChilds = (int)callNode->children().size();
    for (int i = 1; i < nChilds; ++i)
        childCodegen(statement, i, pState);

    //Evaluate function reference expression
    childCodegen(statement, 0, pState);
    
    callInstruction (nChilds, pState);
}

/**
 * Generates code for function call which receives a 'this' reference.
 * @param statement
 * @param pState
 */
void thisCallCodegen (Ref<AstStatement> statement, CodegenState* pState)
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
 * Generates code for a literal expression.
 * @param statement
 * @param pState
 */
void literalCodegen (Ref<AstStatement> statement, CodegenState* pState)
{
    Ref<AstLiteral>     literal = statement.staticCast<AstLiteral>();
    
    pushConstant(literal->value, pState);
}

/**
 * Code generation for identifier reading.
 * It generates code for reading it. The assignment operator replaces the instruction,
 * if necessary, for a write instruction
 * @param statement
 * @param pState
 */
void identifierCodegen (Ref<AstStatement> statement, CodegenState* pState)
{
    Ref<AstIdentifier>  id = statement.staticCast<AstIdentifier>();
    
    pushConstant(id->name, pState);
    if (pState->scopes.rbegin()->isDeclared(id->name))
        instruction8 (OC_RD_LOCAL, pState);
    else
        instruction8 (OC_RD_GLOBAL, pState);
}

/**
 * Generates code for an array literal.
 * @param statement
 * @param pState
 */
void arrayCodegen (Ref<AstStatement> statement, CodegenState* pState)
{
    const StatementList children = statement->children();
    
    pushConstant((int)children.size(), pState);
    pushConstant("@newArray", pState);
    instruction8 (OC_RD_GLOBAL, pState);
    instruction8(OC_CALL+1, pState);
    
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
void objectCodegen (Ref<AstStatement> statement, CodegenState* pState)
{
    Ref<AstObject>              obj = statement.staticCast<AstObject>();
    const AstObject::PropsMap   properties= obj->getProperties();
    
    pushConstant("@newObj", pState);
    instruction8 (OC_RD_GLOBAL, pState);
    instruction8(OC_CALL, pState);

    //TODO: Properties are not evaluated in definition order, but in 
    //alphabetical order, as they are defined in a map.
    AstObject::PropsMap::const_iterator     it;
    for (it = properties.begin(); it != properties.end(); ++it)
    {
        instruction8(OC_CP, pState);        //Copy object reference
        pushConstant(it->first, pState);    //Property name
        codegen(it->second, pState);        //Value expression
        instruction8(OC_WR_FIELD, pState);
    }
    
    //After the loop, the object reference is on the top of the stack
}

/**
 * Generates code for array access operator ('[index]')
 * @param statement
 * @param pState
 */
void arrayAccessCodegen (Ref<AstStatement> statement, CodegenState* pState)
{
    childrenCodegen(statement, pState);
    instruction8 (OC_RD_FIELD, pState);
}

/**
 * Generates code for member access operator ('object.field')
 * @param statement
 * @param pState
 */
void memberAccessCodegen(Ref<AstStatement> statement, CodegenState* pState)
{
    //It is just the same code
    arrayAccessCodegen(statement, pState);
}

/**
 * Generates code for the conditional operator.
 * @param statement
 * @param pState
 */
void conditionalCodegen (Ref<AstStatement> statement, CodegenState* pState)
{
    //The conditional operator is just a fancy way to write an 'if'
    ifCodegen(statement, pState);    
}

/**
 * Generates code for a binary operators
 * @param statement
 * @param pState
 */
void binaryOpCodegen (Ref<AstStatement> statement, CodegenState* pState)
{
    Ref<AstBinaryOp>    op = statement.staticCast<AstBinaryOp>();
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
void prefixOpCodegen(Ref<AstStatement> statement, CodegenState* pState)
{
    Ref<AstPrefixOp>    op = statement.staticCast<AstPrefixOp>();
    const int           opCode = op->code;
    
    if (opCode == LEX_PLUSPLUS || opCode == LEX_MINUSMINUS)
    {
        //It is translated into a '+=' or '-=' equivalent operation
        Ref<AstLiteral>     l = AstLiteral::create(op->position(), 1);
        Ref<AstAssignment>  newOp = AstAssignment::create(op->position(),
                                                         op->code == LEX_PLUSPLUS ? '+' : '-',
                                                         op->subExprs()[0], 
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
        pushConstant(function, pState);
        instruction8 (OC_CALL+1, pState);
    }
}

/**
 * Generates code for a prefix operator ('++' or '--')
 * @param statement
 * @param pState
 */
void postfixOpCodegen (Ref<AstStatement> statement, CodegenState* pState)
{
    Ref<AstPostfixOp>    op = statement.staticCast<AstPostfixOp>();
 
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
    pushConstant(op->code == LEX_PLUSPLUS ? "@inc": "@dec", pState);
    instruction8(OC_CALL+1, pState);
    instruction8(wrInst, pState);
}

/**
 * Generates code for a logical operator
 * @param opCode
 * @param statement
 * @param pState
 */
void logicalOpCodegen (const int opCode, Ref<AstStatement> statement, CodegenState* pState)
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
 * Generates the instruction which pushes a constant form the constant table into
 * the stack
 * @param value
 * @param pState
 */
void pushConstant (Ref<JSValue> value, CodegenState* pState)
{
    //TODO: Definitively, there is an optimization opportunity, by avoiding repeated 
    //values into the constant tables of the functions. Or even, by not repeating constants
    //in the whole script.
    const int id = (int)pState->curScript->constants.size();
    
    pState->curScript->constants.push_back(value);
    
    if (id < OC_PUSHC)
        instruction8(OC_PUSHC + id, pState);
    else
    {
        const int id16 = id - OC_PUSHC;

        //TODO: 32 bit instructions, to have more constants.
        if (id16 >= OC16_PUSHC)
            errorAt (pState->curScript->position,  "Too much constants. Maximum is 8256 per function");
        else
            instruction16(id16, pState);
    }
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
    
    pState->curScript->blocks.rbegin()->instructions.push_back(opCode);
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
    
    ByteVector&     block = pState->curScript->blocks.rbegin()->instructions;
    
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
    const ByteVector& block = pState->curScript->blocks.rbegin()->instructions;
    
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
    ByteVector& block = pState->curScript->blocks.rbegin()->instructions;
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
        operators['&'] =                "@binAnd";
        operators['|'] =                "@binOr";
        operators['^'] =                "@binXor";
        operators[LEX_LSHIFT] =         "@lshift";
        operators[LEX_RSHIFT] =         "@rshift";
        operators[LEX_RSHIFTUNSIGNED] = "@rshiftu";
        operators['<'] =                "@less";
        operators['>'] =                "@greater";
        operators[LEX_EQUAL] =          "@isEqual";
        operators[LEX_TYPEEQUAL] =      "@isTypeEqual";
        operators[LEX_NEQUAL] =         "@notEqual";
        operators[LEX_NTYPEEQUAL] =     "@notTypeEqual";
        operators[LEX_LEQUAL] =         "@lequal";
        operators[LEX_GEQUAL] =         "@gequal";
    }
    
    OpMap::const_iterator it = operators.find(tokenCode);
    
    ASSERT (it != operators.end());
    pushConstant(it->second, pState);
    instruction8(OC_CALL+2, pState);
}

/**
 * Ends current block, and creates a new one.
 * @param trueJump      Jump target when the value on the top of the stack is 'true'
 * @param falseJump     Jump target when the value on the top of the stack is 'false'
 * @param pState
 */
void endBlock (int trueJump, int falseJump, CodegenState* pState)
{
    MvmBlock    &curBlock = *pState->curScript->blocks.rbegin();
    
    pState->curScript->blocks.push_back(MvmBlock());
    curBlock.nextBlocks[1] = trueJump;
    curBlock.nextBlocks[0] = falseJump;
}

/**
 * Sets the jump target for the specified block, in 'true' case
 * @param blockId
 * @param destinationId
 * @param pState
 */
void setTrueJump (int blockId, int destinationId, CodegenState* pState)
{
    pState->curScript->blocks[destinationId].nextBlocks[1] = blockId;
}

/**
 * Sets the jump target for the specified block, in 'false' case
 * @param blockId
 * @param destinationId
 * @param pState
 */
void setFalseJump (int blockId, int destinationId, CodegenState* pState)
{
    pState->curScript->blocks[destinationId].nextBlocks[0] = blockId;
}

/**
 * Returns current block id.
 */
int curBlockId (CodegenState* pState)
{
    return (int)pState->curScript->blocks.size() - 1;
}
