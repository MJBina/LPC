// <><><><><><><><><><><><><> fan_ctrl_np.c <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//  Copyright(C) 2015 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
//  Fan Control for NP flavor board
//
//  Operational requirements:
//    1) Min Duty-Cycle (D.C.) = 36%. Max D.C. = 100%.
//    2) Fan on at 100% if XFMR_OT or the XFMR_FAN_SWITCH input is active
//    3) Fan starts (Min D.C.) when Heat-Sink temperature reaches 47'C.
//    4) Fan is at max speed (100% D.C.) when Heat-Sink temperature reaches 63'C.
//       Therefore, max fan D.C. increases 4% for each 'C
//    5) Fan D.C. increases at 4.27% /second. (1% every 234 mSec)
//    6) Fan D.C. decreases at .333% /second.
//  
//  Initialize Timer6 for use as a one-shot. Timer is started by fan_Timer().
//  fan_Timer() turn the fan off.  When Timer6 expires, the Timer6
//  ISR turns the fan on
//
//  2017-10-26
//    Fan algorithm was modified so the fan turns off when the FETs turn on.
//    This was to prevent destabilizing the over-load signal.
//    The timer interrupt turns the fan on, rather than off.
//    Note that the timer tick count is complemented.
//
// Note:
//   Over-ride can increase the fan rate but not decrease it from the sensor reading
//
//-----------------------------------------------------------------------------

// -------
// headers
// -------
#include "options.h"    // must be first include
#include "charger.h"
#include "fan_ctrl.h"
#include "hs_temp.h"
#include "hw.h"
#include "inverter.h"
#include "sine_table.h"

// --------------------------
// Conditional Debug Compiles
// --------------------------

// comment out to not use it
//#define DEBUG_FAN_CONTROL 1

// ---------
// Settings
// ---------
// duty values in percentage x 10 (for increased resolution)
#define FAN_INC_DUTY     60  // percentx10
#define FAN_DEC_DUTY      8  // percentx10
#define FAN_MIN_VOLTS   6.0  // volts

#if defined(OPTION_FAN_ALWAYS_ON)
  #undef  FAN_MIN_VOLTS
  #define FAN_MIN_VOLTS  6.9 // Volta requirement
#endif

// ---------
// Constants
// ---------
#define PERCENT_100  (1000)  // x10 for increased resolution


// ------------------------------------------------------
//           F A N   C A L I B R A T I O N
// ------------------------------------------------------

// +-----+------+---------+-------+------+
// | PWM | Timer| Timer   |       |      |
// | Duty|  Val |   On    |  Fan  |  Fan |
// | (%) | ticks| (usecs) | volts |  RPM |
// +-----+------+---------+-------+------+
// |  10 |  173 |  5.0500 |  2.66 |    0 |
// |  12 |  208 |  5.8988 |  2.63 |    0 |
// |  14 |  243 |  6.9352 |  2.64 |    0 |
// |  15 |  260 |  7.1288 |  4.15 | 1015 |
// |  20 |  347 |  9.3196 |  4.95 | 1289 |
// |  30 |  520 | 13.7188 |  6.39 | 1761 | <-- Volta Customer Target 6.9 volts
// |  40 |  694 | 18.1628 |  7.61 | 2117 |
// |  50 |  868 | 22.4540 |  8.51 | 2398 |
// |  60 | 1041 | 26.7896 |  9.37 | 2647 |
// |  70 | 1215 | 31.1236 | 10.05 | 2823 |
// |  80 | 1388 | 35.4576 | 10.87 | 3020 |
// |  90 | 1562 | 39.7924 | 12.20 | 3333 |
// |  95 | 1649 | 41.9292 | 12.94 | 3481 |
// |  99 | 1718 | 43.0300 | 13.61 | 3594 |
// | 100 | 1736 | 43.4184 | 13.62 | 3620 |
// +-----+------+---------+-------+------+


