// <><><><><><><><><><><><><> inverter_cmds.c <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//  Copyright(C) 2015 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
//  Inverter commands used by remote communications
//  
//-----------------------------------------------------------------------------

// -------
// headers
// -------
#include "options.h"    // must be first include
#include "config.h"
#include "charger.h"
#include "charger_cmds.h"
#include "device.h"
#include "hw.h"
#include "inverter_cmds.h"
#include "inverter.h"
#include "nvm.h"
#include "rv_can.h"
#include "batt_temp.h"

// ----------------------------------------------------------------------
// log inverter status flags
void inv_LogStatusFlags()
{
    LOG(SS_INV, SV_INFO, "Status: %04Xh %s%s%s%s%s%s%s", (int16_t)Inv.status.all_flags, 
        IsInvActive()            ? " Act"   : "",
        IsInvOutputting()        ? " Out"   : "",
        IsInvLoadSenseMode()     ? " LdSns" : "",
        IsInvLoadPresent()       ? " Load"  : "",
        IsInvLowBattDetected()   ? " LoBatt": "",
        IsInvHighBattDetected()  ? " HiBatt": "",
        IsInvOverLoadDetected()  ? " OvLd"  : "");
}


// ----------------------------------------------------------------------
// log inverter error flags
void inv_LogErrorFlags()
{
    LOG(SS_INV, SV_INFO, "Errors: %04Xh %s%s%s%s%s%s", (int16_t)Inv.error.all_flags,
        IsInvLowBattShutdown()  ? " LoDwn"  : "",
        IsInvHighBattShutdown() ? " HiDwn"  : "",
        IsInvOverLoadShutdown() ? " OvDwn"  : "",
        IsInvFeedbackProblem()  ? " Fbk"    : "",
        IsInvFeedbackReversed() ? " FbkRev" : "",
        IsInvOutputShorted()    ? " Short"  : "");
}

// ----------------------------------------------------------------------
//               I N V E R T E R    C O M M A N D S
// ----------------------------------------------------------------------

// reboot the microprocessor
void inv_Reboot()
{
    LOG(SS_INVCMD, SV_INFO, "Reboot Inverter");
    CPU_Reboot();
}

// ----------------------------------------------------------------------
// clear fault states
void inv_ClearFaults()
{
    LOG(SS_INVCMD, SV_INFO, "Clear Faults");
    // TODO needs work
}

// ----------------------------------------------------------------------
// restore system to default values
// loads read-only configuration
void inv_RestoreToDefaults()
{
    LOG(SS_INVCMD, SV_INFO, "Restore To Faults");

    // load read-only configuration defaults
	nvm_LoadRomSettings();
	
    // request to save configuration
    nvm_ReqSaveConfig();
}

// ----------------------------------------------------------------------
//                    R V C   S T A T U S    M O D E
// ----------------------------------------------------------------------

// RVC Table 6.20.8b  pg 100
// returns: 0=disabled, 1=invert, 2=AC Passthru, 3=APS only, 4=waiting for load
uint8_t inv_GetRvcStatus()
{
    typedef enum 
    {
        DISABLED    = 0,
        INVERT      = 1,
        AC_PASSTHRU = 2,
        APS_ONLY    = 3,    //  APS - Auxiliary Power Supply
        LOAD_SENSE  = 4,    //  waiting to detect load
        NOT_ACTIVE  = 5,    //  waiting to invert
    } rvcan_STATUS_t;
    
    rvcan_STATUS_t  status = DISABLED;

    if (IsInvEnabled())
    {
        if (IsInvActive()) 
        {
            status = INVERT;
            
          #if IS_PCB_LPC
            if(IsInvLoadSenseMode())
            {
                //  The inverter is enabled and waiting to detect a load
                status = LOAD_SENSE;
            }
          #endif
        }
        else
        {
            status = NOT_ACTIVE;
        }
    }
    return((uint8_t)status);
}

// ----------------------------------------------------------------------
//        B A T T E R Y    T E M P E R A T U R E     S E N S O R
// ----------------------------------------------------------------------
// return: 	0=no battery temp sensor in use, 
//			1=sensor is present and active
uint8_t inv_GetBatteryTempSensorPresent()
{
	//	Always return 0 - Inverter does not use the battery temperature sensor.
	return(0);
}


// ----------------------------------------------------------------------
//        B A T T E R Y    V O L T A G E    T H R E S H O L D S 
// ----------------------------------------------------------------------

