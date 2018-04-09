// <><><><><><><><><><><><><> inverter.c <><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//  Copyright(C) 2018 - Sensata Technologies, Inc.  All rights reserved.
//
//  $Workfile: inverter.c $
//  $Author: A1021170 $
//  $Date: 2018-04-03 11:02:25-05:00 $
//  $Revision: 187 $ 
//
//  Inverter PWM Driver and State Machine
//
//-----------------------------------------------------------------------------

#include "options.h"    // must be first include
#include "analog.h"
#include "config.h"
#include "hw.h"
#include "fan_Ctrl.h"
#include "inv_check_supply.h"
#include "inverter.h"
#include "inverter_cmds.h"
#include "charger.h"
#include "pwm.h"
#include "sine_table.h"
#include "tasker.h"
#include "lpc_cfg.h"

#include <stdint.h>


// -----------
// global data
// -----------
INVERTER_t Inv; // inverter data

// ------------------------------------------
//  Conditional Compile Flags for DEBUGGING
// ------------------------------------------
// comment out flag if not used
#define _ENABLE_SHOW_DEBUG	                    1
#define _ENABLE_SHOW_INV_STATE_TRANSITIONS	    1
#define _ENABLE_SHOW_INV_PWM_STATE_TRANSITIONS	1
//#define _ENABLE_OPEN_LOOP                       1                       

// automatically remove all debugging for no logging option
#if OPTION_NO_CONDITIONAL_DBG
    #undef _ENABLE_SHOW_DEBUG
    #undef _ENABLE_SHOW_SETTINGS
    #undef _ENABLE_SHOW_INV_STATE_TRANSITIONS
    #undef _ENABLE_SHOW_INV_PWM_STATE_TRANSITIONS
#endif

#ifdef _ENABLE_OPEN_LOOP
  #warning    _ENABLE_OPEN_LOOP - Inverter running open-loop for DEBUG ONLY !!!
#endif


// ------------
// Prototyping
// ------------
static void _inv_CheckLoad(int8_t reset);
static void _inv_AdjustRmsSetpoint(void);


//-----------------------------------------------------------------------------
//  Inverter functionality encompasses any operation utilizing power from 
//  battery into the system.  This includes 'degauss' and 'boot charge' as well 
//  as the normal inverter (DC-to-AC) function. The inverter driver services 
//  these functions.
//-----------------------------------------------------------------------------

typedef enum
{
    INV_MODE_NORMAL,
    INV_MODE_DEGAUSS,
    INV_MODE_BOOT_CHARGE,
} INV_MODE_t;

static INV_MODE_t _InvMode = INV_MODE_NORMAL;

typedef enum
{
    INV_INIT            =  0,
    INV_STARTUP_WAIT    =  1,
    INV_NORMAL          =  2,
    INV_LOAD_SENSE_WAIT =  3,
    INV_LOAD_SENSE      =  4,
    INV_PING_WAIT1      =  5,
    INV_PING_WAIT2      =  6,
    INV_PINGING         =  7,
    INV_SHUTDOWN_WAIT   =  8,
    INV_SHUTDOWN        =  9,
    INV_RESTART_DELAY   = 10,
    NUM_INVERTER_STATES // This has to be last
} INV_STATE_t;

static INV_STATE_t _InvState = INV_INIT;

//-----------------------------------------------------------------------------
// get inverter machine state value

int inv_GetState()
{
    return ((int) _InvState);
}

//-----------------------------------------------------------------------------

typedef enum
{
    INV_ISR_REQ_NUL        = 0,
    INV_ISR_REQ_RESET      = 1,
    INV_ISR_REQ_SOFT_START = 2,
    INV_ISR_REQ_START      = 3, // No Soft-Start
    INV_ISR_REQ_PING       = 4,
    INV_ISR_REQ_SOFT_STOP  = 5,
    INV_ISR_REQ_STOP       = 6, // No Soft-Stop
} INV_ISR_REQUEST_t;

typedef enum
{
    PWM_STATE_IDLE      = 0,
    PWM_STATE_SOFTSTART = 1,
    PWM_STATE_NORMAL    = 2,
    PWM_STATE_PING      = 3,
    PWM_STATE_SOFTSTOP  = 4,
    NUM_INV_PWM_STATES // This has to be last
} PWM_STATE_t;

static PWM_STATE_t _PwmState = PWM_STATE_IDLE;

//-----------------------------------------------------------------------------
//  _VacOffset is used to adjust zero-crossing of AC output waveform to reduce 
//	transformer bias
//-----------------------------------------------------------------------------
static int16_t _VacOffset;

//-----------------------------------------------------------------------------
// scale the given value by nominal_vbatt/actual_vbatt
// replaces _E10Comp which was calculated constantly
// now only calculated as needed
INLINE int16_t ScaleByVBatt(uint16_t value) // ~ 1 usec
{
    int32_t value32;  // use 32 bit math because will overflow
    int32_t vbatt_avg = VBattCycleAvg();  // adc value
    if (0 == vbatt_avg) return(value);    // no average value yet so don't scale
    value32 = (int32_t)value*(int32_t)SUPPLY_NOMINAL/(vbatt_avg+2);   // adc/adc  (32-bit mult (280 nsec) 32-bit div (840 nsecs) )
    return((int16_t)value32);
}

//-----------------------------------------------------------------------------
//  _StopRequest is a signal from inv_Stop() to TASK_inv_Driver. 
//-----------------------------------------------------------------------------
static int8_t _StopRequest;

//-----------------------------------------------------------------------------
// OVERLOAD PROCESSING
//
//  Inv.config.ovl_vds_thres and Inv.config.ovl_imeas_thres are used 
//  within the ADC ISR to detect that an overload has occurred. When these 
//  thresholds are exceeded, then _OvlSkipNextPulse will be set to the number of 
//  succeeding PWM pulses to skip (to avoid current walk-up).
//
//	_OvlOccurred - set within inv_CheckForOverload to indicate that an overload
//		has occurred.  It is used/cleared by the function 
//		_inv_ProcessOverloadCounters.
//-----------------------------------------------------------------------------
int8_t _OvlOccurred = 0;

//-----------------------------------------------------------------------------
//  _OvlSkipNextPulse - When an overload is detected by the function 
//		'inv_CheckForOverload' _OvlSkipNextPulse is set to the number of 
//		successive PWM pulses to be skipped. It is used by (decremented/cleared) 
//		by the PWM ISR.
//-----------------------------------------------------------------------------
int16_t _OvlSkipNextPulse = 0;

//-----------------------------------------------------------------------------
//  _OvlVdsThresActive - a flag indicating that we should be using VDS to 
//  	limit output current. (Initially the IAC threshold is used to detect 
//  	overload.)
//-----------------------------------------------------------------------------
int8_t _OvlVdsThresActive = 0;

//  CLOSED LOOP CONTROL
static int16_t _IntegralSum = 0;
static int16_t _SavedIntegralSum = 0; // save when flat topping starts, and restore when exiting flat topping
static int32_t _RmsVacSetpointOffset = 0;

//-----------------------------------------------------------------------------
//  U.L. Requires that we detect problems with the feedback winding, these 
//  include:
//    - Open Sense Winding
//    - Shorted Sense Winding
//    - Reversed Sense Winding
//  We must also be able to handle a shorted output.  Some loads or testing 
//  will be or appear as a dead short on the output.  Therefore, we need to 
//  confirm the condition, before shutting down the system.
//  
//  We unconditionally and continually test for an Open or Shorted Sense 
//  winding by testing if the feedback reading is near zero for an extended
//  period of time.  (Zero == 512 +/- noise) for the A/D. The period of time 
//  is 5 AC cycles (5*(168*2)) = 1680.  See #defines below:
//-----------------------------------------------------------------------------
static int16_t _FeedbackErrorCount = 0;
#define FEEDBACK_ERROR_COUNT_LIMIT  (5*2*MAX_SINE)


//-----------------------------------------------------------------------------
//  _BootChargeCycles controls how long the Boot Charge will last on startup.
//-----------------------------------------------------------------------------
static int16_t _BootChargeCycles = 0;


//-----------------------------------------------------------------------------
//                     D E B U G G I N G      S T A R T
//-----------------------------------------------------------------------------

#ifndef _ENABLE_SHOW_DEBUG
    #define _inv_show_debug()
#else
    #ifndef WIN32
        #pragma message "ENABLED: _inv_show_debug - for debugging"
    #endif

