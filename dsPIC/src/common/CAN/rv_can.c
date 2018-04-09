// <><><><><><><><><><><><><> rv_can.c <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//  Copyright(C) 2015 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
//  RV-C CAN bus protocol 
//  
//-----------------------------------------------------------------------------

// --------
// headers
// --------
#include "options.h"    // must be first include
#include "analog.h"
#include "charger.h"
#include "charger_cmds.h"
#include "config.h"
#include "dsPIC33_CAN.h"
#include "inverter.h"
#include "inverter_cmds.h"
#include "rv_can.h"
#include "sensata_can.h"
#include "nvm.h"
#include "hs_temp.h"
#include "tasker.h"


// ----------------------------------------------------------------------
//                  B A S I C    M E S S A G E S
// ----------------------------------------------------------------------

// is given instance for this device
// returns 0=no, 1=cmd is for me,
int IsRvcCmdForMe(uint8_t instance)
{
    if (instance == g_can.MyInstance) return(1);
    if (instance == RVC_BROADCAST_INSTANCE) return(1);
    return(0);
}

// ----------------------------------------------------------------------
static int8_t s_rvcProductIdStr[100] = { "Sensata*" MODEL_NO_STRING "***" };  // default; gets written over

// build the RVC product ID string
// contains serial number so needs to be build from mfg data
void rvcan_BuildProductId()
{
    // Make*Model*SerialNum*Ver*   must be separated by astericks   RVC 3.2.8
    sprintf(s_rvcProductIdStr, "Sensata*%s*%s*%s*", g_RomMfgInfo.model_no, g_RomMfgInfo.serial_no, g_RomMfgInfo.rev);
    LOG(SS_CAN, SV_INFO, "ProductId: '%s'", s_rvcProductIdStr);
}

// ----------------------------------------------------------------------
// returns the RVC/J1939 product id string
uint8_t* rvcan_GetProductIdString()
{
    //  must be separated by astericks
    return((uint8_t*)s_rvcProductIdStr);
}

// ----------------------------------------------------------------------
// RVC 3.3.4  pg 22
// send manufacturer specific claim request
void rvcan_SendMfgAddressClaimRequest(uint16_t mfgCode)
{
    CAN_DATA data[2];
    CAN_LEN  ndata = sizeof(data);

    data[0] = (mfgCode & 0x7);
    data[1] = (mfgCode >> 3) & 0xFF;

    LOGX(SS_J1939, SV_INFO, "SendMfgAddressClaimReq: ", data, ndata);

    J1939_SendMessage(RVC_DGN_MFG_SPECIFIC_CLAIM_REQUEST, ndata, data);
}

// ----------------------------------------------------------------------
// RVC 3.2.5.1
// Send Diagnostics DM_RV message
void rvcan_Send_DM_RV()
{
    CAN_DATA data[sizeof(RVCS_DM_RV)];
    CAN_LEN  ndata = sizeof(data);

    inv_GetDM_RV((RVCS_DM_RV*)&data);

    LOGX(SS_J1939, SV_INFO, "Send_DM_RV: ", data, ndata);

    J1939_SendMessage(RVC_DGN_DM_RV, ndata, data);
}

// ----------------------------------------------------------------------
// RVC 3.2.5.1
// Send Generic Config Status
void rvcan_SendGenericCfgStatus()
{
    CAN_DATA data[sizeof(RVCS_GENERIC_CFG_STATUS)];
    CAN_LEN  ndata = sizeof(data);
    RVCS_GENERIC_CFG_STATUS* status = (RVCS_GENERIC_CFG_STATUS*)&data[0];

    // TODO set these
    status->mfgCodeLSB   = 0;  
    status->mfgCodeMSG   = 0;   
    status->funcInstance = 0;  
    status->function     = 0;     
    status->fwRevision   = 0;   
    status->cfgTypeLSB   = 0;   
    status->cfgTypeISB   = 0;   
    status->cfgTypeMSB   = 0;   
    status->cfgRevision  = 0;  

    J1939_SendMessage(RVC_DGN_GENERIC_CONFIGURATION_STATUS, ndata, data);
}

// ----------------------------------------------------------------------
// RVC 3.2.8
// Send Product ID as string  Make*Model*SerialNumber*UnitNumber*  (must be separated by astericks)
void rvcan_SendProductID(uint8_t* MakeModelSnUnit)
{
    uint16_t ndata = strlen((char*)MakeModelSnUnit);

    LOG(SS_RVC, SV_INFO, "SendProductID: %s", MakeModelSnUnit);
    J1939_SendMultiPacketMessage(RVC_DGN_PRODUCT_IDENTIFICATION, ndata, MakeModelSnUnit);
}

// ----------------------------------------------------------------------
// RVC 6.4.2
// Send Date/Time status
void rvcan_Send_DateTimeStatus(uint8_t yearOffset,  // 0 = 2000, 1=2001, etc 
                               uint8_t month,       // 1=Jan, 2=Feb, etc 
                               uint8_t date,        // 0-31 
                               uint8_t dayOfWeek,   // 1=Sun, 2=Mon, etc
                               uint8_t hour,        // 0-23  0=12AM  12=12Noon
                               uint8_t minute,      // 0-59
                               uint8_t second,      // 0-59
                               uint8_t timeZone     // 0=GMT hours offset EST=5 
                               )
{
    CAN_DATA data[8];
    CAN_LEN  ndata = sizeof(data);

    data[0] = yearOffset;
    data[1] = month;
    data[2] = date;
    data[3] = dayOfWeek;
    data[4] = hour;
    data[5] = minute;
    data[6] = second;
    data[7] = timeZone;

    J1939_SendMessage(RVC_DGN_DATE_TIME_STATUS, ndata, data);
}

// ----------------------------------------------------------------------
// reset communication statistics
void rvcan_ResetCommStats()
{
    LOG(SS_INVCMD, SV_INFO, "Reset Comm Stats");

    // clear state counters and flags
    g_can.rvcRxErrCount      = 0;
    g_can.rvcTxErrCount      = 0;
    g_can.rvcTxFrameCount    = 0;  
    g_can.rvcRxFrameCount    = 0;  
    g_can.rvcBusOffErr       = 0;     
    g_can.rvcRxFramesDropped = 0;
    g_can.rvcTxFramesDropped = 0;
}

