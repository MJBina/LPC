// <><><><><><><><><><><><><> task_main.c <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//  Copyright(C) 2017 - Sensata Technologies, Inc.  All rights reserved.
//
//  $Workfile: task_main.c $
//  $Author: A1021170 $
//  $Date: 2018-04-05 13:02:35-05:00 $
//  $Revision: 65 $ 
//
//  LPC Main State Machine
//
//-----------------------------------------------------------------------------
//  This level of control is responsible for determining if we are in INVERTER
//  or CHARGER mode. It controls the state of the CHARGER RELAY and TRANSFER
//  RELAY. It processes the REMOTE ON input and the LINE OK input, and timers
//  associated with those inputs.
//
//  Most of the work for the INVERTER and CHARGER is performed in the PWM ISR.
//  To reduce complexity of that processing, individual ISRs are used for each
//  mode. main_Control also configures the ISRs for these modes.
//
//  In general any system/charger error will abort the charging cycle and 
//  shutdown the charger.  The system will attempt to re-start the charger.  
//  When re-starting, the transformer must be degaussed.
//
//  Errors and anomalies associated with the battery will alter or suspend the 
//  charger, and may resume normal operation when the condition is resolved.  
//  These include battery temperature and battery voltage.
//-----------------------------------------------------------------------------

// -------
// headers
// -------
#include "options.h"    // must be first include
#include "hw.h"
#include "dsPIC_serial.h"
#include "tasker.h"
#include "timer3.h"
#include "device.h"
#include "inverter.h"
#include "converter.h" 
#include "bootloader.h"
#include "pwm.h"
#include "nvm.h"
#include "fan_ctrl.h"
#include "ui.h"
#include "lpc_cfg.h"


// ----------------------------------------
//  Conditional Compile Flags for debugging
// ----------------------------------------

// comment out flag if not used
// #define DEBUG_SHOW_MAIN_TRANS 1 // show main state transitions for debugging
#define DEBUG_SHOW_MAIN_STATE 1 // show main state for debugging
// #define DEBUG_SHOW_DEV_CFG    1 // show device configuration
#define DEBUG_VERBOSE_MODE    1 // dump additional info for debugging
#define DEBUG_REDUCE_TIMEOUT  1 // use reduced time outs for testing

// automatically remove all debugging for certain conditions
#if OPTION_NO_CONDITIONAL_DBG
  #undef DEBUG_SHOW_MAIN_TRANS
  #undef DEBUG_SHOW_MAIN_STATE
  #undef DEBUG_SHOW_DEV_CFG
  #undef DEBUG_VERBOSE_MODE
  #undef DEBUG_REDUCE_TIMEOUT
#endif


// ---------
// Constants
// ---------

#ifdef DEBUG_REDUCE_TIMEOUT
  #undef ERROR_SHUTDOWN_TIMEOUT_SEC
  #define ERROR_SHUTDOWN_TIMEOUT_SEC (3*60)	 // 3 minutes reduced time for testing
   #ifndef WIN32
     #pragma message "3 Minute Timer for Low Batt Shutdown for testing"
   #endif
#endif

#define XFER_RELAY_OC_IGNORE_DELAY_MSEC    (200)


// -----------
// Main States
// -----------
typedef enum
{
    MS_INITIAL             = 0,        
    MS_INITIAL2            = 1,           
    MS_BOOT_CHARGE_WAIT    = 2,   
    MS_CHECK_RELAY_REQUEST = 3,    
	MS_WAIT_RELAY_CLOSE    = 4,   
	                        
	MS_CHGR_PRE_CHECK      = 5,	   
    MS_DEGAUSS_WAIT        = 6,       
    MS_CHARGER             = 7,            
	MS_CHGR_STOP_WAIT      = 8,	   
    MS_WAIT_RELAY_OPEN     = 9,    
                             
    MS_INVERTER            = 10,           
    MS_INV_STOP_WAIT       = 11,      
                             
	MS_ERROR               = 12,			   
	MS_ERROR_WAIT          = 13,		   
    MS_SHUTDOWN            = 14,

    MS_PURGATORY,
    
    NUM_MAIN_STATE_t  //  this has to be last
} MAIN_STATE_t;


// ----------
// Local Data
// ----------
// main_state can ONLY be changed via SetMainState()
static MAIN_STATE_t main_state = MS_INITIAL;

// Timer used to ignore over-current checking immediately after pulling in the relay.
// It is also used to prevent running the chgr_Driver() for 200 msecs,
// which is the time when high current transients are occuring.
int16_t xfer_relay_oc_ignore_timer_msec = XFER_RELAY_OC_IGNORE_DELAY_MSEC; 

const char * main_state_str[NUM_MAIN_STATE_t] =
{   // NOTE: must match MAIN_STATE_t enum order
    "INITIAL",         	  
    "INITIAL2",           
    "BOOT_CHARGE_WAIT",   
    "CHECK_RELAY_REQUEST",
    "WAIT_RELAY_CLOSE",   
    "CHGR_PRE_CHECK",	  
    "DEGAUSS_WAIT",       
    "CHARGER",            
    "CHGR_STOP_WAIT",	  
    "WAIT_RELAY_OPEN",    
    "INVERTER",           
    "INV_STOP_WAIT",      
    "ERROR",			  
    "ERROR_WAIT",		  
    "SHUTDOWN",
    "PURGATORY"
};

