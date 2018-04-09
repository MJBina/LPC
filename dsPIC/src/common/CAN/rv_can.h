// <><><><><><><><><><><><><> rv_can.h <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//  Copyright(C) 2015 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
//  RV-C is a CAN-based communication profile for recreation vehicles.
//  It was developed by the Recreation Vehicle Industry Association (RVIA).
//  RVIA, PO Box 2999, Reston, VA 20195-0999, United States of America, Tel. +1-703-620-6003.
// 
//    CAN bus
//    250 kbits/s
//    29  bit addressing
//    LSByte send first
//    
//   RV-C draws heavily from the SAE J1939 protocol. 
//   The primary differences between J1939 and RV-C are:
//       1) RV-C does not encourage the use of shielded cable. 
//            Shielding increases cost significantly, with questionable benefit.
//       2) SAE J1939 does not support RV-C's "instancing".
//       3) The main diagnostic message (DM1) has somewhat different formats, 
//            due to the need in RV-C for instance identification.
//       4) The SAE J1939 NAME PGN is simplified in RV-C.
// 
//   References
//      http://en.wikipedia.org/wiki/RV-C
//      http://www.rv-c.com/
// -----------------------------------------------------------------------------------------------

#ifndef _RV_CAN_H_  // avoid repeated including
#define _RV_CAN_H_

// -------
// headers
// -------
#include "options.h"    // must be first include
#include "dsPIC33_CAN.h"
#include "J1939.h"
#include "rvc_types.h"
#include "structsz.h"

// ---------------------
// Default CAN Addresses
// ---------------------
// RVC Table 6.20.1
#define RVC_DEFAULT_INVERTER_ADDRESS1  (66)
#define RVC_DEFAULT_INVERTER_ADDRESS2  (67)

// RVC Table 6.21.1
#define RVC_DEFAULT_CHARGER_ADDRESS1   (74)
#define RVC_DEFAULT_CHARGER_ADDRESS2   (75)


// ----------------
// Address Claiming
// ----------------

// start at max and work way down RVC 7.2
#define RVC_DYNAMIC_ADDRESS_MAX   143
#define RVC_DYNAMIC_ADDRESS_MIN   128

// ------------------------------------------
//  D A T A    G R O U P   N U M B E R S 
// ------------------------------------------