static void _inv_show_debug(void)
{
    static uint16_t last_status = 0;
    static uint16_t last_error = 0;

    if (last_status != Inv.status.all_flags)
    {
        last_status = Inv.status.all_flags;
        LOG(SS_INV, SV_INFO, "Inv Status Flags: 0x%04X ", (int16_t) Inv.status.all_flags);
        if (IsInvActive())             LOG(SS_INV, SV_INFO, "    inv_active");
        if (IsInvOutputting())         LOG(SS_INV, SV_INFO, "    output_active");
        if (Inv.status.ac_line_locked) LOG(SS_INV, SV_INFO, "    ac_line_locked");
        if (IsInvLoadSenseEn())        LOG(SS_INV, SV_INFO, "    load_sense_enabled");
        if (IsInvLoadSenseMode())      LOG(SS_INV, SV_INFO, "    load_sense_mode");
        if (IsInvLoadPresent())        LOG(SS_INV, SV_INFO, "    load_present");
        if (IsInvLowBattDetected())    LOG(SS_INV, SV_INFO, "    supply_low_detected");
        if (IsInvHighBattDetected())   LOG(SS_INV, SV_INFO, "    supply_high_detected");
        if (IsInvOverLoadDetected())   LOG(SS_INV, SV_INFO, "    overload_detected");
    }
    if (last_error != Inv.error.all_flags)
    {
        last_error = Inv.error.all_flags;
        LOG(SS_INV, SV_INFO, "Inv Error Flags: 0x%04X ", (int16_t) Inv.error.all_flags);
        if (IsInvLowBattShutdown())  LOG(SS_INV, SV_INFO, "    supply_low_shutdown");
        if (IsInvHighBattShutdown()) LOG(SS_INV, SV_INFO, "    supply_high_shutdown");
        if (IsInvOverLoadShutdown()) LOG(SS_INV, SV_INFO, "    overload_shutdown");
        if (IsInvFeedbackProblem())  LOG(SS_INV, SV_INFO, "    feedback_problem");
        if (IsInvFeedbackReversed()) LOG(SS_INV, SV_INFO, "    feedback_reversed");
        if (IsInvOutputShorted())    LOG(SS_INV, SV_INFO, "    output_short");
    }
}
#endif // _ENABLE_SHOW_DEBUG


//------------------------------------------------------------------------------
#ifndef _ENABLE_SHOW_INV_STATE_TRANSITIONS
    #define _inv_show_state_transitions()
#else
    #ifndef WIN32
        #pragma message "ENABLED: _inv_show_state_transitions - for debugging"
    #endif

static int8_t _inv_show_state_transitions(void)
{
    static INV_STATE_t last_state = INV_INIT;
    static const char * inv_state_s[NUM_INVERTER_STATES] = {
        "INIT",
        "STARTUP_WAIT",
        "NORMAL",
        "LOAD_SENSE_WAIT",
        "LOAD_SENSE",
        "PING_WAIT1",
        "PING_WAIT2",
        "PINGING",
        "SHUTDOWN_WAIT",
        "SHUTDOWN",
        "RESTART_DELAY"
    };

    if (last_state != _InvState)
    {
        LOG(SS_INV, SV_INFO, "INV STATE: %s --> %s", inv_state_s[last_state], inv_state_s[_InvState]);
        last_state = _InvState;
        return (1);
    }
    return (0);
}
#endif // _ENABLE_SHOW_INV_STATE_TRANSITIONS

//------------------------------------------------------------------------------
#ifndef _ENABLE_SHOW_INV_PWM_STATE_TRANSITIONS
    #define _inv_show_pwm_state_transitions()
#else
    #ifndef WIN32
        #pragma message "ENABLED: _inv_show_pwm_state_transitions - for debugging"
    #endif

static void _inv_show_pwm_state_transitions(void)
{
    static PWM_STATE_t last_state = PWM_STATE_IDLE;
    static const char * inv_pwm_state[NUM_INV_PWM_STATES] = {
        "PWM_IDLE",
        "PWM_SOFTSTART",
        "PWM_NORMAL",
        "PWM_PING",
        "PWM_SOFTSTOP"
    };

    if (_PwmState != last_state)
    {
        LOG(SS_INV, SV_INFO, "INV PWM STATE: %s --> %s", inv_pwm_state[last_state], inv_pwm_state[_PwmState]);
        last_state = _PwmState;
    }
}
#endif // _ENABLE_SHOW_INV_STATE_TRANSITIONS

//-----------------------------------------------------------------------------
//                     D E B U G G I N G      E N D
//-----------------------------------------------------------------------------

// Terminate the output by overriding the drivers (STAB the transformer)
void StabXfmr()
{
    // Set POS, NEG and STAB outputs by overriding them.  Override control 
    // (POVD) bits are active-low: 
    //   POVD bit = 0 --> output determined by the POUT bit. 
    //   POVD bit = 1 --> output controlled by the PWM module.
    OVDCONbits.POUT1H = 0; // POUT1H (DRIVE_L1 output) override setting
    OVDCONbits.POVD1H = 0; // PWM1H controlled by POUT1H (override)
    OVDCONbits.POUT1L = 1; // POUT1L (DRIVE_L1 output) override setting
    OVDCONbits.POVD1L = 0; // PWM1L controlled by POUT1L (override)

    OVDCONbits.POUT2H = 0; // POUT2H (DRIVE_H2 output) override setting
    OVDCONbits.POVD2H = 0; // PWM2H controlled by POUT2H (override)
    OVDCONbits.POUT2L = 1; // POUT2L (DRIVE_L2 output) override setting
    OVDCONbits.POVD2L = 0; // PWM2L controlled by POUT2L (override)
}

//------------------------------------------------------------------------------
//  inv_CheckForOverload is called from the ADC(DMA) interrupt so that a PWM
//  output pulse may be truncated, if needed, as soon as possible.
//  

void inv_CheckForOverload(void)
{
#if defined(OPTION_IGNORE_OVERLOAD)
    _OvlOccurred = 0;
    _OvlSkipNextPulse = 0;
#else

    uint16_t imeas = 0;

    //  If we are not in inverter mode, then just return. 
    if (!IsInvActive() || (INV_MODE_BOOT_CHARGE == _InvMode))
    {
        return;
    }

    if (IsCycleAvgValid())
    {
        //  Rectify the Measured Current
        if (IMeasRaw() < IMeasCycleAvg())
            imeas = IMeasCycleAvg() - IMeasRaw();
        else
            imeas = IMeasRaw() - IMeasCycleAvg();
    }
    else
    {
        if (IMeasRaw() < IMEAS_ZERO_CROSS_A2D)
            imeas = IMEAS_ZERO_CROSS_A2D - IMeasRaw();
        else
            imeas = IMeasRaw() - IMEAS_ZERO_CROSS_A2D;
    }
    _OvlSkipNextPulse = 0;

    if (_OvlVdsThresActive)
    {
        //  If _OvlVdsThresActive == 1, then an overload is detected when 
        //      An.Status.Ovl >= Inv.config.ovl_vds_threshold (200%). 
        if (An.Status.Ovl >= Inv.config.ovl_vds_threshold)
        {
            StabXfmr();
            _OvlOccurred = 1;
            _OvlSkipNextPulse = OVL_VDS_SKIP_PULSE_COUNT;
        }
    }
    //  Else (_OvlVdsThresActive == 0), an overload is detected when 
    //      imeas >= Inv.config.ovl_imeas_threshold (108%). 
    else if (imeas >= Inv.config.ovl_imeas_threshold)
    {
        StabXfmr();
        _OvlOccurred = 1;
        _OvlSkipNextPulse = OVL_IMEAS_SKIP_PULSE_COUNT;
    }
#endif  //  OPTION_IGNORE_OVERLOAD  
}

//-----------------------------------------------------------------------------
//  _inv_ProcessOverloadCounters
//-----------------------------------------------------------------------------
//  Process Overload Detection Counters.  This needs to be checked once per 
//  zero-crossing. Timing is not critical, so it is done in the foreground.
//
//  This is implemented as a separate function to reduce clutter.
//
//	OVERVIEW:
//	The flag '_OvlVdsThresActive' determines what method will be used to detect 
//  an overload:
//  If _OvlVdsThresActive == 1, then an overload is detected when 
//      An.Status.Ovl >= Inv.config.ovl_vds_threshold (200%). 
//  Else (_OvlVdsThresActive == 0), an overload is detected when 
//      imeas >= Inv.config.ovl_imeas_threshold (108%). 
//		(imeas is the rectified, instantaneous value of iMeas).
//
//-----------------------------------------------------------------------------

