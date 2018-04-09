// <><><><><><><><><><><><><> charger_isr.c <><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//  Copyright(C) 2018 - Sensata Technologies, Inc.  All rights reserved.
//
//  $Workfile: charger_isr.c $
//  $Author: clark_dailey $
//  $Date: 2018-03-20 13:00:34-05:00 $
//  $Revision: 89 $ 
//
//  Charger Interrupt Service Routine
//      
//-----------------------------------------------------------------------------

// -------
// headers
// -------
#include "options.h"    // must be first include
#include "hw.h"
#include "analog.h"
#include "charger.h"
#include "pwm.h"
#include "config.h"
#include "sine_table.h"
#include "sqrt.h"
#include "tasker.h"
#include "Q16.h"

// --------------------------
// Conditional Debug Compiles
// --------------------------

// comment out flag to not use it
//#define  DEBUG_CHG_DISABLE_DRIVES    1

// -----------------------------
// Auto Removal for build types
// -----------------------------
#if OPTION_NO_CONDITIONAL_DBG
  #undef DEBUG_CHG_DISABLE_DRIVES
#endif	


// ------------------
// Charger PWM States
// ------------------
typedef enum 
{
    CHGR_PWM_INIT             = 0,
    CHGR_PWM_POS_WAIT_NEG_1   = 1,       
    CHGR_PWM_NEG_WAIT_POS_1   = 2,          
    CHGR_PWM_POS_WAIT_NEG_2   = 3, 
    CHGR_PWM_NEG_WAIT_POS_2   = 4,      
    CHGR_PWM_CONFIRM_NEG_POS  = 5,      
    CHGR_PWM_POS_DETECT_OC    = 6,      
    CHGR_PWM_CONFIRM_POS_NEG  = 7,         
    CHGR_PWM_NEG_DECT_OC      = 8,   
	NUM_CHGR_PWM_STATES  // This has to be last
} INV_STATE_t;



//-----------------------------------------------------------------------------
//  PWM PULSE LIMITS
//
//  At 40MIPS a 23040Hz PWM clock has 12.5nSec resolution.
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
//  The calculated Min Duty Cycle is then: 1.5/.0125 = 120.
//-----------------------------------------------------------------------------

#define MIN_DUTY    (120)
//  We will use one-half the Min Duty Cycle as the Min Threshold
#define MIN_THRES   (MIN_DUTY/2)
//  The Max Duty Cycle is (((FCY/FPWM) - 1) * 2)
//		for: FCY =  40000000, FPWM = 23040  -->  Max Duty = 3470 (0x0D8E)
#define MAX_DUTY    (((FCY/FPWM) - 1) * 2)
//  The Max Threshold is the Max Duty Cycle less the Min Duty Cycle.
#define MAX_THRES   (MAX_DUTY - MIN_DUTY)

//-----------------------------------------------------------------------------
//  ZC_CONF_TIMEOUT defines a number of PWM-clock cycles for qualifying the 
//	zero-crossing AFTER it was detected. 
//	Zero-Crossing is detected when the instantaneous value of the AC voltage 
//	crosses the zero-cross threshold (VAC_ZERO_CROSS_A2D).
//	Zero-crossing is confirmed using the averaged point values.
//	If the zero-crossing is not confirmed within the specified number of 
//	PWM-clock cycles (ZC_CONF_TIMEOUT), then re-synch.
//
//	Zero-crossing confirmation uses averaged values. Therefore, 
//-----------------------------------------------------------------------------
#define ZC_CONF_TIMEOUT    (50) //25

//-----------------------------------------------------------------------------
//  ZC_DET_WIN_START defines a number of PWM-clock cycles before the expected 
//	zero-crossing. This is essentially the start of the window where zero-
//	crossing is expected. (ZC_DET_WIN_END defines the end of that window.)
//	The expected zero-crossing is based on the prior half-cycle of the same 
//	polarity.
//	Zero-Crossing is detected when the instantaneous value of the AC voltage 
//	crosses the zero-cross threshold (VAC_ZERO_CROSS_A2D). 
//	If a zero-crossing is detected prematurely, then re-synch.
//-----------------------------------------------------------------------------
#define ZC_DET_WIN_START	(10)

//-----------------------------------------------------------------------------
//	ZC_DET_WIN_END defines a number of PWM-clock cycles after the expected 
//	zero-crossing. If the zero-crossing is not detected within this window, 
//	then re-synch.
//-----------------------------------------------------------------------------
#define ZC_DET_WIN_END	(25)

//  PWM_PHASE_MAX corresponds to 180-degrees
#define PWM_PHASE_MAX   (MAX_SINE)


#define IMEAS_MAX       20

//	DEBUG
#define PDC_05   ((int16_t)(MAX_DUTY * .05))
#define PDC_10   ((int16_t)(MAX_DUTY * .10))
#define PDC_20   ((int16_t)(MAX_DUTY * .20))
#define PDC_30   ((int16_t)(MAX_DUTY * .30))
#define PDC_40   ((int16_t)(MAX_DUTY * .40))
#define PDC_50   ((int16_t)(MAX_DUTY * .50))
#define PDC_55   ((int16_t)(MAX_DUTY * .55))
#define PDC_60   ((int16_t)(MAX_DUTY * .60))
#define PDC_70   ((int16_t)(MAX_DUTY * .70))
#define PDC_80   ((int16_t)(MAX_DUTY * .80))
#define PDC_90   ((int16_t)(MAX_DUTY * .90))
#define PDC_95   ((int16_t)(MAX_DUTY * .95))
#define PDC_100  ((int16_t)(MAX_DUTY))
//#define PDC_100  ((int16_t)(MAX_DUTY * .95))  //  DEBUG


//-----------------------------------------------------------------------------
//                G L O B A L    D A T A
//-----------------------------------------------------------------------------

static   DWORD_t temp32;
static   DWORD_t curr_duty = {{0}};
uint16_t ZcThreshold = VAC_ZERO_CROSS_A2D;
uint16_t _comp_q16   = 0;
uint16_t _duty_cycle = 0;
// static uint16_t _comp_q16 = 0;

