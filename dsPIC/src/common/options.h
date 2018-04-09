// <><><><><><><><><><><><><> options.h <><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//  Copyright(C) 2017 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
//  $Workfile: options.h $
//  $Author: clark_dailey $
//  $Date: 2018-04-02 10:28:38-05:00 $
//  $Revision: 185 $ 
//
//  This header is included as the first header in every C fille.
//  Acts similar to a pre-compiled header file.
//
//  Options are set here, so every file must include this file.
//
//-----------------------------------------------------------------------------

#ifndef _OPTIONS_H    // include only once
#define _OPTIONS_H

#ifdef WIN32
 #pragma warning(disable : 4260)  // translation unit empty
 #pragma warning(disable : 4996)  //_CRT_SECURE_NO_WARNINGS;  vsprintf may be unsafe
#endif
 

// -----------------------------------------------------------
// This file is used as a pre-compiled header file
// and is required to be included in every C file
// in order to honor the option settings.
// It pulls in basic headers that are used by the system.
// -----------------------------------------------------------

#include "optimize.h"  // MUST be first
#include "inline.h"    // support inlining routines for speed
#include <xc.h>        // microcprocessor
// utility headers
#include "uc.h"        // constants and basic types
#include "structsz.h"  // structure size checking
#include "timer1.h"    // timing routines GetSysTicks()
#include "log.h"       // logging functions
#include "itoa.h"      // conversions to human readable format
// standard headers
#include <stdint.h>    // basic types
#include <stdio.h>     // sprintf etc
#include <string.h>    // string routines
#ifdef WIN32
 extern int abs(int value);
#else
 #include <stdlib.h>    // basic library routines
#endif


// ------------------------------------------------------------------------------------------------------------
//   Project setting must define the model as MODEL_nnn
//   The MODEL_nnn sets the option flags needed for the particular device
//
//        MODEL_NO_STRING - text string of the model number (default value - overwritten by mfg tool)
//          Mandatory
// ------------------------------------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------------------------------
//        FW_ID_STRING - text string of the firmware file
//          Mandatory
// ------------------------------------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------------------------------
//        BUILD_DATE - text string of the build date ie "2017-11-09-A"
//          Mandatory
// ------------------------------------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------------------------------
//      Build Type  'BUILD_TYPE'
//        Mandatory: must be set to one of these
// ------------------------------------------------------------------------------------------------------------
          #define   BUILD_RELEASE   1   // release version for production
          #define   BUILD_DVT       2   // design verification test
          #define   BUILD_DEV       3   // working development version

// ------------------------------------------------------------------------------------------------------------
//      Device Type  'OPTION_DEV'
//        Mandatory: must be set to one of these
// ------------------------------------------------------------------------------------------------------------
//        CAUTION! Dont change these number since they are exported
//        Inverters and Converters can have optional charger functionality
//        Adding more items requires changes to LCD firmware and CAN Bus Diag program
// ------------------------------------------------------------------------------------------------------------
          #define   DEV_INVERTER    1   // inverter device
          #define   DEV_CONVERTER   2   // DC+DC converter

		  
////////////////////////////////////////////////////////////////////////////////
//	NOTE: This is left as a TODO until we branch NP for release.
//	TODO: Refactor this to combine OPTION_PCB and OPTION_PCB_REV to reduce 
//		redundancy. The new item should be named PCA (Printed Circuit 
//		Assembly). PCB (Printed Circuit Board) refers to bare fiberglass. 
//		PCA encompasses both the function of the circuit board and its 
//		revision.  Ideally, these defines may include the PCA number for
//		easy reference of the schematic.  
//		The suggested list of items is then:
// 
//      #define   PCA_170165_REVA	100   // LPC PCA 170165 Rev-A
//      #define   PCA_171586_REVB   200   // NP  Engineering version
//      #define   PCA_171600_REV1   201   // NP  Production  version
//      #define   PCB_REV_BDC_1     301   // Bidirectional DC converter 
////////////////////////////////////////////////////////////////////////////////
	
