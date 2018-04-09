// <><><><><><><><><><><><><> ui.c <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//  Copyright(C) 2018 - Sensata Technologies, Inc.  All rights reserved.
//
//  $Workfile: ui.c $
//  $Author: A1021170 $
//  $Date: 2018-04-05 08:49:46-05:00 $
//  $Revision: 73 $
//
//  User Interface Implementation
//
//  +------------------------------------------------------------------------+
//  |                           LPC Blink Codes                              |
//  +------------------------------------------------------------------------+
//  |  First 5 minutes after power up, shows battery type blink code         |
//  |  Normal LED Blink Rate: 250 msec on, 250 msec off                      |
//  |  Blink codes are dependent upon operation mode: Inverter / Charger     |
//  |                                                                        |
//  |  Normal Blink = on 250 msec, off 250 msec (2 hz)                       |
//  |  Fast   Blink = on 100 msec, off 100 msec (5 hz)                       |
//  |                                                                        |
//  |  Battery Type Blink codes: (first 5 minutes of power on, unless fault) |
//	|    Wet Lead Acid   1 Amber  1 Green                                    |
//  |    AGM             1 Amber  2 Green                                    |
//  |    GEL             1 Amber  3 Green                                    |
//  |    Lithium Ion     1 Amber  4 Green                                    |
//  |                                                                        |
//  |  Fault Conditions:                                                     |
//  |    If any fault occurs,                                                |
//  |    the battery type code is aborted and never resumes                  |
//  |                                                                        |
//  |    If two or more faults exist simultaneously,                         |
//  |    fault displayed shall be per the following priority                 |
//  |      1)  Low battery, High battery                                     |
//  |      2)  Overload                                                      |
//  |      3)  Over-temperature                                              |
//  |      4)  Feedback fault                                                |
//  |                                                                        |
//  +---------------------------+-------------------+------------------------+
//  | Inverter Mode             | Local LED         | Remote LED (Red Only)  |
//  +---------------------------+-------------------+------------------------+
//  | Normal operation          | Grn, constant     | Constant               |
//  | Low battery 0-5 sec       | Org, constant     | Constant               |
//  | Overload 0-5 sec          | Red, constant     | Constant               |
//  | Low battery > 5 sec       | Red, 1 blink      | 1 blink                |
//  | Overload > 5 sec          | Red, 2 blinks     | 2 blinks               |
//  | High Temp Heatsink        | Red, 3 blinks     | 3 blinks               |
//  | Feedback fault            | Red, 4 blinks     | 4 blinks               |
//  | High battery 0-5 minutes  | Red, 5 blinks     | 5 blinks               |
//  | High battery > 5 minutes  | Red, 6 blinks     | 6 blinks               |
//  | High Temp Transformer     | Red, 7 blinks     | 7 blinks               |
//  | Inverter off by CAN       | Red, 8 blinks     | 8 blinks               |
//  +---------------------------+-------------------+------------------------+
//  | Charger Mode              | Local LED         | Remote LED (Red Only)  |
//  +---------------------------+-------------------+------------------------+
//  |  BULK   (Const Current)   | Grn, 1-blink      | Constant               |
//  |  ACCEPT (Const Voltage)   | Grn, 2-blinks     | Constant               |
//  |  FLOAT                    | Grn, 3-blinks     | Constant               |
//  |  Load Management Limited  | Grn, 4-blinks     | Constant               |
//  |  Equalization             | Grn, 5-blinks     | Constant               |
//  |  Monitor                  | Grn, 6-blinks     | Constant               |
//  |  Chgr off-Check Batt Probe| Org, 1-blinks     | Fast blink, continuous |
//  |  Chgr off-Warm Batt       | Org, 2-blinks     | Fast blink, continuous |
//  |  Chgr off-High Batt Volt  | Org, 3-blinks     | Fast blink, continuous |
//  |  Chgr off-High Batt Temp  | Org, 4-blinks     | Fast blink, continuous |
//  |  Chgr off-Lo Batt Volt    | Org, 5-blinks     | Fast blink, continuous |
//  |  Chgr off-High Temp Xfmr  | Org, 6-blinks     | Fast blink, continuous |
//  |  Chgr off-High Temp HtSnk | Org, 7-blinks     | Fast blink, continuous |
//  |  Chgr off-0 Amp Limit Set | Org, 8-blinks     | Fast blink, continuous |
//  |  Chgr Off-CC Timeout      | org, 9-blinks     | Fast blink, continuous |   
//  |            / OC Shutdown  |                   |                        |
//  |  Chgr off-Disabled by CAN | Org, 10-blinks    | Fast blink, continuous |
//  +---------------------------+-------------------+------------------------+
//
//-----------------------------------------------------------------------------

