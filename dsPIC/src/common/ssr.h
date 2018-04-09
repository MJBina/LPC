// <><><><><><><><><><><><><> ssr.h <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//  Copyright(C) 2016 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
//  Solid-State-Relay - Volta specific
//
//  This function is intended to address a High Voltage spike on the DC 
//  power bus. (This may result from the Volta Battery pack suddenly 
//  disconnecting itself from the DC Bus while being charged.)
//
//-----------------------------------------------------------------------------

#ifndef __SSR_H   // include only once
#define __SSR_H

// --------
// headers
// --------
#include "options.h"    // must be first include
#include "analog.h"

// conditinal compiler entire file
#if defined(OPTION_SSR)

// ---------
// Constants
// ---------
#define SSR_CLAMP_TIMEOUT_MSEC  ((int16_t)(200))
#define SSR_VDC_CLAMP_THRES     (VBATT_VOLTS_ADC(65.0))
#define SSR_VDC_CLAMP_RECOV     (VBATT_VOLTS_ADC(62.0))

// -------------------
// Access Global Data
// -------------------
//  ssr_TriggerOccurred is set within the ADC(DMA) ISR when the DC voltage 
//  exceeds the SSR_VDC_CLAMP_THRES. It is used witin the MainControl state-
//  machine/task to process the SSR.
extern int8_t ssr_TriggerOccurred;

//  ssr_Shutdown is set within the _DC_Clamp() function in the MainControl 
//  state-machine/task.  It is used solely by the UI to display the correct 
//  related message.
extern int8_t ssr_Shutdown;
extern int8_t ssr_Failure;

// -------------
// Prototyping
// -------------
extern void ssr_Detect(void);
extern void ssr_Reset(void);

#endif  //  OPTION_SSR
#endif  //  __SSR_H

// <><><><><><><><><><><><><> ssr.h <><><><><><><><><><><><><><><><><><><><><><>
