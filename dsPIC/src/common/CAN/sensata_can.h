// <><><><><><><><><><><><><> sensata_can.h <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//	Copyright(C) 2015 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
//	RV-C Sensata CAN bus proprietary commands
//	
// -----------------------------------------------------------------------------------------------

#ifndef _SENSATA_CAN_H_	// include only once
#define _SENSATA_CAN_H_

// -------
// headers
// -------
#include "options.h"    // must be first include
#include "dsPIC33_CAN.h"


// ------------------------------------------
//  D A T A    G R O U P   N U M B E R S 
// ------------------------------------------

// Sensata Custom DGN
#define SENSATA_CUSTOM_SET_FIELD_DGN        0x1FA01
#define SENSATA_CUSTOM_SET_FIELD_RSP_DGN    0x1FA02
#define SENSATA_CUSTOM_GET_FIELD_DGN        0x1FA03
#define SENSATA_CUSTOM_GET_FIELD_RSP_DGN    0x1FA04

// Sensata Custom Field Types
typedef uint16_t  SENFLD; 

#define SENFLD_MAX_STRING   32

// Sensata Field Types
#define SENFLD_TYPE_NONE    0     
#define SENFLD_TYPE_INT16   1     
#define SENFLD_TYPE_FLOAT   2  
#define SENFLD_TYPE_INT32   3  
#define SENFLD_TYPE_STRING  4  // SENFLD_MAX_STRING
#define SENFLD_TYPE_Q16		5

// system commands
#define SENFLD_SAVE_TO_NVM                              1      // save to non-volatile memory (SetField)
#define SENFLD_TEST                                     2      // general test (SetField)
#define SENFLD_ENTER_BOOTLOADER                         3      // enter bootloader to allow firmware upgrades

// --------------------------------------------------------------
// Data Formats
//
//  STRING
//    1) text
//    2) allows a max length
//
//  INT16: 16 bit signed integer
//    1) range -32767 to +32767
//    2) consumes 2 bytes of memory
//    
//  UINT16: 16 bit unsigned integer
//    1) range 0 to 65535
//    2) consumes 2 bytes of memory
//
//  FLOAT: Floating point values
//    1) can use a decimal point such as 35.3456
//    2) consumes 4 bytes of memory
//    3) can be negative or positive
//    4) support 7 digits of precision (ie 7 decimal places)
//
// --------------------------------------------------------------

// Device Info
#define SENFLD_DEV_TYPE                                  10 	// UINT16 (read only) Device Type: 1=Inverter, 2=DC Converter
#define SENFLD_DEV_PCB                                   11     // UINT16 (read only) Device PCB   1=NP, 2=LPC
#define SENFLD_DEV_DCIN_VOLT                             12     // UINT16 (read only) Device Model Input Voltage: 12,24,48,51
#define SENFLD_DEV_FEATURES                              13     // UINT16 (read only) Device Feature Bitmask
  // bitmask values for SENFLD_DEV_FEATURES
  #define SENFLD_DEV_FEATURES_HAS_CHARGER            0x0001     // has charger capability
  #define SENFLD_DEV_FEATURES_HAS_CHARGER_EN_SWITCH  0x0002     // has charger enable switch
  #define SENFLD_DEV_FEATURES_HAS_SSR                0x0004     // has solid state relay clamp
  #define SENFLD_DEV_FEATURES_HAS_VOLTA_UI           0x0008    	// Volta user interface
  