// ------------------------------------------------------------------------------------------------------------
//      Board Type   'OPTION_PCB'
//        Mandatory: must be set to one of these
// ------------------------------------------------------------------------------------------------------------
//       CAUTION! Dont change these number since they are exported
          #define   PCB_NP          1   // NP 
          #define   PCB_LPC         2   // LPC
          #define   PCB_BDC         3   // Bidirection DC Converter

// ------------------------------------------------------------------------------------------------------------
//      Board Revision  OPTION_PCB_REV
//        Mandatory: must be set to one of these
// ------------------------------------------------------------------------------------------------------------
          #define   PCB_REV_1         1   // Generic
          // LPC
          #define   PCB_REV_LPC_1   100   // LPC PCA 170165 Rev-A
          // NP
          #define   PCB_REV_NP_1    200   // NP  Engineering version
          #define   PCB_REV_NP_2    201   // NP  Production  version
          // BDC
          #define   PCB_REV_BDC_1   301   // Bidirectional DC converter    

// ------------------------------------------------------------------------------------------------------------
//      Input Supply Voltage 'OPTION_DCIN'
//        Mandatory: must be set to one of these
// ------------------------------------------------------------------------------------------------------------
//      CAUTION! Dont change these number since they are exported
          #define   DCIN_12V        12
          #define   DCIN_24V        24
          #define   DCIN_48V        48
          #define   DCIN_51V        51

// ------------------------------------------------------------------------------------------------------------
//      Calibration File used 'OPTION_CAL'
//        Mandatory: must be set to one of these
 //       These must be all unique numbers
// ------------------------------------------------------------------------------------------------------------
          // LPC Prefix=1
          #define   CAL_12LPC     11200
          #define   CAL_51LPC     15100
          // NP Prefix=2
          #define   CAL_12NP18    21218
          #define   CAL_12NP20    21220
          #define   CAL_12NP24    21224
          #define   CAL_12NP30    21230
          #define   CAL_24NP24    22424
          #define   CAL_24NP36    22436
          #define   CAL_48NPXX    24800
          #define   CAL_51NPXX    25100
          // DC+DC Prefix=3
          #define   CAL_12DC51    31251
          #define   CAL_51DC12    35112
          #define   CAL_51DC24    35124

