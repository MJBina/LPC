// <><><><><><><><><><><><><> sine_table.h <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//  Copyright(C) 2013 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
//  Sine Wave table used for generating an output sine wave for the inverter
//
//-----------------------------------------------------------------------------

#ifndef __SINE_TABLE_H    // include only once
#define __SINE_TABLE_H

// -------
// headers
// -------
#include "options.h"    // must be first include


// ---------------
// waveform cycle
// ---------------
typedef enum
{
    VECTOR1 = 0, //   0 to 179 degrees
    VECTOR2 = 1, // 180 to 359 degrees
} VECTOR_CYCLE;


//-----------------------------------------------------------------------------
//  The sinusoidal output is created using a lookup table. The lookup table
//  entries which represent a half sine wave from 0 to 180-degrees.  
//  MAX_SINE represents 180 degrees and also the number of entries in the 
//  table.
//
//  PTPER: PWM Time Base Period register
//  Computed Period based on CPU speed and desired PWM frequency ( > 20KHz)
//  PTPER = ((FCY/FPWM) - 1);
//-----------------------------------------------------------------------------

// -------------
// 8 Mhz Crystal
// -------------
#define MAX_SINE        192 
#define FPWM            23040
#define PTPER_INIT_VAL  1735

// ------------------
// access global data
// ------------------
extern const uint16_t _SineTableQ16[MAX_SINE];


#endif  //  __SINE_TABLE_H

// <><><><><><><><><><><><><> sine_table.h <><><><><><><><><><><><><><><><><><><><><><>