#define SENFLD_DEV_STATUS                                14    	// UINT16 (read only) Device Status bits
#define SENFLD_DEV_ERRORS                                15    	// UINT16 (read only) Device Error  bits
#define SENFLD_DEV_HEAT_SINK_TEMP                        16    	// INT16  (read only) PWM Heat Sink temperature (Celcius)
#define SENFLD_DEV_AC_QUAL_TIMER                         17 	// INT16  (read only) Countdown timer - zero when ac_line_qualified
#define SENFLD_DEV_REMOTE_MODE                           18 	// INT16  remote    input mode 0=Disabled, 1=Snap, 2=Momentary, 3=custom        (LPC only)(LPC only; requires reboot when changed)
#define SENFLD_DEV_AUX_MODE                              19 	// INT16  auxillary input mode 0=Disabled, 1=RV,   2=Utility,   3=Wired Control (LPC only)(LPC only; requires reboot when changed)
#define SENFLD_DEV_PUSH_BTN_ENABLE                       20 	// INT16  push button enabled  0=disabled, 1=enabled (LPC only; requires reboot when changed)
#define SENFLD_DEV_HAS_BATT_TEMP_SENSOR                  21 	// INT16  0=no, 1=battery temp sensor is present

// can bus
#define SENFLD_CAN_BAUD                                 101    // UINT16 CAN Bus baud rate: 0=250k, 1=500k, 2=1m         (requires reboot)
#define SENFLD_CAN_ADDRESS                              102    // UINT16 RVC device id:     66=inverter, 67=dc converter (requires reboot)
#define SENFLD_CAN_INSTANCE                             103    // UINT16 Device Instance #: 1-13

// manufacturing                                        
#define SENFLD_MFG_MODEL_NO                             201    // STRING 16    (read only)  Mfg Model 
#define SENFLD_MFG_MFG_DATE                             202    // STRING 11    (read only)  Mfg Date
#define SENFLD_MFG_SERIAL_NO                            203    // STRING 17    (read only)  Mfg Unit Serial #
#define SENFLD_MFG_REV_NO                               204    // STRING 16+11 (read only)  Mfg Rev
#define SENFLD_MFG_FW_NO                                205    // STRING 16    (read only)  Firmware Rev

// ----------------------------------------------------------------------
// analog calibration    linear equation form  y = slope * x + intercept
//    units to adc counts:       adc   = (slope * units) + intercept                                               
//    adc counts to units:       units = (adc - intercept)/slope                                               
//    10 bit adc value  0-1023
// ----------------------------------------------------------------------

// battery voltage
#define SENFLD_ANA_VBATT_SLOPE                          301    // FLOAT  (read only)  Batt Volts (volts)  = (adc-intercept)/slope
#define SENFLD_ANA_VBATT_INTERCEPT                      302    // FLOAT  (read only)
#define SENFLD_ANA_VBATT_ADC                            303    // UINT16 (read only)  adc counts (0-1023)
// voltage regulator 15v
#define SENFLD_ANA_VREG15_SLOPE                         310    // FLOAT  (read only)  Voltage Regulator 15v = (adc-intercept)/slope
#define SENFLD_ANA_VREG15_INTERCEPT                     311    // FLOAT  (read only)
#define SENFLD_ANA_VREG15_ADC                           312    // UINT16 (read only)  adc counts (0-1023)
// voltage ac
#define SENFLD_ANA_VAC_SLOPE                            320    // FLOAT  (read only)  AC Volts (volts)    = (adc-intercept)/slope
#define SENFLD_ANA_VAC_INTERCEPT                        321    // FLOAT  (read only)
#define SENFLD_ANA_VAC_ADC                              322    // UINT16 (read only)  adc counts (0-1023)
#define SENFLD_ANA_VAC_MIN_ADC                          323    // UINT16 (read only)  adc counts (0-1023)
#define SENFLD_ANA_VAC_MAX_ADC                          324    // UINT16 (read only)  adc counts (0-1023)
// transformer current
#define SENFLD_ANA_IMEAS_SLOPE                          330    // FLOAT  (read only)  Xfrm Curr (amps)    = (adc-intercept)/slope
#define SENFLD_ANA_IMEAS_INTERCEPT                      331    // FLOAT  (read only)
#define SENFLD_ANA_IMEAS_ADC                            332    // UINT16 (read only)  adc counts (0-1023)
// line current
#define SENFLD_ANA_ILINE_SLOPE                          340    // FLOAT  (read only)  Line Curr (amps)    = (adc-intercept)/slope
#define SENFLD_ANA_ILINE_INTERCEPT                      341    // FLOAT  (read only)
#define SENFLD_ANA_ILINE_ADC                            342    // UINT16 (read only)  adc counts (0-1023)
#define SENFLD_ANA_ILIMIT_ADC                           343    // UINT16 (read only)  adc counts (0-1023)
#define SENFLD_ANA_ILIMIT_RATIO                         344    // FLOAT  (read only)  ratio of ILimit/IMeas
// DC-DC converter (HSTEMP)
#define SENFLD_ANA_DCDC_SLOPE                           350    // FLOAT  (read only)  DC-DC output (volts) = (adc-intercept)/slope
#define SENFLD_ANA_DCDC_INTERCEPT                       351    // FLOAT  (read only)
#define SENFLD_ANA_DCDC_ADC                             352    // UINT16 (read only)  adc counts (0-1023)

