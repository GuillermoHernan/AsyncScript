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
#include "asActors.h"
#include "executionScope.h"
#include "ScriptException.h"

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

int execBlock (const MvmBlock& block, ExecutionContext* ec);
void execInstruction16 (const int opCode, ExecutionContext* ec);
void execInstruction8 (const int opCode, ExecutionContext* ec);
void execPushC8 (const int opCode, ExecutionContext* ec);
void execPushC16 (const int opCode, ExecutionContext* ec);
void execCall8 (const int opCode, ExecutionContext* ec);
void execCall16 (const int opCode, ExecutionContext* ec);
void execCall (const int nArgs, ExecutionContext* ec);
void callLog (Ref<FunctionScope> fnScope, ExecutionContext* ec);
void returnLog (Ref<FunctionScope> fnScope, Ref<JSValue> result, ExecutionContext* ec);
void execCp8 (const int opCode, ExecutionContext* ec);
void execSwap (const int opCode, ExecutionContext* ec);
void execPop (const int opCode, ExecutionContext* ec);
void execPushScope (const int opCode, ExecutionContext* ec);
void execPopScope (const int opCode, ExecutionContext* ec);
void execRdLocal (const int opCode, ExecutionContext* ec);
void execWrLocal (const int opCode, ExecutionContext* ec);
void execRdGlobal (const int opCode, ExecutionContext* ec);
void execWrGlobal (const int opCode, ExecutionContext* ec);
void execRdField (const int opCode, ExecutionContext* ec);
void execWrField (const int opCode, ExecutionContext* ec);
void execRdIndex (const int opCode, ExecutionContext* ec);
void execWrIndex (const int opCode, ExecutionContext* ec);
void execNewVar (const int opCode, ExecutionContext* ec);
void execNewConst (const int opCode, ExecutionContext* ec);
void execNewConstField (const int opCode, ExecutionContext* ec);
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
    execSwap,       execPop,        execPushScope,  execPopScope,
    
    //16
    execRdLocal,    execWrLocal,    execRdGlobal,   execWrGlobal,
    execRdField,    execWrField,    execRdIndex,    execWrIndex,
    
    //24
    execNewVar,     execNewConst,   execNewConstField, invalidOp,
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
 * @param locals
 * @param callHook
 * @return 
 */
Ref<JSValue> mvmExecute (Ref<MvmRoutine> code, 
                         Ref<IScope> globals, 
                         Ref<IScope> locals)
{
    ExecutionContext    ec;
    GlobalsSetter       g(globals);
    
    ec.scopes.push_back(globals);
    if (locals.notNull())
        ec.scopes.push_back(locals);
    
    return mvmExecRoutine (code, &ec);
}

/**
 * Executes a Micro VM routine
 *
 * @param code
 * @param ec        Execution context
 * @return 
 */
Ref<JSValue> mvmExecRoutine (Ref<MvmRoutine> code, ExecutionContext* ec)
{
    if (code->blocks.empty())
        return jsNull();
    
    ValueVector*    prevConstants = ec->constants;
    const size_t    scopesStackLen = ec->scopes.size();
    
    int nextBlock = 0;
    
    //Set constants
    ec->constants = &code->constants;
    
    while (nextBlock >= 0)
    {
        nextBlock = execBlock (code->blocks[nextBlock], ec);
    }
    
    //Scope stack unwind.
    ASSERT (ec->scopes.size() >= scopesStackLen);
    ec->scopes.resize(scopesStackLen);
    
    ec->constants = prevConstants;
    
    //'AUX' register is cleared when finishing a script, to prevent memory leaks
    //(and may be also a good security measure)
    ec->auxRegister = jsNull();
    
    if (ec->stack.empty())
        return jsNull();
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
    const int decoded = opCode & 0x3FFF;
    
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
    
//    if (!fnVal->isFunction())
//        error ("Trying to call a non-function value");
    
//    const Ref<JSFunction>   function = fnVal.staticCast<JSFunction>();

    Ref<JSValue> thisObj = jsNull();
    
    //Set 'this' pointer
    size_t  i = ec->stack.size() - nArgs;
    if (nArgs > 0)
        thisObj = ec->stack[i++];
    
    ValueVector params(ec->stack.begin()+i, ec->stack.end());
    Ref<FunctionScope>  fnScope = FunctionScope::create (fnVal, thisObj, params);
    
    //Remove function parameters from the stack
    ec->stack.resize(ec->stack.size() - nArgs);
    
    callLog (fnScope, ec);
    
    ec->scopes.push_back(fnScope);

    const size_t            initialStack = ec->stack.size();
    
    Ref<JSValue> result = fnVal->call(fnScope);
    
    ASSERT (initialStack == ec->stack.size());
    ec->scopes.pop_back();      //Remove function scope
    
    ec->push(result);

    returnLog(fnScope, result, ec);
}

