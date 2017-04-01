/* 
 * File:   mvmDisassembly.cpp
 * Author: ghernan
 * 
 * Micro virtual machine disassembly functions
 * 
 * Created on December 29, 2016, 14:20 PM
 */

#include "ascript_pch.hpp"
#include "microVM.h"
#include "asActors.h"

using namespace std;

Ref<JSObject> disassemblyFunction (Ref<JSFunction> function);
Ref<JSObject> disassemblyActorClass (Ref<AsActorClass> actorClass);
Ref<JSObject> disassemblyInputEndPoint (Ref<AsEndPoint> ep);
Ref<JSObject> disassemblyOutputEndPoint (Ref<AsEndPoint> ep);
Ref<JSObject> disassemblyClass (Ref<JSClass> cls);
Ref<JSObject> disassemblyMembers (Ref<JSValue> container);

/**
 * Generates a Javascript object representation of a single constant
 * @param constants
 * @return 
 */
Ref<JSValue> constantToJS (Ref<JSValue> constant)
{
    switch (constant->getType())
    {
    case VT_ACTOR_CLASS:
        return disassemblyActorClass (constant.staticCast<AsActorClass>());
        
    case VT_CLASS:
        return disassemblyClass (constant.staticCast<JSClass>());
    
    case VT_INPUT_EP:
        return disassemblyInputEndPoint (constant.staticCast<AsEndPoint>());
    
    case VT_OUTPUT_EP:
        return disassemblyOutputEndPoint (constant.staticCast<AsEndPoint>());
    
    default:
        if (constant->isFunction())
            return disassemblyFunction (constant.staticCast<JSFunction>());
        else
            return constant;
    }//switch
}

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
        Ref<JSValue>    value = constantToJS(constants[i]);
        
        obj->writeField(name, value, false);
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
        case OC_CP+4:       return "CP(4)";
        case OC_CP+5:       return "CP(5)";
        case OC_CP+6:       return "CP(6)";
        case OC_CP+7:       return "CP(7)";
        case OC_WR+1:       return "WR(1)";
        case OC_WR+2:       return "WR(2)";
        case OC_WR+3:       return "WR(3)";
        case OC_WR+4:       return "WR(4)";
        case OC_WR+5:       return "WR(5)";
        case OC_WR+6:       return "WR(6)";
        case OC_WR+7:       return "WR(7)";
        case OC_SWAP:       return "SWAP";
        case OC_POP:        return "POP";
//        case OC_PUSH_SCOPE: return "PUSH_SCOPE";
//        case OC_POP_SCOPE:  return "POP_SCOPE";
//        case OC_RD_LOCAL:   return "RD_LOCAL";
//        case OC_WR_LOCAL:   return "WR_LOCAL";
//        case OC_RD_GLOBAL:      return "RD_GLOBAL";
//        case OC_WR_GLOBAL:      return "WR_GLOBAL";
        case OC_RD_FIELD:       return "RD_FIELD";
        case OC_WR_FIELD:       return "WR_FIELD";
        case OC_RD_INDEX:       return "RD_INDEX";
        case OC_WR_INDEX:       return "WR_INDEX";
//        case OC_NEW_VAR:        return "NEW_VAR";
//        case OC_NEW_CONST:      return "NEW_CONST";
        case OC_NEW_CONST_FIELD:return "NEW_CONST_FIELD";