// '_comp_gain_q16' and '_comp_offset_q16' are Q16 Fractional values, having
// a range from zero to 0.999.  These variables allow the higher-level code to
// adjust the PWM Duty-Cycle, through interface functions.
// static int16_t _comp_gain_q16 = 0;
// static int16_t _comp_offset_q16 = 0;


// -------------
//	Prototyping
// -------------

// static void _ToggleTP(int16_t count);


//-----------------------------------------------------------------------------
void _chgr_DutyReset(void)
{
	_comp_q16 = 0;
}

//-----------------------------------------------------------------------------
//  _chgr_DutyAdjust
//  Returns 1 (non-zero) if the adjustment was successful.
//  Returns 0 (zero) when the adjustment is at its limit.
//-----------------------------------------------------------------------------

#define MAX_DUTY_COUNTS  (31135)  // number of duty adjust counts from 0% to 100% duty cycle

int8_t _chgr_DutyAdjust(int16_t count)
{
    int8_t result = 0;

	if (count < 0)
    {
        // decrement duty cycle
        count = -count; // positive
        if (count >= _comp_q16)
        {
            // at min
       		_comp_q16 = 0;
       		result    = 0; // at min
        }
        else
        {
            // not at min
       		_comp_q16 -= count;
       		result     = 1; // ok
        }
    }
    else if (count > 0)
    {
        // increment duty cycle
        if (count >= (0xFFFF - _comp_q16))
        {
            // at max
       		_comp_q16 = 0xFFFF;
       		result    = 0; // at max
        }
        else
        {
            // not at max
       		_comp_q16 += count;
       		result     = 1; // ok
        }
    }
    return(result);
}

//-----------------------------------------------------------------------------
//	chgr_IsPwmDutyMinimum is used when testing for 'Peak Rectification'.
//	Peak Rectification refers to the the condition where the FET body diodes 
//	conduct due to a low-voltage on the charger output. This can result in an
//	uncontrolled high-current.
//
//	Peak Rectification is detected if ALL of the following conditions are true:
//		If the charger relay is closed, AND
//		If the Charger PWM Duty Cycle is at minimum (charger SHOULD not be 
//			producing current, AND
//		If the Charger Output current is NOT zero 
//
//	NOTE: Peak Rectification will likely occur when the output of the charger is 
//	short-circuited.
//-----------------------------------------------------------------------------

int8_t chgr_IsPwmDutyMinimum(void)
{
    //	'CHGR_MIN_COMP_THRES_Q16' is provided to accommodate modification of the
    //	minimum duty cycle. 
    //	In response to a high-current the charger algorithm will attempt to 
    //	'ramp-down' the duty-cycle.  The algorithm is intentionally slow for 
    //	better stability. Peak Rectification may be a safety hazard, and needs
    //	an immediate response.
    //
    //	The charger output may be zero even when the PWM is not. Hence, 
    //	adjusting this threshold may improve the responsiveness of the Peak
    //	Rectification detection algorithm as described above.

#define CHGR_MIN_COMP_THRES_Q16		((uint16_t)(0))
    return ((CHGR_MIN_COMP_THRES_Q16 >= _comp_q16) ? 1 : 0);
}



//-----------------------------------------------------------------------------
//	_chgr_DutyDecStop()
//
//	The charger may be stopped by multiple sources. It was determined that  
//	abruptly stop the charger may cause large transients, particularly if the
//	charger is producing high current. 
//
//	A specific scenario that we need to address is Volta. Volta can disable the
//	charger via a Hardware Contact Closure input.  2-seconds after disabling 
//	the charger, they open the main contactor. We need the charger to shutdown 
//	prior to opening the contactor.  To satisfy that requirement, we will use 
//	one-second as a time-budget.
//
//	Rather than attempt to compute a smooth transition based upon time, we will
//	calculate a worst case duty-cycle increment. That is, we want to transition 
//	from MAX_DUTY to MIN_DUTY within one-second (60-cycles) 
//
//	The application will call _chgr_DutyDecStop() once-per-cycle.
//	_chgr_DutyDecStop() will return 1 while actively decrementing.
//	_chgr_DutyDecStop() will return 0 when it has reached MIN_DUTY.
//	The application will stop calling _chgr_DutyDecStop() when it returns 0
//
//-----------------------------------------------------------------------------

#define CHG_SOFT_STOP_DEC_VAL	((0xFFFF)/60)

int8_t _chgr_DutyDecStop(void)
{
	_comp_q16 -= CHG_SOFT_STOP_DEC_VAL;
	
	if(_comp_q16 < 0)
	{
		_comp_q16 = 0;
		
		// Override L1 Drivers to be OFF (Low)
        OVDCONbits.POUT1L = 0;      //  PWM1L = OFF --> Drive output is LOW
        OVDCONbits.POVD1L = 0;      //  Override: 1L controlled by POUT1L

        // Override L2 Drivers to be OFF (Low)
        OVDCONbits.POUT2L = 0;      //  PWM2L = OFF --> Drive output is LOW
        OVDCONbits.POVD2L = 0;      //  Override: 2L controlled by POUT2L

		PDC1 = 0;
		PDC2 = 0;

		return(0);
	}
	return(1);
}

//-----------------------------------------------------------------------------
//  _chgr_oc_detect()
//-----------------------------------------------------------------------------
//  This INLINE function was created 2018-02-15 to add filtering to the IMeas
//  current signal to address the differences in the response times of the 
//  current sensors between NP and LPC. NP uses a current sense transformer 
//  having a response time of 400 Hz. LPC uses a Hall-Sensor with a response 
//  time of 120 KHz.
//
//  This is a simple averaging filter, and will add delay proportional to the 
//  length of the filtering array.
//  
//  [From L. Mihai's email 2018-02-15] The NP current sensor has a 400Hz BW 
//  (2.5 mSec). Therefore we want this filter to be 2.5 mSec.
//
//  Tpwm = 1/23,040Hz = ~43.4 uSec   --->  2500/43.4 = 57.6
//
//  An A2D value is 10-bits. We should be able to accommodate a filter up to 
//  32 elements in length without (16-bit signed) integer overflow. Therefore, 
//  a 32-bit (signed) integer is used for the sum.
//-----------------------------------------------------------------------------

