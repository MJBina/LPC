// <><><><><><><><><><><><><> charger_3step.c <><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//  Copyright(C) 2017 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//  $Workfile: charger_3step.c $
//  $Author: clark_dailey $
//  $Date: 2018-03-28 17:13:06-05:00 $
//  $Revision: 242 $ 
//
//-----------------------------------------------------------------------------
//
// Charger 3-Step Functionality
//
//	NOTES: 
//	1)	The charger 'algorithm' code was broken into separate functions for 
//		Volta and 'Generic' battery recipes. Volta uses a Li-Ion battery, but
//		also may accommodate customizations specific to Volta. Hence, the Li-Ion
//		code may not be generic. Therefore, it is maintained as a separate file
//		for now. 
//	
//		The defines here allow both source modules charger_liion.c and 
//		charger_3step.c to be included in the project... until a better 
//		method is devised.
//
//	2)	Current Limiting:
//		The following affect Current Limits.  The lowest value has priority.
//
//		Branch Circuit Rating (BCR) - This is the maximum AC Current budget for 
//		the charger.
//		Values configured via CAN are saved in Chgr.config.bcr_int8
//		Values configured via a remote display will overwrite the values saved
//		in Chgr.config.bcr_int8.
//		Values saved in Chgr.config.bcr_int8 are persisted in NVM:
//		g_nvm.settings.chgr.config.bcr_int8.
//		On startup, the value saved in NVM will be used to initialize 
//		Chgr.config.bcr_int8.
//
//		Chgr.config.amps_limit - This is the maximum AC Current that the 
//		charger is allowed to draw. This is platform/model dependent. 
//
//		Chgr.status.amp_limit is the acting limit.  It reflects the lowest 
//		limiting value.  Firmware charging decisions are based upon 
//		Chgr.status.amp_limit.
//
//-----------------------------------------------------------------------------

// -------
// headers
// -------
#include "options.h"    // must be first include
#include "analog.h"
#include "batt_temp.h"
#include "charger.h"
#include "nvm.h"
#include "pwm.h"

// -------------------------
// File optionally compiled
// -------------------------
#ifdef OPTION_CHARGE_3STEP

#ifndef WIN32
 #pragma message "standard battery <charger_3step.c>" // show selection during compile time
#endif

// --------------------------
// Conditional Debug Compiles
// --------------------------

// comment out flag to not use it
//  #define  DEBUG_SHOW_CHGR         1
//  #define  DEBUG_SHOW_CHGR_TIMERS	 1
#define  DEBUG_USE_VERBOSE      1
#define  DEBUG_CHGR_STATE       1   // Need to set Code=Large to compile; Show Charger State Transitions ###
//  #define  DEBUG_SHOW_BATT_RECIPE 1
// -----------------------------
// Auto Removal for build types
// -----------------------------
#if OPTION_NO_CONDITIONAL_DBG
  #undef DEBUG_SHOW_CHGR
  #undef DEBUG_SHOW_CHGR_TIMERS	
  #undef DEBUG_USE_VERBOSE
  #undef DEBUG_CHGR_STATE
  #undef DEBUG_SHOW_BATT_RECIPE
#endif	


//------------
// local data
//------------
static int8_t          _ChargeTimerEnable = 0;
static CHARGER_STATE_t charger_state         = CS_INITIAL;
static CHARGER_STATE_t charger_voltage_state = CS_INITIAL; // state to return to after an interruption

//-----------------------------------------------------------------------------
// return charger state
CHARGER_STATE_t chgr_GetState() // this needs to be duplicated in lion ### or moved to charger.c
{
    return(charger_state);
}

//-----------------------------------------------------------------------------
CHARGER_STATE_t chgr_InitChargerState()  // this needs to be duplicated in lion ### or moved to charger.c
{
    return(charger_voltage_state);
}


//-----------------------------------------------------------------------------
//              D E B U G G I N G      R O U T I N E S
//-----------------------------------------------------------------------------

static const char* GetChgrStateStr(int16_t state)
{
    static const char* _chgr_state_str[NUM_CHARGER_STATE_t] =
    {
    	// NOTE: must mirror CHARGER_STATE_t
        "CS_INVALID"     ,  
        "CS_INITIAL"     ,         
        "CS_SOFT_START"  ,      
        "CS_LOAD_MGMT"   ,       
        "CS_CONST_CURR"  ,      
        "CS_CONST_VOLT"  ,      
        "CS_FLOAT"       ,           
        "CS_WARM_BATT"   ,       
        "CS_SOFT_STOP"   ,       
        "CS_SHUTDOWN"    ,        
        "CS_EQUALIZE"    ,		
        "CS_MONITOR"     ,			
    };
    return(_chgr_state_str[state]);
}

//-----------------------------------------------------------------------------
#ifndef DEBUG_CHGR_STATE
  #define SHOW_CHGR_STATE(format, ...) { } //  do nothing
#else
  #ifndef WIN32
	#pragma message "DEBUG_CHGR_STATE flag enabled"
  #endif
  #define SHOW_CHGR_STATE(format, ...) LOG(SS_CHG, SV_DBG, format, ##__VA_ARGS__)
#endif

//-----------------------------------------------------------------------------
#ifndef DEBUG_SHOW_CHGR	       
  #define _chgr_show_debug() { } // do nothing
#else
  #ifndef WIN32
	#pragma message "DEBUG_SHOW_CHGR flag enabled"
  #endif

