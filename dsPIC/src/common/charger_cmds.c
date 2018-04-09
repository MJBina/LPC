// <><><><><><><><><><><><><> charger_cmds.c <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//	Copyright(C) 2015 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
//  Charger remote command interface
//	
//-----------------------------------------------------------------------------

// -------
// headers
// -------
#include "options.h"    // must be first include
#include "config.h"
#include "charger.h"
#include "charger_cmds.h"
#include "inverter_cmds.h"
#include "rv_can.h"
#include "nvm.h"
#include "tasker.h"

// -------------------------
// Conditional compile flags
// -------------------------

// comment out flag to not use it
//#define  DEBUG_SHOW_CHGR_STATE   1
#define  DEBUG_SHOW_CHGR_CMDS   1

// -----------------------------
// Auto Removal for build types
// -----------------------------
#if OPTION_NO_CONDITIONAL_DBG
  #undef DEBUG_SHOW_CHGR_STATE
  #undef DEBUG_SHOW_CHGR_CMDS
#endif	


// ----------------------------------------------------------------------
// set charger current limit (max IMeas A/C amps allowed for charging)
// amps is max IMeas A/C Amps
void chgr_SetAmpsLimit(float acAmps)
{
    // convert IMEAS amps to ILIMIT adc counts (at pot mid-point)
	g_nvm.settings.chgr.amps_limit = Chgr.config.amps_limit = ILIMIT_AMPS_ADC(acAmps, ILIMIT_POT_MID);        
    nvm_SetDirty();

   #ifdef DEBUG_SHOW_CHGR_CMDS
    {
        float dcAmps = IDC_CHG_ADC_AMPS(VBattCycleAvg(), ChgrCfgMaxAcCurrA2D());
        float vbatt  =  VBATT_ADC_VOLTS(VBattCycleAvg());
      	LOG(SS_CHGCMD, SV_INFO, "Set Chgr Limit=%.1fAAC (%u A/D), %.1fADC, VBatt=%.1f volts",
            acAmps, ChgrCfgMaxAcCurrA2D(), dcAmps, vbatt);
    }
   #endif
}

// ----------------------------------------------------------------------
// get charger current limit (max IMeas A/C amps)
// returns max IMeas A/C Amps in effect for charging
float chgr_GetAmpsLimit()
{
    // convert ILIMIT A/D counts to IMEAS amps (at pot mid-point)
    float acAmps = ILIMIT_ADC_AMPS(ChgrCfgMaxAcCurrA2D(), ILIMIT_POT_MID);        

    /*{
        float dcAmps = IDC_CHG_ADC_AMPS(VBattCycleAvg(), ChgrCfgMaxAcCurrA2D());
        float vbatt  =  VBATT_ADC_VOLTS(VBattCycleAvg());
      	LOG(SS_CHGCMD, SV_INFO, "Get Chgr Limit=%.1fAAC (%u A/D), %.1fADC, VBatt=%.1f volts",
            acAmps, ChgrCfgMaxAcCurrA2D(), dcAmps, vbatt);
    }*/
    return(acAmps);
}

// ----------------------------------------------------------------------
// set branch circuit rating (max ac amp draw)
void chgr_SetBranchCircuitAmps(int16_t amps)
{
	//	if the value has not changed then return;
	if (amps == ChgrCfgBcrAmps()) return;

    // clip if over max
    if (amps > MAX_BRANCH_CIRCUIT_AMPS)
    {
      #ifdef DEBUG_SHOW_CHGR_CMDS
    	LOG(SS_CHGCMD, SV_INFO, "SetBCR=%uA Max=%u", amps, MAX_BRANCH_CIRCUIT_AMPS);
      #endif
        amps = MAX_BRANCH_CIRCUIT_AMPS;
    }
    g_nvm.settings.chgr.bcr_int8 = Chgr.config.bcr_int8 = amps;
	g_nvm.settings.chgr.bcr_adc  = Chgr.config.bcr_adc	= ILINE_AMPS_ADC((int16_t)((float)amps)) - BCR_SAFETY_A2D;
     
    nvm_SetDirty();
  #ifdef DEBUG_SHOW_CHGR_CMDS
  	LOG(SS_CHGCMD, SV_INFO, "SetBCR=%uA", amps);
  #endif
}