//  PS Protocol Specific                           PS ----+--------+ 
//                                                        +-------+| 
//                                                                || 
//  PF Protocol Format                             PF ----+------+|| 
//   < F0 then target specific                            +-----+||| 
//     else entire network                                      |||| 
//                                       Data Page (0,1) ------+|||| 
//                                                             ||||| 
#define RVC_DGN_PROPRIETARY_INSTANCE                         0x1EF00    // target specific
#define RVC_DGN_DATE_TIME_STATUS                             0x1FFFF
#define RVC_DGN_SET_DATE_TIME_COMMAND                        0x1FFFE
#define RVC_DGN_DC_SOURCE_STATUS_1                           0x1FFFD
#define RVC_DGN_DC_SOURCE_STATUS_2                           0x1FFFC
#define RVC_DGN_DC_SOURCE_STATUS_3                           0x1FFFB
#define RVC_DGN_COMMUNICATION_STATUS_1                       0x1FFFA
#define RVC_DGN_COMMUNICATION_STATUS_2                       0x1FFF9
#define RVC_DGN_COMMUNICATION_STATUS_3                       0x1FFF8
#define RVC_DGN_WATERHEATER_STATUS                           0x1FFF7
#define RVC_DGN_WATERHEATER_COMMAND                          0x1FFF6
#define RVC_DGN_GAS_SENSOR_STATUS                            0x1FFF5
#define RVC_DGN_CHASSIS_MOBILITY_STATUS                      0x1FFF4
#define RVC_DGN_CHASSIS_MOBILITY_COMMAND                     0x1FFF3
#define RVC_DGN_AAS_CONFIG_STATUS                            0x1FFF2
#define RVC_DGN_AAS_CONFIG_COMMAND                           0x1FFF1
#define RVC_DGN_AAS_STATUS                                   0x1FFF0
#define RVC_DGN_AAS_SENSOR_STATUS                            0x1FFEF
#define RVC_DGN_LEVELING_CONTROL_COMMAND                     0x1FFEE
#define RVC_DGN_LEVELING_CONTROL_STATUS                      0x1FFED
#define RVC_DGN_LEVELING_JACK_STATUS                         0x1FFEC
#define RVC_DGN_LEVELING_SENSOR_STATUS                       0x1FFEB
#define RVC_DGN_HYDRAULIC_PUMP_STATUS                        0x1FFEA
#define RVC_DGN_LEVELING_AIR_STATUS                          0x1FFE9
#define RVC_DGN_SLIDE_STATUS                                 0x1FFE8
#define RVC_DGN_SLIDE_COMMAND                                0x1FFE7
#define RVC_DGN_SLIDE_SENSOR_STATUS                          0x1FFE6
#define RVC_DGN_SLIDE_MOTOR_STATUS                           0x1FFE5
#define RVC_DGN_FURNACE_STATUS                               0x1FFE4
#define RVC_DGN_FURNACE_COMMAND                              0x1FFE3
#define RVC_DGN_THERMOSTAT_STATUS_1                          0x1FFE2
#define RVC_DGN_AIR_CONDITIONER_STATUS                       0x1FFE1
#define RVC_DGN_AIR_CONDITIONER_COMMAND                      0x1FFE0
#define RVC_DGN_GENERATOR_AC_STATUS_1                        0x1FFDF
#define RVC_DGN_GENERATOR_AC_STATUS_2                        0x1FFDE
#define RVC_DGN_GENERATOR_AC_STATUS_3                        0x1FFDD
#define RVC_DGN_GENERATOR_STATUS_1                           0x1FFDC
#define RVC_DGN_GENERATOR_STATUS_2                           0x1FFDB
#define RVC_DGN_GENERATOR_COMMAND                            0x1FFDA
#define RVC_DGN_GENERATOR_START_CONFIG_STATUS                0x1FFD9     
#define RVC_DGN_GENERATOR_START_CONFIG_COMMAND               0x1FFD8     
#define RVC_DGN_INVERTER_AC_STATUS_1                         0x1FFD7 
#define RVC_DGN_INVERTER_AC_STATUS_2                         0x1FFD6 
#define RVC_DGN_INVERTER_AC_STATUS_3                         0x1FFD5 
#define RVC_DGN_INVERTER_STATUS                              0x1FFD4 
#define RVC_DGN_INVERTER_COMMAND                             0x1FFD3 
#define RVC_DGN_INVERTER_CONFIGURATION_STATUS_1              0x1FFD2 
#define RVC_DGN_INVERTER_CONFIGURATION_STATUS_2              0x1FFD1 
#define RVC_DGN_INVERTER_CONFIGURATION_COMMAND_1             0x1FFD0 
#define RVC_DGN_INVERTER_CONFIGURATION_COMMAND_2             0x1FFCF 
#define RVC_DGN_INVERTER_STATISTICS_STATUS                   0x1FFCE 
#define RVC_DGN_INVERTER_APS_STATUS                          0x1FFCD 
#define RVC_DGN_INVERTER_DCBUS_STATUS                        0x1FFCC 
#define RVC_DGN_INVERTER_OPS_STATUS                          0x1FFCB 
#define RVC_DGN_CHARGER_AC_STATUS_1                          0x1FFCA 
#define RVC_DGN_CHARGER_AC_STATUS_2                          0x1FFC9 
#define RVC_DGN_CHARGER_AC_STATUS_3                          0x1FFC8 
#define RVC_DGN_CHARGER_STATUS                               0x1FFC7 
#define RVC_DGN_CHARGER_CONFIGURATION_STATUS                 0x1FFC6 
#define RVC_DGN_CHARGER_COMMAND                              0x1FFC5 
#define RVC_DGN_CHARGER_CONFIGURATION_COMMAND                0x1FFC4 
#define RVC_DGN_reserved                                     0x1FFC3 
#define RVC_DGN_CHARGER_APS_STATUS                           0x1FFC2 
#define RVC_DGN_CHARGER_DCBUS_STATUS                         0x1FFC1 
#define RVC_DGN_CHARGER_OPS_STATUS                           0x1FFC0 
#define RVC_DGN_AC_LOAD_STATUS                               0x1FFBF 
#define RVC_DGN_AC_LOAD_COMMAND                              0x1FFBE 
#define RVC_DGN_DC_LOAD_STATUS                               0x1FFBD 
#define RVC_DGN_DC_LOAD_COMMAND                              0x1FFBC 
#define RVC_DGN_DC_DIMMER_STATUS_1                           0x1FFBB 
#define RVC_DGN_DC_DIMMER_STATUS_2                           0x1FFBA 
#define RVC_DGN_DC_DIMMER_COMMAND                            0x1FFB9 
#define RVC_DGN_DIGITAL_INPUT_STATUS                         0x1FFB8 
#define RVC_DGN_TANK_STATUS                                  0x1FFB7 
#define RVC_DGN_TANK_CALIBRATION_COMMAND                     0x1FFB6 
#define RVC_DGN_TANK_GEOMETRY_STATUS                         0x1FFB5 
#define RVC_DGN_TANK_GEOMETRY_COMMAND                        0x1FFB4 
#define RVC_DGN_WATER_PUMP_STATUS                            0x1FFB3 
#define RVC_DGN_WATER_PUMP_COMMAND                           0x1FFB2 
#define RVC_DGN_AUTOFILL_STATUS                              0x1FFB1 
#define RVC_DGN_AUTOFILL_COMMAND                             0x1FFB0 
#define RVC_DGN_WASTEDUMP_STATUS                             0x1FFAF 
#define RVC_DGN_WASTEDUMP_COMMAND                            0x1FFAE 
#define RVC_DGN_ATS_AC_STATUS_1                              0x1FFAD 
#define RVC_DGN_ATS_AC_STATUS_2                              0x1FFAC 
#define RVC_DGN_ATS_AC_STATUS_3                              0x1FFAB 
#define RVC_DGN_ATS_STATUS                                   0x1FFAA 
#define RVC_DGN_ATS_COMMAND                                  0x1FFA9 
#define RVC_DGN_RESERVED_1                                   0x1FFA8 
#define RVC_DGN_RESERVED_2                                   0x1FFA7 
#define RVC_DGN_RESERVED_3                                   0x1FFA6 
#define RVC_DGN_WEATHER_STATUS_1                             0x1FFA5 
#define RVC_DGN_WEATHER_STATUS_2                             0x1FFA4 
#define RVC_DGN_ALTIMETER_STATUS                             0x1FFA3 
#define RVC_DGN_ALTIMETER_COMMAND                            0x1FFA2 
#define RVC_DGN_WEATHER_CALIBRATE_COMMAND                    0x1FFA1 
#define RVC_DGN_COMPASS_BEARING_STATUS                       0x1FFA0 
#define RVC_DGN_COMPASS_CALIBRATE_COMMAND                    0x1FF9F 
#define RVC_DGN_BRIDGE_COMMAND                               0x1FF9E 
#define RVC_DGN_BRIDGE_DGN_LIST                              0x1FF9D 
#define RVC_DGN_THERMOSTAT_AMBIENT_STATUS                    0x1FF9C     
#define RVC_DGN_HEAT_PUMP_STATUS                             0x1FF9B 
#define RVC_DGN_HEAT_PUMP_COMMAND                            0x1FF9A 
#define RVC_DGN_CHARGER_EQUALIZATION_STATUS                  0x1FF99 
#define RVC_DGN_CHARGER_EQUALIZATION_CONFIGURATION_STATUS    0x1FF98 
#define RVC_DGN_CHARGER_EQUALIZATION_CONFIGURATION_COMMAND   0x1FF97 
#define RVC_DGN_CHARGER_CONFIGURATION_STATUS_2               0x1FF96 
#define RVC_DGN_CHARGER_CONFIGURATION_COMMAND_2              0x1FF95 
#define RVC_DGN_GENERATOR_AC_STATUS_4                        0x1FF94 
#define RVC_DGN_GENERATOR_ACFAULT_CONFIGURATION_STATUS_1     0x1FF93 
#define RVC_DGN_GENERATOR_ACFAULT_CONFIGURATION_STATUS_2     0x1FF92 
#define RVC_DGN_GENERATOR_ACFAULT_CONFIGURATION_COMMAND_1    0x1FF91 
#define RVC_DGN_GENERATOR_ACFAULT_CONFIGURATION_COMMAND_2    0x1FF90 
#define RVC_DGN_INVERTER_AC_STATUS_4                         0x1FF8F 
#define RVC_DGN_INVERTER_ACFAULT_CONFIGURATION_STATUS_1      0x1FF8E 
#define RVC_DGN_INVERTER_ACFAULT_CONFIGURATION_STATUS_2      0x1FF8D 
#define RVC_DGN_INVERTER_ACFAULT_CONFIGURATION_COMMAND_1     0x1FF8C 
#define RVC_DGN_INVERTER_ACFAULT_CONFIGURATION_COMMAND_2     0x1FF8B 
#define RVC_DGN_CHARGER_AC_STATUS_4                          0x1FF8A 
#define RVC_DGN_CHARGER_ACFAULT_CONFIGURATION_STATUS_1       0x1FF89 
#define RVC_DGN_CHARGER_ACFAULT_CONFIGURATION_STATUS_2       0x1FF88 
#define RVC_DGN_CHARGER_ACFAULT_CONFIGURATION_COMMAND_1      0x1FF87 
#define RVC_DGN_CHARGER_ACFAULT_CONFIGURATION_COMMAND_2      0x1FF86 
#define RVC_DGN_ATS_AC_STATUS_4                              0x1FF85 
#define RVC_DGN_ATS_ACFAULT_CONFIGURATION_STATUS_1           0x1FF84 
#define RVC_DGN_ATS_ACFAULT_CONFIGURATION_STATUS_2           0x1FF83 
#define RVC_DGN_ATS_ACFAULT_CONFIGURATION_COMMAND_1          0x1FF82 
#define RVC_DGN_ATS_ACFAULT_CONFIGURATION_COMMAND_2          0x1FF81 
#define RVC_DGN_GENERATOR_DEMAND_STATUS                      0x1FF80 
#define RVC_DGN_GENERATOR_DEMAND_COMMAND                     0x1FEFF 
#define RVC_DGN_AGS_CRITERION_STATUS                         0x1FEFE 
#define RVC_DGN_AGS_CRITERION_COMMAND                        0x1FEFD 
#define RVC_DGN_FLOOR_HEAT_STATUS                            0x1FEFC 
#define RVC_DGN_FLOOR_HEAT_COMMAND                           0x1FEFB 
#define RVC_DGN_THERMOSTAT_STATUS_2                          0x1FEFA 
#define RVC_DGN_THERMOSTAT_COMMAND_1                         0x1FEF9 
#define RVC_DGN_THERMOSTAT_COMMAND_2                         0x1FEF8 
#define RVC_DGN_THERMOSTAT_SCHEDULE_STATUS_1                 0x1FEF7 
#define RVC_DGN_THERMOSTAT_SCHEDULE_STATUS_2                 0x1FEF6 
#define RVC_DGN_THERMOSTAT_SCHEDULE_COMMAND_1                0x1FEF5 
#define RVC_DGN_THERMOSTAT_SCHEDULE_COMMAND_2                0x1FEF4 
#define RVC_DGN_AWNING_STATUS                                0x1FEF3 
#define RVC_DGN_AWNING_COMMAND                               0x1FEF2 
#define RVC_DGN_TIRE_RAW_STATUS                              0x1FEF1 
#define RVC_DGN_TIRE_STATUS                                  0x1FEF0 
#define RVC_DGN_TIRE_SLOW_LEAK_ALARM                         0x1FEEF 
#define RVC_DGN_TIRE_TEMPERATURE_CONFIGURATION_STATUS        0x1FEEE 
#define RVC_DGN_TIRE_PRESSURE_CONFIGURATION_STATUS           0x1FEED 
#define RVC_DGN_TIRE_PRESSURE_CONFIGURATION_COMMAND          0x1FEEC 
#define RVC_DGN_TIRE_TEMPERATURE_CONFIGURATION_COMMAND       0x1FEEB 
#define RVC_DGN_TIRE_ID_STATUS                               0x1FEEA 
#define RVC_DGN_TIRE_ID_COMMAND                              0x1FEE9 
#define RVC_DGN_INVERTER_DC_STATUS                           0x1FEE8 
#define RVC_DGN_GENERATOR_DEMAND_CONFIGURATION_STATUS        0x1FEE7 
#define RVC_DGN_GENERATOR_DEMAND_CONFIGURATION_COMMAND       0x1FEE6 
#define RVC_DGN_LOCK_STATUS                                  0x1FEE5 
#define RVC_DGN_LOCK_COMMAND                                 0x1FEE4 
#define RVC_DGN_WINDOW_STATUS                                0x1FEE3 
#define RVC_DGN_WINDOW_COMMAND                               0x1FEE2 
#define RVC_DGN_DC_MOTOR_CONTROL_COMMAND                     0x1FEE1 
#define RVC_DGN_DC_MOTOR_CONTROL_STATUS                      0x1FEE0 
#define RVC_DGN_WINDOW_SHADE_CONTROL_COMMAND                 0x1FEDF 
#define RVC_DGN_WINDOW_SHADE_CONTROL_STATUS                  0x1FEDE 
#define RVC_DGN_AC_LOAD_STATUS_2                             0x1FEDD 
#define RVC_DGN_DC_LOAD_STATUS_2                             0x1FEDC 
#define RVC_DGN_DC_DIMMER_COMMAND_3                          0x1FEDB 
#define RVC_DGN_DC_DIMMER_STATUS_3                           0x1FEDA 
#define RVC_DGN_GENERIC_INDICATOR_COMMAND                    0x1FED9 
#define RVC_DGN_GENERIC_CONFIGURATION_STATUS                 0x1FED8 
#define RVC_DGN_GENERIC_INDICATOR_STATUS                     0x1FED7 
#define RVC_DGN_MFG_SPECIFIC_CLAIM_REQUEST                   0x1FED6 
#define RVC_DGN_AGS_DEMAND_CONFIGURATION_STATUS              0x1FED5 
#define RVC_DGN_AGS_DEMAND_CONFIGURATION_COMMAND             0x1FED4 
#define RVC_DGN_GPS_STATUS                                   0x1FED3 
#define RVC_DGN_AGS_CRITERION_STATUS_2                       0x1FED2 
#define RVC_DGN_SUSPENSION_AIR_PRESSURE_STATUS               0x1FED1 
#define RVC_DGN_DM_RV                                        0x1FECA    // diagnostics 
#define RVC_DGN_GENERAL_RESET                                0x17F00    // target specific 
#define RVC_DGN_DOWNLOAD                                     0x17E00    // target specific 
#define RVC_DGN_TERMINAL                                     0x17D00    // target specific 
#define RVC_DGN_INSTANCE_ASSIGNMENT                          0x17C00    // target specific 
#define RVC_DGN_INSTANCE_STATUS                              0x17B00    // target specific 
#define RVC_DGN_PRODUCT_IDENTIFICATION                       0x0FEEB    // make*model*sn*unitno
// Magnum proprietary
#define RVC_DGN_PROP_MAGNUM_INVERTER_STATUS                  0x0EF42    // Magnum Proprietary


