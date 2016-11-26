/*
 * TinyJS
 *
 * A single-file Javascript-alike engine
 *
 * Authored By Gordon Williams <gw@pur3.co.uk>
 *
 * Copyright (C) 2009 Pur3 Ltd
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

/* Version 0.1  :  (gw) First published on Google Code
   Version 0.11 :  Making sure the 'root' variable never changes
                   'symbol_base' added for the current base of the sybmbol table
   Version 0.12 :  Added findChildOrCreate, changed string passing to use references
                   Fixed broken string encoding in getJSString()
                   Removed getInitCode and added getJSON instead
                   Added nil
                   Added rough JSON parsing
                   Improved example app
   Version 0.13 :  Added tokenEnd/tokenLastEnd to lexer to avoid parsing whitespace
                   Ability to define functions without names
                   Can now do "var mine = function(a,b) { ... };"
                   Slightly better 'trace' function
                   Added findChildOrCreateByPath function
                   Added simple test suite
                   Added skipping of blocks when not executing
   Version 0.14 :  Added parsing of more number types
                   Added parsing of string defined with '
                   Changed nil to null as per spec, added 'undefined'
                   Now set variables with the correct scope, and treat unknown
                              as 'undefined' rather than failing
                   Added proper (I hope) handling of null and undefined
                   Added === check
   Version 0.15 :  Fix for possible memory leaks
   Version 0.16 :  Removal of un-needed findRecursive calls
                   symbol_base removed and replaced with 'scopes' stack
                   Added reference counting a proper tree structure
                       (Allowing pass by reference)
                   Allowed JSON output to output IDs, not strings
                   Added get/set for array indices
                   Changed Callbacks to include user data pointer
                   Added some support for objects
                   Added more Java-esque builtin functions
   Version 0.17 :  Now we don't deepCopy the parent object of the class
                   Added JSON.stringify and eval()
                   Nicer JSON indenting
                   Fixed function output in JSON
                   Added evaluateComplex
                   Fixed some reentrancy issues with evaluate/execute
   Version 0.18 :  Fixed some issues with code being executed when it shouldn't
   Version 0.19 :  Added array.length
                   Changed '__parent' to 'prototype' to bring it more in line with javascript
   Version 0.20 :  Added '%' operator
   Version 0.21 :  Added array type
                   String.length() no more - now String.length
                   Added extra constructors to reduce confusion
                   Fixed checks against undefined
   Version 0.22 :  First part of ardi's changes:
                       sprintf -> sprintf_s
                       extra tokens parsed
                       array memory leak fixed
                   Fixed memory leak in evaluateComplex
                   Fixed memory leak in FOR loops
                   Fixed memory leak for unary minus
   Version 0.23 :  Allowed evaluate[Complex] to take in semi-colon separated
                     statements and then only return the value from the last one.
                     Also checks to make sure *everything* was parsed.
                   Ints + doubles are now stored in binary form (faster + more precise)
   Version 0.24 :  More useful error for maths ops
                   Don't dump everything on a match error.
   Version 0.25 :  Better string escaping
   Version 0.26 :  Add CScriptVar::equals
                   Add built-in array functions
   Version 0.27 :  Added OZLB's TinyJS.setVariable (with some tweaks)
                   Added OZLB's Maths Functions
   Version 0.28 :  Ternary operator
                   Rudimentary call stack on error
                   Added String Character functions
                   Added shift operators
   Version 0.29 :  Added new object via functions
                   Fixed getString() for double on some platforms
   Version 0.30 :  Rlyeh Mario's patch for Math Functions on VC++
   Version 0.31 :  Add exec() to TinyJS functions
                   Now print quoted JSON that can be read by PHP/Python parsers
                   Fixed postfix increment operator
   Version 0.32 :  Fixed Math.randInt on 32 bit PCs, where it was broken
   Version 0.33 :  Fixed Memory leak + brokenness on === comparison

    NOTE:
          Constructing an array with an initial length 'Array(5)' doesn't work
          Recursive loops of data such as a.foo = a; fail to be garbage collected
          length variable cannot be set
          The postfix increment operator returns the current value, not the previous as it should.
          There is no prefix increment operator
          Arrays are implemented as a linked list - hence a lookup time is O(n)

    TODO:
          Utility va-args style function in TinyJS for executing a function directly
          Merge the parsing of expressions/statements so eval("statement") works like we'd expect.
          Move 'shift' implementation into mathsOp

 */

#include "TinyJS.h"
#include "TinyJS_Lexer.h"
#include "jsOperators.h"
#include "utils.h"
#include <assert.h>

#include "OS_support.h"

/* Frees the given link IF it isn't owned by anything else */
//#define CLEAN(x) { CScriptVarLink *__v = x; if (__v && !__v->owned) { delete __v; } }
/* Create a LINK to point to VAR and free the old link.
 * BUT this is more clever - it tries to keep the old link if it's not owned to save allocations */
//#define CREATE_LINK(LINK, VAR) { if (!LINK || LINK->owned) LINK = new CScriptVarLink(VAR); else LINK->replaceWith(VAR); }

#include <string>
#include <string.h>
#include <sstream>
#include <cstdlib>
#include <stdio.h>

using namespace std;

static const char *TINYJS_TEMP_NAME = "";



// ----------------------------------------------------------------------------------- Memory Debug

#define DEBUG_MEMORY 0

#if DEBUG_MEMORY

vector<CScriptVar*> allocatedVars;
vector<CScriptVarLink*> allocatedLinks;

void mark_allocated(CScriptVar *v) {
    allocatedVars.push_back(v);
}

void mark_deallocated(CScriptVar *v) {
    for (size_t i=0;i<allocatedVars.size();i++) {
      if (allocatedVars[i] == v) {
        allocatedVars.erase(allocatedVars.begin()+i);
        break;
      }
    }
}

void mark_allocated(CScriptVarLink *v) {
    allocatedLinks.push_back(v);
}

void mark_deallocated(CScriptVarLink *v) {
    for (size_t i=0;i<allocatedLinks.size();i++) {
      if (allocatedLinks[i] == v) {
        allocatedLinks.erase(allocatedLinks.begin()+i);
        break;
      }
    }
}

void show_allocated() {
    for (size_t i=0;i<allocatedVars.size();i++) {
      printf("ALLOCATED, %d refs\n", allocatedVars[i]->getRefs());
      allocatedVars[i]->trace("  ");
    }
    for (size_t i=0;i<allocatedLinks.size();i++) {
      printf("ALLOCATED LINK %s, allocated[%d] to \n", allocatedLinks[i]->name.c_str(), allocatedLinks[i]->var->getRefs());
      allocatedLinks[i]->var->trace("  ");
    }
    allocatedVars.clear();
    allocatedLinks.clear();
}
#endif

/**
 * To return the result of a parse / execute operation
 */
struct SResult {
    CScriptToken    token;
    Ref<JSValue>    value;
    
    SResult (CScriptToken _token, Ref<JSValue> _value)
        : token(_token)
        ,value (_value)
    {}
};

//CScriptToken ignoreResult (SResult r)
//{
//    CLEAN (r.varLink);
//    return r.token;
//}


//SResult buildVarResult (CScriptToken token, CScriptVar* var)
//{
//    return SResult (token, new CScriptVarLink(var));
//}

// ----------------------------------------------------------------------------------- CSCRIPTVARLINK
#if 0
CScriptVarLink::CScriptVarLink(CScriptVar *var, const std::string &name) {
#if DEBUG_MEMORY
    mark_allocated(this);
#endif
    this->name = name;
    this->nextSibling = 0;
    this->prevSibling = 0;
    this->var = var->ref();
    this->owned = false;
}

CScriptVarLink::CScriptVarLink(const CScriptVarLink &link) {
    // Copy constructor
#if DEBUG_MEMORY
    mark_allocated(this);
#endif
    this->name = link.name;
    this->nextSibling = 0;
    this->prevSibling = 0;
    this->var = link.var->ref();
    this->owned = false;
}

CScriptVarLink::~CScriptVarLink() {
#if DEBUG_MEMORY
    mark_deallocated(this);
#endif
    var->unref();
}

