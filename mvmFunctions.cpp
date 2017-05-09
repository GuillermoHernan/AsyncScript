/* 
 * File:   mvmFunctions.cpp
 * Author: ghernan
 * 
 * Functions which implement Micro virtual machine primitive operations
 * 
 * Created on December 4, 2016, 11:48 PM
 */

#include "ascript_pch.hpp"
#include "mvmFunctions.h"
#include "jsArray.h"
#include "scriptMain.h"
#include "ScriptException.h"

#include <math.h>
#include <string>

using namespace std;

/**
 * Creates a new array
 * @param pScope
 * @return 
 */
ASValue mvmNewArray (ExecutionContext* ec)
{
    const size_t size = ec->getParam(0).toSizeT();
    
    return JSArray::create(size)->value();
}

/**
 * Increments a value by one.
 * @param pScope
 * @return 
 */
ASValue mvmInc (ExecutionContext* ec)
{
    return jsDouble(ec->getParam(0).toDouble(ec) + 1);
}

/**
 * Decrements a value by one
 * @param pScope
 * @return 
 */
ASValue mvmDec (ExecutionContext* ec)
{
    return jsDouble(ec->getParam(0).toDouble(ec) - 1);
}

/**
 * Negation: reverses sign of the input.
 * @param pScope
 * @return 
 */
ASValue mvmNegate (ExecutionContext* ec)
{
    return jsDouble(- ec->getParam(0).toDouble(ec));
}

/**
 * Javascript 'add' operation. Adds numbers or concatenates strings.
 * @param pScope
 * @return 
 */
ASValue mvmAdd (ExecutionContext* ec)
{
    ASValue opA = ec->getParam(0);
    ASValue opB = ec->getParam(1);
    
    const JSValueTypes typeA = opA.getType();
    const JSValueTypes typeB = opB.getType();

    if (typeA >= VT_STRING || typeB >= VT_STRING)
        return jsString(opA.toString(ec) + opB.toString(ec));
    else
        return jsDouble(opA.toDouble(ec) + opB.toDouble(ec));
}

/**
 * Substract operation.
 * @param pScope
 * @return 
 */
ASValue mvmSub (ExecutionContext* ec)
{
    const double valA = ec->getParam(0).toDouble(ec);
    const double valB = ec->getParam(1).toDouble(ec);
    
    return jsDouble( valA - valB );
}

/**
 * Multiply operation
 * @param pScope
 * @return 
 */
ASValue mvmMultiply (ExecutionContext* ec)
{
    const double valA = ec->getParam(0).toDouble(ec);
    const double valB = ec->getParam(1).toDouble(ec);
    
    return jsDouble( valA * valB );
}

/**
 * Divide operation
 * @param pScope
 * @return 
 */
ASValue mvmDivide (ExecutionContext* ec)
{
    const double valA = ec->getParam(0).toDouble(ec);
    const double valB = ec->getParam(1).toDouble(ec);
    
    return jsDouble( valA / valB );
}

/**
 * Modulus operation
 * @param pScope
 * @return 
 */
ASValue mvmModulus (ExecutionContext* ec)
{
    const double valA = ec->getParam(0).toDouble(ec);
    const double valB = ec->getParam(1).toDouble(ec);
    
    return jsDouble( fmod(valA, valB) );
}

/**
 * Power operation
 * @param pScope
 * @return 
 */
ASValue mvmPower (ExecutionContext* ec)
{
    const double valA = ec->getParam(0).toDouble(ec);
    const double valB = ec->getParam(1).toDouble(ec);
    
    return jsDouble( pow(valA, valB) );
}

/**
 * Binary 'NOT' operation
 * @param pScope
 * @return 
 */
ASValue mvmBinNot (ExecutionContext* ec)
{
    const int valA =  ec->getParam(0).toInt32();
    
    return jsInt(~valA);
}

/**
 * Binary 'AND' operation
 * @param pScope
 * @return 
 */
ASValue mvmBinAnd (ExecutionContext* ec)
{
    const int valA =  ec->getParam(0).toInt32();
    const int valB =  ec->getParam(1).toInt32();
    
    return jsInt (valA & valB);
}

/**
 * Binary 'OR' operation
 * @param pScope
 * @return 
 */
ASValue mvmBinOr (ExecutionContext* ec)
{
    const int valA =  ec->getParam(0).toInt32();
    const int valB =  ec->getParam(1).toInt32();
    
    return jsInt (valA | valB);
}

