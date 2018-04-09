// <><><><><><><><><><><><><> analog_dsPIC33F.c <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//  Copyright(C) 2017 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
//  Analog to Digital Conversion specific to dsPIC33F microprocessor
//
//  It was determined by experiment on the dsPIC30F platform that the ADC10
//  performs faster if we sample 4-channels at once and convert them. The
//  alternative is to scan all channels. Scanning is slower since the sample
//  time must be applied to each channel read.
//
//  Since we are reading only 7 signals, one of the signals may be read twice.
//  VAC is read twice since it is critical to the inverter PID loop. 
//  The 2 readings may be averaged.
//
//  The 8-signals are read as two groups MUXA and MUXB as described below:
//
//  +----------+----------------------+--------------------+---------------------+             
//  |          | LPC                  | NP                 |  BDC      			 |
//  +----------+----------------------+--------------------+---------------------+             
//  | MUXA,CH0 | AN8 = 15V_RAIL_RESET | AN8 = CHRG_I_LIMIT |  AN8 = CHRG_I_LIMIT |
//  | MUXA,CH1 | AN0 = BATT_V         | AN0 = BATT_V       |  AN0 = BATT_V		 |
//  | MUXA,CH2 | AN1 = VAC            | AN1 = VAC          |  AN1 = DCout_SCALED |
//  | MUXA,CH3 | AN2 = OVL            | AN2 = HS_BAR_IN    |  AN2 = HS_BAR_IN    |
//  +----------+----------------------+--------------------+---------------------+             
//  | MUXB,CH0 | AN2 = OVL            | AN11= 15V_RAIL     |  AN11= 15V_RAIL     |
//  | MUXB,CH1 | AN3 = BATT_TEMP      | AN3 = BATT_TEMP    |  AN3 = BATT_TEMP    |
//  | MUXB,CH2 | AN4 = I_MEASURE      | AN4 = I_MEASURE    |  AN4 = I_MEASURE    |
//  | MUXB,CH3 | AN5 = LOAD_MGMT      | AN5 = LOAD_MGMT    |  AN5 = THERMISTOR	 |
//  +----------+----------------------+--------------------+---------------------+             


//-----------------------------------------------------------------------------
// NOTE:  This revision is being checked in as a work-in-progress. This revision
//	is a working version with both ADC1 and ADC2 functional. ADC1 is triggered 
//	from Timer3 and ADC2 is cascaded from that.  ADC1 uses DMA0.  The DMA0 ISR 
//	starts ADC2 by clearing its SAMP bit.  
//
//	Attempts to trigger both ADC1 & ADC2 from Timer3 were unsuccessful. ADC1 
//	worked but ADC2 did not.
//
//	It was also notified that if the ADC buffer used by ADC2 was not read, then
//	DMA interrupts for ADC2 did not occur.  This should be re-verified.
//
//	The next revision of this code will comment out ADC2-related code, to avoid 
//	potential issues with baseline code.
//
//  Also, the declarations of 'Adc1Dma0Buf' and 'Adc2Dma3Buf' are critcal - the
//	a2d functions may work, but other code using DMA (ECAN Rcv) fails.
//-----------------------------------------------------------------------------

// -------
// headers
// -------
#include "options.h"    // must be first include
#include "analog.h"
#include "inverter.h"
#include "sine_table.h"
#include "sqrt.h"
#include "tasker.h"

#if defined(OPTION_SSR)
 #include "ssr.h"
#endif

//-----------------------------------------------------------------------------
//                   G L O B A L     D A T A
//-----------------------------------------------------------------------------

// Align DMA buffer to 128 words (256 bytes)
uint16_t Adc1Dma0Buf[8] __attribute__((space(dma),aligned(256)));
//adc2 uint16_t Adc2Dma3Buf[8] __attribute__((space(dma),aligned(256)));

TASK_ID_t an_task = -1;

//adc2 uint16_t	an_AN6 = 0;
//adc2 uint16_t	an_AN7 = 0;

