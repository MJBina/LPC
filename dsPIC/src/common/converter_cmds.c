// <><><><><><><><><><><><><> converter_cmds.c <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//	Copyright(C) 2015 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
//  DC+DC Converter remote interface commands
//
//-----------------------------------------------------------------------------

// -------
// headers
// -------
#include "options.h"    // must be first include
#include "analog.h"
#include "converter_cmds.h"
#include "nvm.h"

#if !IS_DEV_CONVERTER

 // dummy stubs if not a converter
 void  cnv_SetDcSetPoint(float volts) { }
 float cnv_GetDcSetPoint() { return (0.0); }
 float cnv_GetDcOutVolts(uint8_t saved) { return (0.0); }

#else

// ----------------------------------------------------------------------
//                  D C + D C    C O N V E R T E R
// ----------------------------------------------------------------------

// set DC+DC desired output voltage (ie setpoint)
void cnv_SetDcSetPoint(float volts)
{
    Cnv.dc_set_point_adc = ClipADC10(DCDC_VOLTS_ADC(volts));
    g_nvm.settings.conv.dc_set_point_adc = Cnv.dc_set_point_adc;
    nvm_SetDirty();
    LOG(SS_SEN, SV_INFO, "DC_SetPoint %.3fv adc=%u", volts, g_nvm.settings.conv.dc_set_point_adc);
}                              

// ----------------------------------------------------------------------
// get DC+DC desired output voltage (ie setpoint) in volts
float cnv_GetDcSetPoint()
{
    float volts = DCDC_ADC_VOLTS(Cnv.dc_set_point_adc);
    return(volts);
}

// ----------------------------------------------------------------------
// get DC+DC current average output voltage in volts
float cnv_GetDcOutVolts(uint8_t saved)
{
    int16_t adc; 
    float volts;

    if (saved) adc = AnSaved.Status.DcOutput.avg.val;
    else       adc =      An.Status.DcOutput.avg.val;

    volts = DCDC_ADC_VOLTS(adc);
    return(volts);
}

#endif // IS_DEV_CONVERTER

// <><><><><><><><><><><><><> converter_cmds.c <><><><><><><><><><><><><><><><><><><><><><>