/**
 * Binary 'XOR' (exclusive OR) operation
 * @param pScope
 * @return 
 */
ASValue mvmBinXor (ExecutionContext* ec)
{
    const int valA =  ec->getParam(0).toInt32();
    const int valB =  ec->getParam(1).toInt32();
    
    return jsInt (valA ^ valB);
}

/**
 * Logical 'NOT' operation. 
 * @param pScope
 * @return 
 */
ASValue mvmLogicNot (ExecutionContext* ec)
{
    const bool valA = ec->getParam(0).toBoolean(ec);
    
    return jsBool (!valA);
}

/**
 * Left shift operation
 * @param pScope
 * @return 
 */
ASValue mvmLshift (ExecutionContext* ec)
{
    const int valA =  ec->getParam(0).toInt32();
    const unsigned valB = unsigned( ec->getParam(1).toInt32() );
    
    return jsInt (valA << valB);
}

/**
 * Right shift operation on signed values.
 * @param pScope
 * @return 
 */
ASValue mvmRshift (ExecutionContext* ec)
{
    const int valA =  ec->getParam(0).toInt32();
    const unsigned valB = unsigned( ec->getParam(1).toInt32() );
    
    return jsInt (valA >> valB);
}

/**
 * Right shift operation, interpreting the values as unsigned numbers
 * @param pScope
 * @return 
 */
ASValue mvmRshiftu (ExecutionContext* ec)
{
    const unsigned valA = unsigned( ec->getParam(0).toInt32() );
    const unsigned valB = unsigned( ec->getParam(1).toInt32() );
    
    return jsDouble (double(valA >> valB));
}

/**
 * '<' comparision operation
 * @param pScope
 * @return 
 */
ASValue mvmLess (ExecutionContext* ec)
{
    ASValue opA = ec->getParam(0);
    ASValue opB = ec->getParam(1);
    
    if (opA.isNull() || opB.isNull())
        return jsFalse();
    else
        return jsBool (opA.compare(opB, ec) < 0);
}

/**
 * '>' comparision operation
 * @param pScope
 * @return 
 */
ASValue mvmGreater (ExecutionContext* ec)
{
    ASValue opA = ec->getParam(0);
    ASValue opB = ec->getParam(1);
    
    if (opA.isNull() || opB.isNull())
        return jsFalse();
    else
        return jsBool (opA.compare(opB, ec) > 0);
}

/**
 * '<=' comparision operation
 * @param pScope
 * @return 
 */
ASValue mvmLequal (ExecutionContext* ec)
{
    ASValue opA = ec->getParam(0);
    ASValue opB = ec->getParam(1);
    
    if (opA.isNull() || opB.isNull())
        return jsFalse();
    else
        return jsBool (opA.compare(opB, ec) <= 0);
}

/**
 * '<=' comparision operation
 * @param pScope
 * @return 
 */
ASValue mvmGequal (ExecutionContext* ec)
{
    ASValue opA = ec->getParam(0);
    ASValue opB = ec->getParam(1);
    
    if (opA.isNull() || opB.isNull())
        return jsFalse();
    else
        return jsBool (opA.compare(opB, ec) <= 0);
}

/**
 * Equality compare (==)
 * @param pScope
 * @return 
 */
//bool mvmAreEqual (ASValue opA, ASValue opB)
//{
//    return opA.compare(opB, ec) == 0;
//}
ASValue mvmAreEqual (ExecutionContext* ec)
{
    ASValue opA = ec->getParam(0);
    ASValue opB = ec->getParam(1);
 
    return jsBool (opA.compare(opB, ec) == 0);
}

/**
 * Type and value equality compare (===)
 * @param pScope
 * @return 
 */
//bool mvmAreTypeEqual (ASValue opA, ASValue opB)
//{
//    if (opA.getType() != opB.getType() )
//        return false;
//    else
//        return opA.compare(opB, ec) == 0;
//}
ASValue mvmAreTypeEqual (ExecutionContext* ec)
{
    ASValue opA = ec->getParam(0);
    ASValue opB = ec->getParam(1);

    if (opA.getType() != opB.getType() )
        return jsFalse();
    else
        return jsBool (opA.compare(opB, ec) == 0);
}

/**
 * Inequality compare (!=)
 * @param pScope
 * @return 
 */
