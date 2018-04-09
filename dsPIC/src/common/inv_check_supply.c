// <><><><><><><><><><><><><> inv_check_supply.c <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//  Copyright(C) 2013 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
//  Check supply voltage and compare against thresholds
//
//-----------------------------------------------------------------------------

// -------
// headers
// -------
#include "options.h"    // must be first include
#include "hw.h"
#include "analog.h"
#include "inverter.h"

// --------------------------
// Conditional Debug Compiles
// --------------------------

// comment out flag to not use it
//#define  DEBUG_CHK_SUPPLY  1 // debug check supply


// -----------------------------
// Auto Removal for build types
// -----------------------------
#if OPTION_NO_CONDITIONAL_DBG
  #undef DEBUG_CHK_SUPPLY
#endif


// -------
// enums
// -------
typedef enum
{
    SUPPLY_STATE_NORMAL = 0,
    SUPPLY_STATE_LOW_DETECTED,
    SUPPLY_STATE_LOW_SHUTDOWN,
    SUPPLY_STATE_HIGH_DETECTED,
    SUPPLY_STATE_HIGH_SHUTDOWN
} SUPPLY_STATE_t;

// ------------
// global data
// ------------

#ifdef DEBUG_CHK_SUPPLY
 const char* supply_state_str[] =
 {   // CAUTION! must match SUPPLY_STATE_t
     "NORMAL",         	  
     "LO_BATT_DETECT",           
     "LO_BATT_SHUTDN",   
     "HI_BATT_DETECT",
     "HI_BATT_SHUTDN",   
 };
#endif


//-----------------------------------------------------------------------------
// called once-per-millisecond.

