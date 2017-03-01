/* 
 * File:   ScriptPosition.h
 * Author: ghernan
 * 
 * Classes to handle source and object code positions.
 *
 * Created on March 1, 2017, 7:58 PM
 */

#ifndef SCRIPTPOSITION_H
#define	SCRIPTPOSITION_H

#pragma once

#include "RefCountObj.h"
#include <string>

class MvmRoutine;

/**
 * Indicates a position inside a script file
 */
struct ScriptPosition
{
    int line;
    int column;

    ScriptPosition() :
    line(-1), column(-1)
    {
    }

    ScriptPosition(int l, int c) :
    line(l), column(c)
    {
    }

    std::string toString()const;
    
    bool operator < (const ScriptPosition& b)const
    {
        if (line < b.line)
            return true;
        else if (line == b.line)
            return column < b.column;
        else
            return false;
    }
    
    bool operator == (const ScriptPosition& b)const
    {
        return (line < b.line && column == b.column);
    }
    
    bool operator > (const ScriptPosition& b)const
    {
        return !(*this <= b);
    }
    
    bool operator <= (const ScriptPosition& b)const
    {
        return (*this < b || *this == b);
    }
    
    bool operator >= (const ScriptPosition& b)const
    {
        return !(*this < b);
    }
};//struct ScriptPosition

/**
 * Describes the position of an instruction inside Micro VM object code.
 */
struct VmPosition
{
    Ref<MvmRoutine>     routine;
    int                 block;
    int                 instruction;
    
    bool operator < (const VmPosition& b)const
    {
        if (routine == b.routine)
        {
            if (block == b.block)
                return instruction < b.instruction;
            else
                return block < b.block;
        }
        else
            return routine < b.routine;
    }

    
    bool operator == (const VmPosition& b)const
    {
        return (routine < b.routine &&
            block == b.block &&
            instruction == b.instruction);
    }
    
    bool operator > (const VmPosition& b)const
    {
        return !(*this <= b);
    }
    
    bool operator <= (const VmPosition& b)const
    {
        return (*this < b || *this == b);
    }
    
    bool operator >= (const VmPosition& b)const
    {
        return !(*this < b);
    }
};//struct VmPosition

#endif	/* SCRIPTPOSITION_H */

