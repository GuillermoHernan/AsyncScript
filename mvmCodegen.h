/* 
 * File:   mvmCodegen.h
 * Author: ghernan
 * 
 * Code generator for micro VM,
 * 
 * Created on December 2, 2016, 9:59 PM
 */

#ifndef MVMCODEGEN_H
#define	MVMCODEGEN_H
#pragma once

#include "microVM.h"
#include "ast.h"
#include <vector>

Ref<MvmRoutine> scriptCodegen ( Ref<AstNode> script, CodeMap* pMap);



#endif	/* MVMCODEGEN_H */

