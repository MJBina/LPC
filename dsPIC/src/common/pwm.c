// <><><><><><><><><><><><><> pwm.c <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//  Copyright(C) 2017 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
//  Pulse-Width-Modulation driver
//
//  OVERRIDE:
//      
//  Configure the Override Control bits
//  The override control (POVD) bits are active-low. When the POVD bit is
//  cleared, the output of the corresponding PWM I/O pin is determined by 
//  the POUT bit. When the POVD bit is set, the corresponding PWM I/O pin 
//  is controlled by the PWM module.  
//
//  PWM3 is not used, outputs are configured as General Purpose I/O. 
//  PWM4 is not used, outputs are configured as General Purpose I/O. 
//
//  PROCESSOR REGISTERS/FLAGS:
//      
//  Variants of the dsPIC processor may implement some flags in different 
//  control registers.  Microchip provides macros to handle these variations.
//  In general, it is preferred to use the entire register name, since that 
//  facilitates debug.  However, then we'd also need to accomodate processor
//  variants with conditional compiles which add clutter. An example is 
//  provided below.
//
//  This conditional: 
//    #if defined(__dsPIC33FJ128MC706A__) || defined(__dsPIC33FJ64MC506A__)
//        IFS3bits.PWMIF = 0;     // Clear interrupt flag
//        IEC3bits.PWMIE = 1;     //  Enable PWM Interrupts
//    #else
//        IFS2bits.PWMIF = 0;     // Clear interrupt flag
//        IEC2bits.PWMIE = 1;     //  Enable PWM Interrupts
//    #endif
//  ... has been refactored to:
//    _PWMIF = 0;     // Clear interrupt flag
//    _PWMIE = 1;     // Enable PWM Interrupts    
//
//-----------------------------------------------------------------------------

// -------
// headers
// -------
#include "options.h"    // must be first include
#include "analog.h"
#include "fan_ctrl.h"	// fan timer called from PWM ISR
#include "hw.h"
#include "inverter.h"
#include "pwm.h"
#include "sine_table.h"
#include "timer3.h"
#include "tasker.h"

// ------------
// Global Data
// ------------
int16_t ac_sync_pulse_count = 0;

// ----------
// local data
// ----------
static PWM_CONFIG_t _pwm_config = PWM_CONFIG_DEFAULT;
static void (*_pwm_isr)(void) = &pwm_DefaultIsr;


//-----------------------------------------------------------------------------
// default pwm interrupt service routine

void pwm_DefaultIsr(void)
{
    static int16_t sine_table_index = 0;
	
    //  The override control (POVD) bits are active-low. When the POVD bit 
    //  is cleared, the output of the corresponding PWM I/O pin is 
    //  determined by the POUT bit. When the POVD bit is set, the 
    //  corresponding PWM I/O pin is controlled by the PWM module.

    //  Always Override High-side drivers off.
    OVDCONbits.POUT1L = 0;      //  PWM1L = OFF
    OVDCONbits.POVD1L = 0;      //  Output 1L is controlled by POUT1L
    OVDCONbits.POUT1H = 0;      //  PWM1H = OFF
    OVDCONbits.POVD1H = 0;      //  Output 1H is controlled by POUT1H

    OVDCONbits.POUT2L = 0;      //  PWM2L = OFF
    OVDCONbits.POVD2L = 0;      //  Output 2L is controlled by POUT2L
    OVDCONbits.POUT2H = 0;      //  PWM2H = OFF
    OVDCONbits.POVD2H = 0;      //  Output 2H is controlled by POUT2H

	//-----------------------------------------------------------------------------
	//	The A2D was/is triggered from the PWM ISR.  If we simply stop the ISR, 
	//	then we stop reading the analog channels.  Also, the acquired data is 
	//	processed at the zero-crossing.  The intent of this default ISR is that
	//	the analog data is still acquired and processed even when the inverter
	//	or charger are not running.
	//-----------------------------------------------------------------------------
	
    if (++sine_table_index > ((2*MAX_SINE)-1))
	{
		sine_table_index = 0;
        an_ProcessAnalogData();
	}
    return;
}