// ----------------------------------------------------------------------
// RVC 6.2.1  pg 32
void rvcan_GeneralReset(uint8_t isReboot,           // 0=no, 1=reboot cpu
                        uint8_t isClearFaults,      // 0=no, 1=clear faults
                        uint8_t isRestoreDefaults,  // 0=no, 1=restore to default values
                        uint8_t isResetCommStats)   // 0=no, 1=reset communication statistics
{
    LOG(SS_RVC, SV_INFO,"GenReset(reboot=%u clrFaults=%u rstDefaults=%u resetComm=%u)",
                        (int)isReboot, (int)isClearFaults, (int)isRestoreDefaults, isResetCommStats);

    if (isClearFaults)     inv_ClearFaults();
    if (isRestoreDefaults) inv_RestoreToDefaults();
    if (isResetCommStats)  rvcan_ResetCommStats();
    J1939_SendAck(RVC_DGN_GENERAL_RESET);
    if (isReboot) inv_Reboot();
}

// ----------------------------------------------------------------------
// RVC 6.2.4  pg 36
void rvcan_SendInstanceStatus()
{
    CAN_DATA data[sizeof(RVCS_INSTANCE_STATUS)];
    CAN_LEN  ndata = sizeof(data);

    inv_GetInstStatus((RVCS_INSTANCE_STATUS*)&data[0]);

    LOGX(SS_RVC, SV_INFO, "SendInstStat: ", data, ndata);

    J1939_SendMessage(RVC_DGN_INSTANCE_STATUS, ndata, data);
}

// ----------------------------------------------------------------------
// RVC 6.2.4  pg 34
void rvcan_InstanceAssignment(uint8_t deviceType, uint8_t baseInstance,uint8_t maxInstance,
                              uint16_t baseIntAddr,uint16_t maxIntAddr)
{
    LOG(SS_RVC, SV_INFO,"instanceAssign(devtype=%u baseInst=%u maxInst=%u baseIntAddr=%u maxIntAddr=%u)",
                        (int)deviceType, (int)baseInstance, (int)maxInstance, (int)baseIntAddr, (int)maxIntAddr);
    // TODO needs work
    rvcan_SendInstanceStatus();
}

// ----------------------------------------------------------------------
//      C O M M U N I C A T I O N S   S T A T U S    M E S S A G E S
// ----------------------------------------------------------------------

// RVC 6.6.2
// Send Comm Status 1
void rvcan_SendCommStatus1()
{
    CAN_DATA data[8];
    CAN_LEN  ndata = sizeof(data);
    uint32_t timerCount = GetSysTicks();

    data[0] = (uint8_t)(timerCount    );
    data[1] = (uint8_t)(timerCount>> 8);
    data[2] = (uint8_t)(timerCount>>16);
    data[3] = (uint8_t)(timerCount>>24);
    data[4] = LOBYTE(g_can.rvcRxErrCount);
    data[5] = HIBYTE(g_can.rvcRxErrCount);
    data[6] = LOBYTE(g_can.rvcTxErrCount);
    data[7] = HIBYTE(g_can.rvcTxErrCount);

    LOGX(SS_RVC, SV_INFO, "SendCommStatus1: ", data, ndata);

    J1939_SendMessage(RVC_DGN_COMMUNICATION_STATUS_1, ndata, data);
}

// ----------------------------------------------------------------------
// RVC 6.6.3
// Send Comm Status 2
void rvcan_SendCommStatus2()
{
    CAN_DATA data[8];
    CAN_LEN  ndata = sizeof(data);

    data[0] = (uint8_t)(g_can.rvcTxFrameCount    );
    data[1] = (uint8_t)(g_can.rvcTxFrameCount>> 8);
    data[2] = (uint8_t)(g_can.rvcTxFrameCount>>16);
    data[3] = (uint8_t)(g_can.rvcTxFrameCount>>24);
    data[4] = (uint8_t)(g_can.rvcRxFrameCount    );
    data[5] = (uint8_t)(g_can.rvcRxFrameCount>> 8);
    data[6] = (uint8_t)(g_can.rvcRxFrameCount>>16);
    data[7] = (uint8_t)(g_can.rvcRxFrameCount>>24);

    LOGX(SS_RVC, SV_INFO, "SendCommStatus2: ", data, ndata);

    J1939_SendMessage(RVC_DGN_COMMUNICATION_STATUS_2, ndata, data);
}

// ----------------------------------------------------------------------
// RVC 6.6.4
// Send Comm Status 3
void rvcan_SendCommStatus3()
{
    CAN_DATA data[6];
    CAN_LEN  ndata = sizeof(data);

    data[0] = LOBYTE(g_can.rvcBusOffErr);
    data[1] = HIBYTE(g_can.rvcBusOffErr);
    data[2] = LOBYTE(g_can.rvcRxFrameCount);
    data[3] = HIBYTE(g_can.rvcRxFrameCount);
    data[4] = LOBYTE(g_can.rvcTxFrameCount);
    data[5] = HIBYTE(g_can.rvcTxFrameCount);

    LOGX(SS_RVC, SV_INFO, "SendCommStatus3: ", data, ndata);

    J1939_SendMessage(RVC_DGN_COMMUNICATION_STATUS_3, ndata, data);
}

// ----------------------------------------------------------------------
//            D C   S O U R C E   S T A T U S   N E S S A G E S
// ----------------------------------------------------------------------

//  ref:    RVC 6.5.2  pg 40
//  Name:   DC_SOURCE_STATUS_1
//  DGN:    0x1FFFD
void rvcan_SendDcSourceStatus1()
{
    CAN_DATA data[sizeof(RVCS_DC_SOURCE_STATUS_1)];
    CAN_LEN  ndata = sizeof(data);

    inv_GetDcSourceStatus1((RVCS_DC_SOURCE_STATUS_1*)&data[0]);

    LOGX(SS_RVC, SV_INFO, "SendDcSrcStat1: ", data, ndata);

    J1939_SendMessage(RVC_DGN_DC_SOURCE_STATUS_1, ndata, data);
}

