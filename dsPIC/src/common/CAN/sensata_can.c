// <><><><><><><><><><><><><> sensata_can.c <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//	Copyright(C) 2015 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
//	RV-C Sensata CAN bus proprietary commands
//
// -----------------------------------------------------------------------------------------------

// -------
// headers
// -------
#include "options.h"    // must be first include
#include "analog.h"
#include "charger.h"
#include "charger_cmds.h"
#include "config.h"
#include "converter_cmds.h"
#include "dsPIC33_CAN.h"
#include "inverter.h"
#include "inverter_cmds.h"
#include "nvm.h"
#include "sensata_can.h"
#include "tasker.h"
#include "pid.h"
#include "fan_ctrl.h"
#include "hs_temp.h"
#include "batt_temp.h"
#include "battery_Recipe.h"
#include "device.h"

// -----------
//  Constants
// -----------
#define AC_LINE_DELAY_MIN        (5000)  // enforce minimum time for A/C Line Delay (msecs)
#define AC_LINE_DELAY_MAX       (60000)  // enforce maximum time for A/C Line Delay (msecs)
#define AC_LINE_DELAY_IN_TEST    (2000)  // milliseconds; per Leonard
#define LOAD_SENSE_DELAY_IN_TEST  (5*2)  // 1/2 second intervals; per Leonard


// -----------
// Global data
// -----------
uint16_t g_test_mode      = 0;   // 0=off, 1=in test mode
uint16_t g_led_test_color = 0;   // 0=off, 1=red, 2=green, 3=amber

// ------------------
// access global data
// ------------------
extern char _sysShutDown;   // 0=run, 1=shutdown system
#ifdef OPTION_DC_CONVERTER
 extern PID  CnvPID;
#endif

// ----------------------------------------------------------------------
//      A U T O M A T E D    M F G    T E S T E R    M O D E
// ----------------------------------------------------------------------

// ----------------------------------------------------------------------
uint16_t IsInTestMode()
{
    return(g_test_mode);
}

// ----------------------------------------------------------------------
// enter/exit test mode; 0=exit, 1=enter
void SetTestMode(int16_t mode)
{
    // saved values while in test mode; used to restore values when exiting
    static int16_t g_ac_line_qual_delay_save = 0;
    static int16_t g_load_sense_delay_save   = 0;
    static int8_t  g_load_sense_enabled_save = 0;

    // prevent reentrancy from clobbering saved values
    if (mode    && g_test_mode   ) return; // already in     test mode
    if (mode==0 && g_test_mode==0) return; // already out of test mode
    g_test_mode = mode ? 1 : 0;
    if (g_test_mode)
    {
        // ---------------
        // enter test mode
        // ---------------
        // save values for restoring
        g_ac_line_qual_delay_save = Device.config.ac_line_qual_delay;
        g_load_sense_delay_save   = Inv.config.load_sense_delay;
        g_load_sense_enabled_save = Inv.status.load_sense_enabled;

        // set values used for testing
        Device.config.ac_line_qual_delay = AC_LINE_DELAY_IN_TEST;
        Inv.config.load_sense_delay      = LOAD_SENSE_DELAY_IN_TEST;
        Inv.status.load_sense_enabled    = 1; // enable load sense timing for test mode
    }
    else
    {
        // ---------------
        // exit test mode
        // ---------------
        // restore values
        Device.config.ac_line_qual_delay = g_ac_line_qual_delay_save;
        Inv.config.load_sense_delay      = g_load_sense_delay_save;
        Inv.status.load_sense_enabled    = g_load_sense_enabled_save;
    }
}

// ----------------------------------------------------------------------
//   S E N S A T A   C U S T O M   M E S S A G E    H A N D L E R S
// ----------------------------------------------------------------------

// ----------------------------------------------------------------------
void SetAcLineDelay(uint16_t msecs)
{
    if (msecs < AC_LINE_DELAY_MIN) msecs = AC_LINE_DELAY_MIN;
    if (msecs > AC_LINE_DELAY_MAX) msecs = AC_LINE_DELAY_MAX;
    Device.config.ac_line_qual_delay      = msecs;
    g_nvm.settings.dev.ac_line_qual_delay = msecs;
    nvm_SetDirty();
}

/// ----------------------------------------------------------------------
uint16_t GetLedTestColor()
{
    return(g_led_test_color);
}

// ----------------------------------------------------------------------
// sets a field value from CAN formatted request
// returns: 0=ok, 1=not handled

