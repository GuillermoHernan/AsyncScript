/* 
 * File:   microVM.cpp
 * Author: ghernan
 * 
 * Minimalist Virtual Machine for Javascript
 * 
 * A virtual machine with a very small instruction set.
 * 
 * Created on December 2, 2016, 9:18 PM
 */

#include "OS_support.h"
#include "microVM.h"

#include <vector>

using namespace std;

typedef vector < Ref<IScope> >  ScopeStack;

/**
 * MVM execution context
 */
struct ExecutionContext
{
    ValueVector     stack;
    ValueVector*    constants;
    ScopeStack      scopes;
    Ref<JSValue>    auxRegister;
    
    Ref<JSValue> pop()
    {
        checkStackNotEmpty();
        
        Ref<JSValue>    r = stack.back();
        stack.pop_back();
        return r;
    }
    
    Ref<JSValue> push(Ref<JSValue> value)
    {
        ASSERT (value.notNull());
        stack.push_back(value);
        return value;
    }
    
    ExecutionContext() : constants(NULL)
    {        
    }

    bool checkStackNotEmpty()
    {
        if (stack.empty())
            error ("Stack underflow!");
        
        return !stack.empty();
    }
};

//Forward declarations
////////////////////////////////////////

typedef void (*OpFunction) (const int opCode, ExecutionContext* ec);


Ref<JSValue> execScript (Ref<MvmScript> code, ExecutionContext* ec);
int execBlock (const MvmBlock& block, ExecutionContext* ec);
void execInstruction16 (const int opCode, ExecutionContext* ec);
void execInstruction8 (const int opCode, ExecutionContext* ec);
void execPushC8 (const int opCode, ExecutionContext* ec);
void execPushC16 (const int opCode, ExecutionContext* ec);
void execCall8 (const int opCode, ExecutionContext* ec);
void execCall16 (const int opCode, ExecutionContext* ec);
void execCall (const int nArgs, ExecutionContext* ec);
void execCp8 (const int opCode, ExecutionContext* ec);
void execSwap (const int opCode, ExecutionContext* ec);
void execPop (const int opCode, ExecutionContext* ec);
void execRdLocal (const int opCode, ExecutionContext* ec);
void execWrLocal (const int opCode, ExecutionContext* ec);
void execRdGlobal (const int opCode, ExecutionContext* ec);
void execWrGlobal (const int opCode, ExecutionContext* ec);
void execRdField (const int opCode, ExecutionContext* ec);
void execWrField (const int opCode, ExecutionContext* ec);
void execNop (const int opCode, ExecutionContext* ec);
void execCpAux (const int opCode, ExecutionContext* ec);
void execPushAux (const int opCode, ExecutionContext* ec);

void invalidOp (const int opCode, ExecutionContext* ec);

// 8 bit instruction dispatch table.
///////////////////////////////////////
static const OpFunction s_instructions[64] = 
{
    //0
    execCall8,      execCall8,      execCall8,      execCall8, 
    execCall8,      execCall8,      execCall8,      execCall8, 
    
    //8
    execCp8,        execCp8,        execCp8,        execCp8,
    execSwap,       execPop,        invalidOp,      invalidOp,
    
    //16
    execRdLocal,    execWrLocal,    execRdGlobal,   execWrGlobal,
    execRdField,    execWrField,    invalidOp,      invalidOp,
    
    //24
    invalidOp,      invalidOp,      invalidOp,      invalidOp,
    invalidOp,      invalidOp,      invalidOp,      invalidOp,
    
    //32
    execCpAux,      execPushAux,    invalidOp,      invalidOp,
    invalidOp,      invalidOp,      invalidOp,      invalidOp,
    
    //40
    invalidOp,      invalidOp,      invalidOp,      invalidOp,
    invalidOp,      invalidOp,      invalidOp,      invalidOp,
    
    //48
    invalidOp,      invalidOp,      invalidOp,      invalidOp,
    invalidOp,      invalidOp,      invalidOp,      invalidOp,
    
    //56
    invalidOp,      invalidOp,      invalidOp,      invalidOp,
    invalidOp,      invalidOp,      invalidOp,      execNop
};