// ----------------------------------------------------------------------
// RVC 6.5.3  pg 41
void rvcan_SendDcSourceStatus2()
{
    CAN_DATA data[sizeof(RVCS_DC_SOURCE_STATUS_2)+1];
    CAN_LEN  ndata = sizeof(data);

    data[0] = g_can.MyInstance;
    inv_GetDcSourceStatus2((RVCS_DC_SOURCE_STATUS_2*)&data[1]);

    LOGX(SS_RVC, SV_INFO, "SendDcSrcStat2: ", data, ndata);

    J1939_SendMessage(RVC_DGN_DC_SOURCE_STATUS_2, ndata, data);
}

// ----------------------------------------------------------------------
// RVC 6.5.3  pg 41
void rvcan_SendDcSourceStatus3()
{
    CAN_DATA data[sizeof(RVCS_DC_SOURCE_STATUS_3)+1];
    CAN_LEN  ndata = sizeof(data);

    data[0] = g_can.MyInstance;
    inv_GetDcSourceStatus3((RVCS_DC_SOURCE_STATUS_3*)&data[1]);

    LOGX(SS_RVC, SV_INFO, "SendDcSrcStat3: ", data, ndata);

    J1939_SendMessage(RVC_DGN_DC_SOURCE_STATUS_3, ndata, data);
}

// ----------------------------------------------------------------------
//             I N V E R T E R   S T A T U S    M E S S A G E S
// ----------------------------------------------------------------------

//  Name:   INVERTER_STATUS
//  DGN:    0x1FFD4
//  ref:    RVC 6.20.8  pg 100
void rvcan_SendInverterStatus()
{
    CAN_DATA data[sizeof(RVCS_INV_STATUS)+1];
    CAN_LEN  ndata = sizeof(data);

    data[0] = g_can.MyInstance;
    data[1] = inv_GetRvcStatus();
    data[2] = (inv_GetBatteryTempSensorPresent()?1:0) + (IsInvLoadSenseEn()?4:0);

    LOGX(SS_RVC, SV_INFO, "SendInvStat: ", data, ndata);

    J1939_SendMessage(RVC_DGN_INVERTER_STATUS, ndata, data);
}

// ----------------------------------------------------------------------
// RVC 6.20.10  pg 101
void rvcan_SendInverterConfigStatus1()
{
    CAN_DATA data[sizeof(RVCS_INV_CFG_STATUS1)+1];
    CAN_LEN  ndata = sizeof(data);

    data[0] = g_can.MyInstance;
    inv_GetConfigStatus1((RVCS_INV_CFG_STATUS1*)&data[1]);

    LOGX(SS_RVC, SV_INFO, "SendInvCfgStat1: ", data, ndata);

    J1939_SendMessage(RVC_DGN_INVERTER_CONFIGURATION_STATUS_1, ndata, data);
}

// ----------------------------------------------------------------------
// RVC 6.20.11  pg 102
void rvcan_SendInverterConfigStatus2()
{
    CAN_DATA data[sizeof(RVCS_INV_CFG_STATUS2)+1];
    CAN_LEN  ndata = sizeof(data);

    data[0] = g_can.MyInstance;
    inv_GetConfigStatus2((RVCS_INV_CFG_STATUS2*)&data[1]);

    LOGX(SS_RVC, SV_INFO, "SendInvCfgStat2: ", data, ndata);

    J1939_SendMessage(RVC_DGN_INVERTER_CONFIGURATION_STATUS_2, ndata, data);
}

// ----------------------------------------------------------------------
// RVC 6.20.18  pg 106
void rvcan_SendInverterDcStatus()
{
    CAN_DATA data[sizeof(RVCS_INV_DC_STATUS)+1];
    CAN_LEN  ndata = sizeof(data);

    data[0] = g_can.MyInstance;
    inv_GetDcStatus((RVCS_INV_DC_STATUS*)&data[1]);

    LOGX(SS_RVC, SV_INFO, "SendInvDcStat: ", data, ndata);

    J1939_SendMessage(RVC_DGN_INVERTER_DC_STATUS, ndata, data);
}

// ----------------------------------------------------------------------
// RVC 6.1.6  pg 30
void rvcan_SendInverterAcFaultCfgStatus1()
{
    CAN_DATA data[sizeof(RVCS_AC_FAULT_STATUS_1)+1];
    CAN_LEN  ndata = sizeof(data);

    data[0] = g_can.MyInstance;
    inv_GetAcFaultStatus1((RVCS_AC_FAULT_STATUS_1*)&data[1]);

    LOGX(SS_RVC, SV_INFO, "SendInvAcFaultStat1: ", data, ndata);

    J1939_SendMessage(RVC_DGN_INVERTER_ACFAULT_CONFIGURATION_STATUS_1, ndata, data);
}

// ----------------------------------------------------------------------
// RVC 6.1.7 pg 31
void rvcan_SendInverterAcFaultCfgStatus2()
{
    CAN_DATA data[sizeof(RVCS_AC_FAULT_STATUS_2)+1];
    CAN_LEN  ndata = sizeof(data);

    data[0] = g_can.MyInstance;
    inv_GetAcFaultStatus2((RVCS_AC_FAULT_STATUS_2*)&data[1]);

    LOGX(SS_RVC, SV_INFO, "SendInvAcFaultStat2: ", data, ndata);

    J1939_SendMessage(RVC_DGN_INVERTER_ACFAULT_CONFIGURATION_STATUS_2, ndata, data);
}

// ----------------------------------------------------------------------
// RVC 6.20.2  pg 98
void rvcan_SendInverterAcStatus1()
{
    CAN_DATA data[sizeof(RVCS_AC_PNT_PG1)+1];
    CAN_LEN  ndata = sizeof(data);

    data[0] = g_can.MyInstance | 0x40;  // output
    inv_GetAcStatus1((RVCS_AC_PNT_PG1*)&data[1]);

    LOGX(SS_RVC, SV_INFO, "SendInvAcStat1: ", data, ndata);

    J1939_SendMessage(RVC_DGN_INVERTER_AC_STATUS_1, ndata, data);
}