const char* MainStateToString(MAIN_STATE_t main_state)
{
    int state = (int)main_state;
    
    if (state < 0 || state >= (int)NUM_MAIN_STATE_t) return("INVALID");
    return(main_state_str[state]);
}


//------------------------------------------------------------------------------
// only TASK_main or routines it calls can call this
INLINE void SetMainState(MAIN_STATE_t newState)
{
    if (newState == main_state) return;
  #ifdef DEBUG_SHOW_MAIN_TRANS
    LOG(SS_SYS, SV_INFO, "MS: %s->%s", MainStateToString(main_state), MainStateToString(newState));
  #endif
    main_state = newState;
//  RB6_NumToScope(main_state); // debugging
}

//------------------------------------------------------------------------------
int dev_MainState()
{
	return((int)main_state);
}

// -----------
// Prototyping
// -----------
void _show_device_config(void); // debugging


//------------------------------------------------------------------------------
//                      D E B U G G I N G 
//------------------------------------------------------------------------------

// called every millisecond
#ifdef DEBUG_SHOW_MAIN_STATE
static void _show_main_state()
{
    static MAIN_STATE_t last_state = -1;
    static uint16_t last_status = 0;
    static uint16_t last_error = 0;
	static uint16_t	last_portb = 0xFFFF;
	static uint16_t	last_portd = 0xFFFF;

	if(last_portb != (PORTB & 0x4000))
	{
		last_portb = (PORTB & 0x4000);		//	B14 = BIAS_BOOST (170165 Rev-A)
		LOG(SS_SYS, SV_INFO, "PB %04X BIAS_BOOST = %d ", PORTB, PORTBbits.RB14?1:0);
	}
	
	if(last_portd != PORTD)
	{
		last_portd = PORTD;		//	D5 = KEEP_ALIVE 
//		LOG(SS_SYS, SV_INFO, "PD %04X KEEP_ALIVE = %d ", PORTD, PORTDbits.RD5?1:0);
	}
	
    if(last_state != main_state)
    {
        LOG(SS_SYS, SV_INFO, "MAIN_STATE: %s --> %s", main_state_str[last_state], main_state_str[main_state]);
        last_state = main_state;
    }

    if( last_status != Device.status.all_flags )
    {
        last_status = Device.status.all_flags;
        LOG(SS_SYS, SV_INFO, "Device Status: 0x%04X", Device.status.all_flags);

     #ifdef DEBUG_VERBOSE_MODE
        if(IsInvEnabled())                  LOG(SS_SYS, SV_INFO,    " inv_enable"    );
        if(HasInvRequest())                 LOG(SS_SYS, SV_INFO,    " inv_request"   );
        if(HasChgrRequest())                LOG(SS_SYS, SV_INFO,    " chgr_request"  );
        if(HasRelayRequest())               LOG(SS_SYS, SV_INFO,    " relay_request" );
        if(IsRemoteOn())     	            LOG(SS_SYS, SV_INFO,    " remote_on" );
        
        if(IsAcLineValid())                 LOG(SS_SYS, SV_INFO,    " ac_line_valid"    );
        if(IsAcLineQualified())             LOG(SS_SYS, SV_INFO,    " ac_line_qualified");
        if(IsChgrEnByCan())                 LOG(SS_SYS, SV_INFO,    " chgr_enable_can"  );
        if(IsChgrEnByBcr())                 LOG(SS_SYS, SV_INFO,    " chgr_enable_bcr"  );
        if(IsChgrEnByTime())                LOG(SS_SYS, SV_INFO,    " chgr_enable_time" );
        if(Device.status.pass_thru_enabled) LOG(SS_SYS, SV_INFO,    " pass_thru_enabled");

        if(IsChgrRelayActive())             LOG(SS_SYS, SV_INFO,    " chgr_relay_active"   );
        if(IsXferRelayActive())             LOG(SS_SYS, SV_INFO,    " xfer_relay_active"   );
        if(Device.status.tmr_shutdown_enabled) LOG(SS_SYS, SV_INFO, " tmr_shutdown_enabled");
        if(Device.status.tmr_shutdown) 		LOG(SS_SYS, SV_INFO,    " tmr_shutdown"        );
     #endif // DEBUG_VERBOSE_MODE
    }
    
    if( last_error != Device.error.all_flags )
    {
        last_error = Device.error.all_flags;
        LOG(SS_SYS, SV_INFO, "Device.error: 0x%04X", Device.error.all_flags);
        
      #ifdef DEBUG_VERBOSE_MODE
        if(IsXfrmOverTemp())     LOG(SS_SYS, SV_INFO, " xfmr_ot" );
        if(IsHeatSinkOverTemp()) LOG(SS_SYS, SV_INFO, " hs_temp" );
        if(IsNvmBad())           LOG(SS_SYS, SV_INFO, " nvm_is_bad" );
        if(IsBattTempSnsOpen())  LOG(SS_SYS, SV_INFO, " batt_temp_snsr_open" );
        if(IsBattTempSnsShort()) LOG(SS_SYS, SV_INFO, " batt_temp_snsr_short" );
        if(Is15VoltBad())        LOG(SS_SYS, SV_INFO, " vreg15_invalid" );
        if(device_error_battery_low())  LOG(SS_SYS, SV_INFO, " battery_low" );
      #endif // DEBUG_VERBOSE_MODE
    }
    
}
#endif // DEBUG_SHOW_MAIN_STATE


