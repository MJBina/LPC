// <><><><><><><><><><><><><> nvm.c <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//  Copyright(C) 2017 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
// Non-Volatile Memory Driver
// 
// NVM Chip:  Microchip 24LC64 EEPROM SOT-23  5 pin    21189T.PDF
//   I2C Bus #2
//   Address 0xA0 (not changeable)
//   8K bytes
//   32 byte page writes
//   256 pages
//   <5msec per page write
//   full memory reads
//-----------------------------------------------------------------------------

// -------
// headers
// -------
#include "options.h"    // must be first include
#include "charger.h"
#include "config.h"
#include "converter.h"
#include "converter_cmds.h"
#include "dsPIC33_CAN.h"
#include "inverter.h"
#include "nvm.h"

// --------------------------
// Conditional Debug Compiles
// --------------------------

// comment out flag to not use it
//#define DEBUG_NVM_VALIDATE   1   // define to debug bootup validation process
//#define DEBUG_NVM_CMDS       1   // define to debug nvm commands
//#define DEBUG_NVM_MP_WRITES  1   // define to debug multi-packet writes

// -----------------------------
// Auto Removal for build types
// -----------------------------
#if OPTION_NO_CONDITIONAL_DBG
  #undef DEBUG_NVM_VALIDATE
  #undef DEBUG_NVM_CMDS
  #undef DEBUG_NVM_MP_WRITES
#endif

// ---------------------------------------------------------
// NVM Chip is on I2C channel 1 or 2 depending upon hardware
// ---------------------------------------------------------
#if (OPTION_NVM == OPTION_NVM_I2C1)
    // NVM is on I2C channel 1
	#define _MI2CxInterrupt _MI2C1Interrupt
	#define _MI2CxIF		IFS1bits.MI2C1IF
	#define _I2CxMD			PMD1bits.I2C1MD
	#define _MI2CxIE		IEC1bits.MI2C1IE
	
	#define _I2CxTRN 		I2C1TRN	
	#define _I2CxRCV 		I2C1RCV
	#define _I2CxBRG		I2C1BRG
	#define _I2CxADD		I2C1ADD
	#define _I2CxCONbits	I2C1CONbits
	#define _I2CxSTATbits	I2C1STATbits
	
#elif (OPTION_NVM == OPTION_NVM_I2C2)
    // NVM is on I2C channel 2
	#define _MI2CxInterrupt _MI2C2Interrupt
	#define _MI2CxIF		IFS3bits.MI2C2IF
	#define _I2CxMD			PMD3bits.I2C2MD			//	enable peripheral
	#define _MI2CxIE		IEC3bits.MI2C2IE		//	I2C Master Interrupt Enable
	
	#define _I2CxTRN 		I2C2TRN
	#define _I2CxRCV 		I2C2RCV
	#define _I2CxBRG		I2C2BRG
	#define _I2CxADD		I2C2ADD
	#define _I2CxCONbits	I2C2CONbits
	#define _I2CxSTATbits	I2C2STATbits
	
#else
	#error 'OPTION_NVM' is not defined... 
#endif



// -------------
// NVM constants
// -------------
#define  NVM_I2C_ADDRESS    0xA0    // fixed for the 24LC64 SOT-23 part
#define  NVM_MAX_RETRY       5      // max retries on resending a byte
#define  NVM_WRITE_DELAY     8      // page write delay (msecs) needed for chip; spec says max is 5 msecs

// -------------
// NVM baud rates
// -------------
// baud rate choices
#define NVM_BAUD_400KHZ    91    // hi speed
#define NVM_BAUD_100KHZ   394    // lo speed
// value used to set speed
#define NVM_BAUD_RATE   NVM_BAUD_400KHZ
// ----------------
// I2C-2 Baud Rates
// From measuring
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
NVM_DRV_t  g_nvm;


// -----------
// Prototyping
// -----------
// NVM data addresses are zero based
int  nvm_L0_BegWritePage(uint16_t addr, uint8_t  nbytes, uint8_t* data); // hw limits writes to a page or less
int  nvm_L0_BegReadBuf  (uint16_t addr, uint16_t nbytes, uint8_t* data);
int  nvm_L0_BegWriteBuf (uint16_t addr, uint16_t nbytes, uint8_t* data);
int  nvm_IsSigValid(NVM_HDR_BLK_t* pHdr);
void nvm_InitHeader(NVM_HDR_BLK_t* pHdr);
void nvm_RunUnitTest(void);
uint16_t nvm_CalcChecksum(uint8_t* data, int16_t len);
static void nvm_EraseAll(void);
static void nvm_VerifyErased(void);

// ------------------
// readability macros
// ------------------
#define nvm_IsrStatus()   (g_nvm.isr.status)
#define nvm_IsError()     (NVM_STATUS_ERR  == nvm_IsrStatus() ? 1 : 0)  // is isr error occurred
#define nvm_IsIsrBusy()   (NVM_STATUS_BUSY == nvm_IsrStatus() ? 1 : 0)  // is isr busy
#define nvm_IsMwBusy()    (g_nvm.mpwr.state != 0 ? 1 : 0)   // is multi-write busy
#define nvm_IsCmdInProg() (g_nvm.curCmd != NVM_CMD_NONE)    // is command in progress
#define nvm_IsReqCmd()    (g_nvm.reqCmd)                    // is a requested command pending
#define nvm_IsDirty()     (g_nvm.isDirty)                   // is a nvm need saving


// ---------
// debugging
// ---------
#define DBG_NVM_INFO(str)   // { serial_puts(str); serial_putbuf("\r\n",2); }
#define DBG_NVM_ERR(str)    // { serial_puts(str); serial_putbuf("\r\n",2); }


