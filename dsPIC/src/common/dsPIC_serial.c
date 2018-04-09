// <><><><><><><><><><><><><> dsPIC_serial.c <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//  Copyright(C) 2012 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
// UART communications functionality for dsPIC33 microprocessor
//
//-----------------------------------------------------------------------------

// -------
// headers
// -------
#include "options.h"    // must be first include
#include "hw.h"
#include "dsPIC_serial.h"
#include <stdarg.h>


//-----------------------------------------------------------------------------
//	There was a Hardware issue on the NP and LPC boards which attempted to use
//	a CAN Driver interface (chip) connected to the UART Tx and Rx lines, for 
//	Stacking inverters. The CAN Driver interface connected the Tx and Rx lines 
//	which caused errors in the data received. THE CAN DRIVER INTERFACE MUST BE 
//	DISCONNECTED FROM THE UART FOR CORRECT OPERATION.  Later revisions have 
//	corrected this issue.
//	_DISABLE_RX_INTERRUPTS_ is defined here to disable Receive Interrupts from 
//	the UART.
//-----------------------------------------------------------------------------

#if defined(OPTION_SERIAL_IFC)
	#undef _DISABLE_RX_INTERRUPTS_
#else
	#define _DISABLE_RX_INTERRUPTS_	1
#endif

#ifdef OPTION_UART2
    // Using UART2 as debug console
    #define     _UxRXInterrupt  _U2RXInterrupt
    #define     _UxTXInterrupt  _U2TXInterrupt
    #define     UxRXIF          IFS1bits.U2RXIF
    #define     UxTXIF          IFS1bits.U2TXIF
    #define     UxTXEN          U2STAbits.UTXEN
    #define     UxTXREG         U2TXREG
    #define     UxRXREG         U2RXREG
    #define     UxPDSEL         U2MODEbits.PDSEL
    #define     UxSTSEL         U2MODEbits.STSEL
    #define     UxBRG           U2BRG
  #ifdef __dsPIC30F4011__
    #define     UxUTXISEL       U2STAbits.UTXISEL
  #else
    #define     UxUTXISEL       U2STAbits.UTXISEL0
    #define     UxUTXISEL0      U2STAbits.UTXISEL0
    #define     UxUTXISEL1      U2STAbits.UTXISEL1
  #endif
    #define     UxURXISEL       U2STAbits.URXISEL
    #define     UxTXIE          IEC1bits.U2TXIE
    #define     UxRXIE          IEC1bits.U2RXIE
    #define     UxUARTEN        U2MODEbits.UARTEN
    #define     UxTRMT          U2STAbits.TRMT
    #define     UxTXBF          U2STAbits.UTXBF
#else
    // Using UART1 as debug console
    #define     _UxRXInterrupt  _U1RXInterrupt
    #define     _UxTXInterrupt  _U1TXInterrupt
    #define     UxRXIF          IFS0bits.U1RXIF
    #define     UxTXIF          IFS0bits.U1TXIF
    #define     UxTXEN          U1STAbits.UTXEN
    #define     UxTXREG         U1TXREG
    #define     UxRXREG         U1RXREG
    #define     UxPDSEL         U1MODEbits.PDSEL
    #define     UxSTSEL         U1MODEbits.STSEL
    #define     UxBRG           U1BRG
  #ifdef __dsPIC30F4011__
    #define     UxUTXISEL       U1STAbits.UTXISEL
  #else
    #define     UxUTXISEL       U1STAbits.UTXISEL0
    #define     UxUTXISEL0      U1STAbits.UTXISEL0
    #define     UxUTXISEL1      U1STAbits.UTXISEL1
  #endif
    #define     UxURXISEL       U1STAbits.URXISEL
    #define     UxTXIE          IEC0bits.U1TXIE
    #define     UxRXIE          IEC0bits.U1RXIE
    #define     UxUARTEN        U1MODEbits.UARTEN
    #define     UxTRMT          U1STAbits.TRMT
    #define     UxTXBF          U1STAbits.UTXBF
#endif

//#define TXBUF_SIZE  1024    // bytes
#define TXBUF_SIZE  2048    // bytes
static uint8_t TxBuf[TXBUF_SIZE];

//#define RXBUF_SIZE  16      // bytes
#define RXBUF_SIZE  128
static uint8_t RxBuf[RXBUF_SIZE];

