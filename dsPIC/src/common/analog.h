// <><><><><><><><><><><><><> analog.h <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//  Copyright(C) 2015 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
//  Analog to Digial Conversion  Routines
//
//  This header is used to define the analog signals that are collected by
//  the ADC10, and their related processing.  This processing includes; scaling 
//  and/or averaging and conversions to other formats for display etc.
//
//  Analog functions are included in: "analog_dsPIC33F.c" and "analog.c".  
//
//  "analog_dsPIC33F.c" has functions specific to the hardware, for example, 
//  the dsPIC33F uses DMA to interface with the ADC. This is different than the 
//  predecessor dsPIC30F.  
//
//  "analog.c" has generic functions such as re-scaling or conversion of the 
//  analog data.
//
//  Some definitions and variables associated with the use of these analog
//  signals are included in functional-specific header files. These includes
//  thresholds, levels, and setpoints. For example; Battery voltage thresholds
//  and definitions, which are used by the inverter are included in
//  'inverter.h'
//
//  Ping-Pong Buffering
//  RMS values are computed on a multiple of the AC cycle. The sum-of-
//  squares is calculated when the ADC sample is acquired. The square-root
//  calculation is done in a foreground process, it may span more than one
//  sample time if necessary. A semaphore is used to assure mutually
//  exclusive access to the sum-of-squares value. A simple semaphore is
//  implemented as described below:
//
//  1) The there are two copies of the rms_sos[] (an array of 2). One copy
//     indexed by 'RmsSosIndxIsr' is updated (written) by the ISR while the
//     other copy indexed by 'RmsSosIndxFg' is used (read) by the fore-
//     ground task (main loop).
//
//  2) At zero-crossing, the required number of samples have been collected,
//     the (Inverter & Charger) PWM ISR will then:
//     a) Swap the values of indexes  RmsSosIndxFg & RmsSosIndxIsr
//     b) Sets the rms_sos[_RmsSosIndxIsr] == 0
//     c) Queue the task 'TASK_AnalogData'.
//
//  Some considerations: When running in Charge Mode, the number of samples
//  may vary slightly since we are dependent on the AC line. However, near
//  zero-crossing the amplitudes are minimal, and a few missing, or extra
//  samples do not make a significant difference.
//
//-----------------------------------------------------------------------------
//
//  optimize = 1  (nanoseconds)
//     int16 * int16   100 nsec
//     int16 / int16   560 nsec
//     int32 * int32   280 nsec
//     int32 / int32   840 nsec
//     float * float  2000 nsec
//     float / float  4800 nsec
//
//  Floating Point Calculations  
//     timing is for 1,000,000 loops
//
//                   optimize=0        optimize=1
//    Operation    msec   usec/op    msec   usec/op
//    c = a + b    5094    5.094     4687    4.687
//    c = a * b    3833    3.833     3425    3.425
//    c = a / b    8317    8.317     7908    7.908
//
//-----------------------------------------------------------------------------

#ifndef __ANALOG_H    // include only once
#define __ANALOG_H

// --------
// headers
// --------
#include "options.h"    // must be first include


// ----------------------------------
// Analog to Digital Converter ranges
// ----------------------------------
#define MAX_A2D_VALUE  (1023) // 10 bit value
#define MIN_A2D_VALUE  (0)
#define MID_A2D_VALUE  (512)  // 10 bit value


// Volts AC is dc biased to analog mid point using 1.65 volt reference
#define VAC_ZERO_CROSS_A2D     (MID_A2D_VALUE)  // A/D counts

// A/C Current is dc biased to anaolog mid point
#define IMEAS_ZERO_CROSS_A2D   (MID_A2D_VALUE*4)  // A/D counts; Imeas x 4 for higher resolution

// ---------------------
// Conversion Equations
// ---------------------

#define VOLTS_MVOLTS(volts) ((int32_t)((volts)*1000L))  // volts to millivolts
#define   AMPS_MAMPS( amps) ((int32_t)(( amps)*1000L))  // amps  to milliamps
#define WATTS_MWATTS(watts) ((int32_t)((watts)*1000L))  // watts to millwatts


// ---------------------
// Calibration Equations
// ---------------------

// linear conversion equations
#define UNITS_TO_ADC(units,slope,intercept)    ((int16_t)(((slope)*(units)) + (intercept)))
#define ADC_TO_UNITS(adc,  slope,intercept)    ((((float)(adc))-(intercept))/(slope))