#if !defined(OPTION_IGNORE_OVERLOAD)

  #if IS_DCIN_51V
    #define IMEAS_PEAK_LIMIT_ADC	(IMEAS_CHG_AMPS_ADC(50.0))  // per Gary
  #else
    #define IMEAS_PEAK_LIMIT_ADC	(IMEAS_CHG_AMPS_ADC(20.0))  // per Pao 2017-11-20
  #endif


INLINE int8_t _chgr_oc_detect(void)
{
    #define IMEAS_FILTER_LENGTH ((int16_t)(4)) // 4 PWM cycles per Leonard 2018-03-15
    static int16_t ay[IMEAS_FILTER_LENGTH] = { 0 };
    static int16_t ix = 0;       //  always points to next element to get value
    static int16_t valid = 0;
    static int32_t sum32 = 0;
    int8_t result = 0;
    
    
    if(IsCycleAvgValid()) 
    {
        sum32 -= ay[ix];        //  subtract the oldest element from the sum
        ay[ix] = IMeasRaw();    //  update the array with new value
        sum32 += ay[ix];        //  add the new value to the sum
        if(++ix >= IMEAS_FILTER_LENGTH)
        {
            ix = 0;
            valid = 1;
        }
        if(valid)
        {
            int16_t avg = sum32/IMEAS_FILTER_LENGTH;
            if((avg > (IMeasCycleAvg() + IMEAS_PEAK_LIMIT_ADC)) ||
                (avg < (IMeasCycleAvg() - IMEAS_PEAK_LIMIT_ADC)))
            {
                result = 1;
            }
        }
    }
    return(result);
}

#endif	//  OPTION_IGNORE_OVERLOAD 

    

//-----------------------------------------------------------------------------
//  _chgr_pwm_isr()
//-----------------------------------------------------------------------------
//  The Charger PWM ISR comprises the Control Loop and Zero-Crossing Detection.
//  It interfaces with chgr_Driver() using global variables to configure the
//  control loop.
//
//  The Control Loop regulates voltage or current. These modes are configured
//  by chgr_Driver() using the variable '_ChargeMode'.
//
//  Voltage regulation is controlled by sampling the battery voltage each PWM
//  cycle, and compensating for errors. The output pulse varies for each output
//  pulse.
//
//  Current regulation uses the RMS value of the current, which is calculated
//  once each AC cycle. Hence compensation controls also run once per AC cycle.
//
//-----------------------------------------------------------------------------

int16_t chgr_isr_hist[0x08];
int16_t chgr_isr_hist_index = 0;
int8_t  chgr_resynch = 0;
int8_t  chgr_over_volt = 0;
int8_t  chgr_over_curr = 0;
int8_t  chgr_pos_neg_zc = 0; //
static int16_t chgr_pwm_isr_state = CHGR_PWM_INIT;

