// <><><><><><><><><><><><><> rvc_types.h <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//  Copyright(C) 2016 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
//  Description:
//      Tpes compatable with the RV-C specification.
//      Table 5.3 - Basic data scale definition
//  +--------+---------+----------------+----------------+-----------+---------------+
//  | Unit   |Data type| Min            | Max            | Precision | Special values|
//  +--------+---------+----------------+----------------+-----------+---------------+
//  |  %     | uint8   |     0          |   125          |    0.5%   | -             |
//  +--------+---------+----------------+----------------+-----------+---------------+
//  |Instance| uint8   |     0          |   250          |           | 0 = all       |
//  +--------+---------+----------------+----------------+-----------+---------------+
//  |  °C    | uint8   |   -40          |   210          |     1 °C  | -             |
//  |        | uint16  |  -273          |  1735          | 0.03125 °C| -             |
//  +--------+---------+----------------+----------------+-----------+---------------+
//  |  V     | uint8   |     0          |   250          |     1V    | -             |
//  |        | uint16  |     0          |  3212.5        | 0.050V    | -             |
//  +--------+---------+----------------+----------------+-----------+---------------+
//  |  A     | uint8   |     0          |   250          |     1A    | -             |
//  |        | uint16  | -1600          |  1612.5        |  0.05A    | 0A=0x7D00     |
//  |        | uint32  |2,000,000.000A  | 2,221,081.200A | 0.001A    | 0A=0x77359400 |
//  +--------+---------+----------------+----------------+-----------+---------------+

#ifndef _RVC_TYPES_H_    // include only once
#define _RVC_TYPES_H_

// -------
// headers
// -------
#include <stdint.h>

// ---------------------------------
// Basic Types defined by RV-C spec
// ---------------------------------

// percentage
typedef uint8_t      PERCENT_t;

// device instance
typedef uint8_t      INSTANCE_t;
#define RVC_ALL_INSTANCES   (0)

// temperature Celcius
typedef uint8_t      CELSIUS8_t;
typedef uint16_t     CELSIUS16_t;
typedef CELSIUS16_t  CELSIUS_t;
#define RVC_CELSIUS16_INVALID   (0xFFFF)

// voltages
typedef uint8_t      VOLTAGE8_t;
typedef uint16_t     VOLTAGE16_t;
typedef VOLTAGE16_t  VOLTAGE_t;
#define RVC_VOLTAGE16_INVALID   (0xFFFF)

// amperage
typedef uint8_t      AMPERAGE8_t;
typedef uint16_t     AMPERAGE16_t;
typedef uint32_t     AMPERAGE32_t;
typedef AMPERAGE16_t AMPERAGE_t;


//-----------------------------------------------------------------------------
//             U N I T     C O N V E R S I O N     M A C R O S
//-----------------------------------------------------------------------------

// voltage
#define  VOLTS_TO_RVC16(volts)      ((VOLTAGE16_t)((volts)*20.0))
#define MVOLTS_TO_RVC16(mvolts)     ((VOLTAGE16_t)((mvolts)/50.0))
#define  RVC16_TO_VOLTS(rvc16)      ((rvc16)/20.0)
#define  RVC16_TO_VOLTSx10(rvc16)   ((rvc16)/2.0)
#define  RVC16_TO_AMPS(rvc16)       ((rvc16)/20.0)-1600.0)

// current
#define  AMPS_TO_RVC32(amps)        ((AMPERAGE32_t)(( ((amps)+2000000.0) *1000.0  )))
#define  AMPS_TO_RVC16(amps)        ((AMPERAGE16_t)(( ((amps)+1600.0   ) *20.0    )))
#define MAMPS_TO_RVC16(mamps)       ((AMPERAGE16_t)(( ((mamps)/50.0    ) +32000.0 )))

// temperature Celcius
#define  CELCIUS_TO_RVC16(celcius)  ((CELSIUS16_t)(((celcius)*32)+8735))
#define  RVC16_TO_CELCIUS(rvc16)    (((rvc16+16)/32)-273)  // +.5 for rounding
#define  CELCIUS_TO_RVC8(celcius)   (celcius)  // no conversion needed
#define  RVC8_TO_CELCIUS(rvc8)      (rvc8)     // no conversion needed

// frequency (not defined by RVC)
#define  FREQ_TO_RVC16(freq)        ((freq)*128)

// power (not defined by RVC
#define   WATTS_TO_RVC16(watts)     (watts)
#define  MWATTS_TO_RVC16(mwatts)    ((mwatts)/1000)

#endif  //  _RVC_TYPES_H_

// <><><><><><><><><><><><><> rvc_types.h <><><><><><><><><><><><><><><><><><><><><><>
