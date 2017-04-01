#ifndef TINYJS_MATHFUNCTIONS_H
#define TINYJS_MATHFUNCTIONS_H
#pragma once

#include "jsVars.h"
#include "asObjects.h"

/// Register useful math. functions with the TinyJS interpreter
extern void registerMathFunctions(Ref<JSObject> scope);

#endif
