// <><><><><><><><><><><><><> hs_temp.c <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//	Copyright(C) 2012 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
//  Heat Sink PWM Temperature Sensor
//     Temperateure is linear with output duty cycle
//     Analog Devices TMP05/06
//     uses IC4 comparator via interrupts to detect duty cycle
//
//	Description:
//     The temperature sensor is an Analog Devices TMP05/06. It outputs a PWM
//     signal corresponding to temperature. The high/low times are measured using
//     the input capture (IC4) module of the dsPIC30. 'pulse_flag' is set just
//     after we acquire the low (and high) times (about 100 to 150 mSec) for the
//     temperature sensor. Based upon that timing, we can use 'pulse_hi' and
//     'pulse_lo' without interference from the _IC4Interrupt.
//
//     'pulse_hi' and 'pulse_lo' are converted to a temperature value based upon
//     the following formula:
//         degrees-C = 421 - (751 * (pulse_hi / pulse_lo))
//
//     The sensor's initial readings are inaccurate. 'startup_ticks' is used to
//     accomodate this by substituting a default value for the first 10-readings.
//	
//-----------------------------------------------------------------------------

// -------
// headers
// -------
#include "options.h"    // must be first include
#include "hs_temp.h"
#include "tasker.h"

// --------------------------
// Conditionsl Debug Compiles
// --------------------------
//#define DEBUG_TEMP_SENSOR   1

// -----------------------------
// Auto Removal for build types
// -----------------------------
#if OPTION_NO_CONDITIONAL_DBG
  #undef  DEBUG_TEMP_SENSOR
#endif

// --------
// Defines
// --------
#define HS_TEMP_READINGS (4)  // average 4 readings
#define ALLOWED_DIFF_C   (10) // difference in temperatures allowed to save reading

// -------
// globals 
// -------
volatile int16_t  g_hsTempC = ROOM_TEMP_C;  // initial reported heat sink
                         //temperature (C) output before actual is established
// -------------
// internal data
// -------------

// signal edge
#define EDGE_POS  0
#define EDGE_NEG  1

// signal edge
static uint8_t _Edge = EDGE_POS;

// pwm hi / low times determine temperature
static int16_t pulse_hi = 0;
static int16_t pulse_lo = 0;

// temperature hysterisis
static int16_t s_indexTemps  = 0;   // index into s_hsTemps
static int16_t s_hsTemps[HS_TEMP_READINGS] = { ROOM_TEMP_C, ROOM_TEMP_C, ROOM_TEMP_C };  // averaging array

//-----------------------------------------------------------------------------
//  Heat Sink Temperature task driver
//-----------------------------------------------------------------------------

// convert PWM to Celcius Temperature (lo can't be zero)
#define PWM_TO_CELCIUS(hi,lo)  ((uint16_t)(421L - 751L*(uint32_t)(hi)/(uint32_t)(lo)))

TASK_ID_t _task_hs = -1; // heat sink task handle

void TASK_HeatSink(void)
{
    static int16_t s_startup_ticks = 0;
    int16_t newTempC, avgTemp;

    // calculate new temperature from pwm hardware
    if (pulse_lo == 0)  // avoid division by zero
        newTempC = ROOM_TEMP_C;
    else
        newTempC = PWM_TO_CELCIUS(pulse_hi, pulse_lo);

    // check if in startup mode
    if (++s_startup_ticks < 11)
    {
        // -------------
        // startup mode
        // -------------
        // fill the hysteresis buffer
        s_hsTemps[s_indexTemps++] = newTempC;
        if (s_indexTemps>=HS_TEMP_READINGS) s_indexTemps = 0; // wrap index

        // use room temperature until startup complete
        newTempC = ROOM_TEMP_C;
    }
    else
    {
        // -------------
        // normal mode
        // -------------
        s_startup_ticks = 10; // keep it running

        // calculate average
        avgTemp = (s_hsTemps[0] + s_hsTemps[1] + s_hsTemps[2] + s_hsTemps[HS_TEMP_READINGS-1])/4;

        // only accept if within tolerance of average value
        if ((newTempC < (avgTemp + ALLOWED_DIFF_C)) && (newTempC > (avgTemp - ALLOWED_DIFF_C)))
        {
            // fill the hysteresis buffer
            s_hsTemps[s_indexTemps++] = newTempC;
            if (s_indexTemps>=HS_TEMP_READINGS) s_indexTemps = 0; // wrap indec

          #ifdef DEBUG_TEMP_SENSOR
            LOG(SS_SYS, SV_INFO, "HS-Temp=%d Avg=%d hi=%u lo=%u", newTempC, avgTemp, pulse_hi, pulse_lo);
          #endif
        }
        else
        {
            // bad temperature
            LOG(SS_SYS, SV_ERR, "Wild HS-Temp Tossed=%d Avg=%d hi=%u lo=%u", newTempC, avgTemp, pulse_hi, pulse_lo);
        }
        newTempC = avgTemp; // use average temperature
    }
    g_hsTempC = newTempC;
}

