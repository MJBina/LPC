// <><><><><><><><><><><><><> config.h <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//  Copyright(C) 2015 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
//  ROM configuration items
//  
//-----------------------------------------------------------------------------

#ifndef __CONFIG_H    // include only once
#define __CONFIG_H

// -------
// headers
// -------
#include "options.h"    // must be first include
#include "structsz.h"


// ----------------------------------------------------------------------------
//         F L A S H     M E M O R Y    C O N F I G U R A T I O N
// ----------------------------------------------------------------------------

// Read-Only Memory Manufacturing Information
// CAUTION!
//    This info is patched in at time of firmware burning and is Read-Only
//    and the structure MUST match the structure in the NP Final Tester program
#pragma pack(1)  // structure packing on byte alignment
typedef struct
{
    uint8_t  model_no [24];   // model number string  (null terminated)  21 in pc/MRP
    uint8_t  rev      [ 4];   // revision number      (null terminated)   2 in pc/MRP
    uint8_t  serial_no[20];   // serial number string (null terminated)  16 in pc/MRP
    uint8_t  mfg_date [20];   // YYYY-MM-DD HH:MM   date tested (null terminated) 
    uint8_t  future_use[188]; // set to FF's (erased)
} ROM_MFG_CONFIG_t;
STRUCT_SIZE_CHECK(ROM_MFG_CONFIG_t,256);
#pragma pack()  // restore packing setting


// ------------------
// access global data
// ------------------
extern const ROM_MFG_CONFIG_t  g_RomMfgInfo;
extern const char*             FwIdStr;

// --------------------   
// Function Prototyping 
// --------------------   
extern void cfg_ShowMfgInfo(void);

#endif  //  __CONFIG_H

// <><><><><><><><><><><><><> config.h <><><><><><><><><><><><><><><><><><><><><><>