// ----------------------------------------------------------------------
// 	Request Equalization
void chgr_StartEqualization()
{
	g_nvm.settings.chgr.eq_request = Chgr.config.eq_request = 1;
    nvm_SetDirty();
}


// -----------------------------------------------------------------------------
//	chgr_GetRvcState
// -----------------------------------------------------------------------------
//
//	Name:	CHARGER_STATUS
//	DGN:	1FFC7h
//	ref:    RV-C, December 17 2015, table 6.21.8B p122, "Operating state"
//	ref:    RV-C, December 17 2015, table 6.5.5B p43, "Desired charge state"
//
//  0 = Not Charging  (RVC: Undefined, charging source decides (Default)
//  1 = Monitoring    (RVC: Do not charge)
//  2 = Bulk
//  3 = Absorption
//  4 = Overcharge
//  5 = Equalize
//  6 = Float
//  7 = Constant Voltage/Current
//
// -----------------------------------------------------------------------------

uint8_t chgr_GetRvcState(void)
{
    CHARGER_STATE_t initChgrState;
    uint8_t rc = 0;
    
    if(!IsChgrEnabled())
    {
      #ifdef DEBUG_SHOW_CHGR_STATE
        LOG(SS_CHGCMD, SV_INFO, "State: Disabled");
      #endif
        return((uint8_t)0);  // not charging
    }

    if(!IsChgrActive())
    {
      #ifdef DEBUG_SHOW_CHGR_STATE
        LOG(SS_CHGCMD, SV_INFO, "State: Not Active");
      #endif
        return((uint8_t)0);  // not charging
    }

    switch(ChgrState())
    {
    default :
    case CS_INVALID: 
    case CS_INITIAL:
        rc = 0; // not charging
      #ifdef DEBUG_SHOW_CHGR_STATE
        LOG(SS_CHGCMD, SV_INFO, "State: Not Active");
      #endif
        break;

    case CS_SOFT_START:
        rc = 7;     //  no good choice for this
      #ifdef DEBUG_SHOW_CHGR_STATE
        LOG(SS_CHGCMD, SV_INFO, "State: Soft Start");
      #endif
        break;

    case CS_CONST_CURR :      
        rc = 2;     //  Bulk
      #ifdef DEBUG_SHOW_CHGR_STATE
        LOG(SS_CHGCMD, SV_INFO, "State: Bulk Charging");
      #endif
        break;
        
    case CS_CONST_VOLT :    
        rc = 3;     //  Absorption/Accept
      #ifdef DEBUG_SHOW_CHGR_STATE
        LOG(SS_CHGCMD, SV_INFO, "State: Absorption/Accept");
      #endif
        break;
    
    case CS_WARM_BATT :   // RVC does not have a warm batt state; use state we came from
        initChgrState = chgr_InitChargerState();
        switch (initChgrState)
        {
        default:
        case CS_FLOAT:      rc = 6; break; 
        case CS_CONST_VOLT: rc = 3; break; 
        case CS_EQUALIZE:   rc = 5; break; 
        case CS_MONITOR:    rc = 1; break; 
        }
        break;

    case CS_FLOAT :
        rc = 6;     //  Float
      #ifdef DEBUG_SHOW_CHGR_STATE
        LOG(SS_CHGCMD, SV_INFO, "State: Float");
      #endif
        break;
        
    case CS_SOFT_STOP:
    case CS_MONITOR:
        rc = 1;     //  Monitoring
      #ifdef DEBUG_SHOW_CHGR_STATE
        LOG(SS_CHGCMD, SV_INFO, "State: Monitoring");
      #endif
        break;

    case CS_SHUTDOWN :
        rc = 1;     //  Monitoring
      #ifdef DEBUG_SHOW_CHGR_STATE
        LOG(SS_CHGCMD, SV_INFO, "State: Shutdown");
      #endif
        break;
        
    case CS_LOAD_MGMT :
        rc = 7;     //  Load management
      #ifdef DEBUG_SHOW_CHGR_STATE
        LOG(SS_CHGCMD, SV_INFO, "State: Load Management");
      #endif
        break;
		
    case CS_EQUALIZE :
        rc = 5;     //  Load management
      #ifdef DEBUG_SHOW_CHGR_STATE
        LOG(SS_CHGCMD, SV_INFO, "State: Equalization");
      #endif
        break;
		
    } // switch
    return(rc);
}