void CScriptVarLink::replaceWith(CScriptVar *newVar) {
    CScriptVar *oldVar = var;
    var = newVar->ref();
    oldVar->unref();
}

void CScriptVarLink::replaceWith(CScriptVarLink *newVar) {
    if (newVar)
      replaceWith(newVar->var);
    else
      replaceWith(new CScriptVar());
}

int CScriptVarLink::getIntName() {
    return atoi(name.c_str());
}
void CScriptVarLink::setIntName(int n) {
    char sIdx[64];
    sprintf_s(sIdx, sizeof(sIdx), "%d", n);
    name = sIdx;
}

// ----------------------------------------------------------------------------------- CSCRIPTVAR

CScriptVar::CScriptVar() {
    refs = 0;
#if DEBUG_MEMORY
    mark_allocated(this);
#endif
    init();
    flags = SCRIPTVAR_UNDEFINED;
}

CScriptVar::CScriptVar(const string &str) {
    refs = 0;
#if DEBUG_MEMORY
    mark_allocated(this);
#endif
    init();
    flags = SCRIPTVAR_STRING;
    data = str;
}


CScriptVar::CScriptVar(const string &varData, int varFlags) {
    refs = 0;
#if DEBUG_MEMORY
    mark_allocated(this);
#endif
    init();
    flags = varFlags;
    if (varFlags & SCRIPTVAR_INTEGER) {
      intData = strtol(varData.c_str(),0,0);
    } else if (varFlags & SCRIPTVAR_DOUBLE) {
      doubleData = strtod(varData.c_str(),0);
    } else
      data = varData;
}

CScriptVar::CScriptVar(double val) {
    refs = 0;
#if DEBUG_MEMORY
    mark_allocated(this);
#endif
    init();
    setDouble(val);
}

CScriptVar::CScriptVar(int val) {
    refs = 0;
#if DEBUG_MEMORY
    mark_allocated(this);
#endif
    init();
    setInt(val);
}

CScriptVar::~CScriptVar(void) {
#if DEBUG_MEMORY
    mark_deallocated(this);
#endif
    removeAllChildren();
}

void CScriptVar::init() {
    firstChild = 0;
    lastChild = 0;
    flags = 0;
    jsCallback = 0;
    jsCallbackUserData = 0;
    data = TINYJS_BLANK_DATA;
    intData = 0;
    doubleData = 0;
}

CScriptVar *CScriptVar::getReturnVar() {
    return getParameter(TINYJS_RETURN_VAR);
}

void CScriptVar::setReturnVar(CScriptVar *var) {
    findChildOrCreate(TINYJS_RETURN_VAR)->replaceWith(var);
}


CScriptVar *CScriptVar::getParameter(const std::string &name) {
    return findChildOrCreate(name)->var;
}

CScriptVarLink *CScriptVar::findChild(const string &childName) {
    CScriptVarLink *v = firstChild;
    while (v) {
        if (v->name.compare(childName)==0)
            return v;
        v = v->nextSibling;
    }
    return 0;
}

CScriptVarLink *CScriptVar::findChildOrCreate(const string &childName, int varFlags) {
    CScriptVarLink *l = findChild(childName);
    if (l) return l;

    return addChild(childName, new CScriptVar(TINYJS_BLANK_DATA, varFlags));
}

CScriptVarLink *CScriptVar::findChildOrCreateByPath(const std::string &path) {
  size_t p = path.find('.');
  if (p == string::npos)
    return findChildOrCreate(path);

  return findChildOrCreate(path.substr(0,p), SCRIPTVAR_OBJECT)->var->
            findChildOrCreateByPath(path.substr(p+1));
}

CScriptVarLink *CScriptVar::addChild(const std::string &childName, CScriptVar *child) {
  if (isUndefined()) {
    flags = SCRIPTVAR_OBJECT;
  }
    // if no child supplied, create one
    if (!child)
      child = new CScriptVar();

    CScriptVarLink *link = new CScriptVarLink(child, childName);
    link->owned = true;
    if (lastChild) {
        lastChild->nextSibling = link;
        link->prevSibling = lastChild;
        lastChild = link;
    } else {
        firstChild = link;
        lastChild = link;
    }
    return link;
}

CScriptVarLink *CScriptVar::addChildNoDup(const std::string &childName, CScriptVar *child) {
    // if no child supplied, create one
    if (!child)
      child = new CScriptVar();

    CScriptVarLink *v = findChild(childName);
    if (v) {
        v->replaceWith(child);
    } else {
        v = addChild(childName, child);
    }

    return v;
}

void CScriptVar::removeChild(CScriptVar *child) {
    CScriptVarLink *link = firstChild;
    while (link) {
        if (link->var == child)
            break;
        link = link->nextSibling;
    }
    ASSERT(link);
    removeLink(link);
}

void CScriptVar::removeLink(CScriptVarLink *link) {
    if (!link) return;
    if (link->nextSibling)
      link->nextSibling->prevSibling = link->prevSibling;
    if (link->prevSibling)
      link->prevSibling->nextSibling = link->nextSibling;
    if (lastChild == link)
        lastChild = link->prevSibling;
    if (firstChild == link)
        firstChild = link->nextSibling;
    delete link;
}

void CScriptVar::removeAllChildren() {
    CScriptVarLink *c = firstChild;
    while (c) {
        CScriptVarLink *t = c->nextSibling;
        delete c;
        c = t;
    }
    firstChild = 0;
    lastChild = 0;
}

CScriptVar *CScriptVar::getArrayIndex(int idx) {
    char sIdx[64];
    sprintf_s(sIdx, sizeof(sIdx), "%d", idx);
    CScriptVarLink *link = findChild(sIdx);
    if (link) return link->var;
    else return new CScriptVar(TINYJS_BLANK_DATA, SCRIPTVAR_NULL); // undefined
}

void CScriptVar::setArrayIndex(int idx, CScriptVar *value) {
    char sIdx[64];
    sprintf_s(sIdx, sizeof(sIdx), "%d", idx);
    CScriptVarLink *link = findChild(sIdx);

    if (link) {
      if (value->isUndefined())
        removeLink(link);
      else
        link->replaceWith(value);
    } else {
      if (!value->isUndefined())
        addChild(sIdx, value);
    }
}

int CScriptVar::getArrayLength() {
    int highest = -1;
    if (!isArray()) return 0;

    CScriptVarLink *link = firstChild;
    while (link) {
      if (isNumber(link->name)) {
        int val = atoi(link->name.c_str());
        if (val > highest) highest = val;
      }
      link = link->nextSibling;
    }
    return highest+1;
}

int CScriptVar::getChildren() {
    int n = 0;
    CScriptVarLink *link = firstChild;
    while (link) {
      n++;
      link = link->nextSibling;
    }
    return n;
}

int CScriptVar::getInt() {
    /* strtol understands about hex and octal */
    if (isInt()) return intData;
    if (isNull()) return 0;
    if (isUndefined()) return 0;
    if (isDouble()) return (int)doubleData;
    return 0;
}

double CScriptVar::getDouble() {
    if (isDouble()) return doubleData;
    if (isInt()) return intData;
    if (isNull()) return 0;
    if (isUndefined()) return 0;
    return 0; /* or NaN? */
}

const string &CScriptVar::getString() {
    /* Because we can't return a string that is generated on demand.
     * I should really just use char* :) */
    static string s_null = "null";
    static string s_undefined = "undefined";
    if (isInt()) {
      char buffer[32];
      sprintf_s(buffer, sizeof(buffer), "%ld", intData);
      data = buffer;
      return data;
    }
    if (isDouble()) {
      char buffer[32];
      sprintf_s(buffer, sizeof(buffer), "%f", doubleData);
      data = buffer;
      return data;
    }
    if (isNull()) return s_null;
    if (isUndefined()) return s_undefined;
    // are we just a string here?
    return data;
}

void CScriptVar::setInt(int val) {
    flags = (flags&~SCRIPTVAR_VARTYPEMASK) | SCRIPTVAR_INTEGER;
    intData = val;
    doubleData = 0;
    data = TINYJS_BLANK_DATA;
}

