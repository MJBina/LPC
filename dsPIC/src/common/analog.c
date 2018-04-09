// <><><><><><><><><><><><><> analog.c <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//  Copyright(C) 2015 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
//  Analog to Digial Conversion  Routines
//
//  This file contains analog functions which are 'generic' in that they do
//  not require special hardware.  It is a sister to "analog_dsPIC33F.c" which
//  is specific to the dsPIC33F (i.e. it uses DMA).
//  
//-----------------------------------------------------------------------------

// --------
// headers
// --------
#include "options.h"    // must be first include
#include "analog.h"
#include "sine_table.h"
#include "charger.h"
#include "inverter.h"
#include "config.h"
#include "rv_can.h"


// --------------------------
// Conditional Debug Compiles
// --------------------------

// comment out flag to not use it
//#define  DEBUG_AN_CONVERSIONS  1 // debug analog conversions


// -----------------------------
// Auto Removal for build types
// -----------------------------
#if OPTION_NO_CONDITIONAL_DBG
  #undef DEBUG_AN_CONVERSIONS
#endif


// ----------
// Constants
// ----------
#define MINIMUM_WATTS  (10)    // show zero if less that this
#define IMEAS_OFFSET   (2048)  // (A2D counts) offset bias for inverter startup (IMeas is x 4)

// -----------
// global data
// -----------
ANALOG_t An;
ANALOG_t AnSaved;  // saved analog state on error for diagnostics
uint8_t  devSaved; // 1=inverter, 2=charger

// ----------------------------------------------------------------------
//  Initialize data fields used for RMS conversions
void an_InitRms()
{
    An.Status.RmsSosIndxIsr = 0;
    An.Status.RmsSosIndxFg  = (An.Status.RmsSosIndxIsr ^ 0x0001);
    
    //  TODO: Should An.Status.VBatt be initialized here?
    An.Status.IMeas.rms.sos[An.Status.RmsSosIndxIsr] = ((int32_t)IMEAS_OFFSET * MAX_SINE * 2);
    An.Status.IMeas.rms.sos[An.Status.RmsSosIndxFg ] = ((int32_t)IMEAS_OFFSET * MAX_SINE * 2);
}

// ----------------------------------------------------------------------
// save analog state on error for diagnostics
void an_SaveState(int dev)  // dev 1=inverter, 2=charger
{
    // make a copy of it
    memcpy(&AnSaved, &An, sizeof(ANALOG_STATUS_t));
    devSaved = dev;
}

// ----------------------------------------------------------------------
float an_GetBatteryVoltage(uint8_t saved)
{
    int16_t adc; 
    float volts;

    if (saved) adc = AnSaved.Status.VBatt.avg.val;
    else       adc = VBattCycleAvg();

    volts = VBATT_ADC_VOLTS(adc);
    if (volts < 0.0) volts = 0.0;
  #ifdef DEBUG_AN_CONVERSIONS
    LOG(SS_DAC, SV_DBG, "OutMVoltsDC=%ld VBatt.avg.val=%d", VOLTS_MVOLTS(volts), adc);
  #endif
    return(volts);
}

// ----------------------------------------------------------------------
float an_GetChgrBatteryCurrent(uint8_t saved)
{
    int16_t wattsAdc,vbattAdc; 
	float dc_amps;

    if (!IsChgrOutputting()) return(0.0);
    if (saved)
    {
        vbattAdc = AnSaved.Status.VBatt.avg.val;
        wattsAdc = AnSaved.AvgWACr.val;
    }
    else
    {
        vbattAdc = VBattCycleAvg();
        wattsAdc = WattsLongAvg();
    }

    if (vbattAdc == 0)
        dc_amps = 0.0;
    else
        dc_amps = CHG_DC_AMPS(vbattAdc,wattsAdc);
    if (dc_amps < 0.0) dc_amps = 0.0;
  #ifdef DEBUG_AN_CONVERSIONS
    LOG(SS_DAC, SV_DBG, "OutMAmpsDC=%ld wattsAdc=%d VBatt.adc=%d", AMPS_MAMPS(dc_amps), wattsAdc, vbattAdc);
  #endif
    return(dc_amps);
}

// ----------------------------------------------------------------------
// charger bypass current = ILine - IMeas
float an_GetChgrBypassCurrent(uint8_t saved)
{
    float dc_amps, ILineAmps, IMeasAmps;

    ILineAmps = an_GetChgrILineCurrent(saved);
    IMeasAmps = an_GetChgrIMeasCurrent(saved);
    dc_amps   = ILineAmps - IMeasAmps;
    
  #ifdef DEBUG_AN_CONVERSIONS
    LOG(SS_DAC, SV_DBG, "ChgBypassMAmps=%ld", AMPS_MAMPS(dc_amps));
  #endif
    return(dc_amps);
}

// ----------------------------------------------------------------------
float an_GetInvBatteryCurrent(uint8_t saved)
{
    int16_t wattsAdc,vbattAdc; 
    float dc_amps;
    
    if (!IsInvOutputting()) return(0.0);
    if (saved)
    {
        vbattAdc = AnSaved.Status.VBatt.avg.val;
        wattsAdc = AnSaved.AvgWACr.val;
    }
    else
    {
        vbattAdc = VBattCycleAvg();
        wattsAdc = WattsLongAvg();
    }

    dc_amps = INV_DC_AMPS(vbattAdc,wattsAdc);
    if (dc_amps < 0.0) dc_amps = 0.0;
  #ifdef DEBUG_AN_CONVERSIONS
    LOG(SS_DAC, SV_DBG, "InMAmpsDC=%ld wattsAdc=%d VBattAdc=%d", AMPS_MAMPS(dc_amps), wattsAdc, vbattAdc);
  #endif
    return(dc_amps);
}

