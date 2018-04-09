//-----------------------------------------------------------------------------
//	Copyright(C) 2015 - Sensata Technologies, Inc.  All rights reserved.
//
//	$Workfile: main.c $
//	$Author: clark_dailey $
//	$Date: 2018-01-10 09:15:54-06:00 $
//	$Revision: 6 $ 
//
//	Project:	LPC Transfer Switch
//
//	Description:
// 		PROCESSOR : PIC16F1823-I/SL 
// 		CLOCK     : Internal 32MHz
//
//=============================================================================

#define ALLOCATE_SPACE
#undef ALLOCATE_SPACE

#include <xc.h>
#include <stdint.h>

// -------------
// Configuration
// -------------
#define NUM_AC_CYCLES  (1)   // number of A/C cycles to average over
//   1 =  17 msec
//   2 =  34 msec
//   3 =  50 msec
//   6 = 100 msec


// ----------
// Constants
// ----------
#define AC_CYCLE_MSECS   (16. + (2./3.))    // A/C cycle is 16+2/3 milliseconds
#define RMS_SAMPLES	 ((int)(((float)NUM_AC_CYCLES*AC_CYCLE_MSECS) + (1./3.)+.01))  // 1 AC = 17 msecs


//-----------------------------------------------------------------------------
//-- PIC16F1823 DEVICE CONFIGURATION ------------------------------------------
//	ref. Datasheet Configuration Word 1
#pragma config FCMEN=OFF		//	Fail-Safe Clock Monitor is disabled
#pragma config IESO=OFF			//	Internal/External Switchover mode is disabled
#pragma config CLKOUTEN=OFF		//	CLKOUT function is disabled.  I/O or oscillator function on the CLKOUT pin
#pragma config BOREN=ON			//	Brown-out reset enabled
#pragma config CPD=OFF			//	Data memory code protection is disabled
#pragma config CP=OFF			//	Program memory code protection is disabled
#pragma config MCLRE=ON			//	MCLR/VPP pin function is MCLR
#pragma config PWRTE=OFF		//	Power-Up Timer disabled
#pragma config WDTE=OFF			//	WDT disabled
#pragma config FOSC=INTOSC		//	Internal Oscillator:  I/O functions on CLKIN pin
//	ref. Datasheet Configuration Word 2
#pragma config LVP=OFF			//	Low-voltage programming disabled
#pragma config BORV=LO			//	Brown-out Reset voltage (Vbor), low trip point selected
#pragma config STVREN=OFF		//	Stack Overflow or Underflow will NOT cause a Reset
#pragma config PLLEN=OFF		//	4x PLL disabled - Controlled by OSCCONbits.SPLLEN 
#pragma config WRT=OFF			//	Flash Memory Self-Write Protection Off 


//-----------------------------------------------------------------------------
//-- TYPE DEFINITIONS ---------------------------------------------------------

typedef struct tagDWord
{
	union 
	{
		int32_t	dword;
		struct
		{
			int16_t lsw;
			int16_t msw;
		};
	};
} DWORD_t;


//-----------------------------------------------------------------------------
//-- GLOBAL VARIABLES ---------------------------------------------------------

static uint16_t _Timer0Count = 0;


//-- Line Voltage Trip points -------------------------------------------------

//	Calibrated 2015-02-26 using Rev 1 PCA.
#define	VOLTS_SLOPE		(2.88786)
#define VOLTS_INTERCEPT	(-5.25918)
#define VOLTS_CAL(V)    ((V)*VOLTS_SLOPE+VOLTS_INTERCEPT)    // calibration equation

// squared term so don't have to take square root
#define VOLTS(V)        ((int32_t)(VOLTS_CAL(V)*VOLTS_CAL(V))) // squared term

#define XFER_VOLT_LOW_THRESHOLD		VOLTS(80.0)		//	80V
#define XFER_VOLT_LOW_RECOVER		VOLTS(85.0)		//	85V
#define XFER_VOLT_HIGH_RECOVER		VOLTS(130.0)	//	130V
#define XFER_VOLT_HIGH_THRESHOLD	VOLTS(140.0)	//	140V

//	Parameters read from the ADC buffer.
uint16_t	an_LineV;			//	AN1 = V_AC