// A/D channel physical units
#define SENFLD_INV_DC_IN_VOLTS                          380    // FLOAT  (read only)  (volts    )
#define SENFLD_INV_DC_IN_AMPS                           381    // FLOAT  (read only)  (amps     )
#define SENFLD_INV_AC_OUT_VOLTS                         382    // FLOAT  (read only)  (volts RMS)
#define SENFLD_INV_AC_OUT_AMPS                          383    // FLOAT  (read only)  (amps  RMS)
#define SENFLD_CHG_AC_IN_VOLTS                          384    // FLOAT  (read only)  (volts RMS)
#define SENFLD_CHG_AC_IN_AMPS                           385    // FLOAT  (read only)  (amps  RMS)
#define SENFLD_CHG_DC_OUT_VOLTS                         386    // FLOAT  (read only)  (volts    )
#define SENFLD_CHG_DC_OUT_AMPS                          387    // FLOAT  (read only)  (amps     )
#define SENFLD_BATT_TEMP                                388    // FLOAT  (read only)  (Celcius  )
#define SENFLD_INV_AC_OUT_WATTS                         389    // FLOAT  (read only)  (Real Power)
#define SENFLD_CHG_BYPASS_AMPS                          390    // FLOAT  (read only)  ILine - IMeas
#define SENFLD_CHG_ILINE_AMPS                           391    // FLOAT  (read only)  (amps     )
#define SENFLD_AC_OUT_WATTS_ADC                         392    // INT16  (read only)  (adc      )
#define SENFLD_CNV_DC_OUT_VOLTS                         393    // FLOAT  (read only)  (volts    ) DC converter output voltage

// A/D channel physical units saved on last error
#define SENFLD_ERR_INV_DC_IN_VOLTS                     1380    // FLOAT  (read only)  (volts    )
#define SENFLD_ERR_INV_DC_IN_AMPS                      1381    // FLOAT  (read only)  (amps     )
#define SENFLD_ERR_INV_AC_OUT_VOLTS                    1382    // FLOAT  (read only)  (volts RMS)
#define SENFLD_ERR_INV_AC_OUT_AMPS                     1383    // FLOAT  (read only)  (amps  RMS)
#define SENFLD_ERR_CHG_AC_IN_VOLTS                     1384    // FLOAT  (read only)  (volts RMS)
#define SENFLD_ERR_CHG_AC_IN_AMPS                      1385    // FLOAT  (read only)  (amps  RMS)
#define SENFLD_ERR_CHG_DC_OUT_VOLTS                    1386    // FLOAT  (read only)  (volts    )
#define SENFLD_ERR_CHG_DC_OUT_AMPS                     1387    // FLOAT  (read only)  (amps     )
#define SENFLD_ERR_BATT_TEMP                           1388    // FLOAT  (read only)  (Celcius  )
#define SENFLD_ERR_INV_AC_OUT_WATTS                    1389    // FLOAT  (read only)  (Real Power)
#define SENFLD_ERR_CHG_BYPASS_AMPS                     1390    // FLOAT  (read only)  ILine - IMeas
#define SENFLD_ERR_CHG_ILINE_AMPS                      1391    // FLOAT  (read only)  (amps     )
#define SENFLD_ERR_AC_OUT_WATTS_ADC                    1392    // INT16  (read only)  (adc      )
#define SENFLD_ERR_CNV_DC_OUT_VOLTS                    1393    // FLOAT  (read only)  (volts    ) DC converter output voltage