/**
 * Executes MVM code.
 * @param code
 * @param globals
 * @return 
 */
Ref<JSValue> mvmExecute (Ref<MvmScript> code, Ref<IScope> globals)
{
    ExecutionContext    ec;
    
    ec.scopes.push_back(globals);
    
    return execScript (code, &ec);
}

/**
 * Executes an script.
 *
 * @param code
 * @param ec        Execution context
 * @return 
 */
Ref<JSValue> execScript (Ref<MvmScript> code, ExecutionContext* ec)
{
    if (code->blocks.empty())
        return undefined();
    
    ValueVector*    prevConstants = ec->constants;
    
    int nextBlock = 0;
    
    //Set constants
    ec->constants = &code->constants;
    
    while (nextBlock >= 0)
    {
        nextBlock = execBlock (code->blocks[nextBlock], ec);
    }
    
    ec->constants = prevConstants;
    
    //'AUX' register is cleared when finishing a script, to prevent memory leaks
    //(and may be also a good security measure)
    ec->auxRegister = undefined();
    
    if (ec->stack.empty())
        return undefined();
    else
        return ec->pop();
}

/**
 * Executes a block of code.
 * @param block
 * @param ec
 * @return Returns the next block to be executed.
 */
int execBlock (const MvmBlock& block, ExecutionContext* ec)
{
    for (size_t i = 0; i < block.instructions.size();)
    {
        int opCode = block.instructions[i++];
        
        if (opCode & OC_EXT_FLAG)
        {
            opCode = (opCode << 8) | block.instructions[i++];
            execInstruction16 (opCode, ec);
        }
        else
            execInstruction8 (opCode, ec);
    }
    
    if (block.nextBlocks[0] == block.nextBlocks[1])
        return block.nextBlocks[0];
    else
    {
        ec->checkStackNotEmpty();
        
        const bool r = ec->pop()->toBoolean();
        
        return block.nextBlocks[r?1:0];
    }
}

/**
 * Executes a 16 bit instruction
 * @param opCode
 * @param ec
 */
void execInstruction16 (const int opCode, ExecutionContext* ec)
{
    const int decoded = opCode & 0x3FF;
    
    if (decoded >= OC16_PUSHC)
        execPushC16 (decoded, ec);
    else if (decoded <= OC16_CALL_MAX)
        execCall16(decoded, ec);
    else
        error ("Invalid 16 bit opCode: %04X", opCode);
}

/**
 * Executes an 8 bit instruction.
 * @param opCode
 * @param ec
 */
void execInstruction8 (const int opCode, ExecutionContext* ec)
{
    if (opCode >= OC_PUSHC)
        execPushC8 (opCode, ec);
    else
    {
        //The remaining op codes are decoded with a table (there are only 64)
        s_instructions[opCode](opCode, ec);
    }
}

/**
 * Pushes a constant from the constant table on the top of the stack.
 * Constant index is encoded into the instruction. 8 bit version encodes
 * indexes [0-63]
 * 
 * @param opCode
 * @param ec
 */
void execPushC8 (const int opCode, ExecutionContext* ec)
{
    ec->push(ec->constants->at(opCode - OC_PUSHC));
}

/**
 * Pushes a constant from the constant table on the top of the stack.
 * Constant index is encoded into the instruction. 16 bit version encodes
 * indexes [64 - 8255]
 * 
 * @param opCode
 * @param ec
 */
void execPushC16 (const int opCode, ExecutionContext* ec)
{
    ec->push(ec->constants->at(opCode - (OC16_PUSHC - 64)));
}

/**
 * Executes a function call instruction, 8 bit version
 * @param opCode
 * @param ec
 */
void execCall8 (const int opCode, ExecutionContext* ec)
{
    ASSERT (opCode >= OC_CALL && opCode <= OC_CALL_MAX);
    execCall (opCode - OC_CALL, ec);
}