//-----------------------------------------------------------------------------
void _Adc1Config(void)
{
 #ifndef WIN32
 
    //  Assure that ADC is Off, writing the ADC configuration registers
    //  SSRC<2:0>, ASAM, SIMSAM, SMPI<3:0>, BUFM and ALTS bits, as well as the
    //  ADCON3 and ADCSSL registers, might have indeterminate results.
    AD1CON1bits.ADON = 0;          // turn ADC Off

    //  Select 10-bit mode
    AD1CON1bits.AD12B = 0;

    //  Select port pins as analog inputs -------------------------------------
    //  PCFG<15:0>: Analog Input Pin Configuration Control bits
    //    1 = Analog input pin in Digital mode, port read input enabled,
    //      A/D input multiplexer input connected to AVSS
    //    0 = Analog input pin in Analog mode, port read input disabled,
    //      A/D samples pin voltage
    AD1PCFGLbits.PCFG0 = 0;     //  AN0 = BATT_V
    AD1PCFGLbits.PCFG1 = 0;     //  AN1 = V_AC      (XFMR_FDBK)
    AD1PCFGLbits.PCFG2 = 0;     //  AN2 = HS_BAR_IN/OVL
	//	TODO: 'OPTION_HAS_CHGR_EN_SWITCH' Conditional compile can be removed
	//	when we have system to test it on. Resulting reading is not used by
	//	higher-level code based on [redundant] conditional compile.
  #if(!defined(OPTION_HAS_CHGR_EN_SWITCH))	
    AD1PCFGLbits.PCFG3 = 0;     //  AN3 = BATT_TEMP
  #endif
    AD1PCFGLbits.PCFG4 = 0;     //  AN4 = I_MEASURE
    AD1PCFGLbits.PCFG5 = 0;     //  AN5 = LOAD_MGMT
    AD1PCFGLbits.PCFG8 = 0;     //  AN8 = CHRG_I_LIMIT/15V_RAIL_RESET
  #if IS_PCB_NP || IS_PCB_BDC
	AD1PCFGLbits.PCFG11 = 0;    //  AN11 = 15V_RAIL
  #endif
  
    //  Select voltage reference source ---------------------------------------
    //  Voltage Reference is internal
    //  VCFG<2:0>: Voltage Reference Configuration bits
    //      VCFG<2:0>   A/D VREFH   A/D VREFL
    //          000     AVDD        AVSS
    //          001     Ext VREF+   AVSS
    //          010     AVDD        Ext VREF-
    //          011     Ext VREF+   Ext VREF-
    //          1xx     AVDD        AVSS
    AD1CON2bits.VCFG = 0b000;

    //  Select the analog conversion clock ------------------------------------
    //  A/D Conversion Clock
    //  We want to sample at least once per PWM (20KHz) Period, but must
    //  ensure a minimum TAD of 83.33 nSec. 'dsPIC30F Family Reference Manual,
    //  section-17, provides the following formula:
    //      ADCS = ((2*TAD)/TCY)-1 = ((2*TAD)*FCY)-1 = 5.66 --> 6
    //      TAD = TCY*(ADCS+1)/2 = (ADCS+1)/(2*FCY) = 87.5 nSec
    //
    //  ADCON3bits.ADCS<5:0>: A/D Conversion Clock Select bits
    //      111111 = (TCY/2)*(ADCS<5:0> + 1) = 32*TCY
    //       ...
    //      000111 = (TCY/2)*(ADCS<5:0> + 1) = 4*TCY
    //       ...
    //      000001 = (TCY/2)*(ADCS<5:0> + 1) = TCY
    //      000000 = (TCY/2)*(ADCS<5:0> + 1) = TCY/2
    AD1CON3bits.ADCS = 0b000111;       //  4*TCY

    //  Determine how many S/H channels will be used --------------------------
    //  ADCON2:CHPS<1:0> Selects Channels Utilized bits
    //      1x = Converts CH0, CH1, CH2 and CH3
    //      01 = Converts CH0 and CH1
    //      00 = Converts CH0
    AD1CON2bits.CHPS = 0b011;

    //  Determine how sampling will occur -------------------------------------
    //  SIMSAM: Simultaneous Sample Select bit (ADCON1:CHPS = 01 or 1x)
    //  The SIMSAM bit controls the acquire/convert sequence for multiple
    //  channels. If only 1 channel selected, SIMSAM is not applicable.
    //  If the SIMSAM bit is 0, the two or four selected channels are acquired
    //      and converted sequentially with two or four sample clocks.
    //  If the SIMSAM bit is 0, two or four selected channels are acquired
    //      simultaneously with one sample clock.  The channels are then
    //      converted sequentially.
    AD1CON1bits.SIMSAM = 1;

    //  Determine how inputs will be allocated to S/H channels ----------------
    //  If ALTS == 0, only the MUX A inputs are selected for sampling.
    //  If ALTS == 1, alternate MUX A and MUX B inputs each sample.
    AD1CON2bits.ALTS = 1;

    //  MUX A setup:
    //      MUX A,CH0   AN8
    //      MUX A,CH1   AN0
    //      MUX A,CH2   AN1 
    //      MUX A,CH3   AN2 *
    //
    //  CH0SA<3:0>: Channel 0 Positive Input Select for MUX A
    //      1000 = Channel 0 positive input is AN8 (CHRG_I_LIMIT)
    AD1CHS0bits.CH0SA = 0b01000;
    //  CH0NA: Channel 0 Negative Input Select for MUX A
    //      0 = Channel 0 negative input is VREF
    AD1CHS0bits.CH0NA = 0;
    //  CH123SA: Channel 1, 2, 3 Positive Input Select for MUX A
    //      0 = CH1 pos is AN0, CH2 pos is AN1, CH3 pos is AN2
    AD1CHS123bits.CH123SA = 0;
    //  CH123NA<1:0>: Channel 1, 2, 3 Negative Input Select for MUX A
    //      0x = CH1, CH2, CH3 negative input is VREF
    AD1CHS123bits.CH123NA = 0b00;

    //  MUX B setup:
    //      MUX B,CH0   AN2 *
    //      MUX B,CH1   AN3
    //      MUX B,CH2   AN4
    //      MUX B,CH3   AN5
    //
    //  CH0SB<3:0>: Channel 0 Positive Input Select for MUX B
  #if IS_PCB_LPC
	//	For LPC: MuxB,CH0 = AN2 = OVL	
    //      0010 = Channel 0 positive input is AN2
    AD1CHS0bits.CH0SB = 0x02;
  #else
	//	For NP and BDC: MuxB,CH0 = AN11 = 15V_RAIL
	//		1011 = Channel 0 positive input is AN11
    AD1CHS0bits.CH0SB = 0x0B;
  #endif

    //  CH0NB: Channel 0 Negative Input Select for MUX B
    //      0 = Channel 0 negative input is VREF
    AD1CHS0bits.CH0NB = 0;
    //  CH123SB: Channel 1, 2, 3 Positive Input Select for MUX B
    //      1 = CH1 pos is AN3, CH2 pos is AN4, CH3 pos is AN5
    AD1CHS123bits.CH123SB = 1;
//    _CH123SB = 0;
    //  CH123NB<1:0>: Channel 1, 2, 3 Negative Input Select for MUX B
    //      0x = CH1, CH2, CH3 negative input is VREF
    AD1CHS123bits.CH123NB = 0;

    //  Select the appropriate sample/conversion sequence ---------------------

    //  ADCON1<7:0> and ADCON3<12:8>
    //  SSRC<2:0>: Conversion Trigger Source Select bits
    //   111 = Conversion trigger is under A/D clock control
    //   011 = Motor Control PWM interval ends sampling and starts conversion
    // * 010 = GP Timer3 compare ends sampling and starts conversion
    //   001 = Active transition on INT0 pin ends sampling and starts conv.
    //   000 = Clearing SAMP bit ends sampling and starts conversion
    AD1CON1bits.SSRC = 0b0010; //  Timer3

    //  ASAM: A/D Sample Auto-Start bit
    //   * 1 = Sampling begins immediately after last conversion completes.
    //         SAMP bit is auto set
    //     0 = Sampling begins when SAMP bit set
    //  NOTE: Using the modes where an external event trigger pulse ends
    //  sampling and starts conversion (SSRC = 001, 10, 011) may be used in
    //  combination with auto-sampling (ASAM = 1) to cause the A/D to
    //  synchronize the sample conversion events to the trigger pulse source.
    AD1CON1bits.ASAM = 1;

    //  SAMC<4:0>: Auto-Sample Time bits
    //      11111 = 31 TAD
    //      .....
    //      00001 = 1 TAD
    //      00000 = 0 TAD (only allowed if performing sequential conversions
    //              using more than one S/H amplifier)
    //  A note in section 18.15 of the 'dsPIC30F Family Reference Manual' says:
    //  'At least 1 TAD time period should be allowed between conversions for
    //  the sample time'.  I think that we are safe since we start sampling
    //  immediately after last conversion completes.
    AD1CON3bits.SAMC = 0b00001;

    //  Select how conversion results are presented in the buffer -------------
    //  ADCON1:FORM<1:0>: Data Output Format bits
    //    11(3) = Signed Fractional (DOUT = sddd dddd dd00 0000)
    //    10(2) = Fractional (DOUT = dddd dddd dd00 0000)
    //    01(1) = Signed Integer (DOUT = ssss sssd dddd dddd)
    //    00(0) = Integer (DOUT = 0000 00dd dddd dddd)
    AD1CON1bits.FORM = 0b00;

    //  ADDMABM: DMA Buffer Build Mode bit
    //  1 = DMA buffers are written in the order of conversion. The module will
    //      provide an address to the DMA channel that is the same as the
    //      address used for the non-DMA stand-alone buffer
    //  0 = DMA buffers are written in Scatter/Gather mode. The module will
    //      provide a scatter/gather address to the DMA channel, based on the
    //      index of the analog input and the size of the DMA buffer
    AD1CON1bits.ADDMABM = 1;

    //  SMPI<3:0>: Selects Increment Rate for DMA Addresses bits or number of
    //  sample/conversion operations per interrupt
    //      1111 = Increments the DMA address or generates interrupt after
    //          completion of every 16th sample/conversion operation
    //      1110 =  ... every 15th sample/conversion operation
    //          ...
    //      0001 =  ... every 2nd sample/conversion operation
    //      0000 =  ... every sample/conversion operation
    //
    AD1CON2bits.SMPI = 0b0001;     //  every 2nd sample/conversion

    //  DMABL<2:0>: Selects Number of DMA Buffer Locations per Analog Input bits
    //      111 = Allocates 128 words of buffer to each analog input
    //      110 = Allocates  64 words of buffer to each analog input
    //      101 = Allocates  32 words of buffer to each analog input
    //      100 = Allocates  16 words of buffer to each analog input
    //      011 = Allocates   8 words of buffer to each analog input
    //      010 = Allocates   4 words of buffer to each analog input
    //      001 = Allocates   2 words of buffer to each analog input
    //      000 = Allocates   1 word  of buffer to each analog input
    AD1CON4bits.DMABL = 0b000;     //    1 word  of buffer to each analog input
 #endif // WIN32
}