//-- RMS Calculations ---------------------------------------------------------
//	The AC input signal is sampled on a one-millisecond interval. The ADC 
//	conversion is triggered by the 1mSec ISR. The ADC generates an interrupt
//	when the conversion is complete.
//
//	A simple semaphore:  
//	The RMS value is computed on a multiple of the AC cycle. The sum-of-squares
//	is calculated when the ADC sample is acquired. The square-root calculation
//	is done in a foreground process, it may span more than one sample time if 
//	necessary. A semaphore is used to assure mutually exclusive access to the 
//	sum-of-squares value. A simple semaphore is implemented as described below:
//
//   1) The there are two copies of the rms_sos[] (an array of 2). One copy 
//		indexed by '_RmsSosIndxIsr' is updated (written) by the ISR while the
//		other copy indexed by '_RmsSosIndxFg' is used (read) by the foreground 
//		task (main loop).
//	 2) The ADC ISR also maintains a count of the number of samples taken. When
//		the specified number of samples is reached, then the ADC ISR:
//		 a) Swaps the values of indexes  _RmsSosIndxFg & _RmsSosIndxIsr
//		 b) Sets the rms_sos[_RmsSosIndxIsr] == 0
//		 c) Sets the flag _RmsReady to signal the forground task.
//	4) The foreground process monitors the flag _RmsReady. When the flag is set
//	   the foreground task will compute the RMS value, and then clear the flag.
//
//	Since we are using a 1-millisecond sample rate, we choose a number of 
//	samples that provides an integral number of (60Hz) AC cycles.  
//		1000 samples = 60 AC cycles
//		 500 samples = 30 AC cycles
//		 100 samples =  6 AC cycles
//		  50 samples =  3 AC cycles
//
//	Using a 32-bit Sum-of-Squares, and a 10-bit ADC, I compute the maximum 
//	number of samples:
//		(2^10)-1 = 1,023 (0x3FF)
//		0x03FF * 0x03FF = 0x000F F801 (1,046,529)
//		(2^32)-1 = 4,294,967,295
//		4,294,967,295 / 1,046,529 = 4,104
//
//	so we have enough capacity for 4,104 samples

//	TODO: This may change when we have zero-crossing detection in place.

static int16_t	_RmsSosIndxFg = 0;
static int16_t	_RmsSosIndxIsr = 1;
static int16_t  _RmsSampleCount = 0;
static int8_t	_RmsReady = 0;

typedef struct
{
	int32_t	rms_sos[2];
	int32_t	rms;
	int32_t	avg_sum[2];
	int16_t	avg;
} AC_CALC_t;

AC_CALC_t LineV;



//-----------------------------------------------------------------------------
//	_HwConfig()
//-----------------------------------------------------------------------------
//	Hardware Configuration for the 170165 (12LP10) board using the 
//	Microchip PIC16F1823-I/SL for RMS Line-Detect
//
//	Ref: "Z:\2013\_12LPC15 Documents\171575 Rev-1 Documents\170165-1 12LPC15 Schematic.PDF"
//
//=============================================================================