//-----------------------------------------------------------------------------
//                 U T I L I T Y   R O U T I N E S
//-----------------------------------------------------------------------------

// turn Field Effect Transistors off
static void FetsOff(void)
{
    //  Configure all outputs (shared by PWM) to be low
    LATEbits.LATE0 = 0;     //  RE0/PWM1L   pin-60 output   DRIVE_L1
    LATEbits.LATE1 = 0;     //  RE1/PWM1H   pin-61 output   DRIVE_H1
    LATEbits.LATE2 = 0;     //  RE2/PWM2L   pin-62 output   DRIVE_L2
    LATEbits.LATE3 = 0;     //  RE3/PWM2H   pin-63 output   DRIVE_H2
//  LATEbits.LATE4 = 0;     //  RE4/PWM3L   pin-64 output   RMT_FAULT_RD_LED
//  LATEbits.LATE5 = 0;     //  RE5/PWM3H   pin-1 output    FAN_CONTROL
//  LATEbits.LATE6 = 0;     //  RE6/PWM4L   pin-2 output    STATUS_GRN_LED
//  LATEbits.LATE7 = 0;     //  RE7/PWM4H   pin-3 output    STATUS_RD_LED
    
    //  Configure all outputs (shared by PWM) to be outputs
    TRISEbits.TRISE0 = 0;
    TRISEbits.TRISE1 = 0;
    TRISEbits.TRISE2 = 0;
    TRISEbits.TRISE3 = 0;
//  TRISEbits.TRISE4 = 0;
//  TRISEbits.TRISE5 = 0;
//  TRISEbits.TRISE6 = 0;
//  TRISEbits.TRISE7 = 0;
    
    //  Set all PWM I/O pins as general purpose I/O     
    PWMCON1bits.PEN1L = 0;
    PWMCON1bits.PEN2L = 0;
    PWMCON1bits.PEN3L = 0;
    PWMCON1bits.PEN4L = 0;
//  PWMCON1bits.PEN1H = 0;
//  PWMCON1bits.PEN2H = 0;
//  PWMCON1bits.PEN3H = 0;
//  PWMCON1bits.PEN4H = 0;
}


//------------------------------------------------------------------------------
//  device_CheckBattery is responsible for monitoring the battery voltage and
//  setting and clearing the flag device_error_battery_low().
//
//  For reference DEVICE_BATT_LOW_SHUTDOWN == 8.50VDC, and
//  DEVICE_BATT_HYSTERESIS == 0.50VDC. device_error_battery_low() is set
//  when the battery voltage falls below 8.50VDC, and cleared when it rises 
//  above 9.00VDC.
//
//  TODO: device_CheckBattery should be moved to task_dev.c

//  // DEBUG - FOR DEVELOPMENT TESTING ONLY!!!
//  //  NOTE: FET Body Diode Peak rectification produces about 8.6VDC on VBatt.
//  //      Need a higher threshold for testing firmware.
//  #ifdef DEVICE_BATT_LOW_SHUTDOWN
//  #undef DEVICE_BATT_LOW_SHUTDOWN
//  #define  DEVICE_BATT_LOW_SHUTDOWN       VBATT_VOLTS_ADC(9.5)   
//  #endif // DEVICE_BATT_LOW_SHUTDOWN


INLINE void device_CheckBattery(void)
{
    if(!IsCycleAvgValid())
    {
        device_error_battery_low() = 0;
    }
    else if(device_error_battery_low())
    {
        //  We are in a Battery-Low Voltage Situation.
        if(VBattCycleAvg() > (DEVICE_BATT_LOW_SHUTDOWN + DEVICE_BATT_HYSTERESIS)) 
        {
            LOG(SS_SYS, SV_ERR, "battery_low clear Vbatt=%4.2F ", VBATT_ADC_VOLTS(VBattCycleAvg()));
            device_error_battery_low() = 0;
        }
    }
    else
    {
        //  We are NOT in a Battery-Low Voltage Situation.
        if(VBattCycleAvg() < DEVICE_BATT_LOW_SHUTDOWN) 
        {
            LOG(SS_SYS, SV_ERR, "battery_low SET Vbatt=%4.2F ", VBATT_ADC_VOLTS(VBattCycleAvg()));
            device_error_battery_low() = 1;
        }
    }
}


//------------------------------------------------------------------------------
//             C H E C K   F O R   S Y S T E M   E R R O R S 
//------------------------------------------------------------------------------
//
//  CheckForSystemErrors returns 1 if system errors are detected. System errors
//  are not generally specific to a state, but should cause a transition to the 
//  ERROR state. In some cases we need to ignore errors on startup.
//
//  NOTE: CheckForSystemErrors is normally called once-per-millisecond.