//adc2 //-----------------------------------------------------------------------------
//adc2 //  _Adc2Config
//adc2 //-----------------------------------------------------------------------------
//adc2 //	AN6 = J6B 
//adc2 //	AN7 = J6C
//adc2 //-----------------------------------------------------------------------------
//adc2 
//adc2 void _Adc2Config(void)
//adc2 {
//adc2  #ifndef WIN32
//adc2     //  Assure that ADC is Off, writing the ADC configuration registers
//adc2     //  SSRC<2:0>, ASAM, SIMSAM, SMPI<3:0>, BUFM and ALTS bits, as well as the
//adc2     //  ADCON3 and ADCSSL registers, might have indeterminate results.
//adc2     AD2CON1bits.ADON = 0;          // turn ADC Off
//adc2 
//adc2     //  Select 10-bit mode
//adc2     AD2CON1bits.AD12B = 0;
//adc2 
//adc2     //  Select port pins as analog inputs -------------------------------------
//adc2     //  PCFG<15:0>: Analog Input Pin Configuration Control bits
//adc2     //    1 = Analog input pin in Digital mode, port read input enabled,
//adc2     //      A/D input multiplexer input connected to AVSS
//adc2     //    0 = Analog input pin in Analog mode, port read input disabled,
//adc2     //      A/D samples pin voltage
//adc2     AD2PCFGLbits.PCFG0 = 0;     //  AN0 = BATT_V
//adc2     AD2PCFGLbits.PCFG1 = 0;     //  AN1 = V_AC      (XFMR_FDBK)
//adc2     AD2PCFGLbits.PCFG2 = 0;     //  AN2 = HS_BAR_IN/OVL
//adc2     AD1PCFGLbits.PCFG6 = 0;     //  AN6 = J6B *****
//adc2     AD2PCFGLbits.PCFG6 = 0;     //  AN6 = J6B *****
//adc2 	
//adc2     AD2PCFGLbits.PCFG4 = 0;     //  AN4 = I_MEASURE
//adc2     AD2PCFGLbits.PCFG5 = 0;     //  AN5 = LOAD_MGMT
//adc2     AD2PCFGLbits.PCFG8 = 0;     //  AN8 = CHRG_I_LIMIT/15V_RAIL_RESET
//adc2 	AD1PCFGLbits.PCFG7 = 0;     //  AN7 = J6C *****
//adc2 	AD2PCFGLbits.PCFG7 = 0;     //  AN7 = J6C *****
//adc2   
//adc2     //  Select voltage reference source ---------------------------------------
//adc2     //  Voltage Reference is internal
//adc2     //  VCFG<2:0>: Voltage Reference Configuration bits
//adc2     //      VCFG<2:0>   A/D VREFH   A/D VREFL
//adc2     //          000     AVDD        AVSS
//adc2     //          001     Ext VREF+   AVSS
//adc2     //          010     AVDD        Ext VREF-
//adc2     //          011     Ext VREF+   Ext VREF-
//adc2     //          1xx     AVDD        AVSS
//adc2     AD2CON2bits.VCFG = 0b000;
//adc2 
//adc2     //  Select the analog conversion clock ------------------------------------
//adc2     //  A/D Conversion Clock
//adc2     //  We want to sample at least once per PWM (20KHz) Period, but must
//adc2     //  ensure a minimum TAD of 83.33 nSec. 'dsPIC30F Family Reference Manual,
//adc2     //  section-17, provides the following formula:
//adc2     //      ADCS = ((2*TAD)/TCY)-1 = ((2*TAD)*FCY)-1 = 5.66 --> 6
//adc2     //      TAD = TCY*(ADCS+1)/2 = (ADCS+1)/(2*FCY) = 87.5 nSec
//adc2     //
//adc2     //  ADCON3bits.ADCS<5:0>: A/D Conversion Clock Select bits
//adc2     //      111111 = (TCY/2)*(ADCS<5:0> + 1) = 32*TCY
//adc2     //       ...
//adc2     //      000111 = (TCY/2)*(ADCS<5:0> + 1) = 4*TCY
//adc2     //       ...
//adc2     //      000001 = (TCY/2)*(ADCS<5:0> + 1) = TCY
//adc2     //      000000 = (TCY/2)*(ADCS<5:0> + 1) = TCY/2
//adc2     AD2CON3bits.ADCS = 0b000111;       //  4*TCY
//adc2 
//adc2     //  Determine how many S/H channels will be used --------------------------
//adc2     //  ADCON2:CHPS<1:0> Selects Channels Utilized bits
//adc2     //      1x = Converts CH0, CH1, CH2 and CH3
//adc2     //      01 = Converts CH0 and CH1
//adc2     //      00 = Converts CH0
//adc2     AD2CON2bits.CHPS = 0b011;
//adc2 
//adc2     //  Determine how sampling will occur -------------------------------------
//adc2     //  SIMSAM: Simultaneous Sample Select bit (ADCON1:CHPS = 01 or 1x)
//adc2     //  The SIMSAM bit controls the acquire/convert sequence for multiple
//adc2     //  channels. If only 1 channel selected, SIMSAM is not applicable.
//adc2     //  If the SIMSAM bit is 0, the two or four selected channels are acquired
//adc2     //      and converted sequentially with two or four sample clocks.
//adc2     //  If the SIMSAM bit is 0, two or four selected channels are acquired
//adc2     //      simultaneously with one sample clock.  The channels are then
//adc2     //      converted sequentially.
//adc2     AD2CON1bits.SIMSAM = 1;
//adc2 
//adc2     //  Determine how inputs will be allocated to S/H channels ----------------
//adc2     //  If ALTS == 0, only the MUX A inputs are selected for sampling.
//adc2     //  If ALTS == 1, alternate MUX A and MUX B inputs each sample.
//adc2     AD2CON2bits.ALTS = 1;
//adc2 
//adc2     //  MUX A setup:
//adc2     //      MUX A,CH0   AN6
//adc2     //      MUX A,CH1   AN0
//adc2     //      MUX A,CH2   AN1 
//adc2     //      MUX A,CH3   AN2 *
//adc2     //
//adc2     //  CH0SA<3:0>: Channel 0 Positive Input Select for MUX A
//adc2     //      0110 = Channel 0 positive input is AN6 (J6B)
//adc2     AD2CHS0bits.CH0SA = 0b0110;
//adc2     //  CH0NA: Channel 0 Negative Input Select for MUX A
//adc2     //      0 = Channel 0 negative input is VREF
//adc2     AD2CHS0bits.CH0NA = 0;
//adc2     //  CH123SA: Channel 1, 2, 3 Positive Input Select for MUX A
//adc2     //      0 = CH1 pos is AN0, CH2 pos is AN1, CH3 pos is AN2
//adc2     AD2CHS123bits.CH123SA = 0;
//adc2     //  CH123NA<1:0>: Channel 1, 2, 3 Negative Input Select for MUX A
//adc2     //      0x = CH1, CH2, CH3 negative input is VREF
//adc2     AD2CHS123bits.CH123NA = 0b00;
//adc2 
//adc2     //  MUX B setup:
//adc2     //      MUX B,CH0   AN7
//adc2     //      MUX B,CH1   AN3
//adc2     //      MUX B,CH2   AN4
//adc2     //      MUX B,CH3   AN5
//adc2     //
//adc2     //  CH0SB<3:0>: Channel 0 Positive Input Select for MUX B
//adc2     //      0111 = Channel 0 positive input is AN6
//adc2     AD2CHS0bits.CH0SB = 0b0111;
//adc2     //  CH0NB: Channel 0 Negative Input Select for MUX B
//adc2     //      0 = Channel 0 negative input is VREF
//adc2     AD2CHS0bits.CH0NB = 0;
//adc2     //  CH123SB: Channel 1, 2, 3 Positive Input Select for MUX B
//adc2     //      1 = CH1 pos is AN3, CH2 pos is AN4, CH3 pos is AN5
//adc2     AD2CHS123bits.CH123SB = 1;
//adc2     //  CH123NB<1:0>: Channel 1, 2, 3 Negative Input Select for MUX B
//adc2     //      0x = CH1, CH2, CH3 negative input is VREF
//adc2     AD2CHS123bits.CH123NB = 0b00;
//adc2 
//adc2 
//adc2     //  Select the appropriate sample/conversion sequence ---------------------
//adc2 
//adc2     //  ADCON1<7:0> and ADCON3<12:8>
//adc2     //  SSRC<2:0>: Conversion Trigger Source Select bits
//adc2     //   111 = Conversion trigger is under A/D clock control
//adc2     //   011 = Motor Control PWM interval ends sampling and starts conversion
//adc2     // * 010 = GP Timer3 compare ends sampling and starts conversion
//adc2     //   001 = Active transition on INT0 pin ends sampling and starts conv.
//adc2     //   000 = Clearing SAMP bit ends sampling and starts conversion
//adc2     AD2CON1bits.SSRC = 0b0000; 	//  AD2CON1.SAMP
//adc2 
//adc2     //  ASAM: A/D Sample Auto-Start bit
//adc2     //   * 1 = Sampling begins immediately after last conversion completes.
//adc2     //         SAMP bit is auto set
//adc2     //     0 = Sampling begins when SAMP bit set
//adc2     //  NOTE: Using the modes where an external event trigger pulse ends
//adc2     //  sampling and starts conversion (SSRC = 001, 10, 011) may be used in
//adc2     //  combination with auto-sampling (ASAM = 1) to cause the A/D to
//adc2     //  synchronize the sample conversion events to the trigger pulse source.
//adc2     AD2CON1bits.ASAM = 1;
//adc2 
//adc2     //  SAMC<4:0>: Auto-Sample Time bits
//adc2     //      11111 = 31 TAD
//adc2     //      .....
//adc2     //      00001 = 1 TAD
//adc2     //      00000 = 0 TAD (only allowed if performing sequential conversions
//adc2     //              using more than one S/H amplifier)
//adc2     //  A note in section 18.15 of the 'dsPIC30F Family Reference Manual' says:
//adc2     //  'At least 1 TAD time period should be allowed between conversions for
//adc2     //  the sample time'.  I think that we are safe since we start sampling
//adc2     //  immediately after last conversion completes.
//adc2     AD2CON3bits.SAMC = 0b00001;
//adc2 
//adc2     //  Select how conversion results are presented in the buffer -------------
//adc2     //  ADCON1:FORM<1:0>: Data Output Format bits
//adc2     //    11(3) = Signed Fractional (DOUT = sddd dddd dd00 0000)
//adc2     //    10(2) = Fractional (DOUT = dddd dddd dd00 0000)
//adc2     //    01(1) = Signed Integer (DOUT = ssss sssd dddd dddd)
//adc2     //    00(0) = Integer (DOUT = 0000 00dd dddd dddd)
//adc2     AD2CON1bits.FORM = 0b00;
//adc2 
//adc2     //  ADDMABM: DMA Buffer Build Mode bit
//adc2     //  1 = DMA buffers are written in the order of conversion. The module will
//adc2     //      provide an address to the DMA channel that is the same as the
//adc2     //      address used for the non-DMA stand-alone buffer
//adc2     //  0 = DMA buffers are written in Scatter/Gather mode. The module will
//adc2     //      provide a scatter/gather address to the DMA channel, based on the
//adc2     //      index of the analog input and the size of the DMA buffer
//adc2     AD2CON1bits.ADDMABM = 1;
//adc2 
//adc2     //  SMPI<3:0>: Selects Increment Rate for DMA Addresses bits or number of
//adc2     //  sample/conversion operations per interrupt
//adc2     //      1111 = Increments the DMA address or generates interrupt after
//adc2     //          completion of every 16th sample/conversion operation
//adc2     //      1110 =  ... every 15th sample/conversion operation
//adc2     //          ...
//adc2     //      0001 =  ... every 2nd sample/conversion operation
//adc2     //      0000 =  ... every sample/conversion operation
//adc2     //
//adc2     AD2CON2bits.SMPI = 0b0001;     //  every 2nd sample/conversion
//adc2 
//adc2     //  DMABL<2:0>: Selects Number of DMA Buffer Locations per Analog Input bits
//adc2     //      111 = Allocates 128 words of buffer to each analog input
//adc2     //      110 = Allocates  64 words of buffer to each analog input
//adc2     //      101 = Allocates  32 words of buffer to each analog input
//adc2     //      100 = Allocates  16 words of buffer to each analog input
//adc2     //      011 = Allocates   8 words of buffer to each analog input
//adc2     //      010 = Allocates   4 words of buffer to each analog input
//adc2     //      001 = Allocates   2 words of buffer to each analog input
//adc2     //      000 = Allocates   1 word  of buffer to each analog input
//adc2     AD2CON4bits.DMABL = 0b000;     //    1 word  of buffer to each analog input
//adc2 	
//adc2  #endif // WIN32
//adc2 }