void CScriptVar::setDouble(double val) {
    flags = (flags&~SCRIPTVAR_VARTYPEMASK) | SCRIPTVAR_DOUBLE;
    doubleData = val;
    intData = 0;
    data = TINYJS_BLANK_DATA;
}

void CScriptVar::setString(const string &str) {
    // name sure it's not still a number or integer
    flags = (flags&~SCRIPTVAR_VARTYPEMASK) | SCRIPTVAR_STRING;
    data = str;
    intData = 0;
    doubleData = 0;
}

void CScriptVar::setUndefined() {
    // name sure it's not still a number or integer
    flags = (flags&~SCRIPTVAR_VARTYPEMASK) | SCRIPTVAR_UNDEFINED;
    data = TINYJS_BLANK_DATA;
    intData = 0;
    doubleData = 0;
    removeAllChildren();
}

void CScriptVar::setArray() {
    // name sure it's not still a number or integer
    flags = (flags&~SCRIPTVAR_VARTYPEMASK) | SCRIPTVAR_ARRAY;
    data = TINYJS_BLANK_DATA;
    intData = 0;
    doubleData = 0;
    removeAllChildren();
}

bool CScriptVar::equals(CScriptVar *v) {
    CScriptVar *resV = mathsOp(v, LEX_EQUAL);
    bool res = resV->getBool();
    delete resV;
    return res;
}

CScriptVar *CScriptVar::mathsOp(CScriptVar *b, int op) {
    CScriptVar *a = this;
    // Type equality check
    if (op == LEX_TYPEEQUAL || op == LEX_NTYPEEQUAL) {
      // check type first, then call again to check data
      bool eql = ((a->flags & SCRIPTVAR_VARTYPEMASK) ==
                  (b->flags & SCRIPTVAR_VARTYPEMASK));
      if (eql) {
        CScriptVar *contents = a->mathsOp(b, LEX_EQUAL);
        if (!contents->getBool()) eql = false;
        if (!contents->refs) delete contents;
      }
                 ;
      if (op == LEX_TYPEEQUAL)
        return new CScriptVar(eql);
      else
        return new CScriptVar(!eql);
    }
    // do maths...
    if (a->isUndefined() && b->isUndefined()) {
      if (op == LEX_EQUAL) return new CScriptVar(true);
      else if (op == LEX_NEQUAL) return new CScriptVar(false);
      else return new CScriptVar(); // undefined
    } else if ((a->isNumeric() || a->isUndefined()) &&
               (b->isNumeric() || b->isUndefined())) {
        if (!a->isDouble() && !b->isDouble()) {
            // use ints
            int da = a->getInt();
            int db = b->getInt();
            switch (op) {
                case '+': return new CScriptVar(da+db);
                case '-': return new CScriptVar(da-db);
                case '*': return new CScriptVar(da*db);
                case '/': return new CScriptVar(da/db);
                case '&': return new CScriptVar(da&db);
                case '|': return new CScriptVar(da|db);
                case '^': return new CScriptVar(da^db);
                case '%': return new CScriptVar(da%db);
                case LEX_EQUAL:     return new CScriptVar(da==db);
                case LEX_NEQUAL:    return new CScriptVar(da!=db);
                case '<':     return new CScriptVar(da<db);
                case LEX_LEQUAL:    return new CScriptVar(da<=db);
                case '>':     return new CScriptVar(da>db);
                case LEX_GEQUAL:    return new CScriptVar(da>=db);
                default: throw CScriptException("Operation "+getTokenStr(op)+" not supported on the Int datatype");
            }
        } else {
            // use doubles
            double da = a->getDouble();
            double db = b->getDouble();
            switch (op) {
                case '+': return new CScriptVar(da+db);
                case '-': return new CScriptVar(da-db);
                case '*': return new CScriptVar(da*db);
                case '/': return new CScriptVar(da/db);
                case LEX_EQUAL:     return new CScriptVar(da==db);
                case LEX_NEQUAL:    return new CScriptVar(da!=db);
                case '<':     return new CScriptVar(da<db);
                case LEX_LEQUAL:    return new CScriptVar(da<=db);
                case '>':     return new CScriptVar(da>db);
                case LEX_GEQUAL:    return new CScriptVar(da>=db);
                default: throw CScriptException("Operation "+getTokenStr(op)+" not supported on the Double datatype");
            }
        }
    } else if (a->isArray()) {
      /* Just check pointers */
      switch (op) {
           case LEX_EQUAL: return new CScriptVar(a==b);
           case LEX_NEQUAL: return new CScriptVar(a!=b);
           default: throw CScriptException("Operation "+getTokenStr(op)+" not supported on the Array datatype");
      }
    } else if (a->isObject()) {
          /* Just check pointers */
          switch (op) {
               case LEX_EQUAL: return new CScriptVar(a==b);
               case LEX_NEQUAL: return new CScriptVar(a!=b);
               default: throw CScriptException("Operation "+getTokenStr(op)+" not supported on the Object datatype");
          }
    } else {
       string da = a->getString();
       string db = b->getString();
       // use strings
       switch (op) {
           case '+':           return new CScriptVar(da+db, SCRIPTVAR_STRING);
           case LEX_EQUAL:     return new CScriptVar(da==db);
           case LEX_NEQUAL:    return new CScriptVar(da!=db);
           case '<':     return new CScriptVar(da<db);
           case LEX_LEQUAL:    return new CScriptVar(da<=db);
           case '>':     return new CScriptVar(da>db);
           case LEX_GEQUAL:    return new CScriptVar(da>=db);
           default: throw CScriptException("Operation "+getTokenStr(op)+" not supported on the string datatype");
       }
    }
    ASSERT(0);
    return 0;
}

void CScriptVar::copySimpleData(CScriptVar *val) {
    data = val->data;
    intData = val->intData;
    doubleData = val->doubleData;
    flags = (flags & ~SCRIPTVAR_VARTYPEMASK) | (val->flags & SCRIPTVAR_VARTYPEMASK);
}

void CScriptVar::copyValue(CScriptVar *val) {
    if (val) {
      copySimpleData(val);
      // remove all current children
      removeAllChildren();
      // copy children of 'val'
      CScriptVarLink *child = val->firstChild;
      while (child) {
        CScriptVar *copied;
        // don't copy the 'parent' object...
        if (child->name != TINYJS_PROTOTYPE_CLASS)
          copied = child->var->deepCopy();
        else
          copied = child->var;

        addChild(child->name, copied);

        child = child->nextSibling;
      }
    } else {
      setUndefined();
    }
}

CScriptVar *CScriptVar::deepCopy() {
    CScriptVar *newVar = new CScriptVar();
    newVar->copySimpleData(this);
    // copy children
    CScriptVarLink *child = firstChild;
    while (child) {
        CScriptVar *copied;
        // don't copy the 'parent' object...
        if (child->name != TINYJS_PROTOTYPE_CLASS)
          copied = child->var->deepCopy();
        else
          copied = child->var;

        newVar->addChild(child->name, copied);
        child = child->nextSibling;
    }
    return newVar;
}

void CScriptVar::trace(string indentStr, const string &name) {
    TRACE("%s'%s' = '%s' %s\n",
        indentStr.c_str(),
        name.c_str(),
        getString().c_str(),
        getFlagsAsString().c_str());
    string indent = indentStr+" ";
    CScriptVarLink *link = firstChild;
    while (link) {
      link->var->trace(indent, link->name);
      link = link->nextSibling;
    }
}

string CScriptVar::getFlagsAsString() {
  string flagstr = "";
  if (flags&SCRIPTVAR_FUNCTION) flagstr = flagstr + "FUNCTION ";
  if (flags&SCRIPTVAR_OBJECT) flagstr = flagstr + "OBJECT ";
  if (flags&SCRIPTVAR_ARRAY) flagstr = flagstr + "ARRAY ";
  if (flags&SCRIPTVAR_NATIVE) flagstr = flagstr + "NATIVE ";
  if (flags&SCRIPTVAR_DOUBLE) flagstr = flagstr + "DOUBLE ";
  if (flags&SCRIPTVAR_INTEGER) flagstr = flagstr + "INTEGER ";
  if (flags&SCRIPTVAR_STRING) flagstr = flagstr + "STRING ";
  return flagstr;
}