// ----------------------------------------------------------------------
// get charger status info
void chgr_GetStatusInfo(RVCS_CHARGER_STATUS* status)
{
    // RVC 6.21.8 pg 109
    status->chargeVoltage  = VOLTS_TO_RVC16(an_GetBatteryVoltage(0));
    status->chargeCurrent  =  AMPS_TO_RVC16(an_GetChgrBatteryCurrent(0));
    status->chargePercent  = RVC_NOT_SUPPORTED_8BIT;
    status->opState        = chgr_GetRvcState();
    status->defaultPowerUp = Device.config.chgr_enabled;
}

// ----------------------------------------------------------------------
// returns CHGR_ALGORITHM_nnn
//	ref:	RVC Table 6.21.9b - Charger Algorithm
//	The algorithm currently being applied to the battery.
//		0 - Constant voltage
//		1 - Constant current
//		2 - 3-Stage
//		3 - 2-Stage
//		4 - Trickle
//		249 - Custom algorithm #2
//		250 - Custom algorithm #1
//
//	[2017-05-12] At present, we have implemented only the 3-step and a custom 
//	(Li-Ion) 2-step algorithm for Volta.  The later will be indicated as a type
//	'3' so that the 'Custom' types will remain available.
// ----------------------------------------------------------------------

uint8_t  chgr_GetChargingAlgorithm(void)
{
  #if !defined(OPTION_HAS_CHARGER)
	return((uint8_t)0xFF);	// undefined
  #elif defined(OPTION_CHARGE_LION)
    return((uint8_t)2);		//	Li-Ion 2-Step Charging Algorithm`
  #else  
	return((uint8_t)3);		//	Standard 3-step algorithm
  #endif
}


// ----------------------------------------------------------------------
//             C H A R G E R    C O N F I G    S T A T U S
// ----------------------------------------------------------------------

void chgr_SetBattType(uint16_t batteryType, uint8_t forced)
{
    if (!forced && (batteryType == Chgr.config.battery_recipe.batt_type)) return;  // dont reset if no change
	switch(batteryType)
	{
    default: return;
	case BATT_WET:
		g_nvm.settings.chgr.battery_recipe = Chgr.config.battery_recipe = ChgrConfigBattRecipe_Wet;	
      #ifdef DEBUG_SHOW_CHGR_CMDS
        LOG(SS_CHGCMD, SV_INFO, "Battery Type WET");
      #endif
		break;
		
    case BATT_GEL:
		g_nvm.settings.chgr.battery_recipe = Chgr.config.battery_recipe = ChgrConfigBattRecipe_Gel;	
      #ifdef DEBUG_SHOW_CHGR_CMDS
        LOG(SS_CHGCMD, SV_INFO, "Battery Type GEL");
      #endif
		break;		
		
    case BATT_AGM:
		g_nvm.settings.chgr.battery_recipe = Chgr.config.battery_recipe = ChgrConfigBattRecipe_Agm;	
      #ifdef DEBUG_SHOW_CHGR_CMDS
        LOG(SS_CHGCMD, SV_INFO, "Battery Type AGM");
      #endif
		break;
		
	case BATT_LI: // Volta Lithium Ion
		g_nvm.settings.chgr.battery_recipe = Chgr.config.battery_recipe = ChgrConfigBattRecipe_LI;	
      #ifdef DEBUG_SHOW_CHGR_CMDS
        LOG(SS_CHGCMD, SV_INFO, "Battery Type LION");
      #endif
		break;
    } // switch
	nvm_SetDirty();
}

// ----------------------------------------------------------------------
// set custom battery recipe flag  0 or 1
void chgr_SetBattCustomFlag(uint16_t isCustom)
{
    if (isCustom)
    {
        // setting to custom
        if (Chgr.config.battery_recipe.is_custom) return; // no change
        // was standard
        g_nvm.settings.chgr.battery_recipe.is_custom = 1;
        Chgr.config.battery_recipe.is_custom         = 1;
        nvm_SetDirty();
      #ifdef DEBUG_SHOW_CHGR_CMDS
        LOG(SS_CHGCMD, SV_INFO, "Custom Battery Recipe");
      #endif
    }
    else
    {
        // setting to standard
        if (Chgr.config.battery_recipe.is_custom)
        {
            // was custom; change back to standard
            // need to reload battery fields with default values
            chgr_SetBattType(Chgr.config.battery_recipe.batt_type, 1); // force reload of values
        }
    }
}