//-----------------------------------------------------------------------------
//  _T2Config
//-----------------------------------------------------------------------------
//  Initialize Timer2 to be a free-running timer used for input capture.
//  Timer2 resolution is configured to be about 20-uSec.
//  The timer module is a 16-bit timer/counter affected by the following
//  registers:
//       TMRx: 16-bit timer count register
//       PRx: 16-bit period register associated with the timer
//       TxCON: 16-bit control register associated with the timer
//
//  FCY = (FOSC * PLL) = (7.3728MHz * 16) = 29491200Hz	//	TODO: Check This!
//  PR2 = 20-uSec/(TCY*Prescale)
//-----------------------------------------------------------------------------

static void _T2Config(void)
{
 #ifndef WIN32
    // TCS: Timer Clock Source Select bit
    //      1 = External clock from pin TxCK
    //      0 = Internal clock (Tcy)
    T2CONbits.TCS = 0;

    // TCKPS<1:0>: Timer Input Clock Prescale Select bits
    //    * 11 = 1:256 prescale value
    //      10 = 1:64 prescale value
    //      01 = 1:8 prescale value
    //      00 = 1:1 prescale value
    T2CONbits.TCKPS = 0b011;

    //  Clear the timer register (Timer1)
    TMR2 = 0;
    //  PR2: 16-bit period register for Timer2.  We need to capture high and
    //  low times for an input pulse that has a period ranging from 99mSec to
    //  150mSec. So we need a period greater than 150mSec to assure that we
    //  won't miss the edges.
    //  Measured period with Prescale = 1:256
    //      PR2=0x1FFF --> 70.88 mSec
    //      PR2=0x3FFF --> 141.44 mSec
    //      PR2=0x7FFF --> 281.99 mSec
    PR2 = 0x07FFF;
 #endif // WIN32
}

//-----------------------------------------------------------------------------
//  _T2Start - Start Timer2
//-----------------------------------------------------------------------------
static void _T2Start(void)
{
    IFS0bits.T2IF = 0;      // Clear Timer2 Interrupt Flag
    IEC0bits.T2IE = 0;      // Disable Interrupts for Timer2
    T2CONbits.TON = 1;      // turn on Timer2
}

//-----------------------------------------------------------------------------
//  _T2Stop - Stop Timer2
//-----------------------------------------------------------------------------
static void _T2Stop(void)
{
    IFS0bits.T2IF = 0;      // Clear Timer2 Interrupt Flag
    IEC0bits.T2IE = 0;      // Disable Interrupts for Timer2
    T2CONbits.TON = 0;      // turn off Timer2
}

//-----------------------------------------------------------------------------
//  _T2Interrupt()
//  This interrupt is used as a free running timer for the interrupt compare
//-----------------------------------------------------------------------------
void __attribute__((interrupt, no_auto_psv)) _T2Interrupt (void)
{
    IFS0bits.T2IF = 0;      // Clear Timer2 Interrupt Flag
}

