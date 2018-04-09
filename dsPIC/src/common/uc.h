// <><><><><><><><><><><><><> uc.h <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//  Copyright(C) 2013 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
//  Micro processor specific defines and basic types
//
//-----------------------------------------------------------------------------

#ifndef _UC_H_  // include only once
#define _UC_H_

// ----------------
// standard headers
// ----------------
#include <stdint.h>	 // only need basic types here


// -----------
// Dump Macro
// -----------

// dump macro at compile time for debugging
#define DUMP_MACRO2(x)  #x
#define DUMP_MACRO(x)   DUMP_MACRO2(x)
// Usage
// #pragma message(DUMP_MACRO(PEAK_VAC_SETPOINT))

// ----------------
// useful constants
// ----------------
#define SQRT_OF_TWO	    (1.414213562373)
#define PI              (3.141592653590)

// --------------
// utility macros
// --------------
#define LOBYTE(word)		   (uint8_t)(word   )
#define HIBYTE(word)		   (uint8_t)(word>>8)
#define MKWORD(lobyte,hibyte)  (uint16_t)((((uint16_t)hibyte)<<8)|((uint16_t)lobyte&0xFF))
#define ARRAY_LENGTH(name)     (sizeof(name)/sizeof(name[0]))  // number of elements in an array
#define SECS_TO_MSECS(secs)    ((uint32_t)((uint32_t)(secs)*1000L)) // seconds to milliseconds

// -----------
// Double Word
// -----------
#pragma pack(1)  // structure packing on byte alignement
typedef struct tagDWord
{
    union 
    {
        int32_t dword;
        struct
        {
            int16_t lsw;
            int16_t msw;
        };
    };
} DWORD_t;
#pragma pack()  // restore packing setting


#endif // _UC_H_

// <><><><><><><><><><><><><> uc.h <><><><><><><><><><><><><><><><><><><><><><>
