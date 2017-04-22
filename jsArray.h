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
class JSArray : public JSObject
{
public:
    static Ref<JSArray> create();
    static Ref<JSArray> create(size_t size);
    static Ref<JSArray> createStrArray(const StringVector& strList);
    static Ref<JSArray> fromVector(const ValueVector& values);

    size_t push(ASValue value);

    size_t length()const
    {
        return m_content.size();
    }

    ASValue getAt(size_t index)const;
    ASValue setAt(size_t index, ASValue value);
//
//    // JSValue
//    /////////////////////////////////////////
//    virtual std::string toString()const;
//
//    virtual std::string getJSON(int indent);
//    
//    virtual JSMutability getMutability()const
//    {
//        return m_mutability;
//    }
//
    virtual ASValue freeze();
    virtual ASValue deepFreeze(ASValue::ValuesMap& transformed);
    virtual ASValue unFreeze(bool forceClone=false);
//
//    virtual ASValue readField(const std::string& key)const;
//    virtual ASValue writeField(const std::string& key, ASValue value, bool isConst);
//    
//    virtual ASValue getAt(ASValue index);
//    virtual ASValue setAt(ASValue index, ASValue value);
//
    /////////////////////////////////////////

    static Ref<JSClass> ArrayClass;
    
    static std::string join(Ref<JSArray> arr, ASValue sep, ExecutionContext* ec);
private:

    JSArray() : JSObject(ArrayClass, MT_MUTABLE)
    {
    }
    
    void setLength(ASValue value);

private:
    std::vector< ASValue >      m_content;
//    JSMutability                m_mutability;
};

/**
 * Object to iterate over an array
 * @return 
 */
//class JSArrayIterator : public JSObject
//{
//public:
//    static ASValue create(Ref<JSArray> arr, size_t index);
//
//    virtual ASValue iterator()override
//    {
//        //TODO: temporary function
//        return jsNull();
//    }
//
//private:
//    //TODO: Should it has its own class?
//
//    JSArrayIterator(Ref<JSArray> arr, size_t index) :
//    JSObject(DefaultClass, arr->getMutability() == MT_DEEPFROZEN ? MT_DEEPFROZEN : MT_FROZEN),
//    m_array(arr),
//    m_index(index)
//    {
//    }
//
//    Ref<JSArray> m_array;
//    size_t m_index;
//};


#endif	/* JSARRAY_H */