// inverter load-sensing & timers
#define SENFLD_INV_LOADSENSE_ENABLE                     400    // UINT16 (0=disabled, 1=enabled)
#define SENFLD_INV_LOADSENSE_ENABLE_ON_STARTUP          401    // UINT16 (0=disabled, 1=enabled)
#define SENFLD_INV_LOADSENSE_ACTIVE                     402    // UINT16 (0=inactive, 1=active)
#define SENFLD_INV_LOADSENSE_DELAY                      403    // UINT16 delay before load-sense (pinging) [resolution = .5-sec] 
#define SENFLD_INV_LOADSENSE_INTERVAL                   404    // UINT16 delay between pings [resolution = .5-sec]               
#define SENFLD_INV_LOADSENSE_THRESHOLD                  405    // FLOAT  load RMS threshold (adc counts)
#define SENFLD_INV_LOADSENSE_TIMER                      406    // UINT16 start pinging when this timer expires

// shut down timer
#define SENFLD_TMR_SHUTDOWN_ENABLE	 					410    // UINT16 0=no, 1=enable
#define SENFLD_TMR_SHUTDOWN_ENABLE_ON_STARTUP			411    // UINT16 0=no, 1=enable
#define SENFLD_TMR_SHUTDOWN_DELAY                       412    // UINT16 Delay before Timer Shutdown [.5-sec resolution]
#define SENFLD_TMR_SHUTDOWN_TIMER                       413    // UINT16 shutdown when this timer expires

// inverter                                                     
#define SENFLD_INV_STATUS                               501    // UINT16 (read only)  bit mapped
#define SENFLD_INV_ERRORS                               502    // UINT16 (read only)  bit mapped
// P-I-D feedback loop tuning values
#define SENFLD_INV_AC_SETPOINT                          510    // FLOAT  AC Setpoint target (volts)
#define SENFLD_INV_P_GAIN                               511    // Q16  Proportional gain value  (0=no gain multiplier)
#define SENFLD_INV_I_GAIN                               512    // Q16  Integral gain value      (0=no gain multiplier)
#define SENFLD_INV_D_GAIN                               513    // Q16  Differential gain value  (0=no gain multiplier)
#define SENFLD_INV_I_LIMIT                              514    // UINT16 Integral limit value   (adc counts)
// supply (ie battery) voltage thresholds
#define SENFLD_INV_SUPPLY_LOW_HYSTER                    520    // FLOAT  Supply Low Hystesis   (volts)
#define SENFLD_INV_SUPPLY_LOW_RECOVER                   521    // FLOAT  Supply Recovery       (volts)
#define SENFLD_INV_SUPPLY_LOW_THRES                     522    // FLOAT  Supply Low Threshold  (volts)
#define SENFLD_INV_SUPPLY_LOW_SHUTDOWN                  523    // FLOAT  Supply LOw Shutdown   (volts)
#define SENFLD_INV_SUPPLY_HIGH_RECOVER                  524    // FLOAT  Supply High Recovery  (volts)
#define SENFLD_INV_SUPPLY_HIGH_THRES                    525    // FLOAT  Supply High Threshold (volts)
#define SENFLD_INV_SUPPLY_HIGH_SHUTDOWN                 526    // FLOAT  Supply High Shutdown  (volts)