void _inv_ProcessOverloadCounters(int8_t reset)
{
#if defined(OPTION_IGNORE_OVERLOAD)
    Inv.status.overload_detected = 0;
    Inv.error.overload_shutdown = 0;
    return;
#else
    
    //  Timing is based on 60Hz zero-crossings.
    #define OVL_VDS_RETRY_TIMEOUT   ((int16_t)(30 * 60))    //  30-sec  200% overload interval
    #define OVL_VDS_TIMEOUT         ((int16_t)(1.5 * 60))   //  1.5-sec 200% overload allowed duration
    #define OVL_IRMS_TIMEOUT        ((int16_t)(5 * 60))     //  5-sec overall overload duration
    #define OVL_RESET_TIMEOUT       ((int16_t)(0.4 * 60))   //  ~400 mSec
    
    static int16_t cycle_count = 0;     //  up counter
    static int16_t reset_count = 0;     //  up/down counter
    static int16_t state = 0;
    static int16_t vds_retry_count = 0; //  down counter
    static int16_t last_state = -1;

    if (reset)
    {
        LOG(SS_INV, SV_INFO, "*** _inv_ProcessOverloadCounters reset");
        state = 0;
        return;
    }

    //  DEBUG - Start
    if (last_state != state)
    {
        LOG(SS_INV, SV_INFO, "*** _inv_ProcessOverloadCounters: %d --> %d", last_state, state);
        last_state = state;
    }
    
    if((vds_retry_count > 0) && (vds_retry_count % 60 == 0))
    {
        LOG(SS_INV, SV_INFO, "vds_retry_count: %d ", (vds_retry_count / 60));
    }
    //  DEBUG - End
    
    if(--vds_retry_count <= 0)
    {
        vds_retry_count = 0;
    }

    switch (state)
    {
    case 0:
        //-------------------------------------------------------------------
        //  Monitor for initial overload - allow 200% overload (based on VDS)
        //  The flag '_OvlVdsThresActive' configures the function 
        //  'inv_CheckForOverload' to truncate the PWM when VDS exceeds the 
        //  threshold configured for 200% overload.
        //-------------------------------------------------------------------
        Inv.status.overload_detected = 0;
        cycle_count = 0;
        
        if (IMeasRMS() >= (OVL_RMS_THRESHOLD))
        {
//            LOG(SS_INV, SV_INFO, "_inv_ProcessOverloadCounters IMeas: %d ", IMeasRMS());
            if(0 == vds_retry_count)
            {
                //  If the timer is NOT running (zero) then allow VDS (200%)
                //  else if the timer is running then remain at default (108%)
                _OvlVdsThresActive = 1;
            }
            state++;
        }
        break;

    case 1:
        //  Limit at 200% for one-second.  (Timeout adjustable in #define above.)
        if (++cycle_count >= (OVL_VDS_TIMEOUT))
        {
//            LOG(SS_INV, SV_INFO, "_inv_ProcessOverloadCounters IMeas: %d ", IMeasRMS());
            // If we have ANY 200% overloads, then start the retry timer.
            if(0 == vds_retry_count)
            {
                vds_retry_count = OVL_VDS_RETRY_TIMEOUT;
            }
            _OvlVdsThresActive = 0; //  switch to Iac Limiting  
            state++;
        }
        break;

    case 2:
        if ((IMeasRMS() >= (OVL_RMS_THRESHOLD)) || _OvlOccurred)
        {
            Inv.status.overload_detected = 1;
            reset_count = OVL_RESET_TIMEOUT;
            //  Allow 108% (based upon IMeas) for an additional 4-seconds 
            //  NOTE: 'count' has already accumulated some time.
            if (++cycle_count >= OVL_IRMS_TIMEOUT)
            {
                cycle_count = OVL_IRMS_TIMEOUT;     //  limit 
                Inv.error.overload_shutdown = 1;
                state++;
            }
        }
        else
        {
            //  If we are below the overload threshold, then count-down and 
            //  reset this state machine.
            --cycle_count;
            --reset_count;
            if ((cycle_count <= 0) || (reset_count <= 0))
            {
//                LOG(SS_INV, SV_INFO, "_inv_ProcessOverloadCounters IMeas: %d ", IMeasRMS());
                cycle_count = 0;
                reset_count = 0;
                Inv.status.overload_detected = 0;
                state = 0;
            }
        }
        break;

    case 3: //  Overload shutdown
        //  No chance of restarting after a shutdown.. unless we are reset
        break;

    default:
        state = 0;
        break;
    }
    _OvlOccurred = 0; //	Just in case

#endif  //  OPTION_IGNORE_OVERLOAD  
}

//-----------------------------------------------------------------------------
//  _inv_PwmIsr() 
//-----------------------------------------------------------------------------
//
//  PWM PULSE LIMITS
//
//  At 40MIPS and 20KHz PWM clock, we have 12.5nSec resolution.
//  The Duty Cycle calculations below are expressed as the quantity of 
//  12.5nSec increments. 
//  
//  We want a minimum pulse width of 1uSec for FETs.  That means that the 
//  number of cycles corresponding minimum Duty Cycle is 1/.0125 = 80.
//  
//  The Min Duty Cycle affects the pulse applied to the High-side driver.
//  The Max Duty Cycle affects the pulse applied to the Low-side driver.
//  Design Goals:  Minimum pulse width of 1uSec, Dead-Time = 0.5uSec
//  
//  Since Dead-Time is applied symmetrically to the output pulse, the 
//  calculation is then Min Duty + Dead-Time.
//-----------------------------------------------------------------------------
//
//  The calculated Min Duty Cycle is then: 1.5/.0125 = 120.
#define MIN_DUTY    (120)

//  We will use one-half the Min Duty Cycle as the Min Threshold 
#define MIN_THRES   (MIN_DUTY/2)

//  The Max Duty Cycle is (((FCY/FPWM) - 1) * 2)
//      = (((29491200/20158) - 1) * 2) = 2924
#define MAX_DUTY    (((FCY/FPWM) - 1) * 2)  

//  The Max Threshold is the Max Duty Cycle less the Min Duty Cycle.
#define MAX_THRES   (MAX_DUTY - MIN_DUTY)


//-----------------------------------------------------------------------------
//  DYNAMIC RE-BALANCING:
//  Objective: Reduce potential for transformer saturation.
//  Implementation: Sum the duty cycles during one-half of the AC cycle, then 
//  subtract from that sum during the other half. At the zero-crossing, use
//  the result to increment or decrement the VAC Offset value. 
//-----------------------------------------------------------------------------

static DWORD_t duty_cycle_sum = { {0} };

//  Limits applied to VAC Offset
#define VAC_OFFSET_MIN  (VAC_ZERO_CROSS_A2D - 64)   //-64
#define VAC_OFFSET_MAX  (VAC_ZERO_CROSS_A2D + 64)   //+64

//  Thresholds applied to duty_cycle_sum
#define DUTY_CYCLE_SUM_POS_THRES    (500)
#define DUTY_CYCLE_SUM_NEG_THRES    (-500)

// only allow flat topping after this may a/c cycles to prevent oscillations
#define ALLOW_FLAT_TOP_AFTER_AC_CYLCES    (30) // 30 * *16.6 msec = 498 msec


// ---------------------------------------------------------------------------------------------------------
// Transformer Saturation Compensation
// ---------------------------------------------------------------------------------------------------------
// handle transformer saturation from line mode to inverter mode
//   The saturation algorithm works on every a/c cycle for a given number of cycles (30) (same as flat-topping)
//   The detection algorithm only operates when transitioning from line mode to inverter mode
//
//   Saturation Detection:
//      monitoring battery voltage and comparing 1st half cycle against 2nd half cycle works to detect
//
//   Saturation Compensation
//      reducing the saturated half-cycle by a percent
//      the percent reduction is scaled by the vbatt voltage difference
// ---------------------------------------------------------------------------------------------------------

// -----------------------
// saturation calibration
// -----------------------
//#define ENABLE_SATURATION_COMP     (1)  // comment out to disable saturation compensation
#define SATURATION_THRESHOLD   (VBATT_VOLTS_ADC(0.15)-VBATT_VOLTS_ADC(0)) // 1st vbatt vs 2nd half vbatt average (A2D counts)
#define SATURATION_DUTY_GAIN    ((int16_t)(500))   // 10 x %duty_cycle / vbatt_volts_diff;  10=1.0% 100=10.0%)
#define VBATT_START_PHASE       ((3*MAX_SINE)/4)   // (3/4 into half-cycle) where to start vbatt summing for average

// reduce the i_gain on the PID loop during this time to INV_SAFE_IGAIN_Q16; prevents oscillation if xfmr is saturated
#define ENABLE_SAFE_IGAIN           (1)  // comment out to not use safe i_gain 
#define IGAIN_TRANSITION_AC_CYCLES  (8)  // A/C cycle counts (16.667 msecs) 5 no work, 7 works, 8 works, 10 works