/**
 * Executes a function call instruction, 16 bit version
 * @param opCode
 * @param ec
 */
void execCall16 (const int opCode, ExecutionContext* ec)
{
    ASSERT (opCode >= OC16_CALL && opCode <= OC16_CALL_MAX);
    const int   nArgs = (OC_CALL_MAX - OC_CALL) + 1 + (opCode - OC16_CALL);
    
    execCall(nArgs, ec);
}

/**
 * Executes call instruction.
 * @param nArgs     Argument count (including 'this' pointer)
 * @param ec
 */
void execCall (const int nArgs, ExecutionContext* ec)
{
    if (nArgs + 1 > (int)ec->stack.size())
        error ("Stack underflow executing function call");
    
    const Ref<JSValue>  fnVal = ec->pop();
    
    if (!fnVal->isFunction())
        error ("Trying to call a non-funcion value");
    
    const Ref<JSFunction>   function = fnVal.staticCast<JSFunction>();
    
    Ref<FunctionScope>  fnScope = FunctionScope::create (ec->scopes.front(), function);
    
    //Set 'this' pointer
    size_t  i = ec->stack.size() - nArgs;
    if (nArgs > 0)
        fnScope->setThis(ec->stack[i++]);
    
    //Set parameter values in scope
    for (; i < ec->stack.size(); ++i)
        fnScope->addParam(ec->stack[i]);
    
    //Remove function parameters from the stack
    ec->stack.resize(ec->stack.size() - nArgs);
    
    ec->scopes.push_back(fnScope);

    Ref<JSValue> result;
    
    if (function->isNative())
        result = function->nativePtr()(fnScope.getPointer());
    else
    {
        const Ref<MvmScript>    code = function->getCodeMVM().staticCast<MvmScript>();
        const size_t            initialStack = ec->stack.size();
        
        result = execScript(code, ec);
        ASSERT (initialStack == ec->stack.size());
    }
    ec->scopes.pop_back();      //Remove function scope
    
    ec->push(result);
}


/**
 * Copies an element in the stack to the top of the stack
 * @param opCode
 * @param ec
 */
void execCp8 (const int opCode, ExecutionContext* ec)
{
    const int offset = opCode - OC_CP;
    
    if (offset > int(ec->stack.size())-1)
        error ("Stack underflow in CP operation");
    
    ec->push (*(ec->stack.rbegin() + offset));
}

/**
 * Exchanges the top two elements of the stack
 * @param opCode
 * @param ec
 */
void execSwap (const int opCode, ExecutionContext* ec)
{
    const Ref<JSValue>  a = ec->pop();
    const Ref<JSValue>  b = ec->pop();
    
    ec->push(a);
    ec->push(b);
}

/**
 * Discards the top element of the stack
 * @param opCode
 * @param ec
 */
void execPop (const int opCode, ExecutionContext* ec)
{
    ec->pop();
}


/**
 * Reads a local variable.
 * @param opCode
 * @param ec
 */
void execRdLocal (const int opCode, ExecutionContext* ec)
{
    const Ref<JSValue>  name = ec->pop();
    const Ref<JSValue>  val = ec->scopes.back()->get(name->toString());
    
    ec->push(val);
}

/**
 * Writes a local variable
 * @param opCode
 * @param ec
 */
void execWrLocal (const int opCode, ExecutionContext* ec)
{
    const Ref<JSValue>  val = ec->pop();
    const Ref<JSValue>  name = ec->pop();
    
    ec->scopes.back()->set(name->toString(), val, true);
}

/**
 * Reads a global variable.
 * @param opCode
 * @param ec
 */
void execRdGlobal (const int opCode, ExecutionContext* ec)
{
    const Ref<JSValue>  name = ec->pop();
    const Ref<JSValue>  val = ec->scopes.front()->get(name->toString());
    
    ec->push(val);
}

/**
 * Writes a global variable
 * @param opCode
 * @param ec
 */
void execWrGlobal (const int opCode, ExecutionContext* ec)
{
    const Ref<JSValue>  val = ec->pop();
    const Ref<JSValue>  name = ec->pop();
    
    ec->scopes.front()->set(name->toString(), val);
}