//-----------------------------------------------------------------------------
//  _ac_line_synchronize
//-----------------------------------------------------------------------------
//
//  This function will monitor the AC-Line Synch signal so that the inverter can 
//  start sychronous to the AC-Line.
//
//  The incoming AC_LINE_PHASE signal is not a 50% duty-cycle.  
//
//  The AC_LINE_PHASE signal is generated from the incoming AC-line through an 
//  opto-coupler.  The positive-going half of the sine-wave turns on the photo-
//  diode (LED) which turns on the photo transistor pulling the output low.
//
//  Forward voltage drop across the opto-coupler's LED results in a slight delay
//  for turn-on and premature turn off.  This results in a somewhat shorter 
//  negative half to the AC_LINE_PHASE signal.
//
//  In theory, these discrepancies are stable. Applying a delay to the premature 
//  turn-off is more reliable than predicting the delayed turn-on. Therefore, we
//  synchronize to the falling edge of the AC_LINE_PHASE signal.  
//
//  The falling edge of the AC_LINE_PHASE signal corresponds to the positive to 
//  negative transition of the incoming AC sine wave.
//
//  The exact zero-crossing is offset from this signal by 1/2 the difference 
//  between the positive and negative halves of the AC_LINE_PHASE.
//-----------------------------------------------------------------------------


static void _ac_line_synchronize(void)
{
    static int16_t state = 0;

	//-----------------------------------------------------------------------------
    //  We need to detect phase even if the AC Signal is NOT (No longer) 
    //  qualified. For example, the charger is running and we lose AC power. 
    //  In that case we should start within one AC-cycle.
    //  We also have to prevent overflow of the ac_sync_pulse_count to prevent
    //  false triggers.
	//-----------------------------------------------------------------------------

    //  One AC-cycle = (MAX_SINE * 2)
    if(ac_sync_pulse_count < (MAX_SINE * 3))
    {
        ac_sync_pulse_count++;
    }
    else if(!AC_LINE_VALID())    
    {
        state = 0;
        return;
    }
  
  
    switch(state)
    {
    default:
    case 0 :    // determine where we are relative to the PHASE signal
        if(0 == PORTCbits.RC13)
        {
            //  we are in the positive half
            state = 2;
        }
        else 
        {
            //  we are in the negative half
            state = 1;
        }
        break;
    
    case 1 :    // negative half of PHASE signal
        if(0 == AC_LINE_PHASE()) 
        {
            //  This is falling-edge transition of the PHASE Signal, which  
            //  corresponds to the negative-to-positive transition of the 
            //  AC Signal, which we will call zero-degrees.

            //  Synchronize to this edge!
            ac_sync_pulse_count = 0;    //  reset the pulse count
            
            state = 2;
        }
        break;
    
    case 2 :    // positive half of PHASE signal
        if(1 == AC_LINE_PHASE()) 
        {
            state = 1;
        }
        break;
    } // switch state
}

//-----------------------------------------------------------------------------
//  _PWMInterrupt()
//-----------------------------------------------------------------------------
//  _PWMInterrupt is the default name for the PWM ISR.
//
//  The ADC requires that the PWM is running. The ADC reads 4-inputs each time
//  it is triggered. The ADC interrupts after 2-sets of readings (2x4 = 8).
//  The sequence is as follows:
//   1) The PWM ISR starts Timer3
//   2) Timer3 expires
//      2a) ADC10 starts conversion (MuxA inputs)
//      2b) Timer3 restarts itself
//   3) Timer3 expires - ADC10 starts conversion (MuxB inputs)
//   4) ADC10 completes conversions
//
//	TIMING:
//	2016-03-30 - Instrumentation was added to measure execution times below:
//		PWM ISR Period:  			                43.3 microseconds
//		PWM ISR Execution time - Inverter mode:  	10.3 microseconds
//		PWM ISR Execution time - Charger mode:  	 6.2 microseconds
//
//  2016-04-01 - Repeated timing test with Optimization 1
//		PWM ISR Execution time - Inverter mode:  	 8.5 microseconds
//		PWM ISR Execution time - Charger mode:  	 4.2 microseconds
//-----------------------------------------------------------------------------