// debugging
//#define ENABLE_SATURATION_DEBUG      (1)  // uncomment to enable debugging; shows sat comp cycles via pin RB6 for scope monitoring

// ----------
// variables
// ----------
// batt voltage summation for 1st and 2nd half cycles
uint8_t  is_xfrm_saturated = 0;  // 0=no, 1=transformer is saturated based upon vbatt differential (calculated on each a/c cycle)
uint8_t  xfrm_2half_sat    = 0;  // 0=1st half saturated, 1=2nd half saturated; only valid if:  is_xfrm_saturated != 0
uint32_t vbatt_sum_1half   = 0;  // battery voltage sum of a/d counts first  half cycle
uint32_t vbatt_sum_2half   = 0;  // battery voltage sum of a/d counts second half cycle
uint16_t vbatt_diff_a2d    = 0;  // vbatt volt difference 1st vs 2nd cycle (+ 1st half > 2nd half)
uint16_t saturation_adj    = 0;  // percent x 10  adjustment decrease if saturated  (0-1000: 10=1% 100=10%)

// constants; don't change
// duty cycle is reduced by a percentage scaled to the vbatt voltage differential of 1st and 2nd half cycles
#define VBATT_1VOLT_A2D    (VBATT_VOLTS_ADC(1.0)-VBATT_VOLTS_ADC(0)) // one delta vbatt volt (A2D counts)
#define VBAT_AVG_SAMPLES   (MAX_SINE-VBATT_START_PHASE)   // number of vbatt samples per half sub cycle

//-----------------------------------------------------------------------------
// inverter PWM interrupt service routine runs at 23 khz

