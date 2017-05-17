/* 
 * File:   modules.cpp
 * Author: ghernan
 * 
 * Module system for AsyncScript
 * 
 * Created on May 15, 2017, 11:09 PM
 */

#include "ascript_pch.hpp"
#include "modules.h"
#include "utils.h"
#include "microVM.h"
#include "scriptMain.h"
#include "ScriptException.h"

using namespace std;

/**
 * Normalizes a module path.
 * @param modulePath
 * @param ec
 * @return 
 */
std::string normalizeModulePath (const std::string& modulePath, ExecutionContext *ec)
{
    string result;
    
    if (isPathRelative(modulePath))
    {
        string base = dirFromPath( ec->modulePath );
        result = joinPaths (base, modulePath);
    }
    else
        result = modulePath;
    
    return normalizePath (result);
}

/**
 * Tries to find a module into the currently loaded module list.
 * Returns null if not found.
 * @param modulePath
 * @param ec
 * @return 
 */
ASValue findModule (const std::string& modulePath, ExecutionContext *ec)
{
    ASSERT (ec->modules != NULL);
    
    auto it = ec->modules->modules.find(modulePath);
    
    if (it != ec->modules->modules.end())
        return it->second;
    else
        return jsNull();
}

/**
 * Loads a new module, from its path in the file system. 
 * 
 * @param modulePath
 * @param ec
 * @return 
 */
ASValue loadModule (const std::string& modulePath, ExecutionContext *ec)
{
    string script = readTextFile(modulePath);
    
    if (script.empty())
        rtError ("Module '%s' not found", modulePath.c_str());
    
    auto globals = createDefaultGlobals();
    ec->modules->modules[modulePath] = globals->value();
    evaluate(script.c_str(), globals, modulePath, ec);
    
    return globals->value();
}

/**
 * Mix a module contents into a target module
 * @param target
 * @param source
 * @param ec
 */
void mixModule (ASValue target, ASValue source, ExecutionContext *ec)
{
    ASSERT (target.getType() == VT_OBJECT);
    ASSERT (source.getType() == VT_OBJECT);
    
    auto tgtObj = target.staticCast<JSObject>();
    auto srcObj = source.staticCast<JSObject>();
    
    StringSet srcFields = srcObj->getFields(false);
    
    for (auto it = srcFields.begin(); it != srcFields.end(); ++it)
    {
        if (srcObj->getFieldProperty(*it, "export").toBoolean(ec) == true)
            tgtObj->writeField(*it, srcObj->readField(*it), true);
    }
}