// ------------------------------------------------------
//  S T A N D A R D    A C K N O W L E D G M E N T S
// ------------------------------------------------------
#define RVC_ACK                 0   // J1939_ACK
#define RVC_NAK                 1   // J1939_NAK
#define RVC_BAD_CMD             2
#define RVC_NOT_EXECUTABLE      3
#define RVC_BAD_FORMAT          4
#define RVC_OUT_OF_RANGE        5
#define RVC_NO_PASSWORD         6
#define RVC_NEEDS_MORE_TIME     7
#define RVC_OVER_RIDDEN         8
//   9-127 Reserved
// 128-254 Command specific

// -----------------------------------------------
//    M A N U F A C T U R E R     C O D E S
// -----------------------------------------------
#define RVC_MFG_Atwood_Mobile_Products                    101 
#define RVC_MFG_Carefree_of_Colorado                      102 
#define RVC_MFG_Dometic_Corporation                       103 
#define RVC_MFG_Freightliner_Custom_Chassis_Corp          104 
#define RVC_MFG_General_Dynamics_Intellitec_Products      105 
#define RVC_MFG_Girard_Systems                            106 
#define RVC_MFG_Hopkins_Manufacturing_Corp                107 
#define RVC_MFG_HWH_Corporation                           108 
#define RVC_MFG_Integrated_Power_Systems                  109 
#define RVC_MFG_Onan_Cummins_Power_Generation             110 
#define RVC_MFG_Progressive_Dynamics_Inc                  111 
#define RVC_MFG_SilverLeaf_Electronics_Inc                112 
#define RVC_MFG_Spartan_Motors_Chassis_Inc                113 
#define RVC_MFG_Technology_Research_Corporation           114 
#define RVC_MFG_Transportation_Systems_Design_Inc         115 
#define RVC_MFG_Vehicle_Systems_Inc                       116 
#define RVC_MFG_Wire_Design_Inc                           117 
#define RVC_MFG_Workhorse_Custom_Chassis                  118 
#define RVC_MFG_Xantrex_Technology_Inc                    119 
#define RVC_MFG_Power_Gear                                120 
#define RVC_MFG_RV_Products                               121 
#define RVC_MFG_Suburban                                  122 
#define RVC_MFG_Borg_Warner                               123 
#define RVC_MFG_Garnet_Instruments                        124 
#define RVC_MFG_American_Technology                       125 
#define RVC_MFG_Automated_Engineering_Corp                126 

