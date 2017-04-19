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
Ref<JSValue> mvmNewArray (ExecutionContext* ec)
{
    const size_t size = toSizeT( ec->getParam(0) );
    
    return JSArray::create(size);
}

/**
 * Increments a value by one.
 * @param pScope
 * @return 
 */
Ref<JSValue> mvmInc (ExecutionContext* ec)
{
    return jsDouble(ec->getParam(0)->toDouble() + 1);
}

/**
 * Decrements a value by one
 * @param pScope
 * @return 
 */
Ref<JSValue> mvmDec (ExecutionContext* ec)
{
    return jsDouble(ec->getParam(0)->toDouble() - 1);
}

/**
 * Negation: reverses sign of the input.
 * @param pScope
 * @return 
 */
Ref<JSValue> mvmNegate (ExecutionContext* ec)
{
    return jsDouble(- ec->getParam(0)->toDouble());
}

/**
 * Javascript 'add' operation. Adds numbers or concatenates strings.
 * @param pScope
 * @return 
 */
Ref<JSValue> mvmAdd (ExecutionContext* ec)
{
    Ref<JSValue> opA = ec->getParam(0);
    Ref<JSValue> opB = ec->getParam(1);
    
    const JSValueTypes typeA = opA->getType();
    const JSValueTypes typeB = opB->getType();

    if (typeA >= VT_STRING || typeB >= VT_STRING)
        return jsString(opA->toString() + opB->toString());
    else
        return jsDouble(opA->toDouble() + opB->toDouble());
}

/**
 * Substract operation.
 * @param pScope
 * @return 
 */
Ref<JSValue> mvmSub (ExecutionContext* ec)
{
    const double valA = ec->getParam(0)->toDouble();
    const double valB = ec->getParam(1)->toDouble();
    
    return jsDouble( valA - valB );
}

/**
 * Multiply operation
 * @param pScope
 * @return 
 */
Ref<JSValue> mvmMultiply (ExecutionContext* ec)
{
    const double valA = ec->getParam(0)->toDouble();
    const double valB = ec->getParam(1)->toDouble();
    
    return jsDouble( valA * valB );
}

/**
 * Divide operation
 * @param pScope
 * @return 
 */
Ref<JSValue> mvmDivide (ExecutionContext* ec)
{
    const double valA = ec->getParam(0)->toDouble();
    const double valB = ec->getParam(1)->toDouble();
    
    return jsDouble( valA / valB );
}

/**
 * Modulus operation
 * @param pScope
 * @return 
 */
Ref<JSValue> mvmModulus (ExecutionContext* ec)
{
    const double valA = ec->getParam(0)->toDouble();
    const double valB = ec->getParam(1)->toDouble();
    
    return jsDouble( fmod(valA, valB) );
}

/**
 * Power operation
 * @param pScope
 * @return 
 */
Ref<JSValue> mvmPower (ExecutionContext* ec)
{
    const double valA = ec->getParam(0)->toDouble();
    const double valB = ec->getParam(1)->toDouble();
    
    return jsDouble( pow(valA, valB) );
}

/**
 * Binary 'NOT' operation
 * @param pScope
 * @return 
 */
Ref<JSValue> mvmBinNot (ExecutionContext* ec)
{
    const int valA = toInt32( ec->getParam(0) );
    
    return jsInt(~valA);
}

/**
 * Binary 'AND' operation
 * @param pScope
 * @return 
 */
Ref<JSValue> mvmBinAnd (ExecutionContext* ec)
{
    const int valA = toInt32( ec->getParam(0) );
    const int valB = toInt32( ec->getParam(1) );
    
    return jsInt (valA & valB);
}

/**
 * Binary 'OR' operation
 * @param pScope
 * @return 
 */
Ref<JSValue> mvmBinOr (ExecutionContext* ec)
{
    const int valA = toInt32( ec->getParam(0) );
    const int valB = toInt32( ec->getParam(1) );
    
    return jsInt (valA | valB);
}