static void _inv_PwmIsr(INV_ISR_REQUEST_t request)
{
    static PWM_STATE_t pwm_state = PWM_STATE_IDLE;
    static INV_ISR_REQUEST_t _my_request = INV_ISR_REQ_NUL;

    static DWORD_t temp32;
    static int16_t sine_table_index = 0;
    static VECTOR_CYCLE phase_vector = VECTOR1;

    static int16_t curr_error = 0;
    static int16_t prev_error = 0;
    static int32_t multiplier = 0;
    static int16_t p_adj = 0;
    static int16_t i_adj = 0;
    static int16_t d_adj = 0;

    static int16_t half_cycle_count = 0;

    //-----------------------------------------------------------------------------
    //	The system has about a 250 nSec delay between the stimulus applied to 
    //	the transformer primary and the feedback signal. The elements below 
    //	implement a delay for error calculations. The amount of delay may be 
    //	adjusted by the initial value of 'vac_sp_ay_tail'.
    //-----------------------------------------------------------------------------
#define VAC_SP_AY_SIZE  (0x08)
#define VAC_SP_AY_MASK  (0x07)
    static int16_t vac_setpoint[VAC_SP_AY_SIZE];
    //    static int16_t  vac_sp_ay_tail = 5;
    static int16_t vac_sp_ay_tail = 0;
    static int16_t vac_sp_ay_head = 0;
    static int16_t flattop_end_index = 0;
    static int16_t flattop_allow_counter = 0; // dont allow flat topping until threshold a/c cylces
    static int16_t last_duty = 0;

    // handle transformer saturation during line mode to transformer mode
    // reduce the i_gain value on the PID loop during initial normal mode
    static int16_t igain_counter = 0;

    //  20 counts = 1 mSec 
#define SOFT_START_MAX_DUTY (MAX_DUTY * .853)
#define SOFT_START_CYCLES   (10)
#define SOFT_START_INC      (SOFT_START_MAX_DUTY/SOFT_START_CYCLES)

    static int16_t soft_start_cycles = SOFT_START_CYCLES;
    static int16_t soft_start_inc = SOFT_START_INC;

#define PING_MAX_DUTY       (((MAX_DUTY * .853)*2)/3)
#define PING_HALF_CYCLES    (2)
#define PING_INC            (PING_MAX_DUTY/PING_HALF_CYCLES)

    static int16_t ping_cycles = (2 * PING_HALF_CYCLES);
    static int16_t ping_inc = PING_INC;

    DWORD_t curr_duty = { {0L} };

    //  2015-08-06 - AC output measurements with a DVM
    //    const int32_t MAX_DUTY_Q16 = (MAX_DUTY * .800);   // 119.68  Vrms
    //    const int32_t MAX_DUTY_Q16 = (MAX_DUTY * .810);   //  121.13 Vrms
    //    const int32_t MAX_DUTY_Q16 = (MAX_DUTY * .815);   //  121.90 Vrms
    //    const int32_t MAX_DUTY_Q16 = (MAX_DUTY * .8155);   //  121.96 Vrms
    const int32_t MAX_DUTY_Q16 = (MAX_DUTY * .816); //  122.02 Vrms
    //    const int32_t MAX_DUTY_Q16 = (MAX_DUTY * .820);   //  122.60 Vrms
    //    const int32_t MAX_DUTY_Q16 = (MAX_DUTY * .853);   //  127.29 Vrms
    int32_t sine_table_val;
    int32_t i_gain;

    switch (request)
    {
    default:
        //  unrecognized command - do nothing!
    case INV_ISR_REQ_NUL:
        break;

    case INV_ISR_REQ_RESET:
    case INV_ISR_REQ_STOP:
#ifdef INV_DEBUG_WAVEFORM_CAPTURE
        debug_arm = 0;
        debug_trigger = 0;
        debug_cycle_count = 0;
        debug_array_ix = 0;
#endif  //  INV_DEBUG_WAVEFORM_CAPTURE

        //  Initialize variables used by the ISR
        sine_table_index = 0;
        phase_vector = VECTOR1;

        half_cycle_count = 0;
        soft_start_cycles = SOFT_START_CYCLES;
        soft_start_inc = SOFT_START_INC;

        _inv_ProcessOverloadCounters(1);

        pwm_state = PWM_STATE_IDLE;
        return;

    case INV_ISR_REQ_PING:
    case INV_ISR_REQ_SOFT_START:
    case INV_ISR_REQ_SOFT_STOP:
        _my_request = request;
        return;
    }

    if ((_OvlSkipNextPulse <= 0) && (PWM_STATE_IDLE != pwm_state))
    {
        // Outputs were overridden when an overload occured.
        // Restore the output drives, except if we are IDLE.
        OVDCON = 0x3F00; //  Normal Operation
    }

    sine_table_val = (int32_t) _SineTableQ16[sine_table_index];

    switch (pwm_state)
    {
    case PWM_STATE_IDLE:
        //  Idle state: wait here until commanded to start
        SetInvNotOutputting();

        //  In an idle state, we are sitting with the low-drives on. This 
        //  consumes about 60mA. To conserve power, turn off all drives while 
        //  in the IDLE state, and re-enable when we exit.

        OVDCON = 0x3000; // Set all PWM outputs low by overriding them.

        //  Reset the operating variables so that we can restart immediately
        flattop_end_index = 0;
        flattop_allow_counter = 0;
        igain_counter  = 0;
        vac_sp_ay_tail = 0;
        vac_sp_ay_head = 0;
        _IntegralSum = 0;
        temp32.dword = 0;
        multiplier   = 0;
        last_duty    = 0;
        curr_error   = 0;
        prev_error   = 0;
        p_adj = 0;
        i_adj = 0;
        d_adj = 0;
        _RmsVacSetpointOffset = 0;

        // transformer saturation compensation
        is_xfrm_saturated = 0;
        vbatt_sum_1half   = 0;
        vbatt_sum_2half   = 0;
        vbatt_diff_a2d    = 0;
        saturation_adj    = 0;

        //  The input signal should be zero VAC, use this as an offset that
        //  to can be applied to future calculations.
        _VacOffset = IsCycleAvgValid() ? VacCycleAvg() : VAC_ZERO_CROSS_A2D;

        // check for requests
        if (INV_ISR_REQ_SOFT_STOP == _my_request)
        {
            _my_request = INV_ISR_REQ_NUL;
        }
        else if (INV_ISR_REQ_SOFT_START == _my_request)
        {
          #ifdef _ENABLE_OPEN_LOOP
            soft_start_inc = SOFT_START_INC;
          #else
            soft_start_inc = ScaleByVBatt(SOFT_START_INC);
          #endif    //  _ENABLE_OPEN_LOOP
          
            // init values
            multiplier = (int32_t)soft_start_inc;
            sine_table_index = 0;
            sine_table_val = (int32_t)_SineTableQ16[sine_table_index];
            half_cycle_count = 0;
            phase_vector     = VECTOR1;

            // transition to next state
            _my_request = INV_ISR_REQ_NUL;
            pwm_state = PWM_STATE_SOFTSTART;
        }
        else if (INV_ISR_REQ_PING == _my_request)
        {
            // init values
            ping_inc = ScaleByVBatt(PING_INC);
            multiplier = (int32_t) ping_inc;
            sine_table_index = 0;
            sine_table_val = (int32_t)_SineTableQ16[sine_table_index];
            half_cycle_count = 0;
            phase_vector     = VECTOR1;

            // transition to next state
            _my_request = INV_ISR_REQ_NUL;
            pwm_state = PWM_STATE_PING;
        }
        break;

    case PWM_STATE_PING:
        SetInvOutputting();
        //  We are (re-)using half_cycle_count as a flag and counter, because
        //  it is available, not necessarily to monitor half-cycles.

        // wait for the zero crossing
        if (0 == sine_table_index)
        {
            // enable PWM drive on first time; over-load may modify this later
            if (0 == half_cycle_count) OVDCON = 0x3F00; // FETS now driven by PWM

            half_cycle_count++;
            if (half_cycle_count < (ping_cycles >> 1))
            {
                multiplier += (int32_t) ping_inc;
            }
            else if ((half_cycle_count > (ping_cycles >> 1)) &&
                    (half_cycle_count < ping_cycles))
            {
                multiplier -= (int32_t) ping_inc;
            }
            else if (half_cycle_count == ping_cycles)
            {
                multiplier = 0;
                pwm_state = PWM_STATE_IDLE;
            }
        }
        break;

    case PWM_STATE_SOFTSTART:
        SetInvOutputting();
        //  Increase the amplitude of the output every half-cycle, at the 
        //  zero-crossing.
        if (0 == sine_table_index)
        {
            // enable PWM drive on first time; over-load may modify this later
            if (0 == half_cycle_count) OVDCON = 0x3F00; // FETS now driven by PWM

            if (++half_cycle_count < soft_start_cycles)
            {
                multiplier += (int32_t) soft_start_inc;
            }
            else
            {
                multiplier = ScaleByVBatt(SOFT_START_MAX_DUTY);
                _IntegralSum = 0;
                _VacOffset = VAC_ZERO_CROSS_A2D;
                pwm_state = PWM_STATE_NORMAL;
            }
        }

        //  detect REVERSED SENSE WINDING at peaks
        //  VECTOR1 = Positive half-cycle
        //  VECTOR2 = Negative half-cycle
        if (((MAX_SINE / 2) == sine_table_index) && (half_cycle_count > 2) &&
                (((VECTOR1 == phase_vector) && (VacRaw() < (_VacOffset - soft_start_inc))) ||
                ((VECTOR2 == phase_vector ) && (VacRaw() > (_VacOffset + soft_start_inc)))))
        {
            Inv.error.feedback_reversed = 1;
            pwm_state = PWM_STATE_SOFTSTOP;
        }
        break;

    case PWM_STATE_NORMAL:
        SetInvOutputting();
        // If an overload occurred, the outputs have been overridden,
        // we are skipping this pulse.  
        if (_OvlSkipNextPulse > 0)
        {
            _OvlSkipNextPulse--;

            //  Reset historical compensation values
            _RmsVacSetpointOffset = 0;
            break; // done here
        }

        //  PID Control Calculations

        //  SETPOINT
        //  (Integer) Setpoint value will be in Most-Significant Word

        //  TODO:  RMS Setpoint Compensation needs to be tweaked for LPC.       
        //        temp32.dword = sine_table_val * (int32_t)Inv.config.vac_setpoint;
        temp32.dword = sine_table_val * (int32_t) (Inv.config.vac_setpoint + _RmsVacSetpointOffset);

        vac_setpoint[vac_sp_ay_head++] = temp32.msw;
        vac_sp_ay_head &= VAC_SP_AY_MASK;

        //  ERROR
        //  Checking phase_vector here will be off by one cycle at the zero 
        //  crossing.  However, we can use a sine-weighted error calculation
        //  to negate that effect.
        if (VECTOR1 == phase_vector)
        {
            curr_error = vac_setpoint[vac_sp_ay_tail] - (VacRaw() - _VacOffset);
        }
        else
        {
            curr_error = vac_setpoint[vac_sp_ay_tail] - (_VacOffset - VacRaw());
        }

        vac_sp_ay_tail++;
        vac_sp_ay_tail &= VAC_SP_AY_MASK;

        //  We could scale the error here, but we should be able to produce the
        //  same results using the PID gain values.

        //  Apply sine-weighting to error - errors near the zero-crossing have
        //  less-affect than those around the peaks.
        temp32.dword = (int32_t) curr_error * sine_table_val;
        curr_error = (int16_t) temp32.msw;

        // PROPORTIONAL
        temp32.dword = (int32_t) curr_error * Inv.config.p_gain;
        p_adj = temp32.msw;

        // INTEGRAL
        if (flattop_end_index == 0)
        {
            //	When 'flat-topped' the duty-cycle is 100%, so exclude errors 
            //	from the _IntegralSum, else we might overflow/wind-up.
            _IntegralSum += curr_error;
        }

        //  clamp
        if (_IntegralSum <= -Inv.config.i_limit)
        {
            _IntegralSum = -Inv.config.i_limit;
        }
        if (_IntegralSum >= Inv.config.i_limit)
        {
            _IntegralSum = Inv.config.i_limit;
        }

        // use lower safe i_gain to prevent oscillations if xfmr is saturated
        i_gain = Inv.config.i_gain;
     #ifdef ENABLE_SAFE_IGAIN
        if (chgr_hasRun && igain_counter < IGAIN_TRANSITION_AC_CYCLES)
            i_gain = INV_SAFE_IGAIN_Q16;
     #endif
        temp32.dword = ((int32_t) _IntegralSum) * i_gain;
        i_adj = temp32.msw;

        //  DERIVATIVE
        temp32.dword = ((curr_error - prev_error) * Inv.config.d_gain);
        d_adj = temp32.msw;

        prev_error = curr_error;

      #ifdef _ENABLE_OPEN_LOOP
        multiplier = MAX_DUTY_Q16;  //  Open-loop for DEBUG
      #else
        multiplier = MAX_DUTY_Q16 + p_adj + i_adj + d_adj; // Closed loop
      #endif
      
        if ((0 == sine_table_index) && (VECTOR1 == phase_vector))
        {
            // start of new full a/c cycle
            if (flattop_allow_counter <= ALLOW_FLAT_TOP_AFTER_AC_CYLCES)
                flattop_allow_counter++; // one more a/c cycle

            if (igain_counter <= IGAIN_TRANSITION_AC_CYCLES)
                igain_counter++; // one more a/c cycle

            //  accept commands only when starting a new cycle
            if (INV_ISR_REQ_SOFT_STOP == _my_request)
            {
                _my_request = INV_ISR_REQ_NUL;
                half_cycle_count = 0;
                multiplier     = ScaleByVBatt(MAX_DUTY_Q16);
                soft_start_inc = ScaleByVBatt(SOFT_START_INC);
                pwm_state = PWM_STATE_SOFTSTOP;
                break;
            }
        }
        //  Test for an OPEN or SHORTED SENSE WINDING
        #define FEEDBACK_ERROR_NOISE_BAND   (20)    

        if ((VacRaw() > (_VacOffset - FEEDBACK_ERROR_NOISE_BAND)) &&
            (VacRaw() < (_VacOffset + FEEDBACK_ERROR_NOISE_BAND)))
        {
            ++_FeedbackErrorCount;
        }
        else if (_FeedbackErrorCount > 0)
        {
            --_FeedbackErrorCount;
        }
        break;

    case PWM_STATE_SOFTSTOP:
        SetInvOutputting();
        if (0 == sine_table_index)
        {
            if (++half_cycle_count >= soft_start_cycles)
            {
                multiplier = 0;
                pwm_state = PWM_STATE_IDLE;
            }
            else
            {
                multiplier -= (int32_t) soft_start_inc;
                if (multiplier <= 0)
                {
                    multiplier = 0;
                }
            }
        }
        break;

    default: //  error state? 
        break;
    }

    curr_duty.dword = sine_table_val * multiplier;

    //  Check for duty-cycle limits (deadband)
    if (curr_duty.msw < MIN_THRES)
    {
        curr_duty.msw = 0;
    }
    else if (curr_duty.msw < MIN_DUTY)
    {
        curr_duty.msw = MIN_DUTY;
    }
    else if ((curr_duty.msw > MAX_THRES) && (flattop_allow_counter > ALLOW_FLAT_TOP_AFTER_AC_CYLCES))
    {
        //------------------------------------------------------------------------
        //  MAX_THRES is the maximum allowed duty cycle, less than 100%.
        //  (At 100% the FETs are on continuously.) Consider that at MAX_THRES
        //  duty-cycle the drives will be pulsed OFF for a minimal time.  If 
        //  that minimal time is too short, then the FETs may operate in the
        //  linear region resulting in damage. So, when above MAX_THRES assure 
        //  that the FETs stay on to reduce potential damage.
        //
        //  It is expected this will occur just before the peak, and result in 
        //  flat-topping. 'flattop_end_index' is calculated from the current value 
        //  of 'sine_table_index' as the point where we resume normal operation. 
        //  It is intended that flat-topping is symmetric about the peak.
        //------------------------------------------------------------------------

        //	'flattop_end_index' is also used as a flag - if non-zero then we are 
        //	flat-topped.
        flattop_end_index = (MAX_SINE - sine_table_index);
        _SavedIntegralSum = _IntegralSum;   // save to allow restoring when exiting flat topping
    }

    if (flattop_allow_counter > ALLOW_FLAT_TOP_AFTER_AC_CYLCES)
    {
        chgr_hasRun = 0; // clear charger run flag
        if (flattop_end_index > 0)
        {
            //	When 'flat-topped' the duty-cycle should be 100%. (Adding MIN_DUTY 
            //	assures that we are just slightly greater than 100% - no gaps). 

            _IntegralSum = 0;  // disable integral term while flat topping
            curr_duty.msw = MAX_DUTY + MIN_DUTY;
            if (sine_table_index > flattop_end_index)
            {
                _IntegralSum = _SavedIntegralSum;  // restore value before entering flat topping
                flattop_end_index = 0;
            }
        }
        //  This is intended to prevent oscillation on the falling side of the sine-
        //	wave.  From 90(270) to 180(360) degrees. If we are in the falling side,
        //	and we attempt to increase the output, use the prior value instead.
        else if ((sine_table_index > (MAX_SINE / 2)) && (curr_duty.msw > last_duty))
        {
            curr_duty.msw = last_duty;
        }
    }

    //	'last_duty' is saved here for minimizing oscillation. See previous note.
    last_duty = curr_duty.msw;

    switch (phase_vector)
    {
    case VECTOR1:
        //  Drives energized for negative polarity
        //  The duty cycle written takes affect on the next PWM clock.
        if (is_xfrm_saturated && !xfrm_2half_sat)
        {
            uint16_t delta_duty = (int16_t)(((uint32_t)curr_duty.msw*(uint32_t)saturation_adj)/1000);
            int16_t  new_duty   = curr_duty.msw - delta_duty;  // reduce duty cycle
            if (new_duty < MIN_DUTY) new_duty = MIN_DUTY;
         // if ((sine_table_index&0x3F)==1) SLOG(SS_SYS, SV_DBG, "V1SAT", curr_duty.msw, new_duty, saturation_adj, VBATT_1VOLT_A2D);
          #ifdef ENABLE_SATURATION_COMP
            curr_duty.msw = new_duty;
          #endif
        }
        PDC1 = curr_duty.msw;
        PDC2 = 0;

        if (0 == sine_table_index)
        {
            //  Re-balance VAC Offset once per cycle
            if ((duty_cycle_sum.msw == 0) &&
                (duty_cycle_sum.lsw > (int16_t) DUTY_CYCLE_SUM_POS_THRES))
            {
                if (--_VacOffset < VAC_OFFSET_MIN)
                      _VacOffset = VAC_OFFSET_MIN;
            }
            else if (duty_cycle_sum.msw < 0)
            {
                //--------------------------------------------------------
                //  We need to be able to handle 32-bit negative values.
                //  Note that -1 is 0xFFFFFFFF. 

                //  The Microchip compiler does not support 32-bit integer
                //  comparisons well (### Very odd. Where do you get this factoid?).
                //  To compare the 32-bit sum against the 
                //  threshold add the threshold to the sum. If the sum
                //  is still negative, then the 32-bit sum was below the 
                //  threshold value.
                //--------------------------------------------------------

                duty_cycle_sum.dword -= (int32_t) DUTY_CYCLE_SUM_NEG_THRES;
                if (duty_cycle_sum.msw < 0)
                {
                    if (++_VacOffset > VAC_OFFSET_MAX)
                          _VacOffset = VAC_OFFSET_MAX;
                }
            }
            //  Re-initialize duty_cycle_sum
            duty_cycle_sum.msw = 0;
            duty_cycle_sum.lsw = curr_duty.msw;
        }
        else
        {
            // battery voltage detects transformer saturation
            if (sine_table_index >= VBATT_START_PHASE)
            {
                if (sine_table_index == VBATT_START_PHASE)
                    vbatt_sum_1half  = (uint32_t)An.Status.VBatt.raw_val; // initial
                else
                    vbatt_sum_1half += (uint32_t)An.Status.VBatt.raw_val; // summing
            }
            duty_cycle_sum.dword += curr_duty.msw;
        }

        if (++sine_table_index > (MAX_SINE - 1))
        {
            //  setup for next phase:
            sine_table_index = 0;
            phase_vector = VECTOR2;
            _IntegralSum = 0;
        }
        break;

    case VECTOR2:
        //  Drives energized for positive polarity
        if (is_xfrm_saturated && xfrm_2half_sat)
        {
            uint16_t delta_duty = (int16_t)(((uint32_t)curr_duty.msw*(uint32_t)saturation_adj)/1000);
            int16_t  new_duty   = curr_duty.msw - delta_duty;
            if (new_duty < MIN_DUTY) new_duty = MIN_DUTY;
         // if ((sine_table_index&0x3F)==1) SLOG(SS_SYS, SV_DBG, "V2SAT", curr_duty.msw, new_duty, saturation_adj, VBATT_1VOLT_A2D);
          #ifdef ENABLE_SATURATION_COMP
            curr_duty.msw = new_duty;
          #endif
        }
        PDC1 = 0;
        PDC2 = curr_duty.msw;
        duty_cycle_sum.dword -= curr_duty.msw;

        // battery voltage detects transformer saturation
        if (sine_table_index >= VBATT_START_PHASE)
        {
            if (sine_table_index == VBATT_START_PHASE)
                vbatt_sum_2half  = (uint32_t)An.Status.VBatt.raw_val; // initial
            else
                vbatt_sum_2half += (uint32_t)An.Status.VBatt.raw_val; // summing
        }

        if (++sine_table_index > (MAX_SINE - 1))
        {
            // only check for transformer saturation at begining of normal mode and charger has run
            if (chgr_hasRun && (PWM_STATE_NORMAL == pwm_state))
            {
                // check if transformer is saturated
                uint32_t diff;
                if (vbatt_sum_1half > vbatt_sum_2half)
                {
                    diff = (vbatt_sum_1half - vbatt_sum_2half)/(uint32_t)(VBAT_AVG_SAMPLES);
                    xfrm_2half_sat = 1;  // 2nd half saturated
                }
                else
                {
                    diff = (vbatt_sum_2half - vbatt_sum_1half)/(uint32_t)(VBAT_AVG_SAMPLES);
                    xfrm_2half_sat = 0;  // 1st half saturated
                }
                vbatt_diff_a2d = (uint16_t)diff;

                if (vbatt_diff_a2d > SATURATION_THRESHOLD)
                {
                    is_xfrm_saturated = 1;
                    saturation_adj   = (uint16_t)(((uint32_t)SATURATION_DUTY_GAIN * (uint32_t)vbatt_diff_a2d)/(uint32_t)VBATT_1VOLT_A2D);
                  #ifdef ENABLE_SATURATION_DEBUG
                    RB6_SET(1);
                  #endif
                  // SLOG(SS_SYS, SV_DBG, "SAT", vbatt_diff_a2d, saturation_adj, xfrm_2half_sat, VBATT_1VOLT_A2D);
                }
                else
                {
                    is_xfrm_saturated = 0;
                    saturation_adj   = 0;
                  #ifdef ENABLE_SATURATION_DEBUG
                    RB6_SET(0);
                  #endif
                }
            }
            else
            {
                // Not Normal Mode: Soft Start, Soft Stop, etc
                vbatt_diff_a2d    = 0;
                is_xfrm_saturated = 0;
              #ifdef ENABLE_SATURATION_DEBUG
                RB6_SET(0);
              #endif
            }
            an_ProcessAnalogData(); //  an_ProcessAnalogData starts a task

            //  setup for next phase:
            sine_table_index = 0;
            phase_vector = VECTOR1;
            _IntegralSum = 0;

            _inv_AdjustRmsSetpoint(); //  TBD - This was needed in LP for flat-top regulation
            _inv_ProcessOverloadCounters(0); //  _inv_processOverloadCounters must be called directly - not a task
        }
        break;
    }

    //  copy the private state to (read-only) public
    _PwmState = pwm_state;
}