// -----------------------------------------------------------------------------
//                     L E V E L    0    D R I V E R
// -----------------------------------------------------------------------------

// I2C Master Interrupt Service Routine
// Performs bytes transfers to/from hardware
void __attribute__((interrupt, no_auto_psv)) _MI2CxInterrupt(void)
{
    switch (g_nvm.isr.state)
    {
    case 0: // idle state
        break;

        // ---------------------
        // Control/Address Phase            
        // ---------------------
    case 2:
        // Start Byte with device select id
        g_nvm.isr.state=3;       // set the state first
        _I2CxTRN = NVM_I2C_ADDRESS;  // generates interrupt
        DBG_NVM_INFO("I2C-2");
        break;

    case 3:
        // Send address hi byte, if ack is received. Else Retry
        if (_I2CxSTATbits.ACKSTAT==1)         // Ack Not received, Retry
        {
            if (g_nvm.isr.nretries < NVM_MAX_RETRY)
            {
                g_nvm.isr.nretries++;
                g_nvm.isr.state=19;
                _I2CxCONbits.PEN=1;  // generates interrupt
                DBG_NVM_INFO("I2C-3 NACK; Retry");
                break;
            }
            else
            {
                g_nvm.isr.state=17;     // Flag error and exit
                _I2CxCONbits.PEN=1;  // generates interrupt
                DBG_NVM_ERR("I2C-3 Retrys Failed");
                break;
            }
        }
        g_nvm.isr.nretries = 0;
        g_nvm.isr.state=4;
        _I2CxTRN = (g_nvm.isr.addr>>8)&0xFF;   // hi byte; generates interrupt
        DBG_NVM_INFO("I2C-3");
        break;

    case 4:
        // Send address lo byte, if ack is received. Else Flag error and exit
        if (_I2CxSTATbits.ACKSTAT==1)         // Ack Not received, Flag error and exit
        {
            g_nvm.isr.state=17;
            _I2CxCONbits.PEN=1;  // generates interrupt
            DBG_NVM_INFO("I2C-4 NACK");
            break;
        }
        g_nvm.isr.state=5;
        _I2CxTRN = (g_nvm.isr.addr)&0xFF;  // lo byte; generates interrupt
        DBG_NVM_INFO("I2C-4");
        break;

    case 5:
        // Read or Write
        if (_I2CxSTATbits.ACKSTAT==1)         // Ack Not received, Flag error and exit
        {
            g_nvm.isr.state=17;
            _I2CxCONbits.PEN=1;  // generates interrupt
            DBG_NVM_INFO("I2C-5 NACK");
        }
        else
        {
            if (g_nvm.isr.cmd == 1)
            {
                // Send data byte
                g_nvm.isr.state=7;
                _I2CxTRN = g_nvm.isr.buf[g_nvm.isr.index++];  // generates interrupt
                DBG_NVM_INFO("I2C-5 Wr");
            }
            else if (g_nvm.isr.cmd == 2)
            {
                // Repeat Start
                g_nvm.isr.state=9;
                _I2CxCONbits.RSEN=1; // resend; generates interrupt
                DBG_NVM_INFO("I2C-5 Repeat Start");
            }
        }
        break;

        // ----------------
        // Write Data Phase                 
        // ----------------
    case 7:
        // Look for end of data or no Ack
        if (_I2CxSTATbits.ACKSTAT==1)         // Ack Not received, Flag error and exit
        {
            g_nvm.isr.state=17;
            _I2CxCONbits.PEN=1;  // generates interrupt
            DBG_NVM_INFO("I2C-7 NACK");
            break;
        }
        else
        {
            if (g_nvm.isr.index >= g_nvm.isr.n)
            {   
                // close the frame
                g_nvm.isr.state=15;
                _I2CxCONbits.PEN=1;  // stop bit; generates interrupt
                DBG_NVM_INFO("I2C-7 Close");
                break;
            }
        }
        // Send data byte
        g_nvm.isr.state=7;
        _I2CxTRN = g_nvm.isr.buf[g_nvm.isr.index++];  // generates interrupt
        DBG_NVM_INFO("I2C-7");
        break;

        // ---------------
        // Read Data Phase                  
        // ---------------
    case 9:
        // Re-send control byte with W/R=R
        g_nvm.isr.state=10;
        _I2CxTRN = NVM_I2C_ADDRESS | 1;  // read bit; generates interrupt
        DBG_NVM_INFO("I2C-9 RSND");
        break;

    case 10:
        // Check, if control byte went ok
        if (_I2CxSTATbits.ACKSTAT==1)         // Ack Not received, Flag error and exit
        {
            g_nvm.isr.state=17;
            _I2CxCONbits.PEN=1;  // generates interrupt
            DBG_NVM_INFO("I2C-10 NACK");
            break;
        }
        // Receive Enable
        g_nvm.isr.state=12;
        _I2CxCONbits.RCEN=1; // receive enable; generates interrupt
        DBG_NVM_INFO("I2C-10");
        break;

    case 12:
        // Receive data
        g_nvm.isr.state=13;
        g_nvm.isr.buf[g_nvm.isr.index++] = _I2CxRCV;  // receive data byte
        if (g_nvm.isr.index >= g_nvm.isr.n)
        {
            _I2CxCONbits.ACKDT=1;        // No ACK; signal done
            DBG_NVM_INFO("I2C-12 NACK");
        }
        else
        {
            _I2CxCONbits.ACKDT=0;        // ACK
            DBG_NVM_INFO("I2C-12 ACK");
        }
        _I2CxCONbits.ACKEN = 1;			// ack enable; generates interrupt
        break;

    case 13:
        if (g_nvm.isr.index >= g_nvm.isr.n)
        {
            g_nvm.isr.state=15;
            _I2CxCONbits.PEN=1;  // stop bit; generates interrupt
            DBG_NVM_INFO("I2C-13 Done");
        }
        else
        {
            // Receive Enable
            g_nvm.isr.state=12;
            _I2CxCONbits.RCEN=1; // receive enable; generates interrupt
            DBG_NVM_INFO("I2C-13 More");
        }
        break;

        // -------------
        // Stop Sequence                    
        // -------------
    case 15:
        g_nvm.isr.state    = 0;   // idle
        g_nvm.isr.status   = NVM_STATUS_OK;
        g_nvm.isr.index    = 0;
        g_nvm.isr.nretries = 0;
        DBG_NVM_INFO("I2C-15");
        break;

    case 17:
        g_nvm.isr.state    = 0; // idle
        g_nvm.isr.status   = NVM_STATUS_ERR;
        g_nvm.isr.index    = 0;
        g_nvm.isr.nretries = 0;
        DBG_NVM_ERR("I2C-17 Err");
        break;

    case 19:
        g_nvm.isr.index=0;
        // Start Condition
        g_nvm.isr.state=2;  // retry
        _I2CxCONbits.SEN=1;
        break;
    } // switch
	
    _MI2CxIF = 0;  // Clear the I2C Interrupt Flag;
}