void __attribute__((interrupt, no_auto_psv)) _PWMInterrupt (void)
{
    T3_Start();      //  Timer3 is used to trigger ADC

    _ac_line_synchronize();

  #ifdef  ENABLE_TASK_TIMING
    g_fanTiming.count++; // one more isr
    TMR7 = 0;  // clear timer
  #endif
 
    fan_Timer(); // perform fan timing for duty cycle

  #ifdef  ENABLE_TASK_TIMING
    {
    uint16_t ticks = TMR7;
    g_fanTiming.tsum += ticks;  // add to sum
    if (ticks < g_fanTiming.tmin) g_fanTiming.tmin = ticks;  // min value
    if (ticks > g_fanTiming.tmax) g_fanTiming.tmax = ticks;  // max value
    g_pwmTiming.count++; // one more isr
    }
  #endif

    _pwm_isr();

 #ifdef  ENABLE_TASK_TIMING
    uint16_t ticks = TMR7;
    g_pwmTiming.tsum += ticks;  // add to sum
    if (ticks < g_pwmTiming.tmin) g_pwmTiming.tmin = ticks;  // min value
    if (ticks > g_pwmTiming.tmax) g_pwmTiming.tmax = ticks;  // max value
    g_pwmTiming.count++; // one more isr
  #endif
   
    _PWMIF = 0;  // Clear interrupt flag    
    
    return;
}

//-----------------------------------------------------------------------------
//  _pwm_config_default()
//-----------------------------------------------------------------------------
//  This is a special configuration to assure all outputs are off on startup.
//-----------------------------------------------------------------------------

static void _pwm_config_default(void)
{
    #define MAX_DUTY    (((FCY/FPWM) - 1) * 2)
    
    //  Disable Timebase
    PTCONbits.PTEN = 0;     
    
    //  Initialize Duty Cycle registers: 
    
    //  PWM Timebase setup
    PTCONbits.PTCKPS = 0;       //  PWM Time Base Prescalar
                                //  00 = 1:1

    //  Computed Period based on CPU speed and desired PWM frequency ( > 20KHz)
    //  Refer to "sine_table.h" for details.
    PTPER = PTPER_INIT_VAL; 
                                
    //  Initialize Duty Cycle registers: Setting the duty cycle registers to 
    //  zero, turns off the PWM output(s).
    PDC1 = 0;      
    PDC2 = 0;       
    
    //  OVERRIDE SYNCHRONIZATION:
    //  If OSYNC is set, all output overrides performed via the OVDCON register
    //  are synchronized to the PWM time base. For the Edge-Aligned mode, 
    //  overrides are synchronized to PTMR == 0. 
    //
    //  We want override to take effect immediatly, so we do not want it to be 
    //  synchronized with the PWM time base.
    PWMCON2bits.OSYNC = 0;

    //  PWM Output Override
    OVDCONbits.POUT1L = 0;  //  PWM1L = OFF
    OVDCONbits.POVD1L = 0;  //  Output 1L is controlled by POUT1L
    OVDCONbits.POUT1H = 0;  //  PWM1H = OFF
    OVDCONbits.POVD1H = 0;  //  Output 1H is controlled by POUT1H

    OVDCONbits.POUT2L = 0;  //  PWM2L = OFF
    OVDCONbits.POVD2L = 0;  //  Output 2L is controlled by POUT2L
    OVDCONbits.POUT2H = 0;  //  PWM2H = OFF
    OVDCONbits.POVD2H = 0;  //  Output 2H is controlled by POUT2H

    _PWMIF = 0;     // Clear interrupt flag
    _PWMIE = 0;     // Disable PWM Interrupts
}