/**
 * Reads an object field.
 * @param opCode
 * @param ec
 */
void execRdField (const int opCode, ExecutionContext* ec)
{
    const Ref<JSValue>  name = ec->pop();
    const Ref<JSValue>  objVal = ec->pop();
    const Ref<JSValue>  val = objVal->memberAccess(name->toString());
    
    ec->push(val);
}

/**
 * Writes a value to an object field.
 * @param opCode
 * @param ec
 */
void execWrField (const int opCode, ExecutionContext* ec)
{
    const Ref<JSValue>  val = ec->pop();
    const Ref<JSValue>  name = ec->pop();
    const Ref<JSValue>  objVal = ec->pop();
    
    //TODO: Is silent failure appropriate?
    if (objVal->isObject())
        objVal.staticCast<JSObject>()->set (name->toString(), val);
}


/**
 * No operation. It does nothing
 * @param opCode
 * @param ec
 */
void execNop (const int opCode, ExecutionContext* ec)
{
    //This one is easy
}

/**
 * Copies current value on the top of the stack to the 'AUX' register.
 * It does not remove it from the stack, it is a copy
 * @param opCode
 * @param ec
 */
void execCpAux (const int opCode, ExecutionContext* ec)
{
    if (ec->stack.empty())
        error ("Empty stack executing OC_CP_AUX");
    
    ec->auxRegister = ec->stack.back();
}

/**
 * Pushes the current value of the 'AUX' register to the top of the stack
 * @param opCode
 * @param ec
 */
void execPushAux (const int opCode, ExecutionContext* ec)
{
    if (ec->auxRegister.notNull())
        ec->stack.push_back(ec->auxRegister);
    else
        ec->stack.push_back(undefined());
}



void invalidOp (const int opCode, ExecutionContext* ec)
{
    error ("Invalid operation code: %04X", opCode);
}

Ref<JSObject> disassemblyFunction (Ref<JSFunction> function);

/**
 * Generates a textual representation of a constant list
 * @param constants
 * @return 
 */
Ref<JSObject> constantsToJS (const ValueVector& constants)
{
    Ref<JSObject>   obj = JSObject::create();
    char            name[64];
    
    for (size_t i=0; i < constants.size(); ++i )
    {
        sprintf_s (name, "%04lu", i);
        Ref<JSValue>    value;

        if (constants[i]->isFunction())
            value = disassemblyFunction (constants[i].staticCast<JSFunction>());
        else if (constants[i]->isNull())
            value = jsNull();
        else
            value = constants[i];
        
        obj->set(name, value);
    }
    
    return obj;
}

/**
 * Disassemblies a 'push constant' operation
 * @param index
 * @param constants
 * @return 
 */
string disassemblyPushC (int index, const ValueVector& constants)
{
    ostringstream   output;

    output << "PUSHC(" << index << ") -> " << constants[index]->toString();
    //output << "PUSHC(" << index << ")";
    return output.str();
    
}

/**
 * Disassemblies an 8 bit instruction
 * @param opCode
 * @param constants
 * @return 
 */
string disassembly8bitInst (int opCode, const ValueVector& constants)
{
    if (opCode >= OC_PUSHC)
        return disassemblyPushC (opCode - OC_PUSHC, constants);
    else
    {
        switch (opCode)
        {
        case OC_CALL:   return "CALL(0)";
        case OC_CALL+1:   return "CALL(1)";
        case OC_CALL+2:   return "CALL(2)";
        case OC_CALL+3:   return "CALL(3)";
        case OC_CALL+4:   return "CALL(4)";
        case OC_CALL+5:   return "CALL(5)";
        case OC_CALL+6:   return "CALL(6)";
        case OC_CALL+7:   return "CALL(7)";
        case OC_CP:         return "CP(0)";
        case OC_CP+1:       return "CP(1)";
        case OC_CP+2:       return "CP(2)";
        case OC_CP+3:       return "CP(3)";
        case OC_SWAP:       return "SWAP";
        case OC_POP:        return "POP";
        case OC_RD_LOCAL:   return "RD_LOCAL";
        case OC_WR_LOCAL:   return "WR_LOCAL";
        case OC_RD_GLOBAL:   return "RD_GLOBAL";
        case OC_WR_GLOBAL:   return "WR_GLOBAL";
        case OC_RD_FIELD:   return "RD_FIELD";
        case OC_WR_FIELD:   return "WR_FIELD";
        case OC_CP_AUX:     return "CP_AUX";
        case OC_PUSH_AUX:   return "PUSH_AUX";
        case OC_NOP:        return "NOP";
        default:
            return "BAD_OP_CODE_8";
        }
    }
}