// -----------------------------------------------------------------------------
// start the write operation to non-volatile memory
// returns: 0=ok, 1=bad inputs, 2=driver is busy
// CAUTION! data buffer must remain available until transmit completion
// Level 0 driver
// need to wait until !nvm_IsIsrBusy()
int nvm_L0_BegWritePage(uint16_t addr, uint8_t  nbytes, uint8_t* data)
{
//  LOG(SS_NVM, SV_INFO, "BegWritePage(addr=%u,n=%u)", (unsigned)addr, (unsigned)nbytes);

    // range check inputs
    if (nbytes > NVM_PAGE_BYTES || addr >= NVM_BYTES_TOTAL)
    {
        LOG(SS_NVM, SV_ERR, "BegWrPg n=%u a=%u", (uint16_t)nbytes, addr);
        return(1);
    }
    // check if driver is busy
    if (NVM_STATUS_BUSY == g_nvm.isr.status) 
    {
        LOG(SS_NVM, SV_ERR, "BegWrPg Busy");
        return(2);
    }
    g_nvm.isr.status   = NVM_STATUS_BUSY;   // driver is now busy

    // save input parameters
    g_nvm.isr.buf      = data;
    g_nvm.isr.n        = nbytes;
    g_nvm.isr.addr     = addr;
    // init state flags for interrupt routine
    g_nvm.isr.index    = 0;
    g_nvm.isr.nretries = 0;
    g_nvm.isr.cmd      = 1;
    // prime the hardware
    g_nvm.isr.state    = 2;
    _I2CxCONbits.SEN    = 1;   // send start bit; generates interrupt
    return(0);
}

// -----------------------------------------------------------------------------
// start the read operation from non-volatile memory
// returns: 0=ok, 1=bad inputs
// CAUTION! data buffer must remain available until receive completion
// Note: read can be done on entire nvm chip, rather than just a page
// Level 0 driver
// need to wait until !nvm_IsIsrBusy()
int nvm_L0_BegReadBuf(uint16_t addr, uint16_t  nbytes, uint8_t* data)
{
//  LOG(SS_NVM, SV_INFO, "ReadBuf(addr=%u,n=%u)", (unsigned)addr, (unsigned)nbytes);

    // range check inputs
    if (nbytes > NVM_BYTES_TOTAL || addr >= NVM_BYTES_TOTAL)
    {
        LOG(SS_NVM, SV_ERR, "BegRdBf n=%u a=%u", (uint16_t)nbytes, addr);
        return(1);
    }
    // check if driver is busy
    if (NVM_STATUS_BUSY == g_nvm.isr.status)
    {
        LOG(SS_NVM, SV_ERR, "BegRBuf Busy");
        return(2);
    }
    g_nvm.isr.status   = NVM_STATUS_BUSY;   // driver is now busy

    // save input parameters
    g_nvm.isr.buf      = data;
    g_nvm.isr.n        = nbytes;
    g_nvm.isr.addr     = addr;
    // init state flags for interrupt routine
    g_nvm.isr.index    = 0;
    g_nvm.isr.nretries = 0;
    g_nvm.isr.cmd      = 2;
    // prime the hardware
    g_nvm.isr.state    = 2;
    _I2CxCONbits.SEN    = 1;   		// send start bit; generates interrupt
    return(0);
}

// -----------------------------------------------------------------------------
// multi-page write
// returns: 0=ok, 1=input error
int nvm_L0_BegWriteBuf(uint16_t addr, uint16_t  nbytes, uint8_t* data)
{
    if (nbytes < 1)
    {
        LOG(SS_NVM, SV_ERR, "BegWrByf n=%u", (uint16_t)nbytes);
        return(1);
    }
    g_nvm.mpwr.nToWrite = 0;
    g_nvm.mpwr.nWritten = 0;
    g_nvm.mpwr.nLeft    = nbytes;

    g_nvm.mpwr.addr     = addr;
    g_nvm.mpwr.nbytes   = nbytes;
    g_nvm.mpwr.data     = data;

    g_nvm.mpwr.state    = 1;   // do this last to start the operation
    return(0);
}

// -----------------------------------------------------------------------------
//            P R O G R A M M I N G   F L A S H   R O U T I N E S
// -----------------------------------------------------------------------------