// rx / tx buffer empty
#define IS_RXBUF_EMPTY()  (RxFifo.putIn == RxFifo.takeOut)
#define IS_TXBUF_EMPTY()  (TxFifo.putIn == TxFifo.takeOut)

// Circlular buffer
#pragma pack(1)  // structure packing on byte alignement
typedef struct
{
    uint8_t* buf;       // buffer
    int16_t  putIn;     // index of next byte to put  in
    int16_t  takeOut;   // index of next byte to take out
} FIFOBUF_t;
#pragma pack()  // restore packing setting

// Circular serial buffers
static FIFOBUF_t    TxFifo; // transmit buffer
static FIFOBUF_t    RxFifo; // receive  buffer

// allow serial port waiting during initialization
static char g_serialAllowTxWait = 0;  // 0=no, 1=allow serial to wait
static char g_needNewLine       = 0;  // generate new line to accommodate '+' '-' 

//-----------------------------------------------------------------------------
//                     R E C E I V E     I N T E R R U P T
//-----------------------------------------------------------------------------

void __attribute__ ((interrupt, no_auto_psv)) _UxRXInterrupt(void) 
{
	//	check for receive errors
	if(U1STAbits.FERR == 1)
	{
//		LOG(SS_SYS, SV_INFO, "U1STAbits.FERR ");
	}
	
	while(U1STAbits.URXDA)
	{
		RxFifo.buf[RxFifo.putIn] = UxRXREG;
		if (++RxFifo.putIn >= RXBUF_SIZE) RxFifo.putIn = 0; // wrap
	}
    UxRXIF = 0; // clear rx interrupt flag; allow interrupts again
		
	//	Clear the overrun error to keep UART receiving.
	//	The data in the receive FIFO should be read prior to clearing the OERR 
	//	bit. The FIFO is reset when OERR is cleared. All data in the buffer will be lost.
	if(U1STAbits.OERR == 1)
	{
//		LOG(SS_SYS, SV_INFO, "U1STAbits.OERR ");
		U1STAbits.OERR = 0;
	}	
}

//-----------------------------------------------------------------------------
//                     T R A N S M I T     I N T E R R U P T
//-----------------------------------------------------------------------------

void __attribute__ ((interrupt, no_auto_psv)) _UxTXInterrupt(void) 
{
    //  If there is nothing to transmit, then just return
    if ( IS_TXBUF_EMPTY() )
    {
        UxTXIF = 0; // Clear tx interrupt flag; allow interrupts again
        return;
    }

    //  The transmit buffer is 9 bits wide and 4 characters deep. Including
    //  the Transmit Shift register (UxTSR), the user effectively has a 5-deep
    //  FIFO (First In First Out) buffer. The UTXBF Status bit (UxSTA<9>)
    //  indicates whether the transmit buffer is full.

    while ( !IS_TXBUF_EMPTY() && (UxTXBF == 0))
    {
        UxTXREG = TxFifo.buf[TxFifo.takeOut];  // load transmit register
        UxTXEN  = 1;    //  Transmit Enable bit
        if (++TxFifo.takeOut >= TXBUF_SIZE) TxFifo.takeOut = 0; // wrap
    }
    UxTXIF = 0; // Clear tx interrupt flag; allow interrupts again
}