// charger                                  
#define SENFLD_CHGR_STATUS                              601    // UINT16 (read only)  bit mapped
#define SENFLD_CHGR_ERRORS                              602    // UINT16 (read only)  bit mapped
#define SENFLD_CHGR_BRANCH_CIRCUIT_RATING               603    // UINT16 Branch Circuit Rating limit (amps) (0-30)
#define SENFLD_CHGR_AC_QUAL_MSEC                        604    // UINT16 AC Qualification Time (msecs) TODO ### Not Saved in NVM
#define SENFLD_CHGR_STATUS2                             605    // UINT16 (read only)  bit mapped
#define SENFLD_CHGR_AMPS_LIMIT                          606    // FLOAT  Charger Current Limit (Amps A/C)

// battery recipe parameters
#define SENFLD_CHGR_BATT_TYPE                           609    // INT16 Battery Type (0=flooded, 1=Gel, 2=AGM)
#define SENFLD_CHGR_BATT_RECIPE_FLOAT_THRESHOLD         610    // FLOAT  Float threshold (volts)
#define SENFLD_CHGR_BATT_RECIPE_CUSTOM_FLAG             611    // INT16  (0=standard, 1=custom recipe) // standard can not change any values
#define SENFLD_CHGR_BATT_RECIPE_CV_ROC_SDEV_THRES       612    // INT16  cv rate-of-change Std Dev Threshold (adc)
#define SENFLD_CHGR_BATT_RECIPE_CV_ROC_TIMEOUT          613    // INT16  cv rate-of-change timeout (minutes)
#define SENFLD_CHGR_BATT_RECIPE_EQ_VOLTS_MAX            614    // FLOAT  Equalization Maximum Voltage  (volts)
#define SENFLD_CHGR_BATT_RECIPE_EQ_VSETPOINT            615    // FLOAT  Equalization Voltage Setpoint (volts)
#define SENFLD_CHGR_BATT_RECIPE_CV_VSETPOINT            616    // FLOAT  CV Voltage Setpoint               (volts)
#define SENFLD_CHGR_BATT_RECIPE_FLOAT_VSETPOINT         617    // FLOAT  Float Voltage Threshold/Setpoint  (volts)
#define SENFLD_CHGR_BATT_RECIPE_WARM_BATT_TEMP_RECOV    618    // INT16  Change from Warm Batt to Normal   (Celsius)
#define SENFLD_CHGR_BATT_RECIPE_WARM_BATT_TEMP_THRES    619    // INT16  Change from Normal to Warm Batt   (Celsius)
#define SENFLD_CHGR_BATT_RECIPE_SHUTDOWN_TEMP_RECOV     620    // INT16  Change from Shutdown to Warm Batt (Celsius)
#define SENFLD_CHGR_BATT_RECIPE_SHUTDOWN_TEMP_THRES     621    // INT16  Change from Warm Batt to Shutdown (Celsius)
#define SENFLD_CHGR_BATT_RECIPE_FLOAT_TIMEOUT           623    // INT16  Float-mode Timeout (minutes)
#define SENFLD_CHGR_BATT_RECIPE_MAX_AMP_HOURS           624    // INT16  Constant-Current Amp*Hour Timeout (amp*hours)
#define SENFLD_CHGR_BATT_RECIPE_CV_VOLTS_MAX            625    // FLOAT  CV Maximum Voltage                (volts)
#define SENFLD_CHGR_BATT_RECIPE_FLOAT_VOLTS_MAX         626    // FLOAT  Float Maximum Voltage             (volts)
#define SENFLD_CHGR_BATT_RECIPE_EQ_TIMEOUT              627    // INT16  Equalization Timeout              (minutes)   
#define SENFLD_CHGR_BATT_RECIPE_CV_TIMEOUT              628    // INT16  CV Timeout                        (minutes)   
//	In general, Timers count-down. If Timer value is zero, then it has timed-
//	out, or is disabled. Refer to chgr_TimerOneMinuteUpdate() for details.
#define SENFLD_CHGR_VOLT_SETPOINT                       630    // INT16  Charge Voltage Setpoint           (volts) (Read Only)
#define SENFLD_CHGR_AMPS_SETPOINT                       631    // INT16  Charge Amperage Setpoint          (Amps)  (Read Only)
#define SENFLD_CHGR_CC_TIMER_AMP_MINUTES                632    // INT32  Const-Current Amp-Minutes Remaining (amp-minutes) (Read Only)
#define SENFLD_CHGR_CC_ELAPSED_MINUTES                  633    // INT16  Const-Current Time Elapsed        (minutes) (Read Only)
#define SENFLD_CHGR_CV_TIMER_MINUTES                    634    // INT16  Const-Voltage Time Remaining      (minutes) (Read Only)
#define SENFLD_CHGR_CV_ROC_TIMER_MINUTES                635    // INT16  Const-Voltage ROC Time Remaining  (minutes) (Read Only)
#define SENFLD_CHGR_FLOAT_TIMER_MINUTES                 636    // INT16  Float Time Remaining              (minutes) (Read Only)
#define SENFLD_CHGR_SS_CC_LM_TIMER_MINUTES              637    // INT16  Sum of SS, CC, and [Current-Limit] Time Remaining (minutes) (Read Only)
#define SENFLD_CHGR_CC_CV_TIMER_MINUTES                 638    // INT16  Sum of CC, and CV Time Remaining  (minutes) (Read Only)
#define SENFLD_CHGR_EQ_TIMER_MINUTES                    639    // INT16  Equalization Time Remaining       (minutes) (Read Only)