// this flag resides in flash and is rewritten to 0 during execution
// note that changing a 1 to a 0 does not require erasing flash memory
__prog__ uint16_t s_isJustFlashed __attribute__((space(prog),aligned(4))) = 1;

// -----------------------------------------------------------------------------
void flash_ClearRomFlag()
{
    unsigned int tbloffset;
    unsigned int zero = 0; // set value 

    // single word to FLASH
    TBLPAG    = __builtin_tblpage(&s_isJustFlashed);  // Load upper FLASH address
    tbloffset = __builtin_tbloffset(&s_isJustFlashed);  // offset within page
    __builtin_tblwtl(tbloffset, zero);  // load the write buffer
    NVMCON = 0x4003;        // Setup NVMCON for Single Word Write operation

    // unlock sequence
    __builtin_disi(5);      // Disable interrupts for 5 cycles
    NVMKEY = 0x55;          // Write requisite KEY values for FLASH access
    NVMKEY = 0xAA;
    NVMCONbits.WR = 1;      // Activate the erase action
    __builtin_nop();        // Requires two nops for time delay
    __builtin_nop();
    NVMCONbits.WR = 0;      // Clear the write action bit
}

// -----------------------------------------------------------------------------
//            I N I T I A L I Z A T I O N  /  C O N F I G U R A T I O N
// -----------------------------------------------------------------------------
// initialize I2C bus 2 which is used for NVM transport
static void I2C_init()
{
    TRISFbits.TRISF4 = 1;   //  RF4/CN17/U2RX   pin-31 i/o      SDA2 (NVM)
    TRISFbits.TRISF5 = 0;   //  RF5/CN18/U2TX   pin-32 output   SCL2 (NVM)
    _I2CxMD  = 0;   		// enable peripheral

    _I2CxCONbits.I2CEN  = 0; // disable while configuring

    _I2CxCONbits.A10M   = 0; 	// 7 bit I2C address
    _I2CxCONbits.SCLREL = 1; 	// no clock stretch

    _I2CxBRG = NVM_BAUD_RATE;
    _I2CxADD = NVM_I2C_ADDRESS;

    LOG(SS_NVM, SV_INFO, "I2C_init");
}

// -----------------------------------------------------------------------------
// configure non-volatile memory driver
void nvm_Config()
{
    // iniitalize data structures
    memset(&g_nvm, 0, sizeof(g_nvm));

    // load nvm rom settings into active settings
    nvm_LoadRomSettings();

    // initialize hardware transport
    I2C_init();

    LOG(SS_NVM, SV_INFO, "Config size=%u", sizeof(NVM_LAYOUT_t));
}

// -----------------------------------------------------------------------------
// wait for nvm isr to complete; enough for one page write
// returns: 0=ok, 1=timed-out
// CAUTION! blocking; use only during startup
static int nvm_WaitIsrDone()
{
    SYSTICKS startTicks = GetSysTicks();
    for (;;)
    {
        if (!nvm_IsIsrBusy()) break;
        if (IsTimedOut(25,startTicks)) return(1);  // 12 msec nominal
    }
    return(0);
}