// +-----+-------+------+-------+
// | Fan |       |  PWM | Timer |
// | Duty|   Fan |  Duty| Value |
// | x10 |  Volts|  x10 | ticks |
// +-----+-------+------+-------+
// |   10|  5.711|  267 |   463 |
// |   50|  6.095|  297 |   515 |
// |  100|  6.567|  334 |   579 |
// |  200|  7.4  |  408 |   708 |
// |  300|  8.12 |  482 |   836 |
// |  400|  8.77 |  556 |   965 |
// |  500|  9.35 |  630 |  1093 |
// |  600|  9.98 |  704 |  1222 |
// |  700| 10.71 |  778 |  1350 |
// |  800| 11.67 |  852 |  1472 |
// |  900| 12.71 |  926 |  1608 |
// |  950| 13.48 |  963 |  1671 |
// |  970| 13.67 |  977 |  1696 |
// | 1000| 13.69 | 1000 |  1736 |
// +-----+-------+------+-------+


// fan timer ticks per pwm cycle  // FCY=40,000,000 FPWM=23,040
#define FAN_TIMER_TICKS_PER_PWM  ((uint16_t)((uint32_t)FCY/(uint32_t)FPWM))  // fan clocks per PWM cycle (1736)

// Fan Calibration: Fan Volts to PWM Duty x10
// -1.2413*v^3 + 33.621*v^2 - 192.46*v + 499.46  from Excel fit
#define FAN_VOLTS_TO_PWMX10(volts)  ((uint16_t)((-1.2413)*(volts)*(volts)*(volts)+(33.621)*(volts)*(volts)+(-192.46)*(volts)+(499.46)))

// minimum fan pwm duty allowed
#define FAN_MIN_PWM_DUTY  ((uint16_t)FAN_VOLTS_TO_PWMX10(FAN_MIN_VOLTS))   // percent x10

// fan timer ticks                                              
#define FAN_TIMER_MAX     ((uint16_t)(FAN_TIMER_TICKS_PER_PWM))                           // clock ticks
#define FAN_TIMER_MIN     ((uint16_t)((FAN_TIMER_TICKS_PER_PWM*FAN_MIN_PWM_DUTY)/1000L))  // clock ticks

// fan duty to pwm duty
#define FAN_PWMX10_TO_DUTY(pwmx10)   ((uint16_t)((((uint32_t)(pwmx10)-FAN_MIN_PWM_DUTY)*1000)/(PERCENT_100-FAN_MIN_PWM_DUTY)))

// fan mininum duty based upon fan voltage
#define FAN_MIN_DUTY    ((uint16_t)(FAN_PWMX10_TO_DUTY(FAN_MIN_PWM_DUTY)+1)) // not zero

// ------------
// Show values
// ------------
#ifndef WIN32
  #pragma message("Fan Min Volts=" DUMP_MACRO(FAN_MIN_VOLTS))
#endif


// ----------
// local data
// ----------
// all values in percentage x 10 (for increased resolution)
static uint16_t g_fan_duty_x10    = 0;  // actual rpm duty regulating fan  (percentx10)
static uint16_t g_pwm_duty_x10    = 0;  // pwm duty driving fan            (percentx10)
static uint16_t g_over_ride_duty  = 0;  // duty requested by over-ride     (percentx10)
static uint16_t g_fan_tmr_val     = 0;  // timer value
#if defined(OPTION_FAN_ALWAYS_ON)
  static uint8_t  g_fan_always_on_exception = 0;  // 0=fan alwyas on, 1=allow fan to turn off
#endif


//	fan_Start() and fan_Stop() were added for the LPC to provide high-level
//	control of the fan to mitigate issues with the housekeeping supply. 
//	Specifically, that if we attempt to do a boot-charge while the fan is 
//	running, the housekeeping supply is drawn down. 
//	These are just stubs here so that NP can use a common header 'fan_ctrl.h'.

void fan_Start(void) { }

void fan_Stop(void) { }



//-----------------------------------------------------------------------------
//  fan_Config() - Configure T6 Timer
//
//	Timer6 generates a PWM signal to control fan-speed.
//	The NP control board has filtering that is optimized for a PWM signal of 
//	approximately 20KHz.  
//
//  TMRx: 16-bit timer count register
//  PRx: 16-bit period register associated with the timer
//  TxCON: 16-bit control register associated with the timer
//-----------------------------------------------------------------------------