#define RVC_MFG_Sensata                                   593 // assigned by SAE in 2014


// ----------------------------------------------------------------------------
//  S E R V I C E    P O I N T S    F O R    C H A R G E R  / I N V E R T E R
// ----------------------------------------------------------------------------

// Use these to identify specific problems with the device

// byte 1
#define SPN_MSB1_DC_VOLTAGE                      0 
#define SPN_MSB1_DC_CURRENT                      1 
#define SPN_MSB1_BATTERY_TEMPERATURE             2 
#define SPN_MSB1_DC_SOURCE_STATE_OF_CHAREG       3 
#define SPN_MSB1_DC_SOURCE_STATE_OF_HEALTH       4 
#define SPN_MSB1_DC_SOURCE_CAPACITY              5 
#define SPN_MSB1_DC_COUSRCE_AC_RIPPLE            6 
#define SPN_MSB1_AC_BACKFEED                     7 

// byte 2
#define SPN_MSB2_FET_1_TEMPERATURE               0 
#define SPN_MSB2_FET_2_TEMPERATURE               1 

// byte 3
#define SPN_MSB3_DC_BULK_CAPACITOR_TEMPERATURE   0 
#define SPN_MSB3_TRANSFORMER_TEMPERATURE         1 
#define SPN_MSB3_AMBIENT_TEMPERATURE             2 
#define SPN_MSB3_BATTERY_CHARGER_TIMEOUT         3 
#define SPN_MSB3_BATTERY_EQUALIZATION            4 
#define SPN_MSB3_DC_BRIDGE                       5 
#define SPN_MSB3_TRANSFER_RELAY                  6 
#define SPN_MSB3_STACKING_CONFIGURATION          7 

