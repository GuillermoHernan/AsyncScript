/* 
 * File:   executionScope.h
 * Author: ghernan
 * 
 * Code which manages the AsyncScript execution scopes
 * 
 * Created on January 2, 2017, 10:15 PM
 */

#ifndef EXECUTIONSCOPE_H
#define	EXECUTIONSCOPE_H

#pragma once

#include "jsVars.h"

class JSArray;

/**
 * Interface for name scopes. 
 * Having a common interface helps to define custom logic for each kind of scope.
 */
struct IScope : public RefCountObj
{
    virtual bool isDefined(const std::string& name)const = 0;
    virtual Ref<JSValue> get(const std::string& name)const = 0;
    virtual Ref<JSValue> set(const std::string& name, Ref<JSValue> value) = 0;
    virtual Ref<JSValue> newVar(const std::string& name, Ref<JSValue> value, bool isConst) = 0;
    virtual bool isBlockScope()const = 0;
    
protected:
    virtual ~IScope(){};
};


/**
 * Name scope for a block of code.
 * It is also used for the global scope.
 * 
 * @param parent Parent scope. If a symbol is not found in the current scope,
 * it looks in the parent scope.
 */
class BlockScope : public IScope
{
public:
    static Ref<BlockScope> create(Ref<IScope> parent)
    {
        return refFromNew(new BlockScope(parent));
    }

    virtual bool isDefined(const std::string& name)const;
    virtual Ref<JSValue> get(const std::string& name)const;
    virtual Ref<JSValue> set(const std::string& name, Ref<JSValue> value);
    virtual Ref<JSValue> newVar(const std::string& name, Ref<JSValue> value, bool isConst);
    virtual bool isBlockScope()const
    {
        return true;
    }

private:
    BlockScope(Ref<IScope> parent);
    
    Ref<IScope> m_pParent;

    VarMap  m_symbols;
};

/**
 * Local scope of a function
 * @param globals
 * @param targetFn
 */
class FunctionScope : public IScope
{
public:
    static Ref<FunctionScope> create(Ref<IScope> globals, Ref<JSValue> targetFn)
    {
        return refFromNew(new FunctionScope(globals, targetFn));
    }

    void setThis(Ref<JSValue> value)
    {
        m_this = value;
    }

    Ref<JSValue> getThis()const
    {
        return m_this;
    }

    int addParam(Ref<JSValue> value);
    
    Ref<JSValue> getParam(const std::string& name)const;

    // IScope
    ////////////////////////////////////
    virtual bool isDefined(const std::string& name)const;
    virtual Ref<JSValue> get(const std::string& name)const;
    virtual Ref<JSValue> set(const std::string& name, Ref<JSValue> value);
    virtual Ref<JSValue> newVar(const std::string& name, Ref<JSValue> value, bool isConst);
    virtual bool isBlockScope()const
    {
        return false;
    }
    ////////////////////////////////////

    Ref<IScope> getGlobals()const
    {
        return m_globals;
    }
    
    Ref<JSValue> getFunction()const
    {
        return m_function;
    }

private:
    VarMap          m_params;
    Ref<JSValue>    m_function;
    Ref<JSArray>    m_arguments;
    Ref<JSValue>    m_this;
    Ref<IScope>     m_globals;
    
    FunctionScope(Ref<IScope> globals, Ref<JSValue> targetFn);
};

/**
 * Class to manage the global scope.
 * @param name
 * @return 
 */
class GlobalScope : public IScope
{
public:
    static Ref<GlobalScope> create()
    {
        return refFromNew(new GlobalScope);
    }
    
    // IScope
    /////////////////////////////////
    virtual bool isDefined(const std::string& name)const;
    virtual Ref<JSValue> get(const std::string& name)const;
    virtual Ref<JSValue> set(const std::string& name, Ref<JSValue> value);
    virtual Ref<JSValue> newVar(const std::string& name, Ref<JSValue> value, bool isConst);
    virtual Ref<JSValue> deleteVar(const std::string& name);
    virtual bool isBlockScope()const
    {
        return false;
    }
    /////////////////////////////////
    
    Ref<JSObject>       toObject();
    
    Ref<GlobalScope>    share();

    void newNotSharedVar(const std::string& name, Ref<JSValue> value, bool isConst);
    
private:
    /**
     * Envelope class to share objects between globals scopes. It is just
     * a reference-counted envelope over a variables map.
     * Only 'deep-frozen' objects stored at global scope are shareable.
     */
    struct SharedVars : public RefCountObj
    {
        VarMap  vars;
    };
    
    GlobalScope();
    GlobalScope(Ref<SharedVars> shared);
    
    void copyOnWrite();

    Ref<SharedVars>     m_shared;
    VarMap              m_notShared;
    bool                m_sharing;
};



#endif	/* EXECUTIONSCOPE_H */