//-----------------------------------------------------------------------------

static void _inv_PwmIsrRequest(INV_ISR_REQUEST_t request)
{
    uint8_t saved_ipl;

    SET_AND_SAVE_CPU_IPL(saved_ipl, 7); //  Start of Protected Code
    _inv_PwmIsr(request);
    RESTORE_CPU_IPL(saved_ipl); //  End of Protected Code
}


//-----------------------------------------------------------------------------
//  inv_PwmIsr()
//-----------------------------------------------------------------------------
//  inv_PwmIsr is called from the PWM ISR when Inv status inv_active is set.
//
//  inv_PwmIsr calls the static function _inv_PwmIsr() with the reset flag
//  cleared.
//
//-----------------------------------------------------------------------------

void inv_PwmIsr(void)
{
    _inv_PwmIsr(INV_ISR_REQ_NUL);
}

//-----------------------------------------------------------------------------
//  inv_PwmIsrBootCharge()
//-----------------------------------------------------------------------------

void inv_PwmIsrBootCharge(void)
{
    //  PWM Output Override
    //  OVDCON = 0x3000;    //  (Override) All FETS off
    OVDCONbits.POUT1H = 0; //  PWM1H = OFF
    OVDCONbits.POVD1H = 0; //  Output 1H is controlled by POUT1H

    OVDCONbits.POUT2L = 0; //  PWM2L = OFF
    OVDCONbits.POVD2L = 0; //  Output 2L is controlled by POUT2L
    OVDCONbits.POUT2H = 0; //  PWM2H = OFF
    OVDCONbits.POVD2H = 0; //  Output 2H is controlled by POUT2H

    PDC2 = 0;

    if (--_BootChargeCycles > 0)
    {
        OVDCONbits.POVD1L = 1; //  Output 1L is controlled by PWM1
        PDC1 = (MAX_DUTY * .15); //  15% high / 85% low
    }
    else
    {
        _BootChargeCycles = 0; //  prevent wrap-around
        OVDCONbits.POUT1L = 0; //  PWM1L = OFF
        OVDCONbits.POVD1L = 0; //  Output 1L is controlled by POUT1L

        PDC1 = 0;
    }
}