INLINE int8_t CheckForSystemErrors(void)
{
    int8_t  result = 0;
    
    if(--xfer_relay_oc_ignore_timer_msec <= 0)
    {
        xfer_relay_oc_ignore_timer_msec = 0;
    }
    
	//	PRIORITY: If Housekeeping supply goes low, then immediately shutdown 
	//	- we have no control of the FETs.
	if( (MS_INITIAL != main_state) && (MS_INITIAL2 != main_state) && Is15VoltBad())
	{
		//  LOG(SS_SYS, SV_ERR, "Vreg15 = 0x%04X ", VReg15Raw());
        result = 1;
	}

	//	PRIORITY: If device is Over-Temp, then stop.
	//	The LPC hardware utilizes mechanical temperature sensors that may
	//	interrupt processor control of FET drives. In that case, we cannot do a
	//	soft-stop.  Rather than complicate the code by determining if we could 
	//	do a soft-stop, we simply shut-down. A degauss is required before 
	//	resuming charger.
	//		
	//	If the Transformer is Over-Temp, OR the Heatsink is Over-Temp.		
	if(IsDevOverTemp())
	{
        //  LOG(SS_SYS, SV_ERR, "Device Over-Temperature");
        pwm_Stop();
        pwm_Config(PWM_CONFIG_DEFAULT, pwm_DefaultIsr);
        pwm_Start();
        FetsOff();
        SetInvNotActive();
        SetChgrNotActive();
        result = 1;
	}
 #if !defined(OPTION_UL_TEST_CONFIG)
	//	PRIORITY: Don't allow charger, if the battery voltage is low.
	//	If the battery voltage is too low, then we could forward-bias the FET's
	//	body diodes (a.k.a. Bulk Rectification) causing excessive current.
	else if(HasChgrRequest() && device_error_battery_low())
	{
        result = 1;
	}
	//	PRIORITY: Continually check for excessive current in Charge Mode. 
	//	Perhaps the battery shorted after the charger started.
	//
	//	The Charge/Transfer relay is closed, and the charger duty-cycle is at a 
	//  minimum, then the charger should not be producing any current. 
	//  Shutdown if the battery current is:
	//		a) Exceeds 6A for at least (approx) 200mSec, OR
	//		b) Peaks over 16A.
	else if((IsChgrActive() && chgr_IsPwmDutyMinimum()) &&
        (IS_XFER_CHG_RELAY_ON() && (0 == xfer_relay_oc_ignore_timer_msec)))
	{
		static int16_t chgr_OcCount = 0;

		if(IMeasRMS() >= CHGR_OC_CHECK_HI_THRES)
		{
			Chgr.error.oc_shutdown = 1;
			XFER_CHG_RELAY_OFF();
			LOG(SS_SYS, SV_ERR, "Charger OC1 IMeasRMS(%d)>=HI_THRES(%d)", IMeasRMS(), CHGR_OC_CHECK_HI_THRES);
            result = 1;
		}
		else if(IMeasRMS() >= CHGR_OC_CHECK_THRES)
		{
			if(++chgr_OcCount > CHGR_OC_CHECK_COUNT)
			{
				Chgr.error.oc_shutdown = 1;
				XFER_CHG_RELAY_OFF();
				LOG(SS_SYS, SV_ERR, "Charger OC2 IMeasRMS(%d)>=OC_COUNT(%d)", IMeasRMS(), CHGR_OC_CHECK_COUNT);
                result = 1;
			}
		}
		else
		{
			chgr_OcCount = 0;
		}
	}
  #endif // OPTION_UL_TEST_CONFIG
  
    return(result);
}


//------------------------------------------------------------------------------
//            C H E C K     F O R     S H U T D O W N 
//------------------------------------------------------------------------------
	
static int16_t low_batt_shutdown_secs   = ERROR_SHUTDOWN_TIMEOUT_SEC;
static int16_t idle_timer_secs          = INVERTER_IDLE_TIMEOUT_SEC;

//------------------------------------------------------------------------------
INLINE void ResetShutdownCounter()
{
    // Device.status.tmr_shutdown_timer_sec represents time in seconds, 
    //  remaining until timer shutdown.
   	Device.status.tmr_shutdown_timer_sec = Device.config.tmr_shutdown_delay; 
}
    
   
//  Re-Try Inverter by toggling E6...
//	If AC is not qualified, then allow the operator to re-try the inverter by 
//  toggling E6: On|Off|On, or pressing the pushbutton twice (respectively).
//  NOTE: In Snap or Momentary mode, Inverter Request is active upon entry

// 		if (!IsAcLineQualified() && !HasInvRequest() && (IsRemModeSnap() || IsRemModeMom()))
// 		if (!IsAcLineValid() && !HasInvRequest() && (IsRemModeSnap() || IsRemModeMom()))
int8_t RetryInverter(void)
{
    static int16_t inv_restart_state = 0;
    static int8_t result = 0;
    
    
    if (IsAcLineValid())
    {
        inv_restart_state = 0;
        return (0);
    }
    
    switch(inv_restart_state)
    {
    default:
    case 0 :
        result = 0;
    case 1 :
        //  Verify that we have an Inverter Request.
        if (HasInvRequest())
        {
            inv_restart_state++;
        }
        break;
        
    case 2 :
        //  Verify that we DO NOT have an Inverter Request.
        if (!HasInvRequest())
        {
            result = 1;
            inv_restart_state = 0;
        }
        break;
    }
    return(result);
}