void inv_SetSupplyLowShutdown(uint16_t adc_count)
{
    LOG(SS_INVCMD, SV_INFO, "supply_low_shutdown = 0x%04X", adc_count);
    g_nvm.settings.inv.supply_low_shutdown = Inv.config.supply_low_shutdown = adc_count;
    nvm_SetDirty();
}

void inv_SetSupplyLowThreshold(uint16_t adc_count)
{
    LOG(SS_INVCMD, SV_INFO, "supply_low_thres = 0x%04X", adc_count);
    g_nvm.settings.inv.supply_low_thres  = Inv.config.supply_low_thres  = adc_count;
    g_nvm.settings.inv.supply_low_hyster = Inv.config.supply_low_hyster = adc_count + INV_DFLT_SUPPLY_LOW_DELTA;
    nvm_SetDirty();
}

void inv_SetSupplyLowRecover(uint16_t adc_count)
{
    LOG(SS_INVCMD, SV_INFO, "supply_low_recover = 0x%04X", adc_count);
    g_nvm.settings.inv.supply_low_recover = Inv.config.supply_low_recover = adc_count;
    nvm_SetDirty();
}

void inv_SetSupplyHighRecover(uint16_t adc_count)
{
    LOG(SS_INVCMD, SV_INFO, "supply_high_recover = 0x%04X", adc_count);
    g_nvm.settings.inv.supply_high_recover = Inv.config.supply_high_recover = adc_count;
    nvm_SetDirty();
}

void inv_SetSupplyHighThreshold(uint16_t adc_count)
{
    LOG(SS_INVCMD, SV_INFO, "supply_high_thres = 0x%04X", adc_count);
    g_nvm.settings.inv.supply_high_thres = Inv.config.supply_high_thres = adc_count;
    nvm_SetDirty();
}

void inv_SetSupplyHighShutdown(uint16_t adc_count)
{
    LOG(SS_INVCMD, SV_INFO, "supply_high_shutdown = 0x%04X", adc_count);
    g_nvm.settings.inv.supply_high_shutdown = Inv.config.supply_high_shutdown = adc_count;
    nvm_SetDirty();
}


// ----------------------------------------------------------------------
//                 I N V E R T E R   
// ----------------------------------------------------------------------

// enable/disable inverter on startup 
void inv_SetPGain(uint8_t * val)
{
    LOG(SS_INVCMD, SV_INFO, "Inverter P-Gain Q16: %X%X.%X%X", *val, *(val+1), *(val+2), *(val+3));
    memcpy((uint8_t *)&Inv.config.p_gain, val, 4);
	g_nvm.settings.inv.p_gain = Inv.config.p_gain;
    nvm_SetDirty();
}

void inv_SetIGain(uint8_t * val)
{
    LOG(SS_INVCMD, SV_INFO, "Inverter I-Gain Q16: %X%X.%X%X", *val, *(val+1), *(val+2), *(val+3));
    memcpy((uint8_t *)&Inv.config.i_gain, val, 4);
	g_nvm.settings.inv.i_gain = Inv.config.i_gain;
    nvm_SetDirty();
}

void inv_SetILimit(int16_t limit)
{
    LOG(SS_INVCMD, SV_INFO, "Inverter I-Limit: %d", limit);
    g_nvm.settings.inv.i_limit = Inv.config.i_limit = limit;
    nvm_SetDirty();
}

void inv_SetDGain(uint8_t * val)
{
    LOG(SS_INVCMD, SV_INFO, "Inverter D-Gain Q16: %X%X.%X%X", *val, *(val+1), *(val+2), *(val+3));
    memcpy((uint8_t *)&Inv.config.d_gain, val, 4);
	g_nvm.settings.inv.d_gain = Inv.config.d_gain;
    nvm_SetDirty();
}


// ----------------------------------------------------------------------
//                   L O A D  -  S E N S E
// ----------------------------------------------------------------------

void inv_SetLoadSenseEnabled(int enable)
{
    LOG(SS_INVCMD, SV_INFO, enable?"Inverter LoadSense ENABLED":"Inverter LoadSense DISABLED");
	Inv.status.load_sense_enabled = enable ? 1 : 0;
}

void inv_SetLoadSenseEnabledOnStartup(int enable) // 0=disable, 1=enable
{
    LOG(SS_INVCMD, SV_INFO, enable?"Inverter LoadSense ENABLED on startup":"Inverter LoadSense DISABLED on startup");
	g_nvm.settings.inv.load_sense_enabled = Inv.config.load_sense_enabled = enable ? 1 : 0;
    nvm_SetDirty();
}

