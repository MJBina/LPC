// <><><><><><><><><><><><><> inverter.h <><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//  Copyright(C) 2017 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
//  Description:
//
//      This file contains miscellaneous definitions and macros, specific to
//      to the inverter.  This file is common between the LPC and NP inverter.
//      (It should also be compatible with the LP, if revised in the future.)
//
//      The initial COMMON version will include #define options for each model.
//      These should be combined in the future to support a common interface
//      and configuration.
//
//  Definitions to guide development:
//      STATUS - reflects the operating conditions
//      CONFIGURATION - affects the operation
//      Inv.status.inv_active - Inverter PWM ISR is running and the task
//          inv_Driver() should be called from the main loop.
//      Inv.status.output_active - AC output is 'On' because of inverter
//
//-----------------------------------------------------------------------------
//  A high(er) level task monitors the flag Inv.status.inv_active.
//
//  inv_Driver() must be called while Inv.status.inv_active is set (1).
//
//  inv_Start() Configures and starts the Inverter function.
//
//  inv_StartBootCharge()   Configures and starts the Boot Charging (Cold-Start)
//          function. Includes timer for the boot-charge period.
//
//  inv_StartDegauss()   Configures and starts the Degauss function. Includes
//          timer for degauss period.
//
//  inv_Stop()  Requests the Inverter function to stop.
//          Inv.status.inv_active will be set to zero (0) after the inverter
//          has stopped.
//-----------------------------------------------------------------------------

#ifndef __INVERTER_H    // include only once
#define __INVERTER_H

// -------
// headers
// -------
#include "options.h"    // must be first include
#include "analog.h"
#include "device.h"
#include "inv_check_supply.h"
#include "Q16.h"


#if IS_PCB_LPC
    //-----------------------------------------------------------------------------
    // OVERLOAD DETECTION
    //  The Shoebox Specification, Rev6 provides the following requirements
    //  (NOTE: Some revision applied for brevity):
    //
    //     "In response to qualifying OVL conditions, the processor will truncate
    //      the existing PWM drive pulse, skip the subsequent PWM pulse and resume
    //      output on the [next] PWM pulse."
    //
    //     "There are be two thresholds as a percentage of nominal rated power:
    //      200% for one second and 105% for 4 additional seconds.
    //      After 5 seconds of OVL the unit shall latch off."
    //
    //  Overload is detected in _ADCInterrupt(). When the overload is detected,
    //  the drive output is terminated within _ADCInterrupt(), we must also signal
    //  _PWMInterrupt() to skip the next output pulse.
    //
    //  The desired operation, as implemented in other models, is that the duration
    //  of an overload comprises at least one overload occurring during contiguous
    //  AC cycles.

    //  OVL_SKIP_PULSE_COUNT - the number of PWM pulses skipped after an overload

    //  200% Overload is detected based upon the VDS (Drain-Source Voltage) of
    //  the FETs. The VDS signal is available to the dsPIC ADC on AN2. Refer to
    //  the signal labeled 'OVL' on schematics.

    //  Overload settings determined by Pao's testing 2014-01-23
    //  200% Overload Threshold: 1500 mV (1.500V)
    //-----------------------------------------------------------------------------
 #ifdef OPTION_HAS_CHARGER
  // -------------
  // LPC (Charger)
  // -------------
  #ifdef DCIN_12V
    #define OVL_VDS_THRES_VOLTS         (1.8)  // 12LPC: VDS design margin threshold for motor starting, 200% (1.5 sec avg) of rated AC PEAK current
  #else
    #error "OVL_VDS_THRES_VOLTS not defined for xxLPC"
  #endif
 #else
    // ------------------
    // LP (Inverter Only)
    // ------------------
  #ifdef DCIN_12V
    #define OVL_VDS_THRES_VOLTS         (1.9)  // 12LP: VDS design margin threshold for motor starting, 210% (1.5 sec avg) of rated AC PEAK current
  #else
    #error "OVL_VDS_THRES_VOLTS not defined for xxLP"
  #endif
 #endif
    #define OVL_VDS_THRES_ADC           ((int16_t)((OVL_VDS_THRES_VOLTS/3.300)*(MAX_A2D_VALUE+1)))
    #define OVL_VDS_SKIP_PULSE_COUNT    ((int16_t)(2))
    #define OVL_VDS_THRESHOLD           OVL_VDS_THRES_ADC


    //-----------------------------------------------------------------------------
    //  Secondary Overload is detected based upon the units output current. Output
    //  current signal is available to the dsPIC on AN4. Refer to the signal
    //  labeled 'I_MEASURE' on schematics.
	//
    //  The threshold value was determined experimentally. An initial approximation
    //  was achieved by reading the actual output current with a meter, and
    //  correlating those measurements with readings from the ADC. Final adjustment
    //  was an iterative process by observing the performance of the unit.
    //-----------------------------------------------------------------------------

    #define OVL_IMEAS_SKIP_PULSE_COUNT  ((int16_t)(2))

    //-----------------------------------------------------------------------------
    //  OVL_IMEAS_THRESHOLD
    //     Instantaneous current limit threshold. 
    //     When the instantaneous output current value exceeds this point, the PWM 
    //     output pulse will be terminated and successive pulses skipped, hence 
    //     limiting the output current.
    //     It is used to initialize Inv.config.ovl_imeas_thres (inv_LoadRomDefaults).
    //     Inv.config.ovl_imeas_thres is compared against the 
    //     rectified value of IMeas when the ADC is sampled (in the function 
    //     inv_CheckForOverload, called from the ADC DMA ISR.)
    //     It doe NOT affect the shutdown time-out.
    //-----------------------------------------------------------------------------
    #define OVL_IMEAS_THRESHOLD  IMEAS_INV_AMPS_ADC(20.3) // 115% IMEAS OVL peak clipping at 20.3Apeak (14.375Arms/.707) output (ADC=1110)

    //-----------------------------------------------------------------------------
    //  OVL_RMS_THRESHOLD
    //     Continuous current limit threshold. 
    //     The RMS output current is compared against this threshold.  If the RMS
    //     output is higher than this threshold, the overload timer is incremented,
    //     else the timer is decremented. If the overload timer exceeds the timeout 
    //     then an overload condition exists - causing the inverter to shutdown.
    //	   It does NOT alter the PWM pulse output to limit current.
    //-----------------------------------------------------------------------------
    #define OVL_RMS_THRESHOLD    IMEAS_INV_AMPS_ADC(13.5)  // 108% RMS OVL no clipping at 13.5Aac output) (ADC=738)
    
