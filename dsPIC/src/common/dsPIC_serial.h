// <><><><><><><><><><><><><> dsPIC_serial.h <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//	Copyright(C) 2017 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
// UART communications functionality for dsPIC33 microprocessor
//	
//-----------------------------------------------------------------------------

#ifndef __DSPIC_SERIAL_H    // include only once
#define __DSPIC_SERIAL_H

// -------
// headers
// -------
#include "options.h"    // must be first include

//-----------------------------------------------------------------------------
//	The function serial_printf() calls variants of the printf() function which 
//	have substantial resource requirements. It was determined that including 
//	serial_printf() requires an additional 3321 bytes Flash and 188 bytes RAM.
//	Alternatives should be used for production code.
//  #define _OMIT_PRINTF
//-----------------------------------------------------------------------------
#undef _OMIT_PRINTF


// -----------
// Prototyping
// -----------

extern void     _serial_Config(void);
extern int16_t  _serial_getc(void);
extern int16_t  _serial_getchar(void);
extern int8_t   _serial_IsTxBuffEmpty(void);
extern int16_t	_serial_putbuf( unsigned char *data, int len );
extern int16_t  _serial_putc(char ch);
extern int16_t  _serial_puts(const char * str);
extern void     _serial_RunMode(void);
extern int16_t  _serial_RxBE(void);
extern int16_t  _serial_RxFreeSpace(void);
extern void     _serial_Start(void);
extern void     _serial_Stop(void);
extern int16_t  _serial_TxBE(void);
extern int16_t  _serial_TxFreeSpace(void);
#ifndef _OMIT_PRINTF
 extern int16_t _serial_printf(char * fmt, ...);
#endif

// -----------------
// Function Mapping
// -----------------
#define serial_Config()         _serial_Config()
#define serial_getc()           _serial_getc()
#define serial_IsTxBuffEmpty()	_serial_IsTxBuffEmpty()
#define serial_putbuf(data,len) _serial_putbuf(data,len)
#define serial_putc(ch)         _serial_putc(ch)
#define serial_puts(str)        _serial_puts(str)
#define serial_Start()          _serial_Start()
#define serial_Stop()           _serial_Stop()
#define serial_RunMode()        _serial_RunMode()
#define serial_TxBE()           _serial_TxBE()

#ifdef WIN32
   #define serial_printf
#else
 #ifdef _OMIT_PRINTF
	#define serial_printf(format, ...)		//	(format, ##__VA_ARGS__)
 #else
	#define serial_printf(format, ...) 	_serial_printf(format, ##__VA_ARGS__)
 #endif
#endif // WIN32
 
#endif //	__DSPIC_SERIAL_H

// <><><><><><><><><><><><><> dsPIC_serial.h <><><><><><><><><><><><><><><><><><><><><><>
