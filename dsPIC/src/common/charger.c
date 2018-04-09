// <><><><><><><><><><><><><> charger.c <><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//  Copyright(C) 2017 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//  $Workfile: charger.c $
//  $Author: A1021170 $
//  $Date: 2018-04-03 15:13:27-05:00 $
//  $Revision: 138 $ 
//
//  Charger Interface functions
//
//-----------------------------------------------------------------------------

// -------
// headers
// -------
#include "options.h"    // must be first include
#include "charger.h"
#include "pwm.h"

//	Make this code compile even if we don't have a charger
#if !defined(OPTION_CHARGE_3STEP) && !defined(OPTION_CHARGE_LION)
  #ifndef WIN32
	#pragma message "Charger is NOT DEFINED!!"
  #endif
	//	stub functions 
	void chgr_Driver(int8_t reset) {};
	void chgr_ChargeCycleReset(void) {};
#endif


// -----------
// global data
// -----------
CHARGER_t Chgr; // charger data

int8_t  chgr_StopRequest = 0; //
uint8_t chgr_hasRun = 0;


//-----------------------------------------------------------------------------
//  chgr_Driver
//-----------------------------------------------------------------------------
//  chgr_Driver is the (public) high-level state machine that controls the 
//  charger mode. The lower-level chgr_Driver() accommodates a parameter to 
//	reset its internal state machine.
//-----------------------------------------------------------------------------

void TASK_chgr_Driver(void)
{
    if(IsChgrActive())
    {
        chgr_Driver(0);
    }
}

//-----------------------------------------------------------------------------
//  CHARGER OPERATION 
//
//  Flags:
//      Chgr status charger_active - indicates we are in CHARGER mode and the 
//          Charger Driver [chgr_Driver()] is running. The CHARGER may be 
//          Shutdown, not producing output, but still running. For example, 
//          waiting for a fault to clear.
//      Chgr status output_active - indicates that the CHARGER is producing
//          DC output.
//
//  There is a UL requirement that the output be 'Off' when we indicate that we
//  are shutdown. That means that we have to deactivate the 
//  Xfer/Charger Relay. Therefore, if we encounter an
//      issue with the charger, and we shutdown, then we need to deactivate the
//      Charger Relay. Also, we cannot (re-)activate the charger relay while 
//      the issue exists.  
//
//      This functionality allows the main control loop to poll the status of
//      the Charger, to assure that no issues exist, before activating the 
//      Charger Relay.
//-----------------------------------------------------------------------------

void chgr_Start(void)
{
    LOG(SS_CHG, SV_INFO, "*** chgr_Start");
    pwm_Stop();
    ChgrClearErrors();   // clear all faults and errors.
    chgr_Driver(1);      // reset the driver
    _chgr_pwm_isr(1);    // reset the charger PWM ISR
    pwm_Config(PWM_CONFIG_CHARGER, &chgr_PwmIsr);
    pwm_Start();

    chgr_StopRequest = 0;
    SetChgrActive();
    chgr_hasRun = 1;
}

//-----------------------------------------------------------------------------
void chgr_Stop(void)
{
    LOG(SS_CHG, SV_INFO, "*** chgr_Stop");
    chgr_StopRequest = 1;
}

//-----------------------------------------------------------------------------
void chgr_StopNow(void)
{
    LOG(SS_CHG, SV_INFO, "*** chgr_StopNow");
	chgr_StopRequest = 0;

    pwm_Stop();
	pwm_Config(PWM_CONFIG_DEFAULT, NULL);
	pwm_Start();

    Chgr.status.charger_active = 0;
    Chgr.status.output_active = 0;
}

//-----------------------------------------------------------------------------
#define DEBUG_LPC_CHGR_MAX_AMPS  1

// return charger max a/c current allowed in A/D counts
uint16_t ChgrCfgMaxAcCurrA2D()
{
  #if IS_PCB_LPC
	// ------------
	//     LPC 
	// ------------
	// LPC current is limited based upon supply voltage
   #ifdef DEBUG_LPC_CHGR_MAX_AMPS
	static int8_t amp_last = 0; // 1=low, 2=full
   #endif // DEBUG_LPC_CHGR_MAX_AMPS
	if (IsChgrVBattLowPower())
	{
      #ifdef DEBUG_LPC_CHGR_MAX_AMPS
		if (amp_last != 1)
		{
			LOG(SS_CHG, SV_DBG, "Chgr Low Power: VBatt=%u Thresh=%u", VBattCycleAvg(), LPC_CHGR_VBATT_LOW_POWER);
			amp_last = 1;
		}
      #endif // DEBUG_LPC_CHGR_MAX_AMPS
		return(LPC_CHGR_LOW_POWER_AMPS);
	}
	// full power
  #ifdef DEBUG_LPC_CHGR_MAX_AMPS
	if (amp_last != 2)
	{
		LOG(SS_CHG, SV_DBG, "Chgr Full Power: VBatt=%u Thresh=%u", VBattCycleAvg(), LPC_CHGR_VBATT_LOW_POWER);
        amp_last = 2;
	}
  #endif // DEBUG_LPC_CHGR_MAX_AMPS
	return(Chgr.config.amps_limit);
  #else
	// ------------
	//     NP 
	// ------------
	// NP can handle the max current at all times
	return(Chgr.config.amps_limit);
  #endif
}

// <><><><><><><><><><><><><> charger.c <><><><><><><><><><><><><><><><><><><><>