//-----------------------------------------------------------------------------
//  Input Capture 4 - Configuration
//-----------------------------------------------------------------------------
static void _IC4Config(void)
{
    TRISDbits.TRISD11 = 1;      // input from RD11/IC4

//  IC4CONbits.ICM = 0b000;     // 000 = Input capture module turned off

    //  ICTMR: Input Capture Timer Select bits
    //      1 = TMR2 contents are captured on capture event
    //      0 = TMR3 contents are captured on capture event
    IC4CONbits.ICTMR = 1;

    //  ICI: Select Number of Captures per Interrupt bits
    //      11 = Interrupt on every fourth capture event
    //      10 = Interrupt on every third capture event
    //      01 = Interrupt on every second capture event
    //      00 = Interrupt on every capture event
    IC4CONbits.ICI = 0; // 0b00;      // Interrupt on every capture event

    //  ICM: Input Capture Mode
    IC4CONbits.ICM = 3; // 0b011;     // 011 = Capture mode, every rising edge
    // Generate capture event on every rising edge

//  IC4CONbits.ICM = 0b001;     //  capture every edge

    _Edge = EDGE_POS;
}

//-----------------------------------------------------------------------------
//  Input Capture 4 - Start
//-----------------------------------------------------------------------------
static void _IC4Start(void)
{
    // Enable Capture Interrupt
    IFS2bits.IC4IF=0;               //  Clear IC4 Interrupt Status Flag
    IEC2bits.IC4IE=1;               //  IC4 Interrupt Enable
}

//-----------------------------------------------------------------------------
//  Input Capture 4 - Stop
//-----------------------------------------------------------------------------
static void _IC4Stop(void)
{
    // Disable Capture Interrupt
    IFS2bits.IC4IF=0;               //  Clear IC4 Interrupt Status Flag
    IEC2bits.IC4IE=0;               //  IC4 Interrupt Disable
}

//-----------------------------------------------------------------------------
//  Input Capture 4 Interrupt Service Routine
//-----------------------------------------------------------------------------
// confirmed that PWM interrupts can interrupt this isr
// typical run time <10usec
void __attribute__((interrupt, no_auto_psv)) _IC4Interrupt(void)
{
    //  Check for Overflow
    if (IC4CONbits.ICOV)
    {
        //  TODO: What happens on an overflow?
    }

    switch(_Edge)
    {
    case EDGE_POS :
        //  ICBNE - Input Capture Buffer Not Empty
        while(IC4CONbits.ICBNE)
        {
            pulse_lo = IC4BUF;
        }
        //  If we reset the timer here, then the captured clock in the
        //  EDGE_NEG state is the time that the pulse is low.
        TMR2 = 0;
        IC4CONbits.ICM = 2; // 0b010;     // Capture next falling edge
        _Edge = EDGE_NEG;
        break;

    case EDGE_NEG :
        while(IC4CONbits.ICBNE)
        {
            pulse_hi = IC4BUF;
        }
        //  If we reset the timer here, then the captured clock in the
        //  EDGE_POS state is the time that the pulse is high.
        TMR2 = 0;
        IC4CONbits.ICM = 3; // 0b011;     // Capture next rising edge
        task_MarkAsReady(_task_hs);
        _Edge = EDGE_POS;
        break;
    }
	
    IFS2bits.IC4IF = 0;		//  Clear IC4 Interrupt Status Flag
}

//-----------------------------------------------------------------------------
// configure heat sink temp sensor
void hs_Config(void)
{
    _T2Config();
    _IC4Config();
    _task_hs = task_AddToQueue(TASK_HeatSink, "hs   "); 
}

//-----------------------------------------------------------------------------
// start heat sink temp sensor
void hs_Start(void)
{
    _T2Start();
    _IC4Start();
}

//-----------------------------------------------------------------------------
// stop heat sink temp sensor
void hs_Stop(void)
{
    _T2Stop();
    _IC4Stop();
}

// <><><><><><><><><><><><><> hs_temp.c <><><><><><><><><><><><><><><><><><><><><><>