//-----------------------------------------------------------------------------
//  _Dma0Config
//-----------------------------------------------------------------------------
// DMA0 configuration
// Direction: Read from peripheral address 0-x300 (ADC1BUF0) and write to DMA RAM
// AMODE: Peripheral Indirect Addressing Mode
// MODE: Continuous (NO Ping-Pong) Mode
// IRQ: ADC Interrupt
//-----------------------------------------------------------------------------

static void _Dma0Config(void)
{
  #ifndef WIN32

    DMA0CONbits.CHEN = 0;       // Disable DMA

    //  AMODE<1:0>: DMA Channel Operating Mode Select bits
    //      11 = Reserved
    //      10 = Peripheral Indirect Addressing mode
    //      01 = Register Indirect without Post-Increment mode
    //      00 = Register Indirect with Post-Increment mode
//  DMA0CONbits.AMODE = 0b01;   //  Register Indirect w/o Post-Increment
    DMA0CONbits.AMODE = 0b10;   //  Peripheral Indirect

    //  MODE<1:0>: DMA Channel Operating Mode Select bits
    //      11 = One-Shot, Ping-Pong modes enabled (one block transfer from/to each DMA RAM buffer)
    //      10 = Continuous, Ping-Pong modes enabled
    //      01 = One-Shot, Ping-Pong modes disabled
    //      00 = Continuous, Ping-Pong modes disabled
    DMA0CONbits.MODE = 0b00;    //  Continuous, no ping-pong

    //  DMA Channel 0 Peripheral Address Register
    DMA0PAD=(int)&ADC1BUF0;

    //  DMA Channel 0 Transfer Count Register
    //  Number of DMA Transfers = Count + 1;
    DMA0CNT = (4-1);

    //  DMA Channel 0 IRQ Select Register
    DMA0REQ = 0b001101;		// 0001101 = ADC1 – ADC1 Convert done

    //  DMA Channel 0 RAM Start Address Offset Register A
    DMA0STA = __builtin_dmaoffset(&Adc1Dma0Buf);
	
  #endif // WIN32
}

//-----------------------------------------------------------------------------
static void _Dma0Start(void)
{
    IFS0bits.DMA0IF = 0;            // Clear the DMA interrupt flag bit
    IEC0bits.DMA0IE = 1;            // Set the DMA interrupt enable bit

    DMA0CONbits.CHEN = 1;           // Enable DMA
}