void inv_SetLoadSenseDelay(uint16_t delay)
{
    LOG(SS_INVCMD, SV_INFO, "Inverter LoadSense Delay: %d", delay);
	g_nvm.settings.inv.load_sense_delay = Inv.config.load_sense_delay = delay;
	nvm_SetDirty();
}

void inv_SetLoadSenseInterval(uint16_t interval)
{
    LOG(SS_INVCMD, SV_INFO, "Inverter LoadSense Interval: %d", interval);
	g_nvm.settings.inv.load_sense_interval = Inv.config.load_sense_interval = interval;
	nvm_SetDirty();
}

void inv_SetLoadSenseThreshold(int16_t threshold)
{
    LOG(SS_INVCMD, SV_INFO, "Inverter LoadSense threshold: %d", threshold);
	g_nvm.settings.inv.load_sense_threshold = Inv.config.load_sense_threshold = threshold;
	nvm_SetDirty();
}


// ----------------------------------------------------------------------
//            D E V I C E  
// ----------------------------------------------------------------------

uint8_t dev_GetInverterEnabled(void)
{
    return(Device.config.inv_enabled ? 1 : 0);
}

void dev_SetInverterEnabled(int enable)
{
    LOG(SS_INVCMD, SV_INFO, enable?"Inverter ENABLED":"Inverter DISABLED");
	Device.status.inv_enabled = enable ? 1 : 0;
    Device.status.inv_disabled_source = 0;  // CAN
}

void dev_SetInverterEnabledOnStartup (int enable)
{
    LOG(SS_INVCMD, SV_INFO, enable?"Inverter ENABLED on startup":"Inverter DISABLED on startup");
	g_nvm.settings.dev.inv_enabled = Device.config.inv_enabled = enable ? 1 : 0;
    nvm_SetDirty();
}


void dev_SetPassThruEnabled(int enable)
{
    LOG(SS_INVCMD, SV_INFO, enable?"Pass-thru ENABLED":"Pass-thru DISABLED");
	Device.status.pass_thru_enabled = enable ? 1 : 0;
}

void dev_SetPassThruEnabledOnStartup (int enable)
{
    LOG(SS_INVCMD, SV_INFO, enable?"Pass-thru ENABLED on startup":"Pass-thru DISABLED on startup");
	g_nvm.settings.dev.pass_thru_enabled = Device.config.pass_thru_enabled = enable ? 1 : 0;
    nvm_SetDirty();
}


uint8_t dev_GetTimerShutdownEnabled(void)
{
    return(Device.config.tmr_shutdown_enabled ? 1 : 0);
}

void dev_SetTimerShutdownEnabled(int enable)
{
    LOG(SS_INVCMD, SV_INFO, enable?"Shutdown Timer ENABLED":"Shutdown Timer DISABLED");
	Device.status.tmr_shutdown_enabled = enable ? 1 : 0;
}

void dev_SetTimerShutdownEnabledOnStartup(int enable)
{
    LOG(SS_INVCMD, SV_INFO, enable?"Shutdown Timer ENABLED on startup":"Shutdown Timer DISABLED on startup");
	g_nvm.settings.dev.tmr_shutdown_enabled = Device.config.tmr_shutdown_enabled = enable ? 1 : 0;
    nvm_SetDirty();
}

//  NOTE: The Shutdown Timer uses 0.5-second resolution for consistency with 
//  other RV-C timer types. This IS NOT a requirement, since we are not using 
//  an RV-C command to set these values, but is intended for the convenience
//  and consistency.  The shutdown timer does not require 0.5-second resolution.

void dev_SetTimerShutdownDelay(uint16_t delay)
{
    LOG(SS_INVCMD, SV_INFO, "Shutdown Timer Delay: %d", (delay/2));
	g_nvm.settings.dev.tmr_shutdown_delay = 
        Device.config.tmr_shutdown_delay = 
        Device.status.tmr_shutdown_timer_sec = (delay/2);
	nvm_SetDirty();
}

uint16_t dev_GetTimerShutdownDelay(void)
{
    return(Device.config.tmr_shutdown_delay*2);
}

uint16_t dev_GetTimerShutdownTimer(void)
{
    return(Device.status.tmr_shutdown_timer_sec*2);
}



