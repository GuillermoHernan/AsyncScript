/* 
 * File:   RefCountObj.h
 * Author: ghernan
 * 
 * Reference counted objects and references implementation.
 *
 * Created on November 28, 2016, 6:58 PM
 */

#ifndef REFCOUNTOBJ_H
#define	REFCOUNTOBJ_H
#pragma once

#include <stdlib.h>

/**
 * Base class for reference counted objects.
 */
class RefCountObj
{
public:
    int addref()
    {
        return ++m_refCount;
    }

    void release()
    {
        if (--m_refCount == 0)
            delete this;
    }

protected:
    
    RefCountObj() : m_refCount(1)
    {        
    }
    
    virtual ~RefCountObj(){}

private:
    int m_refCount;
    
    //Copy operations forbidden
    RefCountObj(const RefCountObj& orig);
    RefCountObj& operator=(const RefCountObj& orig);
};


/**
 * Smart reference class, for reference-counted objects
 */
template <class ObjType>
class Ref
{
public:

    Ref() : m_ptr(NULL)
    {
    }

    Ref(ObjType* ptr) : m_ptr(ptr)
    {
        if (ptr != NULL)
            ptr->addref();
    }

    Ref(const Ref<ObjType>& src)
    {
        m_ptr = src.getPointer();

        if (m_ptr != NULL)
            m_ptr->addref();
    }

    template <class SrcType>
    Ref(const Ref<SrcType>& src)
    {
        m_ptr = src.getPointer();

        if (m_ptr != NULL)
            m_ptr->addref();
    }

    ~Ref()
    {
        if (m_ptr != NULL)
            m_ptr->release();
    }

    Ref<ObjType> & operator=(const Ref<ObjType>& src)
    {
        ObjType*  srcPtr = src.getPointer();

        if (srcPtr != NULL)
            srcPtr->addref();
        
        if (m_ptr != NULL)
            m_ptr->release();

        m_ptr = srcPtr;
        
        return *this;
    }

    template <class SrcType>
    Ref<ObjType> & operator=(const Ref<SrcType>& src)
    {
        return this->operator =(src.staticCast<ObjType>());
    }

    bool isNull()const
    {
        return m_ptr == NULL;
    }

    bool notNull()const
    {
        return m_ptr != NULL;
    }

    ObjType* operator->()const
    {
        return m_ptr;
    }

    ObjType* getPointer()const
    {
        return m_ptr;
    }

    template <class DestType>
    Ref<DestType> staticCast()const
    {
        return Ref<DestType> (static_cast<DestType*> (m_ptr));
    }
    
    template <class DestType>
    bool operator == (const Ref<DestType>& x)const
    {
        return this->getPointer() == x.getPointer();
    }
    
    bool operator < (const Ref<ObjType>& b)const
    {
        return this->getPointer() < b.getPointer();
    }

private:
    ObjType* m_ptr;
};


/**
 * Creates a reference for a pointer just returned from 'new' operator
 * @param ptr
 * @return 
 */
template <class T>
inline Ref<T> refFromNew(T* ptr)
{
    Ref<T> r(ptr);

    ptr->release();
    return r;
}

/**
 * Creates a reference from a pointer
 * @param ptr
 * @return 
 */
template <class T>
inline Ref<T> ref(T* ptr)
{
    return Ref<T> (ptr);
}



#endif	/* REFCOUNTOBJ_H */

