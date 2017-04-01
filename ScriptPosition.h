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
    Ref<RefCountObj>  Routine;
    int               Block;
    int               Instruction;
    
    VmPosition () : Block(-1), Instruction(-1)
    {
    }
    
    VmPosition (Ref<RefCountObj> _routine, int _block, int _instruction) :
    Routine(_routine), Block(_block), Instruction(_instruction)
    {
    }
    
    bool operator < (const VmPosition& b)const
    {
        if (Routine == b.Routine)
        {
            if (Block == b.Block)
                return Instruction < b.Instruction;
            else
                return Block < b.Block;
        }
        else
            return Routine < b.Routine;
    }

    
    bool operator == (const VmPosition& b)const
    {
        return (Routine < b.Routine &&
            Block == b.Block &&
            Instruction == b.Instruction);
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

/**
 * Maps 'VmPositions' to 'ScriptPositions'. Used to give the source location
 * of run time errors.
 */
class CodeMap
{
public:
    const ScriptPosition& get(const VmPosition& vmPos)const;
    bool add (const VmPosition& vmPos, const ScriptPosition& scPos);
    
private:
    typedef std::map<VmPosition, ScriptPosition>    VM2SCmap;
    typedef std::map<ScriptPosition, VmPosition>    SC2VMmap;
    
    VM2SCmap    m_vm2sc;
    SC2VMmap    m_sc2vm;
};

#endif	/* SCRIPTPOSITION_H */