// ----------------------------------------------------------------------
// RVC 6.20.4  pg 98
void rvcan_SendInverterAcStatus2()
{
    CAN_DATA data[sizeof(RVCS_AC_PNT_PG2)+1];
    CAN_LEN  ndata = sizeof(data);

    data[0] = g_can.MyInstance;
    inv_GetAcStatus2((RVCS_AC_PNT_PG2*)&data[1]);

    LOGX(SS_RVC, SV_INFO, "SendInvAcStat2: ", data, ndata);

    J1939_SendMessage(RVC_DGN_INVERTER_AC_STATUS_2, ndata, data);
}

// ----------------------------------------------------------------------
// RVC 6.20.5  pg 99
void rvcan_SendInverterAcStatus3()
{
    CAN_DATA data[sizeof(RVCS_AC_PNT_PG3)+1];
    CAN_LEN  ndata = sizeof(data);

    data[0] = g_can.MyInstance;
    inv_GetAcStatus3((RVCS_AC_PNT_PG3*)&data[1]);

    LOGX(SS_RVC, SV_INFO, "SendInvAcStat3: ", data, ndata);

    J1939_SendMessage(RVC_DGN_INVERTER_AC_STATUS_3, ndata, data);
}

// ----------------------------------------------------------------------
// RVC 6.20.6  pg 99
void rvcan_SendInverterAcStatus4()
{
    CAN_DATA data[sizeof(RVCS_AC_PNT_PG4)+1];
    CAN_LEN  ndata = sizeof(data);

    data[0] = g_can.MyInstance;
    inv_GetAcStatus4((RVCS_AC_PNT_PG4*)&data[1]);

    LOGX(SS_RVC, SV_INFO, "SendInvAcStat4: ", data, ndata);

    J1939_SendMessage(RVC_DGN_INVERTER_AC_STATUS_4, ndata, data);
}

// ----------------------------------------------------------------------
//             C H A R G E R   S T A T U S    M E S S A G E S
// ----------------------------------------------------------------------

#ifdef OPTION_HAS_CHARGER

//  Name:   CHARGER_STATUS
//  DGN:    0x1FFC7
//  ref:    RVC 6.21.8  pg 109
void rvcan_SendChargerStatus(void)
{
    CAN_DATA data[sizeof(RVCS_CHARGER_STATUS)+1];
    CAN_LEN  ndata = sizeof(data);
    
    data[0] = g_can.MyInstance;
    chgr_GetStatusInfo((RVCS_CHARGER_STATUS*)&data[1]);

    LOGX(SS_RVC, SV_INFO, "SendChgrStatus: ", data, ndata);

    J1939_SendMessage(RVC_DGN_CHARGER_STATUS, ndata, data);
}

// ----------------------------------------------------------------------
// RVC 6.21.9  pg 110
void rvcan_SendChargerConfigStatus(void)
{
    CAN_DATA data[sizeof(RVCS_CHARGER_CFG_STATUS)+1];
    CAN_LEN  ndata = sizeof(data);

    data[0] = g_can.MyInstance;
    chgr_GetConfigStatus((RVCS_CHARGER_CFG_STATUS*)&data[1]);

    LOGX(SS_RVC, SV_INFO, "SendChgrCfgStatus: ", data, ndata);

    J1939_SendMessage(RVC_DGN_CHARGER_CONFIGURATION_STATUS, ndata, data);
}

// ----------------------------------------------------------------------
// RVC 6.21.12  pg 113
void rvcan_SendChargerConfigStatus2(void)
{
    CAN_DATA data[sizeof(RVCS_CHARGER_CFG_STATUS2)+1];
    CAN_LEN  ndata = sizeof(data);

    data[0] = g_can.MyInstance;
    chgr_GetConfigStatus2((RVCS_CHARGER_CFG_STATUS2*)&data[1]);

    LOGX(SS_RVC, SV_INFO, "SendChgrCfgStatus2: ", data, ndata);

    J1939_SendMessage(RVC_DGN_CHARGER_CONFIGURATION_STATUS_2, ndata, data);
}

// ----------------------------------------------------------------------
// RVC 6.21.14  pg 114
void rvcan_SendChargerEqualStatus(void)
{
    CAN_DATA data[sizeof(RVCS_CHARGER_EQUAL_STATUS)+1];
    CAN_LEN  ndata = sizeof(data);

    data[0] = g_can.MyInstance;
    chgr_GetEqualStatus((RVCS_CHARGER_EQUAL_STATUS*)&data[1]);

    LOGX(SS_RVC, SV_INFO, "SendChgrEqualStatus: ", data, ndata);

    J1939_SendMessage(RVC_DGN_CHARGER_EQUALIZATION_STATUS, ndata, data);
}

// ----------------------------------------------------------------------
// RVC 6.1.6  pg 30
void rvcan_SendChargerAcFaultCfgStatus1(void)
{
    CAN_DATA data[sizeof(RVCS_AC_FAULT_STATUS_1)+1];
    CAN_LEN  ndata = sizeof(data);

    data[0] = g_can.MyInstance;
    chgr_GetAcFaultStatus1((RVCS_AC_FAULT_STATUS_1*)&data[1]);

    LOGX(SS_RVC, SV_INFO, "SendChgrAcFaultStatus1: ", data, ndata);

    J1939_SendMessage(RVC_DGN_CHARGER_ACFAULT_CONFIGURATION_STATUS_1, ndata, data);
}

// ----------------------------------------------------------------------
// RVC 6.1.7 pg 31
void rvcan_SendChargerAcFaultCfgStatus2(void)
{
    CAN_DATA data[sizeof(RVCS_AC_FAULT_STATUS_2)+1];
    CAN_LEN  ndata = sizeof(data);

    data[0] = g_can.MyInstance;
    chgr_GetAcFaultStatus2((RVCS_AC_FAULT_STATUS_2*)&data[1]);

    LOGX(SS_RVC, SV_INFO, "SendChgrAcFaultStatus2: ", data, ndata);
    
    J1939_SendMessage(RVC_DGN_CHARGER_ACFAULT_CONFIGURATION_STATUS_2, ndata, data);
}

