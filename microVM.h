/* 
 * File:   microVM.h
 * Author: ghernan
 *
 * Minimalist Virtual Machine for Javascript
 * 
 * A virtual machine with a very small instruction set.
 * Created on December 2, 2016, 9:18 PM
 */

#ifndef MICROVM_H
#define	MICROVM_H
#pragma once

#include "JsVars.h"
#include <vector>
#include <string>

struct MvmScript;

Ref<JSValue>    mvmExecute (Ref<MvmScript> code, Ref<IScope> globals);
std::string     mvmDisassembly (Ref<MvmScript> code);

/**
 * 8 bit instruction codes.
 */
enum OpCodes8
{
    OC_CALL = 0,
    OC_CALL_MAX = 7,
    OC_CP = 8,
    OC_CP_MAX = 11,
    OC_SWAP = 12,
    OC_POP = 13,
    
    //OC_RETURN = 15,   //No return instruction, by the moment
    OC_RD_LOCAL = 16,
    OC_WR_LOCAL = 17,
    OC_RD_GLOBAL = 18,
    OC_WR_GLOBAL = 19,
    OC_RD_FIELD = 20,
    OC_WR_FIELD = 21,
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
    OC16_PUSHC = 0x2000,
    OC16_32BIT_FLAG = 0x4000,   //Reserved for future extension to 32 bit instructions.
    OC16_16BIT_FLAG = 0x8000    //Always active for 16 bit instructions
};

typedef std::vector<unsigned char>      ByteVector;
typedef std::vector<Ref<JSValue> >      ValueVector;

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

class MvmScript : public RefCountObj
{
public:
    static Ref<MvmScript> create(const ScriptPosition& pos)
    {
        return refFromNew(new MvmScript(pos));
    }

    const ScriptPosition position;    
    
    ValueVector constants;
    BlockVector blocks;
    
protected:
    MvmScript(const ScriptPosition& pos):position (pos)    
    {
        blocks.push_back(MvmBlock());
    }
};
#endif	/* MICROVM_H */