//-----------------------------------------------------------------------------
//  _pwm_config_boot_charge()
//-----------------------------------------------------------------------------
//  This is a special configuration for charging the bulk caps of the inverter.
//  It will pulse one low-side driver (PWM1L).
//  It is intended that this function be invoked only once after power-up. 
//  The ISR will terminate the output after a fixed number of cycles.
//-----------------------------------------------------------------------------

static void _pwm_config_boot_charge(void)
{
    #define MAX_DUTY    (((FCY/FPWM) - 1) * 2)
    
    //  Disable Timebase
    PTCONbits.PTEN = 0;     
    
    PWMCON1bits.PMOD1 = 1;      // independent outputs
    PWMCON1bits.PEN1H = 0;
    PWMCON1bits.PEN1L = 1;      // PWM1L (pin-38) is enabled for PWM output
    
    //  PWM Timebase setup
    PTCONbits.PTCKPS = 0;       //  PWM Time Base Prescalar
                                //  00 = 1:1
    // Computed Period based on CPU speed and desired PWM frequency ( > 20KHz)
    
    //  PTPER: PWM Time Base Period register - refer to sine_table.h
    PTPER = PTPER_INIT_VAL;
    
    //  Initialize Duty Cycle registers: 
    //  Setting the duty cycle registers to zero, turns off the PWM output.
    //  Off means the output is high.
    PDC1 = (MAX_DUTY * .15);       //   15% high / 85% low
    
    //  Edge-aligned PWM signals are produced by the module when the PWM time
    //  base is in the Free-Running or Single-Shot mode.
    PTCONbits.PTMOD = 0;    //  PTMOD:  00 = Free-Running

    
    //  OVERRIDE SYNCHRONIZATION:
    //  If OSYNC is set, all output overrides performed via the OVDCON register
    //  are synchronized to the PWM time base. For the Edge-Aligned mode, 
    //  overrides are synchronized to PTMR == 0. 
    //
    //  We want override to take effect immediatly, so we do not want it to be 
    //  synchronized with the PWM time base.
    PWMCON2bits.OSYNC = 0;

    //  PWM1L configured to pulse the L1 FET
    OVDCONbits.POVD1L = 1;  //  Output 1L is controlled by PWM1
    
    //  PWM Output Override configuration
    //  OVDCON = 0x3000;    //  (Override) All FETS off
    OVDCONbits.POUT1H = 0;  //  PWM1H = OFF
    OVDCONbits.POVD1H = 0;  //  Output 1H is controlled by POUT1H

    OVDCONbits.POUT2L = 0;  //  PWM2L = OFF
    OVDCONbits.POVD2L = 0;  //  Output 2L is controlled by POUT2L
    OVDCONbits.POUT2H = 0;  //  PWM2H = OFF
    OVDCONbits.POVD2H = 0;  //  Output 2H is controlled by POUT2H

    _PWMIF = 0;     // Clear interrupt flag
    _PWMIE = 0;     // Disable PWM Interrupts
}

//-----------------------------------------------------------------------------
//  _pwm_config_inverter()
//-----------------------------------------------------------------------------
//
//  Outputs are set and cleared, dependent upon where we are in the AC cycle.
//  The labels of the H-Bridge on the Sensata schematics are different than 
//  those used by Microchip.  To avoid confusion, comments in firmware will 
//  refer to the Microchip labels, except as noted.
//
//      +---------------+-------------------+-------------------+
//      | AC Cycle      | Drives energized  |Drives de-energized|
//      +---------------+-------------------+-------------------+
//      | 0-180 deg.    | PWM2H = ON        | PWM2H = OFF       |
//      |               | PWM2L = OFF       | PWM2L = ON        |
//      |               +-------------------+-------------------+
//      |               |           PWM1H = OFF                 |
//      |               |           PWM1L = ON                  |
//      +---------------+-------------------+-------------------+
//      | 181-360 deg.  | PWM1H = ON        | PWM1H = OFF       |
//      |               | PWM1L = OFF       | PWM1L = ON        |
//      |               +-------------------+-------------------+
//      |               |           PWM2H = OFF                 |
//      |               |           PWM2L = ON                  |
//      +---------------+---------------------------------------+
//
//  NOTE: It is not necessary to configure the I/O pins.
//  The PWM High and Low I/O Enable bits (PENyH and PENyL) in the PWM Control 
//  Register 1 (PWMxCON1<7:0>) enable each PWM output pin for use by the MCPWM 
//  module. When a pin is enabled as PWM output, the PORT and TRIS registers 
//  controlling the pin are disabled.
//
//  PWM I/O pins are set to Complementary PWM Output mode by default on a device 
//  Reset.  Complementary PWM Output mode is selected for each PWM I/O pin pair 
//  by clearing the PWM I/O Pair Mode bit (PMOD) in the PWM Control Register 1 
//  (PWMxCON1<11:8>). 
//-----------------------------------------------------------------------------