// -----------------------------------------------------------------------------
// validate that NVM has two good copies
// CAUTION! this is blocking and is called only on startup
// on entry: Config contains ROM defaults
// on exit:  Config contains valid data
static void nvm_ValidateNVM()
{
    int      rc, cs1ok, cs2ok, alg, rev1, rev2, useCopy1=0, needToSave=1;
    uint16_t csCalc1, csRead1, csCalc2, csRead2;
    SYSTICKS startTicks = GetSysTicks();

    LOG(SS_NVM, SV_INFO, "Validate: newFW=%u", s_isJustFlashed);

    // if firmware has been just flashed, force to use ROM settings
    if (s_isJustFlashed)
    {
        LOG(SS_NVM, SV_INFO, "New Firmware detected; Using ROM defaults!");
        flash_ClearRomFlag();
        nvm_EraseAll();
        nvm_VerifyErased();
        nvm_ReqSaveConfig();
        nvm_ApplySettings();
        return;
    }

    // read copy 1 into scratch
  #ifdef DEBUG_NVM_VALIDATE
    LOG(SS_NVM, SV_INFO, "VAL: Read Copy1 offset=%u", NVM_OFFSET_COPY1);
  #endif
    nvm_L0_BegReadBuf(NVM_OFFSET_COPY1, sizeof(g_nvm.scratch), (uint8_t*)&g_nvm.scratch);
    rc = nvm_WaitIsrDone();
    if (rc) 
    { 
        LOG(SS_NVM, SV_ERR, "VAL: Read Copy1 FAILED");
        LOGX(SS_NVM, SV_ERR, "[1] ", (uint8_t*)&g_nvm.scratch, sizeof(g_nvm.scratch));
    }
    rev1    = g_nvm.scratch.hdr.revNum;
    csCalc1 = nvm_CalcChecksum((uint8_t*)&g_nvm.scratch.hdr.nBytes, NVM_CHECKSUM_SIZE);
    csRead1 = g_nvm.scratch.hdr.chksumCfg;
    cs1ok   = (nvm_IsSigValid(&g_nvm.scratch.hdr) && (csCalc1 == csRead1)) ? 1 : 0;
  #ifdef DEBUG_NVM_VALIDATE
    LOGX(SS_NVM, SV_INFO, "[1] ", (uint8_t*)&g_nvm.scratch, sizeof(g_nvm.scratch));
  #endif
    
    // read copy 2 into scratch
  #ifdef DEBUG_NVM_VALIDATE
    LOG(SS_NVM, SV_INFO, "VAL: Read Copy2 offset=%u", NVM_OFFSET_COPY2);
  #endif
    nvm_L0_BegReadBuf(NVM_OFFSET_COPY2, sizeof(g_nvm.scratch), (uint8_t*)&g_nvm.scratch);
    rc = nvm_WaitIsrDone();
    if (rc) 
    {
        LOG(SS_NVM, SV_ERR, "VAL: Read Copy2 FAILED");
        LOGX(SS_NVM, SV_INFO, "[2] ", (uint8_t*)&g_nvm.scratch, sizeof(g_nvm.scratch));
    }
    rev2    = g_nvm.scratch.hdr.revNum;
    csCalc2 = nvm_CalcChecksum((uint8_t*)&g_nvm.scratch.hdr.nBytes, NVM_CHECKSUM_SIZE);
    csRead2 = g_nvm.scratch.hdr.chksumCfg;
    cs2ok   = (nvm_IsSigValid(&g_nvm.scratch.hdr) && (csCalc2 == csRead2)) ? 2 : 0;
  #ifdef DEBUG_NVM_VALIDATE
    LOGX(SS_NVM, SV_INFO, "[2] ", (uint8_t*)&g_nvm.scratch, sizeof(g_nvm.scratch));
  #endif

    // determine copy algorithm
    alg = cs1ok | cs2ok;
  #ifdef DEBUG_NVM_VALIDATE
    LOG(SS_NVM, SV_INFO, "CS1 Rd=%04X:%04X=Calc  CS2 Rd=%04X:%04X=Calc", csRead1, csCalc1, csRead2, csCalc2);
  #endif
    switch(alg)
    {
    case 0:
        // ROM defaults are already in Config
        LOG(SS_NVM, SV_ERR, "Both NVM Copies Bad; Using Default ROM Values"); 
        g_nvm.revNum = 1;    // start at 1
        Device.error.nvm_is_bad = 1;    // mark nvm chip as bad
        break;
    case 1:
        LOG(SS_NVM, SV_WARN, "NVM Copy 1 Good; Copy 2 Bad; Using Copy 1");
        g_nvm.revNum = rev1;
        useCopy1 = 1;
        break;
    case 2:
        LOG(SS_NVM, SV_WARN, "NVM Copy 2 Good; Copy 1 Bad; Using Copy 2");
        memcpy(&g_nvm.settings, &g_nvm.scratch.settings, sizeof(g_nvm.settings) );
        g_nvm.revNum = rev2;
        break;
    case 3: // both NVM copies are good; use highest revision
        if (rev1 == rev2)
        {
            LOG(SS_NVM, SV_INFO, "Both NVM Copies Good");
            memcpy(&g_nvm.settings, &g_nvm.scratch.settings, sizeof(g_nvm.settings) );
            needToSave = 0; // both copies are good and the same; no need to save
            g_nvm.revNum = rev1;
            break;
        }
        if (rev1 < rev2)
        {
            LOG(SS_NVM, SV_WARN, "NVM Copy 1 older than Copy 2; Using Copy 2");
            memcpy(&g_nvm.settings, &g_nvm.scratch.settings, sizeof(g_nvm.settings) );
            g_nvm.revNum = rev2;
        }
        else
        {
            LOG(SS_NVM, SV_WARN, "NVM Copy 2 older than Copy 1; Using Copy 1");
            g_nvm.revNum = rev1;
            useCopy1 = 1;
        }
        break;
    } // switch

    // may have to reread copy1; we only have one scratch buffer
    if (useCopy1)
    {
        // red copy 1
        nvm_L0_BegReadBuf(NVM_OFFSET_COPY1, sizeof(g_nvm.scratch), (uint8_t*)&g_nvm.scratch);
        rc = nvm_WaitIsrDone();
        if (rc)
        {
            LOG(SS_NVM, SV_ERR, "VAL: Read Copy1 ReRead FAILED");
        }
        else
        {
            // make it active
            memcpy(&g_nvm.settings, &g_nvm.scratch.settings, sizeof(g_nvm.settings) );
        }
    }

    // set the active flags from the configuration values
    nvm_ApplySettings();

    // show results
    LOG(SS_NVM, SV_INFO, "Rev=%u Validate Done %lu msec", g_nvm.revNum, GetSysTicks()-startTicks);  // 13 msec for 242 size

    // set save flag if need be
    if (needToSave)
        nvm_ReqSaveConfig();
}

// -----------------------------------------------------------------------------
// start nvm driver
void nvm_Start()
{
    _MI2CxIE  = 1;   	// Master Interrupt Enable
    _MI2CxIF  = 0;   	// clear
    _I2CxCONbits.I2CEN = 1;  	// enable I2C

    LOG(SS_NVM, SV_INFO, "Start");
    nvm_ValidateNVM();
}

// -----------------------------------------------------------------------------
// stop nvm driver
void nvm_Stop()
{
    _MI2CxIE  = 0;   	// Master Interrupt Disable
    _MI2CxIF  = 0;   	// clear
    _I2CxCONbits.I2CEN = 0;   	// disable I2C

//  LOG(SS_NVM, SV_INFO, "Stop");
}

// -----------------------------------------------------------------------------
//                 L E V E L    1    D R I V E R
// -----------------------------------------------------------------------------
// request saving config to nvm
void nvm_ReqSaveConfig(void)
{
    g_nvm.reqCmd = NVM_CMD_SAVE_CFG;
}