// convert physical units (volts/amps) to adc counts
#define   VBATT_VOLTS_ADC(volts)  UNITS_TO_ADC(volts, VBATT_SLOPE,      VBATT_INTERCEPT    )
#define IMEAS_INV_AMPS_ADC(amps)  UNITS_TO_ADC(amps,  IMEAS_INV_SLOPE,  IMEAS_INV_INTERCEPT)
#define IMEAS_CHG_AMPS_ADC(amps)  UNITS_TO_ADC(amps,  IMEAS_CHG_SLOPE,  IMEAS_CHG_INTERCEPT)
#define     ILINE_AMPS_ADC(amps)  UNITS_TO_ADC(amps,  ILINE_SLOPE,      ILINE_INTERCEPT    )
#define     VAC_VOLTS_ADC(volts)  UNITS_TO_ADC(volts, VAC_SLOPE,        VAC_INTERCEPT      )
#define    DCDC_VOLTS_ADC(volts)  UNITS_TO_ADC(volts, DCDC_SLOPE,       DCDC_INTERCEPT     )

// convert adc counts to physical units (volts/amps/watts)
// benchmark testing shows conversion takes 5 usecs
#define    VBATT_ADC_VOLTS(adc)   ADC_TO_UNITS(adc,   VBATT_SLOPE,      VBATT_INTERCEPT    )
#define IMEAS_INV_ADC_AMPS(adc)   ADC_TO_UNITS(adc,   IMEAS_INV_SLOPE,  IMEAS_INV_INTERCEPT)
#define IMEAS_CHG_ADC_AMPS(adc)   ADC_TO_UNITS(adc,   IMEAS_CHG_SLOPE,  IMEAS_CHG_INTERCEPT)
#define     ILINE_ADC_AMPS(adc)   ADC_TO_UNITS(adc,   ILINE_SLOPE,      ILINE_INTERCEPT    )
#define      VAC_ADC_VOLTS(adc)   ADC_TO_UNITS(adc,   VAC_SLOPE,        VAC_INTERCEPT      )
#define     DCDC_ADC_VOLTS(adc)   ADC_TO_UNITS(adc,   DCDC_SLOPE,       DCDC_INTERCEPT     )
#define     WACR_ADC_WATTS(adc)   ADC_TO_UNITS(adc,   WACR_SLOPE,       WACR_INTERCEPT     )

// -------------------
// Inverter D/C Current
// -------------------
// using Watts/VBatt is more accurate, but watts not always available
#define   INV_DC_AMPS(VbattAdc,WattsAdc)   ((float)(IDC_INV_SLOPE * ((float)WattsAdc/(float)VbattAdc) + IDC_INV_INTERCEPT))
                                               
// using VBatt and IMeas is not as accurate
#define   IDC_INV_ADC_AMPS(VbattAdc,IMeasAdc)   ((float)((((IDC_INV_VBATT_SLOPE * VbattAdc) + IDC_INV_VBATT_INTERCEPT) * IMeasAdc) + IDC_INV_IMEAS_INTERCEPT))

// -------------------
// Charger D/C Current
// -------------------
// using Watts/VBatt is more accurate, but watts not always available
#define   CHG_DC_AMPS(VbattAdc,WattsAdc)        ((float)(IDC_CHG_SLOPE * ((float)WattsAdc/(float)VbattAdc) + IDC_CHG_INTERCEPT))

// using VBatt and IMeas is not as accurate
#define   IDC_CHG_ADC_AMPS(VbattAdc,IMeasAdc)   ((float)((((IDC_CHG_VBATT_SLOPE * VbattAdc) + IDC_CHG_VBATT_INTERCEPT) * IMeasAdc) + IDC_CHG_IMEAS_INTERCEPT))


// ------------------
// ILIMIT Calibration
// ------------------
#if IS_PCB_LPC
  // LPC does not have ILIMIT sensor so ILIMIT is set to IMEAS and Pot Multiplier is 1.0
  #define  ILIMIT_POT_MAX   (1.0)
  #define  ILIMIT_POT_MID   (1.0)
  #define  ILIMIT_POT_MIN   (1.0)
#else
  // ILIMIT - A/C current sensor with adjustable pot for dynamic calibration
  //   same signal as IMEAS with a variable gain
  //   ILIMIT = pot_mult * IMEAS      // Pot Position
  #define  ILIMIT_POT_MAX   (0.7359)  //  left (counter-clockwise)
  #define  ILIMIT_POT_MID   (0.4196)  //  center
  #define  ILIMIT_POT_MIN   (0.1152)  //  right (clockwise)
#endif

// ILIMIT Calibration ILIMIT = potMult * IMEAS * calTweakMult
#define  ILIMIT_AMPS_ADC(IMeasAmps, potMult)  IMEAS_CHG_AMPS_ADC((IMeasAmps) * (potMult) * ILIMIT_POT_ADJUST)	// A2D counts
#define  ILIMIT_ADC_AMPS(adc,       potMult)  ((float)IMEAS_CHG_ADC_AMPS(adc)/ (potMult  * ILIMIT_POT_ADJUST))


