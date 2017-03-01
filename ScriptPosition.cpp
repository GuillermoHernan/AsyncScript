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