// ----------------------------------------------------------------------
// return pointer to battery recipe stored in read-only-memory for given battery type
// returns NULL if invalid battery type

BATTERY_RECIPE_t* PtrRomBattRecipe(BATT_TYPE_t batteryType)
{
    BATTERY_RECIPE_t* pRomRecipe = NULL;
	switch(batteryType)
	{
	case BATT_WET: pRomRecipe = (BATTERY_RECIPE_t*)&ChgrConfigBattRecipe_Wet; break;
    case BATT_GEL: pRomRecipe = (BATTERY_RECIPE_t*)&ChgrConfigBattRecipe_Gel; break;		
    case BATT_AGM: pRomRecipe = (BATTERY_RECIPE_t*)&ChgrConfigBattRecipe_Agm; break;
	case BATT_LI:  pRomRecipe = (BATTERY_RECIPE_t*)&ChgrConfigBattRecipe_LI;  break;	
    } // switch
    return(pRomRecipe);
}

// --------------------
// Charger Config State
// --------------------

void chgr_SetCfgCmd(uint8_t  chargingAlgorithm,    // 
                    uint8_t  chargerMode,          // 
                    uint8_t  batterySensePresent,  // 0=no, 1=yes
                    uint8_t  installationLine,     // 0=line1, 1=line2
                    uint16_t batteryBankSize,      // amp-hours; resolution 1 amp-hour
                    uint16_t batteryType,          // 
                    uint8_t  maxChargingCurrent)   // amps
{
	//	Name: 	CHARGER_CONFIGURATION_COMMAND
	//	DGN:	0x1FFC4
	// 	Ref:	RVC 6.21.11

// Changing via RVC command can be dangerous to our systems (use Sensata Fields for this)
//
//	g_nvm.settings.dev.batt_temp_sense_present = 
//	     Device.config.batt_temp_sense_present = batterySensePresent;

//  #if !IS_DEV_CONVERTER
//	chgr_SetBattType(batteryType, 0);
//  #endif
  
//	nvm_SetDirty();
}

// ----------------------------------------------------------------------
// [Get] CHARGER CONFIGURATION STATUS
//	ref:    RVC 6.21.11 pg 124
//  Name:   CHARGER_CONFIGURATION_STATUS
//  DGN:    0x1FFC4

void chgr_GetConfigStatus(RVCS_CHARGER_CFG_STATUS* status)
{
    // RVC 6.21.9 pg 110
    status->chargingAlgorithm  = chgr_GetChargingAlgorithm();
    status->chargerMode        = 0; // stand-alone
    status->batteryTempSensor  = Device.config.batt_temp_sense_present;
    status->chargerInstallLine = 0; // line-1
    status->batteryType        = Chgr.config.battery_recipe.batt_type;	//	(4-bit)
    status->batteryBankSize    = RVC_NOT_SUPPORTED_16BIT;
    status->maxChargeCurrent   = AMPS_TO_RVC16( IDC_CHG_MAX_AMPS );
}

// ----------------------
// Charger Config 2 State
// ----------------------

void chgr_SetCfgCmd2(uint8_t chargePercent, uint8_t chargeRatePercent, uint8_t breakerSize)
{
    if (RVC_DONT_CHANGE_8BIT != breakerSize) chgr_SetBranchCircuitAmps((int16_t)breakerSize);
}

// ----------------------------------------------------------------------
// get charger config status2
void chgr_GetConfigStatus2(RVCS_CHARGER_CFG_STATUS2* status)
{
	//  Name:   CHARGER_CONFIGURATION_STATUS_2
	//  DGN:    0x1FF96
	//	ref:    RV-C, December 17 2015, 6.21.12a pg 125  
    status->maxChargeCurrPercent = RVC_NOT_SUPPORTED_8BIT;
    status->chargeRateLimit      = RVC_NOT_SUPPORTED_8BIT;
    status->shoreBreakerSize     = ChgrCfgBcrAmps();	//	<-- RV-C uint8 type
    status->defaultBattTemp      = CELCIUS_TO_RVC8(25);
    status->rechargeVoltage      = RVC_NOT_SUPPORTED_16BIT;;
}