// default recipe parameters (read only values)
#define SENFLD_CHGR_DFLT_RECIPE_TYPE                   1609    // UINT16 Battery Type (0=flooded, 1=Gel, 2=AGM)
#define SENFLD_CHGR_DFLT_RECIPE_FLOAT_THRESHOLD        1610    // FLOAT  Float threshold (volts)
#define SENFLD_CHGR_DFLT_RECIPE_CUSTOM_FLAG            1611    // INT16  (0=standard, 1=custom recipe) // standard can not change any values
#define SENFLD_CHGR_DFLT_RECIPE_CV_ROC_SDEV_THRES      1612    // INT16  cv rate-of-change Std Dev Threshold (adc)
#define SENFLD_CHGR_DFLT_RECIPE_CV_ROC_TIMEOUT         1613    // INT16  cv rate-of-change timeout (minutes)
#define SENFLD_CHGR_DFLT_RECIPE_EQ_VOLTS_MAX           1614    // FLOAT  Equalization Maximum Voltage  (volts)
#define SENFLD_CHGR_DFLT_RECIPE_EQ_VSETPOINT           1615    // FLOAT  Equalization Voltage Setpoint (volts)
#define SENFLD_CHGR_DFLT_RECIPE_CV_VSETPOINT           1616    // FLOAT  CV Voltage Setpoint               (volts)
#define SENFLD_CHGR_DFLT_RECIPE_FLOAT_VSETPOINT        1617    // FLOAT  Float Voltage Threshold/Setpoint  (volts)
#define SENFLD_CHGR_DFLT_RECIPE_WARM_BATT_TEMP_RECOV   1618    // INT16  Change from Warm Batt to Normal   (Celsius)
#define SENFLD_CHGR_DFLT_RECIPE_WARM_BATT_TEMP_THRES   1619    // INT16  Change from Normal to Warm Batt   (Celsius)
#define SENFLD_CHGR_DFLT_RECIPE_SHUTDOWN_TEMP_RECOV    1620    // INT16  Change from Shutdown to Warm Batt (Celsius)
#define SENFLD_CHGR_DFLT_RECIPE_SHUTDOWN_TEMP_THRES    1621    // INT16  Change from Warm Batt to Shutdown (Celsius)
#define SENFLD_CHGR_DFLT_RECIPE_FLOAT_TIMEOUT          1623    // INT16  Float-mode Timeout (minutes)
#define SENFLD_CHGR_DFLT_RECIPE_MAX_AMP_HOURS          1624    // INT16  Constant-Current Amp*Hour Timeout (amp*hours)
#define SENFLD_CHGR_DFLT_RECIPE_CV_VOLTS_MAX           1625    // FLOAT  CV Maximum Voltage                (volts)
#define SENFLD_CHGR_DFLT_RECIPE_FLOAT_VOLTS_MAX        1626    // FLOAT  Float Maximum Voltage             (volts)
#define SENFLD_CHGR_DFLT_RECIPE_EQ_TIMEOUT             1627    // INT16  Equalization Timeout              (minutes)   
#define SENFLD_CHGR_DFLT_RECIPE_CV_TIMEOUT             1628    // INT16  CV Timeout                        (minutes)   


