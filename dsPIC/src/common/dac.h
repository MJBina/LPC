// <><><><><><><><><><><><><> dac.h <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//	Copyright(C) 2015 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
// 
// Digital/Analog Converter
// 
// TI DAC101c081C1MK/NOPQ SOT-6  6 pin
// 10 Bit DAC on I2C Bus #1
// Address 0x1A
// 
//  NOT USED - data too noisy
//
//-----------------------------------------------------------------------------

#ifndef __DAC_H__    // include only once
#define __DAC_H__

// -------
// headers
// -------
#include "options.h"    // must be first include

/* not used - data too noisy

// ---------
// constants
// ---------
#define DAC_MAX_VALUE    1023   // 0x3FF
#define DAC_MIN_VALUE       0

// ------
// status
// ------
#define DAC_STATUS_OK     0 // completed ok; ready for another operation
#define DAC_STATUS_BUSY   1 // busy
#define DAC_STATUS_ERR    2 // completed with errors

typedef enum 
{
    DAC_MODE_NORMAL        = 0, // see datasheet
    DAC_MODE_PULLDOWN_2_5K = 1,
    DAC_MODE_PULLDOWN_100K = 2,
    DAC_MODE_HI_IMPENDANCE = 3,
} DAC_MODE_t;

// ---------------------
// DAC Driver state data
// ---------------------
#pragma pack(1)  // structure packing on byte alignement
typedef struct
{
    uint16_t  dac;        // dac value to write
    uint8_t   state;      // state of isr state-machine
    uint8_t   nretries;   // retry counter
    uint8_t   status;     // completion status; DAC_STATUS_nnn
} DAC_DRV_t;
#pragma pack()  // restore packing setting


// -----------
// Prototyping
// -----------
void dac_Config(void);  // configure and initialize dac driver
void dac_Start(void);   // start dac interrupts
void dac_Stop(void);    // stop  dac interrupts
int  dac_Write(DAC_MODE_t mode, uint16_t dac);
void dac_RunUnitTest(void); 


//-------------------
// Access Global Data
//-------------------
extern DAC_DRV_t  g_dac;    // state info


//-------------------
// Inlines for Speed
//-------------------

INLINE int dac_Status()  { return(g_dac.status); }  // returns: DAC_STATUS_nnn
INLINE int dac_IsBusy()  { return(DAC_STATUS_BUSY == dac_Status()); }
INLINE int dac_IsError() { return(DAC_STATUS_ERR  == dac_Status()); }

*/ // obsolete

#endif   // __DAC_H__

// <><><><><><><><><><><><><> dac.h <><><><><><><><><><><><><><><><><><><><><><>