void _chgr_pwm_isr(int8_t reset)
{
    static uint16_t hist[4];
    uint16_t avg[3];
    static int16_t count = 0;
    static int16_t neg_count = 0;
    static int16_t pos_count = 0;
    static int16_t sine_table_index = 0;
	static int16_t last_state = 0;

	//	These macros are used for calculating peak current from the RMS
	//	values, to protect against over-current
	#define IMEAS_PEAK_ADC(y)	(IMEAS_INV_AMPS_ADC(Y) * SQRT_OF_TWO)
	
	//	This macro defines how large of an [opposite polarity] spike is 
	//	tolerated before we force a re-synch. Used in states 6 & 8 below.
	#define VAC_SPIKE_TOLERANCE	VAC_VOLTS_ADC(50)
    
    if(reset)
    {
		//	sigcap_reset();
		_comp_q16 = 0;
        chgr_pwm_isr_state = CHGR_PWM_INIT;
        return;
    }

	//   Safety Precaution: ALWAYS Turn Off High_Side FETs
    OVDCONbits.POUT1H = 0;      //  PWM1H = 0 --> Drive output is LOW
    OVDCONbits.POVD1H = 0;      //  Override: 1H controlled by POUT1H
    
    OVDCONbits.POUT2H = 0;      //  PWM2H = 0 --> Drive output is LOW
    OVDCONbits.POVD2H = 0;      //  Override: 2H controlled by POUT1H

  #if !defined (OPTION_IGNORE_OVERLOAD)	
  
	//	TODO: Rename #defines as 'CHGR_' 
	//	protect against over-voltage
	#if   IS_DCIN_12V
  	  #define INV_VBATT_LIMIT	(VBATT_VOLTS_ADC(18.0)) //17.0
	#elif IS_DCIN_24V
	  #define INV_VBATT_LIMIT	(VBATT_VOLTS_ADC(34.5)) //34.0
	#elif IS_DCIN_48V
	  #define INV_VBATT_LIMIT	(VBATT_VOLTS_ADC(68.0))
	#elif IS_DCIN_51V
	  #define INV_VBATT_LIMIT	(VBATT_VOLTS_ADC(63.0))
	#else
	  #error 'OPTION_DCIN' is not defined!
	#endif

	if(VBattCycleAvg() > INV_VBATT_LIMIT)
	{
		chgr_over_volt = 1;
		_comp_q16 = 0;
	}
    
    //  protect against over-current
    if(_chgr_oc_detect())
    {
		//	sigcap_trigger();
		
		chgr_over_curr = 1;
		_comp_q16 = 0;
	}
  #endif	//  OPTION_IGNORE_OVERLOAD 
    

//	DEBUG - start
    if(last_state != chgr_pwm_isr_state)
    {
        last_state = chgr_pwm_isr_state;
//		_ToggleTP(chgr_pwm_isr_state+1);

		chgr_isr_hist[chgr_isr_hist_index++] = chgr_pwm_isr_state;
		chgr_isr_hist_index &= 0x07;
	}
//	DEBUG - end
	
		
	//	Shape the profile of the PWM Duty cycle as an inverted sine-wave.
	//
	//	These calculations use Q16 values for fixed-point math computations. 
	//	The macro 'Q16' provides an easy way for expressing these values.  For 
	//	example the macro below sets a GAIN1_Q16 to a value of 1.234
	//			#define GAIN1_Q16     Q16(1, .234)
	//
	#define GAIN1_Q16     Q16(0, .50)

	temp32.msw = 0;
	//	The fixed sine-table is used for calculating shaping the PWM waveform.
	//	To accommodate varying frequencies, specifically lower-frequency, means
	//	more PWM clocks than table, we need to check the limits used here for
	//	this calculation.  If we exceed the limits, just use the [0] value, 
	//	which should be zero in all tables.
	if(sine_table_index >= MAX_SINE)
	{
		temp32.lsw = 0;
	}
	else
	{
		temp32.lsw = _SineTableQ16[sine_table_index];
	}
	
		//	Shape the profile of the PWM Duty cycle as an inverted sine-wave.
	//	Calculate Inverse of the sine table (subtract from 1)
	temp32.lsw = ~temp32.lsw;
	temp32.msw = 0;

	//	Multiply by the gain
//	temp32.dword *= (_comp_q16 >> 1);
	temp32.dword *= _comp_q16;
	//	Move product to temp32.lsw
	temp32.lsw = temp32.msw;	
	temp32.msw = 0;
	
	//	Add the offset
//	temp32.dword += (uint32_t)(_comp_q16 >> 1);	
	temp32.dword += (uint32_t)_comp_q16;	
	
	//	Multiply by the Max Duty Cycle
	temp32.dword *= MAX_DUTY;
	
	//	product is in temp21.msw
	curr_duty.msw = temp32.msw;

    //  Constrain the Duty Cycle ----------------------------------------------
    
    //  The Charger Minimum Duty Cycle is 5%
    #define CHRG_MIN_DUTY   ((int16_t)(MAX_DUTY * 0.05))
    //   The Charger Maximum Duty Cycle is 95%
    #define CHRG_MAX_DUTY   ((int16_t)(MAX_DUTY * 0.95))

    if (curr_duty.msw > CHRG_MAX_DUTY)
    {
        //  Limit charger to CHRG_MAX_DUTY;
        curr_duty.msw = CHRG_MAX_DUTY;
    }
    else if (curr_duty.msw < CHRG_MIN_DUTY)
    {
        //  Turn charger off.
        curr_duty.msw = 0;
    }

	//	save _duty_cycle at peak in sine table
	if(sine_table_index == 96)	_duty_cycle = curr_duty.msw;
	
	//	DEBUG - uncomment the following line to disable charger PWM output
	//    curr_duty.msw = 0;
   
	//	sigcap_save_data();
   
    //  Update VAC History for detecting zero-crossing
    hist[3] = hist[2];
    hist[2] = hist[1];
    hist[1] = hist[0];
    hist[0] = VacRaw();
   
    avg[2] = ((hist[3] + hist[2]) >> 1);
    avg[1] = ((hist[2] + hist[1]) >> 1);
    avg[0] = ((hist[1] + hist[0]) >> 1);
	
    switch(chgr_pwm_isr_state)
    {
	//	Synchronization - initialize pos_count and neg_count
	//	ZcThreshold is initialized to the mid-point of the ADC readings
	//	
	#define AC_FREQ_HZ_MIN		(50)    //55
	#define AC_FREQ_HZ_MAX		(70)    //65
	#define PHASE_COUNT_MIN		(FPWM/(2*AC_FREQ_HZ_MAX))		//	177 (65Hz)
	#define PHASE_COUNT_MAX		(FPWM/(2*AC_FREQ_HZ_MIN))		//	209 (55Hz)
	
	case CHGR_PWM_INIT:	//	Initial - wait for first positive-going zero-crossing
//		#if IS_PCB_LPC
//			DEBUG_TP15_HI();
//		#else
//			DEBUG_TP36_HI();
//		#endif
		//	sigcap_arm();
		chgr_resynch = 1;	//	chgr_resynch flag must be cleared by the driver
				
        SetChgrNotOutputting();

        // Override L1 Drivers to be OFF (Low)
        OVDCONbits.POUT1L = 0;      //  PWM1L = OFF --> Drive output is LOW
        OVDCONbits.POVD1L = 0;      //  Override: 1L controlled by POUT1L

        // Override L2 Drivers to be OFF (Low)
        OVDCONbits.POUT2L = 0;      //  PWM2L = OFF --> Drive output is LOW
        OVDCONbits.POVD2L = 0;      //  Override: 2L controlled by POUT2L

		PDC1 = 0;
		PDC2 = 0;
		
        if((avg[2] < avg[1])      && (avg[1] < avg[0]) &&
           (avg[2] < ZcThreshold) && (avg[0] >= ZcThreshold))
        {
            //  This is the zero-crossing - from negative to positive.
            count = 0;
            chgr_pwm_isr_state = CHGR_PWM_POS_WAIT_NEG_1;
        }
        break;

    case CHGR_PWM_POS_WAIT_NEG_1:	//  in positive half-cycle - waiting for negative-going zero-crossing
				//	initialize pos_count with the number of PWM clocks in the positive half-cycle
        SetChgrNotOutputting();
		count++;
		
		if(avg[1] < ZcThreshold) 
		{
			//	test if we get an unexpected negative spike
			chgr_pwm_isr_state = CHGR_PWM_INIT;
		}
        else if((avg[2] > avg[1]) && (avg[1] > avg[0]) &&
                (avg[2] > ZcThreshold) && (avg[0] <= ZcThreshold))
        {
            //  This is the zero-crossing - from positive to negative.
            an_ProcessAnalogData();
			
			if((count < PHASE_COUNT_MIN) || (count > PHASE_COUNT_MAX))
			{
				chgr_pwm_isr_state = CHGR_PWM_INIT;
			}
			else
			{
				pos_count = count;
				count = 0;
				chgr_pwm_isr_state = CHGR_PWM_NEG_WAIT_POS_1;
			}
        }
        break;

		
	case CHGR_PWM_NEG_WAIT_POS_1:	//  in negative half-cycle, waiting for positive-going zero-crossing
				//	Initialize neg_count the number of PWM clocks in the negative half-cycle
        SetChgrNotOutputting();
		count++;
		
		if(avg[1] > ZcThreshold) 
		{
			//	test if we get an unexpected positive spike
			chgr_pwm_isr_state = CHGR_PWM_INIT;
		}
        else if((avg[2] < avg[1]) && (avg[1] < avg[0]) &&
                (avg[2] < ZcThreshold) && (avg[0] >= ZcThreshold))
        {
            //  This is the zero-crossing - from negative to positive.
			if((count < PHASE_COUNT_MIN) || (count > PHASE_COUNT_MAX))
			{
				chgr_pwm_isr_state = CHGR_PWM_INIT;
			}
			else
			{
				neg_count = count;
				count = 0;
				chgr_pwm_isr_state = CHGR_PWM_POS_WAIT_NEG_2;
			}
        }
        break;

		
    case CHGR_PWM_POS_WAIT_NEG_2:    //  in positive half-cycle
				//	Confirm the prior pos_count value (prior value is now stored in 'count')
        SetChgrNotOutputting();
		count++;
		
        if(avg[1] < ZcThreshold) 
		{
			//	test if we get an unexpected negative spike
			chgr_pwm_isr_state = CHGR_PWM_INIT;
		}
		else if((avg[2] > avg[1]) && (avg[1] > avg[0]) &&
                (avg[2] > ZcThreshold) && (avg[0] <= ZcThreshold))
        {
			//  This is the zero-crossing - from positive to negative.
			an_ProcessAnalogData();
			
			if((count < PHASE_COUNT_MIN) || (count > PHASE_COUNT_MAX) ||
				(abs(count - pos_count) > ZC_CONF_TIMEOUT))
			{
				//	restart if we are out of range or (too) different from prior half-cycle
				chgr_pwm_isr_state = CHGR_PWM_INIT;
			}
			else
			{
				//	We passed! - now validate the negative half
				pos_count = count;
				count = 0;
				chgr_pwm_isr_state = CHGR_PWM_NEG_WAIT_POS_2;
			}
        }
        break;

   
	case CHGR_PWM_NEG_WAIT_POS_2:	//  in negative half-cycle, waiting for positive-going zero-crossing
				//	Initialize neg_count the number of PWM clocks in the negative half-cycle
        SetChgrNotOutputting();
		count++;
		
		if(avg[1] > ZcThreshold) 
		{
			//	test if we get an unexpected positive spike
			chgr_pwm_isr_state = CHGR_PWM_INIT;
		}
        else if((avg[2] < avg[1]) && (avg[1] < avg[0]) &&
                (avg[2] < ZcThreshold) && (avg[0] >= ZcThreshold))
		{
            //  This is the zero-crossing - from negative to positive.
			if((count < PHASE_COUNT_MIN) || (count > PHASE_COUNT_MAX) ||
				(abs(count - neg_count) > ZC_CONF_TIMEOUT))
			{
				chgr_pwm_isr_state = CHGR_PWM_INIT;
			}
			else
			{
				//	we passed again! ... proceed
				neg_count = count;
				count = 0;
				_chgr_DutyReset();	//	reset duty-cycle so we soft-start.
				chgr_pwm_isr_state = CHGR_PWM_CONFIRM_NEG_POS;
			}
		}
		break;

	////	CHARGER OUTPUT ACTIVE 	///////////////////////////////////////////

    case CHGR_PWM_CONFIRM_NEG_POS:	//	Confirm the Negative-to-Positive zero-crossing
//		#if IS_PCB_LPC
//			DEBUG_TP15_LO();
//		#else
//			DEBUG_TP36_LO();
//		#endif
		
        SetChgrOutputting();

        // Override L1 Drivers to be OFF (Low)
        OVDCONbits.POUT1L = 0;      //  PWM1L = OFF --> Drive output is LOW
        OVDCONbits.POVD1L = 0;      //  Override: 1L controlled by POUT1L

        // Override L2 Drivers to be OFF (Low)
        OVDCONbits.POUT2L = 0;      //  PWM2L = OFF --> Drive output is LOW
        OVDCONbits.POVD2L = 0;      //  Override: 2L controlled by POUT2L

		if(++count > ZC_CONF_TIMEOUT)
		{
			//	We have exceeded the allowed tolerance of the zero-crossing
			chgr_pwm_isr_state = CHGR_PWM_INIT;		//	Re-Synch
		}
        else if((avg[2] < avg[1]) && (avg[1] < avg[0]) && (avg[2] >= ZcThreshold))
		{
			//	Zero-Cross Confirmed!  
				
			//	We are at, or have exceeded the zero-crossing: proceed!
			//	(avg[2] < avg[1]) && (avg[1] < avg[0])  --> rising slope
			//	avg[2] is the latest average sample.
			//	(avg[2] >= ZcThreshold)	--> positive half-cycle
			
			sine_table_index = 0;
			chgr_pwm_isr_state = CHGR_PWM_POS_DETECT_OC;
		}
		break;
		
    case CHGR_PWM_POS_DETECT_OC:    //  We are in the positive half-cycle
	
		//	Detect over-current	
		//		if((IMeasRaw()-VAC_ZERO_CROSS_A2D) > IMEAS_PEAK_ADC(IMEAS_PEAK_LIMIT)) 
		//		{
		//			_comp_gain = PDC_10;	
		//		}
	
		sine_table_index++;
		++count;
		if(count > (pos_count + ZC_DET_WIN_END))
		{
			//	we have not seen the zero-crossing within the budgeted time
			//	Re-synch!
			// Override L1 Drivers to be OFF (Low)
			OVDCONbits.POUT1L = 0;      //  PWM1L = OFF --> Drive output is LOW
			OVDCONbits.POVD1L = 0;      //  Override: 1L controlled by POUT1L
			
			// Override L2 Drivers to be OFF (Low)
			OVDCONbits.POUT2L = 0;      //  PWM2L = OFF --> Drive output is LOW
			OVDCONbits.POVD2L = 0;      //  Override: L2 controlled by POUT2L
			
			chgr_pwm_isr_state = CHGR_PWM_INIT;
		}
		else if(VacRaw() < ZcThreshold)
		{
			//	We have seen the transition - confirm in the next state
			// Override L1 Drivers to be OFF (Low)
			OVDCONbits.POUT1L = 0;      //  PWM1L = OFF --> Drive output is LOW
			OVDCONbits.POVD1L = 0;      //  Override: 1L controlled by POUT1L
			
			// Override L2 Drivers to be OFF (Low)
			OVDCONbits.POUT2L = 0;      //  PWM2L = OFF --> Drive output is LOW
			OVDCONbits.POVD2L = 0;      //  Override: L2 controlled by POUT2L
			
			PDC1 = 0;
			
			if((count < PHASE_COUNT_MIN) || (count > PHASE_COUNT_MAX) ||
			   (count < (pos_count - ZC_DET_WIN_START)))
			{
				//	this is a premature zero-crossing
				if(VacRaw() < (ZcThreshold - VAC_SPIKE_TOLERANCE))
				{
					//	Re-synch only if its a really big spike.
					chgr_pwm_isr_state = CHGR_PWM_INIT;
				}
			}
			else
			{
				//	This transition occurred where zero-crossing was expected.
				pos_count = count;	
				count = 0;
				chgr_pwm_isr_state = CHGR_PWM_CONFIRM_POS_NEG;
			}
		}
		else
		{
          #if !defined(DEBUG_CHG_DISABLE_DRIVES)
            // DEBUG - disable outputs until we confirm the zero-crossing.			
            //  Low Driver L1 controlled by PWM (Duty-Cycle)
            OVDCONbits.POVD1L = 1;      //  L1 controlled by PWM
            
            // Override the L2 Driver to be ON (High)
            OVDCONbits.POUT2L = 1;      //  PWM2L = ON --> Drive output is High
            OVDCONbits.POVD2L = 0;      //  Override: L2 controlled by POUT2L
            
            PDC1 = curr_duty.msw;
          #endif
		}
        break;


    case CHGR_PWM_CONFIRM_POS_NEG:	//	Confirm the Positive-to-Negative zero-crossing
        SetChgrOutputting();

        // Override L1 Drivers to be OFF (Low)
        OVDCONbits.POUT1L = 0;      //  PWM1L = OFF --> Drive output is LOW
        OVDCONbits.POVD1L = 0;      //  Override: 1L controlled by POUT1L

        // Override L2 Drivers to be OFF (Low)
        OVDCONbits.POUT2L = 0;      //  PWM2L = OFF --> Drive output is LOW
        OVDCONbits.POVD2L = 0;      //  Override: 2L controlled by POUT2L

		if(++count > ZC_CONF_TIMEOUT)
		{
			//	We have exceeded the allowed tolerance of the zero-crossing
			chgr_pwm_isr_state = CHGR_PWM_INIT;		//	Re-Synch
		}
        else if((avg[2] > avg[1]) && (avg[1] > avg[0]) && 
			    (avg[2] <= ZcThreshold))
		{
			//	Zero-Cross Confirmed!
            an_ProcessAnalogData();
			
			chgr_pos_neg_zc = 1;	//	this flag is cleared in _chgr_Driver
			
			sine_table_index = 0;
			chgr_pwm_isr_state = CHGR_PWM_NEG_DECT_OC;
		}
		break;

    case CHGR_PWM_NEG_DECT_OC:    //  We are in the negative half-cycle
		//	Detect over-current	
		//		if((VAC_ZERO_CROSS_A2D - IMeasRaw()) > IMEAS_PEAK_ADC(IMEAS_PEAK_LIMIT))
		//		{
		//			_comp_gain = PDC_10;	
		//		}
		
		sine_table_index++;
		if(++count > (neg_count + ZC_DET_WIN_END))
		{
			//	we have not seen the zero-crossing within the budgeted time
			//	Re-synch!
			// Override L1 Drivers to be OFF (Low)
			OVDCONbits.POUT1L = 0;      //  PWM1L = OFF --> Drive output is LOW
			OVDCONbits.POVD1L = 0;      //  Override: 1L controlled by POUT1L
			
			// Override L2 Drivers to be OFF (Low)
			OVDCONbits.POUT2L = 0;      //  PWM2L = OFF --> Drive output is LOW
			OVDCONbits.POVD2L = 0;      //  Override: L2 controlled by POUT2L
			
			chgr_pwm_isr_state = CHGR_PWM_INIT;
		}
		else if(VacRaw() > ZcThreshold)
		{
			//	We have seen the transition - confirm in the next state
			// Override L1 Drivers to be OFF (Low)
			OVDCONbits.POUT1L = 0;      //  PWM1L = OFF --> Drive output is LOW
			OVDCONbits.POVD1L = 0;      //  Override: 1L controlled by POUT1L
			
			// Override L2 Drivers to be OFF (Low)
			OVDCONbits.POUT2L = 0;      //  PWM2L = OFF --> Drive output is LOW
			OVDCONbits.POVD2L = 0;      //  Override: L2 controlled by POUT2L
			
			PDC2 = 0;
			
			if((count < PHASE_COUNT_MIN) || (count > PHASE_COUNT_MAX) ||
			   (count < (neg_count - ZC_DET_WIN_START)))
			{
				//	This is a premature zero-crossing
				if (VacRaw() > (ZcThreshold + VAC_SPIKE_TOLERANCE))
				{
					//	Re-synch only if its a really big spike.
					chgr_pwm_isr_state = CHGR_PWM_INIT;
				}
			}
			else
			{
				//	This transition occurred where zero-crossing was expected.
				neg_count = count;	
				count = 0;
				chgr_pwm_isr_state = CHGR_PWM_CONFIRM_NEG_POS;
			}
		}
		else
		{
          #if !defined(DEBUG_CHG_DISABLE_DRIVES)
            // DEBUG - disable outputs until we confirm the zero-crossing.			
            //  Low Driver L2 controlled by PWM (Duty-Cycle)
            OVDCONbits.POVD2L = 1;      //  L2 controlled by PWM
            
            // Override the L1 Driver to be ON (High)
            OVDCONbits.POUT1L = 1;      //  PWM1L = ON --> Drive output is High
            OVDCONbits.POVD1L = 0;      //  Override: L1 controlled by POUT1L
            
            PDC2 = curr_duty.msw;
          #endif
		}
        break;
   
    default :
        SetChgrNotOutputting();

		//	We should never get here, but just-in-case... disable outputs.
        // Override L1 Drivers to be OFF (Low)
        OVDCONbits.POUT1L = 0;      //  PWM1L = OFF --> Drive output is LOW
        OVDCONbits.POVD1L = 0;      //  Override: 1L controlled by POUT1L

        // Override L2 Drivers to be OFF (Low)
        OVDCONbits.POUT2L = 0;      //  PWM2L = OFF --> Drive output is LOW
        OVDCONbits.POVD2L = 0;      //  Override: 2L controlled by POUT2L
		
		PDC1 = 0;
		PDC2 = 0;
		
        chgr_pwm_isr_state = CHGR_PWM_INIT;  // re-initialize
        break;
    }
}

