/* 
 * File:   microVM.h
 * Author: ghernan
 *
 * Minimalist Virtual Machine for AsyncScript
 * 
 * A virtual machine with a very small instruction set.
 * Created on December 2, 2016, 9:18 PM
 */

#ifndef MICROVM_H
#define	MICROVM_H
#pragma once

#include "jsVars.h"
#include <vector>
#include <string>

struct MvmRoutine;
struct ExecutionContext;

typedef std::vector<unsigned char>      ByteVector;

ASValue         mvmExecRoutine (Ref<MvmRoutine> code, ExecutionContext* ec, int nParams);
void            mvmExecCall (int nArgs, ExecutionContext* ec);
std::string     mvmDisassembly (Ref<MvmRoutine> code);
std::string     mvmDisassemblyInstruction (int opCode, const ValueVector& constants);
Ref<JSObject>   toJSObject (Ref<MvmRoutine> code);

/**
 * 8 bit instruction codes.
 */
enum OpCodes8
{
    OC_CALL = 0,
    OC_CALL_MAX = 7,
    OC_CP = 8,
    OC_CP_MAX = 15,
    OC_WR = 16,
    OC_WR_MAX = 23,
    OC_SWAP = 24,
    OC_POP = 25,

    OC_RD_FIELD = 26,
    OC_WR_FIELD = 27,
    OC_RD_INDEX = 28,
    OC_WR_INDEX = 29,
    OC_NEW_CONST_FIELD = 30,

    OC_RD_PARAM = 32,
    OC_WR_PARAM = 33,
    OC_NUM_PARAMS = 34,
    OC_PUSH_THIS = 35,
    OC_WR_THISP = 36,
    OC_NOP = 63,
    OC_PUSHC = 64,
    OC_EXT_FLAG = 128
};

/**
 * 16 bit instruction codes.
 */
enum OpCodes16
{
    OC16_CALL = 0,
    OC16_CALL_MAX = 0x03ff,     //1023
    OC16_CP = 0x400,
    OC16_CP_MAX = 0x7ff,
    OC16_WR = 0x800,
    OC16_WR_MAX = 0xbff,
    OC16_PUSHC = 0x2000,
    OC16_32BIT_FLAG = 0x4000,   //Reserved for future extension to 32 bit instructions.
    OC16_16BIT_FLAG = 0x8000    //Always active for 16 bit instructions
};

struct MvmBlock
{
    int         nextBlocks[2];
    ByteVector  instructions;
    
    MvmBlock (int trueJump, int falseJump)
    {
        nextBlocks[1] = trueJump;
        nextBlocks[0] = falseJump;
    }
    
    MvmBlock ()
    {
        nextBlocks[1] = -1;
        nextBlocks[0] = -1;
    }
};

typedef std::vector<MvmBlock>  BlockVector;

class MvmRoutine : public RefCountObj
{
public:
    static Ref<MvmRoutine> create()
    {
        return refFromNew(new MvmRoutine);
    }

    ValueVector constants;
    BlockVector blocks;
    
protected:
    MvmRoutine()   
    {
        blocks.push_back(MvmBlock());
    }
};

/**
 * Structure which contains the information needed for an executed function.
 */
struct CallFrame
{
    ValueVector*    constants = NULL;
    size_t          paramsIndex;
    size_t          numParams;
    ASValue         thisValue;
    
    CallFrame (ValueVector* consts, size_t paramsIdx, size_t nParams, ASValue thisVal)
    : constants(consts), paramsIndex(paramsIdx), numParams(nParams), thisValue(thisVal)
    {}
    
};
typedef std::vector <CallFrame> FrameVector;

typedef void (*TraceFN)(int opCode, const ExecutionContext* ec);
struct Modules;

/**
 * MVM execution context
 */
struct ExecutionContext
{
    ValueVector         stack;
    FrameVector         frames;
    const std::string   modulePath;
    TraceFN             trace = NULL;
    Modules* const      modules;

private:
    ASValue             thisParam;
public:
    
    ExecutionContext (const std::string& _modulePath, Modules* pModules)
        : modulePath(_modulePath), modules(pModules)
    {
    }
    
    ASValue pop()
    {
        checkStackNotEmpty();
        
        ASValue    r = stack.back();
        stack.pop_back();
        return r;
    }
    
    ASValue push(ASValue value)
    {
        stack.push_back(value);
        return value;
    }
    
    ASValue getThisParam ()
    {
        ASValue temp = thisParam;
        thisParam = jsNull();
        return temp;
    }
    
    void setThisParam(ASValue val)
    {
        thisParam = val;
    }
    
    bool checkStackNotEmpty();
    
    ASValue getConstant (size_t index)const;
    
    ASValue getParam (size_t index)const;
    ASValue getThis ()const;
    size_t getNumParams()const;

};

#endif	/* MICROVM_H */