// ----------------------------------------------------------------------
float an_GetInvAcVoltage(uint8_t saved)
{
    int16_t adc; 
    float volts;
	
    if (!IsInvActive()) return(0.0);
    if (saved) adc = AnSaved.Status.VAC.rms.val;
    else       adc = VacRMS();

    volts = VAC_ADC_VOLTS(adc);
    if (volts < 0.0) volts = 0.0;
  #ifdef DEBUG_AN_CONVERSIONS
    LOG(SS_DAC, SV_DBG, "OutMVoltsAC=%ld VAC.rms.val=%d", VOLTS_MVOLTS(volts), adc);
  #endif
    return(volts);
}

// ----------------------------------------------------------------------
float an_GetChgrAcVoltage(uint8_t saved)
{
    int16_t adc; 
    float volts;
	
    if (saved) adc = AnSaved.Status.VAC.rms.val;
    else       adc = VacRMS();

    volts = VAC_ADC_VOLTS(adc);
    if (volts < 0.0) volts = 0.0;
  #ifdef DEBUG_AN_CONVERSIONS
    LOG(SS_DAC, SV_DBG, "InMVAC=%ld VAC.rms.val=%d", VOLTS_MVOLTS(volts), adc);
  #endif
    return(volts);
}

// ----------------------------------------------------------------------
//  There are 2 current sensors on the LPC: ILine and IMeas.  These measure the 
//  current at different points in the circuit, depending on mode.
//  Inverter Mode:
//      ILine is not used.
//      IMeas measures current output by the inverter.
//  Charger Mode:
//      ILine measures total line current through the unit.
//      IMeas measures current input the charger.
//  RV-C may require a sign bit to indicate current direction.  
//  This can be applied to the results from the static functions here.
// ----------------------------------------------------------------------

float an_GetInvIMeasCurrent(uint8_t saved)
{
    int16_t adc; 
    float amps;
    
    if (saved) adc = AnSaved.Status.IMeas.rms.val;
    else       adc = IMeasRMS();

    amps = IMEAS_INV_ADC_AMPS(adc);
    if (amps < 0.0) amps = 0.0;
  #ifdef DEBUG_AN_CONVERSIONS
    LOG(SS_DAC, SV_DBG, "OutMAmpsAC=%ld IMeas.rms.val=%d", AMPS_MAMPS(amps), adc);
  #endif
    return(amps);
}
 
// ----------------------------------------------------------------------
float an_GetChgrIMeasCurrent(uint8_t saved)
{
    int16_t adc; 
    float amps;

    if (saved) adc = AnSaved.Status.IMeas.rms.val;
    else       adc = IMeasRMS();

    amps = IMEAS_CHG_ADC_AMPS(adc);
    if (amps < 0.0) amps = 0.0;
  #ifdef DEBUG_AN_CONVERSIONS
    LOG(SS_DAC, SV_DBG, "InMAmpsAC=%ld IMeas.rms.val=%d", AMPS_MAMPS(amps), adc);
  #endif
    return(amps);
}

// ----------------------------------------------------------------------
float an_GetChgrILineCurrent(uint8_t saved)
{
    int16_t adc; 
    float amps;

    if (saved) adc = AnSaved.Status.ILine.rms.val;
    else       adc = ILineRMS();

    amps = ILINE_ADC_AMPS(adc);
    if (amps < 0.0) amps = 0.0;
  #ifdef DEBUG_AN_CONVERSIONS
    LOG(SS_DAC, SV_DBG, "ILineMAmps=%ld ILine.rms.val=%d", AMPS_MAMPS(amps), adc);
  #endif
    return(amps);
}

// ----------------------------------------------------------------------
float an_GetInvAcAmps(uint8_t saved)
{
    if (!IsInvActive()) return(0.0);
    return(an_GetInvIMeasCurrent(saved));
}

// ----------------------------------------------------------------------
// get ILimit.rms / IMeas.rms ratio which corresponds to ILimit pot adjustment
float an_GetILimitRatio()
{
    float ratio;
    int16_t iLimit = ILimitRMS();
    int16_t iMeas  = IMeasRMS();
    if (iMeas == 0)
    {
      #ifdef DEBUG_AN_CONVERSIONS
        LOG(SS_DAC, SV_DBG, "an_GetILimitRatio() is InValid: ILimit=%d IMeas=%d", iLimit, iMeas);
      #endif
        return(-1.0); // not valid
    }
    ratio = (float)iLimit/(float)iMeas;
  #ifdef DEBUG_AN_CONVERSIONS
    LOG(SS_DAC, SV_DBG, "an_GetILimitRatio()=%ld", ratio);
  #endif
    return(ratio);
}

// ----------------------------------------------------------------------
// float  version  500 msec / 10000 calls   50 usec
// uint32 version  356 msec / 10000 calls   35 usec
float an_GetACWatts(uint8_t saved)
{
    int16_t adc; 
    float   watts;

    if (saved) adc = AnSaved.AvgWACr.val;
    else       adc = WattsLongAvg();
    if (adc < 0) adc = -adc;    // possible?

    watts = WACR_ADC_WATTS(adc);
    if (watts < 0) watts = 0;
	if (watts < MINIMUM_WATTS) watts = 0; // clip to zero
  #ifdef DEBUG_AN_CONVERSIONS
    LOG(SS_DAC, SV_DBG, "WattsAC=%ld WACr.avg.val=%d", WATTS_MWATTS(watts), adc);
  #endif
    return(watts);
}
	
// <><><><><><><><><><><><><> analog.c <><><><><><><><><><><><><><><><><><><><><><>

