// <><><><><><><><><><><><><> charger_liion.c <><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//  Copyright(C) 2017 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//  $Workfile: charger_liion.c $
//  $Author: clark_dailey $
//  $Date: 2018-03-20 12:38:07-05:00 $
//  $Revision: 82 $ 
//
//  Charger for VOLTA Lithium-Ion Battery
//
//	These characteristics are specific to the VOLTA Li-Ion Battery Charger. 
//	(NOTE: 'Generic' Li-Ion Charger may differ.)
//
//	 1) The Charger does not read the Battery Temperature. The battery has an  
//		internal controller that utilizes a temperature sensor.	
//
//	 2) The Battery's internal controller will signal the charger to Turn Off
//		when it is fully charged. This signal is provided via the re-purposed 
//		temperature sensor input indicated as Device.status.chgr_enable_jmpr.
//
//		The desired operation is for the charger to provide maximum current 
//		until the battery voltage reaches the CV-Threshold, at which point the
//		algorithm changes from CC to CV.
//
//		If the Battery's internal controller determines a condition whereby it 
//		needs to stop the charger (i.e. temperature or full charge) it will 
//		disable the charger via the re-purposed temperature sensor input.
//
//	 3) The only timer that is used (from the Battery Recipe) is the CV-Timeout.
//		In the case of the timeout, we go to Monitor Mode.
//
//	 4) In Monitor Mode, if the Battery Voltage falls below 57 VDC then restart
//		the charge cycle.
//
//	 5) The VOLTA Li-Ion charge cycle does not include Warm-Batt, Float, or 
//		Equalization modes.
//
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
//	 Summary of Battery Recipe Entries "used" by VOLTAG Li-Ion:
//	 (NOTE: 'Generic' Li-Ion Charger may differ.)
//		batt_type;                  // Battery Type
//		cv_timeout_minutes;	        // Constant-Voltage Timeout (long)  (minutes)
//		cv_vsetpoint;               // CV Setpoint Voltage              (adc)
//
//-----------------------------------------------------------------------------
//
//	TODO & Questions - These are differences in operation between Generic NP and
//	Volta. Need to decide if these should be implemented identically in both.
//
//	 1) Generic NP allows the AC-line to drop out for 2-minutes before resetting
//		the charge cycle.
//		1a) NP has a 30-second restart timer. This accomodates restarting in 
//			either Float or CV states. For Volta, we don't have Float. Do we 
//			still want the 2-minute delay?
//
//	 2) Generic NP has a 30-second AC-Line Qualification timer.
//
//	 3) Generic NP requires that the charger output be "stable" before timers
//		will run.
//
//	 4) Does Volta have any conditions that require returning to the prior state?
//
//-----------------------------------------------------------------------------

// -------
// headers
// -------
#include "options.h"    // must be first include
#include "analog.h"
#include "charger.h"
#include "pwm.h"

// -------------------------
// File optionally compiled
// -------------------------
#ifdef OPTION_CHARGE_LION

#ifndef WIN32
  #pragma message "Li-Ion battery <charger_liion.c>"	
#endif

// --------------------------
// Conditional Debug Compiles
// --------------------------

// comment out flag to not use it
#define DEBUG_SHOW_CHGR	        1
#define DEBUG_SHOW_CHGR_TIMERS	1
#define DEBUG_SHOW_BATT_RECIPE	1
#define DEBUG_USE_VERBOSE       1


// -----------------------------
// Auto Removal for build types
// -----------------------------
#if OPTION_NO_CONDITIONAL_DBG
  #undef DEBUG_SHOW_CHGR
  #undef DEBUG_SHOW_CHGR_TIMERS	
  #undef DEBUG_SHOW_BATT_RECIPE
#endif	


// ----------
// local data
// ----------
static CHARGER_STATE_t prior_charger_state = CS_INITIAL;


//-----------------------------------------------------------------------------
//              D E B U G G I N G      R O U T I N E S
//-----------------------------------------------------------------------------

#ifndef DEBUG_SHOW_BATT_RECIPE
  #define _chgr_show_battery_recipe() { } // do nothing
