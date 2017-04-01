/*
 * TinyJS
 *
 * A single-file Javascript-alike engine
 *
 * -  Math and Trigonometry functions
 *
 * Authored By O.Z.L.B. <ozlbinfo@gmail.com>
 *
 * Copyright (C) 2011 O.Z.L.B.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "ascript_pch.hpp"
#include "TinyJS_MathFunctions.h"
#include "scriptMain.h"
#include "microVM.h"

#include <math.h>
#include <cstdlib>
#include <sstream>

using namespace std;

//TODO: Replace by constants
#define k_E                 exp(1.0)
#define k_PI                3.1415926535897932384626433832795

#define F_RNG(a,min,max)    ((a)<(min) ? min : ((a)>(max) ? max : a ))

#define scGetInt(a)         ( ec->getParam(a)->toInt32() )
#define scGetDouble(a)      ( ec->getParam(a)->toDouble() )  

#ifdef _MSC_VER
namespace
{

    double asinh(const double &value)
    {
        double returned;

        if (value > 0)
            returned = log(value + sqrt(value * value + 1));
        else
            returned = -log(-value + sqrt(value * value + 1));

        return (returned);
    }

    double acosh(const double &value)
    {
        double returned;

        if (value > 0)
            returned = log(value + sqrt(value * value - 1));
        else
            returned = -log(-value + sqrt(value * value - 1));

        return (returned);
    }
}
#endif

//Math.abs(x) - returns absolute of given value

Ref<JSValue> scMathAbs(ExecutionContext* ec)
{
    return jsDouble(fabs(ec->getParam(0)->toDouble()));
}

//Math.round(a) - returns nearest round of given value

Ref<JSValue> scMathRound(ExecutionContext* ec)
{
    return jsDouble(round(scGetDouble(0)));
}

//Math.min(a,b) - returns minimum of two given values 

Ref<JSValue> scMathMin(ExecutionContext* ec)
{
    const double result = min(scGetDouble(0), scGetDouble(1));

    return jsDouble(result);
}

//Math.max(a,b) - returns maximum of two given values  

Ref<JSValue> scMathMax(ExecutionContext* ec)
{
    const double result = max(scGetDouble(0), scGetDouble(1));

    return jsDouble(result);
}

//Math.range(x,a,b) - returns value limited between two given values  

Ref<JSValue> scMathRange(ExecutionContext* ec)
{
    const double x = scGetDouble(0);
    const double a = scGetDouble(1);
    const double b = scGetDouble(2);

    if (a < b)
        return jsDouble(max(a, min(x, b)));
    else
        return jsDouble(max(b, min(x, a)));
}

//Math.sign(a) - returns sign of given value (-1==negative,0=zero,1=positive)

Ref<JSValue> scMathSign(ExecutionContext* ec)
{
    const double a = scGetDouble(0);
    int result = 0;

    if (a < 0)
        result = -1;
    else if (a > 0)
        result = 1;

    return jsInt(result);
}

//Math.PI() - returns PI value

Ref<JSValue> scMathPI(ExecutionContext* ec)
{
    return jsDouble(k_PI);
}

//Math.toDegrees(a) - returns degree value of a given angle in radians

Ref<JSValue> scMathToDegrees(ExecutionContext* ec)
{
    return jsDouble((180.0 / k_PI)*(scGetDouble(0)));
}

//Math.toRadians(a) - returns radians value of a given angle in degrees

Ref<JSValue> scMathToRadians(ExecutionContext* ec)
{
    return jsDouble((k_PI / 180.0)*(scGetDouble(0)));
}

//Math.sin(a) - returns trig. sine of given angle in radians

Ref<JSValue> scMathSin(ExecutionContext* ec)
{
    return jsDouble(sin(scGetDouble(0)));
}

//Math.asin(a) - returns trig. arcsine of given angle in radians

Ref<JSValue> scMathASin(ExecutionContext* ec)
{
    return jsDouble(asin(scGetDouble(0)));
}

//Math.cos(a) - returns trig. cosine of given angle in radians

Ref<JSValue> scMathCos(ExecutionContext* ec)
{
    return jsDouble(cos(scGetDouble(0)));
}

//Math.acos(a) - returns trig. arccosine of given angle in radians

Ref<JSValue> scMathACos(ExecutionContext* ec)
{
    return jsDouble(acos(scGetDouble(0)));
}

//Math.tan(a) - returns trig. tangent of given angle in radians

Ref<JSValue> scMathTan(ExecutionContext* ec)
{
    return jsDouble(tan(scGetDouble(0)));
}

//Math.atan(a) - returns trig. arctangent of given angle in radians

Ref<JSValue> scMathATan(ExecutionContext* ec)
{
    return jsDouble(atan(scGetDouble(0)));
}

//Math.sinh(a) - returns trig. hyperbolic sine of given angle in radians

Ref<JSValue> scMathSinh(ExecutionContext* ec)
{
    return jsDouble(sinh(scGetDouble(0)));
}

//Math.asinh(a) - returns trig. hyperbolic arcsine of given angle in radians

Ref<JSValue> scMathASinh(ExecutionContext* ec)
{
    return jsDouble(asinh(scGetDouble(0)));
}

//Math.cosh(a) - returns trig. hyperbolic cosine of given angle in radians

Ref<JSValue> scMathCosh(ExecutionContext* ec)
{
    return jsDouble(cosh(scGetDouble(0)));
}

//Math.acosh(a) - returns trig. hyperbolic arccosine of given angle in radians

Ref<JSValue> scMathACosh(ExecutionContext* ec)
{
    return jsDouble(acosh(scGetDouble(0)));
}

//Math.tanh(a) - returns trig. hyperbolic tangent of given angle in radians

Ref<JSValue> scMathTanh(ExecutionContext* ec)
{
    return jsDouble(tanh(scGetDouble(0)));
}

//Math.atan(a) - returns trig. hyperbolic arctangent of given angle in radians

Ref<JSValue> scMathATanh(ExecutionContext* ec)
{
    return jsDouble(atan(scGetDouble(0)));
}

//Math.E() - returns E Neplero value

Ref<JSValue> scMathE(ExecutionContext* ec)
{
    return jsDouble(k_E);
}

//Math.log(a) - returns natural logaritm (base E) of given value

Ref<JSValue> scMathLog(ExecutionContext* ec)
{
    return jsDouble(log(scGetDouble(0)));
}

//Math.log10(a) - returns logaritm(base 10) of given value

Ref<JSValue> scMathLog10(ExecutionContext* ec)
{
    return jsDouble(log10(scGetDouble(0)));
}

//Math.exp(a) - returns e raised to the power of a given number

Ref<JSValue> scMathExp(ExecutionContext* ec)
{
    return jsDouble(exp(scGetDouble(0)));
}

//Math.pow(a,b) - returns the result of a number raised to a power (a)^(b)

Ref<JSValue> scMathPow(ExecutionContext* ec)
{
    return jsDouble(pow(scGetDouble(0), scGetDouble(1)));
}

//Math.sqr(a) - returns square of given value

Ref<JSValue> scMathSqr(ExecutionContext* ec)
{
    return jsDouble((scGetDouble(0) * scGetDouble(0)));
}

//Math.sqrt(a) - returns square root of given value

Ref<JSValue> scMathSqrt(ExecutionContext* ec)
{
    return jsDouble(sqrt(scGetDouble(0)));
}

// ----------------------------------------------- Register Functions

void registerMathFunctions(Ref<JSObject> scope)
{

    // --- Math and Trigonometry functions ---
    addNative("function Math.abs(a)", scMathAbs, scope);
    addNative("function Math.round(a)", scMathRound, scope);
    addNative("function Math.min(a,b)", scMathMin, scope);
    addNative("function Math.max(a,b)", scMathMax, scope);
    addNative("function Math.range(x,a,b)", scMathRange, scope);
    addNative("function Math.sign(a)", scMathSign, scope);

    addNative("function Math.PI()", scMathPI, scope);
    addNative("function Math.toDegrees(a)", scMathToDegrees, scope);
    addNative("function Math.toRadians(a)", scMathToRadians, scope);
    addNative("function Math.sin(a)", scMathSin, scope);
    addNative("function Math.asin(a)", scMathASin, scope);
    addNative("function Math.cos(a)", scMathCos, scope);
    addNative("function Math.acos(a)", scMathACos, scope);
    addNative("function Math.tan(a)", scMathTan, scope);
    addNative("function Math.atan(a)", scMathATan, scope);
    addNative("function Math.sinh(a)", scMathSinh, scope);
    addNative("function Math.asinh(a)", scMathASinh, scope);
    addNative("function Math.cosh(a)", scMathCosh, scope);
    addNative("function Math.acosh(a)", scMathACosh, scope);
    addNative("function Math.tanh(a)", scMathTanh, scope);
    addNative("function Math.atanh(a)", scMathATanh, scope);

    addNative("function Math.E()", scMathE, scope);
    addNative("function Math.log(a)", scMathLog, scope);
    addNative("function Math.log10(a)", scMathLog10, scope);
    addNative("function Math.exp(a)", scMathExp, scope);
    addNative("function Math.pow(a,b)", scMathPow, scope);

    addNative("function Math.sqr(a)", scMathSqr, scope);
    addNative("function Math.sqrt(a)", scMathSqrt, scope);
}