//-----------------------------------------------------------------------------
//  chgr_PwmIsr()
//-----------------------------------------------------------------------------
//  chgr_PwmIsr is called from the PWM ISR when Chgr.status.chgr_active is set.
//
//  chgr_PwmIsr calls the static function _chgr_pwm_isr() with the reset flag
//  cleared.
//-----------------------------------------------------------------------------

void chgr_PwmIsr(void)
{
    _chgr_pwm_isr(0);
}

////	DEBUG	///////////////////////////////////////////////////////////////////
//
////-----------------------------------------------------------------------------
////  Configure T7 Timer
////
////	Timer7 is used to generate a pulsed output on a test point for debug.
////	In this case T7 is used to pulse TP51 corresponding to the state of 
////	Charger-ISR so that we can identify the state on an oscilloscope.
////
////  Initialize Timer7 for a 25-microsecond interrupt
////  The timer module is a 16-bit timer/counter affected by the following
////  registers:
////      TMRx: 16-bit timer count register
////      PRx: 16-bit period register associated with the timer
////      TxCON: 16-bit control register associated with the timer
////
////  FCY = (FOSC * PLL) = (8MHz * 16) = 128MHz
////  PR1 = 1-mSec/(TCY*Prescale)
////-----------------------------------------------------------------------------
//
//void chgr_T7Config(void)
//{
//  #ifndef WIN32
//    T7CONbits.TON = 0;          // Disable Timer1
//
//    // TCS: Timer Clock Source Select bit
//    //      1 = External clock from pin TxCK
//    //      0 = Internal clock (Tcy)
//    T7CONbits.TCS = 0;
//
//    T7CONbits.TGATE = 0;        // Disable Gated Timer mode
//
//    // TCKPS<1:0>: Timer Input Clock Prescale Select bits
//    //      11 = 1:256 prescale value
//    //      10 = 1:64 prescale value
//    //      01 = 1:8 prescale value
//    //      00 = 1:1 prescale value
//    T7CONbits.TCKPS = 0b01;
//
//    //  Clear the timer register (Timer1)
//    TMR7 = 0;
//    //  PR7: 16-bit period register for Timer-1
////    PR7 = 500;  //  100 microseconds
////    PR7 = 250;  //  50 microseconds
////    PR7 = 125;  //  25 microseconds
//    PR7 = 50;  //  10 microseconds
//    
//  #endif
//}
//
////-----------------------------------------------------------------------------
////  Start the T7 Timer
////-----------------------------------------------------------------------------
//
//void chgr_T7Start(void)
//{
//    IFS3bits.T7IF = 0;      // Clear Timer1 Interrupt Flag
//    IEC3bits.T7IE = 1;      // Enable Interrupts for Timer1
//
//    T7CONbits.TON = 1;      // turn on timer 1
//}
//
//
////-----------------------------------------------------------------------------
////  25-microsecond timer interrupt.
////-----------------------------------------------------------------------------
//
//
//static int16_t _toggle_count = 0;
//
//#if IS_PCB_LPC
//	#define DEBUG_TP_HI	DEBUG_TP18_HI
//	#define DEBUG_TP_LO	DEBUG_TP18_LO
//#else
//	#define DEBUG_TP_HI DEBUG_TP51_HI
//	#define DEBUG_TP_LO DEBUG_TP51_LO
//#endif
//
//static void _ToggleTP(int16_t count)
//{
//    _toggle_count = count;
//}
//
//void __attribute__((interrupt, no_auto_psv)) _T7Interrupt (void)
//{
//    static int16_t state = 0;
//    static int16_t my_count = 0;
//
//  
//    switch(state)
//    {
//        case 0 :
////        	DEBUG_TP_LO();
//            if(_toggle_count > 0)
//            {
//                my_count = _toggle_count;
//                _toggle_count = 0;
//                state++;
//            }
//            break;
//            
//        case 1 :
////           	DEBUG_TP_HI();
//            state++;
//            break;
//            
//        case 2 :
////           	DEBUG_TP_LO();
//            if(--my_count <= 0) state = 0;
//            else state--;
//            break;
//            
//        default :
//            state = 0;
//            break;
//    }
//    IFS3bits.T7IF = 0;  // end-of-interrupt
//}
//
//////	DEBUG	///////////////////////////////////////////////////////////////