static void _pwm_config_inverter(void)
{
    //  Initialize to:
    //      PWM @ 20KHz, (determined by sine_table.h)
    //      complimentary outputs, 
    //      Free-Running Mode,
    //      500 nSec of deadtime, 

    //  TODO: Disable Timebase?
    PTCONbits.PTEN = 0;     
    
    // Enable PWM1 output pins and configure as complementary
    PWMCON1bits.PMOD1 = 0;      // complementary outputs
    PWMCON1bits.PEN1H = 1;
    PWMCON1bits.PEN1L = 1;
    
    // Enable PWM2 output pins and configure as complementary
    PWMCON1bits.PMOD2 = 0;      // complementary outputs
    PWMCON1bits.PEN2H = 1;
    PWMCON1bits.PEN2L = 1;

    //  PWM Timebase setup
    PTCONbits.PTCKPS = 0;       //  PWM Time Base Prescalar
                                //  00 = 1:1

    //  Computed Period based on CPU speed and desired PWM frequency ( > 20KHz)
    //  Refer to "sine_table.h" for details.
    PTPER = PTPER_INIT_VAL; 
                                
    //  Initialize Duty Cycle registers: Setting the duty cycle registers to 
    //  zero, turns off the PWM output(s).
    PDC1 = 0;      
    PDC2 = 0;       
    
    //  PWM Special Event Trigger: Synchronize A/D conversions to PWM time base
    //  SEVOPS (Special Event Output Postscaler) 1:1 to 1:16 postscale ratio
    PWMCON2bits.SEVOPS = 0;
    //  TODO: Add better explanation
    //  The Event Trigger is generated when the PTMR value matches SEVTCMP.
    SEVTCMP = ((FCY/FPWM - 1)/2);
    
    //  Edge-aligned PWM signals are produced by the module when the PWM time
    //  base is in the Free-Running or Single-Shot mode.
    PTCONbits.PTMOD = 0;    //  PTMOD:  00 = Free-Running
                            //          10 = Continuous Up/Down
                            //          11 = Double-Update 

    //  From Datasheet, section 15.7.2  Note: The user should not modify the 
    //  DTCON1 register value while the PWM module is operating (PTEN = 1). 
    //  Unexpected results may occur.
    //  TODO: Check if DTCON is affected by changing value of FOSC
    DTCON1 = 20;            // ~500 ns of dead time  (40MIPS)

    //  PWM Output Override
    //  OVDCON = 0x3000;    //  (Override) All FETS off
	
    OVDCONbits.POUT1L = 0;  //  PWM1L = OFF
    //  OVDCONbits.POUT1L = 1;  //  PWM1L = ON
    
    OVDCONbits.POVD1L = 0;  //  Output 1L is controlled by POUT1L
    OVDCONbits.POUT1H = 0;  //  PWM1H = OFF
    OVDCONbits.POVD1H = 0;  //  Output 1H is controlled by POUT1H

    OVDCONbits.POUT2L = 0;  //  PWM2L = OFF
    OVDCONbits.POVD2L = 0;  //  Output 2L is controlled by POUT2L
    OVDCONbits.POUT2H = 0;  //  PWM2H = OFF
    OVDCONbits.POVD2H = 0;  //  Output 2H is controlled by POUT2H

    //  OVERRIDE SYNCHRONIZATION:
    //  If OSYNC is set, all output overrides performed via the OVDCON register
    //  are synchronized to the PWM time base. For the Edge-Aligned mode, 
    //  overrides are synchronized to PTMR == 0. 
    //
    //  We want override to take effect immediatly, so we do not want it to be 
    //  synchronized with the PWM time base.
    PWMCON2bits.OSYNC = 0;

    return;
}