//-----------------------------------------------------------------------------
void _serial_Config(void)
{
    // The buffer is empty when the head and tail point to the same location.
    // setup transmit fifo
    TxFifo.buf   = TxBuf;
    TxFifo.putIn = TxFifo.takeOut = 0;

    // setup receive fifo
    RxFifo.buf   = RxBuf;
    RxFifo.putIn = RxFifo.takeOut = 0;

    
    //-- UARTx ----------------------------------------------------------------
    //  From the dsPIC30F4011 datasheet, 18.3.1 TRANSMITTING IN 8-BIT DATA MODE
    //  The following steps must be performed in order to transmit 8-bit data:
    //   1. Set up the UART:
    //      First, the data length, parity and number of Stop bits must be 
    //      selected. Then, the transmit and receive interrupt enable and 
    //      priority bits are set up in the UxMODE and UxSTA registers. Also,
    //      the appropriate baud rate value must be written to the UxBRG reg.
    //   2. Enable the UART by setting the UARTEN bit (UxMODE<15>).
    //      Once enabled, the UxTX and UxRX pins are configured as an output 
    //      and an input, respectively, overriding the TRIS and LATCH register 
    //      bit settings for the corresponding I/O port pins. The UxTX pin is 
    //      at logic ‘1’ when no transmission is taking place.
    //   3. Set the UTXEN bit (UxSTA<10>), thereby enabling a transmission.
    //   4. Write the byte to be transmitted to the lower byte of UxTXREG. The 
    //      value will be transferred to the Transmit Shift register (UxTSR) 
    //      immediately and the serial bit stream will start shifting out during 
    //      the next rising edge of the baud clock. 
    //      Alternatively, the data byte may be written while UTXEN = 0, 
    //      following which, the user may set UTXEN. This will cause the serial 
    //      bit stream to begin immediately because the baud clock will start 
    //      from a cleared state.
    //   5. A transmit interrupt will be generated depending on the value of 
    //      the interrupt control bit UTXISEL (UxSTA<15>).
    //  
    //  This was copied from an example, as a place to start.  
    //  The Modified Olimex board has a FTDI RS232 to USB converter wired to 
    //  UARTx, so we will be configuring that port.
    
    //  UxMODE: UARTx Mode Register
    UxPDSEL = 0;    //  00 = 8-bit, No Parity
    UxSTSEL = 0;    //  One Stop Bit
    
    
    //  UARTx: Baud Rate Generator (UxBRG) - See section 19.3.1 of datasheet.
    //  UxBRG = (Fcy/(16*BaudRate))-1
    //  Baud Rate Table for ~30 MIPS
    //      Baud     BRG    Calc.   %error
    //      1200    1535    1200    0.00%
    //      2400     767    2400    0.00%
    //      4800     383    4800    0.00%
    //      9600     191    9600    0.00%
    //      19200     95   19200    0.00%
    //      38400     47   38400    0.00%
    //      115200    15  115200    0.00%
    //      230400     7  230400    0.00%
    //
    //  Baud Rate Table for 40.0 MIPS
    //      Baud     BRG    Calc.   %error
    //      1200    2082   1200.706  0.06%
    //      2400    1040   2403.784  0.16%
    //      4800    519    4816.893  0.35%
    //      9600    260    9615.322  0.16%
    //     19200    129   19379.782  0.94%
    //     38400     64   39062.438  1.73%
    //    115200     21  119047.557  3.34% <-- tested to be more reliable
    //    115200     22  113636.301 -1.36%
    //    230400     11  227272.665 -1.36%

  #ifdef __dsPIC30F4011__
    #ifdef OPTION_BAUD_9600
        UxBRG = 191;  // 9600 Baud 
    #else
        UxBRG = 15;   // 115200 Baud
    #endif
  #else
    #ifdef OPTION_BAUD_9600
        UxBRG = 260;  // 9600 Baud 
    #else
        UxBRG = 21;   // 115200 Baud
    #endif
  #endif
  
  
   #ifdef __dsPIC30F4011__
    UxUTXISEL = 1;
  #else
	//	NOTE: FOR dsPIC33FJ UxUTXISEL1 IS BIT 13, UxUTXISEL2 IS BIT 15
	
    //  UTXISEL<1:0> Transmission Interrupt Mode Selection bits
	//	11 = Reserved
	//	10 = Interrupt is generated when a character is transferred to the 
	//		 Transmit Shift register and the transmit buffer becomes empty
	//	01 = Interrupt is generated when the last transmission is over (that 
	//		 is, the last character is shifted out of the Transmit Shift 
	//		 register) and all the transmit operations are completed.
	//	00 = Interrupt generated when any character is transferred to the 
	//		 Transmit Shift register (which implies at least one location is 
	//		 empty in the transmit buffer) bit 14
    UxUTXISEL0 = 0;
    UxUTXISEL1 = 1;
  #endif
  
	//	URXISEL<1:0>: Receive Interrupt Mode Selection bits
	//	   11 = Interrupt flag bit is set when the receive buffer is full (that 
	//			is, receive buffer has 4 data characters)
	//	   10 = Interrupt flag bit is set when the receive buffer is 3/4 full 
	//			(that is, receive buffer has 3 data characters.)
	//     0x = Interrupt flag bit is set when a character is received
    UxURXISEL = 0;  //  Receive Interrupt Mode Selection bit
}