// -----------------------------------------------------------------------------
// validate nvm
// caution! blocking function
/*
void nvm_Test(void)
{
    SYSTICKS startTicks;
    nvm_L0_BegWriteBuf(NVM_OFFSET_COPY1+50, 3, (char*)"BAD");
    LOG(SS_NVM, SV_INFO, "Corrupt Copy 1");
    startTicks = GetSysTicks();
    for (;;)
    {
        TASK_nvm_Driver();
        if (IsTimedOut(100,startTicks)) break;
    }

    nvm_L0_BegWriteBuf(NVM_OFFSET_COPY2+50, 3, (char*)"BAD");
    LOG(SS_NVM, SV_INFO, "Corrupt Copy 2");
    startTicks = GetSysTicks();
    for (;;)
    {
        TASK_nvm_Driver();
        if (IsTimedOut(100,startTicks)) break;
    }
}
*/

// -----------------------------------------------------------------------------
// calculate checksum for nvm purposes
uint16_t nvm_CalcChecksum(uint8_t* data, int16_t len)
{
	uint16_t cs;
	int i;
	
	for (cs=0,i=0; i<len; i++)
	{
		cs += (uint16_t)data[i];
	}
	return (cs);
} 

// -----------------------------------------------------------------------------
// initialize nvm header block
void nvm_InitHeader(NVM_HDR_BLK_t* pHdr)
{
    pHdr->sigA = NVM_SIGA;
    pHdr->sigB = NVM_SIGB;

    pHdr->nBytes = sizeof(NVM_SETTINGS_t);

    memset(pHdr->reserved, 0 , sizeof(pHdr->reserved));
}

// -----------------------------------------------------------------------------
// returns 0=no, 1=signature is valid
int nvm_IsSigValid(NVM_HDR_BLK_t* pHdr)
{
    return(pHdr->sigA == NVM_SIGA && pHdr->sigB == NVM_SIGB);
}

// -----------------------------------------------------------------------------
// drive multi-packet writes in background
static void nvm_DriveMultiPacketWrites()
{
    switch (g_nvm.mpwr.state)
    {
    default:
        LOG(SS_NVM, SV_ERR, "MPWR invalid state=%u", g_nvm.mpwr.state);
        g_nvm.mpwr.state = 0;
        g_nvm.mpwr.nLeft = 0;
        break;
    case 2: // wait for delay
        if (!IsTimedOut(NVM_WRITE_DELAY, g_nvm.mpwr.startTicks)) break;
        g_nvm.mpwr.nWritten += g_nvm.mpwr.nToWrite;
        g_nvm.mpwr.addr     += g_nvm.mpwr.nToWrite;
        g_nvm.mpwr.nLeft    -= g_nvm.mpwr.nToWrite;
        if (g_nvm.mpwr.nLeft < 1)
        {
            g_nvm.mpwr.nLeft = 0;
            g_nvm.mpwr.state = 0;   // done
          #ifdef DEBUG_NVM_MP_WRITES
            LOG(SS_NVM, SV_INFO, "MPWR done #written=%u", g_nvm.mpwr.nWritten);
          #endif
            break;
        }
      #ifdef DEBUG_NVM_MP_WRITES
        LOG(SS_NVM, SV_INFO, "MPWR[2] nLeft=%u", g_nvm.mpwr.nLeft);
      #endif
        g_nvm.mpwr.state = 1;
        // fall thru
    case 1:
        g_nvm.mpwr.nToWrite = NVM_BYTES_TO_PAGE_END(g_nvm.mpwr.addr);
        if (g_nvm.mpwr.nToWrite > g_nvm.mpwr.nLeft) 
            g_nvm.mpwr.nToWrite = g_nvm.mpwr.nLeft;
      #ifdef DEBUG_NVM_MP_WRITES
        LOG(SS_NVM, SV_INFO, "MPWR[1] nToWrite=%u", g_nvm.mpwr.nToWrite);
      #endif
        nvm_L0_BegWritePage(g_nvm.mpwr.addr, (uint8_t)g_nvm.mpwr.nToWrite, &g_nvm.mpwr.data[g_nvm.mpwr.nWritten]);
        g_nvm.mpwr.startTicks = GetSysTicks();
        g_nvm.mpwr.state = 2;
        break;
    } // switch
}


// -----------------------------------------------------------------------------
// save configuration to nvm state machine
void nvm_SaveConfig()
{
    switch (g_nvm.cmdState)
    {
    default:
    case 0:
        g_nvm.curCmd = NVM_CMD_NONE;
        break;

    case 1:
        // populate layout for writing
        g_nvm.revNum++; // bump rev number when writing
        nvm_InitHeader(&g_nvm.scratch.hdr);
        g_nvm.scratch.hdr.revNum = g_nvm.revNum;
        memcpy(&g_nvm.scratch.settings, &g_nvm.settings, sizeof(g_nvm.settings));
        memset(&g_nvm.scratch.settings.future_use, NVM_FILL_BYTE, sizeof(g_nvm.scratch.settings.future_use));
        g_nvm.scratch.hdr.chksumCfg = nvm_CalcChecksum((uint8_t*)&g_nvm.scratch.hdr.nBytes, NVM_CHECKSUM_SIZE);
        LOG(SS_NVM, SV_INFO, "Save copy1");
        // start copy1 write
        nvm_L0_BegWriteBuf(NVM_OFFSET_COPY1, sizeof(g_nvm.scratch), (uint8_t*)&g_nvm.scratch);
        g_nvm.cmdState = 2;
        // caution! do not fall thru here
        break;

    case 2:
        // start copy2 write
        nvm_L0_BegWriteBuf(NVM_OFFSET_COPY2, sizeof(g_nvm.scratch), (uint8_t*)&g_nvm.scratch);
        LOG(SS_NVM, SV_INFO, "Save copy2");
        g_nvm.saveTicks = GetSysTicks();    // time statmp
        g_nvm.cmdState = 0; // done
        g_nvm.isDirty = 0; // clear dirty bit
        break;
    } // switch
}