#include "options.h"    // must be first include
#include "hw.h"
#include "uimsg.h"
#include "charger.h"
#include "inverter.h"
#include "sensata_can.h"


// ----------
// Constants
// ----------

// LED blink timing values
#define FLASH_TIMER_MS    (250)    // Flash duration on/off time (milliseconds)
#define FLASH_PAUSE_CNTS   (5)     // Delay between counts (FLASH_TIMER_MS counts) 
#define FLASH_FAST_MS    (100)     // led off/on time for fast blinking (msecs)
#define BATT_TIMER_CNTS	  (5*60*(1000/FLASH_TIMER_MS))  // show battery type on LED for FLASH_TIMER_MS counts


// special _RemoteCount codes; values > 0 are blink counts
#define LED_CONSTANT     0  // constant on
#define LED_FAST_BLINK  -1  // fast blink
#define LED_OFF         -2  // constant off


// -----------
// Local Data
// -----------

static UI_LED_FLASH_CODE  _Local = {0, 0, 0};  // LED flash code state
static int16_t            _RemoteCount = 0;    // -2=off, -1=fast blink, 0=on, >0 blink count


//-----------------------------------------------------------------------------
// There are two LEDs
//   One is on the main board TriColor Red/Green/Amber
//   One is on the remote switch - Red only
//
static void LedSetup(UI_LED_COLOR local_color, int16_t local_count, int16_t remote_count)
{
    // local LED setup
    _Local.count = local_count;
    switch(local_color)
    {
    case UI_LED_OFF :  _Local.red = 0;  _Local.grn = 0; break; // Red=0 Grn=0  Off
    case UI_LED_RED :  _Local.red = 1;  _Local.grn = 0; break; // Red=1 Grn=0  Red
    case UI_LED_GRN :  _Local.red = 0;  _Local.grn = 1; break; // Red=0 Grn=1  Green
    case UI_LED_YEL :  _Local.red = 1;  _Local.grn = 1; break; // Red=1 Grn=1  Amber
    } // switch 

    // remote LED setup
    _RemoteCount = remote_count;
}