//------------------------------------------------------------------------------
//	CheckForShutdown
//	This function implements timers which affect shutdown. The intended 
//  operation is summarized below:
//    Normal Shutdown - Inverter was last to run: timeout = 5-sec
//	  Normal Shutdown - Charger was last to run: timeout = 2-min (120-sec)
//	  Error Shutdown: timeout = 15-min (60*15 = 7500 sec)
//	  Inverter Timer Shutdown: timeout = variable
//
//  The basic logical flow is:  
//    If Any Error, then:
//    	If Error is Inverter Low Battery, then:
//    		Run the ErrorTimer.
//    		If ErrorTimer has expired, then:
//                shutdown.
//    Else:	
//        If AC-Line Valid, then: 
//            Reset IdleTimer to 2-minutes.
//        Else If Inverter is Active, then: 
//            Reset IdleTimer to 5-seconds.
//            Run the InvShutdownTimer
//            If InvShutdownTimer has expired, then:
//                shutdown.
//        Else:  
//            Run the IdleTimer.
//            If IdleTimer has expired, then:
//                shutdown.
//            
//  NOTE: At present [2018-01-19] We do not shutdown when AC-Line is Valid
//      because the hardware signal (AC-Line valid) asserts Keep Alive, which 
//      overrides its control.
//
//  Test Cases:
//   1) Start Inverter, stop Inverter, verify shutdown after 5-seconds
//   2) Start Charger, stop Charger, verify shutdown after 5-seconds
//   3) Start Inverter, cause low-battery shutdown error, verify shutdown after 15-minutes
//   4) Start Inverter, cause high-temp error, wait 15-minutes, verify no shutdown.
//   4a) [Optional] resolve high-temp error, verify Inverter resumes.
//   5) Start Charger, cause high-temp error, wait 15-minutes, verify no shutdown.
//   5a) [Optional] resolve high-temp error, verify Charger resumes.

static int8_t CheckForShutdown(void)
{
    static int16_t msec_ticks = 0;
    int8_t  result = 0;

    
    //  This code gets called 1000 times a second.
    //  Timers need only one-second resolution.
    if(++msec_ticks < 1000)
    {
        return(0);
    }
    msec_ticks = 0;
   
    
    if (MS_ERROR == main_state) 
    {
        if(IsInvLowBattShutdown() && !IsAcLineValid() && 
            !device_status_aux_input())
        {
            //	If we have Inverter Low-Battery, Shutdown after 15-minutes.
            if(--low_batt_shutdown_secs <= 0) 
            {
                LOG(SS_SYS, SV_INFO, "Inverter Low-Battery Shutdown ");
                low_batt_shutdown_secs = 0;
                result = 1;
            }
        }
        else
        {
            low_batt_shutdown_secs = ERROR_SHUTDOWN_TIMEOUT_SEC;
        }
    }	
	else 
    {
        //  reset the low-battery shutdown timer
        low_batt_shutdown_secs = ERROR_SHUTDOWN_TIMEOUT_SEC;
        
        //	If AC-Line Valid, then: Set/Reset ShutdownTimer to 2-minutes. 
        if (IsAcLineValid())
        {
            ResetShutdownCounter();
            idle_timer_secs = CHARGER_IDLE_TIMEOUT_SEC;
        }
        //  Else if Aux Input is active, then: Set ShutdownTimer to 30-seconds.
        else if (device_status_aux_input())
        {
            ResetShutdownCounter();
            idle_timer_secs = INVERTER_IDLE_TIMEOUT_SEC;
        }
        //	Else If Inverter is (supposed to be) Active, then: Set ShutdownTimer 
        //  to 5-seconds. 
        else if (HasInvRequest())
        {
            idle_timer_secs = INVERTER_IDLE_TIMEOUT_SEC;
            if (Device.status.tmr_shutdown_enabled && IsInvEnabled())
            {
                if (--Device.status.tmr_shutdown_timer_sec <= 0)
                {
                    LOG(SS_SYS, SV_INFO, "Inverter Timer Shutdown ");   //  DEBUG
                    Device.status.tmr_shutdown_timer_sec = 0;   //  prevent wrap-around
                    Device.status.tmr_shutdown = 1;
                }
                LOG(SS_SYS, SV_INFO, "Inverter Shutdown Timer: %d ", Device.status.tmr_shutdown_timer_sec);
            }
            else
            {
                ResetShutdownCounter();
            }
        }		
        //	Else If ShutdownTimer has expired, then shutdown.	
        //	Else run the ShutdownTimer. 
        else if (--idle_timer_secs <= 0) 
        {
            LOG(SS_SYS, SV_INFO, "Idle Timeout Shutdown ");
            idle_timer_secs = 0; // prevent wrap-around
            result = 1;
        }
    }
    return( result );
}

//------------------------------------------------------------------------------
INLINE int8_t IsBattTempSensorOK(void)
{
	return(Device.config.batt_temp_sense_present && !(IsBattTempSnsOpen() || IsBattTempSnsShort()));
}

//------------------------------------------------------------------------------
//                  M A I N     S T A T E     D R I V E R
//------------------------------------------------------------------------------
//	The LPC hardware design does not provide separate transfer and charger 
//	relays (like NP) that accommodate independent functionality. The LPC will 
//	perform a Boot-Charge and Degauss only one-time on startup.
//
//  Degaussing
//	  Degauss is not performed if the initial operation is Inverter.
//	  The desired operation is that we degauss only on startup.
//    The flag 'degauss_needed' is cleared after that initial degauss.
//
//------------------------------------------------------------------------------

