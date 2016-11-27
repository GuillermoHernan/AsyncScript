/* 
 * File:   jsOperators.cpp
 * Author: ghernan
 * 
 * Created on November 26, 2016, 8:37 AM
 */

#include "jsOperators.h"
#include "OS_support.h"

#include <math.h>
#include <string>

using namespace std;

// Operations functions forward declarations.
Ref<JSValue> addOp(Ref<JSValue> opA, Ref<JSValue> opB);
Ref<JSValue> mathOp(int type, Ref<JSValue> opA, Ref<JSValue> opB);
Ref<JSValue> bitOp(int type, Ref<JSValue> opA, Ref<JSValue> opB);
Ref<JSValue> shiftOp(int type, Ref<JSValue> opA, Ref<JSValue> opB);
Ref<JSValue> logicOp(int type, Ref<JSValue> opA, Ref<JSValue> opB);
Ref<JSValue> compareOp(int type, Ref<JSValue> opA, Ref<JSValue> opB);
bool compareStrings(int type, const string& opA, const string& opB);
bool compareNumbers(int type, const double opA, const double opB);
bool compareUndefined(int opType, JSValueTypes typeA, JSValueTypes typeB);

/**
 * Evaluates any Javascript binary operator
 * @param type
 * @param opA
 * @param opB
 * @return 
 */
Ref<JSValue> jsOperator(int type, Ref<JSValue> opA, Ref<JSValue> opB)
{
    switch (type)
    {
    case '+':
        return addOp(opA, opB);
    case '-':
    case '*':
    case '/':
    case '%':
        return mathOp(type, opA, opB);

    case '&':
    case '|':
    case '^':
        return bitOp(type, opA, opB);

    case LEX_LSHIFT:
    case LEX_RSHIFT:
    case LEX_RSHIFTUNSIGNED:
        return shiftOp(type, opA, opB);

    case LEX_ANDAND:
    case LEX_OROR:
        return logicOp(type, opA, opB);

    case '<':
    case '>':
    case LEX_EQUAL:
    case LEX_TYPEEQUAL:
    case LEX_NEQUAL:
    case LEX_NTYPEEQUAL:
    case LEX_LEQUAL:
    case LEX_GEQUAL:
        return compareOp(type, opA, opB);

    default:
        error("Invalid operator: '%c'", type);
        return undefined();
    }//switch
}

/**
 * Javascript addition operation.
 * It can add numbers, or concatenate strings
 * @param opA
 * @param opB
 * @return 
 */
Ref<JSValue> addOp(Ref<JSValue> opA, Ref<JSValue> opB)
{
    const JSValueTypes typeA = opA->getType();
    const JSValueTypes typeB = opB->getType();

    if (typeA >= VT_STRING || typeB >= VT_STRING)
        return jsString(opA->toString() + opB->toString());
    else
        return jsDouble(opA->toDouble() + opB->toDouble());
}

/**
 * Performs generic math operations
 * All these operations try to transform operands into numbers, and then execute
 * the requested math operations
 * @param type
 * @param opA
 * @param opB
 * @return 
 */
Ref<JSValue> mathOp(int type, Ref<JSValue> opA, Ref<JSValue> opB)
{
    //TODO: Review the semantics of 'toDouble' of each type
    const double valA = opA->toDouble();
    const double valB = opB->toDouble();

    switch (type)
    {
    case '-': return jsDouble(valA - valB);
    case '*': return jsDouble(valA * valB);
    case '/': return jsDouble(valA / valB);
    case '%': return jsDouble(fmod(valA, valB));
    default:
        ASSERT(!"Unexpected operator in 'mathOp'");
        return jsDouble(getNaN());
    }//switch

}

/**
 * Performs Javascript bitwise operations.
 * First tries to transform operands into 32 bits integers, and then performs the operation.
 * @param type
 * @param opA
 * @param opB
 * @return 
 */
Ref<JSValue> bitOp(int type, Ref<JSValue> opA, Ref<JSValue> opB)
{
    //TODO: Review the semantics of 'toInt32' of each type
    const int valA = opA->toInt32();
    const int valB = opB->toInt32();

    switch (type)
    {
    case '&': return jsInt(valA & valB);
    case '|': return jsInt(valA | valB);
    case '^': return jsInt(valA ^ valB);
    default:
        ASSERT(!"Unexpected operator in 'bitOp'");
        return jsDouble(getNaN());
    }
}

/**
 * Performs Javascript bitwise shift operations.
 * First tries to transform operands into 32 bits integers, and then performs the operation.
 * @param type
 * @param opA
 * @param opB
 * @return 
 */