int16_t sen_SetField(CAN_DATA* canData)
{
    int16_t  rc = 0;	// assume ok
    uint16_t fieldNum;
    uint16_t int16A, field16B;
    float    floatA;

    if (!IsRvcCmdForMe(canData[0])) return(1); // not for me
    fieldNum = MKWORD(canData[1], canData[2]);
    int16A   = MKWORD(canData[3], canData[4]);
    field16B = MKWORD(canData[5], canData[6]);
    memcpy(&floatA, &canData[3], 4); // get the payload as a float value

    LOG(SS_SEN, SV_INFO, "SetField #=%u value16=%u", fieldNum, int16A); 
    switch (fieldNum)
    {
        // system
    case SENFLD_SAVE_TO_NVM:
        nvm_ReqSaveConfig();     
        break;
    case SENFLD_TEST:
        break;
    case SENFLD_ENTER_BOOTLOADER: // enter bootloader to allow firmware upgrades
        _sysShutDown = 1;   // set system shutdown flag
        break;
    case SENFLD_DEV_REMOTE_MODE:  // requires reboot to take effect
        g_nvm.settings.dev.remote_mode = int16A;
        nvm_SetDirty();
		LOG(SS_SYS, SV_INFO, "Set DevRmtMode=%d", g_nvm.settings.dev.remote_mode);
        break; // INT16
    case SENFLD_DEV_AUX_MODE:     // requires reboot to take effect
        g_nvm.settings.dev.aux_mode = int16A;
        nvm_SetDirty();
		LOG(SS_SYS, SV_INFO, "Set DevAuxMode=%d", g_nvm.settings.dev.aux_mode);
        break; // INT16
    case SENFLD_DEV_PUSH_BTN_ENABLE:   // requires reboot to take effect
        g_nvm.settings.dev.pushbutton_enabled = int16A?1:0;
        nvm_SetDirty();
		LOG(SS_SYS, SV_INFO, "Set DevPushBtnEn=%d", g_nvm.settings.dev.pushbutton_enabled);
        break; // INT16
    case SENFLD_DEV_HAS_BATT_TEMP_SENSOR:   // requires reboot to take effect
	  #if defined(OPTION_HAS_CHGR_EN_SWITCH)  // hw switch
	    LOG(SS_SYS, SV_INFO, "Hardware Not Available ");
	  #else
        g_nvm.settings.dev.batt_temp_sense_present = int16A?1:0;
        nvm_SetDirty();
        LOG(SS_SYS, SV_INFO, "Set BattTempSensorPresent%d", g_nvm.settings.dev.batt_temp_sense_present);
	  #endif
        break;

        // can bus
    case SENFLD_CAN_BAUD:          
        g_nvm.settings.can.baud = int16A; 
        LOG(SS_SEN, SV_INFO, "Set CAN Baud=%u", (int)int16A);
        break; // INT16
    case SENFLD_CAN_ADDRESS:
        g_nvm.settings.can.address = (uint8_t)int16A; 
        LOG(SS_SEN, SV_INFO, "Set CAN Address=%u", (int)int16A);
        break; // INT16
    case SENFLD_CAN_INSTANCE:
        can_SetInstance((CAN_INST)int16A);
        break; // INT16
                                                          
    // inverter load-sensing & timers
    case SENFLD_INV_LOADSENSE_ENABLE:			  inv_SetLoadSenseEnabled(int16A);			    break; 		// INT16
    case SENFLD_INV_LOADSENSE_ENABLE_ON_STARTUP:  inv_SetLoadSenseEnabledOnStartup(int16A);	    break;		// INT16
    case SENFLD_INV_LOADSENSE_DELAY:			  inv_SetLoadSenseDelay((uint16_t)int16A);      break;      // INT16
    case SENFLD_INV_LOADSENSE_INTERVAL:			  inv_SetLoadSenseInterval((uint16_t)int16A);	break;      // INT16
    case SENFLD_INV_LOADSENSE_THRESHOLD:		  inv_SetLoadSenseThreshold(int16A);  	        break;		// INT16

    // timer shutdown	
	case SENFLD_TMR_SHUTDOWN_ENABLE:			  dev_SetTimerShutdownEnabled(int16A);		    break;      // INT16
	case SENFLD_TMR_SHUTDOWN_ENABLE_ON_STARTUP:	  dev_SetTimerShutdownEnabledOnStartup(int16A); break;      // INT16
    case SENFLD_TMR_SHUTDOWN_DELAY:               dev_SetTimerShutdownDelay((uint16_t)int16A);  break;      // INT16

    // inverter                                              
    case SENFLD_INV_AC_SETPOINT:                                       break; // INT16
    case SENFLD_INV_P_GAIN:			        inv_SetPGain(&canData[3]); break; // Q16
    case SENFLD_INV_I_GAIN:     	        inv_SetIGain(&canData[3]); break; // Q16
    case SENFLD_INV_D_GAIN:     	        inv_SetDGain(&canData[3]); break; // Q16
    case SENFLD_INV_I_LIMIT:                inv_SetILimit(int16A);	   break; // INT16 hard-coded
	
    case SENFLD_INV_SUPPLY_LOW_SHUTDOWN:    inv_SetSupplyLowShutdown(	(uint16_t)VBATT_VOLTS_ADC(floatA));  break;
    case SENFLD_INV_SUPPLY_LOW_THRES:       inv_SetSupplyLowThreshold(	(uint16_t)VBATT_VOLTS_ADC(floatA));  break;
    case SENFLD_INV_SUPPLY_LOW_HYSTER:      /* not settable */                                               break;
    case SENFLD_INV_SUPPLY_LOW_RECOVER:     inv_SetSupplyLowRecover(	(uint16_t)VBATT_VOLTS_ADC(floatA));  break;
    case SENFLD_INV_SUPPLY_HIGH_RECOVER:    inv_SetSupplyHighRecover(	(uint16_t)VBATT_VOLTS_ADC(floatA));  break;
    case SENFLD_INV_SUPPLY_HIGH_THRES:      inv_SetSupplyHighThreshold(	(uint16_t)VBATT_VOLTS_ADC(floatA));  break;
    case SENFLD_INV_SUPPLY_HIGH_SHUTDOWN:   inv_SetSupplyHighShutdown(	(uint16_t)VBATT_VOLTS_ADC(floatA));  break;

  #ifdef OPTION_HAS_CHARGER

    #define CREATE_VAR(name1, name2)     name1.name2
    #define SET_BATT_RECIPE_VALUE(name, value)   if (Chgr.config.battery_recipe.is_custom){ CREATE_VAR(g_nvm.settings.chgr.battery_recipe, name) = CREATE_VAR(Chgr.config.battery_recipe, name) = value; nvm_SetDirty(); }

    case SENFLD_CHGR_BRANCH_CIRCUIT_RATING:             chgr_SetBranchCircuitAmps(int16A);                                           break;
    case SENFLD_CHGR_AMPS_LIMIT:                		chgr_SetAmpsLimit(floatA);	   											     break;
    case SENFLD_CHGR_AC_QUAL_MSEC:                      SetAcLineDelay((uint16_t)int16A);                                            break;
    case SENFLD_CHGR_BATT_TYPE:                         chgr_SetBattType(int16A,0);                                                  break;
    case SENFLD_CHGR_BATT_RECIPE_CUSTOM_FLAG:           chgr_SetBattCustomFlag(int16A);                                              break;
    case SENFLD_CHGR_BATT_RECIPE_FLOAT_THRESHOLD:       SET_BATT_RECIPE_VALUE(float_vthreshold         , VBATT_VOLTS_ADC(floatA) );  break;
    case SENFLD_CHGR_BATT_RECIPE_MAX_AMP_HOURS:         SET_BATT_RECIPE_VALUE(cc_max_amp_hours         , int16A                  );  break;
    case SENFLD_CHGR_BATT_RECIPE_CV_ROC_SDEV_THRES:     SET_BATT_RECIPE_VALUE(cv_roc_sdev_threshold    , int16A                  );  break;
    case SENFLD_CHGR_BATT_RECIPE_CV_ROC_TIMEOUT:        SET_BATT_RECIPE_VALUE(cv_roc_timeout_minutes   , int16A                  );  break;
    case SENFLD_CHGR_BATT_RECIPE_CV_TIMEOUT:            SET_BATT_RECIPE_VALUE(cv_timeout_minutes       , int16A                  );  break;
    case SENFLD_CHGR_BATT_RECIPE_EQ_VOLTS_MAX:          SET_BATT_RECIPE_VALUE(eq_volts_max             , VBATT_VOLTS_ADC(floatA) );  break;
    case SENFLD_CHGR_BATT_RECIPE_EQ_VSETPOINT:          SET_BATT_RECIPE_VALUE(eq_vsetpoint             , VBATT_VOLTS_ADC(floatA) );  break;
    case SENFLD_CHGR_BATT_RECIPE_EQ_TIMEOUT:            SET_BATT_RECIPE_VALUE(eq_timeout_minutes       , int16A                  );  break;
    case SENFLD_CHGR_BATT_RECIPE_CV_VSETPOINT:          SET_BATT_RECIPE_VALUE(cv_vsetpoint             , VBATT_VOLTS_ADC(floatA) );  break;
    case SENFLD_CHGR_BATT_RECIPE_CV_VOLTS_MAX:          SET_BATT_RECIPE_VALUE(cv_volts_max             , VBATT_VOLTS_ADC(floatA) );  break;
    case SENFLD_CHGR_BATT_RECIPE_FLOAT_VSETPOINT:       SET_BATT_RECIPE_VALUE(float_vsetpoint          , VBATT_VOLTS_ADC(floatA) );  break;
    case SENFLD_CHGR_BATT_RECIPE_FLOAT_VOLTS_MAX:       SET_BATT_RECIPE_VALUE(float_volts_max          , VBATT_VOLTS_ADC(floatA) );  break;
    case SENFLD_CHGR_BATT_RECIPE_FLOAT_TIMEOUT:         SET_BATT_RECIPE_VALUE(float_timeout_minutes    , int16A                  );  break;
    case SENFLD_CHGR_BATT_RECIPE_WARM_BATT_TEMP_RECOV:  SET_BATT_RECIPE_VALUE(batt_temp_warm_recov     , int16A                  );  break;
    case SENFLD_CHGR_BATT_RECIPE_WARM_BATT_TEMP_THRES:  SET_BATT_RECIPE_VALUE(batt_temp_warm_thres     , int16A                  );  break;
    case SENFLD_CHGR_BATT_RECIPE_SHUTDOWN_TEMP_RECOV:   SET_BATT_RECIPE_VALUE(batt_temp_shutdown_recov , int16A                  );  break;
    case SENFLD_CHGR_BATT_RECIPE_SHUTDOWN_TEMP_THRES:   SET_BATT_RECIPE_VALUE(batt_temp_shutdown_thres , int16A                  );  break;
    //   SENFLD_CHGR_DFLT_RECIPE  are read only

  #endif // OPTION_HAS_CHARGER

	//	DC+DC converter
    case SENFLD_CONV_P_GAIN:                CnvPID.KP    		= floatA;   break; // FLOAT
    case SENFLD_CONV_I_GAIN:                CnvPID.KI         	= floatA;   break; // FLOAT
    case SENFLD_CONV_D_GAIN:                CnvPID.KD         	= floatA;   break; // FLOAT
    case SENFLD_CONV_I_LIMIT:               CnvPID.ILIM       	= int16A;   break; // INT16
    case SENFLD_CONV_D_LIMIT:               CnvPID.DLIM       	= int16A;   break; // INT16
    case SENFLD_CONV_PID_DEADBAND:          CnvPID.deadband   	= int16A;   break; // INT16
    case SENFLD_CONV_FF_GAIN:               CnvPID.KFF    		= floatA;   break; // FLOAT
    case SENFLD_CONV_FF_DEADBAND:           CnvPID.FF_deadband  = int16A;   break; // INT16
    case SENFLD_CONV_DCOUT_SETPOINT:  	    cnv_SetDcSetPoint(floatA);   	break; // FLOAT

    // testing
    case SENFLD_TEST_MODE:                  SetTestMode(int16A);            break; // INT16
    case SENFLD_TEST_COOLING_FAN:           fan_SetOverRide(int16A);        break; // INT16
    case SENFLD_TEST_LED:                   
        if (int16A < 0) int16A = 0; // clip lo
        if (int16A > 3) int16A = 3; // clip hi
        g_led_test_color = int16A;
        break; // INT16

	} // switch
	return(rc);
}

