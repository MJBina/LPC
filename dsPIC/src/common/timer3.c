// <><><><><><><><><><><><><> timer3.c <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//  Copyright(C) 2013 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
//  Timer3 is used to trigger the ADC as a one-short timer.
//
//-----------------------------------------------------------------------------

// -------
// headers
// -------
#include "options.h"    // must be first include
#include "timer3.h"

// ----
// data
// ----
static int16_t _T3Count;    // counts down to zero then shuts off timer

//-----------------------------------------------------------------------------
//  Initialize Timer3 for use as a one-shot. Timer is started by the PWM ISR. 
//  Timer3 will be used to trigger the ADC. When the timer expires, the 
//
//  Fcy == 40.00000 MHZ 
//-----------------------------------------------------------------------------

void T3_Config(void)
{
    // TCS: Timer Clock Source Select bit
    //      1 = External clock from pin TxCK
    //      0 = Internal clock (Tcy)
    T3CONbits.TCS = 0;

    // TCKPS<1:0>: Timer Input Clock Prescale Select bits
    //      11 = 1:256 prescale value
    //      10 = 1:64 prescale value
    //      01 = 1:8 prescale value
    //      00 = 1:1 prescale value
    T3CONbits.TCKPS = 001;     //  --> (40/8) 5.000 MHz clock 

    _T3Count = 0;
    return;
}

//-----------------------------------------------------------------------------
//  Start the timer 
//-----------------------------------------------------------------------------
void T3_Start(void)
{
    IFS0bits.T3IF = 0;      //  Clear Timer3 Interrupt Flag
    IEC0bits.T3IE = 1;      //  Enable Interrupts for Timer3

    TMR3 = 0;
    //  TODO: This should be 10 uSec
    //  Timer Duration values are set by PR3 in increments of .2000 uSec.
    PR3 = 37;               // ~7.4 uSec @ 40.0000 MIPS

    _T3Count = 1;
    T3CONbits.TON = 1;      // turn on Timer3
}

//-----------------------------------------------------------------------------
//  _T3Interrupt()
//-----------------------------------------------------------------------------

void __attribute__((interrupt, no_auto_psv)) _T3Interrupt (void)
{
    if(_T3Count > 0)
    {
        _T3Count--;
    }
    else
    {
        T3CONbits.TON = 0;      //  turn Off Timer3
    }
    IFS0bits.T3IF = 0;      //  Clear Timer3 Interrupt Flag
}

// <><><><><><><><><><><><><> timer3.c <><><><><><><><><><><><><><><><><><><><><><>
