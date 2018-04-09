// <><><><><><><><><><><><><> Q16.h <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//  Copyright(C) 2013 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
//  Q16 fixed point routines
// 
//-----------------------------------------------------------------------------


#ifndef _Q16_H_    // include only once
#define _Q16_H_

//  'Q16fract' is used to calculate the fractional portion of a Q16 value.
//  This is used mainly in inverter.h, but also a few other places.  See 
//  inverter.h for examples.
#define Q16fract(X) ((uint16_t)(65536*(X))) 

// integer portion of Q16 value
#define Q16intgr(N) (((uint32_t)(N))<<16) 

// full value of Q16 number includes integer plus fractional part
#define Q16(N, F)    (uint32_t)(Q16intgr(N) + Q16fract(F))
// usage Q16(1, .066)


#endif // _Q16_H_

// <><><><><><><><><><><><><> Q16.h <><><><><><><><><><><><><><><><><><><><><><>
