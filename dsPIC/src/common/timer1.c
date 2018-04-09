// <><><><><><><><><><><><><> timer1.c <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//  Copyright(C) 2013 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
//  Interrupt driven one millisecond timing provided by hardware Timer1
//
//-----------------------------------------------------------------------------

// -------
// headers
// -------
#include "options.h"    // must be first include
#include "timer1.h"

// -----------
// global data
// -----------
volatile int16_t   _T1TickCount; // gets incremented and decremented
volatile SYSTICKS  _SysTicks;    // continuously incrementing tick counter (wraps in 49.7 days)

//-----------------------------------------------------------------------------
//  Configure T1 Timer
//
//  Initialize Timer1 for 1-mSec interrupt
//  The timer module is a 16-bit timer/counter affected by the following
//  registers:
//      TMRx: 16-bit timer count register
//      PRx: 16-bit period register associated with the timer
//      TxCON: 16-bit control register associated with the timer
//
//  FCY = (FOSC * PLL) = (8MHz * 16) = 128MHz
//  PR1 = 1-mSec/(TCY*Prescale)
//-----------------------------------------------------------------------------

// 8 Mhz crystal: 40.000 MIPS
void _T1Config(void)
{
    _T1TickCount = 0;
    _SysTicks    = 0;
  #ifndef WIN32
    T1CONbits.TON = 0;          // Disable Timer1

    // TCS: Timer Clock Source Select bit
    //      1 = External clock from pin TxCK
    //      0 = Internal clock (Tcy)
    T1CONbits.TCS = 0;

    T1CONbits.TGATE = 0;        // Disable Gated Timer mode

    // TCKPS<1:0>: Timer Input Clock Prescale Select bits
    //      11 = 1:256 prescale value
    //      10 = 1:64 prescale value
    //      01 = 1:8 prescale value
    //      00 = 1:1 prescale value
    T1CONbits.TCKPS = 0b01;

    //  Clear the timer register (Timer1)
    TMR1 = 0;
    //  PR1: 16-bit period register for Timer-1
    PR1 = 5000;
  #endif
}

//-----------------------------------------------------------------------------
//  Start the T1 Timer
//-----------------------------------------------------------------------------

void _T1Start(void)
{
    IFS0bits.T1IF = 0;      // Clear Timer1 Interrupt Flag
    IEC0bits.T1IE = 1;      // Enable Interrupts for Timer1

    T1CONbits.TON = 1;      // turn on timer 1
}

//-----------------------------------------------------------------------------
//  Stop the T1 Timer
//-----------------------------------------------------------------------------

void _T1Stop(void)
{
    IFS0bits.T1IF = 0;      // Clear Timer1 Interrupt Flag
    IEC0bits.T1IE = 0;      // Disable Interrupts for Timer1

    T1CONbits.TON = 0;      // turn off timer 1
}


//-----------------------------------------------------------------------------
//  One-millisecond timer interrupt.
//-----------------------------------------------------------------------------

void __attribute__((interrupt, no_auto_psv)) _T1Interrupt (void)
{
    _T1TickCount++;
    _SysTicks++;        // one more system tick
    IFS0bits.T1IF = 0;  // end-of-interrupt
}

// -----------------------------------------------------------------------
//                    T I M E     R O U T I N E S
// -----------------------------------------------------------------------

// returns milliseconds elapsed from starting time to ending time
uint32_t ElapsedMsec(SYSTICKS startTicks, SYSTICKS endTicks)
{
    uint32_t diffTicks;
    if (startTicks > endTicks)
    {
        // handle 32 bit counter wrap-around
        diffTicks = (0xFFFFFFFF - startTicks) + endTicks;
    }
    else
    {
        diffTicks = endTicks-startTicks;
    }
    return(diffTicks);
}

// <><><><><><><><><><><><><> timer1.c <><><><><><><><><><><><><><><><><><><><><><>