// 8 Mhz crystal: 40.000 MIPS
void fan_Config(void)
{
  #ifndef WIN32
    T6CONbits.TON = 0;          // Disable Timer1

    // TCS: Timer Clock Source Select bit
    //      1 = External clock from pin TxCK
    //      0 = Internal clock (Tcy)
    T6CONbits.TCS = 0;

    T6CONbits.TGATE = 0;        // Disable Gated Timer mode

    // TCKPS<1:0>: Timer Input Clock Prescale Select bits
    //      11 = 1:256 prescale value
    //      10 = 1:64 prescale value
    //      01 = 1:8 prescale value
    //      00 = 1:1 prescale value
    T6CONbits.TCKPS = 0;  // 1:1 (highest resolution)
    // Timer 6 priority is set in main and needs highest priority for proper timing

    //  Clear the timer register (Timer1)
    TMR6 = 0;
    PR6  = 0;
    // 16-bit period register
    
  #endif

    LOG(SS_SYS, SV_INFO, "Fan: MinVolt=%.1f MinDuty=%u MinPWM=%u MinTmr=%u MaxTmr=%u",
        FAN_MIN_VOLTS, FAN_MIN_DUTY, FAN_MIN_PWM_DUTY, FAN_TIMER_MIN, FAN_TIMER_MAX);

  #if defined(OPTION_FAN_ALWAYS_ON)
    LOG(SS_SYS, SV_INFO, "Fan Always On Option");
  #endif
}

// ----------------------------------------------------------------------------
INLINE void FAN_ON()
{
    FAN_CTRL_ON();
 // RB12_SET(1);
}

// ----------------------------------------------------------------------------
INLINE void FAN_OFF()
{
    FAN_CTRL_OFF();
 // RB12_SET(0);
}

// ----------------------------------------------------------------------------
// turn off fan after timer expires
void _T6Start(uint16_t tmr_val)
{
    TMR6 = 0;
    PR6  = tmr_val;

    IFS2bits.T6IF = 0;      // Clear Timer Interrupt Flag
    IEC2bits.T6IE = 1;      // Enable Interrupts for Timer

    T6CONbits.TON = 1;      // turn on timer
}

// ----------------------------------------------------------------------------
// turn off timer
void _T6Stop()
{
    T6CONbits.TON = 0;      // turn off timer
}

// ----------------------------------------------------------------------------
void __attribute__((interrupt, no_auto_psv)) _T6Interrupt (void)
{
    FAN_ON();
    IFS2bits.T6IF = 0;  // end-of-interrupt
}

// ----------------------------------------------------------------------------
//                U T I L I T Y    R O U T I N E S
// ----------------------------------------------------------------------------

// return fan duty cycle 0=off, 100=full-on
uint16_t fan_GetDutyCycle()
{
    return(g_fan_duty_x10/10); // return percentage
}

// ----------------------------------------------------------------------------
uint16_t CalcTimerValue(uint16_t pwm_duty_x10)
{
    uint32_t temp = pwm_duty_x10; // use 32-bit calculation because it can overflow
        
    if (temp > PERCENT_100) temp = PERCENT_100;
    temp = PERCENT_100 - temp; // complement
    temp = ((FAN_TIMER_TICKS_PER_PWM*temp)/1000L);
    return((uint16_t)temp);
}

// ----------------------------------------------------------------------------
uint16_t CalcPwmDuty(uint16_t fan_duty_x10)
{
    uint32_t temp = fan_duty_x10; // use 32-bit calculation because it can overflow

    if (fan_duty_x10 == 0) return(0);
    if (temp > PERCENT_100) temp = PERCENT_100;
    temp = ((PERCENT_100-FAN_MIN_PWM_DUTY) * temp)/1000L + FAN_MIN_PWM_DUTY;
    return((uint16_t)temp);
}

