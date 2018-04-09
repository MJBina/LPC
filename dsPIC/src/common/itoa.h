// <><><><><><><><><><><><><> itoa.h <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//	Copyright(C) 2014 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
// Binary to ASCII string conversions
//
//-----------------------------------------------------------------------------

#ifndef __ITOA_H    // include only once
#define __ITOA_H

// --------
// headers
// --------
#include "options.h"    // must be first include


// ------------
// Prototyping
// ------------

// conversions to ascii string
char*  ui32toa(char* buf,uint32_t uval);
char*   i32toa(char* buf, int32_t ival);
char*  ui16toa(char* buf,uint16_t uval);
char*   i16toa(char* buf, int16_t ival);
char* ui16toa2(char* buf,uint16_t uval); // no null terminator
char*  i16toa2(char* buf, int16_t ival); // no null terminator
char*    ftoa2(char* buf,float    fval, uint16_t places);
char*  ui32tox(char* buf,uint32_t uval);
char*  ui16tox(char* buf,uint16_t uval);
char*   ui8tox(char* buf,uint8_t  uval);
//int itoa_UnitTest(void);

#endif	//	__ITOA_H

// <><><><><><><><><><><><><> itoa.h <><><><><><><><><><><><><><><><><><><><><><>
