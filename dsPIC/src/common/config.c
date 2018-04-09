// <><><><><><><><><><><><><> config.c <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//  Copyright(C) 2015 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
//  ROM configuration items
//      
//-----------------------------------------------------------------------------

// -------
// headers
// -------
#include "options.h"    // must be first include
#include "config.h"

//------------------------------------------------------------------------------
//                        R O M    D E F A U L T S
//------------------------------------------------------------------------------

// Manufacturing info
// CAUTION: these values are patched in by the HiPot tester before firmware burn

const ROM_MFG_CONFIG_t __attribute__ ((space(psv), address (0x0200))) g_RomMfgInfo = 
{ 
    // Manufacturing Info
    // The final tester patches these in before the firmware is flashed.
    // These values are just place holders.
    { MODEL_NO_STRING },     // model_no
    { "A" },                 // rev
    { "W123456789" },        // serial_no
    { "2017-11-09 10:12" },  // mfg_date

    { // future use; set to erased; padded to 256 bytes
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
    } // 256 bytes total
};

// firmware version id string
const char* FwIdStr = FW_ID_STRING;

//------------------------------------------------------------------------------
void cfg_ShowMfgInfo()
{
    LOG(SS_CFG, SV_INFO, "Mfg Model: %-24.24s", g_RomMfgInfo.model_no );
    LOG(SS_CFG, SV_INFO, "Mfg Rev:   %-4.4s",   g_RomMfgInfo.rev      );
    LOG(SS_CFG, SV_INFO, "Mfg SN:    %-20.20s", g_RomMfgInfo.serial_no);
    LOG(SS_CFG, SV_INFO, "Mfg Date:  %-20.20s", g_RomMfgInfo.mfg_date );
}

// <><><><><><><><><><><><><> config.c <><><><><><><><><><><><><><><><><><><><><><>