// ----------------------------------------------------------------------------
void fan_SetDutyCycle(uint16_t fan_duty_x10) // 0-1000
{
    uint16_t duty, tmr_val, pwm_duty;

    duty = fan_duty_x10; // use temporary
    if (duty > PERCENT_100) duty = PERCENT_100;

    // over-ride (for automated testing)
    //  can increase the fan but not decrease it from sensor reading
    if (g_over_ride_duty > duty) duty = g_over_ride_duty;

   #if defined(OPTION_FAN_ALWAYS_ON)
    if (!g_over_ride_duty && (fan_duty_x10 < FAN_MIN_DUTY))
        duty = g_fan_always_on_exception ? 0 : FAN_MIN_DUTY;
   #endif

    // calculate values
    pwm_duty = CalcPwmDuty(duty);
    if (pwm_duty < FAN_MIN_PWM_DUTY)
    {
        pwm_duty = 0;
        tmr_val  = 0;
    }
    else
    {
        tmr_val = CalcTimerValue(pwm_duty);
    }

    // set as a group
    g_fan_duty_x10 = duty;
    __builtin_disi(0x3FF); // dsiable interrupt; atomic
    g_fan_tmr_val  = tmr_val;
    g_pwm_duty_x10 = pwm_duty;
    __builtin_disi(0x1);  // enable ints for one more cycle
}

// ----------------------------------------------------------------------------
// over-ride fan control for testing (can not turn off fan if heatsink indicates on)
// percent 0=off, 100=full-on
void fan_SetOverRide(uint16_t percent) // 0-100
{
    percent = percent * 10;
    if (percent > PERCENT_100) percent = PERCENT_100;
    g_over_ride_duty = percent;
    fan_SetDutyCycle(g_fan_duty_x10);
}

// ----------------------------------------------------------------------------
//           F A N     T I M E R     D R I V E R
// ----------------------------------------------------------------------------

#define MIN_FAN_OFF_TICKS  20    // too small time is a problem 15=ok  10=fail

// The fan timer runs as 23khz and is tuned for the inductor on the fan.
// Changing this frequency will cause problems.

// handle fan duty cycle
void fan_Timer(void)  // called at FPWM frequency by PWM interrupt
{
    _T6Stop();

	if (g_pwm_duty_x10)
	{
        if (g_fan_tmr_val < MIN_FAN_OFF_TICKS)
        {
    		FAN_ON();
            return;
        }
  		FAN_OFF();

        _T6Start(g_fan_tmr_val);    // timer will turn it on
	}
	else
	{
        // fan is off
		FAN_OFF();
	}
}

// ----------------------------------------------------------------------------
#if defined(OPTION_FAN_ALWAYS_ON)
 // returns: 0=no exception, 1=allow fan to turn off
 static uint8_t FanAlwaysOnException(void)
 {
    // --------------------------------------------------------
    // Volta requirement exceptions
    //   If inverter mode and low batt shutdown: allow fan off
    //   If inverter mode and overload shutdown: allow fan off
    //   If inverter or charger and CAN disable: allow fan off
    //   If charge mode and BCR set to zero:     allow fan off
    //
    // As usual, fan will not turn off immediately but will
    // gradually slow down and turn off
    // --------------------------------------------------------
     // similar logic to ui.c message handling
    if (IsInvActive()) return(0); // inverter has active output
    if (IsChgrActive()) return(0); // charger  is running
    if (!HasInvRequest() && !HasChgrRequest()) return(1); // idle
    if (IsInvFeedbackProblem() || IsInvFeedbackReversed()) return(1); // feedback problems
    if (IsAcLineQualified())
    {
        // ac line qualified but not charging
   		if (!Device.status.chgr_enable_can ) return(1); // disabled by CAN
		if (!Device.status.chgr_enable_bcr ) return(1); // BCR is zero
		if (!Device.status.chgr_enable_jmpr) return(1); // disabled by charger enable switch
		if (!Device.status.chgr_enable_time) return(1); // ### ask Gary
    }
    else if (HasInvRequest())
    {
        // inverter requested but not inverting
        if (IsInvLowBattShutdown())  return(1);
        if (IsInvOverLoadShutdown()) return(1);
        if (!IsInvEnabled())         return(1);
    }
    return(0);
 }
#endif // OPTION_FAN_ALWAYS_ON