//-----------------------------------------------------------------------------
//  _inv_CheckLoad
//-----------------------------------------------------------------------------
//  _inv_CheckLoad encompasses functionality associated with Load-Sensing.
//
//	Load-Sensing was originally implemented on the 12LP10. This version has been
//	updated for the 12LPC15 to support a few additional features introduced by
//	RV-C, such as the ability to [dynamically] enable/disable Load-Sensing.
//
//	A state-machine is implemented to provide functionality to support dynamic
//	enable/disable.
//
//	Default values for parameters used here are included in 'inverter.h'.
//
//  Values for Load-Sense Threshold (and Fan Start Threshold) were determined
//  experimentally, by instrumenting the code then collecting and analyzing the
//  data externally.  Refer to 'Shoebox FW Calc.xlsx'.
//
//=============================================================================

void _inv_CheckLoad(int8_t reset)
{
    static int16_t msecs = 0;
    //	static int8_t last = 0;


    if (reset || !IsInvLoadSenseEn())
    {
        //		LOG(SS_INV, SV_INFO, " _inv_CheckLoad - reset! ");
        msecs = 0;
        Inv.status.load_present = 1;
        Inv.status.load_sense_timer = Inv.config.load_sense_delay;
        return;
    }

    if (IMeasRMS() >= Inv.config.load_sense_threshold)
    {
        msecs = 0;
        Inv.status.load_present = 1;
        Inv.status.load_sense_timer = Inv.config.load_sense_delay;
    }

    //  This function is called from the main-loop on a 1-mSec interval. 
    //	Timers run on a 1/2-seond interval (compatible with RV-C)
    if (++msecs >= 500)
    {
        msecs = 0;
        if (--Inv.status.load_sense_timer < 0)
        {
            // limit the value of no_load_timer, prevent wrap-around
            Inv.status.load_sense_timer = 0;
            Inv.status.load_present = 0;
        }
        //		else
        //			LOG(SS_INV, SV_INFO, " lst: %d %d ", 
        //				Inv.status.load_sense_timer, IMeasRMS() );
    }


    //	//	DEBUG
    //	if(last != IsInvLoadPresent())
    //	{
    //		LOG(SS_INV, SV_INFO, " load_present: %d->%d %d", 
    //			last, IsInvLoadPresent(), IMeasRMS() );
    //		last = IsInvLoadPresent();
    //	}
    //	//	/DEBUG
}


//-----------------------------------------------------------------------------
//  inv_Driver()
//  
//  This function comprises the general house-keeping I/O functions for the 
//  inverter controller.  This includes LED signaling, overload detection and 
//  handling of shutdown.
//
//  This is a real-time function. It is intended that to be called periodically 
//  and repeatedly (every one-millisecond, or so...).
//
//  Implements a state-machine to assure that the PWM ISR has sufficient time
//  to respond to shutdown and startup commands.
//
//=============================================================================

// Only _inv_driver() can change this
static INV_STATE_t inv_state = INV_INIT; // inverter state

//-----------------------------------------------------------------------------
// only _inv_Driver() can call this
INLINE void SetInvState(INV_STATE_t state)
{
    inv_state = state;
//  RB7_NumToScope(inv_state); // debugging
}

