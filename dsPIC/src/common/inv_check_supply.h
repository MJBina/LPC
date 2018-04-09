// <><><><><><><><><><><><><> inv_check_supply.h <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//  Copyright(C) 2015 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
//  This header is used to define the battery thresholds used by CheckSupply().
//	CheckSupply() is common to all platforms, therefore it is a common file.
//
//-----------------------------------------------------------------------------

#ifndef __INV_CHECK_SUPPLY_H    // include only once
#define __INV_CHECK_SUPPLY_H

// -------
// headers
// -------
#include "options.h"    // must be first include
#include "analog.h"

//-----------------------------------------------------------------------------
// SUPPLY VOLTAGE PARAMETERS
//  TODO:   Revise this section for improved accuracy
//  These parameters are used for detection of the low-voltage and high-voltage,
//  as well as related calculations.
//-----------------------------------------------------------------------------

#if IS_DCIN_51V
    // --------
    // 51 volts - For Farasis battery
    // --------
	
	//	NOTE: If you change the value of SUPPLY_NOMINAL, you must also
	//	re-calibrate the value of XFMR_EFF in the inverter ISR.
    #define SUPPLY_NOMINAL         VBATT_VOLTS_ADC(51.00)

    #define INV_DFLT_SUPPLY_LOW_SHUTDOWN    VBATT_VOLTS_ADC(32.00)
    #define INV_DFLT_SUPPLY_LOW_THRESHOLD   VBATT_VOLTS_ADC(47.00)
    #define INV_DFLT_SUPPLY_LOW_DELTA      (VBATT_VOLTS_ADC( 1.50)-VBATT_VOLTS_ADC(0))   // 48.50
    #define INV_DFLT_SUPPLY_LOW_RECOVER     VBATT_VOLTS_ADC(49.60)

    #define INV_DFLT_SUPPLY_HIGH_RECOVER    VBATT_VOLTS_ADC(56.00)
  #if IS_DEV_CONVERTER
    #define INV_DFLT_SUPPLY_HIGH_THRESHOLD  VBATT_VOLTS_ADC(65.0)    // per Kirk
    #define INV_DFLT_SUPPLY_HIGH_SHUTDOWN   VBATT_VOLTS_ADC(99.00)   // disable
  #else
    #define INV_DFLT_SUPPLY_HIGH_THRESHOLD  VBATT_VOLTS_ADC(59.20)
    #define INV_DFLT_SUPPLY_HIGH_SHUTDOWN   VBATT_VOLTS_ADC(61.00)
  #endif

#elif IS_DCIN_48V
    // --------
    // 48 volts
    // --------
    #define SUPPLY_NOMINAL         VBATT_VOLTS_ADC(50.40)

    #define INV_DFLT_SUPPLY_LOW_SHUTDOWN    VBATT_VOLTS_ADC(32.00)
    #define INV_DFLT_SUPPLY_LOW_THRESHOLD   VBATT_VOLTS_ADC(42.00)
    #define INV_DFLT_SUPPLY_LOW_DELTA      (VBATT_VOLTS_ADC( 3.00)-VBATT_VOLTS_ADC(0))   // 45.00
    #define INV_DFLT_SUPPLY_LOW_RECOVER     VBATT_VOLTS_ADC(54.00)

    #define INV_DFLT_SUPPLY_HIGH_RECOVER    VBATT_VOLTS_ADC(55.00)
    #define INV_DFLT_SUPPLY_HIGH_THRESHOLD  VBATT_VOLTS_ADC(58.00)
    #define INV_DFLT_SUPPLY_HIGH_SHUTDOWN   VBATT_VOLTS_ADC(61.00)
	
#elif IS_DCIN_24V
    // --------
    // 24 volts
    // --------
    #define SUPPLY_NOMINAL         VBATT_VOLTS_ADC(25.20)

    #define INV_DFLT_SUPPLY_LOW_SHUTDOWN    VBATT_VOLTS_ADC(17.00)
    #define INV_DFLT_SUPPLY_LOW_THRESHOLD   VBATT_VOLTS_ADC(21.00)
    #define INV_DFLT_SUPPLY_LOW_DELTA      (VBATT_VOLTS_ADC( 1.50)-VBATT_VOLTS_ADC( 1.50))   // 22.50
    #define INV_DFLT_SUPPLY_LOW_RECOVER     VBATT_VOLTS_ADC(27.00)

    #define INV_DFLT_SUPPLY_HIGH_RECOVER    VBATT_VOLTS_ADC(32.00)
    #define INV_DFLT_SUPPLY_HIGH_THRESHOLD  VBATT_VOLTS_ADC(33.00)
    #define INV_DFLT_SUPPLY_HIGH_SHUTDOWN   VBATT_VOLTS_ADC(34.00)

