// <><><><><><><><><><><><><> lpg_cfg.h <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//	Copyright(C) 2013 - Sensata Technologies, Inc.  All rights reserved.
//
//	$Workfile: lpc_cfg.h $
//	$Author: A1021170 $
//	$Date: 2018-02-23 14:21:29-06:00 $
//	$Revision: 6 $ 
//
// 	LPC Device Configuration Parameters
//
//=============================================================================

#ifndef __LPC_CFG_H
#define __LPC_CFG_H

// -------
// headers
// -------
#include "options.h"
#include "analog.h"

// ------------------------
// LPC Configuration Values
// ------------------------

// time-outs
#define ERROR_SHUTDOWN_TIMEOUT_SEC	   (15*60)  // 15 minutes (in seconds)
#define CHARGER_IDLE_TIMEOUT_SEC	    (2*60)  // 2-minutes  (in seconds)
#define INVERTER_IDLE_TIMEOUT_SEC	      (30)  // 30-seconds
#define RELAY_CLOSE_DELAY_MSEC	          (16)  // milliseconds
#define RELAY_OPEN_DELAY_MSEC	          (16)  // milliseconds
#define BOOT_CHARGE_DELAY_MSEC            (50)  // milliseconds

// Charger Over-Current Check parameters TODO ### readjust values via testing
#define CHGR_OC_CHECK_COUNT		         (200)  // 200-milliseconds
#define CHGR_OC_CHECK_THRES 	     (IMEAS_CHG_AMPS_ADC(10.0))  // adc counts (specify in A/C amps)
#define CHGR_OC_CHECK_HI_THRES 	     (IMEAS_CHG_AMPS_ADC(20.0))  // adc counts (specify in A/C amps)


#endif 	//	__LPC_CFG_H

// <><><><><><><><><><><><><> lpg_cfg.h <><><><><><><><><><><><><><><><><><><><><><>
