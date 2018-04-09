// <><><><><><><><><><><><><> fan_ctrl.h <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//  Copyright(C) 2015 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
//  Cooling Fan Control
//
//-----------------------------------------------------------------------------

#ifndef __FAN_CTRL_H_    // include only once
#define __FAN_CTRL_H_

// -------
// headers
// -------
#include "options.h"    // must be first include
#include "analog.h"

//  The fan threshold value is based upon the value used for the 105% 
//  overload threshold. That value is scaled appropriately.
//	6% of rated 12.5A AC load, fan turn on with load >=100W  

#if IS_DCIN_12V
    // --------
    // 12 volts
    // --------

    // inverter fan on threshold
    #define ILIMIT_INV_AC_AMPS              ((float)12.5)   // ### TODO put in calibration?
	#define FAN_ON_THRES_INV_AMPS_PERCENT   ((float)6)      // percent   
	#define FAN_ON_THRES_INV_AMPS           IMEAS_INV_AMPS_ADC((ILIMIT_INV_AC_AMPS*FAN_ON_THRES_INV_AMPS_PERCENT)/100)    // 0.75 AC Amps

    // charger fan on threshold
	#define FAN_ON_THRES_CHGR_AMPS_PERCENT   ((float)35)    // percent
	#define FAN_ON_THRES_CHGR_AMPS           IMEAS_CHG_AMPS_ADC((ILIMIT_CHG_AC_AMPS*FAN_ON_THRES_CHGR_AMPS_PERCENT)/100)  // 2.5 AC Amps

	//	If (battery voltage < 10.75VDC (regardless of mode)), then fan is OFF
	#define FAN_OFF_THRES_VOLTS		         ((int16_t)(VBATT_VOLTS_ADC(10.75))) // ### TODO put in calibration?

#elif IS_DCIN_51V

    // --------
    // 51 volts
    // --------

	#define FAN_ON_THRES_INV_AMPS_PERCENT   ((float)(6))   
	#define FAN_ON_THRES_INV_AMPS   ((int16_t)((IMEAS_INV_SLOPE * ((FAN_ON_THRES_INV_AMPS_PERCENT/100) * 12.50)) + IMEAS_INV_INTERCEPT)) 

	//	If (battery voltage < 10.75VDC (regardless of mode)), then fan is OFF
	#define FAN_OFF_THRES_VOLTS		((int16_t)(VBATT_VOLTS_ADC(47.00)))

#else
    // -----------------------
    // Not defined is an error
    // -----------------------
    #error OPTION_nnV Input Voltage not set in 'options.h'
	
#endif



// -----------
// Prototyping
// -----------
// routines required by all fan drivers
uint16_t fan_GetDutyCycle(void); // return duty cycle (0=off 100=full- on)
void fan_SetOverRide(uint16_t duty_cycle);  // over-ride for testing
void fan_Config(void);
void fan_Start(void);
void fan_Stop(void);
void fan_Timer(void);        // called at FPWM frequency by pwm interrupt
void TASK_fan_Driver(void);  // call once per milli-ssecond


#endif // __FAN_CTRL_H_

// <><><><><><><><><><><><><> fan_ctrl.h <><><><><><><><><><><><><><><><><><><><><><>
