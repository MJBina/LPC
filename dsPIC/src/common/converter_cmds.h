// <><><><><><><><><><><><><> converter_cmds.h <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//	Copyright(C) 2015 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
//  DC+DC Converter remote interface commands
//
//-----------------------------------------------------------------------------

#ifndef __CONVERTER_CMDS_H_    // include only once
#define __CONVERTER_CMDS_H_

// -------
// headers
// -------
#include "options.h"    // must be first include

// -----------
// Prototyping
// -----------
void  cnv_SetDcSetPoint(float volts);
float cnv_GetDcSetPoint(void); // setpoint (volts)
float cnv_GetDcOutVolts(uint8_t saved); // output voltage (volts)  saved  0=actual, 1=at fault time


#endif // __CONVERTER_CMDS_H_

// <><><><><><><><><><><><><> converter_cmds.h <><><><><><><><><><><><><><><><><><><><><><>
