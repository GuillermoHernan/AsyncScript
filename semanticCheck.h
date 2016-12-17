/* 
 * File:   semanticCheck.h
 * Author: ghernan
 *
 * Semantic check code for AsyncScript
 * 
 * Created on December 17, 2016, 10:54 PM
 */

#ifndef SEMANTICCHECK_H
#define	SEMANTICCHECK_H

#pragma once

#include "ast.h"
#include <vector>

void semanticCheck(const AstNodeList& statements);

#endif	/* SEMANTICCHECK_H */

