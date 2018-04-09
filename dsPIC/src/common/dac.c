// <><><><><><><><><><><><><> dac.c <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//	Copyright(C) 2015 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
// Digital/Analog Converter
// 
// TI DAC101c081C1MK/NOPQ SOT-6  6 pin
// 10 Bit DAC on I2C Bus #1
// Address 0xD
//
//  NOT USED - data too noisy
//
//-----------------------------------------------------------------------------

// -------
// headers
// -------
#include "options.h"    // must be first include
#include "dsPIC_serial.h"
#include "dac.h"

/*

// -------------
// DAC constants
// -------------
#define  DAC_I2C_ADDRESS    0x1A   // 0xD<<1
#define  DAC_MAX_RETRY       5     // max retries on resending a byte

// -------------
// DAC baud rates
// -------------
// baud rate choices
#define DAC_BAUD_1000KHZ   35    // ultra hi speed (39.6 usec per dac)  811 msec for 20480 dacs
#define DAC_BAUD_400KHZ    91    // hi speed  (80 usec per dac; PWM clock is 40 usec)
#define DCA_BAUD_100KHZ   394    // lo speed
// value used to set speed
#define DAC_BAUD_RATE   DAC_BAUD_1000KHZ
// ----------------
// I2C-1 Baud Rates
// From measuring
//  25 -> 1000Khz
//  80 -> 461Khz
//  90 -> 413Khz
//  91 -> 400Khz
//  92 -> 390Khz
// 100 -> 375Khz
// 140 -> 260Khz
// 200 -> 187Khz
// 300 -> 130Khz
// 350 -> 111Khz
// 394 -> 100Khz
// --------------

// ----
// data
// ----
DAC_DRV_t  g_dac;    // state info

// ---------
// debugging
// ---------
#define DBG_DAC_INFO(str)  // { serial_puts(str); serial_putbuf("\r\n",2); }
#define DBG_DAC_ERR(str)   // { serial_puts(str); serial_putbuf("\r\n",2); }


// -----------------------------------------------------------------------------
// I2C Master Interrupt Service Routine
// Performs bytes transfers to/from hardware
// -----------------------------------------------------------------------------
void __attribute__((interrupt, no_auto_psv)) _MI2C1Interrupt(void)
{
    IFS1bits.MI2C1IF = 0;  // Clear the I2C Interrupt Flag;
    switch (g_dac.state)
    {
    case 0: // idle state
        break;

    case 1: // retry
        g_dac.state=2;
        I2C1CONbits.SEN=1;   // start bit; generates interrupt
        break;

    case 2: // start byte with slave address
        g_dac.state=3;              // set the state first
        I2C1TRN = DAC_I2C_ADDRESS;  // generates interrupt
        DBG_DAC_INFO("DAC-2");
        break;

    case 3: // send dac hi byte
        if (I2C1STATbits.ACKSTAT==1)
        {
            // no ack
            if (g_dac.nretries < DAC_MAX_RETRY)
            {
                // retry
                g_dac.nretries++;
                g_dac.state=1;
                I2C1CONbits.PEN=1;  // generates interrupt
                DBG_DAC_INFO("DAC-3 NACK; Retry");
                break;
            }
            else
            {
                // abort
                g_dac.state=7;      // Flag error and exit
                I2C1CONbits.PEN=1;  // generates interrupt
                DBG_DAC_ERR("DAC-3 Retrys Failed");
                break;
            }
        }
        g_dac.nretries = 0;
        g_dac.state=4;
        I2C1TRN = (g_dac.dac>>8)&0xF;   // hi byte; generates interrupt
        DBG_DAC_INFO("DAC-3");
        break;

    case 4: // send dac lo byte
        if (I2C1STATbits.ACKSTAT==1)
        {
            // retry
            g_dac.state=1;
            I2C1CONbits.PEN=1;  // generates interrupt
            DBG_DAC_INFO("DAC-4 NACK");
            break;
        }
        g_dac.state=5;
        I2C1TRN = ((g_dac.dac)&0x00FF);  // lo byte; generates interrupt
        DBG_DAC_INFO("DAC-4");
        break;

    case 5: // stop bit
        g_dac.state=6;
        I2C1CONbits.PEN=1;  // generates interrupt
        DBG_DAC_INFO("DAC-5");
        break;

    case 6: // done and ok
        g_dac.state  = 0;   // idle
        g_dac.status = DAC_STATUS_OK;
        DBG_DAC_INFO("DAC-6");
        break;

    case 7: // done with err
        g_dac.state  = 0; // idle
        g_dac.status = DAC_STATUS_ERR;
        DBG_DAC_ERR("DAC-7 Err");
        break;
    } // switch
}

// -----------------------------------------------------------------------------
// initialize I2C bus 1 which is used for DAC transport
static void I2C1_init()
{
    TRISGbits.TRISG3 = 1;   // RG3  pin-36  i/o     SDA1 (DAC)
    TRISGbits.TRISG2 = 0;   // RG2  pin-37  output  SCL1 (DAC)
    PMD1bits.I2C1MD  = 0;   // enable peripheral

    I2C1CONbits.I2CEN  = 0; // disable while configuring

    I2C1CONbits.A10M   = 0; // 7 bit I2C address
    I2C1CONbits.SCLREL = 1; // no clock stretch

    I2C1BRG = DAC_BAUD_RATE;
    I2C1ADD = DAC_I2C_ADDRESS;

    LOG(SS_DAC, SV_INFO, "I2C1_init");
}

// -----------------------------------------------------------------------------
// configure dac driver
void dac_Config()
{
    // iniitalize data structures
    memset(&g_dac, 0, sizeof(g_dac));
    g_dac.status = DAC_STATUS_OK;   // indicate ready

    // initialize hardware transport
    I2C1_init();

    LOG(SS_DAC, SV_INFO, "Config");
}

// -----------------------------------------------------------------------------
// start dac interrupts
void dac_Start()
{
    IEC1bits.MI2C1IE  = 1;   // Master Interrupt Enable
    IFS1bits.MI2C1IF  = 0;   // clear
    I2C1CONbits.I2CEN = 1;   // enable I2C

    LOG(SS_DAC, SV_INFO, "Start");
}

// -----------------------------------------------------------------------------
// stop dac interrupts
void dac_Stop()
{
    IEC1bits.MI2C1IE  = 0;   // Master Interrupt Disable
    IFS1bits.MI2C1IF  = 0;   // clear
    I2C1CONbits.I2CEN = 0;   // disable I2C

//  LOG(SS_DAC, SV_INFO, "Stop");
}

// -----------------------------------------------------------------------------
// write to the dac
// returns: 0=ok, 1=driver is busy
int dac_Write(DAC_MODE_t mode, uint16_t dac)
{
//  LOG(SS_DAC, SV_INFO, "dac_Write(mode=%u,dac=%u)", (unsigned)mode, (unsigned)dac);

    // check if driver is busy
    if (g_dac.status == DAC_STATUS_BUSY) return(1);
    g_dac.status = DAC_STATUS_BUSY;   // driver is now busy

    // save input parameters
    g_dac.dac = (dac & DAC_MAX_VALUE) | ((uint8_t)mode&0x3)<<12;
    // init state flags for interrupt routine
    g_dac.nretries = 0;
    // prime the hardware
    g_dac.state      = 2;
    I2C1CONbits.SEN  = 1;   // send start bit; generate interrupt
    return(0);
}
*/

