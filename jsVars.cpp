/* 
 * File:   JsVars.cpp
 * Author: ghernan
 * 
 * Javascript variables and values implementation code
 *
 * Created on November 21, 2016, 7:24 PM
 */

#include "ascript_pch.hpp"
#include "jsVars.h"
#include "utils.h"
#include "executionScope.h"
#include "asString.h"
#include "microVM.h"
#include "ScriptException.h"

#include <cstdlib>
#include <math.h>

using namespace std;

//Ref<JSValue> JSValue::call (Ref<FunctionScope> scope)
//{
//    rtError ("Not a callable object: %s", toString().c_str());
//    return jsNull();
//}

/**
 * Gives an string representation of the type name
 * @return 
 */
std::string getTypeName(JSValueTypes vType)
{
    typedef map<JSValueTypes, string> TypesMap;
    static TypesMap types;

    if (types.empty())
    {
        types[VT_NULL] = "null";
        types[VT_NUMBER] = "Number";
        types[VT_BOOL] = "Boolean";
        types[VT_ACTOR_REF] = "Actor reference";
        types[VT_INPUT_EP_REF] = "Input EP reference";
        types[VT_OUTPUT_EP_REF] = "Output EP reference";
        types[VT_CLASS] = "Class";
        types[VT_OBJECT] = "Object";
        types[VT_STRING] = "String";
        types[VT_ARRAY] = "Array";
        types[VT_ACTOR] = "Actor";
        types[VT_FUNCTION] = "Function";
        types[VT_CLOSURE] = "Closure";
        types[VT_ACTOR_CLASS] = "Actor class";
        types[VT_INPUT_EP] = "Input EP";
        types[VT_OUTPUT_EP] = "Output EP";
    }

    ASSERT(types.find(vType) != types.end());
    return types[vType];
}

Ref<JSValue> jsNull()
{
    static Ref<JSValue> value = refFromNew(new JSNull);
    return value;
}

Ref<JSBool> jsTrue()
{
    static Ref<JSBool> value = refFromNew(new JSBool(true));
    return value;
}

Ref<JSBool> jsFalse()
{
    static Ref<JSBool> value = refFromNew(new JSBool(false));
    return value;
}

Ref<JSValue> jsBool(bool value)
{
    if (value)
        return jsTrue();
    else
        return jsFalse();
}

Ref<JSValue> jsInt(int value)
{
    return JSNumber::create(value);
}

Ref<JSValue> jsSizeT(size_t value)
{
    return JSNumber::create((double)value);
}


Ref<JSValue> jsDouble(double value)
{
    return JSNumber::create(value);
}

Ref<JSValue> jsString(const std::string& value)
{
    return JSString::create(value);
}

/**
 * Class for numeric constants. 
 * It also stores the original string representation, to have an accurate string 
 * representation
 */
class JSNumberConstant : public JSNumber
{
public:

    static Ref<JSNumberConstant> create(const std::string& text)
    {
        if (text.size() > 0 && text[0] == '0' && isOctal(text))
        {
            const unsigned value = strtoul(text.c_str() + 1, NULL, 8);

            return refFromNew(new JSNumberConstant(value, text));
        }
        else
        {
            const double value = strtod(text.c_str(), NULL);

            return refFromNew(new JSNumberConstant(value, text));
        }
    }

    virtual std::string toString()const
    {
        return m_text;
    }

private:

    JSNumberConstant(double value, const std::string& text)
    : JSNumber(value), m_text(text)
    {
    }

    string m_text;
};//class JSNumberConstant

Ref<JSValue> createConstant(CScriptToken token)
{
    if (token.type() == LEX_STR)
        return JSString::create(token.strValue());
    else
        return JSNumberConstant::create(token.text());
}

/**
 * Compares two javascript values.
 * @param a
 * @param b
 * @return 
 */
double jsValuesCompare(Ref<JSValue> a, Ref<JSValue> b)
{
    auto typeA = a->getType();
    auto typeB = b->getType();

    if (typeA != typeB)
        return typeA - typeB;
    else
    {
        if (typeA <= VT_NULL)
            return 0;
        else if (typeA <= VT_BOOL)
            return a->toDouble() - b->toDouble();
        else if (typeA == VT_STRING)
            return a->toString().compare(b->toString());
        else
            return a.getPointer() - b.getPointer();
    }
}

/**
 * Converts a 'JSValue' into a 32 bit signed integer.
 * If the conversion is not posible, it returns zero. Therefore, the use
 * of 'isInteger' function is advised.
 * @param a
 * @return 
 */
int toInt32(Ref<JSValue> a)
{
    const double v = a->toDouble();

    if (isnan(v))
        return 0;
    else
        return (int) v;
}

/**
 * Converts a value into a 64 bit integer.
 * @param a
 * @return The integer value. In case of failure, it returns the largest 
 * number representable by a 64 bit integer (0xFFFFFFFFFFFFFFFF), which cannot
 * be represented by a double.
 */
unsigned long long toUint64(Ref<JSValue> a)
{
    const double v = a->toDouble();

    if (isnan(v))
        return 0xFFFFFFFFFFFFFFFF;
    else
        return (unsigned long long) v;
}

size_t toSizeT(Ref<JSValue> a)
{
    return (size_t) toUint64(a);
}

/**
 * Checks if a value is an integer number
 * @param a
 * @return 
 */
bool isInteger(Ref<JSValue> a)
{
    const double v = a->toDouble();

    return floor(v) == v;
}

