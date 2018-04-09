// <><><><><><><><><><><><><> fan_ctrl_lpc.c <><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//  Copyright(C) 2018 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
// Fan Control for LPC board
//
//	The [Prioritized] Operational Characteristics of the fan are:
//	 1) If(Inverter Overload Occurred), then fan is ON
//	 2) Else if (20-seconds after system power-up/reset), then fan is OFF, 
//	 3) Else if (battery voltage < 10.75VDC (regardless of mode)), then fan is OFF, 
//	 4) Else if Shutdown, the fan will be OFF
//	 5) Else if High-Temp condition), then fan is ON
//	 6a) Else if (Inverter is running, AND power output > X), then fan is ON
//	 6b) Else if (Inverter is running, AND an Overload Occurred), then fan is ON 
//	 7)  Else if (Charger is running), then fan is ON
//
//  TODO: Remove Debug before releasing this code.
//      Debug provided to facilitate testing of this code, so that others might
//      better understand why the fan is running, or stopped.
//  
// ----------------------------------------------------------------------------

// headers
#include "options.h"    // must be first include
#include "hw.h"
#include "analog.h"
#include "charger.h"
#include "fan_ctrl.h"
#include "hw.h"
#include "inverter.h"

// --------------------------
// Conditional Debug Compiles
// --------------------------
//  #define DEBUG_FAN_CONTROL   1
//  #define DEBUG_SHOW_FAN_STATUS   1

#if OPTION_NO_CONDITIONAL_DBG
  #undef DEBUG_FAN_CONTROL
  #undef DEBUG_SHOW_FAN_STATUS
#endif	//  OPTION_NO_CONDITIONAL_DBG


// --------------------------
// Module Global Data
// --------------------------

static int8_t fan_trigger_flag = 0;

#ifndef DEBUG_SHOW_FAN_STATUS
  
  #define fan_debug_show_status()
  #define set_fan_status(FAN_STATUS) 
#else
  #ifndef WIN32
    #pragma message "ENABLED: fan_debug_show_status - for debugging"
  #endif

typedef enum 
{
    FAN_OFF_IDLE  = 0,
    FAN_OFF_SHUTDOWN,       //  Main Control is SHUTDOWN
    FAN_OFF_DELAY,
    FAN_OFF_CHGR_LOW_VBATT,
    FAN_OFF_INV_LOW_VBATT,
    FAN_OFF_CHARGER_NOT_ACTIVE,
    
    FAN_ON_INV_OVERLOAD,
    FAN_ON_INV_HIGH_POWER,
    FAN_ON_HIGH_TEMP,
    FAN_ON_CHARGER_ACTIVE,
    
    NUM_FAN_STATUS_t        //  must be last
} FAN_STATUS_t;

static FAN_STATUS_t fan_status = FAN_OFF_IDLE;
   
const char * fan_status_str[NUM_FAN_STATUS_t] =
{   // CAUTION! must match NUM_FAN_STATUS_t enum order
    "IDLE",         	  
    "SHUTDOWN (Main Control)",           
    "STARTUP DELAY",   
    "CHARGER LOW-BATT",
    "INVERTER LOW-BATT",
    "CHARGER NOT ACTIVE",

    "INVERTER OVERLOAD",   
    "INVERTER HIGH-POWER",	  
    "HIGH-TEMP",       
    "CHARGER ACTIVE"
};

static void fan_debug_show_status(void)
{
    static FAN_STATUS_t last_fan_status = FAN_OFF_IDLE;
    
    if(last_fan_status != fan_status)
    {
        LOG(SS_SYS, SV_INFO, "FAN %s because %s", 
            fan_trigger_flag ? "Triggered" : "Stopping", fan_status_str[fan_status]);
        last_fan_status = fan_status;
    }
}


INLINE void set_fan_status(FAN_STATUS_t status) 
{
    fan_status = status;
}

#endif  //  DEBUG_SHOW_FAN_STATUS

// ----------------------------------------------------------------------------

INLINE void FAN_ON()
{
    FAN_CTRL_ON();
    //  RB6_SET(1);
}

// ----------------------------------------------------------------------------

INLINE void FAN_OFF()
{
    FAN_CTRL_OFF();
    //  RB6_SET(0);
}

// ----------------------------------------------------------------------------
// return fan duty cycle 0=off, 100=full-on

uint16_t fan_GetDutyCycle()
{
    return (fan_trigger_flag ? 100 : 0);
}

// ----------------------------------------------------------------------------
// over-ride fan control for testing

void fan_SetOverRide(uint16_t duty_cycle)
{
    // not supported by this model
}

// ----------------------------------------------------------------------------
// LPC fan is not duty cycled

void fan_Timer(void)
{
}


// ----------------------------------------------------------------------------
//	fan_StateMachine implements soft-start and timers