string CScriptVar::getParsableString() {
  // Numbers can just be put in directly
  if (isNumeric())
    return getString();
  if (isFunction()) {
    ostringstream funcStr;
    funcStr << "function (";
    // get list of parameters
    CScriptVarLink *link = firstChild;
    while (link) {
      funcStr << link->name;
      if (link->nextSibling) funcStr << ",";
      link = link->nextSibling;
    }
    // add function body
    funcStr << ") " << getString();
    return funcStr.str();
  }
  // if it is a string then we quote it
  if (isString())
    return getJSString(getString());
  if (isNull())
      return "null";
  return "undefined";
}

void CScriptVar::getJSON(ostringstream &destination, const string linePrefix) {
   if (isObject()) {
      string indentedLinePrefix = linePrefix+"  ";
      // children - handle with bracketed list
      destination << "{ \n";
      CScriptVarLink *link = firstChild;
      while (link) {
        destination << indentedLinePrefix;
        destination  << getJSString(link->name);
        destination  << " : ";
        link->var->getJSON(destination, indentedLinePrefix);
        link = link->nextSibling;
        if (link) {
          destination  << ",\n";
        }
      }
      destination << "\n" << linePrefix << "}";
    } else if (isArray()) {
      string indentedLinePrefix = linePrefix+"  ";
      destination << "[\n";
      int len = getArrayLength();
      if (len>10000) len=10000; // we don't want to get stuck here!

      for (int i=0;i<len;i++) {
        getArrayIndex(i)->getJSON(destination, indentedLinePrefix);
        if (i<len-1) destination  << ",\n";
      }

      destination << "\n" << linePrefix << "]";
    } else {
      // no children or a function... just write value directly
      destination << getParsableString();
    }
}


void CScriptVar::setCallback(JSCallback callback, void *userdata) {
    jsCallback = callback;
    jsCallbackUserData = userdata;
}

CScriptVar *CScriptVar::ref() {
    refs++;
    return this;
}

void CScriptVar::unref() {
    if (refs<=0) printf("OMFG, we have unreffed too far!\n");
    if ((--refs)==0) {
      delete this;
    }
}

int CScriptVar::getRefs() {
    return refs;
}
#endif

// ----------------------------------------------------------------------------------- CSCRIPT

CTinyJS::CTinyJS(): m_globals(JSObject::create())
{
    //l = 0;
    /*root = (new CScriptVar(TINYJS_BLANK_DATA, SCRIPTVAR_OBJECT))->ref();
    // Add built-in classes
    stringClass = (new CScriptVar(TINYJS_BLANK_DATA, SCRIPTVAR_OBJECT))->ref();
    arrayClass = (new CScriptVar(TINYJS_BLANK_DATA, SCRIPTVAR_OBJECT))->ref();
    objectClass = (new CScriptVar(TINYJS_BLANK_DATA, SCRIPTVAR_OBJECT))->ref();
    root->addChild("String", stringClass);
    root->addChild("Array", arrayClass);
    root->addChild("Object", objectClass);*/
}

CTinyJS::~CTinyJS() {
    //ASSERT(!l);
//    scopes.clear();
//    stringClass->unref();
//    arrayClass->unref();
//    objectClass->unref();
//    root->unref();

#if DEBUG_MEMORY
    show_allocated();
#endif
}

void CTinyJS::trace() {
    //TODO: Replace by an alternate implementation?
    //root->trace();
}

void CTinyJS::execute(const string &code) {
    //CScriptLex *oldLex = l;
    //vector<CScriptVar*> oldScopes = scopes;
    //l = new CScriptLex(code);
#ifdef TINYJS_CALL_STACK
    call_stack.clear();
#endif
//    scopes.clear();
//    scopes.push_back(root);
    
    try {
        bool execute = true;
        CScriptToken    token (code.c_str());
        
        token = token.next();
        while (!token.eof())
            token = statement(execute, token, m_globals.getPointer());
    } catch (const CScriptException &e) {
        ostringstream msg;
        msg << "Error " << e.what();
#ifdef TINYJS_CALL_STACK
        for (int i=(int)call_stack.size()-1;i>=0;i--)
          msg << "\n" << i << ": " << call_stack.at(i);
#endif
        //msg << " at " << l->getPosition();
        //delete l;
        //l = oldLex;

        throw CScriptException(msg.str());
    }
    //delete l;
    //l = oldLex;
    //scopes = oldScopes;
}

Ref<JSValue> CTinyJS::evaluateComplex(const string &code) {
    //CScriptLex *oldLex = l;
    //vector<CScriptVar*> oldScopes = scopes;

    //l = new CScriptLex(code);
#ifdef TINYJS_CALL_STACK
    call_stack.clear();
#endif
//    scopes.clear();
//    scopes.push_back(root);
//    CScriptVarLink* v = NULL;
    Ref<JSValue>    v;
    
    try {
        CScriptToken token (code.c_str());
        bool execute = true;
        
        token = token.next();
        do {
//          CLEAN(v);
          SResult r = base(execute, token, m_globals.getPointer());
          token = r.token;
          v = r.value;
          if (!token.eof()) 
              token = token.match(';');
        } while (!token.eof());
    } catch (const CScriptException &e) {
      ostringstream msg;
      msg << "Error " << e.what();
#ifdef TINYJS_CALL_STACK
      for (int i=(int)call_stack.size()-1;i>=0;i--)
        msg << "\n" << i << ": " << call_stack.at(i);
#endif
      //msg << " at " << l->getPosition();
      //delete l;
      //l = oldLex;

        throw CScriptException(msg.str());
    }
    //delete l;
    //l = oldLex;
    //scopes = oldScopes;

//    if (v) {
//        CScriptVarLink r = *v;
//        CLEAN(v);
//        return r;
//    }
//    // return undefined...
//    return CScriptVarLink(new CScriptVar());
    
    if (!v.isNull())
        return v;
    else
        return undefined();
}

string CTinyJS::evaluate(const string &code) {
    return evaluateComplex(code)->toString();
}

CScriptToken CTinyJS::parseFunctionArguments(JSFunction *function, CScriptToken token) {
  token = token.match('(');
  
  while (token.type() != ')') {
      function->addParameter (token.text());
      //funcVar->addChildNoDup(token.text());
      token = token.match(LEX_ID);
      if (token.type() != ')')
          token = token.match(',');
  }
  return token.match(')');
}

void CTinyJS::addNative(const string &funcDesc, JSNativeFn ptr, void *userdata) {
    CScriptToken    token (funcDesc.c_str());
    token = token.next();

    IScope *scope = m_globals.getPointer();

    token = token.match(LEX_R_FUNCTION);
    string funcName = token.text();
    token = token.match(LEX_ID);
    
    /* Check for dots, we might want to do something like function String.substring ... */
    while (token.type() == '.') {
      token = token.match('.');
      //CScriptVarLink *link = base->findChild(funcName);
      Ref<JSObject> child = getObject (scope, funcName);
      // if it doesn't exist, make an object class
      if (child.isNull()){
          child = JSObject::create();
          scope->set(funcName, child);
      }

      scope = child.getPointer();
      funcName = token.text();
      token = token.match(LEX_ID);
    }

    Ref<JSFunction>     function = JSFunction::createNative (funcName, ptr);
    parseFunctionArguments(function.getPointer(), token);

    scope->set(funcName, function);
}

/**
 * Gets the value of a global symbol
 * @param name
 * @return 
 */
Ref<JSValue> CTinyJS::getGlobal(const std::string& name)const
{
    return m_globals->get(name);
}

/**
 * Sets the value of a global symbol, or creates it if not present.
 * @param name
 * @param value
 * @return 
 */
Ref<JSValue> CTinyJS::setGlobal(const std::string& name, Ref<JSValue> value)
{
    return m_globals->set (name, value);
}

/**
 * Dumps JSON symbols to a stream
 * @param output
 */
string CTinyJS::dumpJSONSymbols ()
{
    return m_globals->getJSON();
}



