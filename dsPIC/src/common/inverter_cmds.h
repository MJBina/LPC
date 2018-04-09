// <><><><><><><><><><><><><> inverter_cmds.h <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//	Copyright(C) 2015 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
//	Inverter commands used by remote communications
//	
//-----------------------------------------------------------------------------

#ifndef __INVERTER_CMDS_H__    // include only once
#define __INVERTER_CMDS_H__

// -------
// headers
// -------
#include "options.h"    // must be first include
#include "rv_can.h"


// -----------
// Prototyping
// -----------
extern void     inv_ClearFaults(void);
extern uint8_t  inv_GetBatteryTempSensorPresent(void);
extern uint8_t  inv_GetRvcStatus(void);
extern void     inv_LogErrorFlags(void);
extern void     inv_LogStatusFlags(void);
extern void     inv_Reboot(void);
extern void     inv_RestoreToDefaults(void);

extern void		inv_SetPGain(uint8_t * val);
extern void		inv_SetIGain(uint8_t * val);
extern void		inv_SetDGain(uint8_t * val);
extern void		inv_SetILimit(int16_t limit);

extern void     inv_SetLoadSenseEnabled(int enable);
extern void 	inv_SetLoadSenseDelay(uint16_t delay);
extern void     inv_SetLoadSenseEnabledOnStartup(int enable);
extern void 	inv_SetLoadSenseInterval(uint16_t interval);
extern void 	inv_SetLoadSenseThreshold(int16_t threshold);

extern void		inv_SetSupplyLowShutdown(uint16_t adc_count);
extern void		inv_SetSupplyLowThreshold(uint16_t adc_count);
extern void		inv_SetSupplyLowRecover(uint16_t adc_count);
extern void		inv_SetSupplyHighRecover(uint16_t adc_count);
extern void		inv_SetSupplyHighThreshold(uint16_t adc_count);
extern void		inv_SetSupplyHighShutdown(uint16_t adc_count);


//  rv_can.h
extern void     inv_GetAcFaultStatus1(RVCS_AC_FAULT_STATUS_1* status);
extern void     inv_GetAcFaultStatus2(RVCS_AC_FAULT_STATUS_2* status);
extern void     inv_GetAcStatus1(RVCS_AC_PNT_PG1* status);
extern void     inv_GetAcStatus2(RVCS_AC_PNT_PG2* status);
extern void     inv_GetAcStatus3(RVCS_AC_PNT_PG3* status);
extern void     inv_GetAcStatus4(RVCS_AC_PNT_PG4* status);
extern void     inv_GetConfigStatus1(RVCS_INV_CFG_STATUS1* status);
extern void     inv_GetConfigStatus2(RVCS_INV_CFG_STATUS2* status);
extern void     inv_GetDM_RV(RVCS_DM_RV* status);
extern void     inv_GetDcSourceStatus1(RVCS_DC_SOURCE_STATUS_1* status);
extern void     inv_GetDcSourceStatus2(RVCS_DC_SOURCE_STATUS_2* status);
extern void     inv_GetDcSourceStatus3(RVCS_DC_SOURCE_STATUS_3* status);
extern void     inv_GetDcStatus(RVCS_INV_DC_STATUS* status);
extern void     inv_GetInstStatus(RVCS_INSTANCE_STATUS* status);
extern void     inv_GetStatus(RVCS_INV_STATUS* status);


//	device
extern uint8_t  dev_GetInverterEnabled(void);
extern void     dev_SetInverterEnabled(int enable);
extern void     dev_SetInverterEnabledOnStartup(int enable);
extern void     dev_SetPassThruEnabled(int enable);
extern void     dev_SetPassThruEnabledOnStartup(int enable);

extern uint16_t dev_GetTimerShutdownDelay(void);
extern uint8_t  dev_GetTimerShutdownEnabled(void);
extern uint16_t dev_GetTimerShutdownTimer(void);
extern void 	dev_SetTimerShutdownDelay(uint16_t delay);
extern void     dev_SetTimerShutdownEnabled(int enable);
extern void     dev_SetTimerShutdownEnabledOnStartup(int enable);


#endif // __INVERTER_CMDS_H__

// <><><><><><><><><><><><><> inverter_cmds.h <><><><><><><><><><><><><><><><><><><><><><>