//-----------------------------------------------------------------------------
//  _pwm_config_charger()
//-----------------------------------------------------------------------------
//
//  Outputs are set and cleared, dependent upon where we are in the AC cycle.
//  The labels of the H-Bridge on the Sensata schematics are different than 
//  those used by Microchip.  To avoid confusion, comments in firmware will 
//  refer to the Microchip labels, except as noted.
//
//      +---------------+---------------------------------------+
//      | AC Cycle      |                                       |
//      +---------------+-------------------+-------------------+
//      | 0-180 deg.    | PWM2H = ON        | PWM2H = OFF       |
//      |               +-------------------+-------------------+
//      |               |           PWM1H = OFF                 |
//      |               |           PWM1L = OFF                 |
//      +---------------+-------------------+-------------------+
//      | 181-360 deg.  | PWM1H = ON        | PWM1H = OFF       |
//      |               +-------------------+-------------------+
//      |               |           PWM2H = OFF                 |
//      |               |           PWM2L = OFF                 |
//      +---------------+---------------------------------------+
//
//  Independent PWM Output mode is used for driving the transformer in charge 
//  mode. Dead-time generators are disabled in Independent PWM Output mode, and 
//  there are no restrictions on the state of the pins for a given output pair. 
//
//-----------------------------------------------------------------------------

static void _pwm_config_charger(void)
{
    //  Initialize to:
    //      PWM @ ~23040KHz, (determined by sine_table.h)
    //      independent outputs (dead-time is not enforced), 
    //      Free-Running Mode,
    
    //  TODO: Disable Timebase?
    PTCONbits.PTEN = 0;     

    // Enable PWM1 output pins and configure as complementary
    PWMCON1bits.PMOD1 = 1;      // Independent outputs
    PWMCON1bits.PEN1H = 1;
    PWMCON1bits.PEN1L = 1;
    
    // Enable PWM2 output pins and configure as complementary
    PWMCON1bits.PMOD2 = 1;      // Independent outputs
    PWMCON1bits.PEN2H = 1;
    PWMCON1bits.PEN2L = 1;
    
    //  PWM Timebase setup
    PTCONbits.PTCKPS = 0;       //  PWM Time Base Prescalar
                                //  00 = 1:1

    //  Computed Period based on CPU speed and desired PWM frequency ( > 20KHz)
    //  Refer to "sine_table.h" for details.
    PTPER = PTPER_INIT_VAL; 
                                
    //  Initialize Duty Cycle registers: Setting the duty cycle registers to 
    //  zero, turns off the PWM output(s).
    PDC1 = 0;      
    PDC2 = 0;       
    
    //  PWM Special Event Trigger: Synchronize A/D conversions to PWM time base
    //  SEVOPS (Special Event Output Postscaler) 1:1 to 1:16 postscale ratio
    PWMCON2bits.SEVOPS = 0;
    //  TODO: Add better explanation
    //  The Event Trigger is generated when the PTMR value matches SEVTCMP.
    SEVTCMP = ((FCY/FPWM - 1)/2);
    
    //  Edge-aligned PWM signals are produced by the module when the PWM time
    //  base is in the Free-Running or Single-Shot mode.
    PTCONbits.PTMOD = 0;    //  PTMOD:  00 = Free-Running
                            //          10 = Continuous Up/Down
                            //          11 = Double-Update 

    //  From Datasheet, section 15.7.2  Note: The user should not modify the 
    //  DTCON1 register value while the PWM module is operating (PTEN = 1). 
    //  Unexpected results may occur.
    //  TODO: Check if DTCON is affected by changing value of FOSC
    DTCON1 = 20;            // ~500 ns of dead time  (40MIPS)

    //  PWM Output Override - make all FET outputs low (off)
    OVDCONbits.POUT1L = 0;  //  PWM1L = OFF
    OVDCONbits.POVD1L = 0;  //  Output 1L is controlled by POUT1L
    OVDCONbits.POUT1H = 0;  //  PWM1H = OFF
    OVDCONbits.POVD1H = 0;  //  Output 1H is controlled by POUT1H

    OVDCONbits.POUT2L = 0;  //  PWM2L = OFF
    OVDCONbits.POVD2L = 0;  //  Output 2L is controlled by POUT2L
    OVDCONbits.POUT2H = 0;  //  PWM2H = OFF
    OVDCONbits.POVD2H = 0;  //  Output 2H is controlled by POUT2H
    
    //  OVERRIDE SYNCHRONIZATION:
    //  If OSYNC is set, all output overrides performed via the OVDCON register
    //  are synchronized to the PWM time base. For the Edge-Aligned mode, 
    //  overrides are synchronized to PTMR == 0. 
    //
    //  Overrides are synchronized with the PWM time base.
    PWMCON2bits.OSYNC = 1;

    return;
}