///////////////////////////////////////////////////////////////////////////////
//	#if IS_PCB_LPC
#if 0
////-----------------------------------------------------------------------------
////  _INT0Config
////-----------------------------------------------------------------------------
////  Allow interrupts from the /INT0 pin.  In this application, this is the OVL
////  input.
////
////-----------------------------------------------------------------------------
// static void _INT0Config(void)
// {
//    DEBUG_TP18_LO();
//
//    //  INT0EP: External Interrupt #0 Edge Detect
//    //  Polarity Select bit:
//    //      1 = Interrupt on negative edge
//    //      0 = Interrupt on positive edge
//    INTCON2bits.INT0EP = 1;
//    return;
// }
//-----------------------------------------------------------------------------
static int16_t _int0_occurred = 0;

//-----------------------------------------------------------------------------
//  _INT0Start
//-----------------------------------------------------------------------------
//  Allow interrupts from the /INT0 pin.  In this application, this is the OVL
//  input.
//  
//  A problem exists if the [negative] edge occurs while we are configuring 
//  for it, so a do .. while loop is used.
//-----------------------------------------------------------------------------
static void _INT0Start(void)
{
    
    //  Enable interrupts from INT0
    _int0_occurred = 0;
    IFS0bits.INT0IF = 0;        // Clear the INT0 Interrupt Flag
    INTCON2bits.INT0EP = 1;     // setup INT0 to interrupt on negative edge
    IEC0bits.INT0IE = 1;        // Enable INT0 Interrupts
    
    #define INT0_ACTIVE()   (0 == PORTFbits.RF6)
    //  Read the INT0/RF6 input pin.  If the signal is already low and the 
    //  int0 interrupt has not happened, then configure for the positive edge.
    if((0 == PORTFbits.RF6) && !_int0_occurred)
    {
        //  the input is low
        INTCON2bits.INT0EP = 0;         //  setup INT0 to interrupt on positive edge
        if(chgr_pwm_isr_state == CHGR_PWM_NEG_WAIT_POS_1) 
        {
            //  in positive half-cycle, Driving L1 - Override the H1 Driver to be ON (High)
            OVDCONbits.POUT1H = 1;      //  PWM1H = ON --> Drive output is HIGH
            OVDCONbits.POVD1H = 0;      //  Override: 1H controlled by POUT1H
        }
        else if(chgr_pwm_isr_state == CHGR_PWM_NEG_WAIT_POS_2) 
        {
            //  in negative half-cycle, Driving L2 - Override the H2 Driver to be OFF (High)
            OVDCONbits.POUT2H = 1;      //  PWM2H = ON --> Drive output is HIGH
            OVDCONbits.POVD2H = 0;      //  Override: 2H controlled by POUT1H
        }
        else
        {
            //  I don't expect to get here but just in case, turn off 1H and 2H 
            OVDCONbits.POUT1H = 0;      //  PWM1H = OFF --> Drive output is LOW
            OVDCONbits.POVD1H = 0;      //  Override: 1H controlled by POUT1H
            
            OVDCONbits.POUT2H = 0;      //  PWM2H = OFF --> Drive output is LOW
            OVDCONbits.POVD2H = 0;      //  Override: 2H controlled by POUT1H
        }
    }
}