SResult CTinyJS::parseFunctionDefinition(CScriptToken token, IScope* pScope)
{
  // actually parse a function...
    token = token.match(LEX_R_FUNCTION);
    string funcName = TINYJS_TEMP_NAME;
    
    /* we can have functions without names */
    if (token.type()==LEX_ID) {
      funcName = token.text();
      token = token.match(LEX_ID);
    }
    Ref<JSFunction> funcVar = JSFunction::createJS(funcName);
    token = parseFunctionArguments(funcVar.getPointer(), token);

    //const char* blockStart = token.code();
    funcVar->setCode (token);

    bool noexecute = false;
    token = block(noexecute, token, pScope);

    //Add function code (the contents of the block)
    //TODO: Executing functions in this way, causes line number information to be wrong
    //(Besides being quite inefficient)
    //funcVar->var->data = string (blockStart, token.code());
    
    return SResult(token, funcVar);
}
/** Handle a function call (assumes we've parsed the function name and we're
 * on the start bracket). 'parent' is the object that contains this method,
 * if there was one (otherwise it's just a normal function).
 */
SResult CTinyJS::functionCall(bool &execute, Ref<JSValue> fnValue, Ref<JSValue> parent, CScriptToken token, IScope* pScope)
{
    if (execute)
    {
        if (!fnValue->isFunction())
        {
            errorAt (token.getPosition(), "Function expected, found '%s'", fnValue->getTypeName().c_str());
        }
        
        Ref<JSFunction>     function = fnValue.staticCast<JSFunction>();
        
        //Create function scope
        FunctionScope fnScope (m_globals.getPointer(), function);
        
        fnScope.setThis(parent);
        
        token = token.match('(');
        // create a new symbol table entry for execution of this function
//        CScriptVar *functionRoot = new CScriptVar(TINYJS_BLANK_DATA, SCRIPTVAR_FUNCTION);
//        if (parent)
//            functionRoot->addChildNoDup("this", parent);
//        // grab in all parameters
//        CScriptVarLink *v = function->var->firstChild;
        while (token.type() != ')')
        {
            SResult r = base(execute, token, pScope);
            token = r.token;
            if (execute)
                fnScope.addParam (r.value);
//            {
//                if (r.varLink->var->isBasic())
//                {
//                    // pass by value
//                    functionRoot->addChild(v->name, r.varLink->var->deepCopy());
//                }
//                else
//                {
//                    // pass by reference
//                    functionRoot->addChild(v->name, r.varLink->var);
//                }
//            }
//            CLEAN(r.varLink);
            if (token.type() != ')')
                token = token.match(',');
            //TODO: Looks that it doesn't support calling a function with a different number of parameters
            //from the declaration argument list. Investigate and fix it, if needed.
            //v = v->nextSibling;
        }//while
        token = token.match(')');
        // setup a return variable
//        CScriptVarLink *returnVar = NULL;
//        // execute function!
//        // add the function's execute space to the symbol table so we can recurse
//        CScriptVarLink *returnVarLink = functionRoot->addChild(TINYJS_RETURN_VAR);
//        scopes.push_back(functionRoot);
#ifdef TINYJS_CALL_STACK
        call_stack.push_back(function->getName() + " from " + token.getPosition().toString());
#endif
        SResult r (token, Ref<JSValue>());
        
        if (function->isNative())
            r.value = function->nativePtr()(&fnScope);
        else {
            block (execute, function->codeBlock(), &fnScope);
            execute = true;
            r.value = fnScope.getResult();
        }

//        if (function->isNative())
//        {
//            function->nativeCall(&fnScope);
////            ASSERT(function->var->jsCallback);
////            function->var->jsCallback(functionRoot, function->var->jsCallbackUserData);
//        }
//        else
//        {
//            // Use another lexer instance for function execution
//            CScriptToken execToken(function->var->getString().c_str());
//            execToken = execToken.next();
//
//            execToken = block(execute, execToken);
//            // because return will probably have called this, and set execute to false
//            execute = true;
//        }
#ifdef TINYJS_CALL_STACK
        if (!call_stack.empty()) call_stack.pop_back();
#endif
        return r;
//        scopes.pop_back();
//        /* get the real return var before we remove it from our function */
//        returnVar = new CScriptVarLink(returnVarLink->var);
//        functionRoot->removeLink(returnVarLink);
//        delete functionRoot;
//        if (returnVar)
//            return SResult(token, returnVar);
//        else
//            return buildVarResult(token, new CScriptVar());
    }
    else
    {
        // function, but not executing - just parse args and be done
        token = token.match('(');
        while (token.type() != ')')
        {
            token = base(execute, token, pScope).token;

            if (token.type() != ')')
                token = token.match(',');
        }
        token = token.match(')');
        if (token.type() == '{')
        { // TODO: why is this here? --> Si no lo sabes tú... Aunque tampoco me extraña mucho.
            token = block(execute, token, pScope);
        }
        /* function will be a blank scriptvarlink if we're not executing,
         * so just return it rather than an alloc/free */
        return SResult(token, fnValue);
    }
}


