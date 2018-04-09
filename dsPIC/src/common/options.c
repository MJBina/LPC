// <><><><><><><><><><><><><> options.c <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//  Copyright(C) 2017 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
//  $Workfile: options.c $
//  $Author: clark_dailey $
//  $Date: 2018-04-02 10:26:53-05:00 $
//  $Revision: 38 $ 
//
//	Cause the options specified by the file 'options.h' to report when compiling
//	
//-----------------------------------------------------------------------------

// -------
// headers
// -------

#define __OPTIONS_C__

// show calibration when compiling
#define  SHOW_CALIBRATION  1

#include "options.h"    // must be first include


#ifndef WIN32

// ---------------------------------
// show the options that are enabled
// ---------------------------------


#if defined (OPTION_UL_TEST_CONFIG)
  #pragma message "OPTION_UL_TEST_CONFIG is defined"
#endif


#if defined (OPTION_DISABLE_CHARGE_TIMER_SHUTDOWN)
  #pragma  message  "Charge Timer Shutdown Disabled"
#endif

#if defined(OPTION_IGNORE_OVERLOAD)
  #pragma message "Ignoring Overload"
#endif

#if defined(OPTION_CHARGER_CV_MODE_ONLY)
  #pragma  message  "Charger locked into CV-Mode!"
#endif

#define  BUILD_STRING  "Building "  MODEL_NO_STRING  " "  FW_ID_STRING " " BUILD_TYPE_STR
#pragma  message  BUILD_STRING

// DC Supply Voltage
#pragma message "OPTION_DCIN = " NUM2STR(OPTION_DCIN)

// Show Firmware ID
#pragma message "FW_ID = " FW_ID_STRING

// Device Type
#if IS_DEV_INVERTER
	  #pragma message "Inverter"
	#if defined(OPTION_SSR)
	  #pragma message "Output Clamping Enabled via SSR"
	#else
	  #pragma message "Output Clamping Disabled"
	#endif
#elif IS_DEV_CONVERTER
    #pragma message  "DC+DC Converter: Vout volts = " NUM2STR(OPTION_DCDC_DFLT_SETPOINT)
	#if defined(OPTION_HAS_CLAMP)
	  #pragma message "Output Clamping Enabled"
	#else
	  #pragma message "Output Clamping Disabled"
	#endif
#endif

// Charger Status
#if defined(OPTION_HAS_CHARGER)
  #if defined(OPTION_CHARGE_3STEP)
	#pragma message "3 Step Charger"
  #endif
  #if defined(OPTION_CHARGE_LION)
	#pragma message "Lion Charger"
  #endif
  #if defined(OPTION_HAS_CHGR_EN_SWITCH)
	#pragma message "Charger Enable/Disable Switch replaces Temp Sense"
  #endif
  #pragma message "ILIMIT Charger Max A/C Amps = "  NUM2STR(ILIMIT_CHG_AC_AMPS)

#endif

// ----------------------
// Optional Functionality
// ----------------------

// A/C line qualification over-ride
#if defined(AC_LINE_QUAL_SECS_OVER_RIDDEN)
  #pragma message ("Over-Ride A/C Line Qual Secs=" DUMP_MACRO(OPTION_AC_LINE_QUAL_SECS))
#endif

// Serial Port Debugging
#if (OPTION_SERIAL_DEBUG == OPTION_SERIAL_DEBUG_UART1)
  //  UART1 is enabed by default
  #pragma message "Using UART1 for Serial Debug"
#elif (OPTION_SERIAL_DEBUG == OPTION_SERIAL_DEBUG_UART2)
  #pragma message "Using UART2 for Serial Debug"
#elif (OPTION_SERIAL_DEBUG == OPTION_SERIAL_DEBUG_NONE)
  #pragma message "Serial Debug disabled"
#else
  #error 'OPTION_SERIAL_DEBUG' is not set in 'models.h'
#endif

// NVM settings
#if (OPTION_NVM == OPTION_NVM_NONE)
	#pragma message "NVM is not available"
#elif (OPTION_NVM == OPTION_NVM_I2C1)
	#pragma message "Using I2C1 for NVM"
#elif (OPTION_NVM == OPTION_NVM_I2C2)
	#pragma message "Using I2C2 for NVM"
#else
	#error 'OPTION_NVM' not set in 'models.h'
#endif

// Logging		
#if defined(OPTION_NO_LOGGING)
	#pragma message "OPTION_NO_LOGGING - logging disabled"
#endif

// Serial debug port baud rate
#if defined(OPTION_BAUD_9600)
	#pragma message "OPTION_BAUD_9600 - Serial Debug 9600 Baud"
#endif

// Solid State Relay clamping functionality
#if defined(OPTION_SSR)
	#pragma message "Using Solid-State-Relay"
#endif

// CAN Bus Baud Rate
#if defined(OPTION_CAN_BAUD_500K)
	#pragma message "CAN Baud Rate 500K - non RVC"
#endif

// Cooling Fan Always On functionality
#if defined(OPTION_FAN_ALWAYS_ON)
	#pragma message "OPTION_FAN_ALWAYS_ON"
#endif

// Safe Interrupt Logging
#ifdef OPTION_SAFE_LOGGING
	#pragma message "Safe Logging Enabled"
#endif

#endif  // WIN32

// <><><><><><><><><><><><><> options.c <><><><><><><><><><><><><><><><><><><><><><>