/**
 * Binary 'XOR' (exclusive OR) operation
 * @param pScope
 * @return 
 */
Ref<JSValue> mvmBinXor (ExecutionContext* ec)
{
    const int valA = toInt32( ec->getParam(0) );
    const int valB = toInt32( ec->getParam(1) );
    
    return jsInt (valA ^ valB);
}

/**
 * Logical 'NOT' operation. 
 * @param pScope
 * @return 
 */
Ref<JSValue> mvmLogicNot (ExecutionContext* ec)
{
    const bool valA = ec->getParam(0)->toBoolean();
    
    return jsBool (!valA);
}

/**
 * Left shift operation
 * @param pScope
 * @return 
 */
Ref<JSValue> mvmLshift (ExecutionContext* ec)
{
    const int valA = toInt32( ec->getParam(0) );
    const unsigned valB = unsigned(toInt32( ec->getParam(1)));
    
    return jsInt (valA << valB);
}

/**
 * Right shift operation on signed values.
 * @param pScope
 * @return 
 */
Ref<JSValue> mvmRshift (ExecutionContext* ec)
{
    const int valA = toInt32( ec->getParam(0) );
    const unsigned valB = unsigned(toInt32 (ec->getParam(1)));
    
    return jsInt (valA >> valB);
}

/**
 * Right shift operation, interpreting the values as unsigned numbers
 * @param pScope
 * @return 
 */
Ref<JSValue> mvmRshiftu (ExecutionContext* ec)
{
    const unsigned valA = unsigned(toInt32( ec->getParam(0) ));
    const unsigned valB = unsigned(toInt32( ec->getParam(1)));
    
    return jsDouble (double(valA >> valB));
}

/**
 * Performs Javascript comparision operations.
 * @param opA
 * @param opB
 * @return 
 */
double jsCompare(Ref<JSValue> opA, Ref<JSValue> opB)
{
    const JSValueTypes typeA = opA->getType();
    const JSValueTypes typeB = opB->getType();

    //TODO: Missing compare semantics between objects

    if (typeA >= VT_STRING && typeB >= VT_STRING)
        return opA->toString().compare(opB->toString());
    else
        return opA->toDouble() - opB->toDouble();
}

/**
 * '<' comparision operation
 * @param pScope
 * @return 
 */
Ref<JSValue> mvmLess (ExecutionContext* ec)
{
    Ref<JSValue> opA = ec->getParam(0);
    Ref<JSValue> opB = ec->getParam(1);
    
    if (opA->isNull() || opB->isNull())
        return jsFalse();
    else
        return jsBool (jsCompare(opA, opB) < 0);
}

/**
 * '>' comparision operation
 * @param pScope
 * @return 
 */
Ref<JSValue> mvmGreater (ExecutionContext* ec)
{
    Ref<JSValue> opA = ec->getParam(0);
    Ref<JSValue> opB = ec->getParam(1);
    
    if (opA->isNull() || opB->isNull())
        return jsFalse();
    else
        return jsBool (jsCompare(opA, opB) > 0);
}

/**
 * '<=' comparision operation
 * @param pScope
 * @return 
 */
Ref<JSValue> mvmLequal (ExecutionContext* ec)
{
    Ref<JSValue> opA = ec->getParam(0);
    Ref<JSValue> opB = ec->getParam(1);
    
    if (opA->isNull() || opB->isNull())
        return jsFalse();
    else
        return jsBool (jsCompare(opA, opB) <= 0);
}

/**
 * '<=' comparision operation
 * @param pScope
 * @return 
 */
Ref<JSValue> mvmGequal (ExecutionContext* ec)
{
    Ref<JSValue> opA = ec->getParam(0);
    Ref<JSValue> opB = ec->getParam(1);
    
    if (opA->isNull() || opB->isNull())
        return jsFalse();
    else
        return jsBool (jsCompare(opA, opB) <= 0);
}