// ----------------------------------------------------------------------
//       C H A R G E R    E Q U A L I Z A T I O N   S T A T U S
// ----------------------------------------------------------------------
//
//	NOTES: 
//	The RV-C specification identifies the second byte of this message includes
//	a 'Bit' data type (typically this means 'Boolean') indicating pre-charge 
// 	status. A 'Bit' type is actually a 2-bit field accommodating a value of 0x03 
//	meaning 'no-change' when used for configuration.
//
//	The current version (December 17,2015) of the RV-C specification leaves 
//	bits 2-7 unused. The current implementation of the charger includes a 2-bit
//	value providing more detail regarding equalization status. This is included
//	in in place of the 'Bit' type in this status message.
// ----------------------------------------------------------------------

// get charger equalization status
void chgr_GetEqualStatus(RVCS_CHARGER_EQUAL_STATUS* status)
{
    // Ref:		RVC 6.21.18,  rev December 17,2015
	// Name:	CHARGER_EQUALIZATION_STATUS
	// DGN:		1FF99h
    status->timeRemaining     = Chgr.status.eq_timer_minutes;
    status->preChargingStatus = Chgr.status.eq_status;
	if(IsChgrCfgEqRequest()) status->preChargingStatus |= 0x10;
}

// ----------------------------------------------------------------------
//             C H A R G E R    A C    S T A T U S
// ----------------------------------------------------------------------

// get inverter ac status 1
void chgr_GetAcStatus1(RVCS_AC_PNT_PG1* status)
{
	//	ref:    RVC 6.21.3 pg 107
	//  Name:   CHARGER_AC_STATUS_1
	//  DGN:    0x1FFCA
    status->RMS_Volts  = VOLTS_TO_RVC16(an_GetChgrAcVoltage(0));
    status->RMS_Amps   =  AMPS_TO_RVC16(an_GetChgrIMeasCurrent(0));
    status->frequency  =  FREQ_TO_RVC16(60);
    status->faultOpenGnd     = 0;
    status->faultOpenNeutral = 0;
    status->faultRevPolarity = 0;
    status->faultGndCurrent  = 0;
}

// ----------------------------------------------------------------------
// get inverter ac status 2
void chgr_GetAcStatus2(RVCS_AC_PNT_PG2* status)
{
    // RVC 6.21.4 pg 108
    // TODO:  set these per spec
    status->peakVoltage   = 0;    
    status->peakCurrent   = 0;            
    status->groundCurrent = 0;            
    status->capacity      = ChgrCfgBcrAmps();
}

// ----------------------------------------------------------------------
// get inverter ac status 3
void chgr_GetAcStatus3(RVCS_AC_PNT_PG3* status)
{
    // RVC 6.21.5 pg 108
    // TODO:  set these per spec
    status->waveformPhaseBits  = 0;   
    status->realPower          = 0;    
    status->reactivePower      = 0;
    status->harmonicDistortion = 0; 
    status->complementaryLeg   = 0;      
}

// ----------------------------------------------------------------------
// get inverter ac status 4
void chgr_GetAcStatus4(RVCS_AC_PNT_PG4* status)
{
    // RVC 6.21.6 pg 108
    // TODO:  set these per spec
    status->voltageFault	  = 0;   
    status->faultSurgeProtect = 0;
    status->faultHighFreq     = 0; 
    status->faultLowFreq      = 0;      
    status->faultBypassActive = 0;      
}

// ----------------------------------------------------------------------
//             C H A R G E R    A C    F A U L T    S T A T U S
// ----------------------------------------------------------------------

// get charger ac fault status 1
void chgr_GetAcFaultStatus1(RVCS_AC_FAULT_STATUS_1* status)
{
    // TODO:  set these per spec
	status->extremeLowVoltage    = 0;	
	status->lowVoltage           = 0;		    
	status->highVoltage          = 0;		
	status->extremeHighVoltage   = 0;	
	status->equalizationSeconds  = 0;
	status->bypassMode           = 0;
}

// ----------------------------------------------------------------------
// get charger ac fault status 2
void chgr_GetAcFaultStatus2(RVCS_AC_FAULT_STATUS_2* status)
{
    // TODO:  set these per spec
	status->loFreqLimit = 0;	
	status->hiFreqLimit = 0;		    
}

// <><><><><><><><><><><><><> charger_cmds.c <><><><><><><><><><><><><><><><><><><><><><>