//adc2 //-----------------------------------------------------------------------------
//adc2 //  _Dma3Config
//adc2 //-----------------------------------------------------------------------------
//adc2 // DMA0 configuration
//adc2 // Direction: Read from peripheral address 0-x300 (ADC1BUF0) and write to DMA RAM
//adc2 // AMODE: Peripheral Indirect Addressing Mode
//adc2 // MODE: Continuous (NO Ping-Pong) Mode
//adc2 // IRQ: ADC Interrupt
//adc2 //-----------------------------------------------------------------------------
//adc2 
//adc2 static void _Dma3Config(void)
//adc2 {
//adc2   #ifndef WIN32                                     
//adc2 
//adc2     DMA3CONbits.CHEN = 0;       // Disable DMA
//adc2 
//adc2     //  AMODE<1:0>: DMA Channel Operating Mode Select bits
//adc2     //      11 = Reserved
//adc2     //      10 = Peripheral Indirect Addressing mode
//adc2     //      01 = Register Indirect without Post-Increment mode
//adc2     //      00 = Register Indirect with Post-Increment mode
//adc2     DMA3CONbits.AMODE = 0b10;   //  Peripheral Indirect
//adc2 
//adc2     //  MODE<1:0>: DMA Channel Operating Mode Select bits
//adc2     //      11 = One-Shot, Ping-Pong modes enabled (one block transfer from/to each DMA RAM buffer)
//adc2     //      10 = Continuous, Ping-Pong modes enabled
//adc2     //      01 = One-Shot, Ping-Pong modes disabled
//adc2     //      00 = Continuous, Ping-Pong modes disabled
//adc2     DMA3CONbits.MODE = 0b00;    //  Continuous, no ping-pong
//adc2 
//adc2    //  DMA Channel 0 Peripheral Address Register
//adc2    DMA3PAD=(int)&ADC2BUF0;
//adc2 
//adc2    //  DMA Channel 0 Transfer Count Register
//adc2    //  Number of DMA Transfers = Count + 1;
//adc2    DMA3CNT = (4-1);
//adc2 
//adc2    //  DMA Channel 0 IRQ Select Register
//adc2    DMA3REQ = 0b010101;		// 0010101 = ADC2 – ADC2 Convert Done
//adc2 
//adc2    //  DMA Channel 0 RAM Start Address Offset Register A
//adc2    DMA3STA = __builtin_dmaoffset(&Adc2Dma3Buf);
//adc2 
//adc2   #endif // WIN32
//adc2 }
//adc2 
//adc2 
//adc2 static void _Dma3Start(void)
//adc2 {
//adc2     IFS2bits.DMA3IF = 0;            // Clear the DMA interrupt flag bit
//adc2     IEC2bits.DMA3IE = 1;            // Set the DMA interrupt enable bit
//adc2 
//adc2     DMA3CONbits.CHEN = 1;           // Enable DMA
//adc2 }

//-----------------------------------------------------------------------------
static void _init_analog_reading(ANALOG_READING_t * rdg, int16_t avg_value)
{
    rdg->raw_val = 0;

    rdg->avg.sum[0] = (uint32_t)0;
    rdg->avg.sum[1] = (uint32_t)0;
    rdg->avg.val = 0;

    rdg->rms.sos[0] = (uint32_t)0;
    rdg->rms.sos[1] = (uint32_t)0;
    rdg->rms.val = 0;
    rdg->avg.val = avg_value;
}

//-----------------------------------------------------------------------------
void _Adc1Start(void)
{
    //  Initialize Analog structures
    _init_analog_reading(&An.Status.VBatt, 0);
	//	TODO: 'OPTION_HAS_CHGR_EN_SWITCH' Conditional compile can be removed.
  #if !defined(OPTION_HAS_CHGR_EN_SWITCH)
    _init_analog_reading(&An.Status.BTemp, 0);
  #endif
    An.Status.VAC_raw_max = An.Status.VAC_run_max = 0;
    An.Status.VAC_raw_min = An.Status.VAC_run_min = MAX_A2D_VALUE;
    _init_analog_reading(&An.Status.VAC   , MID_A2D_VALUE);
    _init_analog_reading(&An.Status.WACr  , MID_A2D_VALUE);
    _init_analog_reading(&An.Status.IMeas , IMEAS_ZERO_CROSS_A2D);
    _init_analog_reading(&An.Status.ILine , MID_A2D_VALUE);
    _init_analog_reading(&An.Status.VReg15, 0);    //  a.k.a. 15V_RAIL_RESET

  #if IS_DEV_CONVERTER && !IS_PCB_BDC
    _init_analog_reading(&An.Status.HsTemp, 0);
  #endif

    An.Status.RmsSosNSamples[0] = 0;
    An.Status.RmsSosNSamples[1] = 0;
    An.Status.RmsSosIndxIsr     = 0;
    An.Status.RmsSosIndxFg      = 1;

    IFS0bits.AD1IF = 0;         // Clear ISR flag
    IEC0bits.AD1IE = 0;         // DO NOT Enable ADC interrupts - We use DMA
    AD1CON1bits.ADON = 1;       // turn ADC ON
}

//adc2 //-----------------------------------------------------------------------------
//adc2 //  _Adc2Start
//adc2 //-----------------------------------------------------------------------------
//adc2 //  Start ADC interrupt
//adc2 //
//adc2 //-----------------------------------------------------------------------------
//adc2 
//adc2 void _Adc2Start(void)
//adc2 {
//adc2     IFS1bits.AD2IF = 0;         // Clear ISR flag
//adc2     IEC1bits.AD2IE = 0;         // DO NOT Enable ADC interrupts - We use DMA
//adc2     AD2CON1bits.ADON = 1;       // turn ADC ON
//adc2 	
//adc2 	AD2CON1bits.SAMP = 1;		//	tell AD2 to sample
//adc2 }

//-----------------------------------------------------------------------------
//	Other Statistical Functions
//
//	'_CalcStats' was developed for Rate-Of-Change detection, to calculate the 
//	Standard Deviation. _CalcStats is called once every 1.067 seconds.
//	NOTE: In charger mode we don't have control of the zero-crossing, so timing
//	will be approximate.
//-----------------------------------------------------------------------------

ANALOG_STATS_t WACr_stats = {MEAN_LEN, MEAN_SHIFT, {0}, 0, 0, 0};

static void _CalcStats(ANALOG_STATS_t* dest, int16_t val)
{
	//	Calculate the running average of the last n-samples
	//	subtract the oldest from the sum
	//	overwrite the oldest with the newest [val]
    volatile int32_t _sum = 0;
	int16_t	ix;
	
	
	dest->ay[dest->ix] = val;                  	//  overwrite oldest with newest
	
	//	This could be refactored (simply) into a 'running' calculation. However, 
	//	'len' is initialized to 64. Therefore, the body of code below (implied
	//	'else' condition) is executed once every 64 * 1.067 = 68.27 seconds.
	if(++dest->ix < dest->len)
		return;
	//	else
	dest->ix = 0;
	
	//  calculate the mean (average)
    _sum = 0;
	for(ix = 0; ix < dest->len; ix++) 
    { 
        _sum += dest->ay[ix]; 
    }
    dest->mean = (_sum >> dest->shift);    	
	
	//  calculate the variance
	_sum = 0;
    for(ix = 0; ix < dest->len; ix++) 
	{ 
		int16_t temp;
		
		temp = (dest->ay[ix] - dest->mean);
		_sum += __builtin_mulss(temp, temp);
	}
	//	Shift by three, which is divide-by-eight.
	dest->var = (int32_t)(_sum >> dest->shift);
    dest->dev = isqrt32(dest->var);
}