//-----------------------------------------------------------------------------
static void _inv_Driver(int8_t reset)
{
    static int16_t msec_ticks = 0;
    static int16_t half_sec_ticks = 0;
#define PING_PRE_DELAY_MS   (10)   // milliseconds
#define PING_POST_DELAY_MS  (20)   // milliseconds
#define RESTART_DELAY_MS    (500)  // milliseconds

    if (reset)
    {
        LOG(SS_INV, SV_INFO, "_inv_Driver - reset");

        SetInvState(INV_INIT);
        InvClearErrors();

        //	Clear error flags
        _FeedbackErrorCount = 0;
        Inv.error.feedback_problem = 0;

        return;
    }

    _inv_CheckLoad(0);

    //-------------------------------------------------------------------------
    //  Temperature sensor are bi-metal switches that connect directly to the
    //  FET drivers. When a high-temp condition occurs, the driver is disabled.
    //  Since firmware does not have control of the hardware at that point, a
    //  Feedback error will occur.
    //-------------------------------------------------------------------------
    if (HTSK_HTEMP_ACTIVE() || XFMR_HTEMP_ACTIVE())
    {
        _FeedbackErrorCount = 0;
    }
    else if (_FeedbackErrorCount > FEEDBACK_ERROR_COUNT_LIMIT)
    {
        //  Cannot distinguish between open and shorted at this time.
        Inv.error.feedback_problem = 1;
    }

    switch (inv_state)
    {
    case INV_INIT:
        BIAS_BOOSTING_INACTIVE(); //	Full-power
        Inv.status.load_sense_mode = 0;
        Inv.status.load_present = 1;
        Inv.error.feedback_problem = 0;
        _FeedbackErrorCount = 0;

        _inv_PwmIsrRequest(INV_ISR_REQ_SOFT_START);
        SetInvState(INV_STARTUP_WAIT);
        break;

    case INV_STARTUP_WAIT:
        if (IsInvOutputting())
        {
            SetInvState(INV_NORMAL);
        }
        break;

    case INV_NORMAL:
        //--------------------------------------------------------------------
        //  If any abnormal status bit is set, then we should shutdown.
        //  In normal operation, only the 'Inverter Active' bit, in 
        //  Inv.status.all_flags, should be set, if any other bit is set, 
        //  then shutdown.
        //  A stop request will only be accepted while the PWM ISR is in the 
        //  Normal state (PWM_STATE_NORMAL).
        //--------------------------------------------------------------------

        //  BIAS_BOOSTING_ACTIVE (pin-10 high) shuts down the boost of the 
        //  housekeeping (switching) power supply to reduce power consumption.

        BIAS_BOOSTING_INACTIVE(); //	Full-power
        if (HasInvAnyErrors() || _StopRequest || (INV_MODE_DEGAUSS == _InvMode))
        {
            _inv_PwmIsrRequest(INV_ISR_REQ_SOFT_STOP);
            SetInvState(INV_SHUTDOWN_WAIT);
        }
        else if (IsInvLoadSenseEn() && !IsInvLoadPresent())
        {
            Inv.status.load_sense_mode = 1;
            _inv_PwmIsrRequest(INV_ISR_REQ_SOFT_STOP);
            SetInvState(INV_LOAD_SENSE_WAIT);
        }
        break;

    case INV_LOAD_SENSE_WAIT:
        //  Wait for PWM ISR to stop output
        if (PWM_STATE_IDLE == _PwmState)
        {
            msec_ticks = 0; //	init for next state
            half_sec_ticks = 0;
            SetInvState(INV_LOAD_SENSE);
        }
        break;

    case INV_LOAD_SENSE:
        if (HasInvAnyErrors() || _StopRequest)
        {
            //	TODO: I'm not sure if this is necessary, but I want to make 
            //	sure that BIAS_BOOSTING is "normal" outside of this state.
            BIAS_BOOSTING_INACTIVE(); //	Full-power
            SetInvState(INV_SHUTDOWN);
        }
        else if (!IsInvLoadSenseEn() || IsInvLoadPresent())
        {
            BIAS_BOOSTING_INACTIVE(); //	Full-power
            Inv.status.load_sense_mode = 0;
            _inv_PwmIsrRequest(INV_ISR_REQ_SOFT_START);
            SetInvState(INV_STARTUP_WAIT);
        }
        else
        {
            if (++msec_ticks > 500)
            {
                msec_ticks = 0;
                half_sec_ticks++;
            }

            //	Load-Sense interval should account for PING_PRE_DELAY_MS and 
            //	PING_POST_DELAY_MS. However, these values are relatively 
            //	insignificant in magnitude, therefore they are ignored.

            if (half_sec_ticks >= Inv.config.load_sense_interval)
            {
                BIAS_BOOSTING_INACTIVE(); //	Full-power
                msec_ticks = 0; //	init for next state
                half_sec_ticks = 0;
                SetInvState(INV_PING_WAIT1);
            }
            else
            {
                BIAS_BOOSTING_ACTIVE(); //	Low-power
            }
        }
        break;

    case INV_PING_WAIT1:
        //  Wait for drive power (BIAS_BOOSTING) to stabilize
        if (++msec_ticks > PING_PRE_DELAY_MS)
        {
            msec_ticks = 0;
            _inv_PwmIsrRequest(INV_ISR_REQ_PING);
            SetInvState(INV_PINGING);
        }
        break;

    case INV_PINGING:
        //  Wait for PWM ISR to execute the command
        if (PWM_STATE_IDLE == _PwmState)
        {
            msec_ticks = 0;
            SetInvState(INV_PING_WAIT2);
        }
        //  TODO:  Need timeout here
        break;

    case INV_PING_WAIT2:
        //  Delay after Ping Pulse
        if (++msec_ticks > PING_POST_DELAY_MS)
        {
            msec_ticks = 0;
            SetInvState(INV_LOAD_SENSE);
        }
        break;

    case INV_SHUTDOWN_WAIT:
        //  Wait for PWM ISR to stop output
        if (PWM_STATE_IDLE == _PwmState)
        {
            SetInvState(INV_SHUTDOWN);
        }
        //  TODO:  Need timeout here
        break;

    case INV_SHUTDOWN:
        if (INV_MODE_DEGAUSS == _InvMode)
        {
            // Just wait here...
        }
        else if (!HasInvAnyErrors())
        {
            // debug            KEEP_ALIVE_ON();        //  Turn on the housekeeping supply
            // debug            RCONbits.BOR = 1;       //  (Re-)Enable Interrupts from Brown-Out
            //  It is assumed that we are here because of high-temp shutdown.
            //  We may restart only if high-temp was the only error condition, 
            //  and the High-temp input is no longer asserted.
            SetInvState(INV_RESTART_DELAY);
        }
        break;

    case INV_RESTART_DELAY:
        if (++msec_ticks > RESTART_DELAY_MS)
        {
            msec_ticks = 0;
            _inv_PwmIsrRequest(INV_ISR_REQ_SOFT_START);
            SetInvState(INV_STARTUP_WAIT);
        }
        break;

    default:
        break;
    }

    //  Set public state
    _InvState = inv_state;
}


//-----------------------------------------------------------------------------
//  TASK_inv_Driver
//-----------------------------------------------------------------------------

// called every millisecond

void TASK_inv_Driver(void)
{
    _inv_show_debug(); //  DEBUG
    _inv_show_state_transitions(); //  DEBUG
    _inv_show_pwm_state_transitions(); //  DEBUG

    inv_CheckSupply(0);

    if (!Inv.status.inv_active)
    {
        return;
    }

    switch (_InvMode)
    {
    case INV_MODE_DEGAUSS:
        _inv_Driver(0);

        if (INV_SHUTDOWN == _InvState)
        {
            //  The high-er level state machine (_MainControl() in main.c) will
            //  call inv_Driver while the inv_active flag is set. After 
            //  clearing the inv_active flag the inv_Driver will not be called.
            SetInvNotActive();
            pwm_Stop();
            pwm_Config(PWM_CONFIG_DEFAULT, &pwm_DefaultIsr);
            pwm_Start();
        }
        break;

    case INV_MODE_NORMAL:
        _inv_Driver(0);

        if ((INV_SHUTDOWN == _InvState) && _StopRequest)
        {
            SetInvNotActive();
            pwm_Stop();
            pwm_Config(PWM_CONFIG_DEFAULT, &pwm_DefaultIsr);
            pwm_Start();
        }
        break;

    case INV_MODE_BOOT_CHARGE:
        if (0 == _BootChargeCycles)
        {
            SetInvNotActive();
            pwm_Stop();
            pwm_Config(PWM_CONFIG_DEFAULT, &pwm_DefaultIsr);
            pwm_Start();
        }
        break;

    default:
        LOG(SS_INV, SV_ERR, "_InvMode = %d", _InvMode);
        _InvMode = INV_MODE_NORMAL;
        break;
    }
}

//-----------------------------------------------------------------------------

static void _inv_AdjustRmsSetpoint(void)
{
    //  This is a simple/slow control algorithm
    if (VacRMS() < RMS_VAC_SETPOINT)
    {
        _RmsVacSetpointOffset++;
    }
    else if (VacRMS() > RMS_VAC_SETPOINT)
    {
        _RmsVacSetpointOffset--;
    }
}

//-----------------------------------------------------------------------------

static void _inv_start(void)
{
    pwm_Stop();
    _inv_PwmIsr(INV_ISR_REQ_RESET); //  reset the PWM ISR
    _inv_Driver(1); //  reset driver
    inv_CheckSupply(1); //  reset Battery-Low/High
    _inv_CheckLoad(1); //	reset Load-Sensing
    an_InitRms(); // initialize RMS calculation data

    pwm_Config(PWM_CONFIG_INVERTER, &inv_PwmIsr);
    pwm_Start();
}

//-----------------------------------------------------------------------------
// initialize inverter data

void inv_Init()
{
    //    memset(&Inv.status,0,sizeof(Inv.status));
}

//-----------------------------------------------------------------------------

void inv_Start(void)
{
    LOG(SS_INV, SV_INFO, "inv_Start()");

    _VacOffset = VAC_ZERO_CROSS_A2D;
    _InvMode = INV_MODE_NORMAL;
    _inv_start();
    _StopRequest = 0;
    SetInvActive();
}

//-----------------------------------------------------------------------------

void inv_StartBootCharge(void)
{
#define BOOT_CHARGE_CYCLES  ((int16_t)(FPWM * (BOOT_CHARGE_DELAY_MSEC * .001)))

    LOG(SS_INV, SV_INFO, "inv_StartBootCharge()");

    pwm_Stop();
    _InvMode = INV_MODE_BOOT_CHARGE;
    _BootChargeCycles = BOOT_CHARGE_CYCLES;
    pwm_Config(PWM_CONFIG_BOOT_CHARGE, &inv_PwmIsrBootCharge);
    pwm_Start();
    SetInvActive();
}

//-----------------------------------------------------------------------------

void inv_StartDegauss(void)
{
    LOG(SS_INV, SV_INFO, "inv_StartDegauss()");

    _InvMode = INV_MODE_DEGAUSS;
    _inv_start();
    SetInvActive();
}


//-----------------------------------------------------------------------------

void inv_Stop(void)
{
    LOG(SS_INV, SV_INFO, "inv_Stop()");
    _StopRequest = 1;
}

//-----------------------------------------------------------------------------

void inv_StopNow(void)
{
    _inv_PwmIsrRequest(INV_ISR_REQ_STOP);
    SetInvNotActive();
}

// <><><><><><><><><><><><><> inverter.c <><><><><><><><><><><><><><><><><><><>
