// <><><><><><><><><><><><><> structsz.h <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//  Copyright(C) 2013 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
//  Structure Size Checking at compile time
//    Guarantees compiler generates structure size properly.
//
//-----------------------------------------------------------------------------

#ifndef _STRUCT_SZ_H_   // include only once
#define _STRUCT_SZ_H_

// ------------------------------------------
// Structure size checking.
// Compiler will fail with negative subscript
// Allocates no memory.
// ------------------------------------------
#ifndef STRUCT_SIZE_CHECK
 #define STRUCT_SIZE_CHECK(name,nbytes)  extern char __struct_size_check__[sizeof(name) == nbytes ? 1 : -1];
#endif

#endif // _STRUCT_SZ_H_

// <><><><><><><><><><><><><> structsz.h <><><><><><><><><><><><><><><><><><><><><><>