//-----------------------------------------------------------------------------
//                        A V E R A G E S
//-----------------------------------------------------------------------------

static int16_t _AcAvgIx = 0;

//-----------------------------------------------------------------------------
//	_initAvg()
//
//	Prime the array with the supplied 'val' and calculate the average.
//
//	NOTES: 
//	  This array is 64-elements long.  It is updated each AC cycle - at the 
//	  zero-crossing.  That means that it requires a little over one-second before 
//	  the averages are valid. (The flag An.AvgValid indicates valid averages.)
//	  The passed variable 'val' allows the array to be 'primed' with a value.  
//	  This may be used so that we don't have to wait on startup.
//
//-----------------------------------------------------------------------------

INLINE void _initAvg(ANALOG_AVG_t * dest, int16_t val)
{
    dest->sum = 0L;
    for(_AcAvgIx = 0; _AcAvgIx<AC_AVG_LEN; _AcAvgIx++)
    {
        dest->ay[_AcAvgIx] = val;
	    dest->sum += val;
	}
    dest->val = (dest->sum >> AC_AVG_SHIFT);
}

//-----------------------------------------------------------------------------
static void _InitAvgs(void)
{
    _initAvg(&An.AvgBTemp,  MID_A2D_VALUE);         //	Battery Temperature (mid range)
    _initAvg(&An.AvgILimit, IMEAS_ZERO_CROSS_A2D);  //	Charger Current Limit (RMS)
    _initAvg(&An.AvgILine,  MID_A2D_VALUE);         //	Line Current (RMS)
    _initAvg(&An.AvgIMeas,  IMEAS_ZERO_CROSS_A2D);  //	Transformer Current (RMS)
    _initAvg(&An.AvgWACr,   0);                     //	'Real' Watts AC
    
    _AcAvgIx    = 0;
    An.AvgValid = 0;
}

//-----------------------------------------------------------------------------
INLINE void _updateAvg(ANALOG_AVG_t * dest, int16_t * src)
{
    dest->sum -= dest->ay[_AcAvgIx];			//  subtract oldest
    dest->ay[_AcAvgIx] = *src;                  //  overwrite with newest
    dest->sum += dest->ay[_AcAvgIx];            //  add newest
    dest->val = (dest->sum >> AC_AVG_SHIFT);    //  calculate average
}

//-----------------------------------------------------------------------------
// _UpdateAvgs is called from TASK_AnalogData once-per-cycle at approximately 
//	the zero-crossing after the RMS (and other) values have been computed.  
//	A 'running' average is calculated so synchronization of the index and the 
//	zero-crossing is not important. 
//-----------------------------------------------------------------------------

static void _UpdateAvgs(void)
{
    _updateAvg(&An.AvgBTemp, (int16_t *)&An.Status.BTemp.avg.val);
    _updateAvg(&An.AvgILimit, &An.Status.ILimit.rms.val);
    _updateAvg(&An.AvgILine,  &An.Status.ILine.rms.val);
    _updateAvg(&An.AvgIMeas,  &An.Status.IMeas.rms.val);
    _updateAvg(&An.AvgWACr, (int16_t *)&An.Status.WACr.avg.val);
	
	//	AC_AVG_LEN is #defined in 'analog.h' to be 64.  That means that the
	//	code below will be executed once per 64/60 = 1.067 seconds.
    if(++_AcAvgIx >= AC_AVG_LEN) 
    {
		_CalcStats(&WACr_stats, An.AvgWACr.val);
        _AcAvgIx = 0;
		
		//	An.AvgValid is a global flag to indicate that the averages are 
		//	valid - the arrays have been filled (at least once) with valid data.
        An.AvgValid = 1;
    }
} 

//-----------------------------------------------------------------------------
//  _DMA0Interrupt(): ISR name is chosen from the device linker script.
//-----------------------------------------------------------------------------
//  ADC Interrupt Service Routine
//
//	---------------+-----------+-----------------+------------------+----------------
//	 Buffer        | MUX Chan  | LPC             | NP               | DC
//	---------------+-----------+-----------------+------------------+---------------- 
//  Adc1Dma0Buf[0] | MUX A,CH0 | AN8/15V_RAIL    | AN8/CHRG_I_LIMIT | AN8/CHRG_I_LIMIT  
//  Adc1Dma0Buf[1] | MUX A,CH1 | AN0/E10_SCALED  | AN0/E10_SCALED   | AN0/E10_SCALED (DCin_SCALED)    
//  Adc1Dma0Buf[2] | MUX A,CH2 | AN1/V_AC        | AN1/V_AC         | AN1/DCout_SCALED          
//  Adc1Dma0Buf[3] | MUX A,CH3 | AN2/OVL         | AN2/HS_BAR_IN    | AN2/HS_BAR_IN     
//  Adc1Dma0Buf[4] | MUX B,CH0 | AN2/OVL         | AN11/15V_RAIL    | AN11/15V_RAIL     
//  Adc1Dma0Buf[5] | MUX B,CH1 | AN3/BATT_TEMP   | AN3/BATT_TEMP    | AN3/BATT_TEMP     
//  Adc1Dma0Buf[6] | MUX B,CH2 | AN4/I_MEASURE   | AN4/I_MEASURE    | AN4/I_MEASURE     
//  Adc1Dma0Buf[7] | MUX B,CH3 | AN5/I_LOAD_MGMT | AN5/I_LOAD_MGMT  | AN5/THERMISTOR   
//	
//	Analog (structures) - where the analog data goes 
//	The structure ANALOG_READING_t was designed to accomodate collection and 
//	processing of data for RMS and/or averaging calculations.
//	Usage is hardware dependent - These may not be used by all platforms.
//
//	 1	ANALOG_READING_t VBatt;   	//  Battery Voltage: E10_SCALED, BATT_V
//	 2	ANALOG_READING_t VAC;     	//  AC Voltage: V_AC, INV_COMP
//	 3	ANALOG_READING_t IMeas;   	//  Inverter I-Out, Charger I-In: I_MEASURE_CL, I_MEASURE
//	 4	ANALOG_READING_t ILine;   	//  Line Current: I_LOAD_MGMT, LOAD_MGMT
//	 5	ANALOG_READING_t ILimit;  	//  Current Limit: CHRG_I_LIMIT
//	 6	ANALOG_READING_t BTemp;   	//  Battery Temperature: BATT_TEMP
//	 7	ANALOG_READING_t HsTemp;  	//  Heatsink Temperature: HS_BAR_IN; DC+DC uses for DC setpoint
//	 8	ANALOG_READING_t VReg15;  	//  Regulated Voltage
//	 9	ANALOG_READING_t WACr; 		//  Watts AC - Real Power
//
//  10	ADC10_RDG_t Ovl;           	//  Overload - VDS Signal raw value only! 
//
//  TIMING:
//  2016-03-30 - Instrumentation was added to measure execution times below:
//      DMA0 ISR Execution time:    22.0 microseconds
//      DMA0 ISR Period:            43.3 microseconds
//
//  2016-04-01 - Repeated timing test with Optimization 1
//      DMA0 ISR Execution time:    10.2 microseconds
//  
//-----------------------------------------------------------------------------