#endif


//-----------------------------------------------------------------------------
// LOAD-SENSE PARAMETERS
//-----------------------------------------------------------------------------

//	LP15: Forced configuration for now 
//	TODO: Determine better method for setting option details.
//	NOTE: This method assures that these settings would be applied if a failure
//		or command caused the defaults to be loaded from ROM.  
//		When updated, see additional, similar configuration in device.h
#define INV_DFLT_LOAD_SENSE_ENABLE      (0)
#define INV_DFLT_LOAD_SENSE_DELAY       ((uint16_t)(60*2))	//	30-seconds 
#define INV_DFLT_LOAD_SENSE_INTERVAL    ((uint16_t)(1*2))	//	one-second
#define INV_DFLT_LOAD_SENSE_THRESHOLD   ((int16_t)(8))		//	A2D counts

//-----------------------------------------------------------------------------
// CLOSED LOOP CONTROL
//  The PID parameters need to be tuned for each model
//-----------------------------------------------------------------------------
#if IS_PCB_LPC
    // Tuned 2018-01-25 Clark Dailey for 12LPC15
    #define INV_DFLT_PGAIN_Q16   Q16(0, .000)
    #define INV_DFLT_IGAIN_Q16   Q16(1, .100)
    #define INV_DFLT_DGAIN_Q16   Q16(0, .000)
    #define INV_SAFE_IGAIN_Q16   Q16(0, .400)
    #define INV_DFLT_ISUM_LIMIT  ((int16_t)(3000))
#else
    #define INV_DFLT_PGAIN_Q16   Q16(0, .000)
    #define INV_DFLT_IGAIN_Q16   Q16(3, .000)
    #define INV_DFLT_DGAIN_Q16   Q16(0, .000)
    #define INV_DFLT_ISUM_LIMIT  ((int16_t)(3000))
#endif