//-----------------------------------------------------------------------------
//  _pwm_config_converter()
//-----------------------------------------------------------------------------
//
//  This is the configuration file for the DC+DC Converter.  The hardware 
//  is driven like the Inverter.  Refer to comments in _pwm_config_inverter
//  for additional details.
//
//  The fundamental difference between the 'inverter' and 'converter' are that
//  the inverter generates a sinusoidal output therefore it uses 'sine_table.h' 
//  which has several #define's that are used by the inverter ISR, but not 
//  needed by the converter.
//
//-----------------------------------------------------------------------------

static void _pwm_config_converter(void)
{
    //  Initialize to:
    //      complimentary outputs, 
    //      Free-Running Mode,
    //      500 nSec of deadtime, 

    
    //  TODO: Disable Timebase?
    PTCONbits.PTEN = 0;     
    
    // Enable PWM1 output pins and configure as complementary
    PWMCON1bits.PMOD1 = 0;      // complementary outputs
    PWMCON1bits.PEN1H = 1;
    PWMCON1bits.PEN1L = 1;
    
    // Enable PWM2 output pins and configure as complementary
    PWMCON1bits.PMOD2 = 0;      // complementary outputs
    PWMCON1bits.PEN2H = 1;
    PWMCON1bits.PEN2L = 1;

    //  PWM Timebase setup
    PTCONbits.PTCKPS = 0;       //  PWM Time Base Prescalar
                                //  00 = 1:1

    //  Computed Period based on CPU speed and desired PWM frequency ( > 20KHz)
    //  Refer to "sine_table.h" for details.
    PTPER = PTPER_INIT_VAL; 
                                
    //  Initialize Duty Cycle registers: Setting the duty cycle registers to 
    //  zero, turns off the PWM output(s).
    PDC1 = 0;      
    PDC2 = 0;       
    
    //  PWM Special Event Trigger: Synchronize A/D conversions to PWM time base
    //  SEVOPS (Special Event Output Postscaler) 1:1 to 1:16 postscale ratio
    PWMCON2bits.SEVOPS = 0;
    //  TODO: Determine if still needed - Event Trigger was used for ADC
    //      Now ADC is triggered by a timer.
    //  TODO: Revise this for the Converter -  we don't use sine_table.h 
    //      FPWM is defined in sine_table.h
    //  The Event Trigger is generated when the PTMR value matches SEVTCMP.
    SEVTCMP = ((FCY/FPWM - 1)/2);
    
    //  Edge-aligned PWM signals are produced by the module when the PWM time
    //  base is in the Free-Running or Single-Shot mode.
    PTCONbits.PTMOD = 0;    //  PTMOD:  00 = Free-Running
                            //          10 = Continuous Up/Down
                            //          11 = Double-Update 

    //  From Datasheet, section 15.7.2  Note: The user should not modify the 
    //  DTCON1 register value while the PWM module is operating (PTEN = 1). 
    //  Unexpected results may occur.
    //  TODO: Check if DTCON is affected by changing value of FOSC
    DTCON1 = 20;            // ~500 ns of dead time  (40MIPS)

    //  PWM Output Override - make all FET outputs low (off)
    OVDCONbits.POUT1L = 0;  //  PWM1L = OFF
    OVDCONbits.POVD1L = 0;  //  Output 1L is controlled by POUT1L
    OVDCONbits.POUT1H = 0;  //  PWM1H = OFF
    OVDCONbits.POVD1H = 0;  //  Output 1H is controlled by POUT1H

    OVDCONbits.POUT2L = 0;  //  PWM2L = OFF
    OVDCONbits.POVD2L = 0;  //  Output 2L is controlled by POUT2L
    OVDCONbits.POUT2H = 0;  //  PWM2H = OFF
    OVDCONbits.POVD2H = 0;  //  Output 2H is controlled by POUT2H
    
    //  OVERRIDE SYNCHRONIZATION:
    //  If OSYNC is set, all output overrides performed via the OVDCON register
    //  are synchronized to the PWM time base. For the Edge-Aligned mode, 
    //  overrides are synchronized to PTMR == 0. 
    //
    //  We want override to take effect immediatly, so we do not want it to be 
    //  synchronized with the PWM time base.
    PWMCON2bits.OSYNC = 0;

    return;
}