/**
 * Checks if a value is a unsigned integer number.
 * @param a
 * @return 
 */
bool isUint(Ref<JSValue> a)
{
    const double v = a->toDouble();

    if (v < 0)
        return false;
    else
        return floor(v) == v;
}

/**
 * Creates a 'deep frozen' copy of an object, making deep frozen copies of 
 * all descendants which are necessary,
 * @param obj
 * @param transformed
 * @return 
 */

Ref<JSValue> JSValue::deepFreeze()
{
    JSValuesMap transformed;
    return deepFreeze(transformed);
}

/**
 * Writes to a variable in a variable map. If the variable already exist, and
 * is a constant, it throws a runtime exception.
 * @param map
 * @param name
 * @param value
 * @param isConst   If true, it creates a new constant
 */
void checkedVarWrite(VarMap& map, const std::string& name, Ref<JSValue> value, bool isConst)
{
    auto it = map.find(name);

    if (it != map.end() && it->second.isConst())
        rtError("Trying to write to constant '%s'", name.c_str());

    map[name] = VarProperties(value, isConst);
}

/**
 * Deletes a variable form a variable map. Throws exceptions if the variable 
 * does not exist or if it is a constant.
 * @param map
 * @param name
 * @return 
 */
Ref<JSValue> checkedVarDelete(VarMap& map, const std::string& name)
{
    auto it = map.find(name);
    Ref<JSValue> value;

    if (it == map.end())
        rtError("'%s' is not defined", name.c_str());
    else if (it->second.isConst())
        rtError("Trying to delete constant '%s'", name.c_str());
    else
    {
        value = it->second.value();
        map.erase(it);
    }

    return value;
}

/**
 * Default implementation of 'readField'. Tries to read it from class members.
 * @param key
 * @return 
 */
Ref<JSValue> JSValue::readField(const std::string& key)const
{
    typedef set<string>  FnSet;
    static FnSet functions;
    
    if (functions.empty())
    {
        functions.insert("toString");
        functions.insert("toBoolean");
        functions.insert("toNumber");
        functions.insert("indexedRead");
        functions.insert("indexedWrite");
        functions.insert("head");
        functions.insert("tail");
        functions.insert("call");
    }
    
    if (functions.count(key) > 0)
        return getGlobals()->get("@" + key);
    else
        return jsNull();
}


// JSNumber
//
//////////////////////////////////////////////////

/**
 * Construction function
 * @param value
 * @return 
 */
Ref<JSNumber> JSNumber::create(double value)
{
    return refFromNew(new JSNumber(value));
}

std::string JSNumber::toString()const
{
    //TODO: Review the standard. Find about number to string conversion formats.
    return double_to_string(m_value);
}

std::string JSNumber::getJSON(int indent)
{
    return double_to_string(m_value);
}

// JSBool
//
//////////////////////////////////////////////////


std::string JSBool::getJSON(int indent)
{
    return toString();
}


// JSFunction
//
//////////////////////////////////////////////////

/**
 * Creates a Javascript function 
 * @param name Function name
 * @return A new function object
 */
Ref<JSFunction> JSFunction::createJS(const std::string& name,
                                     const StringVector& params,
                                     Ref<RefCountObj> code)
{
    return refFromNew(new JSFunction(name, params, code));
}

/**
 * Creates an object which represents a native function
 * @param name  Function name
 * @param fnPtr Pointer to the native function.
 * @return A new function object
 */
Ref<JSFunction> JSFunction::createNative(const std::string& name,
                                         const StringVector& params,
                                         JSNativeFn fnPtr)
{
    return refFromNew(new JSFunction(name, params, fnPtr));
}

JSFunction::JSFunction(const std::string& name,
                       const StringVector& params,
                       JSNativeFn pNative) :
m_name(name),
m_params(params),
m_pNative(pNative)
{
}

JSFunction::JSFunction(const std::string& name,
                       const StringVector& params,
                       Ref<RefCountObj> code) :
m_name(name),
m_params(params),
m_codeMVM(code),
m_pNative(NULL)
{
}

JSFunction::~JSFunction()
{
    //    printf ("Destroying function: %s\n", m_name.c_str());
}

/**
 * Executes a function call (invoked by MicroVM)
 * @param scope
 * @param ec
 * @return 
 */
//Ref<JSValue> JSFunction::call (Ref<FunctionScope> scope)
//{
//    if (isNative())
//        return nativePtr()(scope.getPointer());
//    else
//    {
//        auto    code = getCodeMVM().staticCast<MvmRoutine>();
//        
//        return mvmExecute(code, getGlobals(), scope);
//    }
//}

/**
 * String representation of the function.
 * Just the function and the parameter list, no code.
 * @return 
 */
std::string JSFunction::toString()const
{
    ostringstream output;

    //TODO: another time a code very like to 'String.Join'. But sadly, each time
    //read from a different container.

    output << "function " << getName() << " (";

    for (size_t i = 0; i < m_params.size(); ++i)
    {
        if (i > 0)
            output << ',';

        output << m_params[i];
    }

    output << ")";
    return output.str();
}

Ref<JSClosure> JSClosure::create (Ref<JSFunction> fn, Ref<JSValue> env)
{
    return refFromNew (new JSClosure(fn, env));
}

JSClosure::JSClosure (Ref<JSFunction> fn, Ref<JSValue> env)
    : m_fn(fn), m_env(env)
{
}