/**
 * Equality compare (==)
 * @param pScope
 * @return 
 */
bool mvmAreEqual (Ref<JSValue> opA, Ref<JSValue> opB)
{
    if (opA->isNull() || opB->isNull())
        return opA->isNull() && opB->isNull();
    else
        return jsCompare(opA, opB) == 0;
}
Ref<JSValue> mvmAreEqual (ExecutionContext* ec)
{
    Ref<JSValue> opA = ec->getParam(0);
    Ref<JSValue> opB = ec->getParam(1);
 
    return jsBool (mvmAreEqual(opA, opB));
}

/**
 * Type and value equality compare (===)
 * @param pScope
 * @return 
 */
bool mvmAreTypeEqual (Ref<JSValue> opA, Ref<JSValue> opB)
{
    if (opA->getType() != opB->getType() )
        return false;
    else if (opA->isNull())
        return true;        //Both are 'null', because they have the same types.
    else
        return jsCompare(opA, opB) == 0;
}
Ref<JSValue> mvmAreTypeEqual (ExecutionContext* ec)
{
    Ref<JSValue> opA = ec->getParam(0);
    Ref<JSValue> opB = ec->getParam(1);

    return jsBool (mvmAreTypeEqual(opA, opB));
}

/**
 * Inequality compare (!=)
 * @param pScope
 * @return 
 */
Ref<JSValue> mvmNotEqual (ExecutionContext* ec)
{
    Ref<JSValue> opA = ec->getParam(0);
    Ref<JSValue> opB = ec->getParam(1);
    
    if (opA->isNull() || opB->isNull())
        return jsBool( !(opA->isNull() && opB->isNull()) );
    else
        return jsBool (jsCompare(opA, opB) != 0);
}

/**
 * Type and value equality compare (!==)
 * @param pScope
 * @return 
 */
Ref<JSValue> mvmNotTypeEqual (ExecutionContext* ec)
{
    Ref<JSValue> opA = ec->getParam(0);
    Ref<JSValue> opB = ec->getParam(1);
    
    if (opA->getType() != opB->getType() )
        return jsTrue();
    else if (opA->isNull())
        return jsFalse();        //Both are null, because they have the same types.
    else
        return jsBool (jsCompare(opA, opB) != 0);
}

Ref<JSValue> mvmToString (ExecutionContext* ec)
{
    return jsString(ec->getParam(0)->toString());
}

Ref<JSValue> mvmToBoolean (ExecutionContext* ec)
{
    return jsBool(ec->getParam(0)->toBoolean());
}

Ref<JSValue> mvmToNumber (ExecutionContext* ec)
{
    return jsDouble(ec->getParam(0)->toDouble());
}

Ref<JSValue> mvmIndexedRead (ExecutionContext* ec)
{
    auto index = ec->getParam(1);
    return ec->getParam(0)->indexedRead(index);
}

Ref<JSValue> mvmIndexedWrite (ExecutionContext* ec)
{
    auto index = ec->getParam(1);
    auto value = ec->getParam(2);
    return ec->getParam(0)->indexedWrite(index, value);
}

Ref<JSValue> mvmMakeClosure (ExecutionContext* ec)
{
    auto env = ec->getParam(0);
    auto fn = ec->getParam(1);
    
    if (!fn->isFunction())
        rtError ("'fn' parameter is not a function");
    
    return JSClosure::create(fn.staticCast<JSFunction>(), env);
}


//Ref<JSValue> mvmCall (ExecutionContext* ec)
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
    addNative1("@indexedRead", "index", mvmIndexedRead, scope);
    addNative2("@indexedWrite", "index", "value", mvmIndexedWrite, scope);
    //addNative0("@call", mvmCall, scope);
    
    addNative2("@makeClosure", "env", "fn", mvmMakeClosure, scope);
}
