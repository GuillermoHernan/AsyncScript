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
#include "jsLexer.h"
#include "utils.h"
#include "JsVars.h"
#include "ast.h"
#include "parserResults.h"

ParseResult parseStatement(CScriptToken token);


#endif	/* JSPARSER_H */