static void _HwConfig(void)
{
	// 	OSCILLATOR CONFIGURATION
	//	Configure 32MHz Internal Oscillator - ref: datasheet section 5.2.2.6
	//	The	following settings are required to use the 32 MHz internal
	//	clock source:
	//	  • The FOSC bits in Configuration Word 1 must be set to use the INTOSC
	//		source as the device system clock (FOSC<2:0> = 100).
	//	  • The SCS bits in the OSCCON register must be cleared to use the 
	//		clock determined by FOSC<2:0> in Configuration Word 1 (SCS<1:0> = 00).
	//	  • The IRCF bits in the OSCCON register must be set to the 8 MHz 
	//		HFINTOSC set to use (IRCF<3:0> = 1110).
	//	  • The SPLLEN bit in the OSCCON register must be set to enable the 
	//		4xPLL, or the PLLEN bit of the Configuration Word 2 must be 
	//		programmed to a ‘1’.
	//
	//	NOTE: When using the PLLEN bit of Configuration Word 2, the 4xPLL 
	//	cannot be disabled by software and the 8 MHz HFINTOSC option will no 
	//	longer be available.
	//
	//	The 4xPLL is not available for use with the internal oscillator when 
	//	the SCS bits of the OSCCON register are set to ‘1x’. The SCS bits must 
	//	be set to ‘00’ to use the 4xPLL with the internal oscillator.
	//
	//	Main System Clock Selection (SCS)
	//	SCS<1:0>: System Clock Select bit
	//		1x = Internal oscillator block
	//		01 = Timer1 oscillator
	//		00 = Primary clock (determined by FOSC<2:0> in CONFIG1H).
	OSCCONbits.SCS = 0b00;
	
	//	Internal Frequency selection bits (IRCF, INTSRC)
	//	IRCF<3:0>: Internal Oscillator Frequency Select bits
	//		000x = 31 kHz LF
	//		0010 = 31.25 kHz MF
	//		0011 = 31.25 kHz HF(1)
	//		0100 = 62.5 kHz MF
	//		0101 = 125 kHz MF
	//		0110 = 250 kHz MF
	//		0111 = 500 kHz MF (default upon Reset)
	//		1000 = 125 kHz HF(1)
	//		1001 = 250 kHz HF(1)
	//		1010 = 500 kHz HF(1)
	//		1011 = 1MHz HF
	//		1100 = 2MHz HF
	//		1101 = 4MHz HF
	//		1110 = 8 MHz or 32 MHz HF(see Section 5.2.2.1 “HFINTOSC”)
	//		1111 = 16 MHz HF
	OSCCONbits.IRCF = 0b1110;	//	8MHz 
	
	//	SPLLEN: Software PLL Enable bit
	//		1 = 4x PLL Is enabled
	//		0 = 4x PLL is disabled
	OSCCONbits.SPLLEN = 1;
	
	//	TUN<5:0>: Frequency Tuning bits
	//	011111 = Maximum frequency
	//	011110 =
	//	...
	//	000001 =
	//	000000 = Oscillator module is running at the factory-calibrated frequency.
	//	111111 =
	//	...
	//	100000 = Minimum frequency
	OSCTUNE = 0b000000;
	
	//	I/O PORT CONFIGURATIONS -----------------------------------------------
	//	When setting a pin to an analog input, the corresponding TRIS bit must 
	//	be set to Input mode in order to allow external control of the voltage 
	//	on the pin.
	
	// Port-A
	//	RA0		pin-13	<dedicated ICSP_DATA>
	//	RA1		pin-12	<dedicated ICSP_CLK>
	TRISAbits.TRISA2 = 1;		//	RA2/AN2	pin-11	analog input LINE_V
	ANSELAbits.ANSA2 = 1;		//	Analog input. Digital input buffer disabled.
	WPUAbits.WPUA2 = 0;			//	Weak Pull-ups disabled (not sure if needed for analog.)
	//	RA3		pin-04	<dedicated ~MCLR/Vpp>
	//	RA4/AN3		pin-03	<unused>
	TRISAbits.TRISA4 = 1;		//	RA4/AN4	pin-11	analog input Potentiometer
	ANSELAbits.ANSA4 = 1;		//	Analog input. Digital input buffer disabled.
//	WPUAbits.WPUA4 = 0;			//	Weak Pull-ups disabled (not sure if needed for analog.)
	//	RA5		pin-02	<unused>
	
	// Port-C
	//	RC0/AN4	pin-10	<unused>
	TRISCbits.TRISC1 = 0;	//	RC1/AN5	pin-09	output	LINE_VALID			TP27 (120V_LINE_VALID)
	TRISCbits.TRISC2 = 0;	//	RC2/AN6	pin-08	output	LINE_VALID_EN_HKPS	TP25 (XFER_EN_HKPS)
	//	RC3/AN7	pin-07	<unused>
	TRISCbits.TRISC4 = 0;	//	RC4		pin-06	output	(ZERO_CROSS-DETECT)
	//	RC5		pin-05	<unused>
}


#define	LINE_VALID_HI()		(LATCbits.LATC1 = 1)
#define	LINE_VALID_LO()		(LATCbits.LATC1 = 0)

#define	HKPS_ENABLE()		(LATCbits.LATC2 = 1)
#define	HKPS_DISABLE()		(LATCbits.LATC2 = 0)

#define ZERO_CROSS_HI()		(LATCbits.LATC4 = 1)
#define	ZERO_CROSS_LO()		(LATCbits.LATC4 = 0)