// DC+DC converter                                  
#define SENFLD_CONV_DCOUT_SETPOINT                      701    // FLOAT  DC converter set point target output voltage (volts)
#define SENFLD_CONV_P_GAIN								702	   // FLOAT  Proportional gain value  (0=no gain multiplier)
#define SENFLD_CONV_I_GAIN								703    // FLOAT  Integral gain value      (0=no gain multiplier)
#define SENFLD_CONV_D_GAIN                              704    // FLOAT  Differential gain value  (0=no gain multiplier)
#define SENFLD_CONV_I_LIMIT                             705    // UINT16 Integral limit value     (adc counts)
#define SENFLD_CONV_D_LIMIT                             706    // UINT16 Differential limit value (adc counts)
#define SENFLD_CONV_PID_DEADBAND                        707    // UINT16 PID deadband limit       (adc counts)
#define SENFLD_CONV_FF_GAIN                             708    // FLOAT  Feed Forward gain        (0=no gain multiplier)
#define SENFLD_CONV_FF_DEADBAND                         709    // INT16  Feed Forward deadband    (+/- adc counts)
#define SENFLD_CONV_DCOUT_VOLTS                         710    // FLOAT  (read only)  (volts    ) DC converter output voltage
#define SENFLD_CONV_DCOUT_VOLT_MODEL                    711    // INT16  12,24,48,51 (read only)

// testing
#define SENFLD_TEST_MODE                                900    // UINT16 (0=exit, 1=enter test mode)
#define SENFLD_TEST_COOLING_FAN                         901    // UINT16 (0-100) percent-on (0=off) (does not need test mode on)
#define SENFLD_TEST_LED                                 902    // UINT16 (0=off, 1=red, 2=green, 3=amber) (must be in test mode)

// debugging
#define SENFLD_DBG_STATE_MAIN                           950    // UINT16  Main     STate Machine value (read only)
#define SENFLD_DBG_STATE_INV                            951    // UINT16  Inverter STate Machine value (read only)
#define SENFLD_DBG_STATE_CHGR                           952    // UINT16  Charger  State Machine value (read only)


// Field Payload
//  data[0] = instance
//  data[1] = fieldNum lo byte
//  data[2] = fieldNum hi byte
//  data[3] = field Type
//  data[4] = LSB
//  data[5] = 
//  data[6] = 
//  data[7] = MSB for int32 and float

// max buffer needed for retrieving field data
#define SENFLD_MAX_BYTES     (MAX_CAN_DATA+SENFLD_MAX_STRING+1)  // 8 + 32 + 1 = 41

// -----------
// Prototyping
// -----------
int16_t  sen_SetField(CAN_DATA* msgData);
int16_t  sen_GetField(CAN_DATA* msgData, CAN_DATA* outData, CAN_LEN* ndata);
uint16_t IsInTestMode(void);
uint16_t GetLedTestColor(void);


#endif // _SENSATA_CAN_H_