//-----------------------------------------------------------------------------
static void _chgr_show_debug(void)
{
    static int16_t msec_count = 0;
    static const char* _chgr_state_str[NUM_CHARGER_STATE_t] =
    {
        // NOTE: must mirror CHARGER_STATE_t
    	"NOT_SET"   , 
    	"INITIALIZE",     
    	"SOFT_START",  
    	"LOAD_MGMT"	,   
    	"CONST_CURR",  
    	"CONST_VOLT",  
    	"FLOAT"		,       
    	"WARM_BATT"	,   
    	"SOFT_STOP"	,   
    	"SHUTDOWN"	,    
    	"EQUALIZE"	,	
    	"MONITOR"				
    };
    static const char* _chgr_eq_state_str[NUM_CHARGER_EQ_STATE_t] =
    {
    	// NOTE: must mirror CHARGER_EQ_STATE_t
        "INACTIVE"  ,
        "PRECHARGE" ,
        "ACTIVE"    ,
        "COMPLETE",
    };
	static CHARGER_EQ_STATE_t last_eq_status = (CHARGER_EQ_STATE_t)-1;
	static CHARGER_STATE_t    last_state     = (CHARGER_STATE_t)-1;
	static uint16_t last_chgr_status  = 0;
	static uint16_t last_chgr_status2 = 0;
	static uint16_t last_chgr_error   = 0;
	
    //  limit these messages to once-per-second.
    if(--msec_count <= 0)
    {
        msec_count = 1000;
        
        //  NOTE: The only purpose of the flag 'chgr_over_curr' is for the Charger 
        //  ISR to tell the high-level code that we tripped the over-current test.
        //  The over-current test continuously tests the instantaneous current.
        //  If the current exceeds a threshold, the PWM duty-cycle is reset.
        //  This is not the Peak-Rectification test!
        if(chgr_over_curr) 
        { 
            chgr_over_curr = 0;
            LOG(SS_CHG, SV_ERR, "*** chgr_over_curr - ISR duty-cycle reset ");
        }
        
        //  NOTE: The only purpose of the flag 'chgr_over_volt' is for the Charger 
        //  ISR to tell the high-level code that we tripped the over-voltage test.
        //  The over-voltage test continuously tests the voltage averaged over one-
        //  cycle.  If the voltage exceeds a threshold, the PWM duty-cycle is reset.
        if(chgr_over_volt) 
        {
            chgr_over_volt = 0;
            LOG(SS_CHG, SV_ERR, "*** chgr_over_volt - ISR duty-cycle reset" );
        }
    }
	
	if(last_state != ChgrState())
	{
		LOG(SS_CHG, SV_INFO, "CHGR STATE: %s->%s ", 
			_chgr_state_str[last_state], _chgr_state_str[ChgrState()]);
		last_state = ChgrState();
	}

	if(last_eq_status != (CHARGER_EQ_STATE_t)Chgr.status.eq_status)
	{
		LOG(SS_CHG, SV_INFO, "CHGR EQ %s --> %s ", 
            _chgr_eq_state_str[last_eq_status], 
            _chgr_eq_state_str[Chgr.status.eq_status]);
		last_eq_status = Chgr.status.eq_status;
	}

	if((last_chgr_status != (CHARGER_EQ_STATE_t)Chgr.status.all_flags) || 
		(last_chgr_status2 != Chgr.status.all_flags2))
	{
		last_chgr_status = Chgr.status.all_flags;
		last_chgr_status2 = Chgr.status.all_flags2;
		LOG(SS_SYS, SV_INFO, "Charger Status: %04X %04X", Chgr.status.all_flags2, Chgr.status.all_flags);
	  #ifdef DEBUG_USE_VERBOSE
		if(IsChgrActive()           ) LOG(SS_SYS, SV_INFO, " charger_active"  );
		if(IsChgrOutputting()       ) LOG(SS_SYS, SV_INFO, " output_active"   );
		if(IsChgrWarmBatt()         ) LOG(SS_SYS, SV_INFO, " batt_warm"	      );
		if(IsChgrCvTimeout()        ) LOG(SS_SYS, SV_INFO, " cv_timeout"	  );
		if(IsChgrCvRocTimeout()     ) LOG(SS_SYS, SV_INFO, " cv_roc_timeout"  );
		if(IsChgrFloatTimeout()     ) LOG(SS_SYS, SV_INFO, " float_timeout"   );
		if(IsChgrSsCCLmTimeout()    ) LOG(SS_SYS, SV_INFO, " ss_cc_lm_timeout");
		if(IsChgrBattOverTemp()     ) LOG(SS_SYS, SV_INFO, " batt_ot"		  );
		if(IsChargingComplete()     ) LOG(SS_SYS, SV_INFO, " charge_complete" );
		if(IsChgrEqualizeTimeout()  ) LOG(SS_SYS, SV_INFO, " eq_timeout"      );
		if(IsChgrOverCurrDetected() ) LOG(SS_SYS, SV_INFO, " oc_detected"     );
	  #endif
	}
	
	if( last_chgr_error != Chgr.error.all_flags)
	{
		last_chgr_error = Chgr.error.all_flags;
		LOG(SS_SYS, SV_ERR, "Charger ERROR: %04X", Chgr.error.all_flags);
      #ifdef DEBUG_USE_VERBOSE
		if(IsChgrLowBattShutdown() ) LOG(SS_SYS, SV_INFO, " batt_lo_volt_shutdown");
		if(IsChgrHighBattShutdown()) LOG(SS_SYS, SV_INFO, " batt_hi_volt_shutdown");
		if(IsChgrOverCurrShutdown()) LOG(SS_SYS, SV_INFO, " oc_shutdown"          );
		if(IsChgrCcTimeoutErr()    ) LOG(SS_SYS, SV_INFO, " cc_timeout"           );
	  #endif
	}
}
#endif 	//	DEBUG_SHOW_CHGR	       

//-----------------------------------------------------------------------------
#ifndef DEBUG_SHOW_CHGR_TIMERS	       
  #define _chgr_show_timers() { } // do nothing
#else
  #ifndef WIN32
	#pragma message "DEBUG_SHOW_CHGR_TIMERS flag enabled"
  #endif

static void _chgr_show_timers(void)  
{
    LOG_TIMER_START();

	LOG(SS_CHG, SV_INFO, "TIMERS (minutes):");
	LOG(SS_CHG, SV_INFO, "\tcc_timer_amp  %ld", Chgr.status.cc_timer_amp_minutes    );
	LOG(SS_CHG, SV_INFO, "\tcc_elapsed     %d",  Chgr.status.cc_elapsed_minutes     );
	LOG(SS_CHG, SV_INFO, "\tcv_timer       %d",  Chgr.status.cv_timer_minutes       );
	LOG(SS_CHG, SV_INFO, "\tcv_roc_timer   %d",  Chgr.status.cv_roc_timer_minutes   );
//	LOG(SS_CHG, SV_INFO, "\tss_cc_lm_timer %d",  Chgr.status.ss_cc_lm_timer_minutes );
	LOG(SS_CHG, SV_INFO, "\tcc_cv_timer    %d",  Chgr.status.cc_cv_timer_minutes    );
	LOG(SS_CHG, SV_INFO, "\tfloat_timer    %d",  Chgr.status.float_timer_minutes    );
	LOG(SS_CHG, SV_INFO, "\teq_timer       %d",  Chgr.status.eq_timer_minutes       );

    LOG_TIMER_END();
}
#endif // DEBUG_SHOW_CHGR_TIMERS

#ifndef DEBUG_SHOW_BATT_RECIPE
  #define _chgr_show_battery_recipe() { } // do nothing
#else
  #ifndef WIN32
	#pragma message "DEBUG_SHOW_BATT_RECIPE: _chgr_show_battery_recipe - for debugging"
  #endif
  
static void _chgr_show_battery_recipe(void)
{
    char buf1[10];
    
//    LOG(SS_CHG, SV_INFO, "    : %d ", Chgr.config.battery_recipe.);

    LOG(SS_CHG, SV_INFO, "Battery Recipe");
    LOG(SS_CHG, SV_INFO, "    Type = %d",                   Chgr.config.battery_recipe.batt_type);
    //  is_custom;                  // 0=no, 1=custom battery recipe (allows changing values)
    // NOT USED  cc_max_amp_hours;           // Constant-Current Timeout	        (amp-hours)

    // CV (Constant-Voltage) Timeout (long) (minutes)  
    LOG(SS_CHG, SV_INFO, "    cv_timeout_minutes : %d ", Chgr.config.battery_recipe.cv_timeout_minutes);
    // CV-to-Float Rate-Of-Change Standard-Deviation Threshold (A2D counts ?)  
    LOG(SS_CHG, SV_INFO, "    cv_roc_sdev_threshold: %d ", Chgr.config.battery_recipe.cv_roc_sdev_threshold);
    // CV-to-Float Rate-of-Change   
    LOG(SS_CHG, SV_INFO, "    cv_roc_timeout_minutes: %d ", Chgr.config.battery_recipe.cv_roc_timeout_minutes);
    // Float Timeout (minutes)
    LOG(SS_CHG, SV_INFO, "    float_timeout_minutes: %d ", Chgr.config.battery_recipe.float_timeout_minutes);
    // Equalization Timeout (duration)  (minutes)
    LOG(SS_CHG, SV_INFO, "    eq_timeout_minutes: %d ", Chgr.config.battery_recipe.eq_timeout_minutes);
    
    //  eq_volts_max;        	    // Equalization Maximum Voltage     (adc)   
    //  eq_vsetpoint;        	    // Equalization Setpoint Voltage    (adc) (-1 = disabled)
    //  cv_volts_max;               // CV Maximum Voltage    		    (adc)   
    //  cv_vsetpoint;               // CV Setpoint Voltage              (adc)
    //  float_volts_max;            // Float Maximum Voltage 		    (adc)
    //  float_vsetpoint;            // Float Setpoint Voltage 			(adc)
    //  float_vthreshold;           // Float (init) Threshold Voltage   (adc)
    
    //  // Warm Batt to Normal   (Celsius)
    //  LOG(SS_CHG, SV_INFO, "    batt_temp_warm_recov: %d ", Chgr.config.battery_recipe.batt_temp_warm_recov);
    //  // Normal to Warm Batt   (Celsius)
    //  LOG(SS_CHG, SV_INFO, "    batt_temp_warm_thres: %d ", Chgr.config.battery_recipe.batt_temp_warm_thres);
    //  // Shutdown to Warm Batt (Celsius)
    //  LOG(SS_CHG, SV_INFO, "    batt_temp_shutdown_recov: %d ", Chgr.config.battery_recipe.batt_temp_shutdown_recov);
    //  // Warm Batt to Shutdown (Celsius)
    //  LOG(SS_CHG, SV_INFO, "    batt_temp_shutdown_thres: %d ", Chgr.config.battery_recipe.batt_temp_shutdown_thres);
    }