// ----------------------------------------------------------------------
// RVC 6.21.3  pg 107
void rvcan_SendChargerAcStatus1(void)
{
    CAN_DATA data[sizeof(RVCS_AC_PNT_PG1)+1];
    CAN_LEN  ndata = sizeof(data);

    data[0] = g_can.MyInstance;
    chgr_GetAcStatus1((RVCS_AC_PNT_PG1*)&data[1]);

    LOGX(SS_RVC, SV_INFO, "SendChgrAcStat1: ", data, ndata);

    J1939_SendMessage(RVC_DGN_CHARGER_AC_STATUS_1, ndata, data);
}

// ----------------------------------------------------------------------
// RVC 6.21.4  pg 108
void rvcan_SendChargerAcStatus2(void)
{
    CAN_DATA data[sizeof(RVCS_AC_PNT_PG2)+1];
    CAN_LEN  ndata = sizeof(data);

    data[0] = g_can.MyInstance;
    chgr_GetAcStatus2((RVCS_AC_PNT_PG2*)&data[1]);

    LOGX(SS_RVC, SV_INFO, "SendChgrAcStat2: ", data, ndata);

    J1939_SendMessage(RVC_DGN_CHARGER_AC_STATUS_2, ndata, data);
}

// ----------------------------------------------------------------------
// RVC 6.21.5  pg 108
void rvcan_SendChargerAcStatus3(void)
{
    CAN_DATA data[sizeof(RVCS_AC_PNT_PG3)+1];
    CAN_LEN  ndata = sizeof(data);

    data[0] = g_can.MyInstance;
    chgr_GetAcStatus3((RVCS_AC_PNT_PG3*)&data[1]);

    LOGX(SS_RVC, SV_INFO, "SendChgrAcStat3: ", data, ndata);

    J1939_SendMessage(RVC_DGN_CHARGER_AC_STATUS_3, ndata, data);
}

// ----------------------------------------------------------------------
// RVC 6.21.6  pg 108
void rvcan_SendChargerAcStatus4(void)
{
    CAN_DATA data[sizeof(RVCS_AC_PNT_PG4)+1];
    CAN_LEN  ndata = sizeof(data);

    data[0] = g_can.MyInstance;
    chgr_GetAcStatus4((RVCS_AC_PNT_PG4*)&data[1]);

    LOGX(SS_RVC, SV_INFO, "SendChgAcStat4: ", data, ndata);

    J1939_SendMessage(RVC_DGN_CHARGER_AC_STATUS_4, ndata, data);
}

#endif // OPTION_HAS_CHARGER

// ----------------------------------------------------------------------
//           I N V E R T E R   C O M M A N D    H A N D L E R S
// ----------------------------------------------------------------------

//  1FFD3h	INVERTER_COMMAND
//  ref:    RVC 6.20.9  pg 100
void rvcan_InverterCmd(uint8_t enableFlags, uint8_t startupFlags)
{
    int enInv,    enLdSns,    enPassThru;
    int startInv, startLdSns, startPassThru;

    LOG(SS_RVC, SV_INFO,"InvCmd: enable=0x%02X enStartup=0x%02X", enableFlags, startupFlags);

    // decode enable flags
    enInv      = (enableFlags     ) & 0x3;
    enLdSns    = (enableFlags >> 2) & 0x3;
    enPassThru = (enableFlags >> 4) & 0x3;

    // decode startup flags 2or3=dont change
    startInv      = (startupFlags     ) & 0x3;
    startLdSns    = (startupFlags >> 2) & 0x3;
    startPassThru = (startupFlags >> 4) & 0x3;

    //  These settings take affect immediately
    if (!(enInv      & 2)) dev_SetInverterEnabled (enInv      & 1);
    if (!(enLdSns    & 2)) inv_SetLoadSenseEnabled(enLdSns	& 1);
    if (!(enPassThru & 2)) dev_SetPassThruEnabled (enPassThru & 1);
         
    //  These settings take affect on startup
    if (!(startInv      & 2)) dev_SetInverterEnabledOnStartup (startInv      & 1);
    if (!(startLdSns    & 2)) inv_SetLoadSenseEnabledOnStartup(startLdSns    & 1);
    if (!(startPassThru & 2)) dev_SetPassThruEnabledOnStartup(startPassThru & 1);

    // send back status
    rvcan_SendInverterStatus();
}

// ----------------------------------------------------------------------
//	1FFD0h	INVERTER_CONFIGURATION_COMMAND_1	
// 	ref:	RVC 6.20.13a (6.20.12a in prior revision)
//	
void rvcan_InverterCfgCmd1(uint16_t loadSenseThresh,    // power threshold in watts
                           uint16_t loadSenseInterval,  // seconds
                           uint16_t dcSrcShutdownMin)   // volts
{
    LOG(SS_RVC, SV_INFO,"InvCfgCmd1(ldSnsTh=%u ldSnsInt=%u dcMin=%u)",
                          loadSenseThresh, loadSenseInterval, dcSrcShutdownMin);

    // TODO use settings
    rvcan_SendInverterConfigStatus1();
}

// ----------------------------------------------------------------------
// RVC 6.20.13  pg 103
void rvcan_InverterCfgCmd2(uint16_t dcSrcShutdownMax,   // volts
                           uint16_t dcSrcWarnMin,       // volts
                           uint16_t dcSrcWarnMax)       // volts
{
    LOG(SS_RVC, SV_INFO,"InvCfgCmd2(dcMax=%u dcWarnMin=%u dcWarnMax=%u)",
                        (int)dcSrcShutdownMax, (int)dcSrcWarnMin, (int)dcSrcWarnMax);

    // TODO use settings
    rvcan_SendInverterConfigStatus2();
}