//        case OC_CP_AUX:         return "CP_AUX";
//        case OC_PUSH_AUX:       return "PUSH_AUX";
        case OC_NOP:            return "NOP";
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
    else if (opCode <= OC16_CP_MAX)
    {
        ostringstream   output;
        
        output << "CP(" << ((opCode-OC16_CP) + OC_CP_MAX + 1) << ")";
        return output.str();
    }
    else if (opCode <= OC16_WR_MAX)
    {
        ostringstream   output;
        
        output << "WR(" << ((opCode-OC16_WR) + OC_WR_MAX + 1) << ")";
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
Ref<JSObject> disassemblyInstructions (const ByteVector& code, 
                                       const ValueVector& constants)
{
    auto    result = JSObject::create();
    char    position[64];
    
    for (size_t i=0; i < code.size(); ++i)
    {
        string  instructionStr;
        
        sprintf_s (position, "%04u", (unsigned)i);
        
        if (code[i] & OC_EXT_FLAG)
        {
            const int opCode = int(code[i]) << 8 | code[i+1];
            instructionStr = disassembly16bitInst (opCode, constants);
            ++i;
        }
        else
            instructionStr = disassembly8bitInst (code[i], constants);
        
        result->writeField(position, jsString(instructionStr), false);
    }
    
    return result;
}


/**
 * Blocks disassembly
 * @param blocks
 * @return 
 */
Ref<JSObject> blocksToJS (const BlockVector& blocks, 
                          const ValueVector& constants)
{
    Ref<JSObject>   obj = JSObject::create();
    char            name[64];
    
    for (size_t i=0; i < blocks.size(); ++i )
    {
        sprintf_s (name, "Block%04lu", i);
        Ref<JSObject>   blockObj = JSObject::create();

        obj->writeField(name, blockObj, false);
        
        blockObj->writeField("nextTrue", jsInt(blocks[i].nextBlocks[1]), false);
        blockObj->writeField("nextFalse", jsInt(blocks[i].nextBlocks[0]), false);
        blockObj->writeField("instructions", 
                             disassemblyInstructions (blocks[i].instructions, constants), 
                             false);
        
    }
    
    return obj;
}


/**
 * Generates a human-readable representation of the input script.
 * @param code
 * @return 
 */
string mvmDisassembly (Ref<MvmRoutine> code)
{
    return toJSObject(code)->getJSON(0);
}

/**
 * Transforms a compiled scirpt into a JSObject.
 * It can be used later from JAvascript code or written to a JSOn file.
 * @param code
 * @return 
 */
Ref<JSObject> toJSObject (Ref<MvmRoutine> code)
{
    Ref<JSObject>   obj = JSObject::create();
    
    obj->writeField ("constants", constantsToJS(code->constants), false);
    obj->writeField ("blocks", blocksToJS(code->blocks, code->constants), false);
    
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

    obj->writeField ("header", jsString(function->toString() ), false);
    if (function->isNative())
        obj->writeField("code", jsString("native"), false);
    else
        obj->writeField("code", toJSObject(function->getCodeMVM().staticCast<MvmRoutine>()), false);

    return obj;
}

/**
 * Transforms an actor class into a Javascript object which can be serialized.
 * @param actorClass
 * @return 
 */
Ref<JSObject> disassemblyActorClass (Ref<AsActorClass> actorClass)
{
    Ref<JSObject>   obj = JSObject::create();

    obj->writeField ("actorClass", jsString(actorClass->getName()), false );
    
    auto members = disassemblyMembers(actorClass);
    obj->writeField ("members", members, false);

    return obj;
}

/**
 * Transforms an input endpoint to a Javascript object representation
 * @param ep
 * @return 
 */
Ref<JSObject> disassemblyInputEndPoint (Ref<AsEndPoint> ep)
{
    return disassemblyFunction(ep);
}

/**
 * Transforms an output endpoint to a Javascript object representation
 * @param ep
 * @return 
 */
Ref<JSObject> disassemblyOutputEndPoint (Ref<AsEndPoint> ep)
{
    auto result = JSObject::create();
    result->writeField ("header", jsString(ep->toString() ), false);
    return result;
}

/**
 * Disassemblies a class declaration
 * @param cls
 * @return 
 */
Ref<JSObject> disassemblyClass (Ref<JSClass> cls)
{
    auto result = JSObject::create();
    result->writeField ("Class", jsString(cls->getName()), false);
    
    auto    parent = cls->getParent();
    auto    constructor = constantToJS(cls->getConstructor());
    string  parentName;
    
    if (parent.notNull())
        parentName = parent->getName();
    
    result->writeField ("Parent", jsString(parentName), false);
    result->writeField("constructor", constructor, false);
    
    auto members = disassemblyMembers(cls);
    
    result->writeField("members", members, false);
    
    return result;
}

/**
 * Gets the disassembly representation of the members of a value
 * @param container
 * @return 
 */
Ref<JSObject> disassemblyMembers (Ref<JSValue> container)
{
    auto memberNames = container->getFields(false);
    auto members = JSObject::create();
    
    for (auto it = memberNames.begin(); it != memberNames.end(); ++it)
    {
        auto member = container->readField(*it);
        members->writeField(*it, constantToJS(member), false);
    }
    
    return members;
}