// -----------------------------------------------------------------------------
// erase all of NVM memory to FF's
// CAUTION! this test is blocking; run during startup only
static void nvm_EraseAll()
{
    uint8_t   testBuf[NVM_PAGE_BYTES+2];
    SYSTICKS  startTicks, delayTicks;
    uint16_t  addr, nwerrs=0;
    uint8_t   value; // contents of byte
    int       np;

    addr  = 0;
    value = 0;
    // fill buffer with erase pattern
    memset(testBuf, NVM_ERASE_BYTE, sizeof(testBuf));
    startTicks = GetSysTicks(); // start timer
    for (np=0; np<NVM_PAGES; np++)
    {
        // chip needs delay for writing
        delayTicks = GetSysTicks();
        while (!IsTimedOut(NVM_WRITE_DELAY,delayTicks)) { }

        // begin the writing of page
        nvm_L0_BegWritePage(addr, NVM_PAGE_BYTES, testBuf);
        addr += NVM_PAGE_BYTES;  // next address

        // wait for write to complete
        delayTicks = GetSysTicks();
        while (nvm_IsIsrBusy())
        { 
            if (IsTimedOut(20,delayTicks))
            {
                LOG(SS_NVM, SV_ERR, "EraseAll Timed-out page=%u", np);
                g_nvm.isr.status = NVM_STATUS_ERR;
                break;
            }
        }

        // completed
        if (nvm_IsError()) nwerrs++; // check for errors
    } // for

    // delay to finish the write
    delayTicks = GetSysTicks();
    while (!IsTimedOut(NVM_WRITE_DELAY,delayTicks)) { }
#if !defined(OPTION_NO_LOGGING)
    if (nwerrs==0)
        LOG(SS_NVM, SV_INFO, "Erase All ok");
    else
        LOG(SS_NVM, SV_ERR,  "Erase All with %u errors", nwerrs);
#endif
}

// -----------------------------------------------------------------------------
// verify erased
// CAUTION! this test is blocking; run during startup only
static void nvm_VerifyErased()
{
    uint8_t   testBuf[NVM_PAGE_BYTES+2];
    SYSTICKS  startTicks, delayTicks;
    uint16_t  addr, nrerrs;
    int       i, np;

    LOG(SS_NVM, SV_INFO, "Verify Erasure");
    addr = nrerrs = 0;
    startTicks = GetSysTicks(); // start timer
    for (np=0; np<NVM_PAGES; np++)
    {
        memset(testBuf,0,sizeof(testBuf)); // clear so it does not match
        // begin the reading of page
        nvm_L0_BegReadBuf(addr, NVM_PAGE_BYTES, testBuf);
        addr += NVM_PAGE_BYTES;  // next address

        // wait for read to complete
        delayTicks = GetSysTicks();
        while (nvm_IsIsrBusy()) 
        { 
            if (IsTimedOut(10,delayTicks))
            {
                LOG(SS_NVM, SV_ERR, "Verify Erase: Read Timed-out page=%u", np);
                g_nvm.isr.status = NVM_STATUS_ERR;
                break;
            }
        }

        // completed
        if (nvm_IsError()) nrerrs++; // check for errors

        // verify all bytes are ereased on this page
        for (i=0; i<NVM_PAGE_BYTES; i++)
        {
            if (testBuf[i] != NVM_ERASE_BYTE) nrerrs++;
        }
    } // for
#if !defined(OPTION_NO_LOGGING)
    if (nrerrs < 1)
        LOG(SS_NVM, SV_INFO, "Erased Ok");
    else
        LOG(SS_NVM, SV_ERR,  "Erase Failed %u bytes", nrerrs);
#endif
}

//-----------------------------------------------------------------------------
// load NVM Rom settings in active settings
void nvm_LoadRomSettings()
{
	g_nvm.settings.can	= CanConfigDefaults;
	g_nvm.settings.dev 	= DeviceConfigDefaults;
	g_nvm.settings.inv	= InvConfigDefaults;
	g_nvm.settings.chgr = ChgrConfigDefaults;
	g_nvm.settings.conv = ConvConfigDefaults;
	
    memset(&g_nvm.settings.future_use, NVM_FILL_BYTE, sizeof(g_nvm.settings.future_use));
}

//------------------------------------------------------------------------------
// apply active nvm settings to system
void nvm_ApplySettings()
{
    // apply can bus settings
    g_can.baud     = g_nvm.settings.can.baud;
    g_can.address  = g_nvm.settings.can.address;
    g_can.instance = g_nvm.settings.can.instance;
  #ifdef DEBUG_NVM_VALIDATE
    LOG(SS_NVM, SV_INFO, "Apply CAN: baud=%u address=%u instance=%u",
        (unsigned)g_nvm.settings.can_baud, (unsigned)g_nvm.settings.can_address, (unsigned)g_nvm.settings.can_instance);
  #endif
  
	Device.config 	= g_nvm.settings.dev;
	Inv.config 		= g_nvm.settings.inv;	
	Chgr.config 	= g_nvm.settings.chgr;
  
  #if IS_DEV_CONVERTER
    Cnv.dc_set_point_adc = g_nvm.settings.conv.dc_set_point_adc;
  #endif
}

// -----------------------------------------------------------------------------
//                      N V M    D R I V E R 
// -----------------------------------------------------------------------------