/**
 * This is the default function for handling function calls. It can be overriden 
 * by client code using 'callHook' parameter of 'mvmExecute'
 * @param function      Function reference.
 * @param scope         Function scope object
 * @param ec            Execution context object
 * @return 
 */
//Ref<JSValue> defaultCallHook (  Ref<JSValue> fnVal, 
//                                Ref<FunctionScope> scope, 
//                                ExecutionContext* ec, 
//                                void* defaultHook)
//{
//    if (fnVal->getType() != VT_FUNCTION)
//        error ("Micro VM default call handler only supports functions. Received: %s", 
//               fnVal->toString().c_str());
//    
//    auto function = fnVal.staticCast<JSFunction>();
//    
//    if (function->isNative())
//        return function->nativePtr()(scope.getPointer());
//    else
//    {
//        const Ref<MvmRoutine>    code = function->getCodeMVM().staticCast<MvmRoutine>();
//        
//        return mvmExecRoutine(code, ec);
//    }
//}

static const char *CALL_LOG_DEPTH = "@callLogDepth";

/**
 * Logs function calls, if enabled.
 * @param fnScope
 */
void callLog (Ref<FunctionScope> fnScope, ExecutionContext* ec)
{
    const auto globals = getGlobals();
    
    if (!globals->isDefined("callLogger"))
        return;
    
    const auto logFunction = globals->get("callLogger");
    
    if (logFunction->isNull())
        return;
    
    //Re-entry guard
    if (logFunction == fnScope->getFunction())
        return;
    
    Ref<JSValue> depth;
    
    if (!globals->isDefined(CALL_LOG_DEPTH))
    {
        depth = jsInt(1);
        globals->newVar(CALL_LOG_DEPTH, depth, false);
    }
    else
    {
        depth = globals->get(CALL_LOG_DEPTH);
        const int iDepth = max (1, toInt32( depth ) + 1);
        depth = jsInt(iDepth);
        globals->set(CALL_LOG_DEPTH, depth);
    }
    
    auto obj = JSObject::create();
    obj->writeField("level", depth, false);
    obj->writeField("name", jsString(fnScope->getFunction()->getName()), false);
    obj->writeField("params", fnScope->get("arguments"), false);
    obj->writeField("this", fnScope->getThis(), false);
    
    ec->push(jsNull());     //this
    ec->push(obj);          //Log entry
    ec->push(logFunction);
    
    execCall(2, ec);  
    ec->pop();              //Discard result.
}

/**
 * Logs function return, if enabled.
 * @param fnScope
 * @param result
 * @param ec
 */