#endif // DEBUG_SHOW_BATT_RECIPE




//-----------------------------------------------------------------------------
//                 U T I L I T Y      R O U T I N E S
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
//  Rate-Of-Change
//-----------------------------------------------------------------------------
//	'_update_roc_timer()' is called from 'chgr_TimerOneMinuteUpdate()' while the 
//	charger is in CV state.
//
//	We need to average IMeas over the last minute.  IMeas (RMS) is updated each
//	zero-crossing.  
//-----------------------------------------------------------------------------

void _update_roc_timer(int8_t reset)
{
	//	NOTE: WACr statistics are updated once every 68.27 seconds. However, 
	//	this function is called once-per-minute. It should work ... OK. 
	//	TODO: In a future revision attempt to use a running value for statistics.
		
	if(reset)
	{
		Chgr.status.cv_roc_timer_minutes = Chgr.config.battery_recipe.cv_roc_timeout_minutes;
		Chgr.status.cv_roc_timeout = 0;
		return;
	}
	
	if(IsChgrCvRocTimeout()) return;	
	
	if(WACr_stats.dev <= Chgr.config.battery_recipe.cv_roc_sdev_threshold)  
    {
		if(--Chgr.status.cv_roc_timer_minutes <= 0)
		{
			Chgr.status.cv_roc_timeout = 1;
		}
	}

    LOG(SS_CHG, SV_INFO, "CV ROC: sdev_threshold=%d, sdev=%d, roc_timer=%d", 
        Chgr.config.battery_recipe.cv_roc_sdev_threshold, WACr_stats.dev, 
        Chgr.status.cv_roc_timer_minutes);
}


//-----------------------------------------------------------------------------
//	chgr_ChargeCycleReset() is called from the MainControl when starting a new 
//	charge-cycle. In general, timers are implemented as count-down, and initiate 
//	some action when they expire (0 = expired).  So they get initialized here to
//	a timeout value.
//-----------------------------------------------------------------------------

#define CHGR_SSCCLM_TIMEOUT_MINUTES	((int16_t)(12*60))
#define CHGR_CCCV_TIMEOUT_MINUTES	((int16_t)(12*60))

void chgr_ChargeCycleReset(void)
{
	//	reset chgr_Driver()'s internal state-machine 
	chgr_Driver(1);

	// reset so charger retests VBatt condition
	charger_voltage_state = CS_INITIAL;

	//	Initialize charger-related timer variables.
    Chgr.status.cc_timer_amp_minutes = __builtin_mulss(Chgr.config.battery_recipe.cc_max_amp_hours, (int16_t)60);
    Chgr.status.cc_elapsed_minutes   = 0;		// counts up.
	Chgr.status.cv_timer_minutes     = Chgr.config.battery_recipe.cv_timeout_minutes;
	
    Chgr.status.cv_roc_timer_minutes   = Chgr.config.battery_recipe.cv_roc_timeout_minutes;
    Chgr.status.float_timer_minutes    =  Chgr.config.battery_recipe.float_timeout_minutes;
	Chgr.status.ss_cc_lm_timer_minutes = CHGR_SSCCLM_TIMEOUT_MINUTES;
	Chgr.status.cc_cv_timer_minutes    =  CHGR_CCCV_TIMEOUT_MINUTES;
	//	TODO: If Equalization is disabled, make this timer zero?
	//	TODO: If eq_timer is -1 do we disable Equalization?
	Chgr.status.eq_timer_minutes = Chgr.config.battery_recipe.eq_timeout_minutes;
	
	//	Initialize (reset) charger-related timer flags.
    Chgr.status.cv_timeout 		 = 0; 
    Chgr.status.cv_roc_timeout 	 = 0; 
    Chgr.status.float_timeout 	 = 0; 
	Chgr.status.ss_cc_lm_timeout = 0;
	Chgr.status.cc_cv_timeout	 = 0;
	Chgr.status.charge_complete  = 0;

	//	Reset charger errors
    Chgr.error.cc_timeout 		 = 0;  
}

//-----------------------------------------------------------------------------
//          O N E    M I N U T E    U P D A T E    R O U T I N E S
//-----------------------------------------------------------------------------

INLINE void _update_cc_cv_timer(void)	
{ 
	if(--Chgr.status.cc_cv_timer_minutes <= 0)
	{
		Chgr.status.cc_cv_timer_minutes = 0;
		Chgr.status.cc_cv_timeout = 1;
	}
}

//------------------------------------------------------------------------------
// called once-per-minute from the main-loop.
void chgr_TimerOneMinuteUpdate(void)
{
  #if defined(OPTION_DISABLE_CHARGE_TIMER_SHUTDOWN)
    Chgr.status.ss_cc_lm_timeout = 0;
    Chgr.status.cc_cv_timeout    = 0;
    Chgr.status.cv_timeout       = 0;
    Chgr.status.float_timeout    = 0;
    Chgr.status.eq_timeout       = 0;
    Chgr.error.cc_timeout        = 0;
	
  #else
  
	static int16_t _charge_reset_timer = 2;
	//	If the AC-Line drops out for more than 2-minutes, then reset the Charge-
	//	Cycle/Charge-Timers
	if(IsAcLineQualified()) _charge_reset_timer = 2;	//	Minutes
		
	if(_charge_reset_timer > 0)
	{
		_charge_reset_timer--;
	}
	else
	{
		_charge_reset_timer = 0;
		chgr_ChargeCycleReset();
	}
	
    if(!IsChgrActive())
    {
		return;
	}

	switch(ChgrState())
	{
	case CS_CONST_CURR :
		_update_cc_cv_timer();
		++Chgr.status.cc_elapsed_minutes;
		break;
		
	case CS_CONST_VOLT :
		_update_roc_timer(0);
		_update_cc_cv_timer();
        if(!IsChgrCvTimeout() && _ChargeTimerEnable)
        {
            if(--Chgr.status.cv_timer_minutes <= 0)
            {
                Chgr.status.cv_timer_minutes = 0;
                Chgr.status.cv_timeout = 1;
            }
        }
		break;
		
	case CS_WARM_BATT :
        //  If we have not completed CV-State, then don't run the Float-Timer.
        if(!Chgr.status.cv_timeout && !Chgr.status.cv_roc_timeout) break;
		// Fall thru

	case CS_FLOAT :
        if(!IsChgrFloatTimeout() && _ChargeTimerEnable)
        {
            if(--Chgr.status.float_timer_minutes <= 0)
            {
                Chgr.status.float_timer_minutes = 0;
                Chgr.status.float_timeout = 1;
            }
        }
		break;
		
	case CS_EQUALIZE :
        if(!IsChgrEqualizeTimeout() && _ChargeTimerEnable)
        {
            if(--Chgr.status.eq_timer_minutes <= 0)
            {
                Chgr.status.eq_timer_minutes = 0;
                Chgr.status.eq_timeout = 1;
            }
        }
		break;
        
    default:
        break;
	} // switch
	
  #endif	//	OPTION_DISABLE_CHARGE_TIMER_SHUTDOWN
  
	_chgr_show_timers();
}

//-----------------------------------------------------------------------------
// return the voltage setpoint based upon the mode the charger is in
// applies temperature compensation