//-----------------------------------------------------------------------------
//  Must be called every millisecond to drive the state machine timing
static void _led_RemoteDriver(int8_t reset)
{
    static char    state            = 0;
    static int16_t msec_ticks       = 0; // msec up counter
    static int16_t delay_counter    = 0; // delay counter in FLASH_TIMER_MS times between blink sequences
    static int16_t blinks_remaining = 0; // remaining blink flashes before sequence delay period
	static int16_t last             = LED_OFF-1;

   	if(reset)
	{
		//	Reset the normal state machine
		state            = 0;
		msec_ticks       = 0;
	    delay_counter    = 0;
	    blinks_remaining = 0;
		return;
	}

	//	If the LED setup has changed, restart this state machine so that the 
	//	new settings take effect immediately.
    if (_RemoteCount != last)
    {
        last = _RemoteCount;
        state = 0;
    }

    switch (_RemoteCount)
    {
    case LED_OFF:
        LED_RMT_FAULT_OFF();
        return;

    case LED_CONSTANT:
        LED_RMT_FAULT_ON();
        return;

    case LED_FAST_BLINK:
        if(++msec_ticks < FLASH_FAST_MS) return;
        msec_ticks = 0;
        state = !state; // toggle state  0->1  1->0
        if (state)
            LED_RMT_FAULT_ON();
        else
            LED_RMT_FAULT_OFF();
        return;

    default:
        if (++msec_ticks < FLASH_TIMER_MS) return;
        msec_ticks = 0;
        if (_RemoteCount < 1) return; // not a valid blink count value
        break;

    } // switch
    
    switch(state)
    {
    case 0 :
        LED_RMT_FAULT_ON();
        blinks_remaining = _RemoteCount; 
        state++;
        break;

    case 1 :
        LED_RMT_FAULT_OFF();
        state++;
        break;
        
    case 2 :
        if(--blinks_remaining <= 0)
        {
            delay_counter = FLASH_PAUSE_CNTS;
            state++;
        }
        else
        {
            LED_RMT_FAULT_ON();
            state--;
        }
        break;

    case 3 :
        if(--delay_counter <= 0)
        {
            state = 0;
        }
        break;

    default :
        state = 0;
        break;
    } // switch
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//	Requirements taken from an email from Kirk Soldner May 12, 2017 and approved 
//  by Steve VanDyke on June 21, 2017. Details copied below for reference
//
//      From: Soldner, Kirk 
//      Sent: Friday, May 12, 2017 2:32 PM
//      To: Johnson, Gary; Van Dyke, Steve
//      Cc: Bina, Mark
//      Subject: RE: dsPIC NP battery type indication review
//
//	  - All existing legacy blink codes (colors + patterns) remain the same. 
//		This includes having 5 red blinks meaning two things:  battery probe 
//		open/shorted for 3-step chargers and charger disabled for Volta 
//		lithium-ion. Any new codes we define must be unique.
//
//	  -	For the PCA LED, we will define FOUR new codes that will be displayed 
//		for the first 5 minutes of power-up (inverter or charger mode) IF there 
//		are no faults.  The "5 minutes" is negotiable. Note that amber and 
//		green are not considered "fault" colors  only red is characterized as 
//		a fault.
//		  o AG for wet lead-acid (A = amber, G = green)
//		  o AGG for AGM
//		  o	AGGG for GEL
//		  o	AGGGG for lithium-ion (but arguably this is not needed since 
//			lithium-ion firmware is already unique for the model number see 
//			last bullet below)
//
//	  -	For the prior item, if faults are present initially or appear within the 
//		initial window (e.g. overtemp) then fault display will take precedence.
//
//	  -	None of this applies to Volta firmware, but it does apply to generic NP 
//		firmware. It is possible that we could eventually have lithium-ion 
//		users who are not Volta, although I don't know that we could get away 
//		with generic voltage levels anyway.
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//  Led.count values:
//      -1 = off
//       0 = On - no blinks
//       n = n-blinks 
//
//-----------------------------------------------------------------------------

// LED driver state machine states
#define LED_STATE_BATT_INIT      0
#define LED_STATE_BATT_AMBER_ON  1
#define LED_STATE_BATT_LED_OFF1  2
#define LED_STATE_BATT_GREEN_CFG 3
#define LED_STATE_BATT_GREEN_ON  4
#define LED_STATE_BATT_LED_OFF2  5
#define LED_STATE_BATT_LOOP_BACK 6
#define LED_STATE_NORM_INIT      7
#define LED_STATE_LED_ON         8
#define LED_STATE_LED_OFF        9
#define LED_STATE_LOOP_BACK      10

//  called once per millisecond.
static void _led_LocalDriver(int8_t reset)
{
  #ifdef OPTION_HAS_CHARGER
    static int16_t  led_state = LED_STATE_BATT_INIT;
  #else
    static int16_t  led_state = LED_STATE_NORM_INIT;
  #endif    
	static int16_t	batt_timer       = BATT_TIMER_CNTS; // battery down counter; exit battery blink codes when zero (FLASH_TIMER_MS counts)
    static int16_t  msec_ticks       = 0; // millisecond up counter (reset at FLASH_TIMER_MS)
    static int16_t  delay_counter    = 0; // delay counter in FLASH_TIMER_MS times between blink sequences
    static int16_t  blinks_remaining = 0; // remaining blink flashes before sequence delay period

	if (reset)
	{
		//	Reset the normal state machine
		led_state        = 0;
		msec_ticks       = 0;
	    delay_counter    = 0;
	    blinks_remaining = 0;
		return;
	}
		
    // Divide time into FLASH_TIMER_MS counts
    if (++msec_ticks < FLASH_TIMER_MS)
        return;

    // gets here every FLASH_TIMER_MS milliseconds
    msec_ticks = 0; // reset up counter

    // decrement battery timer; stay at zero if zero
	if (batt_timer) batt_timer--; // stay at zero if zero

    // LOG(SS_UI, SV_ERR, "LED state=%d btimer=%d blinks=%d delay=%d", led_state, batt_timer, blinks_remaining, delay_counter);

    // drive the state machine
    switch(led_state)
    {
	case LED_STATE_BATT_INIT:
		batt_timer = BATT_TIMER_CNTS;	
		led_state  = LED_STATE_BATT_AMBER_ON;
		// fall-thru

	case LED_STATE_BATT_AMBER_ON:
        LED_RED_ON();
        LED_GRN_ON();
		blinks_remaining = 1;	// one amber blink
		led_state = LED_STATE_BATT_LED_OFF1;
		break;
	
    case LED_STATE_BATT_LED_OFF1:
        LED_RED_OFF();
        LED_GRN_OFF();
        if (--blinks_remaining > 0) 
        {
            led_state = LED_STATE_BATT_AMBER_ON;
        }
        else
        {
            delay_counter = FLASH_PAUSE_CNTS;
            led_state = LED_STATE_BATT_GREEN_CFG;
        }
        break;

	case LED_STATE_BATT_GREEN_CFG:
        // determine blink count based upon battery type
        if (Chgr.config.battery_recipe.is_custom)
        {
            blinks_remaining = 5;
        }
        else
        {
		    switch(Chgr.config.battery_recipe.batt_type)
		    {
		    case BATT_WET: blinks_remaining = 1; break;
		    case BATT_AGM: blinks_remaining = 2; break;
		    case BATT_GEL: blinks_remaining = 3; break;
		    case BATT_LI:  blinks_remaining = 4; break;
		    default :	   blinks_remaining = 6; break;
            } // switch
        }
		// fall-thru
		
	case LED_STATE_BATT_GREEN_ON:
        LED_RED_OFF();
        LED_GRN_ON();
		led_state = LED_STATE_BATT_LED_OFF2;
		break;

    case LED_STATE_BATT_LED_OFF2:
        LED_RED_OFF();
        LED_GRN_OFF();
        if (--blinks_remaining > 0) 
        {
            led_state = LED_STATE_BATT_GREEN_ON;
        }
        else
        {
            delay_counter = FLASH_PAUSE_CNTS;
            led_state = LED_STATE_LOOP_BACK;
        }
        break;

    case LED_STATE_NORM_INIT:
        LED_RED_OFF();
        LED_GRN_OFF();
        // wait here until a flash code is configured.
        if (_Local.count >= 0)
        {
            blinks_remaining = _Local.count; 
            led_state = LED_STATE_LED_ON;
        }
        break;

    case LED_STATE_LED_ON:
        LED_RED_SET(_Local.red);
        LED_GRN_SET(_Local.grn);
		//	Led.count == 0 means steady-state "On"
        if (blinks_remaining > 0)
            led_state = LED_STATE_LED_OFF;
        else if(_Local.count != 0)
            led_state = LED_STATE_NORM_INIT;
        break;

    case LED_STATE_LED_OFF:
        LED_RED_OFF();
        LED_GRN_OFF();
        if (--blinks_remaining > 0) 
        {
            led_state = LED_STATE_LED_ON;
        }
        else
        {
            delay_counter = FLASH_PAUSE_CNTS;
            led_state = LED_STATE_LOOP_BACK;
        }
        break;

    case LED_STATE_LOOP_BACK:
        if (--delay_counter <= 0)
        {    
            //	Test for fault:  "red is characterized as a fault"
            //  Test for Red [only] led.
            if ( ((_Local.red != 0) && (_Local.grn == 0)) || 
                //  Test for other errors that preempt battery indication.
				HasInvAnyErrors()   || HasChgrAnyErrors()   || HasDevAnyErrors() ||
                IsBattTempSnsOpen() || IsBattTempSnsShort() || ChgrState() == CS_WARM_BATT)
                // IsInvFeedbackReversed() || IsInvFeedbackProblem() || 
                // device_error_battery_low() || IsXfrmOverTemp() )
            {
                //	Expire the timer to halt battery indication.
                batt_timer = 0;	
            }
            // back to battery state if not expired yet; else normal state		
            led_state = (0 == batt_timer) ? LED_STATE_NORM_INIT : LED_STATE_BATT_AMBER_ON;
        }
        break;

    default :
        led_state = LED_STATE_BATT_INIT;
        break;
    } // switch
}

//-----------------------------------------------------------------------------
// TASK_ui_Driver()
//
// This function provides all LED signaling, and initiates messages to the 
// remote display.
//
//-----------------------------------------------------------------------------

//  must be called every millisecond to drive the blinking state machine
void TASK_ui_Driver(void)
{
	if (IsInTestMode())
	{
        //	Reset the normal state machine
        _led_LocalDriver(1);
        _led_RemoteDriver(1);
        
        switch(GetLedTestColor())	// 0=off, 1=red, 2=green, 3=amber
        {
        default:
        case 0 :	LED_RED_OFF();	LED_GRN_OFF();	LED_RMT_FAULT_OFF(); break;
        case 1 :	LED_RED_ON();	LED_GRN_OFF();	LED_RMT_FAULT_OFF(); break;
        case 2 :	LED_RED_OFF();	LED_GRN_ON();	LED_RMT_FAULT_OFF(); break;
        case 3 :	LED_RED_ON();	LED_GRN_ON();	LED_RMT_FAULT_OFF(); break;
        case 4 :	LED_RED_OFF();	LED_GRN_OFF();	LED_RMT_FAULT_ON();  break;
        } // switch
    	return;
	}
	
	_led_LocalDriver(0);
	_led_RemoteDriver(0);
    
    
    if(!HasInvRequest() && !HasChgrRequest())
    {
        //  All LEDs off
        LED_RMT_FAULT_OFF();
        LED_RED_OFF();
        LED_GRN_OFF();
    }
    else if (IsChgrActive())
    {
        // charging
        if      (IsChgrBattOverTemp())          LedSetup(UI_LED_YEL, 4, LED_FAST_BLINK);
        else if (ChgrState() == CS_CONST_CURR)  LedSetup(UI_LED_GRN, 1, LED_CONSTANT);
        else if (ChgrState() == CS_CONST_VOLT)  LedSetup(UI_LED_GRN, 2, LED_CONSTANT);
        else if (ChgrState() == CS_FLOAT)       LedSetup(UI_LED_GRN, 3, LED_CONSTANT);
        else if (ChgrState() == CS_LOAD_MGMT)   LedSetup(UI_LED_GRN, 4, LED_CONSTANT);
        else if (ChgrState() == CS_EQUALIZE)    LedSetup(UI_LED_GRN, 5, LED_CONSTANT);
		else if (ChgrState() == CS_MONITOR)     LedSetup(UI_LED_GRN, 6, LED_CONSTANT);
        else if (ChgrState() == CS_WARM_BATT)   LedSetup(UI_LED_YEL, 2, LED_FAST_BLINK);
        else                                    LedSetup(UI_LED_GRN, 3, LED_CONSTANT);
    }                                                       
    else if(IsInvActive())                          
    {                                                       
        // inverting                                        
        if      (IsInvLowBattDetected())        LedSetup(UI_LED_YEL, 0, LED_CONSTANT);
        else if (IsInvOverLoadDetected())       LedSetup(UI_LED_RED, 0, LED_CONSTANT);
        else if (IsInvHighBattDetected())       LedSetup(UI_LED_RED, 5, 5           );
        else                                    LedSetup(UI_LED_GRN, 0, LED_CONSTANT); // Normal operation
    }                                                       
    else                                                    
    {   // not inverting or charging                      
		if      (Is15VoltBad())                 LedSetup(UI_LED_RED, 1, 1);
		else if (IsAcLineQualified())
        {
			//	The charger is not running, indicate why; A/C line is qualified
            if      (IsBattTempSnsOpen() || 
                     IsBattTempSnsShort())           LedSetup(UI_LED_YEL, 1, LED_FAST_BLINK);
            else if (ChgrState()==CS_WARM_BATT)      LedSetup(UI_LED_YEL, 2, LED_FAST_BLINK);
            else if (IsChgrHighBattShutdown())       LedSetup(UI_LED_YEL, 3, LED_FAST_BLINK); // ### never happens; no one sets
            else if (IsChgrBattOverTemp())           LedSetup(UI_LED_YEL, 4, LED_FAST_BLINK);
            else if (device_error_battery_low())     LedSetup(UI_LED_YEL, 5, LED_FAST_BLINK);
            else if (IsXfrmOverTemp())               LedSetup(UI_LED_YEL, 6, LED_FAST_BLINK);
            else if (IsHeatSinkOverTemp())           LedSetup(UI_LED_YEL, 7, LED_FAST_BLINK);
            else if (!IsChgrEnByBcr())               LedSetup(UI_LED_YEL, 8, LED_FAST_BLINK);
            else if (IsChgrOverCurrShutdown())	     LedSetup(UI_LED_YEL, 9, LED_FAST_BLINK);
            else if (!IsChgrEnByCan())               LedSetup(UI_LED_YEL,10, LED_FAST_BLINK);
        }
		else if (HasInvRequest())
		{
			//	The inverter is not running, indicate why; inverter request is enabled
            if      (IsInvLowBattShutdown())	  LedSetup(UI_LED_RED, 1, 1);
            else if (IsInvOverLoadShutdown())     LedSetup(UI_LED_RED, 2, 2);
            else if (IsHeatSinkOverTemp())        LedSetup(UI_LED_RED, 3, 3);
            else if (IsInvFeedbackReversed() ||
                     IsInvFeedbackProblem())      LedSetup(UI_LED_RED, 4, 4);
            else if (IsInvHighBattShutdown())     LedSetup(UI_LED_RED, 6, 6);
            else if (IsXfrmOverTemp())            LedSetup(UI_LED_RED, 7, 7);
            else if (!IsInvEnabled())             LedSetup(UI_LED_RED, 8, 8); // disabled by CAN
            else if (IsInvLowBattDetected())      LedSetup(UI_LED_YEL, 0, LED_CONSTANT);
            else if (IsInvOverLoadDetected())     LedSetup(UI_LED_RED, 0, LED_CONSTANT);
            else if (IsInvHighBattDetected())     LedSetup(UI_LED_RED, 5, 5);
		}
        else LedSetup(UI_LED_OFF, 0, LED_OFF);
    }
}

//-----------------------------------------------------------------------------
void ui_Config(void)
{
    _Local.count = 0;
    _Local.red = 0;
    _Local.grn = 0;
}

//-----------------------------------------------------------------------------
void ui_Start(void)
{
}

// <><><><><><><><><><><><><> ui.c <><><><><><><><><><><><><><><><><><><><><><>