// typical execute time ~25usec
void __attribute__((interrupt, no_auto_psv)) _DMA0Interrupt(void)
{
    int16_t temp;
    int16_t temp2;

  #ifdef  ENABLE_TASK_TIMING
    g_dmaTiming.count++; // one more isr
    TMR7 = 0;  // clear timer
  #endif
 
//adc2	AD2CON1bits.SAMP = 0;		//	Trigger AD2 - Clearing the SAMP bit ends sampling and starts conversion

	//	TODO: 'OPTION_HAS_CHGR_EN_SWITCH' Conditional compile can be removed.
  #if !defined(OPTION_HAS_CHGR_EN_SWITCH)
    //  Adc1Dma0Buf[5]   MUX B,CH1   AN3 = BATT_TEMP
    An.Status.BTemp.raw_val = Adc1Dma0Buf[5];
  #endif
    
	//--- Platform-Specific Channel Assignments ----------------------------
	
  #if IS_PCB_LPC
    An.Status.VReg15.raw_val 	= Adc1Dma0Buf[0];       // AN8/15V_RAIL 	
    An.Status.VBatt.raw_val 	= Adc1Dma0Buf[1] << 1;  // AN0/E10_SCALED 
    An.Status.VAC.raw_val 		= Adc1Dma0Buf[2];       // AN1/V_AC       
    An.Status.Ovl 				= Adc1Dma0Buf[3];       // AN2/OVL        
    An.Status.BTemp.raw_val 	= Adc1Dma0Buf[5];       // AN2/OVL        
    An.Status.IMeas.raw_val 	= Adc1Dma0Buf[6] << 2;  // AN3/BATT_TEMP  
    // LPC has no ILimit signal so it is set to IMeas
    An.Status.ILimit.raw_val	= An.Status.IMeas.raw_val;
    An.Status.ILine.raw_val 	= Adc1Dma0Buf[7];       // AN5/I_LOAD_MGMT

	//  IMPORTANT: inv_CheckForOverload() uses the following parameters, which
	//  must be up-to-date before calling:
	//      An.Status.Ovl
	//      An.Status.IMeas.avg.val  <-- its OK to use an old-er value
	//      An.Status.IMeas.raw_val
	inv_CheckForOverload();
	
  #elif IS_PCB_NP || IS_PCB_BDC 
    An.Status.ILimit.raw_val	= Adc1Dma0Buf[0];       // AN8/CHRG_I_LIMIT
    An.Status.VBatt.raw_val 	= Adc1Dma0Buf[1] << 1;  // AN0/E10_SCALED  
    An.Status.VAC.raw_val 		= Adc1Dma0Buf[2];       // AN1/V_AC        
    An.Status.HsTemp.raw_val	= Adc1Dma0Buf[3];       // AN2/HS_BAR_IN   
    An.Status.VReg15.raw_val 	= Adc1Dma0Buf[4];       // AN11/15V_RAIL   
    An.Status.BTemp.raw_val 	= Adc1Dma0Buf[5];       // AN3/BATT_TEMP   
    An.Status.IMeas.raw_val 	= Adc1Dma0Buf[6] << 2;  // AN4/I_MEASURE   
    An.Status.ILine.raw_val 	= Adc1Dma0Buf[7];       // AN5/I_LOAD_MGMT 
	  
  #else
	#error "OPTION_PCB not set by models.h"
  #endif	//	OPTION_PCB

    // update min/max for raw VAC for this full cycle
    if (An.Status.VAC.raw_val > An.Status.VAC_run_max) An.Status.VAC_run_max = An.Status.VAC.raw_val;
    if (An.Status.VAC.raw_val < An.Status.VAC_run_min) An.Status.VAC_run_min = An.Status.VAC.raw_val;
    
    //  Average Sum
    An.Status.VAC.avg.sum   [An.Status.RmsSosIndxIsr] += An.Status.VAC.raw_val;
    An.Status.VBatt.avg.sum [An.Status.RmsSosIndxIsr] += An.Status.VBatt.raw_val;
    An.Status.IMeas.avg.sum [An.Status.RmsSosIndxIsr] += An.Status.IMeas.raw_val;
    An.Status.ILine.avg.sum [An.Status.RmsSosIndxIsr] += An.Status.ILine.raw_val;
    An.Status.ILimit.avg.sum[An.Status.RmsSosIndxIsr] += An.Status.ILimit.raw_val;
    An.Status.HsTemp.avg.sum[An.Status.RmsSosIndxIsr] += An.Status.HsTemp.raw_val;

    //  RMS Sum-of-Squares
    //  RmsSosIndxIsr is managed by chgr_PWM_ISR. This ISR (_ADCInterrupt) is
    //  triggered by the PWM clock, so semaphores are not necessary.

    temp = An.Status.ILimit.raw_val - An.Status.ILimit.avg.val;
    An.Status.ILimit.rms.sos[An.Status.RmsSosIndxIsr] += __builtin_mulss(temp, temp); 
  
    temp = An.Status.ILine.raw_val - An.Status.ILine.avg.val;
    An.Status.ILine.rms.sos[An.Status.RmsSosIndxIsr] += __builtin_mulss(temp, temp);

    temp = An.Status.VAC.raw_val - An.Status.VAC.avg.val;
    An.Status.VAC.rms.sos  [An.Status.RmsSosIndxIsr] += __builtin_mulss(temp, temp);

    temp2 = An.Status.IMeas.raw_val - An.Status.IMeas.avg.val;
    An.Status.IMeas.rms.sos[An.Status.RmsSosIndxIsr] += __builtin_mulss(temp2, temp2);

    //  AC Watts - Real Power (WACr = Watts AC real)
    //  from prior calculations: temp = An.Status.VAC and temp2 = An.Status.IMeas
    An.Status.WACr.avg.sum [An.Status.RmsSosIndxIsr] += __builtin_mulss(abs(temp), abs(temp2));
    
    An.Status.RmsSosNSamples[An.Status.RmsSosIndxIsr]++;
    
   #if defined(OPTION_SSR)
	//	We do SSR-detection here, because this provides the shortest latency
    ssr_Detect();
   #endif
    
    IFS0bits.DMA0IF = 0;        // Clear the DMA0 Interrupt Flag

  #ifdef  ENABLE_TASK_TIMING
    {
    uint16_t ticks = TMR7;
    g_dmaTiming.tsum += ticks;  // add to sum
    if (ticks < g_dmaTiming.tmin) g_dmaTiming.tmin = ticks;  // min value
    if (ticks > g_dmaTiming.tmax) g_dmaTiming.tmax = ticks;  // max value
    }
  #endif

	}

//adc2  //-----------------------------------------------------------------------------
//adc2  //  _DMA3Interrupt(): ISR name is chosen from the device linker script.
//adc2  //-----------------------------------------------------------------------------
//adc2  //  ADC2 Interrupt Service Routine
//adc2  //
//adc2  //	Adc2Dma3Buf[0] = AN6
//adc2  //	Adc2Dma3Buf[1] = AN0/E10_SCALED  --> (DCin_SCALED)
//adc2  //	Adc2Dma3Buf[2] = AN1/V_AC		 -->  An.Status.VAC.raw_val         
//adc2  //	Adc2Dma3Buf[3] = AN2/HS_BAR_IN   -->  An.Status.HsTemp
//adc2  	   
//adc2  //	Adc2Dma3Buf[4] = AN7
//adc2  //	Adc2Dma3Buf[5] = AN3/BATT_TEMP   -->  An.Status.BTemp
//adc2  //	Adc2Dma3Buf[6] = AN4/I_MEASURE   -->  An.Status.IMeas
//adc2  //	Adc2Dma3Buf[7] = AN5/THERMISTOR  -->  An.Status.ILine
//adc2  
//adc2  void __attribute__((interrupt, no_auto_psv)) _DMA3Interrupt(void)
//adc2  {
//adc2  	//	AN6 & AN7 were used for testing - an analog signal could be connected 
//adc2  	//	to these via the ICP header.
//adc2  	
//adc2  	//	NOTE: During testing it was observed that I had to read this buffer 
//adc2  	//	'Adc2Dma3Buf', else DMA3 interrupts did not occur.  
//adc2  	//	TODO: Verify if/why it is necessary to read Adc2Dma3Buf for DMA3 interrupts.
//adc2  	
//adc2      an_AN6 = Adc2Dma3Buf[0];	//	AN6
//adc2      an_AN7 = Adc2Dma3Buf[4];	//	AN7
//adc2  
//adc2      IFS2bits.DMA3IF = 0;    	//	Clear the DMA3 Interrupt Flag
//adc2  	AD2CON1bits.SAMP = 1;		//	tell AD2 to sample
//adc2  }