// ------------------------------------------------------------------------------------------------------------
//  Optional Settings (any device type)
//      OPTION_AC_LINE_QUAL_SECS   - A/C line qualification time in seconds (30 secs if not specified)
//      OPTION_SSR                 - use Solid State Relay clamp
//      OPTION_HAS_CLAMP           - has output clamping functionality
//      OPTION_CAN_BAUD_500K       - use 500K baud for CAN else 250KB
//      OPTION_SAFE_LOGGING        - enable safe logging macro SLOG() for logging from interrupts
//      OPTION_NO_LOGGING          - (DON'T USE THIS) disable all serial port logging
//      OPTION_IGNORE_OVERLOAD     - Overload Protection disabled - U.L. testing, etc
//		OPTION_DISABLE_CHARGE_TIMER_SHUTDOWN - Charge Timer shutdown disabled.
//									for U.L. Testing. Specifically CV-Timeout,
//									other charge timers may function normally. 
//		OPTION_CHARGER_CV_MODE_ONLY - Lock the charger in CV-Mode.
//      OPTION_FAN_ALWAYS_ON       - fan stays on all the time to prevent xfrm over temp
//      OPTION_HAS_CHARGER         - device has charging capability (inverters and converters)
//        must select one of these:
//      	OPTION_CHARGE_3STEP    - use 3 step charging algorithm
//          OPTION_CHARGE_LION     - charging Lithium Ion battery
//      OPTION_VOLTA_UI            - provide lcd user interface in Volta format
//      OPTION_NO_CONDITIONAL_DBG  - turn off all local file conditional debugging flags
//                                       set via BUILD_DVT and BUILD_RELEASE
//
//	SPECIAL NOTE ON U.L TESTING:
//	This note is being placed here for lack of a better place... 
//	U.L. testing is destructive. Firmware protection is turned off.
//	Specific requirements for U.L. testing are highlighted below:
//	  Inverter  Disable Overload protection
//	  Charger  CV-Mode ONLY!  Start & Stay in CV-Mode. Output = 12.6V
//	  Charger  Disable Charge-Mode Timeouts
//	  Charger - Disable Peak-limiting and other protective measures.
//	For simplicity, these are set on the command-line within MPLAB.
//
//
//   Charger Options (only used if OPTION_HAS_CHARGER defined)
//
//      OPTION_HAS_CHGR_EN_SWITCH  - hijack Temp Sensor RJ45 jack for Charger 
//			Enable switch - Volta request
//
//	NOTE: Customer [Volta] requested that they have a feature to allow their 
//	system[battery pack] to turn-off the charger. This was implemented on the 
//	legacy (TI DSP) NP system using the temperature sensor connection, because:
//	1) they incorporate the temperature sensor into their battery pack and they
//	don't use the one on our charger, and 2) it provides a convenient connector. 
//	This implementation was propagated into the dsPIC version.
//
//
//   DC Converter options (only used for IS_DEV_CONVERTER)
//      OPTION_DCDC_DFLT_SETPOINT  (nn.mmm) // volts (float)
//      OPTION_DCDC_DCOUT_VOLTS must be set to one of these
//        CAUTION! Dont change these number since they are exported
          #define   DCDC_DCOUT_12V        12
          #define   DCDC_DCOUT_24V        24
          #define   DCDC_DCOUT_48V        48
          #define   DCDC_DCOUT_51V        51
//
// ------------------------------------------------------------------------------------------------------------

#if defined OPTION_UL_TEST_CONFIG
  // [2017-11-22] NOTE: This is a shortcut until we have a more clear 
  //	definition of U.L. Test Mode.
  #ifndef   OPTION_IGNORE_OVERLOAD
    #define OPTION_IGNORE_OVERLOAD
  #endif
  #ifndef   OPTION_DISABLE_CHARGE_TIMER_SHUTDOWN
    #define OPTION_DISABLE_CHARGE_TIMER_SHUTDOWN
  #endif
  #ifndef   OPTION_CHARGER_CV_MODE_ONLY
    #define OPTION_CHARGER_CV_MODE_ONLY
  #endif
#else
	#undef OPTION_IGNORE_OVERLOAD
	#undef OPTION_DISABLE_CHARGE_TIMER_SHUTDOWN	
	#undef OPTION_CHARGER_CV_MODE_ONLY