// gets called once per millisecond
void TASK_MainControl(void)
{
	//	TODO: Re-factor so that the 'main_state' variable is private (static).
    static int16_t timer_msec         = 0; // general-purpose timer
    static int8_t  degauss_needed     = 1; // 0=no, 1=degaussing is needed
    static int8_t  boot_charge_needed = 1; // 0=no, 1=boot charge is needed
  
  
    device_CheckBattery();
	if((MS_SHUTDOWN != main_state) && (MS_PURGATORY != main_state))
    {
        //  2018-04-05: Changes to address issue: 'Charger timers indicate 
        //  timed-out if cold-starting the charger with system error'.   
        //  Investigation of this issue identified that if a system error 
        //  (i.e. XFMR_OT) exists on startup, the main control state machine 
        //  would go into the ERROR state before resetting the charge cycle.  
        //  If the system error was then cleared, the state machine would resume 
        //  without completing initialization, which included initialization 
        //  of the charge-timers.
        //
        //  Changes (below) were made to assure that the main control state 
        //  would not act on system errors until after completed initialization.
        //
        //  NOTE: It was observed that Firmware will attempt a Boot-Charge and 
        //  Degauss even though an OT condition exists.  Because we are in the 
        //  final stages of DVT - This issue was not addressed at this time to 
        //  limit changes.
        
        if ((MS_ERROR != main_state) && (MS_INITIAL != main_state)
        && (MS_INITIAL2 != main_state) && CheckForSystemErrors())
        {   
            //  CheckForSystemErrors() returns true when a system error occurs.
            //  A "system error" includes things that affect the FET drivers 
            //  directly - meaning that firmware does not have control of the 
            //  FETs. Therefore, we call _StopNow() functions which immediately
            //  shutdown the device, and then go to the error state.
            if(IsChgrActive())
            {
                chgr_StopNow();
            }
            if(IsInvActive())
            {
                inv_StopNow();
            }
            SetMainState(MS_ERROR);
        }
        if (CheckForShutdown())      
        {   
            SetMainState(MS_SHUTDOWN);  
        }
    }
    

	
    // debugging	
  #ifdef DEBUG_SHOW_DEV_CFG
    _show_device_config();
  #endif

  #ifdef DEBUG_SHOW_MAIN_STATE
    _show_main_state();
  #endif

    // execute state machine
    switch(main_state)
    {
    default :
		//	TODO: This should indicate an error.
       	SetMainState(MS_INITIAL);
        // fall thru intentionally

    case MS_INITIAL :
        BIAS_BOOSTING_INACTIVE();	//	Enable Housekeeping Supply - Full-Power
		// 	Check that we see the vreg15_invalid first. During testing it was 
		//	observed that vreg15_invalid bounces valid/invalid/valid. This test 
		//	was added to assure that we don't trigger on the first 'valid'.
		//	TODO: Verify that this test is necessary.
      #if !defined(OPTION_UL_TEST_CONFIG)
		if(Is15VoltBad()) 
      #endif
		{
        	SetMainState(MS_INITIAL2);
		}
		break;
		
	case MS_INITIAL2 : 
		//	Wait for the Housekeeping supply to be valid
		if(!Is15VoltBad()) 
		{
			XFER_CHG_RELAY_OFF();
			chgr_ChargeCycleReset();
			inv_StartBootCharge();
			degauss_needed = 1;
			SetMainState(MS_BOOT_CHARGE_WAIT);
		}
		break;
	
    case MS_BOOT_CHARGE_WAIT :
		//	Wait for Boot Charge to complete
		//	Boot-Charge is an Inverter function: Inv status inv_active is TRUE
		//	while performing the Boot-Charge.  Wait in this state until Boot-
		//	Charge is complete.
	    if(!IsInvActive()) 
		{
			boot_charge_needed = 0;
			InvClearErrors();
			// reset shutdown timer
			Device.status.tmr_shutdown = 0; // clear flag
			ResetShutdownCounter();
			SetMainState(MS_CHECK_RELAY_REQUEST);
		}
		break;

        
    case MS_CHECK_RELAY_REQUEST :
		//	MS_CHECK_RELAY_REQUEST - Neither the inverter nor the charger are active.
        //	
		//  BIAS_BOOSTING_ACTIVE (pin-10 high) shuts down the boost of the 
		//  housekeeping (switching) power supply to reduce power consumption.
		//
		//	Reduced Power Consumption is needed for 'Load-Detection' 
		//	a.k.a. 'Ping' mode. In Inverter Mode, we allow the Inverter [driver] 
		//	to manage BIAS_BOOSTING, since it controls when to apply reduced 
		//	power. In this main loop, we want to simply set Full-power mode.
        // 		
		//	TODO: Is BIAS_BOOSTING_INACTIVE() necessary here? Left-over?
		//	NOTE: BIAS_BOOSTING is manipulated by the inverter (Ping-Mode)
		BIAS_BOOSTING_INACTIVE();	//	Enable Housekeeping Supply - Full-Power
			
        //  If there are any system errors that prevent the Inverter from 
        //  running, then go to error state. The list of things that might 
        //  prevent the Charger from running includes these conditions and more.
        //  Additional errors are tested before starting the Charger.
        if( IsXfrmOverTemp() || IsHeatSinkOverTemp() || IsNvmBad() || 
            Is15VoltBad() || device_error_battery_low())
        {
        //  If there are any 'common' errors then go to the error state.
            SetMainState(MS_ERROR);
        }
        else if(HasRelayRequest() && IS_XFER_CHG_RELAY_ON())
        {
            //  The relay is closed.  We have AC on the transformer, so degauss
            //  is not needed. Clear the flag to be safe.
            degauss_needed = 0;  
            
            if(HasChgrRequest())
            {    
                //  Charger/Transfer Relay is closed, ready to start the Charger...
                if(!IsBattTempSensorOK() || IsBattOverTemp() || HasChgrAnyErrors())
                {
                    //  If there are any errors preventing the Charger from 
                    //  running, then go to the error state.
                    SetMainState(MS_ERROR);
                }
                else
                {
                    LOG(SS_SYS, SV_INFO, "Chgr Start, VBatt=%4.2F ", VBATT_ADC_VOLTS(VBattCycleAvg()));
                    chgr_Start();
                    SetMainState(MS_CHARGER);
                }
            }
        }
        else if(HasRelayRequest())
        {
            if(boot_charge_needed)
            {
                inv_StartBootCharge();
                SetMainState(MS_BOOT_CHARGE_WAIT);	
            }
            else if(degauss_needed)
            {
                inv_StartDegauss();
                SetMainState(MS_DEGAUSS_WAIT);
            }
            else
            {
                //  need to close the Charger/Transfer Relay...
                XFER_CHG_RELAY_ON();
                xfer_relay_oc_ignore_timer_msec = XFER_RELAY_OC_IGNORE_DELAY_MSEC;
                timer_msec = RELAY_CLOSE_DELAY_MSEC;
                SetMainState(MS_WAIT_RELAY_CLOSE);
            }
        }
        else if(IS_XFER_CHG_RELAY_ON())
        {
            //  need to OPEN the Charger/Transfer Relay...
            XFER_CHG_RELAY_OFF();
            timer_msec = RELAY_OPEN_DELAY_MSEC;
            SetMainState(MS_WAIT_RELAY_OPEN);
        }
        else
        {
            //  Charger/Transfer Relay is OPEN, ready to test and start the Inverter.
            //  test for things that prevent the Charger from running.
            if(HasInvRequest())
			{
                if(!IsInvEnabled())
                {
                    //	The inverter might have been disabled via CAN, we don't 
                    //  know for how long, so boot charge to be safe.
                    boot_charge_needed = 1;
                }				
                else if(boot_charge_needed)
                {
                    inv_StartBootCharge();
                    SetMainState(MS_BOOT_CHARGE_WAIT);	
                }
                else 
                {
                    inv_Start();
                    SetMainState(MS_INVERTER);
                }
			}
        }
        break;

        
    case MS_WAIT_RELAY_CLOSE :
		//	Wait for Charge/Transfer relay contacts to close
		//	NOTE: MS_WAIT_RELAY_CLOSE_MSEC is a separate state since we might need
		//		to check for over-current while waiting...
        // fall thru intentionally to speed up process

	case MS_WAIT_RELAY_OPEN :
		//	Wait for Charge/Transfer relay contacts to open
        if (--timer_msec <= 0)
        {
			SetMainState(MS_CHECK_RELAY_REQUEST);
		}
        break;

    case MS_DEGAUSS_WAIT :
        //  Wait for degauss to complete
		//	Degauss is an Inverter function. See comments in MS_BOOT_CHARGE_WAIT
        if(!IsInvActive())
        {
            InvClearErrors();
            degauss_needed = 0;
            SetMainState(MS_CHECK_RELAY_REQUEST);
        }
        break;

	// -------------------------------------------------------------------------
	//	         C H A R G E R      O P E R A T I N G      S T A T E S
	// -------------------------------------------------------------------------
	//	LPC15 --> Charger Operation
	//
	//	NOTE: The LPC implements complex "Input Logic" controls comprising a 
	//	number of physical inputs and configurations. These are handled in 
	//	'TASK_devio_Driver'. from the charger viewpoint, these inputs include 
	//	the charger enables, and AC-Line qualification, and digest into the 
	//	status 'Device status chgr request' refer to the section titled 'NORMAL   
	//	ENABLE PROCESSING' for details.
	//
	//	Basic summary: 
	//		Device.status inv   request = inverter should be running
	//		Device.status relay request = chgr/xfer relay should be active
	//		Device.status.chgr  request = charger should be running
	//	
	//	Device.status inv request and Device status chgr_request are mutually 
	//	exclusive.
    //------------------------------------------------------------------------------

    case MS_CHARGER :
		//	The charger is running.  
		//	Determine if we should stop the charger
		if (!IsBattTempSensorOK() || IsBattOverTemp() || !HasChgrRequest() || HasChgrAnyErrors())
		{
			chgr_StopNow();
			SetMainState(MS_CHGR_STOP_WAIT);
		}
        break;
		
    case MS_CHGR_STOP_WAIT :
        //  Wait for charger to stop
        if(!IsChgrActive())
        {
			if(HasChgrAnyErrors()) 
			{
				SetMainState(MS_ERROR);
			}
			else 
			{
				// go back
				SetMainState(MS_CHECK_RELAY_REQUEST);
			}
        }
        break;
		
	// -------------------------------------------------------------------------
  	//	         I N V E R T E R   O P E R A T I N G    S T A T E S
	// -------------------------------------------------------------------------

	case MS_INVERTER :
        //  The INVERTER is running
		//	Monitor conditions that will shutdown the inverter
        if(HasInvAnyErrors() || HasRelayRequest() || !HasInvRequest() || 
            !IsInvEnabled() || Device.status.tmr_shutdown) 
        {                 
            inv_Stop();
            SetMainState(MS_INV_STOP_WAIT);
        }
        break;

    case MS_INV_STOP_WAIT :
        //  wait for inverter to stop
        if(!IsInvActive())
        {
            degauss_needed = 0;		//	we just soft-stopped
			if(HasInvAnyErrors())
			{
				// if we stopped because of an error,
				SetMainState(MS_ERROR);
			}
            else if(Device.status.tmr_shutdown)
            {
                SetMainState(MS_SHUTDOWN);
            }
			else
			{
				SetMainState(MS_CHECK_RELAY_REQUEST);
			}
        }
        break;

	// -------------------------------------------------------------------------
  	//	                       E R R O R    S T A T E S
	// -------------------------------------------------------------------------

    case MS_ERROR :
		//	'MS_ERROR' state accommodates display of error status. 
        //  The operator may retry an Inverter Error by toggling E6, or by 
        //  pressing the Pushbutton, if enabled. If no intervention the unit 
        //  will timeout and shutdown to conserve the battery. 
        //  Timers are handled elsewhere.
       
        
        if(!( IsXfrmOverTemp() || IsHeatSinkOverTemp() || IsNvmBad() || 
            Is15VoltBad() || device_error_battery_low()))
        {
            if(HasChgrRequest())
            {
                if(IsBattTempSensorOK() && !IsBattOverTemp() && !HasChgrAnyErrors())
                {
                    //  Charger is ready to run
                    SetMainState(MS_CHECK_RELAY_REQUEST);
                }
            }
            else if(HasInvRequest() && !HasInvAnyErrors())
            {
                //  Inverter is ready to run
                boot_charge_needed = 1;
                SetMainState(MS_CHECK_RELAY_REQUEST);
            }
            else if(!HasChgrRequest() && !HasInvRequest())
            {
                //  System is OK, and we have no requests to do anything...
                SetMainState(MS_CHECK_RELAY_REQUEST);
            }
        }

        //  If the Charger/Transfer Relay is On, but should be Off, then...
        //	TODO: Need delay here to assure relay is OPEN?
		if (IS_XFER_CHG_RELAY_ON() && 
            ( !HasRelayRequest() || device_error_battery_low() || Chgr.error.oc_shutdown ))
		{
            XFER_CHG_RELAY_OFF();
            //	The Charger might have been disabled via CAN, we don't know for 
            //  how long, so do a boot-charge to be safe.
            boot_charge_needed = 1;
            degauss_needed = 1;
		}
        else if (!IS_XFER_CHG_RELAY_ON() && 
            HasRelayRequest() && !device_error_battery_low() && !Chgr.error.oc_shutdown )
		{
            //  This will check if the Charger/Transfer Relay should be On, and
            //  take appropriate action. This includes delays, etc.
            SetMainState(MS_CHECK_RELAY_REQUEST);
            boot_charge_needed = 1;
            degauss_needed = 1;
        }
		
        //  Re-Try Inverter by toggling E6...
		//	If AC is not qualified, and the Remote Input is Snap or Momentary 
        //  mode then allow the operator to re-try the inverter by toggling E6, 
        //  or pressing the pushbutton twice (respectively).
 		if (!IsAcLineQualified())
        {
            if (RetryInverter())
            {
                //  Clear all Inverter Error flags.
                Inv.error.all_flags = 0;
                dev_ResetCheckVReg15();            
            }
		}
        break;

		
    case MS_SHUTDOWN :
        //  Waiting for shutdown...
		//	
		//  Shutdown the processor by turning-off Keep-Alive. However, if the 
        //  AC_LINE_VALID (hardware) signal is high, it will override KEEP_ALIVE 
        //  (hardware).
        
        if(!device_status_aux_input())
        {
            LOG(SS_SYS, SV_INFO, "Shutting Down. Bye!");
            KEEP_ALIVE_OFF();
            //	2017-11-09: It was discovered that if the fan is running when  
            //	we want to shutdown, its momentum will generate enough power to 
            //	restart the housekeeping power supply. In this shutdown state, 
            //	we simply want to wait for the power supply to decay and stop.
            fan_Stop();
            FAN_CTRL_OFF();
            XFER_CHG_RELAY_OFF();
            LED_RMT_FAULT_OFF();        // Turn LED_RMT_FAULT_OFF  - TODO: Why?
            BIAS_BOOSTING_INACTIVE();	// Full-Power = more load = faster shutdown
            RCONbits.BOR = 0;           // Disable Interrupts from Brown-Out
        }
        SetMainState(MS_PURGATORY);
        break;
        
    case MS_PURGATORY :
        //  In this state, we can respond to CAN commands, but we don't actively
        //  do anything. We can be reset by CAN, if we need to restart.
        if(device_status_aux_input())
        {
            //  T.B.D. This might cause a loop!!
            if(HasInvRequest() || HasRelayRequest())
            {
                SetMainState(MS_INITIAL2);
            }
        }
        else
        {
            //  use MS_SHUTDOWN state to do shutdown
            SetMainState(MS_SHUTDOWN);
        }
        break;
        
    } // switch main state
}

// <><><><><><><><><><><><><> task_main.c <><><><><><><><><><><><><><><><><><><>