#else
  #ifndef WIN32
	#pragma message "DEBUG_SHOW_BATT_RECIPE: _chgr_show_battery_recipe - for debugging"
  #endif
  
static void _chgr_show_battery_recipe(void)
{
    char buf1[10];
    
    LOG_TIMER_START();

    LOG(SS_CHG, SV_INFO, "Battery Recipe");
    LOG(SS_CHG, SV_INFO, "    Type = %d",                   Chgr.config.battery_recipe.batt_type);
//  LOG(SS_CHG, SV_INFO, "    CC Timeout %d amp*hours",     Chgr.config.battery_recipe.cc_max_amp_hours);
    LOG(SS_CHG, SV_INFO, "    CV Timeout %d minutes",       Chgr.config.battery_recipe.cv_timeout_minutes);
//  LOG(SS_CHG, SV_INFO, "    CV to Float ROC thres: %d",   Chgr.config.battery_recipe.cv_roc_sdev_threshold);
//  LOG(SS_CHG, SV_INFO, "    CV to Float ROC %d minutes",  Chgr.config.battery_recipe.cv_roc_timeout_minutes);
//  LOG(SS_CHG, SV_INFO, "    Float Timeout %d minutes",    Chgr.config.battery_recipe.float_timeout_minutes);
    LOG(SS_CHG, SV_INFO, "    CV Setpoint    %s VDC",       ftoa2(buf1, (float)VBATT_ADC_VOLTS(Chgr.config.battery_recipe.cv_vsetpoint)   ,1));
//  LOG(SS_CHG, SV_INFO, "    CV Voltage Max %s VDC",       ftoa2(buf1, (float)VBATT_ADC_VOLTS(Chgr.config.battery_recipe.cv_volts_max)   ,1));
//  LOG(SS_CHG, SV_INFO, "    Float Setpoint %s VDC",       ftoa2(buf1, (float)VBATT_ADC_VOLTS(Chgr.config.battery_recipe.float_vsetpoint),1));
//  LOG(SS_CHG, SV_INFO, "    Float Voltage Max %s VDC",    ftoa2(buf1, (float)VBATT_ADC_VOLTS(Chgr.config.battery_recipe.float_volts_max),1));
//  LOG(SS_CHG, SV_INFO, "    Eq Setpoint    %s VDC",       ftoa2(buf1, (float)VBATT_ADC_VOLTS(Chgr.config.battery_recipe.eq_vsetpoint)   ,1));
//  LOG(SS_CHG, SV_INFO, "    Eq Voltage Max %s VDC",       ftoa2(buf1, (float)VBATT_ADC_VOLTS(Chgr.config.battery_recipe.eq_volts_max)   ,1));
//  LOG(SS_CHG, SV_INFO, "    Warm Batt to Normal %dC ",    Chgr.config.battery_recipe.batt_temp_warm_recov);
//  LOG(SS_CHG, SV_INFO, "    Normal to Warm Batt %dC ",    Chgr.config.battery_recipe.batt_temp_warm_thres);
//  LOG(SS_CHG, SV_INFO, "    Normal to Warm Batt %dC ",    Chgr.config.battery_recipe.batt_temp_shutdown_recov);
//  LOG(SS_CHG, SV_INFO, "    Warm Batt to Shutdown %dC ",  Chgr.config.battery_recipe.batt_temp_shutdown_thres);

    LOG_TIMER_END();
}
#endif // DEBUG_SHOW_BATT_RECIPE



//-------------------------------------

#ifndef DEBUG_SHOW_CHGR	       
  #define _chgr_show_debug() 	// do nothing
#else
  #ifndef WIN32
	#pragma message "DEBUG_SHOW_CHGR debug enabled"
  #endif