///////////////////////////////////////////////////////////////////////////////
//	Timer0

//-----------------------------------------------------------------------------
//	_Timer0Config
//-----------------------------------------------------------------------------
//	Initialize Timer0 for 1-mSec interrupt
//	The Timer0 module is an 8-bit timer/counter
//
//=============================================================================

// LEGACY:	1:256 prescaler for a delay of: (insruction-cycle * 256-counts)*prescaler = ((8uS * 256)*256) =~ 524mS

static void _Timer0Config(void)
{
	//	TMR0CS: Timer0 Clock Source Select bit
	//		1 = Transition on RA4/T0CKI pin
	//		0 = Internal instruction cycle clock (FOSC/4)
	OPTION_REGbits.TMR0CS = 0;
	
	//	PSA: Prescaler Assignment bit
	//		1 = Prescaler is not assigned to the Timer0 module
	//		0 = Prescaler is assigned to the Timer0 module
	OPTION_REGbits.PSA = 0;
	
	//	PS<2:0>: Prescaler Rate Select bits
	//		000	= 1 : 2
	//		001 = 1 : 4
	//		010 = 1 : 8
	//		011 = 1 : 16
	//		100 = 1 : 32
	//		101 = 1 : 64
	//		110 = 1 : 128
	//		111 = 1 : 256
	OPTION_REGbits.PS = 0b100;

	return;
}

//-----------------------------------------------------------------------------
//	_Timer0Start
//-----------------------------------------------------------------------------
//	Start the timer 
//
//=============================================================================

#define 	TIMER0_INIT	(0x06)
static void _Timer0Start(void)
{
	//	Clear the Timer0 Interrupt Flag
	INTCONbits.TMR0IF = 0;
	
	//	Initialize the timer register (Timer0)
	TMR0 = TIMER0_INIT;
	
	//	Enable the Timer0 rollover interrupt
    INTCONbits.TMR0IE = 1;
	return;
}


//-----------------------------------------------------------------------------
//	_Timer0ISR()
//
//	This is a one-millisecond timer interrupt.
//
//=============================================================================
static void _Timer0ISR(void)
{
	//	Initialize the timer register (Timer0)
	TMR0 = TIMER0_INIT;		
	_Timer0Count++;

	//	Start the ADC conversion by setting the GO/DONE bit.
	ADCON0bits.GO = 1;
	
	return;
}

///////////////////////////////////////////////////////////////////////////////
//	ADC
//
//	16.2.6 A/D CONVERSION PROCEDURE
//	An example procedure to perform an Analog-to-Digital conversion:
//	1. Configure Port (done in _HwConfig()):
//	  • Disable pin output driver (Refer to the TRIS register)
//	  • Configure pin as analog (Refer to the ANSEL register)
//	2. Configure the ADC module:
//	  • Select ADC conversion clock
//	  • Configure voltage reference
//	  • Select ADC input channel
//	  • Turn on ADC module
//	3. Configure ADC interrupt (optional):
//	  • Clear ADC interrupt flag
//	  • Enable ADC interrupt
//	  • Enable peripheral interrupt
//	  • Enable global interrupt
//	4. Wait the required acquisition time(2).
//	5. Start conversion by setting the GO/DONE bit.
//	6. Wait for ADC conversion to complete by one of the following:
//	  • Polling the GO/DONE bit
//	  • Waiting for the ADC interrupt (interrupts enabled)
//	7. Read ADC Result.
//	8. Clear the ADC interrupt flag (required if interrupt is enabled).
//