//-----------------------------------------------------------------------------
//  _INT0Stop
//-----------------------------------------------------------------------------
//  Prevent interrupts from the /INT0 pin.
//-----------------------------------------------------------------------------

static void _INT0Stop(void)
{
    IFS0bits.INT0IF = 0;        // Clear the INT0 Interrupt Flag
    IEC0bits.INT0IE = 0;        // Disable INT0 Interrupts

    //  Assure that Hi-FETs are off
    OVDCONbits.POUT1H = 0;      //  PWM1H = OFF --> Drive output is LOW
    OVDCONbits.POVD1H = 0;      //  Override: 1H controlled by POUT1H
        
    OVDCONbits.POUT2H = 0;      //  PWM2H = OFF --> Drive output is LOW
    OVDCONbits.POVD2H = 0;      //  Override: 2H controlled by POUT1H
}

//-----------------------------------------------------------------------------
//  _INT0Interrupt()
//-----------------------------------------------------------------------------
//  Overload Signal --> INT2 Interrupt
//-----------------------------------------------------------------------------

void __attribute__((interrupt, no_auto_psv)) _INT0Interrupt (void)
{
    _int0_occurred = 1;
    IFS0bits.INT0IF = 0;        // Clear the INT0 Interrupt Flag
    IEC0bits.INT0IE = 1;        // Enable INT0 Interrupts
//    DEBUG_RB7_HI();

    //  INT0EP: External Interrupt #0 Edge Detect
    //      0 = Interrupt on positive edge
    //      1 = Interrupt on negative edge
    if (1 == INTCON2bits.INT0EP) {
        //  Interrupted on the negative edge
        INTCON2bits.INT0EP = 0;         //  setup for positive edge
        if(chgr_pwm_isr_state == CHGR_PWM_NEG_WAIT_POS_1) 
        {
            //  in positive half-cycle, Driving L1 - Override the H1 Driver to be ON (High)
            OVDCONbits.POUT1H = 1;      //  PWM1H = ON --> Drive output is HIGH
            OVDCONbits.POVD1H = 0;      //  Override: 1H controlled by POUT1H
        }
        else if(chgr_pwm_isr_state == CHGR_PWM_NEG_WAIT_POS_2) 
        {
            //  in negative half-cycle, Driving L2 - Override the H2 Driver to be OFF (High)
            OVDCONbits.POUT2H = 1;      //  PWM2H = ON --> Drive output is HIGH
            OVDCONbits.POVD2H = 0;      //  Override: 2H controlled by POUT1H
        }
        else
        {
            //  I don't expect to get here but just in case, turn off 1H and 2H 
            OVDCONbits.POUT1H = 0;      //  PWM1H = OFF --> Drive output is LOW
            OVDCONbits.POVD1H = 0;      //  Override: 1H controlled by POUT1H
            
            OVDCONbits.POUT2H = 0;      //  PWM2H = OFF --> Drive output is LOW
            OVDCONbits.POVD2H = 0;      //  Override: 2H controlled by POUT1H
        }
    }
    else {
        //  Interrupted on the positive edge
        INTCON2bits.INT0EP = 1;         //  setup for negative edge
        if(chgr_pwm_isr_state == CHGR_PWM_NEG_WAIT_POS_1) 
        {
            //  in positive half-cycle, Driving L1 - Override the H1 Driver to be OFF (Low)
            OVDCONbits.POUT1H = 0;      //  PWM1H = OFF --> Drive output is LOW
            OVDCONbits.POVD1H = 0;      //  Override: 1H controlled by POUT1H
        }
        else if(chgr_pwm_isr_state == CHGR_PWM_NEG_WAIT_POS_2) 
        {
            //  in negative half-cycle, Driving L2 - Override the H2 Driver to be OFF (Low)
            OVDCONbits.POUT2H = 0;      //  PWM2H = OFF --> Drive output is LOW
            OVDCONbits.POVD2H = 0;      //  Override: 2H controlled by POUT1H
        }
        else
        {
            //  I don't expect to get here but just in case, turn off 1H and 2H 
            OVDCONbits.POUT1H = 0;      //  PWM1H = OFF --> Drive output is LOW
            OVDCONbits.POVD1H = 0;      //  Override: 1H controlled by POUT1H
            
            OVDCONbits.POUT2H = 0;      //  PWM2H = OFF --> Drive output is LOW
            OVDCONbits.POVD2H = 0;      //  Override: 2H controlled by POUT1H
        }
    }
//    DEBUG_RB7_LO();
    return;
}

#endif	//	IS_PCB_LPC

// <><><><><><><><><><><><><> charger_isr.c <><><><><><><><><><><><><><><><><><>