Ref<JSValue> shiftOp(int type, Ref<JSValue> opA, Ref<JSValue> opB)
{
    //TODO: Review the semantics of 'toInt32' of each type
    const int valA = opA->toInt32();
    const int valB = opB->toInt32();

    switch (type)
    {
    case LEX_LSHIFT: return jsInt(valA << valB);
    case LEX_RSHIFT: return jsInt(valA >> valB);
    case LEX_RSHIFTUNSIGNED: return jsInt(unsigned(valA) >> unsigned(valB));
    default:
        ASSERT(!"Unexpected operator in 'shiftOp'");
        return jsDouble(getNaN());
    }
}

/**
 * Performs Javascript logical operations
 * First tries to transform operands into booleans, and then performs the operation.
 * @param type
 * @param opA
 * @param opB
 * @return 
 */
Ref<JSValue> logicOp(int type, Ref<JSValue> opA, Ref<JSValue> opB)
{
    const bool valA = opA->toBoolean();
    const bool valB = opB->toBoolean();

    switch (type)
    {
    case LEX_ANDAND: return jsBool(valA && valB);
    case LEX_OROR: return jsBool(valA || valB);
    default:
        ASSERT(!"Unexpected operator in 'logicOp'");
        return jsDouble(getNaN());
    }
}

/**
 * Performs Javascript comparision operations.
 * @param type
 * @param opA
 * @param opB
 * @return 
 */
Ref<JSValue> compareOp(int type, Ref<JSValue> opA, Ref<JSValue> opB)
{
    const JSValueTypes typeA = opA->getType();
    const JSValueTypes typeB = opB->getType();

    //TODO: Missing compare semantics between objects

    if (type == LEX_TYPEEQUAL && typeA != typeB)
        return jsFalse();
    else if (type == LEX_NTYPEEQUAL && typeA != typeB)
        return jsTrue();
    
    if (typeA == VT_UNDEFINED || typeB == VT_UNDEFINED)
        return jsBool(compareUndefined(type, typeA, typeB));

    if (typeA >= VT_STRING && typeB >= VT_STRING)
        return jsBool(compareStrings(type, opA->toString(), opB->toString()));
    else
        return jsBool(compareNumbers(type, opA->toDouble(), opB->toDouble()));
}

/**
 * Compares two strings using the given operator
 * @param type  operation type
 * @param opA
 * @param opB
 * @return 
 */
bool compareStrings(int type, const string& opA, const string& opB)
{
    switch (type)
    {
    case '<': return opA < opB;
    case '>': return opA > opB;
    case LEX_EQUAL:
    case LEX_TYPEEQUAL: return opA == opB;
    case LEX_NEQUAL:
    case LEX_NTYPEEQUAL: return opA != opB;
    case LEX_LEQUAL: return opA <= opB;
    case LEX_GEQUAL: return opA >= opB;
    default:
        ASSERT(!"Unexpected operator in 'compareStrings'");
        return false;
    }
}

/**
 * Compares two doubles using the given operator
 * @param type  operation type
 * @param opA
 * @param opB
 * @return 
 */
bool compareNumbers(int type, const double opA, const double opB)
{
    switch (type)
    {
    case '<': return opA < opB;
    case '>': return opA > opB;
    case LEX_EQUAL:
    case LEX_TYPEEQUAL: return opA == opB;
    case LEX_NEQUAL:
    case LEX_NTYPEEQUAL: return opA != opB;
    case LEX_LEQUAL: return opA <= opB;
    case LEX_GEQUAL: return opA >= opB;
    default:
        ASSERT(!"Unexpected operator in 'compareNumbers'");
        return false;
    }
}

/**
 * Handles comparisons when a 'undefined' value is involved.
 * @param opType    Type of comparison operation
 * @param typeA
 * @param typeB
 * @return 
 */
bool compareUndefined(int opType, JSValueTypes typeA, JSValueTypes typeB)
{
    //Ensure that typeA is undefined
    if (typeB == VT_UNDEFINED)
    {
        JSValueTypes temp = typeA;
        typeA = typeB;
        typeB = temp;
    }

    switch (opType)
    {
    case '<': 
    case '>': 
    case LEX_LEQUAL: 
    case LEX_GEQUAL: 
        return false;
        
    case LEX_EQUAL:     return typeB == VT_NULL || typeB == VT_UNDEFINED;
    case LEX_TYPEEQUAL: return typeB == VT_UNDEFINED;
    
    case LEX_NEQUAL:    return typeB != VT_NULL && typeB != VT_UNDEFINED;
    case LEX_NTYPEEQUAL:return typeB != VT_UNDEFINED;
    default:
        ASSERT(!"Unexpected operator in 'compareUndefined'");
        return false;
    }
}
