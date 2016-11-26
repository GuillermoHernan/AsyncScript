/* 
 * File:   OS_support.h
 * Author: ghernan
 * 
 * Specific SO support code.
 *
 * Created on November 21, 2016, 7:24 PM
 */

#ifndef OS_SUPPORT_H
#define	OS_SUPPORT_H
#pragma once

#include <stdarg.h>
#include <stdio.h>

#ifdef _WIN32
#ifdef _DEBUG
   #ifndef DBG_NEW
      #define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
      #define new DBG_NEW
   #endif
#endif
#endif

#ifdef __GNUC__

//#define vsprintf_s vsnprintf
//#define sprintf_s snprintf
#define _strdup strdup

template <size_t size>
int vsprintf_s(char (&buffer)[size], const char* format, va_list args)
{
    const int result = vsnprintf(buffer, size, format, args);
    
    return result;
}

template <size_t size>
int sprintf_s(char (&buffer)[size], const char* format, ...)
{
    va_list aptr;

    va_start(aptr, format);
    const int result = vsprintf_s (buffer, format, aptr);
    va_end(aptr);
    
    return result;
}
#endif

#include <assert.h>

#define ASSERT(X) assert(X)



#endif	/* OS_SUPPORT_H */