//-----------------------------------------------------------------------------
//
//  VAC PEAK VALUES AND SETPOINTS
//  HARDWARE/FIRMWARE DESIGN NOTES:
//
//  Design Goal: VAC Feedback is referenced to a 5V source (Vss)
//      4.50V corresponding to the nominal peak of 170V.
//      2.56V corresponding to 0V output.
//      0.62V corresponding to the nominal peak of -170V.
//      3.88V peak-to-peak swing
//
//  Theoretical A/D readings
//      5.12V --> 1024 counts
//      4.50V -->  900 counts
//      2.56V -->  512 counts
//      0.62V -->  124 counts
//      3.88V -->  776 counts (range)
//
//  Measured value when output is 340Vp-p
//      VAC = 3.490Vp-p
//
//  SETPOINT TUNING
//  The LP10 includes RMS compensation which is intended to maintain the RMS
//  value of the output when it is flat-topped. The normal PID algorithm
//  cannot compensate when the output is flat-topped. PID makes an adjustment
//  once per PWM cycle. RMS is essentially an average, and it too slow to
//  incorporate into the normal PID loop.
//
//  The RMS error is calculated once per AC cycle. The error is used to make
//  gradual adjustments to the setpoint value used by the PID loop.
//  The variable '_RmsVacSetpointOffset' is adjusted in the main loop at
//  the zero-crossing, and added to the setpoint in the PID control within the
//  PWM ISR.
//
//  'RMS_VAC_SETPOINT' is the setpoint for the RMS adjustment.
//
//  Adjustments to the output voltage requires that both setpoints be adjusted.
//  Adjustment is a multi-step process. The procedure below will provide best
//  results with minimal interraction:
//   1) Set '_RmsVacSetpointOffset' to zero. (This will defeat RMS adjustment.)
//   2) Adjust the value of 'RMS_VAC_SETPOINT' for the desired output voltage.
//   3) Determine the value of '_RmsVac' (Refer to the RMS control in the main loop.
//      I determined the value by printing it to the serial port and
//      observing the value on a terminal.)
//   4) Set 'RMS_VAC_SETPOINT' to that value.
//
//-----------------------------------------------------------------------------


// ------------
// calculations
// ------------

#define INV_RMS_VAC_SETPOINT_VOLTS  (123.0)  // Volts A/C RMS Setpoint per Chris Johnson
#define RMS_VAC_SETPOINT            (VAC_VOLTS_ADC(INV_RMS_VAC_SETPOINT_VOLTS*VAC_POT_ADJUST))
#define PEAK_VAC_SETPOINT           ((int16_t)(RMS_VAC_SETPOINT*SQRT_OF_TWO))

//-----------------------------------------------------------------------------
//                  I N V E R T E R    S T A T U S
//-----------------------------------------------------------------------------

#pragma pack(1)  // structure packing on byte alignement
typedef struct
{
    union
    {
        struct
        {
            unsigned all_flags              : 11;    // used to test or set all flags
            unsigned unused                 : 5;
        };
        struct
        {
			// --------------------------------------------------
            // WARNING! Do not change the order of these bits.
            //          They are an external interface used by
            //          CanBusDiag and Automated Tester and will
            //          break functionality if you change them.
            //          You can use the unused0 and unused1 bits.
			// --------------------------------------------------
            unsigned inv_active             : 1;    //  0x0001
            unsigned output_active          : 1;    //  0x0002
            unsigned load_sense_mode        : 1;    //  0x0004
            unsigned load_present           : 1;    //  0x0008
			
            unsigned supply_low_detected    : 1;    //  0x0010
            unsigned supply_high_detected   : 1;    //  0x0020
            unsigned overload_detected      : 1;    //  0x0040
            unsigned unused0       			: 1;    //  0x0080

			unsigned load_sense_enabled		: 1; 	//  0x0100
            unsigned ac_line_sensed         : 1;    //  0x0200  Future use: for hooking to grid
            unsigned ac_line_locked         : 1;    //  0x0400  At zero cross neg to pos
            unsigned unused1            	: 5;
        };
    };
    int16_t    load_sense_timer;  	//  start pinging when this timer expires
} INVERTER_STATUS_t;
#pragma pack()  // restore packing setting