//-----------------------------------------------------------------------------
//                   1 5    V O L T    R E G U L A T O R
//-----------------------------------------------------------------------------
//	The signal '15V_RAIL_RESET' is the output of the 15VDC regulator what feeds 
//	the FET drivers and 3.3VDC regulator(s).
//    733 adc counts => 14.11 volts
//-----------------------------------------------------------------------------

#define	VREG15_SLOPE		(51.95)
#define	VREG15_INTERCEPT	(0)

#if IS_PCB_LPC
 #define VREG15_LOW_SHUTDOWN_VOLTS   (10.25) // volts per Pao 2017-11-20
 #define VREG15_LOW_RECOVER_VOLTS    (12.5)  // volts
#else
 #define VREG15_LOW_SHUTDOWN_VOLTS   (10.5)  // volts
 #define VREG15_LOW_RECOVER_VOLTS    (12.5)  // volts
#endif

//	Cutoff at 10.0VDC (using 10.5VDC due to inherent approx 2mS latency)
#define	VREG15_LOW_SHUTDOWN ((int16_t)((VREG15_LOW_SHUTDOWN_VOLTS * VREG15_SLOPE) + VREG15_INTERCEPT))

//	To accomodate recovery, we need hysteresis.
#define	VREG15_LOW_RECOVER 	((int16_t)((VREG15_LOW_RECOVER_VOLTS  * VREG15_SLOPE) + VREG15_INTERCEPT))


//-----------------------------------------------------------------------------
// ANALOG_READING_t - encapsulates the analog signal data and processing.
// Several analog readings require additional processing for averaging and
// RMS calculations. Processing is performed once per AC cycle. Samples of the
// signals are accumulated at the PWM clock rate and then processed at the
// zero-crossing (@see Ping-Pong Buffering).
//-----------------------------------------------------------------------------

typedef uint16_t   ADC10_RDG_t;   

// clip 10 bit A/D value to valid range
INLINE int16_t ClipADC10(int16_t adc)
{
    if (adc <  MIN_A2D_VALUE) return(MIN_A2D_VALUE);
    if (adc >= MAX_A2D_VALUE) return(MAX_A2D_VALUE-1);
    return(adc);
}

#pragma pack(1)  // structure packing on byte alignment
typedef struct
{
    ADC10_RDG_t raw_val;    // value read directly from the ADC
    struct
    {
        int32_t sum[2];     // sum used to calculate the average
        int16_t val;		// the average value
    } avg;
    struct
    {
        int32_t sos[2];     // sum-of-squares for calculating the RMS
        int16_t val;        // the RMS value
    } rms;
} ANALOG_READING_t;
#pragma pack()  // restore packing setting


// ------------------------
//  AC Current Averaging
// ------------------------
#define AC_AVG_LEN   16 // must be a power of 2;  256 msec per Pao testing 2018-03-20
#define AC_AVG_SHIFT 4

#pragma pack(1)  // structure packing on byte alignment
typedef struct
{
    int32_t sum;
    ADC10_RDG_t ay[AC_AVG_LEN];
    int16_t val;
} ANALOG_AVG_t;
#pragma pack()  // restore packing setting


// ------------------
// Analog Statistics
// ------------------

//  NOTE: The number of samples is a power of 2, to make math functions more
//  efficient. If MEAN_LEN (number of samples) is greater than 64 (i.e. 128)
//  then math overflows will result.
#define MEAN_LEN   64
#define MEAN_SHIFT 6
#pragma pack(1)  // structure packing on byte alignment
typedef struct
{
	const int16_t len;
	const int16_t shift;
    ADC10_RDG_t ay[MEAN_LEN];
	int16_t ix;
    int16_t mean;
    int32_t var;
	int16_t dev;	
} ANALOG_STATS_t;
#pragma pack()  // restore packing setting


// -------------
// Analog Status
// -------------

#pragma pack(1)  // structure packing on byte alignment
typedef struct
{
    int16_t  RmsSosIndxFg;      //  index used in the foreground. (see Ping-Pong Buffering)
    int16_t  RmsSosIndxIsr;     //  index used by the ISR. (see Ping-Pong Buffering)
    int16_t  RmsSosNSamples[2]; //  total number of samples for computing the RMS and Average values.
    
    ADC10_RDG_t VAC_raw_max;    //  max VAC raw value for last full cycle
    ADC10_RDG_t VAC_raw_min;    //  min VAC raw value for last full cycle

    ADC10_RDG_t VAC_run_max;    //  running VAC max value during this full cycle
    ADC10_RDG_t VAC_run_min;    //  running VAC min value during this full cycle

    ANALOG_READING_t VBatt;   	//  Battery Voltage: E10_SCALED, BATT_V
    ANALOG_READING_t VAC;     	//  AC Voltage: V_AC, INV_COMP
    ANALOG_READING_t IMeas;   	//  Inverter I-Out, Charger I-In: I_MEASURE_CL, I_MEASURE
    ANALOG_READING_t ILine;   	//  Line Current: I_LOAD_MGMT, LOAD_MGMT
    ANALOG_READING_t ILimit;  	//  Current Limit: CHRG_I_LIMIT
  	ANALOG_READING_t BTemp;   	//  Battery Temperature: BATT_TEMP
  	ANALOG_READING_t HsTemp;  	//  Heatsink Temperature: HS_BAR_IN; DC+DC uses for DC setpoint
  	ANALOG_READING_t VReg15;  	//  Regulated Voltage
    ADC10_RDG_t Ovl;           	//  Overload - VDS Signal raw value only! 
	ANALOG_READING_t WACr; 		//  Watts AC - Real Power
} ANALOG_STATUS_t;
#pragma pack()  // restore packing setting