// byte 4
#define SPN_MSB4_STACKING_COMMUNICATIONS         0 
#define SPN_MSB4_STACKING_SYNC_CLOCK             1 

// Service Points for AC Input and Output
#define SPN_MSB81H_RMS_VOLTAGE                   0 
#define SPN_MSB81H_RMS_CURRENT                   1 
#define SPN_MSB81H_FREQUENCY                     2 
#define SPN_MSB81H_OPEN_GROUND                   3 
#define SPN_MSB81H_OPEN_NEUTRAL                  4 
#define SPN_MSB81H_REVERSE_POLARITY              5 
#define SPN_MSB81H_GROUND_FAULT                  6 
#define SPN_MSB81H_PEAK_VOLTAGE                  7 

#define SPN_MSB82H_PEAK_CURRENT                  0 
#define SPN_MSB82H_GROUND_CURRENT                1 
#define SPN_MSB82H_REAL_POWER                    2 
#define SPN_MSB82H_REACTIVE_POWER                3 
#define SPN_MSB82H_HARMONIC_DISTORTION           4 
#define SPN_MSB82H_AC_PHASE_STATUS               5 



// -------------------
// Destination Address
// -------------------
#define RVC_DEST_ADDRESS_ALL            J1939_GLOBAL_ADDRESS  // all destinations

// values used when field is to be ignored
#define RVC_DONT_CHANGE_8BIT     0xFF      // dont change
#define RVC_DONT_CHANGE_16BIT    0xFFFF    // dont change
#define RVC_NOT_SUPPORTED_8BIT   0xFF      // not supported
#define RVC_NOT_SUPPORTED_16BIT  0xFFFF    // not supported

// instance Table 5.3
#define RVC_BROADCAST_INSTANCE   0         // instance used for broadcasting


// -------------------------
// structure packing on
// byte alignement required
// -------------------------
#pragma pack(1)  // structure packing on byte alignment

// ---------------
// inverter status
// ---------------
typedef struct
{
    // RVC 6.20.8 pg 100
    uint8_t     status;     // 0=disabled
    // byte 2
    uint8_t     tempSensorPresent : 2;   // 
    uint8_t     loadSenseEnabled  : 6;   // 
} RVCS_INV_STATUS;  // 2 bytes
STRUCT_SIZE_CHECK(RVCS_INV_STATUS,2);

// ------------------------
// inverter config status 1
// ------------------------
typedef struct
{
    // RVC 6.20.10 pg 101
    uint16_t    loadSensePowerThreshold;    
    uint16_t    loadSenseInterval;          
    uint16_t    minDcVoltShutdown;
    uint8_t     enableFlags;
} RVCS_INV_CFG_STATUS1; // 7 bytes
STRUCT_SIZE_CHECK(RVCS_INV_CFG_STATUS1,7);

