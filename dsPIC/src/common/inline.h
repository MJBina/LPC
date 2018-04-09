// <><><><><><><><><><><><><> inline.h <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//  Copyright(C) 2013 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
// Allow inlining of subroutines for faster execution speed
//
// CAUTION!
//   always use the INLINE macro to achieve proper results
//-----------------------------------------------------------------------------

#ifndef _INLINE_H_    // include only once
#define _INLINE_H_

// --------------------------
// inline functions for speed
// --------------------------
#ifdef WIN32
  #define INLINE _inline
#else
  #define INLINE static inline __attribute__((always_inline))
  // IMPORTANT: You must have the option checked in "xc16-gcc\Optimizations"
  //              Do not override inline [X]    (it is off by default)
  // "always_inline" makes it inline even if optimization is turned off
#endif


#endif // _INLINE_H_

// <><><><><><><><><><><><><> inline.h <><><><><><><><><><><><><><><><><><><><><><>