// ----------------------------------------------------------------------
//            A C     P A S S    T H R U    E N A B L E
// ----------------------------------------------------------------------

// enable/disable inverter pass thru
void inv_SetPassThruEnable(int enable)  // 0=disable, 1=enable
{
    LOG(SS_INVCMD, SV_INFO, enable?"Pass Thru ENABLED":"Pass Thru DISABLED");
    Device.status.pass_thru_enabled = enable ? 1 : 0;
}

// ----------------------------------------------------------------------
// enable/disable inverter pass thru
void inv_SetPassThruEnableOnStartup(int enable)  // 0=disable, 1=enable
{
    LOG(SS_INVCMD, SV_INFO, enable?"Pass Thru ENABLED on startup":"Pass Thru DISABLED on startup");
    g_nvm.settings.dev.pass_thru_enabled = Device.config.pass_thru_enabled = enable ? 1 : 0;
    nvm_SetDirty();
}

// ----------------------------------------------------------------------
//             I N V E R T E R    S T A T U S
// ----------------------------------------------------------------------

// get inverter status
//	ref: RVC Table 6.20.8b

void inv_GetStatus(RVCS_INV_STATUS* status)
{
    status->status            = inv_GetRvcStatus();   
    status->tempSensorPresent = inv_GetBatteryTempSensorPresent();   
	status->loadSenseEnabled  = IsInvLoadSenseEn()?1:0;
}

// ----------------------------------------------------------------------
// get instance status
void inv_GetInstStatus(RVCS_INSTANCE_STATUS* status)
{
    // RVC 6.2.4 pg 36
    // TODO set these per spec
    status->deviceType   = 0;   
    status->baseInstance = 0;
    status->maxInstance  = 0;  
    status->baseIntAddr  = 0;  
    status->maxIntAddr   = 0;   
}

// ----------------------------------------------------------------------
//             I N V E R T E R    D C    S T A T U S
// ----------------------------------------------------------------------

// get inverter dc status
void inv_GetDcStatus(RVCS_INV_DC_STATUS* status)
{
    // RVC 6.20.18 pg 106
    status->dcVoltage  = VOLTS_TO_RVC16(an_GetBatteryVoltage(0));
    status->dcAmperage =  AMPS_TO_RVC16(an_GetInvBatteryCurrent(0));
}

// ----------------------------------------------------------------------
//             I N V E R T E R    A C    S T A T U S
// ----------------------------------------------------------------------

// get inverter ac status 1
void inv_GetAcStatus1(RVCS_AC_PNT_PG1* status)
{
    // RVC 6.20.3 pg 98
    status->RMS_Volts  = VOLTS_TO_RVC16(an_GetInvAcVoltage(0));
    status->RMS_Amps   =  AMPS_TO_RVC16(an_GetInvAcAmps(0));
    status->frequency  = 60;
    status->faultOpenGnd     = 0;
    status->faultOpenNeutral = 0;
    status->faultRevPolarity = 0;
    status->faultGndCurrent  = 0;
}

// ----------------------------------------------------------------------
// get inverter ac status 2
void inv_GetAcStatus2(RVCS_AC_PNT_PG2* status)
{
    // RVC 6.20.4 pg 98
    // TODO set these per spec
    status->peakVoltage   = 0;    
    status->peakCurrent   = 0;            
    status->groundCurrent = 0;            
    status->capacity      = 0;  
}

// ----------------------------------------------------------------------
// get inverter ac status 3
void inv_GetAcStatus3(RVCS_AC_PNT_PG3* status)
{
    // RVC 6.20.5 pg 99
    // TODO set these per spec
    status->waveformPhaseBits  = 0;   
    status->realPower          = WATTS_TO_RVC16(an_GetACWatts(0));    
    status->reactivePower      = 0;
    status->harmonicDistortion = 0; 
    status->complementaryLeg   = 0;      
}

// ----------------------------------------------------------------------
// get inverter ac status 4
void inv_GetAcStatus4(RVCS_AC_PNT_PG4* status)
{
    // RVC 6.20.6 pg 99
    // TODO set these per spec
    status->voltageFault      = 0;   
    status->faultSurgeProtect = 0;
    status->faultHighFreq     = 0; 
    status->faultLowFreq      = 0;      
    status->faultBypassActive = 0;      
}

// ----------------------------------------------------------------------
//             I N V E R T E R    A C    F A U L T    S T A T U S
// ----------------------------------------------------------------------