// ----------------------------------------------------------------------
// RVC 6.1.7  pg 31
void rvcan_InverterAcFaultCtrlCfgCmd1(RVCS_AC_FAULT_STATUS_1* cfg)
{
    LOG(SS_RVC, SV_INFO,"InvAcFaultCtrlCfgCmd1(xLoV=%u loV=%u hiV=%u xHiV=%u eqSec=%u, bypass=%u)",
                (int)cfg->extremeLowVoltage,  (int)cfg->lowVoltage,          (int)cfg->highVoltage,
                (int)cfg->extremeHighVoltage, (int)cfg->equalizationSeconds, (int)cfg->bypassMode);

    // TODO use settings
    rvcan_SendInverterAcFaultCfgStatus1();
}

// ----------------------------------------------------------------------
// RVC 6.1.7 pg 31
void rvcan_InverterAcFaultCtrlCfgCmd2(RVCS_AC_FAULT_STATUS_2* cfg)
{
    LOG(SS_RVC, SV_INFO,"InvAcFaultCtrlCfgCmd1(hiFreq=%u loFreq=%u",
                (int)cfg->hiFreqLimit,  (int)cfg->loFreqLimit);

    // TODO use settings
    rvcan_SendInverterAcFaultCfgStatus2();
}

// ----------------------------------------------------------------------
//           C H A R G E R   C O M M A N D    H A N D L E R S
// ----------------------------------------------------------------------

#ifdef OPTION_HAS_CHARGER

//  Name:   CHARGER_COMMAND
//  DGN:    0x1FFCB
//  ref:    RVC 6.21.10  pg 111
//  status: 0=Disable (charger), 1=Enable charger, 2=Start equalization 
//			(bitmapped) 3=Enable Charger + Start equalization >=4= no change
//  startupEnable: 0=disable, 1=enable, 2or3=dont change

void rvcan_ChargerCmd(uint8_t status,     // 0=off, 1=on, >1=dont change
                      uint8_t startupEnable)   // 0=charger disabled, 1=charger enabled, >1=dont change
{
//    LOG(SS_RVC, SV_INFO,"ChgrCmd: status=%u startupEnable=%u", (int)status, (int)startupEnable);

	
    // decode 'status' flags 2 or 3 = dont change
	if((status & 0x02) != 0x02)		Device.status.chgr_enable_can = (status & 0x01)? 1:0;
	
	if((status & 0x08) != 0x08)
	{
		g_nvm.settings.chgr.eq_request = Chgr.config.eq_request = (status & 0x04)? 1:0;;
        nvm_SetDirty();
	}

    // enable on start-up
    if (startupEnable < 2) // 2 or 3 = don't change
    {   
        g_nvm.settings.dev.chgr_enabled = Device.config.chgr_enabled = startupEnable;
        nvm_SetDirty();
    }
	
    LOG(SS_SYS, SV_INFO,"ChgrCmd: status=0x%02X, en:%d, eq:%d ", 
		(int)status, 
		(int)IsChgrEnabled(),
		(int)IsChgrCfgEqRequest() );
//    LOG(SS_SYS, SV_INFO,"ChgrCmd: status=%u startupEnable=%u", (int)status, (int)startupEnable);

    rvcan_SendChargerStatus();
}

// ----------------------------------------------------------------------
//	Name: 	CHARGER_CONFIGURATION_COMMAND
//	DGN:	0x1FFC4
// 	Ref:	RVC 6.21.11
void rvcan_ChargerCfgCmd(uint8_t  chargingAlgorithm,    // 
                         uint8_t  chargerMode,          // 
                         uint8_t  batterySensePresent,  // 0=no, 1=yes
                         uint8_t  installationLine,     // 0=line1, 1=line2
                         uint16_t batteryBankSize,      // amp-hours; resolution 1 amp-hour
                         uint16_t batteryType,          // 
                         uint8_t  maxChargingCurrent)   // amps
{
    LOG(SS_RVC, SV_INFO,"ChgrCfgCmd(alg=%u mode=%u battSns=%u install=%u battSize=%u battType=%u maxCharge=%u)",
                        (int)chargingAlgorithm, (int)chargerMode,     (int)batterySensePresent,
                        (int)installationLine,  (int)batteryBankSize, (int)batteryType, (int)maxChargingCurrent); 

    chgr_SetCfgCmd(chargingAlgorithm,   
                   chargerMode,         
                   batterySensePresent, 
                   installationLine,    
                   batteryBankSize,     
                   batteryType,         
                   maxChargingCurrent);  

    rvcan_SendChargerConfigStatus();
}

// ----------------------------------------------------------------------
// RVC 6.21.13  pg 113
void rvcan_ChargerCfgCmd2(uint8_t  chargePercent,       // max charge current; percent
                          uint8_t  chargeRatePercent,   // charge rate limit; percent of bank size
                          uint8_t  breakerSize)         // amps
{
    LOG(SS_RVC, SV_INFO,"ChgrCfgCmd2(alg=%u mode=%u battSns=%u install=%u battSize=%u battType=%u maxCharge=%u)",
                        (int)chargePercent, (int)chargeRatePercent, (int)breakerSize);

    chgr_SetCfgCmd2(chargePercent,chargeRatePercent,breakerSize);

    rvcan_SendChargerConfigStatus2();
}

// ----------------------------------------------------------------------
// RVC 6.21.16  pg 115
void rvcan_ChargerEqualizationCmd(uint16_t  equalizationVolts,      // equalization voltage
                                  uint16_t  equalizationMinutes)    // equalization time in minutes
{
    LOG(SS_RVC, SV_INFO,"ChgrEqualCmd(eqVolts=%u eqMinutes=%u)",
                        (int)equalizationVolts, (int)equalizationMinutes);

    // TODO use settings
    rvcan_SendChargerEqualStatus();
}

// ----------------------------------------------------------------------
// RVC 6.1.7  pg 31
void rvcan_ChargerAcFaultCtrlCfgCmd1(RVCS_AC_FAULT_STATUS_1* cfg)
{
    LOG(SS_RVC, SV_INFO,"ChgrAcFaultCtrlCfgCmd1(xLoV=%u loV=%u hiV=%u xHiV=%u eqSec=%u, bypass=%u)",
                (int)cfg->extremeLowVoltage,  (int)cfg->lowVoltage,          (int)cfg->highVoltage,
                (int)cfg->extremeHighVoltage, (int)cfg->equalizationSeconds, (int)cfg->bypassMode);

    // TODO use settings
    rvcan_SendChargerAcFaultCfgStatus1();
}

