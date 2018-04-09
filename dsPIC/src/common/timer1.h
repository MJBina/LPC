// <><><><><><><><><><><><><> timer1.h <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//  Copyright(C) 2013 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
//  Interrupt driven one millisecond timing provided by hardware Timer1
//
//-----------------------------------------------------------------------------

#ifndef _TIMER1_H_   // include only once
#define _TIMER1_H_

// -------
// headers
// -------
#include "options.h"    // must be first include

// -----------
// basic types
// -----------
typedef uint32_t SYSTICKS;

// -----------------
// access global data
// -----------------
extern volatile int16_t   _T1TickCount;  // gets incremented and decremented
extern volatile SYSTICKS  _SysTicks;     // continuously incrementing tick counter (wraps in 49.7 days)

// --------------------
// Function Prototyping
// --------------------
void      _T1Config(void);
void      _T1Start(void);
void      _T1Stop(void);
uint32_t  ElapsedMsec(SYSTICKS startTicks, SYSTICKS endTicks);


// --------------------------
// Inline Functions for Speed
// --------------------------

INLINE SYSTICKS GetSysTicks() { return(_SysTicks); }  // get running 1 msec system timer

// check if timer has timed-out
// returns: 0=no, 1=time-out has occurred
INLINE int IsTimedOut(uint32_t msecs,SYSTICKS startTicks)
{
    // if (startTicks == 0) return(0);  // allow disabling
    return( ElapsedMsec(startTicks, GetSysTicks()) > msecs) ;
}

#endif // _TIMER1_H_

// <><><><><><><><><><><><><> timer1.h <><><><><><><><><><><><><><><><><><><><><><>