// ------------------------
// inverter config status 2
// ------------------------
typedef struct
{
    // RVC 6.20.11 pg 102
    uint16_t    maxDcVoltShutdown;
    uint16_t    minDcVoltWarn;
    uint16_t    maxDcVoltWarn;
} RVCS_INV_CFG_STATUS2; // 6 bytes
STRUCT_SIZE_CHECK(RVCS_INV_CFG_STATUS2,6);

// ------------------------
// inverter dc status
// ------------------------
typedef struct
{
    // RVC 6.20.11 pg 102
    uint16_t    dcVoltage;
    uint16_t    dcAmperage;
} RVCS_INV_DC_STATUS;   // 4 bytes
STRUCT_SIZE_CHECK(RVCS_INV_DC_STATUS,4);

// ---------------
// ac point page 1
// ---------------
typedef struct
{
    // RVC 6.1.2 pg 27
    uint16_t    RMS_Volts;  
    uint16_t    RMS_Amps;           
    uint16_t    frequency;  // 0-500 hz
    // faults byte
    uint8_t     faultOpenGnd     : 2;   // 
    uint8_t     faultOpenNeutral : 2;   // 
    uint8_t     faultRevPolarity : 2;   // 
    uint8_t     faultGndCurrent  : 2;   // 
} RVCS_AC_PNT_PG1;  // 7 bytes
STRUCT_SIZE_CHECK(RVCS_AC_PNT_PG1,7);


// ---------------
// ac point page 2
// ---------------
typedef struct
{
    // RVC 6.1.3 pg 28
    uint16_t    peakVoltage;    // volts    
    uint16_t    peakCurrent;    // amps
    uint16_t    groundCurrent;  // amps
    uint8_t     capacity;       // amps
} RVCS_AC_PNT_PG2;  // 7 bytes
STRUCT_SIZE_CHECK(RVCS_AC_PNT_PG2,7);


// ---------------
// ac point page 3
// ---------------
typedef struct
{
    // RVC 6.1.4 pg 28
    uint8_t     waveformPhaseBits;
    uint16_t    realPower;          // watts
    int16_t     reactivePower;      // +/- VAr
    uint8_t     harmonicDistortion; // percent
    uint8_t     complementaryLeg;   // phase status
} RVCS_AC_PNT_PG3;  // MUST be 7 bytes
STRUCT_SIZE_CHECK(RVCS_AC_PNT_PG3        , 7);


// ---------------
// ac point page 4
// ---------------
typedef struct
{
    // RVC 6.1.5 pg 29
    uint8_t     voltageFault;   // 0=ok, 1=xlo-Volt, 2=lo-Volt, 3=hi-Volt, 4=xhi-Volt, 5=openLine1, 6=openLine2
    // faults byte
    uint8_t     faultSurgeProtect : 2;   // 
    uint8_t     faultHighFreq     : 2;   // 
    uint8_t     faultLowFreq      : 2;   // 
    uint8_t     faultBypassActive : 2;   // 
} RVCS_AC_PNT_PG4;  // MUST be 2 bytes
STRUCT_SIZE_CHECK(RVCS_AC_PNT_PG4        , 2);


// ---------------
// ac fault status
// ---------------
typedef struct
{
    // RVC 6.1.6 pg 30
    uint8_t     extremeLowVoltage;      // exterme low voltage (volts)
    uint8_t     lowVoltage;             // low voltage (volts)
    uint8_t     highVoltage;            // high voltage (volts)
    uint8_t     extremeHighVoltage;     // extreme high voltage (volts)
    uint8_t     equalizationSeconds;    // 0-250 seconds            
    uint8_t     bypassMode;             // 0=normal, 1=bypass mode
} RVCS_AC_FAULT_STATUS_1;   // 6 bytes
STRUCT_SIZE_CHECK(RVCS_AC_FAULT_STATUS_1,6);

// -----------------
// ac fault status 2
// -----------------
typedef struct
{
    // RVC 6.1.7 pg 31
    uint8_t     hiFreqLimit;    // hz
    uint8_t     loFreqLimit;    // hz
} RVCS_AC_FAULT_STATUS_2;   // 2 bytes
STRUCT_SIZE_CHECK(RVCS_AC_FAULT_STATUS_2,2);

// ------------------
// dc source status 1
//  
//  Section 6.20 (Inverter) of the RV-C specification includes a note that says: 
//  "The DC Source Instance does not correspond to the Inverter or Charger 
//  Instance."
//  Section 6.5.2 (DC Source Status 1) includes comments to identify the 
//  instance values in comments below.
// ------------------
typedef struct
{
    // RVC 6.5.2 pg 40
    uint8_t     instance;       // 1=main house battery, 2=chassis start batt, 3=2nd house batt
    uint8_t     devicePriority; // 
    uint16_t    dcVoltage;      // 
    uint32_t    dcCurrent;      // <0=charging
} RVCS_DC_SOURCE_STATUS_1;  // 8 bytes
STRUCT_SIZE_CHECK(RVCS_DC_SOURCE_STATUS_1,8);