#if IS_DEV_CONVERTER
 #if IS_PCB_BDC
  #define  DcOutput    VAC      // AN1
  #define  Thermistor  ILine    // AN5  TODO use this for BDC
 #else
  #define  DcOutput    HsTemp   // AN2
 #endif
#endif

// ------
// Analog
// ------

#pragma pack(1)  // structure packing on byte alignment
typedef struct
{
    ANALOG_STATUS_t Status;
    ANALOG_AVG_t AvgBTemp;
    ANALOG_AVG_t AvgILine;
    ANALOG_AVG_t AvgIMeas;   // ### not used
    ANALOG_AVG_t AvgILimit;  // ### not used
    ANALOG_AVG_t AvgWACr;
    int8_t  AvgValid;
} ANALOG_t;
#pragma pack()  // restore packing setting

// ------------------
// Access Global Data
// ------------------
extern ANALOG_t An;
extern ANALOG_t AnSaved;  // saved analog state for error diagnostics
extern uint8_t  devSaved; // 1=inverter, 2=charger
extern ANALOG_STATS_t WACr_stats;


//-----------------------------------------------------------------------------
//      A B S T R A C T I O N   /  R E A D A B I L I T Y    M A C R O S
//-----------------------------------------------------------------------------

// Root-Mean-Square values (A2D values)
#define  VacRMS()              (An.Status.VAC.rms.val)
#define  IMeasRMS()            (An.Status.IMeas.rms.val)
#define  ILineRMS()            (An.Status.ILine.rms.val)
#define  ILimitRMS()           (An.Status.ILimit.rms.val)

// One Reading Raw values (A2D values)
#define  VacRaw()              (An.Status.VAC.raw_val)
#define  IMeasRaw()            (An.Status.IMeas.raw_val)
#define  VReg15Raw()           ((int16_t)An.Status.VReg15.raw_val)
#define  VBattRaw()            (An.Status.VBatt.raw_val)

// One Cycle Average values (A2D values)
#define  VacCycleAvg()         (An.Status.VAC.avg.val)
#define  IMeasCycleAvg()       (An.Status.IMeas.avg.val)
#define  ILineCycleAvg()       (An.Status.ILine.avg.val)
#define  VBattCycleAvg()       (An.Status.VBatt.avg.val)
#define  IsCycleAvgValid()     (An.AvgValid)

// Long Average values (~second) (A2D values)
#define  ILineLongAvg()        (An.AvgILine.val)
#define  WattsLongAvg()        (An.AvgWACr.val)
#define  BattTempLongAvg()     (An.AvgBTemp.val)


// --------------------
// Function Prototyping
// --------------------
extern void    an_InitRms(void);
extern void    an_Config(void);
extern void    an_Start(void);
extern void    an_SaveState(int dev);
extern void    an_ProcessAnalogData(void);
// getters
extern float   an_GetBatteryVoltage(uint8_t saved);
extern float   an_GetChgrBatteryCurrent(uint8_t saved);
extern float   an_GetChgrBypassCurrent(uint8_t saved);
extern float   an_GetInvBatteryCurrent(uint8_t saved);
extern float   an_GetILimitCurrent(uint8_t saved);
extern float   an_GetILineCurrent(uint8_t saved);
extern float   an_GetInvIMeasCurrent(uint8_t saved);
extern float   an_GetChgrIMeasCurrent(uint8_t saved);
extern float   an_GetChgrILineCurrent(uint8_t saved);
extern float   an_GetInvAcVoltage(uint8_t saved);
extern float   an_GetInvAcAmps(uint8_t saved);
extern float   an_GetILimitRatio(void);
extern float   an_GetChgrAcVoltage(uint8_t saved);
extern float   an_GetACWatts(uint8_t saved);

#endif  //  __ANALOG_H

// <><><><><><><><><><><><><> analog.h <><><><><><><><><><><><><><><><><><><><><><>