// ----------------------------------------------------------------------------
// call once per milli-second
void TASK_fan_Driver(void)
{
    #define FAN_TEMP_CUTOFF  (47) // Celcius; greater than turns on fan
    static uint16_t g_msec_counter = 0;
    static int16_t  hs_temp=0, xfrm_ot=0, fan_switch=0;
    uint16_t max_allowed_duty = 0;
    uint16_t new_duty=0, temp_compare=0;

	// check for one second passed
	++g_msec_counter;  // one more milli-second has passed

  #ifdef DEBUG_FAN_CONTROL
    if (g_msec_counter == 1)
    { // log on a different tick than the computation tick
        LOG(SS_SYS, SV_INFO, "FAN OT=%u SW=%u hsTmp=%u fanD=%u pwmD=%u tmrVal=%u",
            xfrm_ot, fan_switch, hs_temp, g_fan_duty_x10, g_pwm_duty_x10, g_fan_tmr_val);
    }
  #endif
	if (g_msec_counter < 1000) return;
	g_msec_counter = 0; // reset counter

	// one second has passed
    // read heat sink temperature (Celcius)
    hs_temp = HeatSinkTempC();  // get current heat sink temperature

    // read sensors
	xfrm_ot    = XFMR_OT_ACTIVE();
	fan_switch = FAN_SWITCH_ACTIVE();

    // calculate new duty cycle
    new_duty = g_fan_duty_x10;
	if (IsInvLowBattShutdown() && !IsChgrActive())
	{
		//	Turn the fan off
		max_allowed_duty = 0;
	}
    else if (xfrm_ot || fan_switch)
    {
        max_allowed_duty = PERCENT_100;
    }
    else
    {
        // if the fan is running, and fan temp is 46 or 47 Celcius, keep the fan running
        temp_compare = hs_temp;
        if (g_fan_duty_x10 && (hs_temp > (FAN_TEMP_CUTOFF-2)) && (hs_temp <= FAN_TEMP_CUTOFF))
            temp_compare = FAN_TEMP_CUTOFF+1; // keep the fan running at 48 Celcius duty cycle

        //  Fan max D.C. increases 4% for each 'C above 47'C
        if (temp_compare > FAN_TEMP_CUTOFF) // Celcius
        {
            uint32_t delta   = ((PERCENT_100-FAN_MIN_DUTY)*((uint32_t)temp_compare - FAN_TEMP_CUTOFF))/17L;  // use 32 bit so no overflow
            max_allowed_duty = (uint16_t)delta + FAN_MIN_DUTY;

          #ifdef DEBUG_FAN_CONTROL
            LOG(SS_SYS, SV_INFO, "FAN-Temp >47C: hsTmp=%u maxDuty=%u delta=%lu", hs_temp, max_allowed_duty, delta);
          #endif
        }
    }

    // turn fan on to minimal value if needed
    if ((max_allowed_duty > FAN_MIN_DUTY) && (new_duty < FAN_MIN_DUTY))
    {
        new_duty = FAN_MIN_DUTY;
    }

    // adjust current duty cycle
    if (new_duty == max_allowed_duty)
    {
        // no change
    }
    else if (new_duty < max_allowed_duty)
    {
        // increase fan speed
        new_duty += FAN_INC_DUTY;
    }
    else if (new_duty > max_allowed_duty)
    {
        // decrease fan speed
        if (new_duty  > FAN_DEC_DUTY)
            new_duty -= FAN_DEC_DUTY;
        else
            new_duty = 0;
    }

    // range check
    if (new_duty > PERCENT_100) new_duty = PERCENT_100;

  #if defined(OPTION_FAN_ALWAYS_ON)
    g_fan_always_on_exception = FanAlwaysOnException();
    if (new_duty < FAN_MIN_DUTY) new_duty = g_fan_always_on_exception ? 0 : FAN_MIN_DUTY;
  #else
    if (new_duty < FAN_MIN_DUTY) new_duty = 0;
  #endif

    // set duty cycle
    fan_SetDutyCycle(new_duty);
}

// <><><><><><><><><><><><><> fan_ctrl_np.c <><><><><><><><><><><><><><><><><><><><><><>