static const char* _chgr_state_str[NUM_CHARGER_STATE_t] =
{
    // CAUTION! must mirror CHARGER_STATE_t
	"NOT_SET"	, 
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

//-------------------------------------
// _chgr_show_debug
static void _chgr_show_debug(void)
{
	static CHARGER_STATE_t last_state = -1;
	static uint16_t last_chgr_status  = 0;
	static uint16_t last_chgr_status2 = 0;
	static uint16_t last_chgr_error   = 0;
	
	
	if(chgr_over_volt)
	{
		chgr_over_volt = 0;
		//	Limits are set in Charger ISR
		LOG(SS_CHG, SV_ERR, "chgr_over_volt!  output = %dVDC", VBattCycleAvg());
	}
	
	if(chgr_over_curr)
	{
		chgr_over_curr = 0;
		//	Limits are set in Charger ISR
		LOG(SS_CHG, SV_ERR, "chgr_over_curr!");
	}
	
	if(chgr_resynch)
	{
		int16_t ix;

		chgr_resynch = 0;

		//  chgr_isr_hist_index is pointing to the oldest entry
		ix = chgr_isr_hist_index;
		LOG(SS_CHG, SV_WARN, "ISR Resync %d %d %d %d %d %d %d %d ",
		  chgr_isr_hist[ix],
		  chgr_isr_hist[((ix+1) & 0x07)], chgr_isr_hist[((ix+2) & 0x07)],
		  chgr_isr_hist[((ix+3) & 0x07)], chgr_isr_hist[((ix+4) & 0x07)],
		  chgr_isr_hist[((ix+5) & 0x07)], chgr_isr_hist[((ix+6) & 0x07)],
		  chgr_isr_hist[((ix+7) & 0x07)] );
	}
	
	
	if(last_state != ChgrState())
	{
		LOG(SS_CHG, SV_INFO, "CHGR STATE:\t%s --> %s ", 
			_chgr_state_str[last_state], _chgr_state_str[ChgrState()]);
		last_state = ChgrState();
	}

	if((last_chgr_status != Chgr.status.all_flags) || 
		(last_chgr_status2 != Chgr.status.all_flags2))
	{
		last_chgr_status = Chgr.status.all_flags;
		last_chgr_status2 = Chgr.status.all_flags2;
		LOG(SS_SYS, SV_INFO, "Charger Status: %04X %04X", Chgr.status.all_flags2, Chgr.status.all_flags);
	  #ifdef DEBUG_USE_VERBOSE
		if(IsChgrActive()          ) LOG(SS_SYS, SV_INFO, " charger_active"  );
		if(IsChgrOutputting()      ) LOG(SS_SYS, SV_INFO, " output_active"   );
		if(IsChgrWarmBatt()        ) LOG(SS_SYS, SV_INFO, " batt_warm"	     );
		if(IsChgrCvTimeout()       ) LOG(SS_SYS, SV_INFO, " cv_timeout"	     );
		if(IsChgrCvRocTimeout()    ) LOG(SS_SYS, SV_INFO, " cv_roc_timeout"  );
		if(IsChgrFloatTimeout()    ) LOG(SS_SYS, SV_INFO, " float_timeout"   );
		if(IsChgrSsCCLmTimeout()   ) LOG(SS_SYS, SV_INFO, " ss_cc_lm_timeout");
		if(IsChgrCcCvTimeout()     ) LOG(SS_SYS, SV_INFO, " cc_cv_timeout"   );
		if(IsChgrBattOverTemp()    ) LOG(SS_SYS, SV_INFO, " batt_ot"         );
		if(IsChargingComplete()    ) LOG(SS_SYS, SV_INFO, " charge_complete" );
		if(IsChgrEqualizeTimeout() ) LOG(SS_SYS, SV_INFO, " eq_timeout"      );
		if(IsChgrOverCurrDetected()) LOG(SS_SYS, SV_INFO, " oc_detected"     );
	  #endif
	}
	
	if( last_chgr_error != Chgr.error.all_flags)
	{
		last_chgr_error = Chgr.error.all_flags;
		LOG(SS_SYS, SV_ERR, "Charger ERROR: %04X", Chgr.error.all_flags);
		
	  #ifdef DEBUG_USE_VERBOSE
		if(IsChgrLowBattShutdown() ) LOG(SS_SYS, SV_INFO, " batt_lo_volt_shutdown ");
		if(IsChgrHighBattShutdown()) LOG(SS_SYS, SV_INFO, " batt_hi_volt_shutdown ");
		if(IsChgrOverCurrShutdown()) LOG(SS_SYS, SV_INFO, " oc_shutdown ");
		if(Chgr.error.cc_timeout   ) LOG(SS_SYS, SV_INFO, " cc_timeout ");
	  #endif
	}
}
#endif 	//	DEBUG_SHOW_CHGR	       

//-------------------------------------

#ifndef DEBUG_SHOW_CHGR_TIMERS	       
  #define _chgr_show_timers() { } // do nothing
#else
  #ifndef WIN32
	#pragma message "DEBUG_SHOW_CHGR_TIMERS debug enabled"
  #endif

static void _chgr_show_timers(void)  
{
    LOG_TIMER_START();

	LOG(SS_CHG, SV_INFO, "TIMERS (minutes):");
//	LOG(SS_CHG, SV_INFO, "\tcc_timer_amp  %ld", Chgr.status.cc_timer_amp_minutes   );
	LOG(SS_CHG, SV_INFO, "\tcc_elapsed     %d",  Chgr.status.cc_elapsed_minutes     );
	LOG(SS_CHG, SV_INFO, "\tcv_timer       %d",  Chgr.status.cv_timer_minutes       );
//	LOG(SS_CHG, SV_INFO, "\tcv_roc_timer   %d",  Chgr.status.cv_roc_timer_minutes   );
//	LOG(SS_CHG, SV_INFO, "\tss_cc_lm_timer %d",  Chgr.status.ss_cc_lm_timer_minutes );
//	LOG(SS_CHG, SV_INFO, "\tcc_cv_timer    %d",  Chgr.status.cc_cv_timer_minutes    );
//	LOG(SS_CHG, SV_INFO, "\tfloat_timer    %d",  Chgr.status.float_timer_minutes    );
//	LOG(SS_CHG, SV_INFO, "\teq_timer       %d",  Chgr.status.eq_timer_minutes       );
  
    LOG_TIMER_END();
}
#endif // DEBUG_SHOW_CHGR_TIMERS

////////////////////////////////////////////////////////////////////////////////


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
	//	reset _chgr_amp_sec_update()'s internal state-machine 
//	_chgr_amp_sec_update(1);

	//	reset chgr_Driver()'s internal state-machine 
	chgr_Driver(1);
//	prior_charger_state = CS_INITIAL;

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
//  chgr_TimerOneMinuteUpdate()
//-----------------------------------------------------------------------------
//	chgr_TimerOneMinuteUpdate 
//	
//	Voltas' requirements are:
//	1) The timer runs in CV-state, else it is reset.
//		1a) If the timer times-out, then shutdown.
//	2) If shutdown, monitor the battery voltage.  
//		2a) If the battery voltage falls below 57.0VDC, then restart the timer.
//
//	This is a lot like the "Monitor" mode.
//	This function is called once-per-millisecond, but could be called on a one-
//	second interval.
//-----------------------------------------------------------------------------

void chgr_TimerOneMinuteUpdate(void)
{
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
		++Chgr.status.cc_elapsed_minutes;
		break;
		
	case CS_CONST_VOLT :
//	        if(!IsChgrCvTimeout() && _ChargeTimerEnable)
        if(!IsChgrCvTimeout())
            {
            if(--Chgr.status.cv_timer_minutes <= 0)
            {
                Chgr.status.cv_timer_minutes = 0;
                Chgr.status.cv_timeout = 1;
            }
        }
		break;
	
	} // switch
	
	_chgr_show_timers();
}

