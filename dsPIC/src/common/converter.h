// <><><><><><><><><><><><><> converter.h <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//  Copyright(C) 2015 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
//  DC+DC Converter Functionality
//
//-----------------------------------------------------------------------------

#ifndef __CONVERTER_H    // include only once
#define __CONVERTER_H

// -------
// headers
// -------
#include "options.h"    // must be first include
#include "hw.h"
#include "pid.h"
#include "sine_table.h"

// --------------------------
// Conditional Debug Compiles
// --------------------------

// comment out flag to not use it
#define DEBUG_EACH_SECOND  1  // dump out values every second for debugging
//#define DEBUG_PID          1  // dump feed back loop parameters for debugging
//#define DEBUG_SOFT_START   1  // dump soft start for debugging
//#define DEBUG_SOFT_STOP    1  // dump soft stop for debugging
//#define DEBUG_XCYCLE       1  // debug transformer cycle 
//#define DEBUG_UNDER_OVER   1  // debug under/over shoot
#define DEBUG_T4T5_SCOPE   1  // debug duty timer via scope RB13=timer4 RB12=timer5


// -----------------------------
// Auto Removal for build types
// -----------------------------
#if OPTION_NO_CONDITIONAL_DBG
  #undef DEBUG_EACH_SECOND
  #undef DEBUG_PID
  #undef DEBUG_SOFT_START
  #undef DEBUG_SOFT_STOP
  #undef DEBUG_XCYCLE
  #undef DEBUG_UNDER_OVER
  #undef DEBUG_T4T5_SCOPE
#endif	


// ---------------------
// transformer frequency
// ---------------------
#define XFMR_FREQ_HZ   (180)  // hz

// ------------------
// PWM counts per
//  transformer cycle
// ------------------
#define USECS_HALF_CYCLE     ((1000000L/(XFMR_FREQ_HZ*2))) // micro-seconds per half cycle (8333 @60hz, 2777 @180hz)
#define PWMS_HALF_CYCLE      (FPWM/(XFMR_FREQ_HZ*2))       // PWM count in half cycle  64 at 180hz
#define DUTY_CYCLE_DEADBAND  (100)                         // micro-seconds
#define MAX_DUTY_USECS       ((uint16_t)(USECS_HALF_CYCLE - DUTY_CYCLE_DEADBAND))   // max duty cycle time (micro-seconds)

// pwm counts per half cycle
//  60 hz  192   8333 usec
//  90 hz  128
// 120 hz  96
// 180 hz  64    2777 usec
// 240 hz  48

// ----------------
// current clamping
// ----------------
#define MAX_CLAMP_SECS    (5)     // max seconds to hold clamp on
#define MIN_CLAMP_CYCLES  (3)     // minimum xfmr cycles that clamp must be on
#define XCLAMP_TS_DEADBAND  (1000)  // (msecs) dont use temp sensor reading directly after exiting clamp
#define CLAMPED_DUTY_PERCENT      (85)    // duty cycle when clamped (percent)
#define OVERLOAD_DUTY_PERCENT     (85)    // duty cycle when overload detected (percent)

// ------------------
// under / over shoot
// ------------------
#define SOFT_START_BELOW_VOLT      (1.00) // volts below set point for soft start to kick into closed loop
#define OVERSHOOT_VOLTS            (1.25) // volts
#define OVERSHOOT_CORRECTION         (90) // percent of current duty cycle on overshoot
#define UNDERSHOOT_VOLTS           (3.00) // volts  100% duty cycle

// -----------
// PID Values
// -----------
#define C_KP               (100)
#define C_KI               (100)
#define C_KD               (500)
#if (OPTION_DCDC_DCOUT_VOLTS == DCDC_DCOUT_12V)
 #define C_KFF              (50)
#else
 #define C_KFF             (100)
#endif
#define C_ILIM             (1000)
#define C_DLIM             (1000)
#define C_PID_DEADBAND     (5)     // ADC counts
#define C_FF_DEADBAND      (10)    // ADC counts

#define PID_DAMPENER_DIVISOR   (100) // higher value dampens more

// special cases
#if defined(MODEL_12DC51_FW0065) // auto tester
  #undef  C_KP
  #undef  C_KFF
  #define C_KP               (30)
  #define C_KFF              (40)
#endif


// --------------
// Converter Data
// --------------
#pragma pack(1)  // structure packing on byte alignment
typedef struct
{
    uint16_t    dc_set_point_adc;   // output voltage setpoint (adc counts)

} CONVERTER_t;
#pragma pack()  // restore packing setting


// ------------------
// access global data
// ------------------

extern CONVERTER_t  Cnv;
extern PID CnvPID;


#pragma pack(1)  // structure packing on byte alignment
typedef struct
{
	uint16_t dc_set_point_adc;	// dc+dc output voltage setpoint (adc counts)
} CONVERTER_CONFIG_t;
#pragma pack()  // restore packing setting

#if !defined(ALLOCATE_SPACE_ROM_DEFAULTS)
	extern const CONVERTER_CONFIG_t ConvConfigDefaults;
#else
	const CONVERTER_CONFIG_t ConvConfigDefaults = 
	{
	  // dc_setpoint
	  #if IS_DEV_CONVERTER
		DCDC_VOLTS_ADC(OPTION_DCDC_DFLT_SETPOINT)
	  #else
		0
	  #endif
	};
  #if !IS_DEV_CONVERTER
	//	TODO: This is temporary - so that the 12LPC15 code will compile.
	CONVERTER_t  Cnv;
	PID CnvPID;
  #endif
	
#endif 


// -----------
// Prototyping
// -----------
void ClampRectifierBridge(void) ;
void CnvIsrInit(void);
void ExitClamp(void);
void cnv_EmergencyStop(void);
void cnv_ConfigFetPins(void);
int  IsClampShutdown(void);
int  IsClamped(void);
int  IsTempSensorReadable(void);
void NormalRectifierBridge(void);
void OneSecondDump(void);
void T4_Config(void);
void T4_Start(uint16_t usecs);
void T4_Stop(void);
void T5_Config(void);
void T5_Start(uint16_t usecs);
void T5_Stop(void);
void cnv_Init(void);
int  cnv_IsEnabled(void);
void cnv_StartBootCharge(void);
void cnv_Stop(void);
void cnv_Start(void);
void TASK_cnv_Driver(void);
void ChargerControl(void);

#endif  //  __CONVERTER_H

// <><><><><><><><><><><><><> converter.h <><><><><><><><><><><><><><><><><><><><><><>
