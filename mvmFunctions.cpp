/* 
 * File:   mvmFunctions.cpp
 * Author: ghernan
 * 
 * Functions which implement Micro virtual machine primitive operations
 * 
 * Created on December 4, 2016, 11:48 PM
 */

#include "OS_support.h"
#include "mvmFunctions.h"
#include "jsArray.h"
#include "scriptMain.h"

#include <math.h>
#include <string>

using namespace std;

/**
 * Creates a new array
 * @param pScope
 * @return 
 */
Ref<JSValue> mvmNewArray (FunctionScope* pScope)
{
    const size_t size = toSizeT( pScope->getThis() );
    
    return JSArray::create(size);
}

/**
 * Increments a value by one.
 * @param pScope
 * @return 
 */
Ref<JSValue> mvmInc (FunctionScope* pScope)
{
    return jsDouble(pScope->getThis()->toDouble() + 1);
}

/**
 * Decrements a value by one
 * @param pScope
 * @return 
 */
Ref<JSValue> mvmDec (FunctionScope* pScope)
{
    return jsDouble(pScope->getThis()->toDouble() - 1);
}

/**
 * Negation: reverses sign of the input.
 * @param pScope
 * @return 
 */
Ref<JSValue> mvmNegate (FunctionScope* pScope)
{
    return jsDouble(- pScope->getThis()->toDouble());
}

/**
 * Javascript 'add' operation. Adds numbers or concatenates strings.
 * @param pScope
 * @return 
 */