INLINE void fan_StateMachine(int8_t reset)
{
    //  Fan Stop Delay = 1-minutes
    #define FAN_STOP_DELAY_SEC    (60)  // 1-minute

    static int16_t fan_state = 0;
    // 	This is the optimal fixed 1mSec On/Off duty-cycle duration to reduce 
    //	the Sunon/Delta Fan high turn ON current down closer to its steady state 
    //	current to maintain the 14V rail from instantly drops down to no less 
    //	than 11V at 100% duty-cycle. This is intended so that we don't trip the 
    //	14_RAIL_RESET invalid supply threshold of 10.2V  at low DC input voltage 
    //	ranges (10.5-11.5V) when the FAN turns ON.
    #define FAN_CYCLE_COUNT     1000    //	2*1000 == 2000 mSec == 2.0-sec
    static int16_t count = 0;
    static int16_t s_msCounter = 0;

    if (reset)
    {
        FAN_OFF();
        fan_state = 0;
        return;
    }


#ifdef DEBUG_FAN_CONTROL
    static int16_t last_state = 0;
    static int8_t last_trigger_flag = 0;
    if (last_state != fan_state || last_trigger_flag != fan_trigger_flag)
    {
        LOG(SS_SYS, SV_INFO, "Fan state=%d run=%d", (int) fan_state, (int) last_trigger_flag);
        last_state = fan_state;
        last_trigger_flag = fan_trigger_flag;
    }
#endif // DEBUG_FAN_CONTROL

    switch (fan_state)
    {
    default:
    case 0: // 	initialization
        fan_state = 1;
        // fall-thru

    case 1: //	wait for command to run
        FAN_OFF();
        if (fan_trigger_flag)
        {
            count = FAN_CYCLE_COUNT;
            fan_state++;
        }
        break;

    case 2: //	ramping up: PWM'ing output high
    case 6: //	ramping down: PWM'ing output high
        FAN_ON();
        fan_state++;
        break;

    case 3: //	ramping up: PWM'ing output low
    case 7: //	ramping down: PWM'ing output low
        FAN_OFF();
        if (--count <= 0)
        {
            fan_state++;
        }
        else
        {
            fan_state--;
        }
        break;

    case 4: //	fan full-speed
        FAN_ON();
        if (!fan_trigger_flag)
        {
            s_msCounter = 0;
            count = FAN_STOP_DELAY_SEC;
            fan_state++;
        }
        break;

    case 5: // 	shutdown delay
        //	one-second timer
        if (fan_trigger_flag)
        {
          #ifdef DEBUG_SHOW_FAN_STATUS
            LOG(SS_SYS, SV_INFO, "Fan Stop Timer Canceled", count );
          #endif  //  DEBUG_SHOW_FAN_STATUS
            fan_state--;
        }
        else if (++s_msCounter > 1000)
        {
          #ifdef DEBUG_SHOW_FAN_STATUS
            LOG(SS_SYS, SV_INFO, "Fan Stopping in: %d seconds", count );
          #endif  //  DEBUG_SHOW_FAN_STATUS

            s_msCounter = 0;
            if (--count <= 0)
            {
                count = FAN_CYCLE_COUNT;
                fan_state++;
            }
        }
        break;
        
    case 9: //	go back to waiting...
        fan_state = 1;
        break;
    } // switch
}


// ----------------------------------------------------------------------------
//	fan_Control is called once-per-millisecond.
//
//	fan_Control is responsible for testing [some] conditions that require the 
//	fan to run.