// -----------------------------------------------------------------------------
//               D A C    U N I T    T E S T 
// -----------------------------------------------------------------------------

/*
// CAUTION! this test is blocking; run during startup only
void dac_RunUnitTest()
{
    #define DAC_TEST_CYCLES  10
    SYSTICKS  startTicks, delayTicks;
    int16_t n,dac,nerrs=0;

    LOG(SS_DAC, SV_INFO, "DAC Unit Test Starting; %u Cycles", DAC_TEST_CYCLES);

//  for (;;)
    {
    // repeat n cycles
    startTicks = GetSysTicks(); // start timer
    for (n=0; n<DAC_TEST_CYCLES; n++)
    {
        // ramp up
        for (dac=DAC_MIN_VALUE; dac<DAC_MAX_VALUE; dac++)
        {
            delayTicks = GetSysTicks();
            while (dac_IsBusy())
            { 
                if (!IsTimedOut(5,delayTicks)) continue;
             // LOG(SS_DAC, SV_ERR, "DAC ramp up timeout dac=%u", dac);
                nerrs++;
                break;
            }
            dac_Write(DAC_MODE_NORMAL, dac);
        }
        
        // ramp down
        for (dac=DAC_MAX_VALUE; dac>=DAC_MIN_VALUE; dac--)
        {
            delayTicks = GetSysTicks();
            while (dac_IsBusy())
            {
                if (!IsTimedOut(5,delayTicks)) continue;
             // LOG(SS_DAC, SV_ERR, "DAC ramp down timeout dac=%u", dac);
                nerrs++;
                break;
            }
            dac_Write(DAC_MODE_NORMAL, dac);
        }
    }
    // pass / fail
    startTicks = GetSysTicks() - startTicks;
    if (nerrs)
        LOG(SS_DAC, SV_ERR,  "DAC FAILED; errors=%u %lu msecs", startTicks);
    else
        LOG(SS_DAC, SV_INFO, "%u DAC PASSED; %lu msecs", DAC_TEST_CYCLES*(DAC_MAX_VALUE+1)*2, startTicks);
    } // for
}
*/

// <><><><><><><><><><><><><> dac.c <><><><><><><><><><><><><><><><><><><><><><>
