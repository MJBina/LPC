// <><><><><><><><><><><><><> batt_temp.h <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//  Copyright(C) 2016 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
//  Magnum Battery Temperature Sensor
//
//  This data taken from Pao's measurements provided in the spreadsheet 
//  '12LPC15 Using Magnum Batt Temp Sense_Chang Zhou (R-T Table).xlsx' tab 
//  'Curve'.  This file was converted to a .CSV, and then edited.
//
//  ADC values were calculated from the volts measured assuming an ideal 
//  conversion. Specifically that 0V = 0 ADC counts and 3.3V = 1024 counts, and 
//  intermediate values are linear.
//  
//-----------------------------------------------------------------------------

#ifndef __BATT_TEMP_SENSOR_H    // include only once
#define __BATT_TEMP_SENSOR_H

// -------
// headers
// -------
#include "options.h"    // must be first include
#include "analog.h"
#include "charger.h"

//-----------------------------------------------------------------------------
//  We want to indicate sensor status using a value compatible with the
//  type returned by the function to read the temperature value.
//
//  Therefore, we use the following values to represent sensor status:
//-----------------------------------------------------------------------------

#define BATT_TEMP_NOMINAL		CHGR_BATT_TEMP_NOMINAL
#define BATT_TEMP_SNSR_OPEN_THRES       (990)
#define BATT_TEMP_SNSR_SHORT_THRES      (40)

// ------
// enums
// ------
typedef enum 
{
	BATT_TEMP_SENSOR_OK 	= 0,
	BATT_TEMP_SENSOR_SHORT 	= 0xFFFD,
	BATT_TEMP_SENSOR_OPEN 	= 0xFFFE,
	BATT_TEMP_SENSOR_ERROR	= 0xFFFF,
} BATT_TEMP_SENSOR_STATUS_t;

// -----------
// Prototyping
// -----------
extern void BattTemp_Evaluate(void);
extern BATT_TEMP_SENSOR_STATUS_t BattTemp_CheckSensor(void); 
extern int16_t     BattTemp_celsius(void);
extern ADC10_RDG_t BattTemp_VBattCompA2d(void);
extern ADC10_RDG_t BattTemp_a2d(int16_t temp_c);

#endif  //  __BATT_TEMP_SENSOR_H

// <><><><><><><><><><><><><> batt_temp.h <><><><><><><><><><><><><><><><><><><><><><>