// needs to be called periodically to keep state machine running
void TASK_nvm_Driver()
{
    // allow isr level 0 activity to complete
    if (nvm_IsIsrBusy()) return;

    // perform any pending multi-packet writes
    if (nvm_IsMwBusy())
    {
        nvm_DriveMultiPacketWrites();
        return;
    }

    // perform any commands in progress
    if (nvm_IsCmdInProg())
    {
        switch (g_nvm.curCmd)
        {
        default: 
            g_nvm.curCmd = NVM_CMD_NONE;
            break;
        case NVM_CMD_SAVE_CFG:
            nvm_SaveConfig();
            break;
        } // switch
        return;
    }

    // check for any requested commands
    if (nvm_IsReqCmd())
    {
        // start the command
        switch(g_nvm.reqCmd)
        {
        case NVM_CMD_SAVE_CFG:    // write cfg to nvm
          #ifdef DEBUG_NVM_CMDS
            LOG(SS_NVM, SV_INFO, "Save NVM requested");
          #endif
            g_nvm.curCmd = g_nvm.reqCmd;
            g_nvm.cmdState = 1;  // start at beginning
            break;
        } // switch
        g_nvm.reqCmd = 0;    // clear request
        return;
    }

    // handle dirty bit
    if (nvm_IsDirty())
    {
        g_nvm.isDirty = 0;
        nvm_ReqSaveConfig();
    }
}

// -----------------------------------------------------------------------------
//                     N V M    U N I T    T E S T 
// -----------------------------------------------------------------------------

// CAUTION! this test is destructive; will erase all of non-volatile memory
// CAUTION! this test is blocking; run during startup only
/*
void nvm_RunUnitTest()
{
    uint8_t   testBuf[NVM_PAGE_BYTES+2];
    SYSTICKS  startTicks, delayTicks;
    uint16_t  addr, nwerrs, nrerrs;
    uint8_t   value; // contents of byte
    int       i, np;

    // ------------
    // write phase
    // ------------
    LOG(SS_NVM, SV_INFO, "NVM Unit Test");
    LOG(SS_NVM, SV_INFO, "Writing Memory");
    addr = nwerrs = 0;
    value = 0;
    startTicks = GetSysTicks(); // start timer
    for (np=0; np<NVM_PAGES; np++)
    {
        // chip needs delay for writing
        delayTicks = GetSysTicks();
        while (!IsTimedOut(NVM_WRITE_DELAY,delayTicks)) { }

        // fill buffer with pattern
        for (i=0; i<NVM_PAGE_BYTES; i++)
        {
            testBuf[i] = value++;   // value will wrap
        }
        // LOGX(SS_NVM, SV_INFO, "buf", testBuf, NVM_PAGE_BYTES);

        // begin the writing of page
        nvm_L0_BegWritePage(addr, NVM_PAGE_BYTES, testBuf);
        addr += NVM_PAGE_BYTES;  // next address

        // wait for write to complete
        delayTicks = GetSysTicks();
        while (nvm_IsIsrBusy())
        { 
            if (IsTimedOut(20,delayTicks))
            {
                LOG(SS_NVM, SV_ERR, "Write Timed-out page=%u", np);
                g_nvm.isr.status = NVM_STATUS_ERR;
                break;
            }
        }

        // completed
        if (nvm_IsError()) nwerrs++; // check for errors
    } // for

    // write results
    startTicks = GetSysTicks() - startTicks;
  #if !defined(OPTION_NO_LOGGING)
    if (nwerrs)
        LOG(SS_NVM, SV_ERR,  "Write FAILED; errors=%u, %u pages in %lu msecs", (unsigned)nwerrs, NVM_PAGES, startTicks);
    else
        LOG(SS_NVM, SV_INFO, "Write PASSED; %u pages in %lu msecs", NVM_PAGES, startTicks);
  #endif
    
    // delay to finish the write
    delayTicks = GetSysTicks();
    while (!IsTimedOut(NVM_WRITE_DELAY,delayTicks)) { }

    // -----------
    // read phase
    // -----------
    LOG(SS_NVM, SV_INFO, "Reading Memory");
    addr = nrerrs = 0;
    value = 0;
    startTicks = GetSysTicks(); // start timer
    for (np=0; np<NVM_PAGES; np++)
    {
        // begin the reading of page
        nvm_L0_BegReadBuf(addr, NVM_PAGE_BYTES, testBuf);
        addr += NVM_PAGE_BYTES;  // next address

        // wait for read to complete
        delayTicks = GetSysTicks();
        while (nvm_IsIsrBusy()) 
        { 
            if (IsTimedOut(10,delayTicks))
            {
                LOG(SS_NVM, SV_ERR, "Read Timed-out page=%u", np);
                g_nvm.isr.status = NVM_STATUS_ERR;
                break;
            }
        }

        // completed
        if (nvm_IsError()) nrerrs++; // check for errors
        for (i=0; i<NVM_PAGE_BYTES; i++)
        {
            if (testBuf[i] != value)
            {
                nrerrs++;
             // LOG(SS_NVM, SV_ERR, "Read p=%u b=%u %02X!=%02X", np, i, testBuf[i], value);
            }
            value++;
        }
    } // for

    // read results
    startTicks = GetSysTicks() - startTicks;
  #if !defined(OPTION_NO_LOGGING)
    if (nrerrs)
        LOG(SS_NVM, SV_ERR,  "Read FAILED; errors=%u, %u pages in %lu msecs", (unsigned)nrerrs, NVM_PAGES, startTicks);
    else
        LOG(SS_NVM, SV_INFO, "Read PASSED; %u pages in %lu msecs", NVM_PAGES, startTicks);

    // pass/fail
    if (nrerrs || nwerrs)
        LOG(SS_NVM, SV_ERR,  "NVM Unit Test FAILED");
    else
        LOG(SS_NVM, SV_INFO, "NVM Unit Test PASSED");
  #endif
}
*/

// <><><><><><><><><><><><><> nvm.c <><><><><><><><><><><><><><><><><><><><><><>
