/* 
 * File:   ScriptPosition.cpp
 * Author: ghernan
 * 
 * Classes to handle source and object code positions.
 * 
 * Created on March 1, 2017, 7:58 PM
 */

#include "ascript_pch.hpp"
#include "ScriptPosition.h"

using namespace std;

/**
 * String representation of a Script position.
 * @return 
 */
string ScriptPosition::toString()const
{
    char buffer [128];

    sprintf_s(buffer, "(line: %d, col: %d): ", this->line, this->column);
    return string(buffer);
}

/**
 * Gets an script position from an WM position
 * @param vmPos
 * @return 
 */
const ScriptPosition& CodeMap::get(const VmPosition& vmPos)
{
    const static ScriptPosition nullPosition;
    
    if (m_vm2sc.empty())
        return nullPosition;
    else
    {
        auto it = m_vm2sc.upper_bound(vmPos);
        
        if (it != m_vm2sc.begin())
            --it;
        return it->second;
    }
}

/**
 * Adds a new entry to the map.
 * @param vmPos
 * @param scPos
 * @return 
 */
bool CodeMap::add (const VmPosition& vmPos, const ScriptPosition& scPos)
{
    auto it = m_sc2vm.find(scPos);
    
    if (it == m_sc2vm.end() || vmPos < it->second)
    {
        m_sc2vm[scPos] = vmPos;
        m_vm2sc[vmPos] = scPos;
        return true;
    }
    else 
        return false;
}