// get inverter ac fault status 1
void inv_GetAcFaultStatus1(RVCS_AC_FAULT_STATUS_1* status)
{
    // RVC 6.1.6  pg 30 
    // TODO set these per spec
    status->extremeLowVoltage    = 0;   
    status->lowVoltage           = 0;           
    status->highVoltage          = 0;       
    status->extremeHighVoltage   = 0;   
    status->equalizationSeconds  = 0;
    status->bypassMode           = 0;
}

// ----------------------------------------------------------------------
// get inverter ac fault status 2
void inv_GetAcFaultStatus2(RVCS_AC_FAULT_STATUS_2* status)
{
    // RVC 6.1.7  pg 31 
    // TODO set these per spec
    status->loFreqLimit = 0;    
    status->hiFreqLimit = 0;            
}

// ----------------------------------------------------------------------
//             I N V E R T E R    D C    S T A T U S
// ----------------------------------------------------------------------

// get inverter dc source status 1
//  called from:    rvcan_SendDcSourceStatus1()
//      ref:    RVC 6.5.2  pg 40
//      Name:   DC_SOURCE_STATUS_1
//      DGN:    0x1FFFD
void inv_GetDcSourceStatus1(RVCS_DC_SOURCE_STATUS_1* status)
{
    status->instance       = 1;     //  Main House Battery Bank 
    status->devicePriority = 100;   //  Inverter/Charger
    status->dcVoltage      = VOLTS_TO_RVC16(an_GetBatteryVoltage(0));
    status->dcCurrent      =  AMPS_TO_RVC32(an_GetInvBatteryCurrent(0));       
}

// ----------------------------------------------------------------------
// get inverter dc source status 2
void inv_GetDcSourceStatus2(RVCS_DC_SOURCE_STATUS_2* status)
{
    // RVC 6.5.3 pg 41
    // TODO set these per spec
  #if defined(OPTION_HAS_CHGR_EN_SWITCH)                                    
    status->temperature   = RVC_NOT_SUPPORTED_16BIT;
  #else
    status->temperature   = CELCIUS_TO_RVC16(BattTemp_celsius());    
  #endif
    status->stateOfCharge = RVC_NOT_SUPPORTED_8BIT;
    status->timeRemaining = RVC_NOT_SUPPORTED_16BIT;
}

// ----------------------------------------------------------------------
// get inverter dc source status 3
void inv_GetDcSourceStatus3(RVCS_DC_SOURCE_STATUS_3* status)
{
    // RVC 6.5.4 pg 42
    // TODO set these per spec
    status->devicePriority    = 0;   
    status->stateOfHealth     = 0;    
    status->capacityRemaining = 0;
    status->relativeCapacity  = 0; 
    status->acRmsRipple       = 0;      
}

// ----------------------------------------------------------------------
//       I N V E R T E R    C O N F I G    S T A T U S
// ----------------------------------------------------------------------

// get inverter config status 1
void inv_GetConfigStatus1(RVCS_INV_CFG_STATUS1* status)
{
    // RVC 6.20.10 pg 101
    // TODO set these per spec
    status->loadSensePowerThreshold = 0;   
    status->loadSenseInterval       = 0;
    status->minDcVoltShutdown       = 0;       
    status->enableFlags = (g_nvm.settings.dev.inv_enabled     ? 0x01:0) |
                          (g_nvm.settings.inv.load_sense_enabled ? 0x04:0) |
                          (g_nvm.settings.dev.pass_thru_enabled ? 0x10:0) ;
}

// ----------------------------------------------------------------------
// get inverter config status 2
void inv_GetConfigStatus2(RVCS_INV_CFG_STATUS2* status)
{
    // RVC 6.20.11 pg 102
    // TODO set these per spec
    status->maxDcVoltShutdown = 0;    
    status->minDcVoltWarn     = 0;            
    status->maxDcVoltWarn     = 0;            
}

// ----------------------------------------------------------------------
//                  I N V E R T E R    D I A G N O S T I C S
// ----------------------------------------------------------------------