#endif

    #define STRINGIFY(x)    #x
    #define NUM2STR(x)      STRINGIFY(x)    // expands numeric macros to strings

    // include the models to build
    #include "Models\models.h"
	
    // include the calibration file
    #include "Cals\cals.h"
	
	// -------------------
	// Readability macros
	// -------------------
	// Build Type
    #define IS_BUILD_RELEASE   (BUILD_TYPE == BUILD_RELEASE)
    #define IS_BUILD_DVT       (BUILD_TYPE == BUILD_DVT    ) 
    #define IS_BUILD_DEV       (BUILD_TYPE == BUILD_DEV    ) 

	// Device Type
    #define IS_DEV_INVERTER   (OPTION_DEV == DEV_INVERTER )
    #define IS_DEV_CONVERTER  (OPTION_DEV == DEV_CONVERTER) 

    // Printed Circuit Board
    #define IS_PCB_NP         (OPTION_PCB == PCB_NP   ) 
    #define IS_PCB_LPC        (OPTION_PCB == PCB_LPC  ) 
    #define IS_PCB_BDC        (OPTION_PCB == PCB_BDC  ) 

    // DC Input (Battery) Voltage
    #define IS_DCIN_12V       (OPTION_DCIN == DCIN_12V) 
    #define IS_DCIN_24V       (OPTION_DCIN == DCIN_24V) 
    #define IS_DCIN_48V       (OPTION_DCIN == DCIN_48V) 
    #define IS_DCIN_51V       (OPTION_DCIN == DCIN_51V) 


    // ------------------------------
    //  Test for MANDATORY settings 
    // ------------------------------
	
    // Firmware String ID
    #ifndef FW_ID_STRING
      #error 'FW_ID_STRING' not set in 'models.h'
    #endif

    // Build Date
    #ifndef BUILD_DATE
      #error 'BUILD_DATE' not set in 'models.h'
    #endif

    // Model Number String
    #ifndef MODEL_NO_STRING
      #error 'MODEL_NO_STRING' not set in 'models.h'
    #endif

    // Build Type
	#if   IS_BUILD_RELEASE
      #define BUILD_TYPE_STR  "Release"
      #define OPTION_NO_CONDITIONAL_DBG  1
    #elif IS_BUILD_DVT
      #define BUILD_TYPE_STR  "DVT"
      #define OPTION_NO_CONDITIONAL_DBG  1
    #elif IS_BUILD_DEV
      #define BUILD_TYPE_STR  "Dev"
    #else
      #error 'BUILD_TYPE' not set in 'models.h'
    #endif

    // Device Type
	#if !IS_DEV_INVERTER & !IS_DEV_CONVERTER
      #error 'OPTION_DEV' not set in 'models.h'
    #endif

    // PCB revision
    #ifndef OPTION_PCB_REV
      #error 'OPTION_PCB_REV' not set in 'models.h'
    #endif

    // DC Input Supply Voltage
    #if !IS_DCIN_51V & !IS_DCIN_48V & !IS_DCIN_24V & !IS_DCIN_12V
      #error 'OPTION_DCIN' input voltage not set in 'models.h'
    #endif

    // Calibration must be set valid
    #if (OPTION_CAL != CAL_12LPC ) & \
        (OPTION_CAL != CAL_51LPC ) & \
        (OPTION_CAL != CAL_12NP18) & \
        (OPTION_CAL != CAL_12NP20) & \
        (OPTION_CAL != CAL_12NP24) & \
        (OPTION_CAL != CAL_12NP30) & \
        (OPTION_CAL != CAL_24NP24) & \
        (OPTION_CAL != CAL_24NP36) & \
        (OPTION_CAL != CAL_48NPXX) & \
        (OPTION_CAL != CAL_51NPXX) & \
        (OPTION_CAL != CAL_12DC51) & \
        (OPTION_CAL != CAL_51DC12) & \
        (OPTION_CAL != CAL_51DC24)
      #error 'OPTION_CAL' not set in 'models.h'
     #endif

	// ---------------------------------------
	//	Features forced by hardware selection
	// ---------------------------------------

    // Non-Volatile-Memory channel determined by hardware selection
    #define	OPTION_NVM_NONE		0
	#define	OPTION_NVM_I2C1		1
	#define	OPTION_NVM_I2C2		2

    // Serial Port channel determined by hardware selection
    #define	OPTION_SERIAL_DEBUG_NONE	0
    #define	OPTION_SERIAL_DEBUG_UART1	1
    #define	OPTION_SERIAL_DEBUG_UART2	2
    // OPTION_UART2   - debug console output - default is UART1

    // LPC
	#if IS_PCB_LPC
		#define OPTION_SERIAL_DEBUG	  OPTION_SERIAL_DEBUG_UART1
		#define OPTION_NVM			  OPTION_NVM_I2C1 
	
    // NP
	#elif IS_PCB_NP

        // serial port is on UART1 for both NP_1 and NP_2 boards (2018-03-15)
		#define OPTION_SERIAL_DEBUG	  OPTION_SERIAL_DEBUG_UART1

      #if (OPTION_PCB_REV == PCB_REV_NP_2)
		#define OPTION_NVM			  OPTION_NVM_I2C1
      #else
		#define OPTION_NVM			  OPTION_NVM_I2C2 
      #endif

    // BDC	
	#elif IS_PCB_BDC
		#define OPTION_SERIAL_DEBUG	  OPTION_SERIAL_DEBUG_UART1
		#define OPTION_NVM			  OPTION_NVM_I2C1 
	
    // Not Defined
	#else
      #error 'OPTION_PCB' board type not set in 'models.h'
    #endif

    // ------------------------------
    // Ensure forced settings are set
    // ------------------------------

    // Serial Port setting
	#if (OPTION_SERIAL_DEBUG == OPTION_SERIAL_DEBUG_UART2)
	    #define OPTION_UART2
	#elif (OPTION_SERIAL_DEBUG == OPTION_SERIAL_DEBUG_NONE)
	  #if !defined(OPTION_NO_LOGGING)
		#define OPTION_NO_LOGGING
	  #endif
	#endif

    // ---------------------------------------------
    // Include Headers Dependent upon option flags
    // ---------------------------------------------
    #if defined(OPTION_SSR)
      #include "ssr.h"
    #endif
    
    #if defined(OPTION_SERIAL_IFC)
      #include "serial_ifc.h"
    #endif

    // --------------------------
    // Charger Options
    // --------------------------
	  
    #if !defined(OPTION_HAS_CHARGER)
        // no charger
        // charger type can not be defined
      #if defined(OPTION_CHARGE_3STEP) | defined(OPTION_CHARGE_LION)
        #error Charger type defined without charger in 'models.h'
      #endif
    #else
        // charger defined
        // charging type must be defined
      #if !defined(OPTION_CHARGE_3STEP) & !defined(OPTION_CHARGE_LION)
        #error 'OPTION_CHARGE_nnn' charger type not set in 'models.h'
      #endif
    #endif

    #if defined(OPTION_HAS_CHGR_EN_SWITCH)
	  #if !defined(OPTION_HAS_CHARGER)
        #error OPTION_HAS_CHGR_EN_SWITCH defined without OPTION_HAS_CHARGER 'models.h'
	  #endif
	  #if !IS_PCB_NP
        #error OPTION_HAS_CHGR_EN_SWITCH can only be used for NP PCA
	  #endif
    #endif
	
    // A/C line qualification over-ride
    #if defined(OPTION_AC_LINE_QUAL_SECS)
      #define AC_LINE_QUAL_SECS_OVER_RIDDEN
    #else
      #define OPTION_AC_LINE_QUAL_SECS  30  // default if not specified
    #endif

    // --------------------------
    // DC+DC Converter Validation
    // --------------------------
    #if IS_DEV_CONVERTER

      // DC Output Supply Voltage
      #if (OPTION_DCDC_DCOUT_VOLTS != DCDC_DCOUT_12V) & (OPTION_DCDC_DCOUT_VOLTS != DCDC_DCOUT_24V) & (OPTION_DCDC_DCOUT_VOLTS != DCDC_DCOUT_48V) & (OPTION_DCDC_DCOUT_VOLTS != DCDC_DCOUT_51V)
        #error DC Convverter 'OPTION_DCDC_DCOUT_VOLTS' output voltage not set in 'models.h'
      #endif

      // Default Set Point must be specified
      #if !defined(OPTION_DCDC_DFLT_SETPOINT)
        #error 'OPTION_DCDC_DFLT_SETPOINT' not set in 'models.h'
      #endif

    #else
      // Not a DC converter; set values
      #define OPTION_DCDC_DCOUT_VOLTS    (0)  // not used
      #define OPTION_DCDC_DFLT_SETPOINT (0.0) // not a converter
    #endif

#endif  //  _OPTIONS_H

// <><><><><><><><><><><><><> options.h <><><><><><><><><><><><><><><><><><><><><><>
