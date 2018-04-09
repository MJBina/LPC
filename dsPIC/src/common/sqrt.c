// <><><><><><><><><><><><><> sqrt.c <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//	Copyright(C) 2013 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
//  Integer square-root routines
//
//-----------------------------------------------------------------------------

// -------
// headers
// -------
#include "options.h"    // must be first include
#include "sqrt.h"

//----------------------------------------------------------------------------
//	isqrt16
//----------------------------------------------------------------------------
//	'isqrt16' is a support function for calculating the RMS value.
//
//	this algorithm borrowed from Wikipedia:
//		http://en.wikipedia.org/wiki/Methods_of_computing_square_roots#Binary_numeral_system_.28base_2.29
//      uses Newton Raphson iteration
//
//-----------------------------------------------------------------------------

uint16_t isqrt16(uint16_t num)
{
    uint32_t remainder, root, place;

    // initialize "bit" at the highest power of four <= argument.
    root      = 0;
    remainder = num;
    place     = 0x40000000; // The second-to-top bit is set
    while (place > remainder)
    {
        place = place >> 2; // place/4
    }

    while (place)
    {
        if (remainder >= root + place)
        {
            remainder = remainder - root - place;
            root = root + (place << 1); // place*2
        }
        root  = root  >> 1; // root/2
        place = place >> 2; // place/4
    }
    return(root);
}

//----------------------------------------------------------------------------
//	_isqrt32
//----------------------------------------------------------------------------
//	'_isqrt32' is a support function for calculating the RMS value.
//   https://www.quora.com/Which-is-the-fastest-algorithm-to-compute-integer-square-root-of-a-number
//
//----------------------------------------------------------------------------

uint16_t isqrt32(uint32_t num)
{
    uint32_t remainder, root, place;

    // initialize "bit" at the highest power of four <= argument.
    root      = 0;
    remainder = num;
    place     = 0x40000000; // The second-to-top bit is set
    while (place > remainder)
    {
        place = place >> 2; // place/4
    }

    while (place)
    {
        if (remainder >= root + place)
        {
            remainder = remainder - root - place;
            root = root + (place << 1); // place*2
        }
        root  = root  >> 1; // root/2
        place = place >> 2; // place/4
    }
    if (root > 64000) // ### should never happen
    {
        LOG(SS_SYS, SV_ERR, "isqrt32(%lu)=%lu; p=%lu r=%lu", num, root, place, remainder);
    }
    return((uint16_t)root);
}

// -----------------------------------------------------------------------------
//                     S Q R T    U N I T    T E S T 
// -----------------------------------------------------------------------------

/*
void sqrt_RunUnitTest(void)
{
  #ifdef WIN32
    #define TESTSQ16(inval, result)  if (isqrt16(inval) != result) { nerrs++; printf("isqrt16(%u)!=%u failed",   inval, result); }
    #define TESTSQ32(inval, result)  if (isqrt32(inval) != result) { nerrs++; printf("isqrt32(%lu)!=%lu failed", inval, result); }
  #else
    #define TESTSQ16(inval, result)  if (isqrt16(inval) != result) { nerrs++; LOG(SS_SYS, SV_ERR, "isqrt16(%u)!=%u failed",   inval, result); }
    #define TESTSQ32(inval, result)  if (isqrt32(inval) != result) { nerrs++; LOG(SS_SYS, SV_ERR, "isqrt32(%lu)!=%lu failed", inval, result); }
  #endif

    int16_t  nerrs=0;

    LOG(SS_SYS, SV_INFO, "sqrt unit test");

    // test isqrt16()
    nerrs = 0; // reset for this test
    TESTSQ16(65535, 255);
    TESTSQ16(62500, 250);
    TESTSQ16(16384, 128);
    TESTSQ16(    0,   0);
    if (nerrs == 0) LOG(SS_SYS, SV_INFO, "isqrt16() unit test PASSED");
    else            LOG(SS_SYS, SV_INFO, "isqrt16() unit test FAILED");

    // test isqrt32()
    nerrs = 0; // reset for this test
    TESTSQ32(268435455L, 16383L);
    TESTSQ32(62500L,       250L);
    TESTSQ32(59000L,       242L);
    TESTSQ32(32767L,       181L);
    TESTSQ32(32766L,       181L);
    TESTSQ32(32765L,       181L);
    TESTSQ32(16384L,       128L);
    TESTSQ32(    0L,         0L);
    if (nerrs == 0) LOG(SS_SYS, SV_INFO, "isqrt32() unit test PASSED");
    else            LOG(SS_SYS, SV_INFO, "isqrt32() unit test FAILED");
}
*/

// <><><><><><><><><><><><><> sqrt.c <><><><><><><><><><><><><><><><><><><><><><>