Ref<JSValue> mvmAdd (FunctionScope* pScope)
{
    Ref<JSValue> opA = pScope->getThis();
    Ref<JSValue> opB = pScope->getParam("b");
    
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
Ref<JSValue> mvmSub (FunctionScope* pScope)
{
    const double valA = pScope->getThis()->toDouble();
    const double valB = pScope->getParam("b")->toDouble();
    
    return jsDouble( valA - valB );
}

/**
 * Multiply operation
 * @param pScope
 * @return 
 */
Ref<JSValue> mvmMultiply (FunctionScope* pScope)
{
    const double valA = pScope->getThis()->toDouble();
    const double valB = pScope->getParam("b")->toDouble();
    
    return jsDouble( valA * valB );
}

/**
 * Divide operation
 * @param pScope
 * @return 
 */
Ref<JSValue> mvmDivide (FunctionScope* pScope)
{
    const double valA = pScope->getThis()->toDouble();
    const double valB = pScope->getParam("b")->toDouble();
    
    return jsDouble( valA / valB );
}

/**
 * Modulus operation
 * @param pScope
 * @return 
 */
Ref<JSValue> mvmModulus (FunctionScope* pScope)
{
    const double valA = pScope->getThis()->toDouble();
    const double valB = pScope->getParam("b")->toDouble();
    
    return jsDouble( fmod(valA, valB) );
}

/**
 * Power operation
 * @param pScope
 * @return 
 */
Ref<JSValue> mvmPower (FunctionScope* pScope)
{
    const double valA = pScope->getThis()->toDouble();
    const double valB = pScope->getParam("b")->toDouble();
    
    return jsDouble( pow(valA, valB) );
}

/**
 * Binary 'NOT' operation
 * @param pScope
 * @return 
 */
Ref<JSValue> mvmBinNot (FunctionScope* pScope)
{
    const int valA = toInt32( pScope->getThis() );
    
    return jsInt(~valA);
}

/**
 * Binary 'AND' operation
 * @param pScope
 * @return 
 */
Ref<JSValue> mvmBinAnd (FunctionScope* pScope)
{
    const int valA = toInt32( pScope->getThis() );
    const int valB = toInt32( pScope->getParam("b") );
    
    return jsInt (valA & valB);
}

/**
 * Binary 'OR' operation
 * @param pScope
 * @return 
 */
Ref<JSValue> mvmBinOr (FunctionScope* pScope)
{
    const int valA = toInt32( pScope->getThis() );
    const int valB = toInt32( pScope->getParam("b") );
    
    return jsInt (valA | valB);
}

/**
 * Binary 'XOR' (exclusive OR) operation
 * @param pScope
 * @return 
 */
Ref<JSValue> mvmBinXor (FunctionScope* pScope)
{
    const int valA = toInt32( pScope->getThis() );
    const int valB = toInt32( pScope->getParam("b") );
    
    return jsInt (valA ^ valB);
}

/**
 * Logical 'NOT' operation. 
 * @param pScope
 * @return 
 */
Ref<JSValue> mvmLogicNot (FunctionScope* pScope)
{
    const bool valA = pScope->getThis()->toBoolean();
    
    return jsBool (!valA);
}

/**
 * Left shift operation
 * @param pScope
 * @return 
 */
Ref<JSValue> mvmLshift (FunctionScope* pScope)
{
    const int valA = toInt32( pScope->getThis() );
    const unsigned valB = unsigned(toInt32( pScope->getParam("b")));
    
    return jsInt (valA << valB);
}

/**
 * Right shift operation on signed values.
 * @param pScope
 * @return 
 */
Ref<JSValue> mvmRshift (FunctionScope* pScope)
{
    const int valA = toInt32( pScope->getThis() );
    const unsigned valB = unsigned(toInt32 (pScope->getParam("b")));
    
    return jsInt (valA >> valB);
}

/**
 * Right shift operation, interpreting the values as unsigned numbers
 * @param pScope
 * @return 
 */
Ref<JSValue> mvmRshiftu (FunctionScope* pScope)
{
    const unsigned valA = unsigned(toInt32( pScope->getThis() ));
    const unsigned valB = unsigned(toInt32( pScope->getParam("b")));
    
    return jsDouble (double(valA >> valB));
}

/**
 * Gets the current element of a sequence.
 * @param pScope
 * @return 
 */
Ref<JSValue> mvmHead (FunctionScope* pScope)
{    
    return pScope->getThis()->head();
}

/**
 * Gets the 'tail' of a sequence. A sublist which excludes the current one.
 * @param pScope
 * @return 
 */
Ref<JSValue> mvmTail (FunctionScope* pScope)
{    
    return pScope->getThis()->tail();
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
Ref<JSValue> mvmLess (FunctionScope* pScope)
{
    Ref<JSValue> opA = pScope->getThis();
    Ref<JSValue> opB = pScope->getParam("b");
    
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
Ref<JSValue> mvmGreater (FunctionScope* pScope)
{
    Ref<JSValue> opA = pScope->getThis();
    Ref<JSValue> opB = pScope->getParam("b");
    
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
Ref<JSValue> mvmLequal (FunctionScope* pScope)
{
    Ref<JSValue> opA = pScope->getThis();
    Ref<JSValue> opB = pScope->getParam("b");
    
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
Ref<JSValue> mvmGequal (FunctionScope* pScope)
{
    Ref<JSValue> opA = pScope->getThis();
    Ref<JSValue> opB = pScope->getParam("b");
    
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
Ref<JSValue> mvmAreEqual (FunctionScope* pScope)
{
    Ref<JSValue> opA = pScope->getThis();
    Ref<JSValue> opB = pScope->getParam("b");
 
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
Ref<JSValue> mvmAreTypeEqual (FunctionScope* pScope)
{
    Ref<JSValue> opA = pScope->getThis();
    Ref<JSValue> opB = pScope->getParam("b");

    return jsBool (mvmAreTypeEqual(opA, opB));
}

/**
 * Inequality compare (!=)
 * @param pScope
 * @return 
 */
Ref<JSValue> mvmNotEqual (FunctionScope* pScope)
{
    Ref<JSValue> opA = pScope->getThis();
    Ref<JSValue> opB = pScope->getParam("b");
    
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
Ref<JSValue> mvmNotTypeEqual (FunctionScope* pScope)
{
    Ref<JSValue> opA = pScope->getThis();
    Ref<JSValue> opB = pScope->getParam("b");
    
    if (opA->getType() != opB->getType() )
        return jsTrue();
    else if (opA->isNull())
        return jsFalse();        //Both are null, because they have the same types.
    else
        return jsBool (jsCompare(opA, opB) != 0);
}

Ref<JSValue> mvmToString (FunctionScope* pScope)
{
    return jsString(pScope->getThis()->toString());
}

Ref<JSValue> mvmToBoolean (FunctionScope* pScope)
{
    return jsBool(pScope->getThis()->toBoolean());
}

Ref<JSValue> mvmToNumber (FunctionScope* pScope)
{
    return jsDouble(pScope->getThis()->toDouble());
}

Ref<JSValue> mvmIndexedRead (FunctionScope* pScope)
{
    auto index = pScope->getParam("index");
    return pScope->getThis()->indexedRead(index);
}

Ref<JSValue> mvmIndexedWrite (FunctionScope* pScope)
{
    auto index = pScope->getParam("index");
    auto value = pScope->getParam("value");
    return pScope->getThis()->indexedWrite(index, value);
}

Ref<JSValue> mvmCall (FunctionScope* pScope)
{
    return pScope->getThis()->call(pScope);
}

/**
 * Registers MVM primitive operations to the given scope.
 * @param scope
 */
void registerMvmFunctions(Ref<IScope> scope)
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

    addNative0("@head", mvmHead, scope);
    addNative0("@tail", mvmTail, scope);
    
    addNative0("@toString", mvmToString, scope);
    addNative0("@toBoolean", mvmToBoolean, scope);
    addNative0("@toNumber", mvmToNumber, scope);
    addNative1("@indexedRead", "index", mvmIndexedRead, scope);
    addNative2("@indexedWrite", "index", "value", mvmIndexedWrite, scope);
    addNative0("@call", mvmCall, scope);
}