void returnLog (Ref<FunctionScope> fnScope, Ref<JSValue> result, ExecutionContext* ec)
{
    const auto globals = getGlobals();

    if (!globals->isDefined("callLogger"))
        return;

    const auto logFunction = globals->get("callLogger");
    
    if (!logFunction->isFunction())
        return;
    
    //Re-entry guard
    if (logFunction == fnScope->getFunction())
        return;
    
    if (!globals->isDefined(CALL_LOG_DEPTH))
        return;
    
    auto depth = globals->get(CALL_LOG_DEPTH);
    
    if (depth->getType() != VT_NUMBER)
        depth = jsInt(0);
    
    if (toInt32 (depth) <= 0)
        globals->set("callLogger", jsNull());    //Remove call logger
    else
    {
        globals->set(CALL_LOG_DEPTH, jsInt(toInt32( depth )-1));
    
        auto obj = JSObject::create();
        obj->writeField("level", depth, false);
        obj->writeField("name", jsString(fnScope->getFunction()->getName()), false);
        obj->writeField("result", result, false);

        ec->push(jsNull());     //this
        ec->push(obj);          //log entry
        ec->push(logFunction);

        execCall(2, ec);    
        ec->pop();              //Discard result.
    }
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
 * Creates a new 'Block scope'
 * @param opCode
 * @param ec
 */
void execPushScope (const int opCode, ExecutionContext* ec)
{
    if (ec->scopes.empty())
        error("Empty scope stack executing 'PUSH SCOPE");
    
    auto scope = BlockScope::create(ec->scopes.back());
    ec->scopes.push_back(scope);
}

/**
 * Removes current scope from the scope stack
 * @param opCode
 * @param ec
 */
void execPopScope (const int opCode, ExecutionContext* ec)
{
    if (ec->scopes.empty())
        error("Empty scope stack executing 'POP SCOPE");
    
    if (!ec->scopes.back()->isBlockScope())
        error("'POP SCOPE' trying to remove a non-block scope");
    
    ec->scopes.pop_back();
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
    
    ec->scopes.back()->set(name->toString(), val);
}

/**
 * Reads a global variable.
 * @param opCode
 * @param ec
 */
void execRdGlobal (const int opCode, ExecutionContext* ec)
{
    //auto globals = ec->scopes.front();
    
    const Ref<JSValue>  name = ec->pop();
    const Ref<JSValue>  val = getGlobals()->get(name->toString());
    
    ec->push(val);
}

/**
 * Writes a global variable
 * @param opCode
 * @param ec
 */
void execWrGlobal (const int opCode, ExecutionContext* ec)
{
    //auto globals = ec->scopes.front();
    const Ref<JSValue>  val = ec->pop();
    const Ref<JSValue>  name = ec->pop();
    
    getGlobals()->set(name->toString(), val);
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
    const Ref<JSValue>  val = objVal->readField(name->toString());
    
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
    
    objVal->writeField (name->toString(), val, false);
}

/**
 * Executes an 'indexed read' operation.
 * @param opCode
 * @param ec
 */
void execRdIndex (const int opCode, ExecutionContext* ec)
{
    const Ref<JSValue>  key = ec->pop();
    const Ref<JSValue>  container = ec->pop();
    const Ref<JSValue>  val = container->indexedRead(key);
    
    ec->push(val);
}

/**
 * Executes an 'indexed write operation
 * @param opCode
 * @param ec
 */
void execWrIndex (const int opCode, ExecutionContext* ec)
{
    const Ref<JSValue>  val = ec->pop();
    const Ref<JSValue>  key = ec->pop();
    const Ref<JSValue>  container = ec->pop();
    
    container->indexedWrite(key, val);    
}

/**
 * Variable creation instruction
 * @param opCode
 * @param ec
 */
void execNewVar (const int opCode, ExecutionContext* ec)
{
    if (ec->scopes.empty())
        error("Empty scope stack executing 'NEW_VAR'");

    const auto  val = ec->pop();
    const auto  name = ec->pop();
    
    ec->scopes.back()->newVar (name->toString(), val, false);
}

/**
 * Constant creation instruction
 * @param opCode
 * @param ec
 */
void execNewConst (const int opCode, ExecutionContext* ec)
{
    if (ec->scopes.empty())
        error("Empty scope stack executing 'NEW_CONST'");

    const auto  val = ec->pop();
    const auto  name = ec->pop();
    
    ec->scopes.back()->newVar (name->toString(), val, true);
}


/**
 * Creates a new constant field in an object. Very similar to 'WR_FIELD', but the
 * written field won't be modifiable.
 * @param opCode
 * @param ec
 */
void execNewConstField (const int opCode, ExecutionContext* ec)
{
    const Ref<JSValue>  val = ec->pop();
    const Ref<JSValue>  name = ec->pop();
    const Ref<JSValue>  objVal = ec->pop();
    
    objVal->writeField (name->toString(), val, true);
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
    //TODO: Review if this check is really necessary.
    if (ec->auxRegister.notNull())
        ec->stack.push_back(ec->auxRegister);
    else
        ec->stack.push_back(jsNull());
}



void invalidOp (const int opCode, ExecutionContext* ec)
{
    error ("Invalid operation code: %04X", opCode);
}