#elif IS_DCIN_12V
    // --------
    // 12 volts
    // --------
    #define SUPPLY_NOMINAL         VBATT_VOLTS_ADC(12.60)

    #define INV_DFLT_SUPPLY_LOW_SHUTDOWN    VBATT_VOLTS_ADC( 8.00)
    #define INV_DFLT_SUPPLY_LOW_THRESHOLD   VBATT_VOLTS_ADC(10.50)
    #define INV_DFLT_SUPPLY_LOW_DELTA      (VBATT_VOLTS_ADC( 0.75)-VBATT_VOLTS_ADC(0))  // 11.25
    #define INV_DFLT_SUPPLY_LOW_RECOVER     VBATT_VOLTS_ADC(13.50)

    #define INV_DFLT_SUPPLY_HIGH_RECOVER    VBATT_VOLTS_ADC(16.00)
    #define INV_DFLT_SUPPLY_HIGH_THRESHOLD  VBATT_VOLTS_ADC(20.00)
    #define INV_DFLT_SUPPLY_HIGH_SHUTDOWN   VBATT_VOLTS_ADC(24.00)
#else
    // -----------------------
    // Not defined is an error
    // -----------------------
    #error OPTION_nnV Input Voltage not set in 'options.h'
#endif


// ----------------------------------
//   Handle Special Over-Ride Cases
// ----------------------------------

// Auto Tester (Final Tester) is Custom Firmware
#if defined(MODEL_12DC51_FW0065)
    #undef SUPPLY_NOMINAL                  
    #undef INV_DFLT_SUPPLY_LOW_SHUTDOWN    
    #undef INV_DFLT_SUPPLY_LOW_THRESHOLD   
    #undef INV_DFLT_SUPPLY_LOW_DELTA
    #undef INV_DFLT_SUPPLY_LOW_RECOVER     
    #undef INV_DFLT_SUPPLY_HIGH_RECOVER    
    #undef INV_DFLT_SUPPLY_HIGH_THRESHOLD  
    #undef INV_DFLT_SUPPLY_HIGH_SHUTDOWN   

    #define SUPPLY_NOMINAL                  VBATT_VOLTS_ADC(12.60)
    #define INV_DFLT_SUPPLY_LOW_SHUTDOWN    VBATT_VOLTS_ADC( 6.00)
    #define INV_DFLT_SUPPLY_LOW_THRESHOLD   VBATT_VOLTS_ADC( 8.00)
    #define INV_DFLT_SUPPLY_LOW_DELTA      (VBATT_VOLTS_ADC( 2.00)-VBATT_VOLTS_ADC(0))   // 10.00
    #define INV_DFLT_SUPPLY_LOW_RECOVER     VBATT_VOLTS_ADC(10.50)
    #define INV_DFLT_SUPPLY_HIGH_RECOVER    VBATT_VOLTS_ADC(29.00)
    #define INV_DFLT_SUPPLY_HIGH_THRESHOLD  VBATT_VOLTS_ADC(30.00)
    #define INV_DFLT_SUPPLY_HIGH_SHUTDOWN   VBATT_VOLTS_ADC(35.00)
#endif

// Low Batt Hysteresis = low batt threshold + delta
#define INV_DFLT_SUPPLY_LOW_HYSTERESIS  (INV_DFLT_SUPPLY_LOW_THRESHOLD + INV_DFLT_SUPPLY_LOW_DELTA) // adc counts

#define VBATT_NOMINAL   SUPPLY_NOMINAL

// ------------
// Prototyping
// ------------
extern void inv_CheckSupply(int8_t reset);

#endif	//	__INV_CHECK_SUPPLY_H

// <><><><><><><><><><><><><> inv_check_supply.h <><><><><><><><><><><><><><><><><><><><><><>