// ----------------------------------------------------------------------
//  RVC 3.2.5.8 Failure mode identifier
// ----------------------------------------------------------------------
//  Val Description
//   0  Datum value above normal range
//   1  Datum value below normal range
//   2  Datum value erratic or invalid
//   3  Short circuit to high voltage (or complete sensor input failure)
//   4  Short circuit to low voltage (or complete sensor input failure)
//   5  Open circuit, or output current below normal
//   6  Grounded circuit, or output current above normal
//   7  Mechanical device not responding
//   8  Datum value showing error of frequency, pulse width, or period
//   9  Datum not updating at proper rate
//  10  Datum value fluctuating at abnormal rate
//  11  Failure not identifiable
//  12  Bad intelligent RV-C node
//  13  Calibration required
//  14  "None of the above" (use sparingly!)
//  15  Datum valid but above normal operational range (least severe)
//  16  Datum valid but above normal operational range (moderately severe)
//  17  Datum valid but below normal operational range (least severe)
//  18  Datum valid but below normal operational range (moderately severe)
//  19  Received invalid network datum
//  20  to 30 reserved
//  31  Failure mode not available
//
// ----------------------------------------------------------------------
//  3.2.5.3  Red, Yellow Fault Indicators
// ----------------------------------------------------------------------
//    The classification of faults into “Yellow” and “Red” is subjective.
//    In general, “Yellow” faults usually refer to conditions that are
//    manageable by the user or require little or no intervention. 
//    “Red” faults generally require a service technician or other substantial intervention.
//    It is possible for both a Yellow and a Red condition to be active at the same time,
//    if multiple faults are occurring simultaneously.
//
//    EXAMPLE - A low battery level to an inverter signal is a 'Yellow' fault.
// ----------------------------------------------------------------------

// get inverter diagnostics state
void inv_GetDM_RV(RVCS_DM_RV* status)
{
    uint8_t  MSB = 0xFF;
    uint8_t  LSB = 0xFF;
    uint8_t  FMI = 0xFF;  // Failure Mode Indicator

    // determine fault code if any                                                                         // RVC Service Point
    if (Is15VoltBad() || IsInvLowBattShutdown() || IsChgrLowBattShutdown())    { MSB=1;    LSB=0; FMI=1; } // DC Voltage; below normal
    else if (IsInvHighBattShutdown() || IsChgrHighBattShutdown())              { MSB=1;    LSB=0; FMI=0; } // DC Voltage; above normal
    else if (IsInvOverLoadShutdown() || IsChgrOverCurrShutdown())              { MSB=1;    LSB=1; FMI=0; } // DC Current; above normal
    else if (IsBattOverTemp())                                                 { MSB=1;    LSB=2; FMI=0; } // Battery Temperature; over temp
    else if (IsBattTempSnsOpen())                                              { MSB=1;    LSB=2; FMI=7; } // Battery Temperature; not responding
    else if (IsBattTempSnsShort())                                             { MSB=1;    LSB=2; FMI=4; } // Battery Temperature; short circuit
    else if (IsInvOutputShorted())                                             { MSB=1;    LSB=7; FMI=4; } // AC Feedback; shorted
    else if (IsXfrmOverTemp())                                                 { MSB=3;    LSB=1; FMI=0; } // FET #1 Temperature; above normal
    else if (IsHeatSinkOverTemp())                                             { MSB=2;    LSB=0; FMI=0; } // Transformer Temperature; above normal
    else if (IsChgrCcTimeoutErr())                                             { MSB=3;    LSB=3; FMI=0; } // Battery Charger Timeout
    else if (IsInvFeedbackReversed() || IsInvFeedbackProblem())                { MSB=0x81; LSB=5; FMI=0; } // Reversed Polarity
    else if (IsInvOutputShorted())                                             { MSB=0x81; LSB=6; FMI=4; } // Ground Fault; shorted
    else if (IsNvmBad())                                                       { MSB=0;    LSB=4; FMI=0; } // Node RAM 

    // RVC 3.2.5.1 pg 12
    status->opStatus1    = 0x01;
    status->opStatus2    = (IsInvActive() || IsChgrActive()) ? 1 : 0;
    status->yellowStatus = (HasDevAnyErrors() || HasInvAnyErrors() || HasChgrAnyErrors()) ? 1 : 0;
    status->redStatus    = 0; // no red errors
    status->DSA          = J1939_InvAddress();   
    status->SPNmsb       = MSB;   
    status->SPNisb       = (MSB==0xFF && LSB==0xFF) ? 0xFF : g_can.MyInstance;   
    status->SPNlsb       = LSB;   
    status->FMI          = FMI;   
    status->occurance    = 0x7F;
    status->reserved     = 0x1;
    status->DSAextension = 0xFF;
    status->bankSelect   = 0xF;
}

// <><><><><><><><><><><><><> inverter_cmds.c <><><><><><><><><><><><><><><><><><><><><><>
