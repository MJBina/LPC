// <><><><><><><><><><><><><> charger_cmds.h <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//	Copyright(C) 2015 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
//  Charger remote command interface
//	
//-----------------------------------------------------------------------------

#ifndef __CHARGER_CMDS_H_    // include only once
#define __CHARGER_CMDS_H_

// -------
// headers
// -------
#include "options.h"    // must be first include
#include "rv_can.h"
#include "battery_recipe.h"


// ------------------
// Charger Algorithms
// ------------------
// from CHRG_GetChargingAlgorithm()
#define CHGR_ALGORITHM_CONSTANT_VOLTAGE		0
#define CHGR_ALGORITHM_CONSTANT_CURRENT		1
#define CHGR_ALGORITHM_3STAGE				2
#define CHGR_ALGORITHM_2STAGE				3
#define CHGR_ALGORITHM_TRICKLE				4
#define CHGR_ALGORITHM_CUSTOM_2			  249
#define CHGR_ALGORITHM_CUSTOM_1			  250


// -------------
// Charger Modes
// -------------
// from CHRG_GetChargerMode()
#define CHGR_MODE_STAND_ALONE		0
#define CHGR_MODE_PRIMARY			1
#define CHGR_MODE_SECONDARY			2

// -------------
// Battery Types
// -------------
// from CHRG_GetBatteryType()
#define CHGR_BATTERY_TYPE_FLOODED	 0
#define CHGR_BATTERY_TYPE_GEL		 1
#define CHGR_BATTERY_TYPE_AGM	  	 2
#define CHGR_BATTERY_TYPE_CUSTOM_2	28
#define CHGR_BATTERY_TYPE_CUSTOM_1	29


// -----------
// Prototyping
// -----------
extern uint8_t chgr_GetChargingAlgorithm(void); // CHGR_ALGORITHM_nnn
extern uint8_t chgr_GetRvcState(void);
extern void    chgr_GetStatusInfo(RVCS_CHARGER_STATUS* status);
extern void    chgr_GetConfigStatus(RVCS_CHARGER_CFG_STATUS* status);
extern void    chgr_GetConfigStatus2(RVCS_CHARGER_CFG_STATUS2* status);
extern void    chgr_GetEqualStatus(RVCS_CHARGER_EQUAL_STATUS* status);
extern void    chgr_GetAcStatus1(RVCS_AC_PNT_PG1* status);
extern void    chgr_GetAcStatus2(RVCS_AC_PNT_PG2* status);
extern void    chgr_GetAcStatus3(RVCS_AC_PNT_PG3* status);
extern void    chgr_GetAcStatus4(RVCS_AC_PNT_PG4* status);
extern void    chgr_GetAcFaultStatus1(RVCS_AC_FAULT_STATUS_1* status);
extern void    chgr_GetAcFaultStatus2(RVCS_AC_FAULT_STATUS_2* status);
extern void    chgr_SetAmpsLimit(float amps_limit);
extern float   chgr_GetAmpsLimit(void);
extern void    chgr_SetBattType(uint16_t batteryType, uint8_t forced);
extern void    chgr_SetBattCustomFlag(uint16_t isCustom);
extern void    chgr_SetBranchCircuitAmps(int16_t amps);
extern void    chgr_SetCfgCmd2(uint8_t chargePercent, uint8_t chargeRatePercent, uint8_t breakerSize);
extern void	   chgr_SetCfgCmd(uint8_t chargingAlgorithm,uint8_t chargerMode,
			      	uint8_t batterySensePresent, uint8_t installationLine,
			      	uint16_t batteryBankSize, uint16_t batteryType, 
			      	uint8_t maxChargingCurrent);

extern void    	chgr_StartEqualization(void);
extern BATTERY_RECIPE_t* PtrRomBattRecipe(BATT_TYPE_t batteryType);


#endif // __CHARGER_CMDS_H_

// <><><><><><><><><><><><><> charger_cmds.h <><><><><><><><><><><><><><><><><><><><><><>