// ------------------
// dc source status 2
// ------------------
typedef struct
{
    // RVC 6.5.3 pg 41
    uint8_t     devicePriority; // 
    uint16_t    temperature;    // 
    uint8_t     stateOfCharge;  // 
    uint16_t    timeRemaining;  // 
} RVCS_DC_SOURCE_STATUS_2;  // 6 bytes
STRUCT_SIZE_CHECK(RVCS_DC_SOURCE_STATUS_2,6);

// ------------------
// dc source status 3
// ------------------
typedef struct
{
    // RVC 6.5.4 pg 42
    uint8_t     devicePriority;     //
    uint8_t     stateOfHealth;      // 
    uint16_t    capacityRemaining;  // 
    uint8_t     relativeCapacity;   // 
    uint16_t    acRmsRipple;        // 
} RVCS_DC_SOURCE_STATUS_3;  // 7 bytes
STRUCT_SIZE_CHECK(RVCS_DC_SOURCE_STATUS_3,7);

// ------------------
// charger status
// ------------------
typedef struct
{
    // RVC 6.21.8 pg 109
    uint16_t    chargeVoltage;  // 
    uint16_t    chargeCurrent;  // 
    uint8_t     chargePercent;  // 
    uint8_t     opState;        // 0=disabled 
    uint8_t     defaultPowerUp; //
} RVCS_CHARGER_STATUS;  // 7 bytes
STRUCT_SIZE_CHECK(RVCS_CHARGER_STATUS,7);

// ----------------------
// charger config status
// ----------------------
typedef struct
{
    // RVC 6.21.8 pg 109
    // byte 1
    uint8_t     chargingAlgorithm;  // 
    // byte 2
    uint8_t     chargerMode;        // 
    // byte 3
    uint8_t     batteryTempSensor  : 2; // 
    uint8_t     chargerInstallLine : 2; //
    uint8_t     batteryType        : 4; // 
    // byte 4,5
    uint16_t    batteryBankSize;    // 
    // byte 6,7
    uint16_t    maxChargeCurrent;   // 
} RVCS_CHARGER_CFG_STATUS;  // 7 bytes
STRUCT_SIZE_CHECK(RVCS_CHARGER_CFG_STATUS,7);

// ----------------------
// charger config status
// ----------------------
typedef struct
{
    // RVC 6.21.12 pg 113
    uint8_t     maxChargeCurrPercent;  // 
    uint8_t     chargeRateLimit;       // 
    uint8_t     shoreBreakerSize;      // 
    uint8_t     defaultBattTemp;       // 
    uint16_t    rechargeVoltage;       // 

    // Not RVC compliant so removed 2017-09-27
    // Sensata extension info
//  uint8_t     sensataStatus;         // status bits
//  uint8_t     sensataErrors;         // error  bits
//  uint8_t     trapErrCode;           // trap error code

} RVCS_CHARGER_CFG_STATUS2; // 6 bytes
STRUCT_SIZE_CHECK(RVCS_CHARGER_CFG_STATUS2,6);

// ---------------------------
// charger equalization status
// ---------------------------
typedef struct
{
    // Ref:		RVC 6.21.18,  rev December 17,2015
	// Name:	CHARGER_EQUALIZATION_STATUS
	// DGN:		1FF99h
    uint16_t    timeRemaining;     // 
    uint8_t     preChargingStatus; // 
} RVCS_CHARGER_EQUAL_STATUS;    // 3 bytes

STRUCT_SIZE_CHECK(RVCS_CHARGER_EQUAL_STATUS,3);

// ---------------
// instance status
// ---------------
typedef struct
{
    // RVC 6.2.4 pg 34
    uint8_t     deviceType;   // 
    uint8_t     baseInstance; // 
    uint8_t     maxInstance;  // 
    uint16_t    baseIntAddr;  // 
    uint16_t    maxIntAddr;   // 
} RVCS_INSTANCE_STATUS; // 7 bytes
STRUCT_SIZE_CHECK(RVCS_INSTANCE_STATUS,7);

// ---------------------
// generic config status
// ---------------------
typedef struct
{
    // RVC 6.3.2 pg 37
    // byte 0
    uint8_t     mfgCodeLSB; // 
    // byte 1
    uint8_t     mfgCodeMSG   : 3; // 
    uint8_t     funcInstance : 5; //
    // byte 2
    uint8_t     function;    // 
    // byte 3
    uint8_t     fwRevision;  // 
    // byte 4
    uint8_t     cfgTypeLSB;  // 
    // byte 5
    uint8_t     cfgTypeISB;  // 
    // byte 6
    uint8_t     cfgTypeMSB;  // 
    // byte 7
    uint8_t     cfgRevision; // 
} RVCS_GENERIC_CFG_STATUS;  // 8 bytes
STRUCT_SIZE_CHECK(RVCS_GENERIC_CFG_STATUS,8);

// ------------------
// diagnostics DM_RV
// ------------------
typedef struct
{
    // RVC 3.2.5.1 pg 12
    // byte 0
    uint8_t     opStatus1    : 2;   // 
    uint8_t     opStatus2    : 2;   // 
    uint8_t     yellowStatus : 2;   // 
    uint8_t     redStatus    : 2;   // 
    // byte 1
    uint8_t     DSA;                // Default Source Address
    // byte 2,3
    uint8_t     SPNmsb;             // Service Point Number (error code)
    uint8_t     SPNisb;             // 
    // byte 4
    uint8_t     FMI          : 5;   // 
    uint8_t     SPNlsb       : 3;   // 
    // byte 5
    uint8_t     occurance    : 7;   // 7Fh if not available
    uint8_t     reserved     : 1;   // always 1
    // byte 6
    uint8_t     DSAextension;       // FFh No DSA extension
    // byte 7
    uint8_t     bankSelect;         // 0-13; 0xF for none

} RVCS_DM_RV;   // 8 bytes
STRUCT_SIZE_CHECK(RVCS_DM_RV,8);