/**
 * Disassemblies a 16 bit instruction
 * @param opCode
 * @param constants
 * @return 
 */
string disassembly16bitInst (int opCode, const ValueVector& constants)
{
    opCode &= ~OC16_16BIT_FLAG;
    if (opCode >= OC16_PUSHC)
        return disassemblyPushC (opCode - (OC16_PUSHC - 64), constants);
    else if (opCode <= OC16_CALL_MAX)
    {
        ostringstream   output;
        
        output << "CALL(" << (opCode + OC_CALL_MAX + 1) << ")";
        return output.str();
    }
    else
        return "BAD_OP_CODE_16";
}

/**
 * Disassemblies a block of instructions
 * @param code
 * @param constants
 * @return 
 */
Ref<JSArray> disassemblyInstructions (const ByteVector& code, const ValueVector& constants)
{
    Ref<JSArray>    result = JSArray::create();
    
    for (size_t i=0; i < code.size(); ++i)
    {
        string  instructionStr;
        
        if (code[i] & OC_EXT_FLAG)
        {
            const int opCode = int(code[i]) << 8 | code[i+1];
            instructionStr = disassembly16bitInst (opCode, constants);
            ++i;
        }
        else
            instructionStr = disassembly8bitInst (code[i], constants);
        
        result->push( jsString(instructionStr) );
    }
    
    return result;
}


/**
 * Blocks disassembly
 * @param blocks
 * @return 
 */
Ref<JSObject> blocksToJS (const BlockVector& blocks, const ValueVector& constants)
{
    Ref<JSObject>   obj = JSObject::create();
    char            name[64];
    
    for (size_t i=0; i < blocks.size(); ++i )
    {
        sprintf_s (name, "Block%04lu", i);
        Ref<JSObject>   blockObj = JSObject::create();

        obj->set(name, blockObj);
        
        blockObj->set("a_nextTrue", jsInt(blocks[i].nextBlocks[1]));
        blockObj->set("a_nextFalse", jsInt(blocks[i].nextBlocks[0]));
        blockObj->set("instructions", disassemblyInstructions (blocks[i].instructions, constants));
        
    }
    
    return obj;
}


/**
 * Generates a human-readable representation of the input script.
 * @param code
 * @return 
 */
string mvmDisassembly (Ref<MvmScript> code)
{
    return toJSObject(code)->getJSON(0);
}

/**
 * Transforms a compiled scirpt into a JSObject.
 * It can be used later from JAvascript code or written to a JSOn file.
 * @param code
 * @return 
 */
Ref<JSObject> toJSObject (Ref<MvmScript> code)
{
    Ref<JSObject>   obj = JSObject::create();
    
    obj->set ("a_constants", constantsToJS(code->constants));
    obj->set ("b_blocks", blocksToJS(code->blocks, code->constants));
    
    return obj;
}


/**
 * Generates an human readable representation of a function
 * @param function
 * @return 
 */
Ref<JSObject> disassemblyFunction (Ref<JSFunction> function)
{
    Ref<JSObject>   obj = JSObject::create();

    obj->set ("a_header", jsString(function->toString() ));
    if (function->isNative())
        obj->set("code", jsString("native"));
    else
        obj->set("code", toJSObject(function->getCodeMVM().staticCast<MvmScript>()));

    return obj;
}