// ----------------------------------------------------------------------
// returns device features bitmask
static uint16_t DeviceFeatures(void)
{
    uint16_t features = 0;

   #ifdef OPTION_HAS_CHARGER
    features |= SENFLD_DEV_FEATURES_HAS_CHARGER;
   #endif

   #ifdef OPTION_HAS_CHGR_EN_SWITCH
    features |= SENFLD_DEV_FEATURES_HAS_CHARGER_EN_SWITCH;
   #endif

   #ifdef OPTION_SSR
    features |= SENFLD_DEV_FEATURES_HAS_SSR;
   #endif

   #ifdef OPTION_VOLTA_UI
    features |= SENFLD_DEV_FEATURES_HAS_VOLTA_UI;
   #endif

    return(features);
}

// ----------------------------------------------------------------------
// return battery voltage recipe value and check for special case of not used
// if not used case, return -1
INLINE float BATT_VOLT_RECIPE_VALUE(uint16_t adc)
{
    return( (adc == BATT_RECIPE_NOT_USED_ADC_VALUE) ? (float)-1.0 : VBATT_ADC_VOLTS(adc));
}


// ----------------------------------------------------------------------
//	retrieves the data for a field; returns it in CAN transport format
// 	returns: 0=ok, 1=invalid field

int16_t sen_GetField(CAN_DATA* canData, CAN_DATA* outData, CAN_LEN* ndata)   // outData[40]
{
    BATTERY_RECIPE_t* pRomRecipe;
    uint16_t fieldNum;
    int len;

    // accessor macros
    #define  GETINT16(name)     { outData[3]=SENFLD_TYPE_INT16;  *pInt16 = name;          }
    #define  GETINT32(name)     { outData[3]=SENFLD_TYPE_INT32;  *pInt32 = name;          }
    #define  GETFLOAT(name)     { outData[3]=SENFLD_TYPE_FLOAT;  *pFloat = (float)name;   }
    #define  GETSTR(name,len)   { outData[3]=SENFLD_TYPE_STRING; memcpy(strA, name, len); }
    #define  GETQ16(name)       { outData[3]=SENFLD_TYPE_Q16;    memcpy(strA, name,   4); }
    #define  ROMBATT()            pRomRecipe=PtrRomBattRecipe(Chgr.config.battery_recipe.batt_type)

    // casting operators for stuffing data into byte array
    #define   fieldType            outData[3]  // field type
    int8_t*   strA   =   (int8_t*)&outData[4]; // alias for string copy
    uint16_t* pInt16 = (uint16_t*)&outData[4]; // stuff int16 as 2 bytes
    float*    pFloat =    (float*)&outData[4]; // stuff float as 4 bytes
    uint32_t* pInt32 = (uint32_t*)&outData[4]; // stuff int32 as 4 bytes

    // echo back instance and field number
    outData[0] = canData[0];
    outData[1] = canData[1];
    outData[2] = canData[2];

    // --------------------------------
    // get data for the specific field
    // --------------------------------
    fieldType = SENFLD_TYPE_NONE;
    fieldNum  = MKWORD(canData[1], canData[2]);
    switch (fieldNum)
    {
    // Device Info
    case SENFLD_DEV_TYPE:            GETINT16(OPTION_DEV);              break; // UINT16 (read only) Device Type: 1=Inverter, 2=DC Converter
    case SENFLD_DEV_PCB:             GETINT16(OPTION_PCB);              break; // UINT16 (read only) Device PCB   1=NP, 2=LPC
    case SENFLD_DEV_DCIN_VOLT:       GETINT16(OPTION_DCIN);             break; // UINT16 (read only) Device Input Voltage: 12,24,48,51
    case SENFLD_DEV_FEATURES:        GETINT16(DeviceFeatures());        break; // UINT16 (read only) Device Feature Bits
    case SENFLD_DEV_STATUS:          GETINT16(Device.status.all_flags); break; // UINT16 (read only) Device Status Bits
    case SENFLD_DEV_ERRORS:          GETINT16(Device.error.all_flags);  break; // UINT16 (read only) Device Error  Bits
	case SENFLD_DEV_AC_QUAL_TIMER:	 GETINT16(Device.status.ac_line_qual_timer_msec);	break; // INT16 running timer value
    case SENFLD_DEV_HEAT_SINK_TEMP:
	  #if(OPTION_PCB == PCB_LPC)
		GETINT16(RVC_NOT_SUPPORTED_16BIT);  // binary temp sensor only; no Celcius reading
	  #else
		GETINT16(HeatSinkTempC());   // UINT16 (read only) PWM Heat Sink temperature Celcius
	  #endif
    	break;
    case SENFLD_DEV_REMOTE_MODE:          GETINT16(g_nvm.settings.dev.remote_mode);             break; // INT16 (need to read nvm value)
    case SENFLD_DEV_AUX_MODE:             GETINT16(g_nvm.settings.dev.aux_mode);                break; // INT16 (need to read nvm value)
    case SENFLD_DEV_PUSH_BTN_ENABLE:      GETINT16(g_nvm.settings.dev.pushbutton_enabled);      break; // INT16 (need to read nvm value)
    case SENFLD_DEV_HAS_BATT_TEMP_SENSOR: GETINT16(g_nvm.settings.dev.batt_temp_sense_present); break; // INT16 (need to read nvm value)
    
        // can bus                  
    case SENFLD_CAN_BAUD:            GETINT16(g_can.baud);       break; // INT16
    case SENFLD_CAN_ADDRESS:         GETINT16(g_can.address);    break; // INT16
    case SENFLD_CAN_INSTANCE:        GETINT16(g_can.instance);   break; // INT16
                                                               
    // manufacturing                                          
    case SENFLD_MFG_MODEL_NO:        GETSTR(g_RomMfgInfo.model_no,  SENFLD_MAX_STRING); break; // STRING 24
    case SENFLD_MFG_MFG_DATE:        GETSTR(g_RomMfgInfo.mfg_date,  SENFLD_MAX_STRING); break; // STRING 20
    case SENFLD_MFG_SERIAL_NO:       GETSTR(g_RomMfgInfo.serial_no, SENFLD_MAX_STRING); break; // STRING 20
    case SENFLD_MFG_REV_NO:          GETSTR(g_RomMfgInfo.rev,       SENFLD_MAX_STRING); break; // STRING 4
    case SENFLD_MFG_FW_NO:           GETSTR(FW_ID_STRING,           SENFLD_MAX_STRING); break; // STRING 24 

	
    // analog
    case SENFLD_ANA_VBATT_SLOPE:     GETFLOAT((float)VBATT_SLOPE);          break; // FLOAT
    case SENFLD_ANA_VBATT_INTERCEPT: GETFLOAT((float)VBATT_INTERCEPT);      break; // FLOAT
    case SENFLD_ANA_VBATT_ADC:       GETINT16(VBattCycleAvg());             break; // INT16

    case SENFLD_ANA_VREG15_SLOPE:    GETFLOAT((float)VREG15_SLOPE);         break; // FLOAT
    case SENFLD_ANA_VREG15_INTERCEPT:GETFLOAT((float)VREG15_INTERCEPT);     break; // FLOAT
    case SENFLD_ANA_VREG15_ADC:      GETINT16(VReg15Raw());                 break; // INT16

    case SENFLD_ANA_VAC_SLOPE:       GETFLOAT((float)VAC_SLOPE);            break; // FLOAT
    case SENFLD_ANA_VAC_INTERCEPT:   GETFLOAT((float)VAC_INTERCEPT);        break; // FLOAT
    case SENFLD_ANA_VAC_ADC:         GETINT16(VacRMS());                    break; // INT16
    case SENFLD_ANA_VAC_MIN_ADC:     GETINT16(An.Status.VAC_raw_min);       break; // INT16
    case SENFLD_ANA_VAC_MAX_ADC:     GETINT16(An.Status.VAC_raw_max);       break; // INT16

    case SENFLD_ANA_IMEAS_SLOPE:     GETFLOAT((float)IMEAS_INV_SLOPE);      break; // FLOAT
    case SENFLD_ANA_IMEAS_INTERCEPT: GETFLOAT((float)IMEAS_INV_INTERCEPT);  break; // FLOAT
    case SENFLD_ANA_IMEAS_ADC:       GETINT16(IMeasRMS());                  break; // INT16

    case SENFLD_ANA_ILINE_SLOPE:     GETFLOAT((float)ILINE_SLOPE);          break; // FLOAT
    case SENFLD_ANA_ILINE_INTERCEPT: GETFLOAT((float)ILINE_INTERCEPT);      break; // FLOAT
    case SENFLD_ANA_ILINE_ADC:       GETINT16(ILineRMS());                  break; // INT16
    case SENFLD_ANA_ILIMIT_ADC:      GETINT16(ILimitRMS());                 break; // INT16
    case SENFLD_ANA_ILIMIT_RATIO:    GETFLOAT(an_GetILimitRatio());         break; // FLOAT  (read only)  ratio of ILimit/IMeas

    case SENFLD_ANA_DCDC_SLOPE:      GETFLOAT(DCDC_SLOPE);                  break; // FLOAT
    case SENFLD_ANA_DCDC_INTERCEPT:  GETFLOAT(DCDC_INTERCEPT);              break; // FLOAT
    case SENFLD_ANA_DCDC_ADC:        GETINT16(An.Status.HsTemp.avg.val);    break; // INT16

    case SENFLD_INV_DC_IN_VOLTS:     GETFLOAT(an_GetBatteryVoltage(0));     break; // FLOAT
    case SENFLD_INV_DC_IN_AMPS:      GETFLOAT(an_GetInvBatteryCurrent(0));  break; // FLOAT
    case SENFLD_INV_AC_OUT_VOLTS:    GETFLOAT(an_GetInvAcVoltage(0));       break; // FLOAT
    case SENFLD_INV_AC_OUT_AMPS:     GETFLOAT(an_GetInvAcAmps(0));          break; // FLOAT
    case SENFLD_CHG_AC_IN_VOLTS:     GETFLOAT(an_GetChgrAcVoltage(0));      break; // FLOAT
    case SENFLD_CHG_AC_IN_AMPS:      GETFLOAT(an_GetChgrIMeasCurrent(0));   break; // FLOAT
    case SENFLD_CHG_DC_OUT_VOLTS:    GETFLOAT(an_GetBatteryVoltage(0));     break; // FLOAT
    case SENFLD_CHG_DC_OUT_AMPS:     GETFLOAT(an_GetChgrBatteryCurrent(0)); break; // FLOAT
  #if defined(OPTION_HAS_CHGR_EN_SWITCH)                                    
    case SENFLD_BATT_TEMP:           GETFLOAT(0);                           break; // FLOAT
  #else
    case SENFLD_BATT_TEMP:           GETFLOAT(BattTemp_celsius());          break; // FLOAT
  #endif
    case SENFLD_INV_AC_OUT_WATTS:    GETFLOAT(an_GetACWatts(0));            break; // INT16
    case SENFLD_CHG_BYPASS_AMPS:     GETFLOAT(an_GetChgrBypassCurrent(0));  break; // FLOAT
    case SENFLD_CHG_ILINE_AMPS:      GETFLOAT(an_GetChgrILineCurrent(0));   break; // FLOAT
    case SENFLD_AC_OUT_WATTS_ADC:    GETINT16(WattsLongAvg());              break; // INT16
    case SENFLD_CNV_DC_OUT_VOLTS:    GETFLOAT(cnv_GetDcOutVolts(0));        break; // FLOAT

    // symmetric fault codes
    case SENFLD_ERR_INV_DC_IN_VOLTS: GETFLOAT(an_GetBatteryVoltage(1));     break; // FLOAT
    case SENFLD_ERR_INV_DC_IN_AMPS:  GETFLOAT(an_GetInvBatteryCurrent(1));  break; // FLOAT
    case SENFLD_ERR_INV_AC_OUT_VOLTS:GETFLOAT(an_GetInvAcVoltage(1));       break; // FLOAT
    case SENFLD_ERR_INV_AC_OUT_AMPS: GETFLOAT(an_GetInvAcAmps(1));          break; // FLOAT
    case SENFLD_ERR_CHG_AC_IN_VOLTS: GETFLOAT(an_GetChgrAcVoltage(1));      break; // FLOAT
    case SENFLD_ERR_CHG_AC_IN_AMPS:  GETFLOAT(an_GetChgrIMeasCurrent(1));   break; // FLOAT
    case SENFLD_ERR_CHG_DC_OUT_VOLTS:GETFLOAT(an_GetBatteryVoltage(1));     break; // FLOAT
    case SENFLD_ERR_CHG_DC_OUT_AMPS: GETFLOAT(an_GetChgrBatteryCurrent(1)); break; // FLOAT
  #if defined(OPTION_HAS_CHGR_EN_SWITCH)                                    
    case SENFLD_ERR_BATT_TEMP:       GETFLOAT(0);                           break; // FLOAT
  #else
    case SENFLD_ERR_BATT_TEMP:       GETFLOAT(BattTemp_celsius());          break; // FLOAT
  #endif
    case SENFLD_ERR_INV_AC_OUT_WATTS:GETFLOAT(an_GetACWatts(1));            break; // FLOAT
    case SENFLD_ERR_CHG_BYPASS_AMPS: GETFLOAT(an_GetChgrBypassCurrent(1));  break; // FLOAT
    case SENFLD_ERR_CHG_ILINE_AMPS:  GETFLOAT(an_GetChgrILineCurrent(1));   break; // FLOAT
    case SENFLD_ERR_AC_OUT_WATTS_ADC:GETINT16(AnSaved.AvgWACr.val);         break; // INT16
    case SENFLD_ERR_CNV_DC_OUT_VOLTS:GETFLOAT(cnv_GetDcOutVolts(1));        break; // FLOAT

    // inverter load-sensing & timers
    case SENFLD_INV_LOADSENSE_ENABLE:	         GETINT16(IsInvLoadSenseEn());                  break; // INT16	
    case SENFLD_INV_LOADSENSE_ENABLE_ON_STARTUP: GETINT16(Inv.config.load_sense_enabled);       break; // INT16	
    case SENFLD_INV_LOADSENSE_ACTIVE:	         GETINT16(IsInvLoadSenseMode());                break; // INT16		
    case SENFLD_INV_LOADSENSE_DELAY:	         GETINT16(Inv.config.load_sense_delay);         break; // INT16		
    case SENFLD_INV_LOADSENSE_INTERVAL:	         GETINT16(Inv.config.load_sense_interval);      break; // INT16
    case SENFLD_INV_LOADSENSE_THRESHOLD:         GETINT16(Inv.config.load_sense_threshold);     break; // INT16
    case SENFLD_INV_LOADSENSE_TIMER:	         GETINT16(Inv.status.load_sense_timer);         break; // INT16
	                                                                                            
    case SENFLD_TMR_SHUTDOWN_ENABLE:	         GETINT16(Device.status.tmr_shutdown_enabled);  break; // INT16	
    case SENFLD_TMR_SHUTDOWN_ENABLE_ON_STARTUP:	 GETINT16(Device.config.tmr_shutdown_enabled);  break; // INT16	
    case SENFLD_TMR_SHUTDOWN_DELAY:   	         GETINT16(dev_GetTimerShutdownDelay());         break; // INT16
    case SENFLD_TMR_SHUTDOWN_TIMER:              GETINT16(dev_GetTimerShutdownTimer());         break; // INT16

    // inverter                              
    case SENFLD_INV_STATUS:                      GETINT16(Inv.status.all_flags);                break; // INT16
    case SENFLD_INV_ERRORS:                      GETINT16(Inv.error.all_flags);                 break; // INT16
	                                            
    case SENFLD_INV_AC_SETPOINT:                 GETFLOAT(INV_RMS_VAC_SETPOINT_VOLTS);          break; // FLOAT
    case SENFLD_INV_P_GAIN:                      GETQ16(&Inv.config.p_gain);	                                break; // Q16
    case SENFLD_INV_I_GAIN:                      GETQ16(&Inv.config.i_gain);	                                break; // Q16
    case SENFLD_INV_D_GAIN:                      GETQ16(&Inv.config.d_gain);	                                break; // Q16
    case SENFLD_INV_I_LIMIT:                     GETINT16(Inv.config.i_limit);                                  break; // INT16

    // battery voltage thresholds
    case SENFLD_INV_SUPPLY_LOW_HYSTER:           GETFLOAT(VBATT_ADC_VOLTS(InvCfgVBattLoHyster()  ));    break; // FLOAT
    case SENFLD_INV_SUPPLY_LOW_RECOVER:          GETFLOAT(VBATT_ADC_VOLTS(InvCfgVBattLoRecover() ));    break; // FLOAT
    case SENFLD_INV_SUPPLY_LOW_THRES:            GETFLOAT(VBATT_ADC_VOLTS(InvCfgVBattLoThresh()  ));    break; // FLOAT
    case SENFLD_INV_SUPPLY_LOW_SHUTDOWN:         GETFLOAT(VBATT_ADC_VOLTS(InvCfgVBattLoShutDown()));    break; // FLOAT
    case SENFLD_INV_SUPPLY_HIGH_RECOVER:         GETFLOAT(VBATT_ADC_VOLTS(InvCfgVBattHiRecover() ));    break; // FLOAT
    case SENFLD_INV_SUPPLY_HIGH_THRES:           GETFLOAT(VBATT_ADC_VOLTS(InvCfgVBattHiThresh()  ));    break; // FLOAT
    case SENFLD_INV_SUPPLY_HIGH_SHUTDOWN:        GETFLOAT(VBATT_ADC_VOLTS(InvCfgVBattHiShutDown()));    break; // FLOAT
                                            
    // charger                              
    case SENFLD_CHGR_STATUS:                            GETINT16(Chgr.status.all_flags);                                       break; // INT16
    case SENFLD_CHGR_STATUS2:                           GETINT16(Chgr.status.all_flags2);                                      break; // INT16
    case SENFLD_CHGR_ERRORS:                            GETINT16(Chgr.error.all_flags);                                        break; // INT16
    case SENFLD_CHGR_BRANCH_CIRCUIT_RATING:             GETINT16(ChgrCfgBcrAmps());                                            break; // INT16
    case SENFLD_CHGR_AMPS_LIMIT:    			     	GETFLOAT(chgr_GetAmpsLimit());                                         break; // FLOAT                               
    case SENFLD_CHGR_AC_QUAL_MSEC:                      GETINT16(Device.config.ac_line_qual_delay);                            break; // INT16

    // battery recipe                                                                                                          
    case SENFLD_CHGR_BATT_TYPE:                         GETINT16(Chgr.config.battery_recipe.batt_type);                                 break; // INT16
    case SENFLD_CHGR_BATT_RECIPE_FLOAT_THRESHOLD:       GETFLOAT(BATT_VOLT_RECIPE_VALUE(Chgr.config.battery_recipe.float_vthreshold));  break; // FLOAT
    case SENFLD_CHGR_BATT_RECIPE_CUSTOM_FLAG:           GETINT16(Chgr.config.battery_recipe.is_custom);                                 break; // INT16
    case SENFLD_CHGR_BATT_RECIPE_MAX_AMP_HOURS:         GETINT16(Chgr.config.battery_recipe.cc_max_amp_hours);                          break; // INT16
    case SENFLD_CHGR_BATT_RECIPE_CV_ROC_SDEV_THRES:     GETINT16(Chgr.config.battery_recipe.cv_roc_sdev_threshold);                     break; // INT16
    case SENFLD_CHGR_BATT_RECIPE_CV_ROC_TIMEOUT:        GETINT16(Chgr.config.battery_recipe.cv_roc_timeout_minutes);                    break; // INT16
    case SENFLD_CHGR_BATT_RECIPE_EQ_VOLTS_MAX:          GETFLOAT(BATT_VOLT_RECIPE_VALUE(Chgr.config.battery_recipe.eq_volts_max));      break; // FLOATA
    case SENFLD_CHGR_BATT_RECIPE_EQ_VSETPOINT:          GETFLOAT(BATT_VOLT_RECIPE_VALUE(Chgr.config.battery_recipe.eq_vsetpoint));      break; // FLOATA
    case SENFLD_CHGR_BATT_RECIPE_EQ_TIMEOUT:            GETINT16(Chgr.config.battery_recipe.eq_timeout_minutes);           	            break; // INT16
    case SENFLD_CHGR_BATT_RECIPE_CV_VSETPOINT:          GETFLOAT(BATT_VOLT_RECIPE_VALUE(Chgr.config.battery_recipe.cv_vsetpoint));      break; // FLOATA
    case SENFLD_CHGR_BATT_RECIPE_CV_VOLTS_MAX:          GETFLOAT(BATT_VOLT_RECIPE_VALUE(Chgr.config.battery_recipe.cv_volts_max));      break; // FLOATA
    case SENFLD_CHGR_BATT_RECIPE_FLOAT_VSETPOINT:       GETFLOAT(BATT_VOLT_RECIPE_VALUE(Chgr.config.battery_recipe.float_vsetpoint));   break; // FLOATA
    case SENFLD_CHGR_BATT_RECIPE_FLOAT_VOLTS_MAX:       GETFLOAT(BATT_VOLT_RECIPE_VALUE(Chgr.config.battery_recipe.float_volts_max));   break; // FLOATA
    case SENFLD_CHGR_BATT_RECIPE_FLOAT_TIMEOUT:         GETINT16(Chgr.config.battery_recipe.float_timeout_minutes);       	            break; // INT16
    case SENFLD_CHGR_BATT_RECIPE_WARM_BATT_TEMP_RECOV:  GETINT16(Chgr.config.battery_recipe.batt_temp_warm_recov);                      break; // INT16
    case SENFLD_CHGR_BATT_RECIPE_WARM_BATT_TEMP_THRES:  GETINT16(Chgr.config.battery_recipe.batt_temp_warm_thres);                      break; // INT16
    case SENFLD_CHGR_BATT_RECIPE_SHUTDOWN_TEMP_RECOV:   GETINT16(Chgr.config.battery_recipe.batt_temp_shutdown_recov);                  break; // INT16
    case SENFLD_CHGR_BATT_RECIPE_SHUTDOWN_TEMP_THRES:   GETINT16(Chgr.config.battery_recipe.batt_temp_shutdown_thres);                  break; // INT16
    case SENFLD_CHGR_BATT_RECIPE_CV_TIMEOUT:            GETINT16(Chgr.config.battery_recipe.cv_timeout_minutes);           	            break; // INT16
    // default recipe (ROM values)
    case SENFLD_CHGR_DFLT_RECIPE_TYPE:                  ROMBATT(); GETINT16(pRomRecipe->batt_type);                        break; // INT16
    case SENFLD_CHGR_DFLT_RECIPE_FLOAT_THRESHOLD:       ROMBATT(); GETFLOAT(BATT_VOLT_RECIPE_VALUE(pRomRecipe->float_vthreshold));break; // FLOAT
    case SENFLD_CHGR_DFLT_RECIPE_CUSTOM_FLAG:           ROMBATT(); GETINT16(pRomRecipe->is_custom);                        break; // INT16
    case SENFLD_CHGR_DFLT_RECIPE_MAX_AMP_HOURS:         ROMBATT(); GETINT16(pRomRecipe->cc_max_amp_hours);                 break; // INT16
    case SENFLD_CHGR_DFLT_RECIPE_CV_ROC_SDEV_THRES:     ROMBATT(); GETINT16(pRomRecipe->cv_roc_sdev_threshold);            break; // INT16
    case SENFLD_CHGR_DFLT_RECIPE_CV_ROC_TIMEOUT :       ROMBATT(); GETINT16(pRomRecipe->cv_roc_timeout_minutes);           break; // INT16
    case SENFLD_CHGR_DFLT_RECIPE_EQ_VOLTS_MAX:          ROMBATT(); GETFLOAT(BATT_VOLT_RECIPE_VALUE(pRomRecipe->eq_volts_max));    break; // FLOATA
    case SENFLD_CHGR_DFLT_RECIPE_EQ_VSETPOINT:          ROMBATT(); GETFLOAT(BATT_VOLT_RECIPE_VALUE(pRomRecipe->eq_vsetpoint));    break; // FLOATA
    case SENFLD_CHGR_DFLT_RECIPE_EQ_TIMEOUT:            ROMBATT(); GETINT16(pRomRecipe->eq_timeout_minutes);           	   break; // INT16
    case SENFLD_CHGR_DFLT_RECIPE_CV_VSETPOINT:          ROMBATT(); GETFLOAT(BATT_VOLT_RECIPE_VALUE(pRomRecipe->cv_vsetpoint));    break; // FLOATA
    case SENFLD_CHGR_DFLT_RECIPE_CV_VOLTS_MAX:          ROMBATT(); GETFLOAT(BATT_VOLT_RECIPE_VALUE(pRomRecipe->cv_volts_max));    break; // FLOATA
    case SENFLD_CHGR_DFLT_RECIPE_FLOAT_VSETPOINT:       ROMBATT(); GETFLOAT(BATT_VOLT_RECIPE_VALUE(pRomRecipe->float_vsetpoint)); break; // FLOATA
    case SENFLD_CHGR_DFLT_RECIPE_FLOAT_VOLTS_MAX:       ROMBATT(); GETFLOAT(BATT_VOLT_RECIPE_VALUE(pRomRecipe->float_volts_max)); break; // FLOATA
    case SENFLD_CHGR_DFLT_RECIPE_FLOAT_TIMEOUT:         ROMBATT(); GETINT16(pRomRecipe->float_timeout_minutes);       	   break; // INT16
    case SENFLD_CHGR_DFLT_RECIPE_WARM_BATT_TEMP_RECOV:  ROMBATT(); GETINT16(pRomRecipe->batt_temp_warm_recov);             break; // INT16
    case SENFLD_CHGR_DFLT_RECIPE_WARM_BATT_TEMP_THRES:  ROMBATT(); GETINT16(pRomRecipe->batt_temp_warm_thres);             break; // INT16
    case SENFLD_CHGR_DFLT_RECIPE_SHUTDOWN_TEMP_RECOV:   ROMBATT(); GETINT16(pRomRecipe->batt_temp_shutdown_recov);         break; // INT16
    case SENFLD_CHGR_DFLT_RECIPE_SHUTDOWN_TEMP_THRES:   ROMBATT(); GETINT16(pRomRecipe->batt_temp_shutdown_thres);         break; // INT16
    case SENFLD_CHGR_DFLT_RECIPE_CV_TIMEOUT:            ROMBATT(); GETINT16(pRomRecipe->cv_timeout_minutes);           	   break; // INT16

    case SENFLD_CHGR_VOLT_SETPOINT:                     GETINT16(Chgr.status.volt_setpoint);           	break;  // 630 INT16  Charge Voltage Setpoint           (volts)
    case SENFLD_CHGR_AMPS_SETPOINT:                     GETINT16(Chgr.status.amps_setpoint);           	break;	// 631 INT16  Charge Amperage Setpoint          (Amps)
    case SENFLD_CHGR_CC_TIMER_AMP_MINUTES:              GETINT32(Chgr.status.cc_timer_amp_minutes);     break;  // 632 INT32  Const-Current Amp-Minutes Remaining (amp-minutes)
    case SENFLD_CHGR_CC_ELAPSED_MINUTES:                GETINT16(Chgr.status.cc_elapsed_minutes);       break;  // 633 INT16  Const-Current Time Elapsed        (minutes)
    case SENFLD_CHGR_CV_TIMER_MINUTES:                  GETINT16(Chgr.status.cv_timer_minutes);         break;  // 634 INT16  Const-Voltage Time Remaining      (minutes)
    case SENFLD_CHGR_CV_ROC_TIMER_MINUTES:              GETINT16(Chgr.status.cv_roc_timer_minutes);     break;  // 635 INT16  Const-Voltage ROC Time Remaining  (minutes)
    case SENFLD_CHGR_FLOAT_TIMER_MINUTES:               GETINT16(Chgr.status.float_timer_minutes);      break;  // 636 INT16  Float Time Remaining              (minutes)
    case SENFLD_CHGR_SS_CC_LM_TIMER_MINUTES:            GETINT16(Chgr.status.ss_cc_lm_timer_minutes);   break;  // 637 INT16  Sum of SS, CC, and [Current-Limit] Time Remaining (minutes)
    case SENFLD_CHGR_CC_CV_TIMER_MINUTES:               GETINT16(Chgr.status.cc_cv_timer_minutes);      break;  // 638 INT16  Sum of CC, and CV Time Remaining  (minutes)
    case SENFLD_CHGR_EQ_TIMER_MINUTES:                  GETINT16(Chgr.status.eq_timer_minutes);        	break;  // 639 INT16  Equalization Time Remaining       (minutes)
	
    //	DC+DC converter
    case SENFLD_CONV_P_GAIN:            GETFLOAT(CnvPID.KP);               break; // FLOAT
    case SENFLD_CONV_I_GAIN:            GETFLOAT(CnvPID.KI);               break; // FLOAT
    case SENFLD_CONV_D_GAIN:            GETFLOAT(CnvPID.KD);               break; // FLOAT
    case SENFLD_CONV_I_LIMIT:           GETINT16(CnvPID.ILIM);             break; // INT16
    case SENFLD_CONV_D_LIMIT:           GETINT16(CnvPID.DLIM);             break; // INT16
    case SENFLD_CONV_PID_DEADBAND:      GETINT16(CnvPID.deadband);         break; // INT16
    case SENFLD_CONV_FF_GAIN:           GETFLOAT(CnvPID.KFF);              break; // FLOAT
    case SENFLD_CONV_FF_DEADBAND:       GETINT16(CnvPID.FF_deadband);      break; // INT16
    case SENFLD_CONV_DCOUT_SETPOINT: 	GETFLOAT(cnv_GetDcSetPoint());     break; // FLOAT
    case SENFLD_CONV_DCOUT_VOLTS:       GETFLOAT(cnv_GetDcOutVolts(0));    break; // FLOAT (read only)
    case SENFLD_CONV_DCOUT_VOLT_MODEL:  GETINT16(OPTION_DCDC_DCOUT_VOLTS); break; // INT16 (read only)

    // testing
    case SENFLD_TEST_MODE:              GETINT16(g_test_mode);             break; // INT16
    case SENFLD_TEST_COOLING_FAN:    	GETINT16(fan_GetDutyCycle());      break; // INT16
    case SENFLD_TEST_LED:               GETINT16(g_led_test_color);        break; // INT16

    // debugging
    case SENFLD_DBG_STATE_MAIN:			GETINT16(dev_MainState());		   break; // INT16
    case SENFLD_DBG_STATE_INV:			GETINT16(inv_GetState());	       break; // INT16
    case SENFLD_DBG_STATE_CHGR:			GETINT16(ChgrState());	           break; // INT16

    } // switch

    // save field type
    if (fieldType == SENFLD_TYPE_NONE)
    {
        // field type not set
        *ndata = 4;
        LOG(SS_SEN, SV_ERR, "GetField Invalid#=%u", fieldNum);
        return(1);  // invalid field
    }

    // --------------------
    // complete the packet
    // --------------------
    switch (fieldType)
    {
    case SENFLD_TYPE_INT16:
        *ndata = 6;
        LOG(SS_SEN, SV_INFO, "GetField #=%u  uint16=%u", fieldNum, *pInt16);
        break;

    case SENFLD_TYPE_FLOAT:
        *ndata = 8;
        LOG(SS_SEN, SV_INFO, "GetField #=%u  float=%.5f", fieldNum, *pFloat);
        break;

    case SENFLD_TYPE_INT32:
        *ndata = 8;
        LOG(SS_SEN, SV_INFO, "GetField #=%u  uint32A=%lu", fieldNum, *pInt32);
        break;

    case SENFLD_TYPE_STRING:
        strA[SENFLD_MAX_STRING] = 0;   // guarantee null terminate
        len = strlen(strA);
        *ndata = (4 + len);
        LOG(SS_SEN, SV_INFO, "GetField #=%u  len=%u str=%s", fieldNum, len, strA);
        break;
		
    case SENFLD_TYPE_Q16:
        *ndata = 8;
        LOG(SS_SEN, SV_INFO, "GetField #=%u  Q16=%X%X.%X%X", fieldNum, outData[7], outData[6], outData[5], outData[4]);
        break;			
    } // switch

    return(0); // ok
}

// <><><><><><><><><><><><><> sensata_can.c <><><><><><><><><><><><><><><><><><><><><><>
