/* 
 * File:   jsArray.h
 * Author: ghernan
 * 
 * Asyncscript arrays runtime support code.
 *
 * Created on January 29, 2017, 2:24 PM
 */

#pragma once
#ifndef JSARRAY_H
#define	JSARRAY_H

#include "asObjects.h"
#include "microVM.h"


/**
 * Javascript array implementation
 */
class JSArray : public JSValueBase<VT_ARRAY>
{
public:
    static Ref<JSArray> create();
    static Ref<JSArray> create(size_t size);
    static Ref<JSArray> createStrArray(const StringVector& strList);
    static Ref<JSArray> fromVector(const ValueVector& values);

    size_t push(Ref<JSValue> value);

    size_t length()const
    {
        return m_content.size();
    }

    Ref<JSValue> getAt(size_t index)const;
    Ref<JSValue> setAt(size_t index, Ref<JSValue> value);

    // JSValue
    /////////////////////////////////////////
    virtual std::string toString()const;

    virtual std::string getJSON(int indent);
    
    virtual JSMutability getMutability()const
    {
        return m_mutability;
    }

    virtual Ref<JSValue> freeze();
    virtual Ref<JSValue> deepFreeze(JSValuesMap& transformed);
    virtual Ref<JSValue> unFreeze(bool forceClone=false);

    virtual Ref<JSValue> readField(const std::string& key)const;
    virtual Ref<JSValue> writeField(const std::string& key, Ref<JSValue> value, bool isConst);
    
    virtual Ref<JSValue> getAt(Ref<JSValue> index);
    virtual Ref<JSValue> setAt(Ref<JSValue> index, Ref<JSValue> value);

    /////////////////////////////////////////

    static Ref<JSClass> ArrayClass;
    
    static std::string join(Ref<JSArray> arr, Ref<JSValue> sep);
private:

    JSArray() : m_mutability(MT_MUTABLE)
    {
    }
    
    void setLength(Ref<JSValue> value);

private:
    std::vector< Ref<JSValue> >     m_content;
    JSMutability                    m_mutability;
};

/**
 * Object to iterate over an array
 * @return 
 */
class JSArrayIterator : public JSObject
{
public:
    static Ref<JSValue> create(Ref<JSArray> arr, size_t index);

    virtual Ref<JSValue> iterator()override
    {
        //TODO: temporary function
        return jsNull();
    }

private:
    //TODO: Should it has its own class?

    JSArrayIterator(Ref<JSArray> arr, size_t index) :
    JSObject(DefaultClass, arr->getMutability() == MT_DEEPFROZEN ? MT_DEEPFROZEN : MT_FROZEN),
    m_array(arr),
    m_index(index)
    {
    }

    Ref<JSArray> m_array;
    size_t m_index;
};


#endif	/* JSARRAY_H */