#pragma pack()  // restore packing setting

// -----------
// Prototyping
// -----------
void rvcan_Send_DM_RV(void);
void rvcan_SendGenericCfgStatus(void);
void rvcan_SendProductID(uint8_t* MakeModelSnUnit);
int  IsRvcCmdForMe(uint8_t instance);
void rvcan_BuildProductId(void);
uint8_t* rvcan_GetProductIdString(void);
void rvcan_SendMfgAddressClaimRequest(uint16_t mfgCode);
void rvcan_SendCommStatus1(void);
void rvcan_SendCommStatus2(void);
void rvcan_SendCommStatus3(void);
void rvcan_SendDcSourceStatus1(void);
void rvcan_SendDcSourceStatus2(void);
void rvcan_SendDcSourceStatus3(void);
void rvcan_SendInverterStatus(void);
void rvcan_SendInverterConfigStatus1(void);
void rvcan_SendInverterConfigStatus2(void);
void rvcan_SendInverterDcStatus(void);
void rvcan_SendInverterAcFaultCfgStatus1(void);
void rvcan_SendInverterAcFaultCfgStatus2(void);
void rvcan_SendInverterAcStatus1(void);
void rvcan_SendInverterAcStatus2(void);
void rvcan_SendInverterAcStatus3(void);
void rvcan_SendChargerStatus(void);
void rvcan_SendChargerConfigStatus(void);
void rvcan_SendChargerConfigStatus2(void);
void rvcan_SendChargerEqualStatus(void);
void rvcan_SendChargerAcFaultCfgStatus1(void);
void rvcan_SendChargerAcFaultCfgStatus2(void);
void rvcan_SendChargerAcStatus1(void);
void rvcan_SendChargerAcStatus2(void);
void rvcan_SendChargerAcStatus3(void);
void rvcan_SendChargerAcStatus4(void);
void rvcan_InverterCmd(uint8_t enableFlags, uint8_t startupFlags);
void rvcan_InverterCfgCmd1(uint16_t loadSenseThresh,     // power threshold in watts
                    uint16_t loadSenseInterval,  // seconds
                    uint16_t dcSrcShutdownMin);  // volts
void rvcan_InverterCfgCmd2(uint16_t dcSrcShutdownMax,    // volts
                    uint16_t dcSrcWarnMin,       // volts
                    uint16_t dcSrcWarnMax);      // volts
void rvcan_InverterAcFaultCtrlCfgCmd1(RVCS_AC_FAULT_STATUS_1* cfg);
void rvcan_InverterAcFaultCtrlCfgCmd2(RVCS_AC_FAULT_STATUS_2* cfg);
void rvcan_ResetCommStats(void);
void rvcan_GeneralReset(uint8_t isReboot,            // 0=no, 1=reboot cpu
                        uint8_t isClearFaults,       // 0=no, 1=clear faults
                        uint8_t isRestoreDefaults,   // 0=no, 1=restore to default values
                        uint8_t isResetCommStats);   // 0=no, 1=reset communication statistics
void rvcan_SendInstanceStatus(void);
void rvcan_InstanceAssignment(uint8_t deviceType, uint8_t baseInstance,
                              uint8_t maxInstance,uint16_t baseIntAddr,uint16_t maxIntAddr);
void rvcan_ChargerCmd(uint8_t status,         // 0=off, 1=on, >1=dont change
               uint8_t state_on_power_up);    // 0=charger disabled, 1=charger enabled
void rvcan_ChargerCfgCmd(uint8_t  chargingAlgorithm,     // 
                  uint8_t  chargerMode,          // 
                  uint8_t  batterySensePresent,  // 0=no, 1=yes
                  uint8_t  installationLine,     // 0=line1, 1=line2
                  uint16_t batteryBankSize,      // amp-hours; resolution 1 amp-hour
                  uint16_t batteryType,          // 
                  uint8_t  maxChargingCurrent);  // amps
void rvcan_ChargerCfgCmd2(uint8_t  chargePercent,        // max charge current; percent
                   uint8_t  chargeRatePercent,   // charge rate limit; percent of bank size
                   uint8_t  breakerSize);        // amps
void rvcan_ChargerEqualizationCmd(uint16_t  equalizationVolts,       // equalization voltage
                           uint16_t  equalizationMinutes);   // equalization time in minutes
void rvcan_ChargerAcFaultCtrlCfgCmd1(RVCS_AC_FAULT_STATUS_1* cfg);
void rvcan_ChargerAcFaultCtrlCfgCmd2(RVCS_AC_FAULT_STATUS_2* cfg);
void rvcan_SendPropInvStatus(uint8_t loDGN);
int16_t rvcan_Dispatcher(CAN_MSG* msg);


#endif // _RV_CAN_H_

// <><><><><><><><><><><><><> rv_can.h <><><><><><><><><><><><><><><><><><><><><><>