//-----------------------------------------------------------------------------
//                  I N V E R T E R    E R R O R S
//-----------------------------------------------------------------------------
// These conditions typically mean that the inverter has shutdown
//-----------------------------------------------------------------------------

#pragma pack(1)  // structure packing on byte alignement
typedef struct
{
    union
    {
        struct
        {
            unsigned all_flags          	: 6;    // used to test or set all flags
            unsigned unused                 : 10;   //
        };
        struct
        {
			// --------------------------------------------------
            // WARNING! Do not change the order of these bits.
            //          They are an external interface used by
            //          CanBusDiag and Automated Tester and will
            //          break functionality if you change them.
            //          You can use the unused2 bits.
            //             0=ok, 1=error in effect
			// --------------------------------------------------
            unsigned supply_low_shutdown    : 1;    //	0X0001
            unsigned supply_high_shutdown   : 1;    //	0X0002
            unsigned feedback_problem       : 1;    //	0X0004
            unsigned feedback_reversed      : 1;    //	0x0008

            unsigned overload_shutdown      : 1;    //	0x0010
            unsigned output_short           : 1;    //	0x0020

            unsigned unused2                : 10;   //
        };
    };
} INVERTER_ERROR_t;
#pragma pack()  // restore packing setting


//-----------------------------------------------------------------------------
//                  I N V E R T E R    C O N F I G U R A T I O N
//-----------------------------------------------------------------------------

#pragma pack(1)  // structure packing on byte alignement
typedef struct
{
   	//	Inverter Load-Sense
	unsigned load_sense_enabled	: 1;	//  1=load sense is enabled (on-startup)
	unsigned unused 			: 15; 	
	
    uint16_t load_sense_delay;      //  delay before load-sense (pinging) [resolution = .5-sec] 
    uint16_t load_sense_interval;   //  delay between pings [resolution = .5-sec] 
    int16_t  load_sense_threshold;  //  Iload RMS threshold [ADC counts]

	//	Inverter Supply Detection
    int16_t supply_low_shutdown;
    int16_t supply_low_thres;
    int16_t supply_low_hyster;
    int16_t supply_low_recover;
    int16_t supply_high_recover;
    int16_t supply_high_thres;
    int16_t supply_high_shutdown;

    // Inverter Control Loop
    int16_t vac_setpoint;           //	adc value corresponding to peak VAC
    int32_t p_gain;                 //	proportional gain
    int32_t i_gain;                 //	integral gain
	int16_t i_limit;				//	integral sum limit (prevent wind-up)
    int32_t d_gain;                 //	differential gain

  #if IS_PCB_LPC
    // overload thresholds for LPC
    int16_t ovl_vds_threshold;
    int16_t ovl_imeas_threshold;
  #endif
} INVERTER_CONFIG_t;
#pragma pack()  // restore packing setting

	
//-----------------------------------------------------------------------------
//                    I N V E R T E R    I N F O
//-----------------------------------------------------------------------------

#pragma pack(1)  // structure packing on byte alignement
typedef struct
{
    INVERTER_CONFIG_t   config;     // inverter configuration
    INVERTER_STATUS_t   status;     // inverter status
    INVERTER_ERROR_t    error;
} INVERTER_t;
#pragma pack()  // restore packing setting


// -------------------
// access global data
// -------------------
extern INVERTER_t  Inv;


//-----------------------------------------------------------------------------
//      A B S T R A C T I O N   /  R E A D A B I L I T Y    M A C R O S
//-----------------------------------------------------------------------------

// inverter status flags
#define  IsInvActive()              (Inv.status.inv_active)
#define  IsInvOutputting()          (Inv.status.output_active)
#define  IsInvLowBattDetected()     (Inv.status.supply_low_detected)
#define  IsInvHighBattDetected()    (Inv.status.supply_high_detected)
#define  IsInvLoadSenseMode()       (Inv.status.load_sense_mode)
#define  IsInvLoadPresent()         (Inv.status.load_present)
#define  IsInvLoadSenseEn()         (Inv.status.load_sense_enabled)
#define  IsInvOverLoadDetected()    (Inv.status.overload_detected)