//-----------------------------------------------------------------------------
void _serial_Start(void)
{
    SYSTICKS startTicks;
	uint16_t junk;

    // Clear UARTx interrupts flags
    UxTXIF = 0; // Clear the Transmit Interrupt Flag
    UxRXIF = 0; // Clear the Receive  Interrupt Flag

    // Enable UARTxall interrupts
    UxTXIE = 1; // Enable Transmit Interrupts

	//	Empty trash from the Rx Buffer before enabling interrupts.
	while(U1STAbits.URXDA) junk = UxRXREG;
  #if defined(_DISABLE_RX_INTERRUPTS_)
    UxRXIE = 0; // Disable Receive Interrupts
  #else
    UxRXIE = 1; // Enable Receive Interrupts
  #endif
  
    //  Once enabled, the UxTX and UxRX pins are configured as output and 
    //  input, respectively, overriding the TRIS and PORT register bit 
    //  settings for the corresponding I/O port pins. 
    UxUARTEN = 1;   //  Turn UARTx on

    // allow uart to settle
    startTicks = GetSysTicks();
    while (!IsTimedOut(100,startTicks)) { };
    
    //  sometimes requires a few characters before the serial port synchronizes.
    g_serialAllowTxWait = 1;  // allow blocking during startup
    serial_puts("\r\n\r\n\r\n");
}

//-----------------------------------------------------------------------------
// stop serial interrupts
void _serial_Stop(void)
{
    UxTXIF = 0;    // Clear the Transmit Interrupt Flag
    UxRXIF = 0;    // Clear the Receive  Interrupt Flag
    UxTXIE = 0;    // Disable Transmit   Interrupts
    UxRXIE = 0;    // Disable Receive    Interrupts
    UxUARTEN = 0;  //  Turn UARTx off
}

//-----------------------------------------------------------------------------
void _serial_RunMode(void)
{
    g_serialAllowTxWait = 0;  // dont allow blocking
}

//-----------------------------------------------------------------------------
//  _serial_printf()
//
//  Return Value
//      On success, the total number of characters written is returned.
//      On failure, a negative number is returned.
//-----------------------------------------------------------------------------

#ifndef _OMIT_PRINTF

int16_t _serial_printf (char * fmt, ...)
{
    char buf[100];  // keep stack usage as small as possible
    va_list args;
    int len;

    va_start (args, fmt);
    len = vsprintf(buf, fmt, args);
    if (len > 0)
    {
        if (0 != _serial_putbuf((unsigned char *)buf, len))
            len = -1;
    }
    va_end (args);
    
    return(len);
}

#endif  //  _OMIT_PRINTF


//-----------------------------------------------------------------------------
//  _serial_getc()
//
//  Return Value
//      On success, the character read is returned (promoted to an int value).
//      On failure, EOF.
//-----------------------------------------------------------------------------

int16_t _serial_getc(void)
{
    static int16_t ch;

    if (IS_RXBUF_EMPTY()) return(EOF);
    
    ch = RxFifo.buf[RxFifo.takeOut];
    if (++RxFifo.takeOut >= RXBUF_SIZE) RxFifo.takeOut = 0; // wrap
    return(ch);
}

//-----------------------------------------------------------------------------
//  _serial_TxBE() - Transmit Buffer Empty
//
//  Return Value
//      0 = Transmit Buffer has data
//      1 = Transmit Buffer is empty
//-----------------------------------------------------------------------------

int16_t _serial_TxBE(void)
{
    return (IS_TXBUF_EMPTY() ? 1 : 0);
}

int8_t _serial_IsTxBuffEmpty(void) { return((int8_t)IS_TXBUF_EMPTY());	} 

//-----------------------------------------------------------------------------
// return number of bytes available in transmit buffer
int16_t _serial_TxFreeSpace()
{
    if (IS_TXBUF_EMPTY()) return(TXBUF_SIZE-1);   // one less than full buffer
    if (TxFifo.putIn > TxFifo.takeOut)
        return(TXBUF_SIZE - TxFifo.putIn + TxFifo.takeOut - 1);
    return(TxFifo.takeOut - TxFifo.putIn - 1);
}

//-----------------------------------------------------------------------------
//  _serial_RxBE() - Receive Buffer Empty
//
//  Return Value
//      0 = Receive Buffer has data
//      1 = Receive Buffer is empty
//-----------------------------------------------------------------------------