uint16_t chgr_GetVoltSetpoint()
{
	uint16_t volt_setpoint;
    CHARGER_STATE_t state = charger_voltage_state; // use return state here

    if (charger_state == CS_WARM_BATT) state = CS_FLOAT; // use float when ever in warm batt mode per Gary 2018-03-20
	switch (state)
	{
	default:
	case CS_FLOAT:      
		volt_setpoint = Chgr.config.battery_recipe.float_vsetpoint + BattTemp_VBattCompA2d();	
		if (volt_setpoint > Chgr.config.battery_recipe.float_volts_max) // dont exceed max
		    volt_setpoint = Chgr.config.battery_recipe.float_volts_max;
		break;

	case CS_CONST_VOLT:
		volt_setpoint = Chgr.config.battery_recipe.cv_vsetpoint + BattTemp_VBattCompA2d();
		if (volt_setpoint > Chgr.config.battery_recipe.cv_volts_max) // dont exceed max
			volt_setpoint = Chgr.config.battery_recipe.cv_volts_max;
		break;

	case CS_EQUALIZE:
		volt_setpoint = Chgr.config.battery_recipe.eq_vsetpoint + BattTemp_VBattCompA2d();
		if (volt_setpoint > Chgr.config.battery_recipe.eq_volts_max) // dont exceed max
		    volt_setpoint = Chgr.config.battery_recipe.eq_volts_max;
		break;
	} // switch
	return(volt_setpoint);
}

//-----------------------------------------------------------------------------
//       C H A R G E R    D R I V E R     S T A T E    M A C H I N E
//-----------------------------------------------------------------------------

#define  REDUCE_DUTY()  { _comp_q16 = (_comp_q16 - (_comp_q16/4)); }  // reduce duty cycle by 25% of current value 


//-----------------------------------------------------------------------------
//	RMS values are updated once-per-cycle on the positive to negative zero-crossing. 
//
//	This code uses the flag 'chgr_pos_neg_zc' which is set in _chgr_pwm_isr() 
//	on the positive-to-negative zero-crossing and cleared in this code.
//
//	It is possible that we lose the AC input, or open the Charger Relay but 
//	still need this code to run. So a backup counter is used.
//
//-----------------------------------------------------------------------------

extern int16_t xfer_relay_oc_ignore_timer_msec;