// set inverter status flags
#define  InvClearStatus()           { Inv.status.all_flags     = 0; }
#define  SetInvActive()             { Inv.status.inv_active    = 1; }
#define  SetInvNotActive()          { Inv.status.inv_active    = 0; }
#define  SetInvOutputting()         { Inv.status.output_active = 1; }
#define  SetInvNotOutputting()      { Inv.status.output_active = 0; }

// inverter error flags
#define  IsInvLowBattShutdown()     (Inv.error.supply_low_shutdown)
#define  IsInvHighBattShutdown()    (Inv.error.supply_high_shutdown)
#define  IsInvOverLoadShutdown()    (Inv.error.overload_shutdown)
#define  IsInvFeedbackProblem()     (Inv.error.feedback_problem)
#define  IsInvFeedbackReversed()    (Inv.error.feedback_reversed)
#define  IsInvOutputShorted()       (Inv.error.output_short)   // ### no one sets this
#define  HasInvAnyErrors()          (Inv.error.all_flags)

// set inverter error flags
#define  InvClearErrors()          { Inv.error.all_flags = 0; }

// inverter configuration
#define  InvCfgVBattHiShutDown()    (Inv.config.supply_high_shutdown)
#define  InvCfgVBattHiThresh()      (Inv.config.supply_high_thres) 
#define  InvCfgVBattHiRecover()     (Inv.config.supply_high_recover)
#define  InvCfgVBattLoRecover()     (Inv.config.supply_low_recover)
#define  InvCfgVBattLoHyster()      (Inv.config.supply_low_hyster)
#define  InvCfgVBattLoThresh()      (Inv.config.supply_low_thres)
#define  InvCfgVBattLoShutDown()    (Inv.config.supply_low_shutdown)

//-----------------------------------------------------------------------------
//                    A L L O C A T E    R O M    D A T A
//-----------------------------------------------------------------------------

#if !defined(ALLOCATE_SPACE_ROM_DEFAULTS)
	extern const INVERTER_CONFIG_t InvConfigDefaults;
#else
	const INVERTER_CONFIG_t InvConfigDefaults = 
	{	
		//	Inverter Load-Sense
		INV_DFLT_LOAD_SENSE_ENABLE,
		0,	// unused bits
		INV_DFLT_LOAD_SENSE_DELAY,
		INV_DFLT_LOAD_SENSE_INTERVAL,
		INV_DFLT_LOAD_SENSE_THRESHOLD,
		//	Inverter Supply Detection
		INV_DFLT_SUPPLY_LOW_SHUTDOWN,
		INV_DFLT_SUPPLY_LOW_THRESHOLD,
		INV_DFLT_SUPPLY_LOW_HYSTERESIS,
		INV_DFLT_SUPPLY_LOW_RECOVER,
		INV_DFLT_SUPPLY_HIGH_RECOVER,
		INV_DFLT_SUPPLY_HIGH_THRESHOLD,
		INV_DFLT_SUPPLY_HIGH_SHUTDOWN,
		//	Inverter Control Loop
		PEAK_VAC_SETPOINT,
		INV_DFLT_PGAIN_Q16,
		INV_DFLT_IGAIN_Q16,  
		INV_DFLT_ISUM_LIMIT,
		INV_DFLT_DGAIN_Q16,
	  #if IS_PCB_LPC
		//	LP/LPC-style Overload Detection
		OVL_VDS_THRESHOLD,
		OVL_IMEAS_THRESHOLD,
	  #endif
	};
#endif 	// ALLOCATE_SPACE_ROM_DEFAULTS


// -----------
// Prototypes
// -----------
extern int   inv_GetState(void);
extern void  inv_CheckOverload(void);
extern void  inv_ClearFeedbackErrors(void);
extern void  inv_ClearOverload(void);
extern void  inv_Start(void);
extern void  inv_StartBootCharge(void);
extern void  inv_StartDegauss(void);
extern void  inv_StartSSR(void);
extern void  inv_Stop(void);
extern void  inv_StopNow(void);
extern const char* inv_StateToStr(uint16_t invState);
extern const char* inv_PwmStateToStr(uint16_t pwmState);
extern void  TASK_inv_Driver(void);
extern void  inv_CheckForOverload(void);


#endif  //  __INVERTER_H

// <><><><><><><><><><><><><> inverter.h <><><><><><><><><><><><><><><><><><><><><><>