int16_t _serial_RxBE(void)
{
    return (IS_RXBUF_EMPTY() ? 1 : 0);
}

//-----------------------------------------------------------------------------
// return number of bytes available in receive buffer

int16_t _serial_RxFreeSpace()
{
    if (IS_RXBUF_EMPTY()) return(RXBUF_SIZE-1);   // one less than full buffer
    if (RxFifo.putIn > RxFifo.takeOut)
        return(RXBUF_SIZE - RxFifo.putIn + RxFifo.takeOut - 1);
    return(RxFifo.takeOut - RxFifo.putIn - 1);
}

//-----------------------------------------------------------------------------
//  _serial_putc(char ch)
//
//  'ch' is the integer promotion of the character to be written.
//  The value is internally converted (back) to an unsigned char when written.
//
//  Return Value
//      On success, the character written is returned.
//      On failure, EOF.
//-----------------------------------------------------------------------------

int16_t _serial_putc(char ch)
{
    if (_serial_TxFreeSpace()<1)
    {
        SYSTICKS startTicks;

        // only startup allows blocking
        if (!g_serialAllowTxWait)
            return(EOF);

        // during startup allow waiting for interrupt to free up transmitter
        startTicks = GetSysTicks();
        while (_serial_TxFreeSpace()<1)
        {
            if (IsTimedOut(100,startTicks)) return(EOF);
        }
    }   

  #ifdef DEBUG_BENCHMARK
    if (g_serial_DiscardData) return(ch);
  #endif
    TxFifo.buf[TxFifo.putIn] = (int8_t)ch;
    if (++TxFifo.putIn >= TXBUF_SIZE) TxFifo.putIn = 0; // wrap

    if (ch == '+' || ch == '-') g_needNewLine = 1;

    UxTXIF = 1; // Set UARTx TX Interrupt Flag

    return(ch);
}

//-----------------------------------------------------------------------------
//  _serial_puts(char *str)
//
//  'ch' is the integer promotion of the character to be written.
//  The value is internally converted (back) to an unsigned char when written.
//
//  Return Value
//      On success, Zero
//      On failure, EOF.
//-----------------------------------------------------------------------------

int16_t _serial_puts(const char * str)
{
    return(_serial_putbuf((unsigned char*)str, (int)strlen(str)));
}
        
//-----------------------------------------------------------------------------
//  _serial_putbuf
//
//  RETURNS: 0   if the data was successfully added to the transmit buffer.
//           EOF if the transmit buffer is full
//-----------------------------------------------------------------------------

int16_t _serial_putbuf(unsigned char *data, int len)
{
    int16_t n;

    // generate new line if after '+' or '-' putc
    if (g_needNewLine)
    {
        g_needNewLine = 0;
        _serial_putc('\r');
        _serial_putc('\n');
    }

    // get free space in transmit buffer
    n = _serial_TxFreeSpace();
    if (len <= n)
    {
        // entire data will fit in transmit buffer
        // copy data blocks to transmit buffer for speed
        n = TXBUF_SIZE - TxFifo.putIn; // calc space available to end of buffer
        if (TxFifo.takeOut < 1) n--;   // special case; dont let buffer totally fill
        if (len <= n)
        {
            // single chunk at end of buffer
            memcpy(&TxFifo.buf[TxFifo.putIn], data, len);
          #ifdef DEBUG_BENCHMARK
            if (g_serial_DiscardData) return(0);
          #endif
            TxFifo.putIn += len;
            if (TxFifo.putIn >= TXBUF_SIZE) TxFifo.putIn = 0; // wrap
        }
        else
        {
            // two chunks needed, due to buffer wrap
            memcpy(&TxFifo.buf[TxFifo.putIn], data, n); // end chunk of buffer
          #ifdef DEBUG_BENCHMARK
            if (g_serial_DiscardData) return(0);
          #endif
            TxFifo.putIn = len - n;
            memcpy(&TxFifo.buf[0], &data[n], TxFifo.putIn); // beginning chunk of buffer
        }

        // normal mode
        UxTXIF = 1; // Trigger UART Interrupt to send the data
    }
    else
    {
        return(EOF); // not enough space in buffer; 
    }
    return(0);
}

// <><><><><><><><><><><><><> dsPIC_serial.c <><><><><><><><><><><><><><><><><><><><><><>