ASValue mvmNotEqual (ExecutionContext* ec)
{
    ASValue opA = ec->getParam(0);
    ASValue opB = ec->getParam(1);
    
    if (opA.isNull() || opB.isNull())
        return jsBool( !(opA.isNull() && opB.isNull()) );
    else
        return jsBool (opA.compare (opB, ec) != 0);
}

/**
 * Type and value equality compare (!==)
 * @param pScope
 * @return 
 */
ASValue mvmNotTypeEqual (ExecutionContext* ec)
{
    ASValue opA = ec->getParam(0);
    ASValue opB = ec->getParam(1);
    
    if (opA.getType() != opB.getType() )
        return jsTrue();
    else
        return jsBool ( opA.compare(opB, ec) != 0 );
}

ASValue mvmToString (ExecutionContext* ec)
{
    return jsString(ec->getParam(0).toString(ec));
}

ASValue mvmToBoolean (ExecutionContext* ec)
{
    return jsBool(ec->getParam(0).toBoolean(ec));
}

ASValue mvmToNumber (ExecutionContext* ec)
{
    return jsDouble(ec->getParam(0).toDouble(ec));
}

ASValue mvmIndexedRead (ExecutionContext* ec)
{
    auto index = ec->getParam(1);
    return ec->getParam(0).getAt(index,ec);
}

ASValue mvmIndexedWrite (ExecutionContext* ec)
{
    auto index = ec->getParam(1);
    auto value = ec->getParam(2);
    return ec->getParam(0).setAt(index, value, ec);
}

ASValue mvmMakeClosure (ExecutionContext* ec)
{
    ASSERT (!ec->frames.empty());
    
    const CallFrame&    curFrame = ec->frames.back();
    const size_t        nParams = curFrame.numParams;
    
    ASSERT (nParams >= 2);
    ASValue*    paramsBegin = &ec->stack[curFrame.paramsIndex];
    auto        fn = paramsBegin[nParams-1];
    
    if (fn.getType() != VT_FUNCTION)
        rtError ("'fn' parameter is not a function");
    
    return JSClosure::create(fn.staticCast<JSFunction>(), paramsBegin, nParams-1)->value();
}


//ASValue mvmCall (ExecutionContext* ec)
//{
//    return ec->getThis()->call(pScope);
//}

/**
 * Registers MVM primitive operations to the given scope.
 * @param scope
 */
void registerMvmFunctions(Ref<JSObject> scope)
{
    //addNative0("@newObj", mvmNewObj, scope);
    addNative0("@newArray", mvmNewArray, scope);

    addNative0("@inc", mvmInc, scope);
    addNative0("@dec", mvmDec, scope);
    addNative0("@negate", mvmNegate, scope);

    addNative1("@add", "b", mvmAdd, scope);
    addNative1("@sub", "b", mvmSub, scope);
    addNative1("@multiply", "b", mvmMultiply, scope);
    addNative1("@divide", "b", mvmDivide, scope);
    addNative1("@modulus", "b", mvmModulus, scope);
    addNative1("@power", "b", mvmPower, scope);

    addNative0("@binNot", mvmBinNot, scope);
    addNative1("@binAnd", "b", mvmBinAnd, scope);
    addNative1("@binOr", "b", mvmBinOr, scope);
    addNative1("@binXor", "b", mvmBinXor, scope);

    addNative0("@logicNot", mvmLogicNot, scope);

    addNative1("@lshift", "b", mvmLshift, scope);
    addNative1("@rshift", "b", mvmRshift, scope);
    addNative1("@rshiftu", "b", mvmRshiftu, scope);

    addNative1("@less", "b", mvmLess, scope);
    addNative1("@greater", "b", mvmGreater, scope);
    addNative1("@areEqual", "b", mvmAreEqual, scope);
    addNative1("@areTypeEqual", "b", mvmAreTypeEqual, scope);
    addNative1("@notEqual", "b", mvmNotEqual, scope);
    addNative1("@notTypeEqual", "b", mvmNotTypeEqual, scope);
    addNative1("@lequal", "b", mvmLequal, scope);
    addNative1("@gequal", "b", mvmGequal, scope);

    addNative0("@toString", mvmToString, scope);
    addNative0("@toBoolean", mvmToBoolean, scope);
    addNative0("@toNumber", mvmToNumber, scope);
    addNative1("@getAt", "index", mvmIndexedRead, scope);
    addNative2("@setAt", "index", "value", mvmIndexedWrite, scope);
    //addNative0("@call", mvmCall, scope);
    
    addNative2("@makeClosure", "env", "fn", mvmMakeClosure, scope);
}