//-----------------------------------------------------------------------------
//  chgr_Driver
//
//  chgr_Driver() is called once-per-millisecond from TASK_chgr_Driver().
//  
//  Control algorithms are based upon RMS values, which are updated once-per-
//  cycle on the positive to negative zero-crossing.
//
//  Control algorithms cannot run faster than the sample rate.
//
//  The flag chgr_pos_neg_zc is set in _chgr_pwm_isr() on the positive to 
//  negative zero-crossing and cleared here when used.
//
//	NOTE: For VOLTA Li-Ion we don't need to check VBatt setpoint against a
//	maximum value. The maximum value was used when temperature-compensating the
//	setpoint value. Volta does not use temperature compensation.
//
//-----------------------------------------------------------------------------

void chgr_Driver(int8_t reset)
{
    #define DMSG_INTERVAL_COUNT	((int16_t)(60))	//	DEBUG 1-sec (60Hz)
	static CHARGER_STATE_t last_state = -1;
    static int16_t _msg_tic = 0; // DEBUG
    int8_t _msg_flag        = 0; // DEBUG

	int16_t p_adj    = 0; // PROPORTIONAL adjustment
	
    static CHARGER_STATE_t charger_state = CS_INITIAL;
    
    static int16_t zc_bkup_tic = 0;
    int8_t zc_flag = 0;

	
    if(reset)
    {
        LOG(SS_CHG, SV_INFO, "_chgr_Driver reset");
		_chgr_show_battery_recipe();
		prior_charger_state = charger_state = CS_INITIAL;
        return;
    }
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
	
	//  The flag 'zc_flag' is used to synchronize control functions that need 
	//	to run once-per-zero-crossing.  It implements a backup counter, in case 
	//	AC goes away and we don't have a zero-crossing.  The backup counter 
	//	assures that the control will run at least once every 18-milliseconds
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
    
    switch(charger_state)
    {
    case CS_INITIAL :
        //  Initialize PWM Duty-Cycle to 20%
        _chgr_DutyReset();
        chgr_pos_neg_zc = 0;
		
		if(IsCycleAvgValid())
		{
			if(prior_charger_state != CS_INITIAL)
			{
				charger_state = prior_charger_state;
			}
			else
			{
				charger_state = CS_SOFT_START;
			}
		}
        break;

        
    case CS_SOFT_START :
        Chgr.status.volt_setpoint = Chgr.config.battery_recipe.cv_vsetpoint;
		Chgr.status.amps_setpoint = (ChgrCfgBcrA2D() < ChgrCfgMaxAcCurrA2D()) ? ChgrCfgBcrA2D() : ChgrCfgMaxAcCurrA2D();
        
		if(chgr_StopRequest)			{ charger_state = CS_SOFT_STOP;		}
		else if(IsChgrCvTimeout())	{ charger_state = CS_MONITOR;      	}
        else if(VBattCycleAvg() >= Chgr.status.volt_setpoint) 
										{ charger_state = CS_CONST_VOLT; 	}
        else if(ILimitRMS() >= ChgrCfgMaxAcCurrA2D())
										{ charger_state = CS_CONST_CURR;    }
        else if(ILineLongAvg() >= ChgrCfgBcrA2D())               
										{ charger_state = CS_LOAD_MGMT;  	}
//	TODO: Make sure that CS_LOAD_MGMT handles case where BCR is lowered suddenly.
//        else if(ILineLongAvg() > (ChgrCfgBcrA2D() + LOAD_MGMT_RESET))
//										{ _chgr_DutyReset();		        }
        else if(zc_flag)
        {
            //  increase the output once-per-zero-crossing
            _chgr_DutyAdjust(+10);   //###+3
        }
		
		if(_msg_flag)
		{	
//				LOG(SS_CHG, SV_INFO, "SOFT_START"); 
		}
        break;
        
        
    case CS_LOAD_MGMT :
		prior_charger_state = charger_state;

		//  Run this control once-per-zero-crossing
        if(0 == zc_flag) break;
        
        Chgr.status.volt_setpoint = Chgr.config.battery_recipe.cv_vsetpoint;
		Chgr.status.amps_setpoint = ChgrCfgBcrA2D();

        if(chgr_StopRequest) 					{ charger_state = CS_SOFT_STOP; }
        else if(ILineLongAvg() > (ChgrCfgBcrA2D() + LOAD_MGMT_RESET))
        {
            _chgr_DutyReset();
            charger_state = CS_SOFT_START;
        }
		// ------------------------------------------------------------------------
		//	Test for BCR Changed
		//	(2017-09-14) LOAD_MGMT_RESET is 20 or 30 A2D counts system-dependent.
		//	The test below will catch an abrupt change in the BCR. For example, 
		//	if the BCR is changed to something less than the the iLine current.
		//
		//	It is expected that normal control will maintain ILine within the 
		//	'LOAD_MGMT_RESET' tolerance.
		// ------------------------------------------------------------------------

		else if(ILineLongAvg() > (Chgr.status.amps_setpoint + LOAD_MGMT_RESET))
		{
			_chgr_DutyReset();
			charger_state = CS_SOFT_START;
		}
		else if(ILineLongAvg() > Chgr.status.amps_setpoint)
		{
			//	Limit Current at the BCR (Branch Circuit Rating)
			//	(2017-09-14) LOAD_MGMT_DEADBAND is 5 A2D counts.
			p_adj = ILineLongAvg() - Chgr.status.amps_setpoint;
			if(p_adj > 10) p_adj = 10;
			
			_chgr_DutyAdjust(-p_adj);
		}
		else if (ILineLongAvg() < (Chgr.status.amps_setpoint - LOAD_MGMT_DEADBAND))
		{
			p_adj = (Chgr.status.amps_setpoint - LOAD_MGMT_DEADBAND) - ILineLongAvg();
			if(p_adj > 10) p_adj = 10;

			if(VBattCycleAvg() >= Chgr.status.volt_setpoint)
			{
				charger_state = CS_CONST_VOLT;
			}
			//	CC should run at the maximum current allowed by charger hardware
			//	If the system is configured correctly, we should never see this 
			//	transition.
			else if(ILimitRMS() >= ChgrCfgMaxAcCurrA2D())
			{
				charger_state = CS_CONST_CURR;
			}
			else
			{
				_chgr_DutyAdjust(+p_adj);
			}
		}
		
        if(_msg_flag)
		{	
			LOG(SS_CHG, SV_INFO, "LD MGMT - SP:%d, ILine:%d ", Chgr.status.amps_setpoint, ILineLongAvg() ); 
		}
        break;

        
    case CS_CONST_CURR :        //  CONSTANT CURRENT / CURRENT-LIMIT
        //  Provide Maximum Allowed Current to the battery.
		
		prior_charger_state = charger_state;
        
        //  Run this control once-per-zero-crossing
        if(!zc_flag) return;
        
        Chgr.status.volt_setpoint = Chgr.config.battery_recipe.cv_vsetpoint;
		Chgr.status.amps_setpoint = ChgrCfgMaxAcCurrA2D();

        if(chgr_StopRequest)					{ charger_state = CS_SOFT_STOP;	}
        else if(VBattCycleAvg() >= Chgr.status.volt_setpoint)	
												{ charger_state = CS_CONST_VOLT; }
		//	Refer to comment: 'Test for BCR Changed' in CS_LOAD_MGMT above.
        else if(ILineLongAvg() > (ChgrCfgBcrA2D() + LOAD_MGMT_RESET))
												{ _chgr_DutyReset();
												  charger_state = CS_SOFT_START; }
		//	if we exceed the BCR, then go to Load Management
        else if(ILineLongAvg() > (ChgrCfgBcrA2D() + LOAD_MGMT_DEADBAND))
												{ charger_state = CS_LOAD_MGMT; }
        else
        {
			// ---------------------------------------------------------------
            //  Control charger current draw using iLimit as feedback
            //  In this mode, we are producing the maximum current that the 
            //  unit is capable of. The limit is hardware-specific and unique
            //  to the model.
            //
            //  ILimitRMS() is derrived from the same signal as 
            //  IMeas, but adjustable by a potentiometer on the circuitboard.
			// ---------------------------------------------------------------
        
            if(ILimitRMS() > ChgrCfgMaxAcCurrA2D())     //  Too high
            {
                _chgr_DutyAdjust(-1);
            }
            else if(ILimitRMS() < ChgrCfgMaxAcCurrA2D())    //  Too low 
            {
                _chgr_DutyAdjust(+1);
            }
			
			if(_msg_flag)
			{	
				LOG(SS_CHG, SV_INFO, "CHGR CC - SP:%d, ILimit:%d ", ChgrCfgMaxAcCurrA2D(), ILimitRMS()); 
			}
        }
        break;
       
        
    case CS_CONST_VOLT :    //  CONSTANT-VOLTAGE 
        //  Run this control once-per-zero-crossing
		prior_charger_state = charger_state;

        Chgr.status.volt_setpoint = Chgr.config.battery_recipe.cv_vsetpoint;

        if(chgr_StopRequest) 					{ charger_state = CS_SOFT_STOP; }
        else if(IsChgrCvTimeout())			    { Chgr.status.charge_complete = 1; charger_state = CS_MONITOR; }
		//	Refer to comment: 'Test for BCR Changed' in CS_LOAD_MGMT above.
        else if(ILineLongAvg() > (ChgrCfgBcrA2D() + LOAD_MGMT_RESET))
												{ _chgr_DutyReset();  charger_state = CS_SOFT_START; }
        else if(ILineLongAvg() >= ChgrCfgBcrA2D())	
												{ charger_state = CS_LOAD_MGMT;	}
        else if(ILimitRMS() > ChgrCfgMaxAcCurrA2D())       //  Current is too high
												{ charger_state = CS_CONST_CURR; }
        else
        {
			//  Run this control once-per-zero-crossing
			if(!zc_flag) return;

			if(VBattCycleAvg() > Chgr.status.volt_setpoint)     //  Too high
            {
				p_adj = VBattCycleAvg() - Chgr.status.volt_setpoint;
				if (p_adj > 30) p_adj = 30;
                _chgr_DutyAdjust(-p_adj);
            }
            else if(VBattCycleAvg() < Chgr.status.volt_setpoint)    //  Too low
            {
				p_adj = Chgr.status.volt_setpoint - VBattCycleAvg();
				if (p_adj > 30) p_adj = 30;
                _chgr_DutyAdjust(+p_adj);
            }
			
			if(_msg_flag)
			{	
//				LOG(SS_CHG, SV_INFO, "CONST VOLTAGE"); 
			}
        }   
        break;
        
        
    case CS_SOFT_STOP :
        //  Run this control once-per-zero-crossing
        if(!zc_flag) return;

		if(0 == _chgr_DutyAdjust(-100))
		{
			//	We've decremented the output to zero...
            SetChgrNotOutputting();
	        if(chgr_StopRequest) 
			{ 
				chgr_StopRequest = 0; 
				//	completely shutdown the charger.
				charger_state = CS_SHUTDOWN;
			}
			else if(IsChargingComplete())
			{
				//	If the charge is complete, then monitor the battery.
				charger_state = CS_MONITOR;
			}
			else
			{
				//	We've got no reason to be here... resume
				charger_state = prior_charger_state;
			}
        }
        break;
        
		
    case CS_MONITOR :
        //  The intent of CS_MONITOR state is to monitor the battery to 
        //  determine if and when we should resume charging.
		//	For Volta: We are here because of a CV-Mode timeout.
		
		//	2) If shutdown, monitor the battery voltage.  
		//		2a) If the battery voltage falls below 57.0VDC, then restart the timer.
		
		//	Keep decrementing.  Lower-level code will not go past zero.
        _chgr_DutyAdjust(-1);
        
        //  Run this control once-per-zero-crossing
        if(!zc_flag) return;
		zc_flag = 0;
		
        //  If the charge-cycle is complete
        if(chgr_StopRequest) 
		{ 
			chgr_StopRequest = 0; 
			//	completely shutdown the charger.
			charger_state = CS_SHUTDOWN;
		}
		else if(IsChargingComplete())
        {
			//	Resume to CS_MONITOR ONLY if we are here because we have 
			//	successfully completed the charge-cycle.
			prior_charger_state = charger_state;
		
			if(VBattCycleAvg() < RESTART_BATT_THRES_CV)
			{
				chgr_ChargeCycleReset();
				charger_state = CS_CONST_VOLT;
			}
        }
		break;
	
        
    case CS_SHUTDOWN :      //  Idle? 
        pwm_Stop();
        pwm_Config(PWM_CONFIG_DEFAULT, NULL);
        pwm_Start();
        SetChgrNotActive();
        SetChgrNotOutputting();
        break;

       
    default :
        LOG(SS_CHG, SV_ERR, "(%d)unknown charge state %d ", __LINE__, charger_state);
        charger_state = CS_INITIAL;
        break;
    }
    SetChgrState(charger_state);
}

#endif	//	Volta Li-Ion systems - end

// <><><><><><><><><><><><><> charger_liion.c <><><><><><><><><><><><><><><><><>