//-----------------------------------------------------------------------------
//  process analog data
//-----------------------------------------------------------------------------

// typical execute time ~65 usec (when run in interrupt routine)
void TASK_AnalogData(void)
{
    #define DIV_BY_MAX_SINE_Q16 (int32_t)(((int32_t)(0x00010000))/(MAX_SINE * 2))

    DWORD_t temp32;
    int16_t n_samples;

    n_samples = An.Status.RmsSosNSamples[An.Status.RmsSosIndxFg];
    // check for zero, so that we don't cause a divide-by-zero
    if(0 == n_samples)
        return;
    
    //  Load Current is calculated as an RMS value. It is measured twice each
    //  PWM clock cycle by the ADC. The ADC ISR calculates the square of the
    //  readings, then adds them to the Sum-of-Squares value.
    //  This function calculates the square-root of the Sum-of-Squares value
    //  once-per-cycle, at the zero-crossing.

    //  IMeas AC Current RMS calculations
    An.Status.IMeas.avg.val = __builtin_divud(An.Status.IMeas.avg.sum[An.Status.RmsSosIndxFg], n_samples);
	if(An.Status.IMeas.avg.val <= 0) LOG(SS_SYS,SV_ERR, "An.Status.IMeas.avg.val <= 0, %s, line-%d ", __FILE__, __LINE__);
    An.Status.IMeas.avg.sum[An.Status.RmsSosIndxFg] = 0;

    temp32.dword = An.Status.IMeas.rms.sos[An.Status.RmsSosIndxFg];
    temp32.dword >>= 5;
    temp32.dword *= DIV_BY_MAX_SINE_Q16;
	temp32.dword >>= 11;
    An.Status.IMeas.rms.val = isqrt32(temp32.dword);
    An.Status.IMeas.rms.sos[An.Status.RmsSosIndxFg] = (uint32_t)0;

    //  ILimit AC Current RMS calculations
  #if IS_PCB_LPC
    //  The LPC board does not have ILimit. Instead we scale IMeasure.
    An.Status.ILimit = An.Status.IMeas;
  #else
    An.Status.ILimit.avg.val = __builtin_divud(An.Status.ILimit.avg.sum[An.Status.RmsSosIndxFg], n_samples);
    An.Status.ILimit.avg.sum[An.Status.RmsSosIndxFg] = 0;

    temp32.dword = An.Status.ILimit.rms.sos[An.Status.RmsSosIndxFg];
    temp32.dword >>= 5;
    temp32.dword *= DIV_BY_MAX_SINE_Q16;
    temp32.dword >>= 11;
    An.Status.ILimit.rms.val = isqrt32(temp32.dword);
    An.Status.ILimit.rms.sos[An.Status.RmsSosIndxFg] = (uint32_t)0;
  #endif
  
    //  ILine AC Current RMS calculations
    An.Status.ILine.avg.val = __builtin_divud(An.Status.ILine.avg.sum[An.Status.RmsSosIndxFg], n_samples);
    An.Status.ILine.avg.sum[An.Status.RmsSosIndxFg] = 0;

    temp32.dword = An.Status.ILine.rms.sos[An.Status.RmsSosIndxFg];
    temp32.dword >>= 5;
    temp32.dword *= DIV_BY_MAX_SINE_Q16;
    temp32.dword >>= 11;
    An.Status.ILine.rms.val = isqrt32(temp32.dword);
    An.Status.ILine.rms.sos[An.Status.RmsSosIndxFg] = (uint32_t)0;

    //  AC Voltage RMS calculations
    An.Status.VAC.avg.val = __builtin_divud(An.Status.VAC.avg.sum[An.Status.RmsSosIndxFg], n_samples);
    An.Status.VAC.avg.sum[An.Status.RmsSosIndxFg] = 0;
    // udpate VAC min/max full cycle values
    An.Status.VAC_raw_max = An.Status.VAC_run_max;
    An.Status.VAC_raw_min = An.Status.VAC_run_min;
    An.Status.VAC_run_max = 0;
    An.Status.VAC_run_min = MAX_A2D_VALUE;

    temp32.dword = An.Status.VAC.rms.sos[An.Status.RmsSosIndxFg];
    temp32.dword >>= 2;
    An.Status.VAC.rms.val = isqrt32(__builtin_divud(temp32.dword, n_samples));
    An.Status.VAC.rms.val <<= 1;
    An.Status.VAC.rms.sos[An.Status.RmsSosIndxFg] = (uint32_t)0;

    //  Battery Voltage calculations
    An.Status.VBatt.avg.val = __builtin_divud(An.Status.VBatt.avg.sum[An.Status.RmsSosIndxFg], n_samples);
    An.Status.VBatt.avg.sum[An.Status.RmsSosIndxFg] = (int32_t)0;

  #if IS_DEV_CONVERTER && !IS_PCB_BDC
    // DC output voltage
    An.Status.HsTemp.avg.val = __builtin_divud(An.Status.HsTemp.avg.sum[An.Status.RmsSosIndxFg], n_samples);
    An.Status.HsTemp.avg.sum[An.Status.RmsSosIndxFg] = 0L;
  #endif

    // Sum is divided by 4 because IMeas is multiplied by 4; remove this scaling factor; use unsigned divide    
    An.Status.WACr.avg.val = __builtin_divud(An.Status.WACr.avg.sum[An.Status.RmsSosIndxFg]/4, (2 * n_samples));
    An.Status.WACr.avg.sum[An.Status.RmsSosIndxFg] = 0;

    An.Status.RmsSosNSamples[An.Status.RmsSosIndxFg] = 0;

    //  Battery Temperature calculations - NOT Volta
    //  The battery temperature sensor is an RTD. Its analog value changes very 
    //  slowly. Therefore, we should not need to calculate an average. 
    An.Status.BTemp.avg.val = An.Status.BTemp.raw_val;
    
    _UpdateAvgs();
}

//------------------------------------------------------------------------------
//  an_ProcessAnalogData
//
//  This function schedules the task to be run.
//-----------------------------------------------------------------------------

void an_ProcessAnalogData(void)
{
    //  Process RMS calculations
    An.Status.RmsSosIndxFg   = An.Status.RmsSosIndxIsr;
    An.Status.RmsSosIndxIsr ^= 0x0001;

    task_MarkAsReady(an_task);
}

//------------------------------------------------------------------------------
//                          d s P I C 3 0 F  
//------------------------------------------------------------------------------

void an_Config(void)
{
    _Adc1Config();	_Dma0Config();
//adc2    _Adc2Config();	_Dma3Config();
    an_task = task_AddToQueue(TASK_AnalogData, "ana  "); 
}

//------------------------------------------------------------------------------
void an_Start(void)
{
    _InitAvgs();
    _Adc1Start();	_Dma0Start();	
//adc2    _Adc2Start();	_Dma3Start();	
}

// <><><><><><><><><><><><><> analog_dsPIC33F.c <><><><><><><><><><><><><><><><><><><><><><>