//-----------------------------------------------------------------------------
void pwm_Config(PWM_CONFIG_t config, void (*isr_func)(void))
{
    if(NULL == isr_func) isr_func = pwm_DefaultIsr;

    //  TODO: Disable Interrupts here.
    _pwm_isr = isr_func;
    _pwm_config = config;

    switch (config)
    {
    default :
    case PWM_CONFIG_DEFAULT :
// 		LOG(SS_PWM, SV_INFO, "PWM_CONFIG_DEFAULT");
        _pwm_config_default();
        break;
        
    case PWM_CONFIG_BOOT_CHARGE :
//		LOG(SS_PWM, SV_INFO, "PWM_CONFIG_BOOT_CHARGE");
        _pwm_config_boot_charge();
        break;
        
    case PWM_CONFIG_CONVERTER :
//		LOG(SS_PWM, SV_INFO, "PWM_CONFIG_CONVERTER");
        _pwm_config_converter();
        break;
        
    case PWM_CONFIG_CHARGER :
//		LOG(SS_PWM, SV_INFO, "PWM_CONFIG_CHARGER");
        _pwm_config_charger();
        break;
        
    case PWM_CONFIG_INVERTER :
//		LOG(SS_PWM, SV_INFO, "PWM_CONFIG_INVERTER");
        _pwm_config_inverter();
        break;
    }
    //  TODO: What about Timer3 and ADC/DMA interrupts
}

//-----------------------------------------------------------------------------
// Start PWM module
void pwm_Start(void)
{
  //LOG(SS_PWM, SV_INFO, "pwm_Start()");

    _PWMIF = 0;     // Clear interrupt flag
    _PWMIE = 1;     // Enable PWM Interrupts    
    
    PTCONbits.PTEN = 1;     //  Start the time-base
}

//-----------------------------------------------------------------------------
// Stop PWM module
void pwm_Stop(void)
{
  //LOG(SS_PWM, SV_INFO, "pwm_Stop()");
    
    _PWMIE = 1;     // Enable PWM Interrupts    
    
    PDC1 = 0;
    PDC2 = 0;
}

// <><><><><><><><><><><><><> pwm.c <><><><><><><><><><><><><><><><><><><><><><>