// called once per millisecond
void chgr_Driver(int8_t reset)
{
    #define DMSG_INTERVAL_COUNT	((int16_t)(60))	//	DEBUG 1-sec (60Hz)

    #define ILIMIT_POS_DEADBAND  (12)  // ### should use cal here
    #define ILINE_NEG_DEADBAND   (18)  // ### should use cal here

	static CHARGER_STATE_t last_state = (CHARGER_STATE_t)-1;
    static int16_t _msg_tic = 0; // DEBUG
    int8_t _msg_flag        = 0; // DEBUG
	int16_t p_adj           = 0; // PROPORTIONAL adjustment
		
    #define STABILITY_COUNT_LIMIT	5
    static int16_t stability_count = 0;

	#define RESTART_TIMEOUT_SECONDS	(30*60)		// 30secs
    static int16_t restart_timer = RESTART_TIMEOUT_SECONDS; 

    static int16_t zc_bkup_tic = 0;
    int8_t zc_flag  = 0;

    if(reset)
    {
        //  NOTE: This resets the state machine, not the charge timers
		_update_roc_timer(1);	//	reset the ROC timer
        charger_state = CS_INITIAL;
        return;
    }

    // Wait for 200 msec delay to expire (after pulling in the charger relay).
    // High current transients are occuring at this time.
    if (xfer_relay_oc_ignore_timer_msec != 0) return;

	_chgr_show_debug();

	//	Update Chgr.status 
    SetChgrState(charger_state);
    Chgr.status.amps_setpoint = ChgrCfgMaxAcCurrA2D();		//	Default
	
	//	When we change state. If we change state, then expire _msg_tic, 
	//	so diagnostic messages are output right away.
	
	if(last_state != ChgrState())
	{
		last_state = ChgrState();
		_msg_tic = DMSG_INTERVAL_COUNT;
	}
	
    //-------------------------------------------------------------------------
    //  The flag 'zc_flag' is used to synchronize control functions that need 
    //  o run once-per-zero-crossing.  It implements a backup counter, in case 
    //  AC goes away and we don't have a zero-crossing.  The backup counter 
    //  assures that the control will run at least once every 18-milliseconds
    //-------------------------------------------------------------------------

	#define ZC_BKUP_TIC_COUNT	((int16_t)(18))
	++zc_bkup_tic;
	if(chgr_pos_neg_zc || (zc_bkup_tic >= ZC_BKUP_TIC_COUNT))
	{
		chgr_pos_neg_zc = 0;
		zc_bkup_tic = 0;
		zc_flag = 1;
		
		if(++_msg_tic >= DMSG_INTERVAL_COUNT)
		{
			_msg_tic  = 0;
			_msg_flag = 1;
		}
		else
		{
			_msg_flag = 0;
		}
	}
	else
	{
		zc_flag = 0;
	}

	if(!Device.config.batt_temp_sense_present)
	{
		Chgr.status.batt_ot = 0;
		Chgr.status.batt_warm = 0;
	}
	else
	{
		//	Configure temperature-related charger status flags here:
		if(Device.status.batt_temp >= Chgr.config.battery_recipe.batt_temp_shutdown_thres)
		{
			Chgr.status.batt_ot = 1;
		}
		else if(Device.status.batt_temp >= Chgr.config.battery_recipe.batt_temp_warm_thres)
		{
			Chgr.status.batt_warm = 1;
		}
		
		if(Device.status.batt_temp <= Chgr.config.battery_recipe.batt_temp_shutdown_recov)
		{
			Chgr.status.batt_ot = 0;
		}
		if(Device.status.batt_temp <= Chgr.config.battery_recipe.batt_temp_warm_recov)
		{
			Chgr.status.batt_warm = 0;
		}
	}
	
  #if defined(OPTION_CHARGER_CV_MODE_ONLY)
	if((CS_SOFT_STOP != charger_state) && (CS_SHUTDOWN != charger_state))
	{
		charger_state = CS_CONST_VOLT;
	}
  #endif	//	OPTION_CHARGER_CV_MODE_ONLY
	
	// branch to current charger state
    switch(charger_state)
    {
    case CS_INITIAL :
        _chgr_DutyReset();
        chgr_pos_neg_zc = 0;
		
		Chgr.status.eq_status = IsChgrCfgEqRequest()?CS_EQ_PRECHARGE:CS_EQ_INACTIVE;
		
		if(!IsCycleAvgValid()) break;
		if(charger_voltage_state != CS_INITIAL)
		{
            charger_state = charger_voltage_state;
            SHOW_CHGR_STATE("CS: INITIAL->%s: (chgr_volt_state)", GetChgrStateStr(charger_state));
			break;
		}
        
        _chgr_show_battery_recipe();
        
        if(VBattCycleAvg() > Chgr.config.battery_recipe.float_vthreshold)
		{
			charger_state = charger_voltage_state = CS_FLOAT;
            SHOW_CHGR_STATE("CS: INITIAL->FLOAT:  VBatt(%u) > float_vthresh(%u))",
                   VBattCycleAvg(), Chgr.config.battery_recipe.float_vthreshold);
            break;
		}
        charger_state = charger_voltage_state = CS_CONST_VOLT;
        SHOW_CHGR_STATE("CS: INITIAL->CONST_VOLT:  VBatt(%u) <= float_vthresh(%u)", 
            VBattCycleAvg(), Chgr.config.battery_recipe.float_vthreshold);
        break;


    case CS_LOAD_MGMT :
        if(!zc_flag) break; // run once per zero crossing

		Chgr.status.eq_status = IsChgrCfgEqRequest() ? CS_EQ_PRECHARGE : CS_EQ_INACTIVE;
		
        //---------------------------------------------------------------------
        //  Control AC Current draw (measured as iLine) to stay within the
        //  current budget set by the Branch Circuit Rating (BCR).
        //
        //  NOTES:
        //   1) Additional loads may affect AC Current draw. Hence iLine is not
        //      solely the result of the charger.
        //   2) The LPC15 schematic identifies the line current sensor signal
        //      as I_LOAD_MGMT.
        //   3) The function _chgr_DutyAdjust() implements limits to prevent
        //      overflow and wrap-around from repeated increment or decrement.
        //---------------------------------------------------------------------

		Chgr.status.amps_setpoint = ChgrCfgBcrA2D();
		Chgr.status.volt_setpoint = chgr_GetVoltSetpoint();
		
        if (chgr_StopRequest)      
		{
			charger_state = CS_SOFT_STOP; 
			SHOW_CHGR_STATE("CS: LOAD_MGMT->SOFT_STOP: stopReq(%u)", chgr_StopRequest);
			break;
		}
        if(IsChgrBattOverTemp())  
		{
			charger_state = CS_SOFT_STOP; 
			SHOW_CHGR_STATE("CS: LOAD_MGMT->SOFT_STOP:  BattOT(%u)", IsChgrBattOverTemp());
			break;
		}
		if(ILineLongAvg() > (ChgrCfgBcrA2D() + LOAD_MGMT_RESET))
		{
			_chgr_DutyReset();
			break;
		}
        if(ILimitRMS() > (ChgrCfgMaxAcCurrA2D() + LOAD_MGMT_RESET))
		{
            REDUCE_DUTY();
			charger_state = CS_CONST_CURR; 
			SHOW_CHGR_STATE("CS: LOAD_MGMT->CONST_CURR:  ILimit(%u) >= MaxAcCurr(%u) + RESET(%u)", 
														 ILimitRMS(), ChgrCfgMaxAcCurrA2D(), LOAD_MGMT_RESET);
			break;
		}
		if(ILineLongAvg() > ChgrCfgBcrA2D())           
		{
			// regulate based on ILine current
            p_adj = (ChgrCfgBcrA2D() - LOAD_MGMT_DEADBAND) - ILineLongAvg(); // +/-
            if(p_adj >  10) p_adj =  10;
            if(p_adj < -10) p_adj = -10;
			_chgr_DutyAdjust(p_adj);
			break;
		}
        if(ILimitRMS() > (ChgrCfgMaxAcCurrA2D() + ILIMIT_POS_DEADBAND))
		{
			charger_state = CS_CONST_CURR; 
			SHOW_CHGR_STATE("CS: LOAD_MGMT->CONST_CURR:  ILimit(%u) > MaxAcCurr(%u) + DB(%u)", ILimitRMS(), ChgrCfgMaxAcCurrA2D(), ILIMIT_POS_DEADBAND);
			break;
		}
        if(IsChgrWarmBatt())	    
		{
			charger_state = CS_WARM_BATT;	
			SHOW_CHGR_STATE("CS: LOAD_MGMT->WARM_BATT: WarmBatt(%u)", IsChgrWarmBatt());
			break;
		}
        if(VBattCycleAvg() > Chgr.status.volt_setpoint)
		{
			charger_state = charger_voltage_state;
			SHOW_CHGR_STATE("CS: LOAD_MGMT->%s:  VBatt(%u) >= volt_setpoint(%u))",
					GetChgrStateStr(charger_state), VBattCycleAvg(), Chgr.status.volt_setpoint);
			break;
		}
		if (ILineLongAvg() < (ChgrCfgBcrA2D() - LOAD_MGMT_DEADBAND))
		{
			// regulate based on ILine current
            p_adj = (ChgrCfgBcrA2D() - LOAD_MGMT_DEADBAND) - ILineLongAvg(); // +/-
            if(p_adj >  10) p_adj =  10;
            if(p_adj < -10) p_adj = -10;
			_chgr_DutyAdjust(p_adj);
			break;
		}
        break;


    case CS_CONST_CURR :        //  CONSTANT CURRENT / CURRENT-LIMIT
        if(!zc_flag) break; // run once per zero crossing

		Chgr.status.eq_status = IsChgrCfgEqRequest()?CS_EQ_PRECHARGE:CS_EQ_INACTIVE;

		// ------------------------------------------------------------------------
        //  Provide Maximum Allowed (DC) Current to the battery.
        //  Maximum charge current is model-specific.
        //
        //  Notes:
        //   1) DC output current is not measured directly, but uses the
        //      Primary AC RMS current as a proxy for DC current.
        //   2) Primary current is measured as the signal iMeasure. NP hardware
        //      includes an additional signal iLimit, which applies gain to the
        //      iMeasure signal accommodating manual adjustment at manufacture.
        //      This control will use iLimit as feedback.
        //   3) The LPC15 does not include the iLimit signal. Instead, iMeasure
        //      is copied over the iLimit data, so that this code may be model-
        //      agnostic. Output current settings may be adjusted by changing
        //      the value of CC_CURRENT_LIMIT.
		// ------------------------------------------------------------------------

		Chgr.status.volt_setpoint = chgr_GetVoltSetpoint();
		
        if(chgr_StopRequest) 					
		{
			charger_state = CS_SOFT_STOP; 
			SHOW_CHGR_STATE("CS: CONST_CURR->SOFT_STOP:  stopReq(%u)", chgr_StopRequest);
			break;
		}
        if(IsChgrBattOverTemp())			
		{
			charger_state = CS_SOFT_STOP;	
			SHOW_CHGR_STATE("CS: CONST_CURR->SOFT_STOP:  BattOT(%u)",  IsChgrBattOverTemp());
			break;
		}
		if(ILineLongAvg() > (ChgrCfgBcrA2D() + LOAD_MGMT_RESET))
		{
			_chgr_DutyReset();
			charger_state = CS_LOAD_MGMT;	
			SHOW_CHGR_STATE("CS: CONST_CURR->LOAD_MGMT:  Iline(%u) > BCR(%u) +RESET(%u)", 
														 ILineLongAvg(), ChgrCfgBcrA2D(), LOAD_MGMT_RESET);
			break;
		}
        if(ILimitRMS() > (ChgrCfgMaxAcCurrA2D() + LOAD_MGMT_RESET))
		{
            REDUCE_DUTY();
			break;
		}
        if(ILimitRMS() < ChgrCfgMaxAcCurrA2D())
        {
            if(ILineLongAvg() > (ChgrCfgBcrA2D() + LOAD_MGMT_DEADBAND))
		    {
		    	charger_state = CS_LOAD_MGMT;	
		    	SHOW_CHGR_STATE("CS: CONST_CURR->LOAD_MGMT:  ILine(%u) > BCR(%u) + DEADBAND(%u)",
		    												 ILineLongAvg(), ChgrCfgBcrA2D(), LOAD_MGMT_DEADBAND);
		    	break;
		    }
            if(IsChgrWarmBatt())			    
		    {
		    	charger_state = CS_WARM_BATT;	
			    SHOW_CHGR_STATE("CS: CONST_CURR->WARM_BATT: WarmBatt(%u)", IsChgrWarmBatt());

		    	break;
		    }
            if(VBattCycleAvg() >= Chgr.status.volt_setpoint)	
		    {
		    	charger_state = charger_voltage_state;
                SHOW_CHGR_STATE("CS: CONST_CURR->%s: VBatt(%u) >= volt_setpoint(%u)", 
                    GetChgrStateStr(charger_state), VBattCycleAvg(), Chgr.status.volt_setpoint);
		    	break;
		    }
        }

        // ------------------------------------------------------------------------
        //  Regulate the current to the MAXIMUM allowed by the charger.  
        //  The maximum current allowed is set by ILimit.
        //  Prevent the output voltage from exceeding the maximum (recipe)
        //			
        //  The LPC does not have a Current Limit adjust potentiometer
        //  like the NP. Current Limit must be software-adjustable.
        //  On NP 'ILimit' is derived from 'IMeas'.  On LPC 'ILimit' is
        //  an alias for 'IMeas' - Therefore, we can use the IMeas 
        //  calibration values for conversion/display. For the LPC, we
        //  need the [CAN] interface to adjust 'Chgr config amps_limit'
        // ------------------------------------------------------------------------
		p_adj = ChgrCfgMaxAcCurrA2D() - ILimitRMS();
		if(p_adj > 10) p_adj =  10;
		_chgr_DutyAdjust(p_adj);
		  
		if(_msg_flag)
		{	
			// LOG(SS_CHG, SV_INFO, "CHGR CC - SP:%d, ILimit:%d ", ChgrCfgMaxAcCurrA2D(), ILimitRMS()); 
		}
        break;


    case CS_CONST_VOLT :    //  CONSTANT-VOLTAGE
        if(!zc_flag) break; // run once per zero crossing

		Chgr.status.eq_status = IsChgrCfgEqRequest()?CS_EQ_PRECHARGE:CS_EQ_INACTIVE;

    #if defined(OPTION_CHARGER_CV_MODE_ONLY)
	  #if (OPTION_DCIN == DCIN_12V)
		Chgr.status.volt_setpoint = (VBATT_VOLTS_ADC(12.60));
	    if(chgr_StopRequest) 					{ charger_state = CS_SOFT_STOP; }

	  #else
		//	This is an error for now because we've not determined 
		//	settings for other voltages - for U.L. Testing.
		#error OPTION_CHARGER_CV_MODE_ONLY currently supports 12V only
	  #endif
	  
	#else

		Chgr.status.volt_setpoint = chgr_GetVoltSetpoint();

        if(chgr_StopRequest) 					
		{
			charger_state = CS_SOFT_STOP; 
			SHOW_CHGR_STATE("CS: CONST_VOLT->SOFT_STOP:  stopReq(%u)", chgr_StopRequest);
			break;
		}
		if(IsChgrBattOverTemp())			
		{
			charger_state = CS_SOFT_STOP; 
			SHOW_CHGR_STATE("CS: CONST_VOLT->SOFT_STOP:  BattOT(%u)",  IsChgrBattOverTemp());
			break;
		}
        //  If current is too high, then reset the duty-cycle
        if(ILineLongAvg() > (ChgrCfgBcrA2D() + LOAD_MGMT_RESET))
        {
            _chgr_DutyReset();
		    charger_state = CS_LOAD_MGMT; 
            SHOW_CHGR_STATE("CS: CONST_VOLT->LOAD_MGMT:  ILine(%u) > BCR(%u) + RESET(%u)", 
			        									 ILineLongAvg(), ChgrCfgBcrA2D(), LOAD_MGMT_RESET);
		    break;
        }
        if (ILimitRMS() > (ChgrCfgMaxAcCurrA2D() + LOAD_MGMT_RESET))
        {
            REDUCE_DUTY();
			charger_state = CS_CONST_CURR; 
            SHOW_CHGR_STATE("CS: CONST_VOLT->CONST_CURR:  ILimit(%u) > MaxAcCurr(%u) + RESET(%u); Reduce Duty(%u)", ILimitRMS(), ChgrCfgMaxAcCurrA2D(), LOAD_MGMT_RESET, _comp_q16);   
			break;
		}
        if(ILineLongAvg() > (ChgrCfgBcrA2D() + LOAD_MGMT_DEADBAND))
		{
			charger_state = CS_LOAD_MGMT; 
            SHOW_CHGR_STATE("CS: CONST_VOLT->LOAD_MGMT:  ILine(%u) > BCR(%u) + DB(%u)", ILineLongAvg(), ChgrCfgBcrA2D(), LOAD_MGMT_DEADBAND);    
			break;
        }
        if(ILimitRMS() > (ChgrCfgMaxAcCurrA2D() + ILIMIT_POS_DEADBAND))
		{
			//  Current is too high
			charger_state = CS_CONST_CURR;
            SHOW_CHGR_STATE("CS: CONST_VOLT->CONST_CURR:  ILimit(%u) > MaxAcCurr(%u) + DB(%u))", ILimitRMS(), ChgrCfgMaxAcCurrA2D(), ILIMIT_POS_DEADBAND);
			break;
		}
        if(IsChgrWarmBatt())			    
		{
			charger_state = CS_WARM_BATT;	
			SHOW_CHGR_STATE("CS: CONST_VOLT->WARM_BATT:  WarmBatt(%u)", IsChgrWarmBatt());
			break;
		}
        if(IsChgrCvRocTimeout() || IsChgrCvTimeout())
		{
			charger_voltage_state = CS_FLOAT;
			charger_state         = CS_FLOAT;		
            SHOW_CHGR_STATE("CS: CONST_VOLT->FLOAT:  Cv(%u) or CvRoc(%u) Timeout)", IsChgrCvTimeout(), IsChgrCvRocTimeout());
			break;
		}
	  #endif	//	OPTION_CHARGER_CV_MODE_ONLY

		// regulate on VBatt
		if(VBattCycleAvg() > Chgr.status.volt_setpoint)
        {
			//  Too high
			p_adj = VBattCycleAvg() - Chgr.status.volt_setpoint; // positive
			if (p_adj > 30) p_adj = 30;
			// decrement duty cycle
            _chgr_DutyAdjust(-p_adj);
			stability_count++;
        }
        else if(VBattCycleAvg() < Chgr.status.volt_setpoint)
        {
			//  Too low
			p_adj = Chgr.status.volt_setpoint - VBattCycleAvg(); // positive
			if (p_adj > 30) p_adj = 30;
			// increment duty cycle
            _chgr_DutyAdjust(+p_adj);
			stability_count++;
        }
		else if(--stability_count < 0)
		{
			stability_count = 0;
		}
		
		if(stability_count > STABILITY_COUNT_LIMIT)
		{
			stability_count = STABILITY_COUNT_LIMIT;
			_ChargeTimerEnable = 0;
		}
		else
        {
            _ChargeTimerEnable = 1;
        }
        break;


    case CS_WARM_BATT : //  EXCEPTION STATE - CONSTANT-VOLTAGE
        if(!zc_flag) break; // run once per zero crossing
        
		Chgr.status.volt_setpoint = chgr_GetVoltSetpoint();

		// regulate on VBatt
		if(VBattCycleAvg() > Chgr.status.volt_setpoint)
        {
			//  Too high
			p_adj = VBattCycleAvg() - Chgr.status.volt_setpoint; // positive
			if (p_adj > 30) p_adj = 30;
			// decrement duty cycle
			_chgr_DutyAdjust(-p_adj);
			stability_count++;
        }
        else if(VBattCycleAvg() < Chgr.status.volt_setpoint)
        {
			//  Too low
			p_adj = Chgr.status.volt_setpoint - VBattCycleAvg();
			if (p_adj > 30) p_adj = 30;
			// increment duty cycle
			_chgr_DutyAdjust(+p_adj);
			stability_count++;
        }
		else if(--stability_count < 0)
		{
			stability_count = 0;
		}
		
		if(stability_count > STABILITY_COUNT_LIMIT)
		{
			stability_count = STABILITY_COUNT_LIMIT;
			_ChargeTimerEnable = 0;
		}
		else 
        {
            _ChargeTimerEnable = 1;
        }
		
        if (chgr_StopRequest) 		 		
		{
			charger_state = CS_SOFT_STOP;
			SHOW_CHGR_STATE("CS: WARM_BATT->SOFT_STOP: stopReq(%u)", chgr_StopRequest);
			break;
		}
        if(IsChgrBattOverTemp())			
		{
			charger_state = CS_SOFT_STOP;	
			SHOW_CHGR_STATE("CS: WARM_BATT->SOFT_STOP: BattOT(%u)", IsChgrBattOverTemp());
			break;
		}
        //  If current is too high, then reset the duty-cycle
        if(ILineLongAvg() > (ChgrCfgBcrA2D() + LOAD_MGMT_RESET))
        {
            _chgr_DutyReset();
	        charger_state = CS_LOAD_MGMT; 
            SHOW_CHGR_STATE("CS: WARM_BATT->LOAD_MGMT:  ILine(%u) > BCR(%u) + RESET(%u)", ILineLongAvg(), ChgrCfgBcrA2D(), LOAD_MGMT_RESET);
	        break;
        }
        if (ILimitRMS() > (ChgrCfgMaxAcCurrA2D() + LOAD_MGMT_RESET))
        {
            REDUCE_DUTY();
 			charger_state = CS_CONST_CURR; 
            SHOW_CHGR_STATE("CS: WARM_BATT->CONST_CURR:  ILimit(%u) > MaxAcCurr(%u) + RESET(%u)", ILimitRMS(), ChgrCfgMaxAcCurrA2D(), LOAD_MGMT_RESET);    
	        break;
        }
        if(ILineLongAvg() > (ChgrCfgBcrA2D() + LOAD_MGMT_DEADBAND))
		{
			charger_state = CS_LOAD_MGMT; 
            SHOW_CHGR_STATE("CS: WARM_BATT->LOAD_MGMT:  ILine(%u) > BCR(%u) + DB(%u)", ILineLongAvg(), ChgrCfgBcrA2D(), LOAD_MGMT_DEADBAND);    
			break;
        }
        if(ILimitRMS() > (ChgrCfgMaxAcCurrA2D() + ILIMIT_POS_DEADBAND))
		{
			charger_state = CS_CONST_CURR; 
            SHOW_CHGR_STATE("CS: WARM_BATT->CONST_CURR:  ILimit(%u) > MaxAcCurr(%u) + DB(%u)", ILimitRMS(), ChgrCfgMaxAcCurrA2D(), ILIMIT_POS_DEADBAND);
			break;
		}
        if(!IsChgrWarmBatt())			    
		{
			charger_state = charger_voltage_state;
			SHOW_CHGR_STATE("CS: WARM_BATT->%s:  WarmBatt(%u)", GetChgrStateStr(charger_state), IsChgrWarmBatt());
            break;
		}
        break;


    case CS_FLOAT :     //  CONSTANT-VOLTAGE (Setpoint lower than CV-Mode)
        if(!zc_flag) break; // run once per zero crossing
        
		Chgr.status.volt_setpoint = chgr_GetVoltSetpoint();
		
        if (chgr_StopRequest) 		 		
		{
			charger_state = CS_SOFT_STOP;
			SHOW_CHGR_STATE("CS: FLOAT->SOFT_STOP: stopReq(%u)", chgr_StopRequest);
			break;
		}
		if(IsChgrBattOverTemp())			
		{
			charger_state = CS_SOFT_STOP;	
			SHOW_CHGR_STATE("CS: FLOAT->SOFT_STOP: BattOT(%u)", IsChgrBattOverTemp());
			break;
		}
        //  If current is too high, then reset the duty-cycle
        if(ILineLongAvg() > (ChgrCfgBcrA2D() + LOAD_MGMT_RESET))
        {
            _chgr_DutyReset();
		    charger_state = CS_LOAD_MGMT; 
            SHOW_CHGR_STATE("CS: FLOAT->LOAD_MGMT:  ILine(%u) > BCR(%u) + RESET(%u)", ILineLongAvg(), ChgrCfgBcrA2D(), LOAD_MGMT_RESET);
		    break;
        }
        if (ILimitRMS() > (ChgrCfgMaxAcCurrA2D() + LOAD_MGMT_RESET))
        {
            REDUCE_DUTY();
		    charger_state = CS_CONST_CURR; 
            SHOW_CHGR_STATE("CS: FLOAT->CONST_CURR:  ILimit(%u) > MaxAcCurr(%u) + RESET(%u); Reduce Duty(%u)", ILimitRMS(), ChgrCfgMaxAcCurrA2D(), LOAD_MGMT_RESET, _comp_q16);    
		    break;
		}
        if(ILineLongAvg() > (ChgrCfgBcrA2D() + LOAD_MGMT_DEADBAND))
		{
			charger_state = CS_LOAD_MGMT; 
            SHOW_CHGR_STATE("CS: FLOAT->LOAD_MGMT:  ILine(%u) > BCR(%u) + DB(%u)", ILineLongAvg(), ChgrCfgBcrA2D(), LOAD_MGMT_DEADBAND);    
			break;
        }
        if(ILimitRMS() > (ChgrCfgMaxAcCurrA2D() + ILIMIT_POS_DEADBAND))
		{
			charger_state = CS_CONST_CURR; 
            SHOW_CHGR_STATE("CS: FLOAT->CONST_CURR:  ILimit(%u) > MaxAcCurr(%u) + DB(%u)", ILimitRMS(), ChgrCfgMaxAcCurrA2D(), ILIMIT_POS_DEADBAND);
			break;
		}
        if(IsChgrWarmBatt())			    
		{
			charger_state = CS_WARM_BATT;
			SHOW_CHGR_STATE("CS: FLOAT->WARM_BATT:  WarmBatt(%u)", IsChgrWarmBatt());
            break;
		}
		if(IsChgrFloatTimeout())
		{
			Chgr.status.charge_complete = 1;
			charger_state = CS_SOFT_STOP;
			SHOW_CHGR_STATE("CS: FLOAT->SOFT_STOP: FloatTimeout(%u) Charge Complete", IsChgrFloatTimeout());
			break;
		}

		// regulate on VBatt
		if(VBattCycleAvg() > Chgr.status.volt_setpoint)
        {
			//  Too high
			p_adj = VBattCycleAvg() - Chgr.status.volt_setpoint; // positive
			if (p_adj > 30) p_adj = 30;
			// decrement duty cycle
			_chgr_DutyAdjust(-p_adj);
			stability_count++;
        }
        else if(VBattCycleAvg() < Chgr.status.volt_setpoint)
        {
			//  Too low
			p_adj = Chgr.status.volt_setpoint - VBattCycleAvg();
			if (p_adj > 30) p_adj = 30;
			// increment duty cycle
			_chgr_DutyAdjust(+p_adj);
			stability_count++;
        }
		else if(--stability_count < 0)
		{
			stability_count = 0;
		}
		
		if(stability_count > STABILITY_COUNT_LIMIT)
		{
			stability_count = STABILITY_COUNT_LIMIT;
			_ChargeTimerEnable = 0;
		}
		else 
        {
            _ChargeTimerEnable = 1;
        }
        break;
	

    case CS_SOFT_STOP :
        if(!zc_flag) break; // run once per zero crossing

		Chgr.status.eq_status = IsChgrCfgEqRequest()?CS_EQ_PRECHARGE:CS_EQ_INACTIVE;

        //  decrement the charger PWM duty-cycle until minimum
		if(0 == _chgr_DutyAdjust(-100))    // 100 ~ 0.333% duty cycle
		{
            SetChgrNotOutputting();
	        if(chgr_StopRequest) 
			{ 
				chgr_StopRequest = 0; 
				charger_state = CS_SHUTDOWN;
				SHOW_CHGR_STATE("CS: SOFT_STOP->SHUTDOWN:  StopReq(%u)", chgr_StopRequest);
				break;
			}
          #if !defined(OPTION_CHARGER_CV_MODE_ONLY)
			if(IsChgrBattOverTemp())
			{
				charger_state = CS_MONITOR;
				SHOW_CHGR_STATE("CS: SOFT_STOP->MONITOR:  BattOT(%u)", IsChgrBattOverTemp());
				break;
			}
			if(IsChargingComplete())
			{
				charger_voltage_state = CS_MONITOR;
				charger_state         = CS_MONITOR;
				SHOW_CHGR_STATE("CS: SOFT_STOP->MONITOR:  ChargingComplete(%u)", IsChargingComplete());
				break;
			}
		  #endif	//	OPTION_CHARGER_CV_MODE_ONLY
		}
        break;

		
	case CS_EQUALIZE:
        if(!zc_flag) break; // run once per zero crossing

		Chgr.status.volt_setpoint = chgr_GetVoltSetpoint();

        if(chgr_StopRequest)
		{
			charger_state = CS_SOFT_STOP;
			SHOW_CHGR_STATE("CS: EQUALIZE->SOFT_STOP:  stopReq(%u)", chgr_StopRequest);
			break;
		}
		if(IsChgrBattOverTemp())
        {
            charger_state = CS_SOFT_STOP;
			SHOW_CHGR_STATE("CS: EQUALIZE->SOFT_STOP:  BattOT(%u)", IsChgrBattOverTemp());
			break;
        }
        //  If current is too high, then reset the duty-cycle
        if(ILineLongAvg() > (ChgrCfgBcrA2D() + LOAD_MGMT_RESET))
        {
            _chgr_DutyReset();
            charger_state = CS_LOAD_MGMT;
            SHOW_CHGR_STATE("CS: EQUALIZE->LOAD_MGMT:  ILine(%u) > BCR(%u) + RESET(%u)", 
									                   ILineLongAvg(), ChgrCfgBcrA2D(), LOAD_MGMT_RESET);
            break;
        }
        if(ILimitRMS() > (ChgrCfgMaxAcCurrA2D() + LOAD_MGMT_RESET)) 
        {
            REDUCE_DUTY();
			charger_state = CS_CONST_CURR;
            SHOW_CHGR_STATE("CS: EQUALIZE->CONST_CURR:  ILimit(%u) > MaxAcCurr(%u) + RESET(%u); Reduce Duty(%u)", ILimitRMS(), ChgrCfgMaxAcCurrA2D(), LOAD_MGMT_RESET, _comp_q16);    
            break;
        }
        if(ILineLongAvg() > (ChgrCfgBcrA2D() + LOAD_MGMT_DEADBAND))
		{
			charger_state = CS_LOAD_MGMT;
            SHOW_CHGR_STATE("CS: EQUALIZE->LOAD_MGMT:  ILine(%u) > BCR(%u) + DB(%u)", ILineLongAvg(), ChgrCfgBcrA2D(), LOAD_MGMT_DEADBAND);    
			break;
        }
        if(ILimitRMS() > (ChgrCfgMaxAcCurrA2D() + ILIMIT_POS_DEADBAND))
        {
			//  Current is too high
            charger_state = CS_CONST_CURR;
			SHOW_CHGR_STATE("CS: EQUALIZE->CONST_CURR:  ILimit(%u) > MaxAcCurr(%u) + DB(%u)", ILimitRMS(), ChgrCfgMaxAcCurrA2D(), ILIMIT_POS_DEADBAND);
			break;
        }
        if(IsChgrWarmBatt())
        {
            charger_state = CS_WARM_BATT;
			SHOW_CHGR_STATE("CS: EQUALIZE->WARM_BATT:  WarmBatt(%u)", IsChgrWarmBatt());
			break;
        }
		if(IsChgrEqualizeTimeout())
		{
			Chgr.status.charge_complete = 1;
            charger_state = CS_SOFT_STOP;
			SHOW_CHGR_STATE("CS: EQUALIZE->SOFT_STOP:  EqualizeTimeout(%u) Charge Complete", IsChgrEqualizeTimeout());
			
			Chgr.status.eq_status = CS_EQ_COMPLETE;
			g_nvm.settings.chgr.eq_request = IsChgrCfgEqRequest() = 0;
			nvm_SetDirty();
			break;
		}
		if (!IsChgrCfgEqRequest())
		{
			charger_voltage_state = CS_MONITOR;
			charger_state         = CS_MONITOR;
			break;
		}

		// regulate on VBatt
		if(VBattCycleAvg() > Chgr.status.volt_setpoint)
        {
			//  Too high
			p_adj = VBattCycleAvg() - Chgr.status.volt_setpoint;
			if (p_adj > 30) p_adj = 30;
			// decrement duty cycle
            _chgr_DutyAdjust(-p_adj);
			stability_count++;
        }
        else if(VBattCycleAvg() < Chgr.status.volt_setpoint)
        {
			//  Too low
			p_adj = Chgr.status.volt_setpoint - VBattCycleAvg();
			if (p_adj > 30) p_adj = 30;
			// increment duty cycle
            _chgr_DutyAdjust(+p_adj);
			stability_count++;
        }
		else if(--stability_count < 0)
		{
			stability_count = 0;
		}
		
		if(stability_count > STABILITY_COUNT_LIMIT)
		{
			stability_count = STABILITY_COUNT_LIMIT;
			_ChargeTimerEnable = 0;
		}
		else _ChargeTimerEnable = 1;
		
		if(_msg_flag)
		{	
//			LOG(SS_CHG, SV_INFO, "EQUALIZE"); 
		}
		break;

		
    case CS_MONITOR :
        if(!zc_flag) break; // run once per zero crossing

		// ----------------------------------------------------------------
        //  Monitor the battery to determine if and when we should resume charging.
        //  [2017-04-27] We are here because either the charge-cycle is complete 
        //  or the battery is over-temperature.
		// ----------------------------------------------------------------
		
        _chgr_DutyAdjust(-100);  // 100 ~ 0.333% duty cycle
        
        //  If the charge-cycle is complete
        if(chgr_StopRequest) 
		{ 
			chgr_StopRequest = 0; 
			charger_state = CS_SHUTDOWN;
			break;
		}
		if(!IsChargingComplete())
		{
			if(IsChgrBattOverTemp()) break;

			// battery no longer over-temp; go to warm batt
			charger_state = CS_WARM_BATT;
			SHOW_CHGR_STATE("CS: MONITOR->WARM_BATT:  NOT BattOT(%u)", IsChgrBattOverTemp());
			break;
		}

		// charging is complete
		if(IsChgrCfgEqRequest() && (Chgr.status.eq_status != CS_EQ_COMPLETE))
		{
       		if(IsChgrBattOverTemp()) break;

			charger_voltage_state = CS_EQUALIZE;
			charger_state         = CS_EQUALIZE;
			SHOW_CHGR_STATE("CS: MONITOR->EQUALIZE: (eq_status(%u) != EQ_COMPLETE(%u)) && EqReq(%u)", 
														  Chgr.status.eq_status, CS_EQ_COMPLETE, IsChgrCfgEqRequest());					
			break;
		}

		if(VBattCycleAvg() > RESTART_BATT_THRES_FLOAT) // 12.6 VDC
		{
			restart_timer = RESTART_TIMEOUT_SECONDS;
			break;
		}
		if(VBattCycleAvg() < RESTART_BATT_THRES_CV) // 12.0 VDC
		{
			chgr_ChargeCycleReset();
			charger_voltage_state = CS_CONST_VOLT;
			charger_state         = CS_CONST_VOLT;
			SHOW_CHGR_STATE("CS: MONITOR->CONST_VOLT:  VBatt(%u) < RESTART_BATT_CV(%u)", VBattCycleAvg(), RESTART_BATT_THRES_CV);
			break;
		}
		if(--restart_timer <= 0)
		{
			//	else if  if VBatt < 12.6 VDC AND the timer has expired, then go to FLOAT 
			chgr_ChargeCycleReset();
			charger_voltage_state = CS_FLOAT; 
			charger_state         = CS_FLOAT; 
			SHOW_CHGR_STATE("CS: MONITOR->FLOAT:  restart_timer expired");
			break;
		}
		break;
		
    case CS_SHUTDOWN :
		Chgr.status.eq_status = IsChgrCfgEqRequest() ? CS_EQ_PRECHARGE : CS_EQ_INACTIVE;
		
		pwm_Stop();
		pwm_Config(PWM_CONFIG_DEFAULT, NULL);
		pwm_Start();
        SetChgrNotActive();
        break;

    default :
        LOG(SS_CHG, SV_ERR, "ChgStateUnknown=%d", charger_state);
    case CS_INVALID :
        charger_state = CS_INITIAL;
		SHOW_CHGR_STATE("CS: INVALID->INITIAL: ");
        break;
    } // switch
}

#endif // OPTION_CHARGE_3STEP

// <><><><><><><><><><><><><> charger_3step.c <><><><><><><><><><><><><><><><><>
