// <><><><><><><><><><><><><> Cal_12LPC.h <><><><><><><><><><><><><><><><><><><><><><>
// ----------------------------------------------------------------------------
// Analog Calibration for 12LPC
//
// Calibration Equation:
//    adc = SLOPE x unit + INTERCEPT
//
// ----------------------------------------------------------------------------

#ifdef SHOW_CALIBRATION
 #ifndef WIN32
  #pragma message "12LPC Calibration"
 #endif
#endif

// -----------------------------------
// VBATT - Battery Voltage
// -----------------------------------

//  VBatt calibrated 2017-11-03 by Clark Dailey
#define VBATT_SLOPE     (62.096425)
#define VBATT_INTERCEPT (-9.468444)

// -----------------------------------
// VAC - A/C Voltage
// -----------------------------------

//  VAC calibrated 2017-11-03 by Clark Dailey
#define VAC_SLOPE       ( 2.238994)
#define VAC_INTERCEPT   (-2.428117)

// -----------------------------------
// VAC Pot Adjustment multiplier
// -----------------------------------

// adjust VAC pot to center
#define VAC_POT_ADJUST    (123.0/123.0)

// -----------------------------------
// IMEAS_INV - Inverter Current Sensor
// -----------------------------------

//  IMeas Inv Calibrated 2017-11-06 by Clark Dailey
#define IMEAS_INV_SLOPE     (54.712770)
#define IMEAS_INV_INTERCEPT (-0.010990)

// -----------------------------------
// IMEAS_CHG - Charger Current Sensor
// -----------------------------------

//  IMeas Chg Calibrated 2017-11-07 by Clark Dailey
#define IMEAS_CHG_SLOPE     ( 57.525184)
#define IMEAS_CHG_INTERCEPT (-10.344843)

// -----------------------------------
// IDC_INV - Inverter
// -----------------------------------

// Calibration using watts/vbatt is more accurate
#define IDC_INV_SLOPE       (4.647309)
#define IDC_INV_INTERCEPT   (3.852782)

// Calibrated 2017-11-07 by Clark Dailey
// DCIn (amps) = (VbattSlope*VbattAdc + VBattIntercept)*IMeasAdc + IMeasIntercept
#define IDC_INV_VBATT_SLOPE       (-0.0002225755)
#define IDC_INV_VBATT_INTERCEPT   ( 0.37157058)
#define IDC_INV_IMEAS_INTERCEPT   ( 3.79114836)

// -----------------------------------
// IDC_CHG - Charger
// -----------------------------------

// Calibration using watts/vbatt is more accurate
#define IDC_CHG_SLOPE       ( 3.252180)
#define IDC_CHG_INTERCEPT   (-3.060768)

// Calibrated 2017-11-07 by Clark Dailey
// DCOut (amps) = (ChgVbattSlope*VbattAdc + ChgVBattIntercept)*IMeasAdc + ChgIMeasIntercept
#define IDC_CHG_VBATT_SLOPE       (-0.0001460227)
#define IDC_CHG_VBATT_INTERCEPT   ( 0.24193679)
#define IDC_CHG_IMEAS_INTERCEPT   (-2.33589915)
#define IDC_CHG_MAX_AMPS		  (65)

// -----------------------------------
// ILINE - Line Current sensor
// -----------------------------------

// LPC I_LOAD_MGMT Calibrated 2018-02-05 by Clark Dailey
#define ILINE_SLOPE       (13.657638)    
#define ILINE_INTERCEPT   (-2.678409)

// -----------------------------------
// ILIMIT - Charger Current limit
// -----------------------------------

// maximum A/C amps allowed when charging
#define ILIMIT_CHG_AC_AMPS    (10.0)  // 67 amps DC at 12.6 vbatt 
// multiplier to tweak pot to center
#define ILIMIT_POT_ADJUST      (1.0)

// -----------------------------------
// WACR - Watts Calibration
// -----------------------------------

// Wattage calibrated 2017-11-03 by Clark Dailey
#define WACR_SLOPE      (16.329072)
#define WACR_INTERCEPT  (82.842520)

// -----------------------------------
// DCDC_OUT - DC-DC Converter Volt Out
// -----------------------------------

// not used
#define DCDC_SLOPE      (17.83)  
#define DCDC_INTERCEPT  (-6.4518)

// <><><><><><><><><><><><><> Cal_12LPC.h <><><><><><><><><><><><><><><><><><><><><><>