SResult CTinyJS::factor(bool &execute, CScriptToken token, IScope* pScope) {
    if (token.type()=='(') {
        token = token.match('(');
        SResult a = base(execute, token, pScope );
        a.token = a.token.match(')');
        return a;
    }
    if (token.type()==LEX_R_TRUE) {
        token = token.match(LEX_R_TRUE);
        //return buildVarResult (token, new CScriptVar(1));
        return SResult (token, jsTrue());
    }
    if (token.type()==LEX_R_FALSE) {
        token = token.match(LEX_R_FALSE);
        //return buildVarResult (token, new CScriptVar(0));
        return SResult (token, jsFalse());
    }
    if (token.type()==LEX_R_NULL) {
        token = token.match(LEX_R_NULL);
        //return buildVarResult (token, new CScriptVar(TINYJS_BLANK_DATA,SCRIPTVAR_NULL));
        return SResult (token, jsNull());
    }
    if (token.type()==LEX_R_UNDEFINED) {
        token = token.match(LEX_R_UNDEFINED);
        //return buildVarResult (token, new CScriptVar(TINYJS_BLANK_DATA,SCRIPTVAR_UNDEFINED));
        return SResult (token, undefined());
    }
    if (token.type()==LEX_ID) {
        Ref<JSValue>    a;
        
        if (execute)
            a = JSReference::create(pScope, token.text());
        //CScriptVarLink *a = execute ? findInScopes(token.text()) : new CScriptVarLink(new CScriptVar());
        //printf("0x%08X for %s at %s\n", (unsigned int)a, token.type()Str.c_str(), l->getPosition().c_str());
        /* The parent if we're executing a method call */
        Ref<JSValue> parent;

        if (execute && a.isNull()) {
          /* Variable doesn't exist! JavaScript says we should create it
           * (we won't add it here. This is done in the assignment operator)*/
          //a = new CScriptVarLink(new CScriptVar(), token.text());
            a = pScope->set(token.text(), jsNull());        //TODO: null or undefined? Is this necessary
        }
        token = token.match(LEX_ID);
        while (token.type()=='(' || token.type()=='.' || token.type()=='[') {
            if (token.type()=='(') { // ------------------------------------- Function Call
                SResult r = functionCall(execute, a, parent, token, pScope);
                a = r.value;
                token = r.token;
            } else if (token.type() == '.') { // ------------------------------------- Record Access
                token = token.match('.');
                if (execute) {
                  string name = token.text();
                  //CScriptVarLink *child = a->var->findChild(name);
                  //TODO: member access should return a 'JSReference'. Think of a better way.
                  Ref<JSValue> child = a->memberAccess(name);
                  
                  if (child.isNull())
                      child = findInParentClasses(a, name);
                  //if (!child) {
                   
                    /* if we haven't found this defined yet, use the built-in
                       'length' properly */
                    /*if (a->var->isArray() && name == "length") {
                      int l = a->var->getArrayLength();
                      child = new CScriptVarLink(new CScriptVar(l));
                    } else if (a->var->isString() && name == "length") {
                      int l = a->var->getString().size();
                      child = new CScriptVarLink(new CScriptVar(l));
                    } else {
                      child = a->var->addChild(name);
                    }*/
                  //}
                  parent = a;
                  a = child;
                }
                token = token.match(LEX_ID);
            } else if (token.type() == '[') { // ------------------------------------- Array Access
                token = token.match('[');
                SResult indexRes = base(execute, token, pScope);
                token = indexRes.token.match(']');
                
                //TODO: resolve better left references for array, because it can be a read or write, which
                //must be handled differently
                if (execute) {
                    //CScriptVarLink *child = a->var->findChildOrCreate(indexRes.varLink->var->getString());
                    Ref<JSValue>    child = a->arrayAccess(indexRes.value);
                    parent = a;
                    a = child;
                }
                //CLEAN(indexRes.varLink);*/
            } else ASSERT(0);
        }
        return SResult (token, a);
    }
    if (token.type()==LEX_INT || token.type()==LEX_FLOAT || token.type()==LEX_STR) {
//        CScriptVar *a = new CScriptVar(token.text(),
//            ((token.type()==LEX_INT)?SCRIPTVAR_INTEGER:SCRIPTVAR_DOUBLE));
//        token = token.next();
//        return buildVarResult(token, a);
        return SResult(token.next(), createConstant(token));
    }
    /*if (token.type()==LEX_STR) {
        CScriptVar *a = new CScriptVar(token.strValue(), SCRIPTVAR_STRING);
        token = token.match(LEX_STR);
        return buildVarResult(token, a);
    }*/
    if (token.type()=='{') {
        //CScriptVar *contents = new CScriptVar(TINYJS_BLANK_DATA, SCRIPTVAR_OBJECT);
        Ref<JSObject>   contents = JSObject::create();
                
        /* JSON-style object definition */
        token = token.match('{');
        while (token.type() != '}') {
          string id;
          // we only allow strings or IDs on the left hand side of an initialisation
          if (token.type()==LEX_STR){              
              id = token.strValue();
              token = token.match(LEX_STR);
          }
          else{
              id = token.text();
              token = token.match(LEX_ID);
          }
          token = token.match(':');
          if (execute) {
            SResult r = base(execute, token, pScope);
            //contents->addChild(id, r.varLink->var);
            contents->set(id, r.value);
            token = r.token;
            //CLEAN(r.varLink);
          }
          // no need to clean here, as it will definitely be used
          if (token.type() != '}')
              token = token.match(',');
        }

        token = token.match('}');
        return SResult(token, contents);
    }
    if (token.type()=='[') {
        Ref<JSArray>    contents = JSArray::create();
        //CScriptVar *contents = new CScriptVar(TINYJS_BLANK_DATA, SCRIPTVAR_ARRAY);
        /* JSON-style array */
        token = token.match('[');
        //int idx = 0;
        while (token.type() != ']') {
          if (execute) {
//            char idx_str[16]; // big enough for 2^32
//            sprintf_s(idx_str, sizeof(idx_str), "%d",idx);

            SResult r = base(execute, token, pScope);
            //contents->addChild(idx_str, r.varLink->var);
            contents->push (r.value);
            token = r.token;
            //CLEAN(r.varLink);
          }
          // no need to clean here, as it will definitely be used
          if (token.type() != ']')
              token = token.match(',');
          //idx++;
        }
        token = token.match(']');
        return SResult(token, contents);
    }
    if (token.type()==LEX_R_FUNCTION) {
        SResult r = parseFunctionDefinition(token, pScope);
//        if (r.varLink->name != TINYJS_TEMP_NAME)
//          TRACE("Functions not defined at statement-level are not meant to have a name");
        return r;
    }
    if (token.type()==LEX_R_NEW) {
      // new -> create a new object
      token = token.match(LEX_R_NEW);
      string className = token.text();
      //if (execute) {
        //CScriptVarLink *objClassOrFunc = findInScopes(className);
        Ref<JSValue> constructor = pScope->get(className);
        if (constructor.isNull()) {
          TRACE("%s is not a valid class name", className.c_str());
          return SResult(token, undefined());
        }
        //TODO: Doesn't support a constructor member of an object. IE: 'a = new Text.Parser'
        token = token.match(LEX_ID);
        //CScriptVar *obj = new CScriptVar(TINYJS_BLANK_DATA, SCRIPTVAR_OBJECT);
        Ref<JSObject>   obj = JSObject::create();

        if (constructor->isFunction()) {
            //TODO: Support 'new' invoke without neither parameters, nor parenthesis.
            SResult r = functionCall(execute, constructor, obj, token, pScope);
            token = r.token;
            //CLEAN(r.varLink);
            return r;
        } else {
            errorAt (token.getPosition(), "'%s' is not a constructor", token.text().c_str());
            //TODO: Review. I don't understand the semantics
          //obj->addChild(TINYJS_PROTOTYPE_CLASS, objClassOrFunc->var);
//          if (token.type() == '(') {
//            token = token.match('(');
//            token = token.match(')');
//          }
        }
        return SResult(token, obj);
//      } else {
//        token = token.match(LEX_ID);
//        if (token.type() == '(') {
//            return functionCall(execute, objClassOrFunc, obj, token);
////          token = token.match('(');
////          //TODO: support new parameters!!!
////          token = token.match(')');
//        }
//      }
    }
    // Nothing we can do here... just hope it's the end...
    token = token.match(LEX_EOF);
    return SResult (token, NULL);
}

SResult CTinyJS::unary(bool &execute, CScriptToken token, IScope* pScope) {
    if (token.type()=='!') {
        token = token.match('!'); // binary not
        SResult r = factor(execute, token, pScope);
        if (execute) {
            const bool res = !r.value->toBoolean();
            r.value = jsBool(res);
//            CScriptVar zero(0);
//            CScriptVar *res = r.varLink->var->mathsOp(&zero, LEX_EQUAL);
//            CREATE_LINK(r.varLink, res);
        }
        
        return r;
    } else
        return factor(execute, token, pScope);
}


SResult CTinyJS::term(bool &execute, CScriptToken token, IScope* pScope) {
    SResult ra = unary(execute, token, pScope);
    
    token = ra.token;
    while (token.type()=='*' || token.type()=='/' || token.type()=='%') {
        const int op = token.type();
        token = token.next();
        SResult rb = unary(execute, token, pScope);
        token = rb.token;
        if (execute) {
            Ref<JSValue> result = jsOperator (op, ra.value, rb.value);
//            CScriptVar *res = r.varLink->var->mathsOp(innerResult.varLink->var, op);
//            CREATE_LINK(r.varLink, res);
            return SResult (token, result);
        }
        //CLEAN(innerResult.varLink);
    }
    
    ra.token = token;
    return SResult (token, undefined());
}

SResult CTinyJS::expression(bool &execute, CScriptToken token, IScope* pScope) {
    bool negate = false;
    if (token.type()=='-') {
        token = token.match('-');
        negate = true;
    }
    SResult r = term(execute, token, pScope);
    token = r.token;
    if (negate) {
        r.value = jsOperator('-', jsInt(0), r.value);
//        CScriptVar zero(0);
//        CScriptVar *res = zero.mathsOp(r.varLink->var, '-');
//        CREATE_LINK(r.varLink, res);
    }

    while (token.type()=='+' || token.type()=='-' ||
        token.type()==LEX_PLUSPLUS || token.type()==LEX_MINUSMINUS) {
        const int op = token.type();
        token = token.next();
        if (op==LEX_PLUSPLUS || op==LEX_MINUSMINUS) {
            //TODO: rethink how to handle it. It should support prefix and postfix operators
//            if (execute) {
//                CScriptVar one(1);
//                CScriptVar *res = r.varLink->var->mathsOp(&one, op==LEX_PLUSPLUS ? '+' : '-');
//                CScriptVarLink *oldValue = new CScriptVarLink(r.varLink->var);
//                // in-place add/subtract
//                r.varLink->replaceWith(res);
//                CLEAN(r.varLink);
//                r.varLink = oldValue;
//            }
        } else {
            SResult rb = term(execute, token, pScope);
            token = rb.token;
            if (execute) {
                // not in-place, so just replace
                r.value = jsOperator(op, r.value, rb.value);
//                CScriptVar *res = r.varLink->var->mathsOp(rb.varLink->var, op);
//                CREATE_LINK(r.varLink, res);
            }
            //CLEAN(rb.varLink);
        }
    }
    
    r.token = token;
    return r;
}