static void fan_Control(int8_t reset)
{
    static int16_t ms_tics = 1000;
    //  Prevent fan from running for 20-seconds after unit startup
    #define FAN_STARTUP_DELAY_SEC   (20)
    static int16_t startup_delay_sec = FAN_STARTUP_DELAY_SEC;
    //  The fan should turn on 20-seconds after the charger starts.  This is to
    //  prevent the fan from interfering with charger startup activities.
    //  NOTE: If the fan is running when the charger starts, it should continue 
    //  to run.
    #define CHGR_ACTIVE_DELAY_SEC   (20)
    static int16_t chgr_active_delay_sec = CHGR_ACTIVE_DELAY_SEC;
    
    //  Inverter Current Threshold Qualification Delay = 20-seconds
    #define FAN_INV_HIGH_PWR_QUAL_SEC   (2)
    static int16_t inv_high_pwr_qual_sec = FAN_INV_HIGH_PWR_QUAL_SEC;
    int8_t inv_high_pwr_flag = 0;
    

    if(reset)
    {
        fan_trigger_flag = 0;
		fan_StateMachine(1);
        startup_delay_sec = FAN_STARTUP_DELAY_SEC;
        inv_high_pwr_qual_sec = FAN_INV_HIGH_PWR_QUAL_SEC;
        return;
    }
    
    
    if (--ms_tics <= 0)
    {
        //  We only need to run this once-per-second...
        ms_tics = 1000;
        
        if(IsInvActive() && IsCycleAvgValid() && (IMeasRMS() > FAN_ON_THRES_INV_AMPS))
        {
            if(--inv_high_pwr_qual_sec <= 0)
            {
                inv_high_pwr_qual_sec = 0;
                inv_high_pwr_flag = 1;
            }
        }
        else
        {
            inv_high_pwr_qual_sec = FAN_INV_HIGH_PWR_QUAL_SEC;
            inv_high_pwr_flag = 0;
        }
        
        if(--startup_delay_sec <= 0)
        {
            startup_delay_sec = 0;
        }
      #ifdef DEBUG_SHOW_FAN_STATUS
        else 
        {
            LOG(SS_SYS, SV_INFO, "startup_delay_sec: %d seconds", 
                startup_delay_sec );
        }
      #endif    //  DEBUG_SHOW_FAN_STATUS
        
        //  NOTE: The test that is used to reset chgr_active_delay_sec must be 
        //  consistent with the whatever test is used to determine that the 
        //  charger is (or should be) running.
        if(!IsChgrOutputting() || (chgr_GetState() == CS_MONITOR))
        {
            chgr_active_delay_sec = CHGR_ACTIVE_DELAY_SEC;
        }
        else if(--chgr_active_delay_sec <= 0)
        {
            chgr_active_delay_sec = 0;
        }
      #ifdef DEBUG_SHOW_FAN_STATUS
        else
        {
            LOG(SS_SYS, SV_INFO, "chgr_active_delay_sec: %d seconds", 
                chgr_active_delay_sec );
        }
      #endif  //  DEBUG_SHOW_FAN_STATUS
    }

    //  These are the prioritized fan control conditions -----------------------
    
    if (IsInvOverLoadDetected())
	{
        set_fan_status(FAN_ON_INV_OVERLOAD);
		fan_trigger_flag = 1;
	}
    else if(startup_delay_sec > 0)
    {
        //  If the startup delay is still active, keep fan off.
        set_fan_status(FAN_OFF_DELAY);
		fan_trigger_flag = 0;
    }
    else if(IsAcLineValid())
    {
        //  TODO: This needs debouncing...
        if(IsCycleAvgValid() && (VBattCycleAvg() < FAN_OFF_THRES_VOLTS))
        {
            set_fan_status(FAN_OFF_CHGR_LOW_VBATT);
            fan_trigger_flag = 0;
            //  reset fan_StateMachine so fan stops immediately
            fan_StateMachine(1);        
        }       
        else if(IsDevOverTemp())
        {
            set_fan_status(FAN_ON_HIGH_TEMP);
            fan_trigger_flag = 1;
        }  
        else if (!IsChgrOutputting())
        {
            set_fan_status(FAN_OFF_CHARGER_NOT_ACTIVE);
            fan_trigger_flag = 0;
        }
        else if(0 == chgr_active_delay_sec)
        {
            set_fan_status(FAN_ON_CHARGER_ACTIVE);
            fan_trigger_flag = 1;
        }
        else
        {
            set_fan_status(FAN_OFF_CHARGER_NOT_ACTIVE);
            fan_trigger_flag = 0;
        }
    }
    else if(IsInvEnabled() && HasInvRequest() && IsInvLowBattShutdown())
    {
        set_fan_status(FAN_OFF_INV_LOW_VBATT);
        fan_trigger_flag = 0;
        //  reset fan_StateMachine so fan stops immediately
        fan_StateMachine(1);        
    }
    else  
    {
        //  NOTE: These are implemented individually for DEBUG           
        //  if(IsDevOverTemp() || inv_high_pwr_flag)
        if(IsDevOverTemp())
        {
            set_fan_status(FAN_ON_HIGH_TEMP);
            fan_trigger_flag = 1;
        }
        else if(inv_high_pwr_flag)
        {
            set_fan_status(FAN_ON_INV_HIGH_POWER);
            fan_trigger_flag = 1;
        }
        else
        {
            set_fan_status(FAN_OFF_IDLE);
            fan_trigger_flag = 0;
        }
    }
}        


// ----------------------------------------------------------------------------
// 	TASK_fan_Driver executes once-per-millisecond
//	TASK_fan_Driver is a state machine to implement delays and soft-start/stop.
//	These features are needed with the LPC to support the housekeeping supply.
//	The fan is powered from the housekeeping supply. So are the FET drivers.
//	If the fan causes a current surge, this will affect the housekeeping supply
//	which may result in malfunction.

void TASK_fan_Driver(void)
{
    //	fan_Control determines When/How the fan should operate - it checks the 
    //	conditions that make the fan run.
    fan_Control(0);

	fan_StateMachine(0);
    
    fan_debug_show_status();
}


// ----------------------------------------------------------------------------
//	fan_Start() and fan_Stop() are TASK control functions.  
//  fan_Start() 'should' be called from main to initialize the task... but is 
//  not necessary on initial power-up.
//  fan_Stop() can be called anytime to stop the task.  It also stops the fan.
//  When fan_Stop() is called, fan_Start() must be called to re-start the task.
//
//  NOTE fan_Stop() should cause the fan to stop immediately so that the fan 
//  can be overridden off at shutdown.  It was determined that the momentum of
//	the fan blades caused the fan to generate enough power to restart the CPU.

void fan_Stop(void)
{
    fan_trigger_flag = 0;
    FAN_OFF();
}

void fan_Start(void)
{
    fan_trigger_flag = 0;
    fan_StateMachine(1);
}

// <><><><><><><><><><><><><> fan_ctrl_lpc.c <><><><><><><><><><><><><><><><><>
