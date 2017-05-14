/* 
 * File:   asString.h
 * Author: ghernan
 * 
 * AsyncScript string class implementation
 * 
 * Created on January 3, 2017, 2:19 PM
 */

#ifndef ASSTRING_H
#define	ASSTRING_H

#pragma once

#include "asObjects.h"


/**
 * Javascript string class.
 * Javascript strings are immutable. Once created, they cannot be modified.
 */
class JSString : public JSObject
{
public:
    static Ref<JSString> create(const std::string& value);

    virtual ASValue unFreeze(bool forceClone=false);

    virtual bool toBoolean()const
    {
        return !m_text.empty();
    }
    virtual double toDouble()const;

    virtual std::string toString()const
    {
        return m_text;
    }

    virtual ASValue readField(const std::string& key)const;
    virtual ASValue getAt(ASValue index);

    virtual std::string getJSON(int indent);

    virtual double compare (const ASValue& b, ExecutionContext* ec)const;

    static Ref<JSClass> StringClass;

protected:

    JSString(const std::string& text) 
    : JSObject(StringClass, MT_DEEPFROZEN), m_text(text)
    {
    }

private:
    const std::string m_text;

};


#endif	/* ASSTRING_H */