// ----------------------------------------------------------------------
// RVC 6.1.7 pg 31
void rvcan_ChargerAcFaultCtrlCfgCmd2(RVCS_AC_FAULT_STATUS_2* cfg)
{
    LOG(SS_RVC, SV_INFO,": ChgrAcFaultCtrlCfgCmd1(hiFreq=%u loFreq=%u",
                (int)cfg->hiFreqLimit,  (int)cfg->loFreqLimit);

    // TODO use settings
    rvcan_SendChargerAcFaultCfgStatus2();
}

#endif // OPTION_HAS_CHARGER

// ----------------------------------------------------------------------
//           P R O P R I E T A R Y    M E S S A G E S
// ----------------------------------------------------------------------

// Proprietary Magnum message
void rvcan_SendPropInvStatus(uint8_t loDGN)
{
    CAN_DATA data[3];
    CAN_LEN  ndata = 3;
    CAN_DGN  dgn;

    dgn = 0xEF00 | loDGN;

    // Cory Rietz - Tech Support at Silverleaf
    // The HMS360 you are using doesn't support the 
    // new Magnum Bridge proprietary messages.
    // We have our own Magnum interface that produces a different set
    // of proprietary messages that the HMS360 is asking for. 

    // proprietary inverter temperature message
    data[0] = 0xE2;
    data[1] = RVC_NOT_SUPPORTED_8BIT;
  #if IS_PCB_LPC 
    data[2] = RVC_NOT_SUPPORTED_8BIT;
  #else
    data[2] = HeatSinkTempC();
  #endif
    LOGX(SS_RVC, SV_INFO, "SendPropInvTemp: ", data, ndata);
    J1939_SendMessage(dgn, ndata, data);

    // proprietary inverter version message
    data[0] = 0xE4;
    data[1] = 40; // magnum model
    data[2] = 12; // version in 0.1 increments
    LOGX(SS_RVC, SV_INFO, "SendPropInvVersion: ", data, ndata);
    J1939_SendMessage(dgn, ndata, data);

}