//-----------------------------------------------------------------------------
//	_AdcConfig
//-----------------------------------------------------------------------------
//	Configure the Analog to Digital Converter (ADC)
//
//=============================================================================
static void _AdcConfig(void)
{
	//	Select ADC conversion clock
	//	ADCS<2:0>: A/D Conversion Clock Select bits
	//		000 = FOSC/2
	//		001 = FOSC/8
	//		010 = FOSC/32
	//		011 = FRC (clock supplied from a dedicated RC oscillator)
	//		100 = FOSC/4
	//		101 = FOSC/16
	//		110 = FOSC/64
	//		111 = FRC (clock supplied from a dedicated RC oscillator)
	ADCON1bits.ADCS = 0b101;	//	FOSC/16 9.11 uSec
	
	//	Configure voltage reference
	//	ADPREF<1:0>: A/D Positive Voltage Reference Configuration bits
	//		00 = VREF+ is connected to AVDD
	//		01 = Reserved
	//		10 = VREF+ is connected to external VREF+(1)
	//		11 = VREF+ is connected to internal Fixed Voltage Reference (FVR)
	ADCON1bits.ADPREF = 0b11;
	
	//	Select ADC input channel
	//	CHS<4:0>: Analog Channel Select bits
	//		00000 = AN0
	//		00001 = AN1
	//		00010 = AN2
	//		00011 = AN3
	//		00100 = AN4(1)
	//		00101 = AN5(1)
	//		00110 = AN6(1)
	//		00111 = AN7(1)
	//		01001 = Reserved. No channel connected.
	//		...
	//		11100 = Reserved. No channel connected.
	//		11101 = Temperature Indicator(4)
	//		11110 = DAC output(2)
	//		11111 =FVR (Fixed Voltage Reference) Buffer 1 Output(3)	
	ADCON0bits.CHS = 0b00010;	//	AN2

	//	ADFM: A/D Result Format Select bit
	//		1 = Right justified. Six MS-bits of ADRESH are set to '0' 
	//		0 = Left justified. Six LS-bits of ADRESL are set to '0’
	ADCON1bits.ADFM = 1;

	//	Configure the internal Fixed Voltage Reference
	//	FVREN: Fixed Voltage Reference Enable bit
	//		0 = Fixed Voltage Reference is disabled
	//		1 = Fixed Voltage Reference is enabled
	FVRCONbits.FVREN = 1;
	
	//	ADFVR<1:0>: ADC Fixed Voltage Reference Selection bits
	//		00 = ADC Fixed Voltage Reference Peripheral output is off
	//		01 = ADC Fixed Voltage Reference Peripheral output is 1x (1.024V)
	//		10 = ADC Fixed Voltage Reference Peripheral output is 2x (2.048V)
	//		11 = ADC Fixed Voltage Reference Peripheral output is 4x (4.096V)
	FVRCONbits.ADFVR = 0b11;
}


//-----------------------------------------------------------------------------
//	_AdcStart
//-----------------------------------------------------------------------------
//	Start the Analog to Digital Converter (ADC)
//
//=============================================================================
static void _AdcStart(void)
{
	//	Turn on ADC module
	//	ADON: ADC Enable bit
	//		1 = ADC is enabled
	//		0 = ADC is disabled and consumes no operating current
	ADCON0bits.ADON = 1;

	//	Clear ADC interrupt flag
	PIR1bits.ADIF = 0;
	
	//	Enable ADC interrupt
	PIE1bits.ADIE = 1;
	
	//	Enable peripheral interrupt
	//	Global interrupts are enabled in the main loop.
	INTCONbits.PEIE = 1;
	
	//	Start conversion by setting the GO/DONE bit.
	//	ADCON0bits.GO = 1;

	//	Initialize variables for RMS Calculations
	_RmsSosIndxIsr = 0;
	_RmsSosIndxFg = _RmsSosIndxIsr ^ 0x0001;
	
	LineV.rms_sos[_RmsSosIndxFg] = (int32_t)0;
	LineV.rms_sos[_RmsSosIndxIsr] = (int32_t)0;
	
	_RmsSampleCount = 0;
	_RmsReady = 0;
}


//-----------------------------------------------------------------------------
//	_AdcISR
//-----------------------------------------------------------------------------
//	Analog to Digital Converter (ADC) Interrupt Service Routine (ISR) 
//
//	Process the data from the prior conversion and then configure the next 
//	conversion.
//
//	ref: Datasheet 16.1.5 INTERRUPTS
//=============================================================================

int32_t	temp32;

static void _AdcISR(void)
{
	an_LineV = ADRES;
	
	// VAC RMS Calculations
	temp32 = (int32_t)((int32_t)an_LineV * (int32_t)an_LineV);
	LineV.rms_sos[_RmsSosIndxIsr] += temp32;
	
	if(++_RmsSampleCount >= RMS_SAMPLES)
	{
		_RmsSampleCount = 0;
		_RmsSosIndxFg = _RmsSosIndxIsr;
		_RmsSosIndxIsr ^= 0x0001;
		LineV.rms_sos[_RmsSosIndxIsr] = (int32_t)0;
		_RmsReady = 1;
	}
}



