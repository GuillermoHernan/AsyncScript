/* 
 * File:   jsParser.h
 * Author: ghernan
 * 
 * Parses Javascript into an Abstract Sintax Tree (AST) structure.
 *
 * Created on November 28, 2016, 10:36 PM
 */

#ifndef JSPARSER_H
#define	JSPARSER_H

#pragma once

#include "RefCountObj.h"
#include "TinyJS_Lexer.h"
#include "utils.h"
#include "JsVars.h"
#include "ast.h"
#include "parserResults.h"

struct ParseResult;

ParseResult parseStatement(CScriptToken token);


#endif	/* JSPARSER_H */