// ----------------------------------------------------------------------
//             R V C    M E S S A G E    D I S P A T C H E R
// ----------------------------------------------------------------------
// check if CAN message is a RV-C message and process it
// returns: 0=not for us, 1=dispatched
int16_t rvcan_Dispatcher(CAN_MSG* msg)
{
    CAN_DATA outData[SENFLD_MAX_BYTES];
    CAN_LEN  ndata;
    int16_t  rc = 0;    // assume dispatched
    uint32_t dgn;

    // must be extended message
    if (msg->frameType != CAN_FRAME_EXT) return(0);

    dgn = can_DGN(msg);
    switch( dgn )
    {
        case SENSATA_CUSTOM_SET_FIELD_DGN:
            sen_SetField(msg->data);
            break;

        case SENSATA_CUSTOM_GET_FIELD_DGN:
            // get data for requested field
            if (!IsRvcCmdForMe(msg->data[0])) break;
            if (sen_GetField(msg->data, outData, &ndata)) break;
            // send CAN response to host
            if (outData[3] == SENFLD_TYPE_STRING)  // string is the odd ball
                J1939_SendMultiPacketMessage(SENSATA_CUSTOM_GET_FIELD_RSP_DGN, ndata, outData);
            else
                J1939_SendMessage(SENSATA_CUSTOM_GET_FIELD_RSP_DGN, ndata, outData);
            break;

        case RVC_DGN_GENERAL_RESET: 
            if (!IsRvcCmdForMe(msg->data[1])) break;    // Sensata extension
            rvcan_GeneralReset((msg->data[0] & (3<<0)) ? 1 : 0,   // 0=no, 1=reboot cpu
                               (msg->data[0] & (3<<2)) ? 1 : 0,   // 0=no, 1=clear faults
                               (msg->data[0] & (3<<4)) ? 1 : 0,   // 0=no, 1=restore to default values
                               (msg->data[0] & (3<<6)) ? 1 : 0);  // 0=no, 1=reset communication statistics

            break;
        case RVC_DGN_INVERTER_COMMAND:
            if (!IsRvcCmdForMe(msg->data[0])) break;
            rvcan_InverterCmd( msg->data[1], msg->data[7] );
            break;

        case RVC_DGN_INVERTER_CONFIGURATION_COMMAND_1:
            if (!IsRvcCmdForMe(msg->data[0])) break;
            rvcan_InverterCfgCmd1( MKWORD(msg->data[1],msg->data[2]),
                                   MKWORD(msg->data[3],msg->data[4]),
                                   MKWORD(msg->data[5],msg->data[6]) );
            break;

        case RVC_DGN_INVERTER_CONFIGURATION_COMMAND_2:
            if (!IsRvcCmdForMe(msg->data[0])) break;
            rvcan_InverterCfgCmd2( MKWORD(msg->data[1],msg->data[2]),
                                   MKWORD(msg->data[3],msg->data[4]),
                                   MKWORD(msg->data[5],msg->data[6]) );
            break;

        case RVC_DGN_INVERTER_ACFAULT_CONFIGURATION_COMMAND_1:        
            if (!IsRvcCmdForMe(msg->data[0])) break;
            rvcan_InverterAcFaultCtrlCfgCmd1((RVCS_AC_FAULT_STATUS_1*)&msg->data[1]);
            break;

        case RVC_DGN_INVERTER_ACFAULT_CONFIGURATION_COMMAND_2:        
            if (!IsRvcCmdForMe(msg->data[0])) break;
            rvcan_InverterAcFaultCtrlCfgCmd2((RVCS_AC_FAULT_STATUS_2*)&msg->data[1]);
            break;

    #ifdef OPTION_HAS_CHARGER
        case RVC_DGN_CHARGER_COMMAND:
            if (!IsRvcCmdForMe(msg->data[0])) break;
            rvcan_ChargerCmd(msg->data[1],      // 0=off, 1=on, 2=start equalization
                             msg->data[2] & 3); // 0=off, 1=on, ignore all others
            break;

        case RVC_DGN_CHARGER_CONFIGURATION_COMMAND:
            if (!IsRvcCmdForMe(msg->data[0])) break;
            rvcan_ChargerCfgCmd(msg->data[1],                       // charging algorithm
                                msg->data[2],                       // charger mode
                                msg->data[3] & 3,                   // battery temp sensor
                               (msg->data[3]>>2) & 3,               // install line
                                MKWORD(msg->data[4],msg->data[5]),  // battery size
                                msg->data[6],                       // battery type
                                msg->data[7]);                      // max charge amps
            break;

        case RVC_DGN_CHARGER_CONFIGURATION_COMMAND_2:                 
            if (!IsRvcCmdForMe(msg->data[0])) break;
            rvcan_ChargerCfgCmd2(msg->data[1],              // charge percent
                                 msg->data[2],              // charge rate percent
                                 msg->data[3]);             // breakerSize
            break;

        case RVC_DGN_CHARGER_EQUALIZATION_CONFIGURATION_COMMAND:      
            if (!IsRvcCmdForMe(msg->data[0])) break;
            rvcan_ChargerEqualizationCmd(MKWORD(msg->data[1],msg->data[2]),     // voltage
                                         MKWORD(msg->data[3],msg->data[4]));    // minutes
            break;

        case RVC_DGN_CHARGER_ACFAULT_CONFIGURATION_COMMAND_1:         
            if (!IsRvcCmdForMe(msg->data[0])) break;
            rvcan_ChargerAcFaultCtrlCfgCmd1((RVCS_AC_FAULT_STATUS_1*)&msg->data[1]);
            break;

        case RVC_DGN_CHARGER_ACFAULT_CONFIGURATION_COMMAND_2:
            if (!IsRvcCmdForMe(msg->data[0])) break;
            rvcan_ChargerAcFaultCtrlCfgCmd2((RVCS_AC_FAULT_STATUS_2*)&msg->data[1]);
            break;
     #endif // OPTION_HAS_CHARGER

        case RVC_DGN_INSTANCE_ASSIGNMENT:
            rvcan_InstanceAssignment(msg->data[0],msg->data[1],msg->data[2],
                              MKWORD(msg->data[3],msg->data[4]),
                              MKWORD(msg->data[5],msg->data[6]) );
            break;

        case RVC_DGN_PROP_MAGNUM_INVERTER_STATUS:
            rvcan_SendPropInvStatus(msg->jid.SourceAddress);
            break;
        /*
        // not received, but sent

        // house keeping
        case RVC_DGN_INSTANCE_STATUS:                                 

        // communications
        case RVC_DGN_COMMUNICATION_STATUS_1:
        case RVC_DGN_COMMUNICATION_STATUS_2:
        case RVC_DGN_COMMUNICATION_STATUS_3:

        // inverter status
        case RVC_DGN_INVERTER_AC_STATUS_1:
        case RVC_DGN_INVERTER_AC_STATUS_2:
        case RVC_DGN_INVERTER_AC_STATUS_3:
        case RVC_DGN_INVERTER_STATUS:
        case RVC_DGN_INVERTER_CONFIGURATION_STATUS_1:
        case RVC_DGN_INVERTER_CONFIGURATION_STATUS_2:
        case RVC_DGN_INVERTER_STATISTICS_STATUS:                      
        case RVC_DGN_INVERTER_APS_STATUS:                             
        case RVC_DGN_INVERTER_DCBUS_STATUS:                           
        case RVC_DGN_INVERTER_OPS_STATUS:                             
        case RVC_DGN_INVERTER_AC_STATUS_4:                            
        case RVC_DGN_INVERTER_ACFAULT_CONFIGURATION_STATUS_1:         
        case RVC_DGN_INVERTER_ACFAULT_CONFIGURATION_STATUS_2:         
        case RVC_DGN_INVERTER_DC_STATUS:                              

        // charger status
        case RVC_DGN_CHARGER_AC_STATUS_1:                             
        case RVC_DGN_CHARGER_AC_STATUS_2:                             
        case RVC_DGN_CHARGER_AC_STATUS_3:                             
        case RVC_DGN_CHARGER_STATUS:                                  
        case RVC_DGN_CHARGER_CONFIGURATION_STATUS:                    
        case RVC_DGN_CHARGER_APS_STATUS:                              
        case RVC_DGN_CHARGER_DCBUS_STATUS:                            
        case RVC_DGN_CHARGER_OPS_STATUS:                              
        case RVC_DGN_CHARGER_EQUALIZATION_STATUS:                     
        case RVC_DGN_CHARGER_EQUALIZATION_CONFIGURATION_STATUS:       
        case RVC_DGN_CHARGER_CONFIGURATION_STATUS_2:                  
        case RVC_DGN_CHARGER_AC_STATUS_4:                             
        case RVC_DGN_CHARGER_ACFAULT_CONFIGURATION_STATUS_1:          
        case RVC_DGN_CHARGER_ACFAULT_CONFIGURATION_STATUS_2:          

        // dc source
        case RVC_DGN_DC_SOURCE_STATUS_1:
        case RVC_DGN_DC_SOURCE_STATUS_2:
        case RVC_DGN_DC_SOURCE_STATUS_3:

        // loads
        case RVC_DGN_AC_LOAD_STATUS_2:                                
        case RVC_DGN_DC_LOAD_STATUS_2:                                

        // date-time
        case RVC_DGN_SET_DATE_TIME_COMMAND:     // LPC has no real-time-clock                                                    
        case RVC_DGN_DATE_TIME_STATUS:          // LPC has no real-time-clock                            
        case RVC_DGN_PROPRIETARY_INSTANCE:
        */

        default:
            rc = 0; // not dispatched
            break;
    } // switch
    return(rc);
}

// <><><><><><><><><><><><><> rv_can.c <><><><><><><><><><><><><><><><><><><><><><>