//-----------------------------------------------------------------------------
//	CheckLineVoltage 
//
//=============================================================================

typedef enum {	
	LS_STATE_STARTUP = 0,	
	LS_STATE_LOW_LINE,		//	1
	LS_STATE_VALID_LINE,	//	2
	LS_STATE_HIGH_LINE 		//	3
	} LINE_SENSE_STATE_t;
	
//	DEBUG 	
volatile LINE_SENSE_STATE_t line_sense_state = LS_STATE_STARTUP;

// line voltage is squared to avoid the square root calculation	
void CheckLineVoltage(int32_t line_volts_sq)
{
//	static LINE_SENSE_STATE_t line_sense_state = LS_STATE_STARTUP;
	
	switch(line_sense_state)
	{
		default:
		case LS_STATE_STARTUP:
			LINE_VALID_LO();
			HKPS_DISABLE();
			line_sense_state = LS_STATE_LOW_LINE;
			break;

		case LS_STATE_LOW_LINE:
			LINE_VALID_LO();
			HKPS_DISABLE();
			if(line_volts_sq >= XFER_VOLT_LOW_RECOVER)
			{
				line_sense_state = LS_STATE_VALID_LINE;
			}
			break;

		case LS_STATE_VALID_LINE:
			LINE_VALID_HI();
			HKPS_ENABLE();
			if(line_volts_sq <= XFER_VOLT_LOW_THRESHOLD)
			{
				line_sense_state = LS_STATE_LOW_LINE;
			}
			else if(line_volts_sq >= XFER_VOLT_HIGH_THRESHOLD)
			{
				line_sense_state = LS_STATE_HIGH_LINE;
			}
			break;
			
		case LS_STATE_HIGH_LINE:
			LINE_VALID_LO();
			HKPS_DISABLE();
			if(line_volts_sq <= XFER_VOLT_HIGH_RECOVER)
			{
				line_sense_state = LS_STATE_VALID_LINE;
			}
			break;
	}
}



void interrupt ISR(void) 
{
	if(INTCONbits.TMR0IF)	//	Timer0 Interrupt Flag
	{
		//	Clear the Timer0 Interrupt Flag
		INTCONbits.TMR0IF = 0;
		_Timer0ISR();
	}
	
	if(PIR1bits.ADIF)		//	ADC Interrupt Flag
	{
		PIR1bits.ADIF = 0;
		_AdcISR();
	}
		
}					

/****************************************************************************
*
* Function: main 
* Description: This is the application main routine
* Inputs: None
* Outputs: None
*
*****************************************************************************/
void main(void)
{
	_HwConfig();

	_Timer0Config();
	_AdcConfig();
	
	_Timer0Start();
	_AdcStart();

    INTCONbits.GIE = 1;		//	Global Interrupt Enable bit

	while(1)
	{
		if(_RmsReady)
		{
			//	This calculation takes about 245 uSec (FOSC = 32MHz)
			_RmsReady = 0;
			LineV.rms = LineV.rms_sos[_RmsSosIndxFg]/RMS_SAMPLES;
			LineV.rms_sos[_RmsSosIndxFg] = (int32_t)0;
			
			CheckLineVoltage(LineV.rms);
		}

		if(_Timer0Count > 0)
		{
			//	one-millisecond...
			_Timer0Count--;
		}
	}
}
 
// EOF
//*****************************************************************************
//	$Log: /FW Dev/LPC/lpc_xfer/src/main.c $
// 
// Revision: 6   Date: 2018-01-10 15:15:54Z   User: clark_dailey 
// 1) allow configuring by specifying NUM_AC_CYCLES. 
// 2) removed sqrt() calculation since it is not needed 
// 
// Revision: 5   Date: 2015-02-26 21:34:47Z   User: A1021170 
// Functional Version. 
// Calibrated switching thresholds using 12LPC15 PCA. 
// 
// Revision: 4   Date: 2015-02-26 20:18:49Z   User: A1021170 
// RMS calculations functional 
// Level detection working. 
// TODO: calibrate thresholds on target 
					
					