SResult CTinyJS::shift(bool &execute, CScriptToken token, IScope* pScope) {
  SResult ra = expression(execute, token, pScope);
  token = ra.token;
  
  if (token.type()==LEX_LSHIFT || token.type()==LEX_RSHIFT || token.type()==LEX_RSHIFTUNSIGNED) {
    int op = token.type();
    token = token.match(op);
    SResult rb = base(execute, token, pScope);
    token = rb.token;
    ra.value = jsOperator(op, ra.value, rb.value);
//    int shift = execute ? rb.value->toInt32() : 0;
//    //CLEAN(rb.varLink);
//    if (execute) {
//        CScriptVarLink *a = ra.varLink;
//        
//        if (op==LEX_LSHIFT) 
//            a->var->setInt(a->var->getInt() << shift);
//        if (op==LEX_RSHIFT) 
//            a->var->setInt(a->var->getInt() >> shift);
//        if (op==LEX_RSHIFTUNSIGNED) 
//            a->var->setInt(((unsigned int)a->var->getInt()) >> shift);
//    }
  }
  
  ra.token = token;
  return ra;
}

SResult CTinyJS::condition(bool &execute, CScriptToken token, IScope* pScope) {
    SResult ra = shift(execute, token, pScope);
    token = ra.token;
    
    while (token.type()==LEX_EQUAL || token.type()==LEX_NEQUAL ||
           token.type()==LEX_TYPEEQUAL || token.type()==LEX_NTYPEEQUAL ||
           token.type()==LEX_LEQUAL || token.type()==LEX_GEQUAL ||
           token.type()=='<' || token.type()=='>') {
        int op = token.type();
        token = token.next();
        
        SResult rb = shift(execute, token, pScope);
        token = rb.token;
        
        if (execute) {
            ra.value = jsOperator(op, ra.value, rb.value);
//            CScriptVar *res = ra.varLink->var->mathsOp(rb.varLink->var, op);
//            CREATE_LINK(ra.varLink,res);
        }
//        CLEAN(rb.varLink);
    }
    
    ra.token = token;
    return ra;
}

SResult CTinyJS::logic(bool &execute, CScriptToken token, IScope* pScope) {
    SResult ra = condition(execute, token, pScope);
    token = ra.token;
    
    //TODO: Not sure if this code is correct. Mixes bitwise and logical operations
    
    while (token.type()=='&' || token.type()=='|' || token.type()=='^' || token.type()==LEX_ANDAND || token.type()==LEX_OROR) {
        bool noexecute = false;
        int op = token.type();
        token = token.match(op);
        bool shortCircuit = false;
        //bool boolean = false;
        // if we have short-circuit ops, then if we know the outcome
        // we don't bother to execute the other op. Even if not
        // we need to tell mathsOp it's an & or |
        if (op==LEX_ANDAND) {
            op = '&';
            shortCircuit = !ra.value->toBoolean();
            //boolean = true;
        } else if (op==LEX_OROR) {
            op = '|';
            shortCircuit = ra.value->toBoolean();
            //boolean = true;
        }
        SResult rb = condition(shortCircuit ? noexecute : execute, token, pScope);
        token = rb.token;
        
        if (execute && !shortCircuit) {
            //This code never executes
//            if (boolean) {
//              CScriptVar *newa = new CScriptVar(ra.varLink->var->getBool());
//              CScriptVar *newb = new CScriptVar(rb.varLink->var->getBool());
//              CREATE_LINK(ra.varLink, newa);
//              CREATE_LINK(rb.varLink, newb);
//            }
//            CScriptVar *res = ra.varLink->var->mathsOp(rb.varLink->var, op);
//            CREATE_LINK(ra.varLink, res);
            ra.value = jsOperator(op, ra.value, rb.value);
        }
        //CLEAN(rb.varLink);
    }
    
    ra.token = token;
    return ra;
}

SResult CTinyJS::ternary(bool &execute, CScriptToken token, IScope* pScope) {
  SResult lhsRes = logic(execute, token, pScope);
  token = lhsRes.token;
  
  bool noexec = false;
  if (token.type()=='?') {
    token = token.match('?');
    if (!execute) {
        SResult    rhsRes = base(noexec, token, pScope);
        token = rhsRes.token;
//        CLEAN(lhsRes.varLink);
//        CLEAN(rhsRes.varLink);
        token = token.match(':');
        
        rhsRes = base(noexec, token, pScope);
        token = rhsRes.token;
//        CLEAN(rhsRes.varLink);
    }
    else {
      bool first = lhsRes.value->toBoolean();
//      CLEAN(lhsRes.varLink);
      if (first) {
        lhsRes = base(execute, token, pScope);
        token = lhsRes.token;
        token = token.match(':');
        
        token = base(noexec, token, pScope).token;
      } else {
        token = base(noexec, token, pScope).token;
        token = token.match(':');
        lhsRes = base(execute, token, pScope);
        token = lhsRes.token;
      }
    }
  }

  lhsRes.token = token;
  return lhsRes;
}

SResult CTinyJS::base(bool &execute, CScriptToken token, IScope* pScope) {
    //TODO: Ternary operator is an invalid left hand side...
    SResult lres = ternary(execute, token, pScope);
    token = lres.token;
    
    if (token.type()=='=' || token.type()==LEX_PLUSEQUAL || token.type()==LEX_MINUSEQUAL) {
        /* If we're assigning to this and we don't have a parent,
         * add it to the symbol table root as per JavaScript. */
//        if (execute && !lres.varLink->owned) {
//          if (lres.varLink->name.length()>0) {
//            CScriptVarLink *realLhs = root->addChildNoDup(lres.varLink->name, lres.varLink->var);
//            CLEAN(lres.varLink);
//            lres.varLink = realLhs;
//          } else
//            TRACE("Trying to assign to an un-named type\n");
//        }

        const CScriptToken  opToken = token;
        const int           op = opToken.type();
        token = token.next();
        SResult rres = base(execute, token, pScope);
        token = rres.token;
        
        if (execute) {
            if (!lres.value->isReference())
                errorAt (opToken.getPosition(), "Invalid left hand side in assignment");
            
            const Ref<JSReference>  lhsRef = lres.value.staticCast<JSReference>();
            Ref<JSValue>            r = rres.value;
            
            if (op=='=')
                r = rres.value;
            else if (op==LEX_PLUSEQUAL)
                r = jsOperator('+', lhsRef, rres.value);
            else if (op==LEX_MINUSEQUAL)
                r = jsOperator('-', lhsRef, rres.value);
            else 
                ASSERT(0);

            lhsRef->set (r);
        }
        //CLEAN(rres.varLink);
    }
    
    lres.token = token;
    return lres;
}

CScriptToken CTinyJS::block(bool &execute, CScriptToken token, IScope* pScope) {
    token = token.match('{');
    
    if (execute) {
        BlockScope  blScope (pScope);
        while (token.type() && token.type()!='}')
            token = statement(execute, token, &blScope);
        token = token.match('}');
    } else {
      // fast skip of blocks
        int brackets = 1;
        while (token.type() && brackets) {
            if (token.type() == '{') brackets++;
            if (token.type() == '}') brackets--;
            token = token.next();
        }
    }

    return token;
}

