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

#include "TinyJS_MathFunctions.h"
#include "scriptMain.h"

#include <math.h>
#include <cstdlib>
#include <sstream>

using namespace std;

//TODO: Replace by constants
#define k_E                 exp(1.0)
#define k_PI                3.1415926535897932384626433832795

#define F_RNG(a,min,max)    ((a)<(min) ? min : ((a)>(max) ? max : a ))

#define scGetInt(a)         ( pScope->getParam(a)->toInt32() )
#define scGetDouble(a)      ( pScope->getParam(a)->toDouble() )  

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

Ref<JSValue> scMathAbs(FunctionScope* pScope)
{
    return jsDouble(fabs(pScope->getParam("a")->toDouble()));
}

//Math.round(a) - returns nearest round of given value

Ref<JSValue> scMathRound(FunctionScope* pScope)
{
    return jsDouble(round(scGetDouble("a")));
}

//Math.min(a,b) - returns minimum of two given values 

Ref<JSValue> scMathMin(FunctionScope* pScope)
{
    const double result = min(scGetDouble("a"), scGetDouble("b"));

    return jsDouble(result);
}

//Math.max(a,b) - returns maximum of two given values  

Ref<JSValue> scMathMax(FunctionScope* pScope)
{
    const double result = max(scGetDouble("a"), scGetDouble("b"));

    return jsDouble(result);
}

//Math.range(x,a,b) - returns value limited between two given values  

Ref<JSValue> scMathRange(FunctionScope* pScope)
{
    const double x = scGetDouble("x");
    const double a = scGetDouble("a");
    const double b = scGetDouble("b");

    if (a < b)
        return jsDouble(max(a, min(x, b)));
    else
        return jsDouble(max(b, min(x, a)));
}

//Math.sign(a) - returns sign of given value (-1==negative,0=zero,1=positive)

Ref<JSValue> scMathSign(FunctionScope* pScope)
{
    const double a = scGetDouble("a");
    int result = 0;

    if (a < 0)
        result = -1;
    else if (a > 0)
        result = 1;

    return jsInt(result);
}

//Math.PI() - returns PI value

Ref<JSValue> scMathPI(FunctionScope* pScope)
{
    return jsDouble(k_PI);
}

//Math.toDegrees(a) - returns degree value of a given angle in radians

Ref<JSValue> scMathToDegrees(FunctionScope* pScope)
{
    return jsDouble((180.0 / k_PI)*(scGetDouble("a")));
}

//Math.toRadians(a) - returns radians value of a given angle in degrees

Ref<JSValue> scMathToRadians(FunctionScope* pScope)
{
    return jsDouble((k_PI / 180.0)*(scGetDouble("a")));
}

//Math.sin(a) - returns trig. sine of given angle in radians

Ref<JSValue> scMathSin(FunctionScope* pScope)
{
    return jsDouble(sin(scGetDouble("a")));
}

//Math.asin(a) - returns trig. arcsine of given angle in radians

Ref<JSValue> scMathASin(FunctionScope* pScope)
{
    return jsDouble(asin(scGetDouble("a")));
}

//Math.cos(a) - returns trig. cosine of given angle in radians

Ref<JSValue> scMathCos(FunctionScope* pScope)
{
    return jsDouble(cos(scGetDouble("a")));
}

//Math.acos(a) - returns trig. arccosine of given angle in radians

Ref<JSValue> scMathACos(FunctionScope* pScope)
{
    return jsDouble(acos(scGetDouble("a")));
}

//Math.tan(a) - returns trig. tangent of given angle in radians

Ref<JSValue> scMathTan(FunctionScope* pScope)
{
    return jsDouble(tan(scGetDouble("a")));
}

//Math.atan(a) - returns trig. arctangent of given angle in radians

Ref<JSValue> scMathATan(FunctionScope* pScope)
{
    return jsDouble(atan(scGetDouble("a")));
}

//Math.sinh(a) - returns trig. hyperbolic sine of given angle in radians

Ref<JSValue> scMathSinh(FunctionScope* pScope)
{
    return jsDouble(sinh(scGetDouble("a")));
}

//Math.asinh(a) - returns trig. hyperbolic arcsine of given angle in radians

Ref<JSValue> scMathASinh(FunctionScope* pScope)
{
    return jsDouble(asinh(scGetDouble("a")));
}

//Math.cosh(a) - returns trig. hyperbolic cosine of given angle in radians

Ref<JSValue> scMathCosh(FunctionScope* pScope)
{
    return jsDouble(cosh(scGetDouble("a")));
}

//Math.acosh(a) - returns trig. hyperbolic arccosine of given angle in radians

Ref<JSValue> scMathACosh(FunctionScope* pScope)
{
    return jsDouble(acosh(scGetDouble("a")));
}

//Math.tanh(a) - returns trig. hyperbolic tangent of given angle in radians

Ref<JSValue> scMathTanh(FunctionScope* pScope)
{
    return jsDouble(tanh(scGetDouble("a")));
}

//Math.atan(a) - returns trig. hyperbolic arctangent of given angle in radians

Ref<JSValue> scMathATanh(FunctionScope* pScope)
{
    return jsDouble(atan(scGetDouble("a")));
}

//Math.E() - returns E Neplero value

Ref<JSValue> scMathE(FunctionScope* pScope)
{
    return jsDouble(k_E);
}

//Math.log(a) - returns natural logaritm (base E) of given value

Ref<JSValue> scMathLog(FunctionScope* pScope)
{
    return jsDouble(log(scGetDouble("a")));
}

//Math.log10(a) - returns logaritm(base 10) of given value

Ref<JSValue> scMathLog10(FunctionScope* pScope)
{
    return jsDouble(log10(scGetDouble("a")));
}

//Math.exp(a) - returns e raised to the power of a given number

Ref<JSValue> scMathExp(FunctionScope* pScope)
{
    return jsDouble(exp(scGetDouble("a")));
}

//Math.pow(a,b) - returns the result of a number raised to a power (a)^(b)

Ref<JSValue> scMathPow(FunctionScope* pScope)
{
    return jsDouble(pow(scGetDouble("a"), scGetDouble("b")));
}

//Math.sqr(a) - returns square of given value

Ref<JSValue> scMathSqr(FunctionScope* pScope)
{
    return jsDouble((scGetDouble("a") * scGetDouble("a")));
}

//Math.sqrt(a) - returns square root of given value

Ref<JSValue> scMathSqrt(FunctionScope* pScope)
{
    return jsDouble(sqrt(scGetDouble("a")));
}

// ----------------------------------------------- Register Functions

void registerMathFunctions(Ref<IScope> scope)
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