void inv_CheckSupply(int8_t reset)
{
  #if defined(OPTION_UL_TEST_CONFIG)
	Inv.error.supply_high_shutdown  = 0;
	Inv.status.supply_high_detected = 0;
	Inv.status.supply_low_detected  = 0;
	Inv.error.supply_low_shutdown   = 0;
	return;
  #else

	static SUPPLY_STATE_t supply_state = SUPPLY_STATE_NORMAL;
	
	static int16_t  _millisecond_tick = 0;
    static int16_t  _SupplyHighDetectTimer = 0;
	#define SUPPLY_HIGH_DETECT_TIMEOUT_SECONDS  (5*60)

    static int16_t  _SupplyLowDetectCount = 0;
	#define INV_DFLT_SUPPLY_LOW_DETECT_COUNT_LIMIT   (5000)
    static int16_t  _SupplyLowResetCount = 0;
	#define INV_DFLT_SUPPLY_LOW_RESET_COUNT_LIMIT    (100)

    static int16_t  _SupplyLowShutdownCount = 0;
	#define INV_DFLT_SUPPLY_LOW_SHUTDOWN_COUNT_LIMIT (1000)

  #ifdef DEBUG_CHK_SUPPLY
    {
		// show state change
        static int16_t last_supply_state = SUPPLY_STATE_NORMAL;
        if (last_supply_state != supply_state)
	    {
			LOG(SS_SYS, SV_DBG, "Supply: %s->%s VBatt=%u Lo=%d Hi=%d",
				supply_state_str[last_supply_state], supply_state_str[supply_state],
				VBattCycleAvg(), InvCfgVBattLoThresh(), InvCfgVBattHiThresh());
            last_supply_state = supply_state;
	    }
		else
		{
			static uint16_t log_cnt = 0;
			if (++log_cnt>250)
			{
				log_cnt = 0;
				LOG(SS_SYS, SV_DBG, "Supply: %s VBatt=%u Lo=%d Hi=%d", supply_state_str[supply_state], VBattCycleAvg(), InvCfgVBattLoThresh(), InvCfgVBattHiThresh());
			}
		}
    }
  #endif

    if(reset)
    {
        Inv.error.supply_high_shutdown  = 0;
        Inv.status.supply_high_detected = 0;
        Inv.status.supply_low_detected  = 0;
        Inv.error.supply_low_shutdown   = 0;

        supply_state = SUPPLY_STATE_NORMAL;
        return;
    }

    switch(supply_state)
    {
    case SUPPLY_STATE_LOW_SHUTDOWN:
        Inv.error.supply_low_shutdown = 1;

        if(VBattCycleAvg() > InvCfgVBattLoRecover())
        {
            supply_state = SUPPLY_STATE_NORMAL;
        }
        break;

    case SUPPLY_STATE_LOW_DETECTED:
        Inv.status.supply_low_detected = 1;

        if(VBattCycleAvg() < InvCfgVBattLoShutDown())
        {
	      #if !defined(OPTION_SSR)
            if (++_SupplyLowShutdownCount > INV_DFLT_SUPPLY_LOW_SHUTDOWN_COUNT_LIMIT)
		  #endif
            {
                _SupplyLowShutdownCount = 0;
                supply_state = SUPPLY_STATE_LOW_SHUTDOWN;
            }
        }
        else
        {
            _SupplyLowShutdownCount = 0;
        }

        if (VBattCycleAvg() > InvCfgVBattLoHyster())
        {
            if (--_SupplyLowResetCount < 0)
            {
                _SupplyLowResetCount = 0;
                _SupplyLowDetectCount = 0;
            }
            if (--_SupplyLowDetectCount < 0)
            {
                _SupplyLowDetectCount = 0;
                supply_state = SUPPLY_STATE_NORMAL;
            }
        }
        else
        {
            if (++_SupplyLowDetectCount > INV_DFLT_SUPPLY_LOW_DETECT_COUNT_LIMIT)
            {
                _SupplyLowDetectCount = INV_DFLT_SUPPLY_LOW_RESET_COUNT_LIMIT;
                supply_state = SUPPLY_STATE_LOW_SHUTDOWN;
            }
        }
        break;

    case SUPPLY_STATE_NORMAL:
        Inv.error.supply_high_shutdown  = 0;
        Inv.status.supply_high_detected = 0;
        Inv.status.supply_low_detected  = 0;
        Inv.error.supply_low_shutdown   = 0;

        if(VBattCycleAvg() > InvCfgVBattHiThresh())
        {
            _millisecond_tick = 0;
            _SupplyHighDetectTimer = 0;
            supply_state = SUPPLY_STATE_HIGH_DETECTED;
        }
        else if(VBattCycleAvg() < InvCfgVBattLoThresh())
        {
            _SupplyLowResetCount = INV_DFLT_SUPPLY_LOW_RESET_COUNT_LIMIT;
            _SupplyLowDetectCount = 0;
            supply_state = SUPPLY_STATE_LOW_DETECTED;
        }
        break;

    case SUPPLY_STATE_HIGH_DETECTED:
        //  Shutdown if:    Supply Voltage > Threshold1 for > 5-min  -OR-
        //                  Supply Voltage > Threshold2
        Inv.status.supply_high_detected = 1;
		
	#if !defined(OPTION_SSR)
        if(VBattCycleAvg() > InvCfgVBattHiShutDown())
        {
            supply_state = SUPPLY_STATE_HIGH_SHUTDOWN;
        }
        else if(VBattCycleAvg() > InvCfgVBattHiThresh())
        {
            if(++_millisecond_tick > 1000)
            {
                _millisecond_tick = 0;
                if(++_SupplyHighDetectTimer >= SUPPLY_HIGH_DETECT_TIMEOUT_SECONDS)
                {
                    _SupplyHighDetectTimer = 0;
                    supply_state = SUPPLY_STATE_HIGH_SHUTDOWN;
                }
            }
        }
        else if(VBattCycleAvg() < InvCfgVBattHiRecover())
	#else	
        if(VBattCycleAvg() < InvCfgVBattHiRecover())
	#endif
        {
            supply_state = SUPPLY_STATE_NORMAL;
        }
        break;

    case SUPPLY_STATE_HIGH_SHUTDOWN:
        Inv.error.supply_high_shutdown = 1;
        if(VBattCycleAvg() < InvCfgVBattHiRecover())
        {
            supply_state = SUPPLY_STATE_NORMAL;
        }
        break;
    }
    return;
    
  #endif	//	OPTION_UL_TEST_CONFIG
}

// <><><><><><><><><><><><><> inv_check_supply.c <><><><><><><><><><><><><><><><><><><><><><>
