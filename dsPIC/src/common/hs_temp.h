// <><><><><><><><><><><><><> hs_temp.h <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//	Copyright(C) 2012 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
//  Heat Sink PWM Temperature Sensor
//	
//-----------------------------------------------------------------------------

#ifndef __HS_TEMP_H__    // include only once
#define __HS_TEMP_H__ 

// -------
// headers
// -------
#include "options.h"    // must be first include

// ----------
// Constants
// ----------
#define ROOM_TEMP_C             (25) // Celcius
#define HEATSINK_OT_THRES       (90)
#define HEATSINK_OT_RESET       (80)

// -----------
// Prototyping
// -----------
INLINE int16_t HeatSinkTempC(void)
{
  #ifdef IS_PCB_LPC // no heat sink temp sensor for LPC
    return(ROOM_TEMP_C); // return room temperature
  #else
    extern volatile int16_t  g_hsTempC;
    return(g_hsTempC); 
  #endif
}
void hs_Config(void);
void hs_Start(void);
void hs_Stop(void);

#endif // __HS_TEMP_H__ 

// <><><><><><><><><><><><><> hs_temp.h <><><><><><><><><><><><><><><><><><><><><><>