CScriptToken CTinyJS::statement(bool &execute, CScriptToken token, IScope* pScope)
{
    if (token.type()==LEX_ID ||
        token.type()==LEX_INT ||
        token.type()==LEX_FLOAT ||
        token.type()==LEX_STR ||
        token.type()=='-') {
        /* Execute a simple statement that only contains basic arithmetic... */
        token = base(execute, token, pScope).token;
        //token = token.match(';');
    } else if (token.type()=='{') {
        /* A block of code */
        token = block(execute, token, pScope);
    } else if (token.type()==';') {
        /* Empty statement - to allow things like ;;; */
        return token.match(';');
    } else if (token.type()==LEX_R_VAR) {
        /* variable creation. TODO - we need a better way of parsing the left
         * hand side. Maybe just have a flag called can_create_var that we
         * set and then we parse as if we're doing a normal equals.*/
        token = token.match(LEX_R_VAR);
        while (token.type() != ';') {
            const string varName = token.text();
            Ref<JSValue>    value = undefined();
            //CScriptVarLink *a = 0;
//            if (execute) {                
//                a = pScope->set(name, undefined());
//                //a = JSReference::create(a);
//            }
                //a = scopes.back()->findChildOrCreate(token.text());
            token = token.match(LEX_ID);
            //TODO: I believe that this is illegal javascript. Confirm and delete
            // now do stuff defined with dots
/*            while (token.type() == '.') {
                token = token.match('.');
                if (execute) {
                    Ref<JSValue>    parent = a;
                    CScriptVarLink *lastA = a;
                    a = lastA->var->findChildOrCreate(token.text());
                }
                token = token.match(LEX_ID);
            }*/
            // sort out initialiser
            if (token.type() == '=') {
                token = token.match('=');
                SResult r = base(execute, token, pScope);
                token = r.token;
                if (execute)
                    value = r.value;
                    //a->replaceWith(r.varLink);
                //CLEAN(r.varLink);
            }
            
            pScope->set(varName, value);            
            
            if (token.type() != ';')
                token = token.match(',');
        }       
        //token = token.match(';');
    } else if (token.type()==LEX_R_IF) {
        token = token.match(LEX_R_IF);
        token = token.match('(');
        SResult r = base(execute, token, pScope);
        token = r.token;
        token = token.match(')');
        bool cond = execute && r.value->toBoolean();
        //CLEAN(r.varLink);
        bool noexecute = false; // because we need to be abl;e to write to it
        token = statement(cond ? execute : noexecute, token, pScope);
        if (token.type()==LEX_R_ELSE) {
            token = token.match(LEX_R_ELSE);
            token = statement(cond ? noexecute : execute, token, pScope);
        }
    } else if (token.type()==LEX_R_WHILE) {
        token = whileLoop(execute, token, pScope);
    } else if (token.type()==LEX_R_FOR) {
        token = forLoop (execute, token, pScope);
    } else if (token.type()==LEX_R_RETURN) {
        CScriptToken    retToken = token;
        token = token.match(LEX_R_RETURN);
        Ref<JSValue> result;
        if (token.type() != ';') {
            SResult r = base (execute, token, pScope);
            token = r.token;
            result = r.value;
        }
        if (execute) {
            IScope* fnScope = pScope->getFunctionScope();
            //CScriptVarLink *resultVar = scopes.back()->findChild(TINYJS_RETURN_VAR);
            if (fnScope)
                ((FunctionScope*)fnScope)->setResult(result);
                //resultVar->replaceWith(result);
            else
                errorAt (retToken.getPosition(), "Illegal return statement. Not inside a function");
                //TRACE("RETURN statement, but not in a function.\n");
            execute = false;
        }
        //CLEAN(result);
        token = token.match(';');
    } else if (token.type()==LEX_R_FUNCTION) {
        const CScriptToken  fnToken = token;
        SResult r = parseFunctionDefinition(token, pScope);
        token = r.token;
        
        if (execute) {
            const string name = r.value.staticCast<JSFunction>()->getName();
            //TODO: Handle error in a different way (having unnamed / named function productions)
            if (name == TINYJS_TEMP_NAME)
                errorAt (fnToken.getPosition(), "Functions defined at statement-level are meant to have a name\n");
            else
                pScope->set(name, r.value);
                //scopes.back()->addChildNoDup(r.varLink->name, r.varLink->var);
        }
        //CLEAN(r.varLink);
    } else token = token.match(LEX_EOF);
    
    if (token.type() == ';')
        return statement (execute, token, pScope);
    else    
        return token;
}

/**
 * While loop parsing / execution
 * @param execute
 * @param token
 * @return 
 */
CScriptToken CTinyJS::whileLoop(bool &execute, CScriptToken token, IScope* pScope)
{
    token = token.match(LEX_R_WHILE);
    token = token.match('(');
    
    CScriptToken    condition = token;
    bool            conditionValue = true;
    
    while (conditionValue) {
        token = condition;      //Go back to evaluate condition
        
        SResult     c = base (execute, token, pScope);
        token = c.token;
        
        conditionValue = execute && c.value->toBoolean();
        //CLEAN (c.varLink);
        
        token = token.match(')');

        bool    bodyExec = conditionValue;
        token = statement(bodyExec, token, pScope);
    }
    
    return token;
}

/**
 * 'for' loop parsing / execution
 * @param execute
 * @param token
 * @return 
 */
CScriptToken CTinyJS::forLoop(bool &execute, CScriptToken token, IScope* pScope)
{
    bool noExec = false;
    
    //First part: find for loop parts
    token = token.match(LEX_R_FOR);

    const CScriptToken init = token.match('(');
    
    const CScriptToken condition = statement (noExec, init, pScope);
    token = base (noExec, condition, pScope).token;
    
    const CScriptToken increment = token.match(';');
    token = statement (noExec, increment, pScope);
    
    const CScriptToken body = token.match(')');
    const CScriptToken nextToken = statement(noExec, body, pScope);
    
    //Second part: execute if requested
    if (execute) {
        bool conditionValue = true;
        
        statement(execute, init, pScope);
        
        while (conditionValue) {
            SResult r = base (execute, condition, pScope);
            
            conditionValue = r.value->toBoolean();
            //CLEAN (r.varLink);
            
            if (conditionValue) {
                statement (execute, body, pScope);
                statement (execute, increment, pScope);
            }            
        }        
    }//if (execute)
    
    return nextToken;
}

/// Get the given variable specified by a path (var1.var2.etc), or return 0
/*CScriptVar *CTinyJS::getScriptVariable(const string &path) {
    // traverse path
    size_t prevIdx = 0;
    size_t thisIdx = path.find('.');
    if (thisIdx == string::npos) thisIdx = path.length();
    CScriptVar *var = root;
    while (var && prevIdx<path.length()) {
        string el = path.substr(prevIdx, thisIdx-prevIdx);
        CScriptVarLink *varl = var->findChild(el);
        var = varl?varl->var:0;
        prevIdx = thisIdx+1;
        thisIdx = path.find('.', prevIdx);
        if (thisIdx == string::npos) thisIdx = path.length();
    }
    return var;
}*/

/// Get the value of the given variable, or return 0
/*const string *CTinyJS::getVariable(const string &path) {
    CScriptVar *var = getScriptVariable(path);
    // return result
    if (var)
        return &var->getString();
    else
        return 0;
}*/

/// set the value of the given variable, return trur if it exists and gets set
/*bool CTinyJS::setVariable(const std::string &path, const std::string &varData) {
    CScriptVar *var = getScriptVariable(path);
    // return result
    if (var) {
        if (var->isInt())
            var->setInt((int)strtol(varData.c_str(),0,0));
        else if (var->isDouble())
            var->setDouble(strtod(varData.c_str(),0));
        else
            var->setString(varData.c_str());
        return true;
    }    
    else
        return false;
}*/

/// Finds a child, looking recursively up the scopes
/*CScriptVarLink *CTinyJS::findInScopes(const std::string &childName) {
    for (int s=scopes.size()-1;s>=0;s--) {
      CScriptVarLink *v = scopes[s]->findChild(childName);
      if (v) return v;
    }
    return NULL;

}*/

/// Look up in any parent classes of the given object
Ref<JSValue> CTinyJS::findInParentClasses(Ref<JSValue> object, const std::string &name) {
    //TODO: Refactor
    return jsNull();
    
    /*// Look for links to actual parent classes
    CScriptVarLink *parentClass = object->findChild(TINYJS_PROTOTYPE_CLASS);
    while (parentClass) {
      CScriptVarLink *implementation = parentClass->var->findChild(name);
      if (implementation) return implementation;
      parentClass = parentClass->var->findChild(TINYJS_PROTOTYPE_CLASS);
    }
    // else fake it for strings and finally objects
    if (object->isString()) {
      CScriptVarLink *implementation = stringClass->findChild(name);
      if (implementation) return implementation;
    }
    if (object->isArray()) {
      CScriptVarLink *implementation = arrayClass->findChild(name);
      if (implementation) return implementation;
    }
    CScriptVarLink *implementation = objectClass->findChild(name);
    if (implementation) return implementation;

    return 0;*/
}

