// <><><><><><><><><><><><><> nvm.h <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//	Copyright(C) 2015 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
// Non-Volatile Memory Driver
// 
// NVM Chip:  Microchip 24LC64 EEPROM SOT-23  5 pin    21189T.PDF
// I2C Bus #2
// Address 0xA0 (not changeable)
// 8K bytes
// 32 byte page writes
// full memory reads
//
//-----------------------------------------------------------------------------

#ifndef __NVM_H__    // include only once
#define __NVM_H__

// -------
// headers
// -------
#include "options.h"    // must be first include
#include "charger.h"
#include "converter.h"
#include "dsPIC33_CAN.h"
#include "device.h"
#include "inverter.h"
#include "structsz.h"
#include "timer1.h"

// ---------
// constants
// ---------
#define  NVM_PAGE_BYTES      (32)   // number of bytes per NVM page memory
#define  NVM_BYTES_TOTAL  (8*1024)  // number of bytes total on chip
#define  NVM_PAGES    (NVM_BYTES_TOTAL/NVM_PAGE_BYTES)  // 256 number of pages

// byte offsets for two configuration copies
#define  NVM_OFFSET_COPY1    (0)
#define  NVM_OFFSET_COPY2    (NVM_BYTES_TOTAL/2)
// number of bytes to page end
#define  NVM_BYTES_TO_PAGE_END(addr)   (NVM_PAGE_BYTES - ((addr)&(NVM_PAGE_BYTES-1)))

#define  NVM_FILL_BYTE   (0x00)
#define  NVM_ERASE_BYTE  (0xFF) // gets erased to this value


// ------
// status
// ------
#define NVM_STATUS_OK     0 // completed ok; ready for another operation (must be zero)
#define NVM_STATUS_BUSY   1 // busy
#define NVM_STATUS_ERR    2 // completed with errors


// --------------
// NVM signatures
// --------------
// constant values to indicate valid starting block of data
#define NVM_SIGA  'N'
#define NVM_SIGB  'V'


// ------------
// NVM Settings
// ------------


#define NVM_SETTING_SIZE  (256)

#define NVM_UNUSED_SIZE 	(NVM_SETTING_SIZE - sizeof(CAN_CONFIG_t) - \
	sizeof(DEVICE_CONFIG_t)  - sizeof(INVERTER_CONFIG_t) - \
	sizeof(CHARGER_CONFIG_t) - sizeof(CONVERTER_CONFIG_t))

#pragma pack(1)  // structure packing on byte alignment
typedef struct
{
	CAN_CONFIG_t		can;
	DEVICE_CONFIG_t		dev;		
	INVERTER_CONFIG_t	inv;
	CHARGER_CONFIG_t	chgr;
	CONVERTER_CONFIG_t	conv;
    uint8_t         	future_use[NVM_UNUSED_SIZE];
} NVM_SETTINGS_t;
#pragma pack()  // restore packing setting

STRUCT_SIZE_CHECK(NVM_SETTINGS_t, NVM_SETTING_SIZE);


// ----------------
// NVM Header Block
// ----------------
#pragma pack(1)  // structure packing on byte alignment
typedef struct
{
    uint8_t   sigA;         // NVM_SIGA
    uint8_t   sigB;         // NVM_SIGB
    uint16_t  chksumCfg;    // cfg checksum
    // checksum starts here thru end of layout.cfg data
    uint16_t  nBytes;       // num cfg bytes
    // nBytes is sizeof of layout.cfg
    uint16_t  revNum;       // revision number lo byte
    uint8_t   reserved[24]; // reserved for future use; set to zero
} NVM_HDR_BLK_t;
STRUCT_SIZE_CHECK(NVM_HDR_BLK_t,32);  // must be 32 bytes for NVM
#pragma pack()  // restore packing setting

#define NVM_CHECKSUM_SIZE  (sizeof(NVM_SETTINGS_t) + sizeof(NVM_HDR_BLK_t) - 4)

// ----------
// NVM Layout
// ----------
#pragma pack(1)  // structure packing on byte alignment
typedef struct
{
    NVM_HDR_BLK_t   hdr;                    // header block
    union  
    {
        NVM_SETTINGS_t  settings;       // settings data
        uint8_t         bytes[sizeof(NVM_SETTINGS_t)];  // access as bytes
    };
} NVM_LAYOUT_t;
#pragma pack()  // restore packing setting

// ----------------------
// Level 0 driver state
// ----------------------
#pragma pack(1)  // structure packing on byte alignment
typedef struct  // holds state for executing a single NVM chip command
{
    // single page read/writes; driven by isr
    uint8_t   cmd;        // 1=write, 2=read
    uint8_t*  buf;        // buffer address
    uint16_t  n;          // number of bytes
    uint16_t  addr;       // NVM data address
    uint8_t   status;     // completion status; NVM_STATUS_nnn (isr updates)
    uint8_t   state;      // state of isr state-machine (isr updates)
    uint16_t  index;      // index into buf[] array (isr updates)
    uint8_t   nretries;   // retry counter (isr updates)

} NVM_CMD_L0_t;
#pragma pack()  // restore packing setting


// multi-page writes; driven by nvm_Driver()
#pragma pack(1)  // structure packing on byte alignment
typedef struct
{
    // inputs
    uint16_t  addr;       // address of NVM to write to
    uint16_t  nbytes;     // number of bytes to write
    uint8_t*  data;       // memory buffer
    // state
    uint16_t  nToWrite;   // number of bytes to write for this page
    uint16_t  nWritten;   // number of bytes written in this block
    uint16_t  nLeft;      // number of bytes left to write (0=nothing to do)
    uint8_t   state;      // 1=start write, 2=wait for delay, 3=advance
    SYSTICKS  startTicks; // wait timer delay between packet writes
} NVM_MP_L0_t;
#pragma pack()  // restore packing setting


// ------------
// NVM Commands
// ------------
#define NVM_CMD_NONE       0  // no command (do not set directly)
#define NVM_CMD_SAVE_CFG   1  // save config to nvm


// ---------------------
// NVM Driver state data
// ---------------------
#pragma pack(1)  // structure packing on byte alignment
typedef struct
{
    uint8_t   reqCmd;         // requested command
    uint8_t   curCmd;         // current executing command
    uint8_t   cmdState;       // current command state 0=none, 1=start; others depend upon command
    uint8_t   isDirty;        // 0=no, 1=need to save to non-volatile memory
    uint16_t  revNum;         // rev number
    SYSTICKS  saveTicks;      // last time config was saved to nvm
    NVM_CMD_L0_t   isr;       // interrupt drive commands (level 0)
    NVM_MP_L0_t    mpwr;      // multi-page writes (level 0)
    NVM_LAYOUT_t   scratch;   // scratch buffer for reading and writing
    NVM_SETTINGS_t settings;  // active settings
} NVM_DRV_t;
#pragma pack()  // restore packing setting


// -----------
// Prototyping
// -----------
void nvm_Config(void);      // configure and initialize nvm driver
void nvm_Start(void);       // start nvm interrupts
void nvm_Stop(void);        // stop  nvm interrupts
void TASK_nvm_Driver(void);      // call this periodically to drive the nvm state machine
void nvm_ReqSaveConfig(void);
void nvm_LoadRomSettings(void);
void nvm_ApplySettings(void);


// ------------------
// Access Global Data
// ------------------
extern NVM_DRV_t  g_nvm;


//-------------------
// Inlines for Speed
//-------------------
// set dirty bit when config changes to indicate needs saving
INLINE void nvm_SetDirty()
{
    g_nvm.isDirty=1; 
    // LOG(SS_NVM, SV_INFO, "SetDirtyBit");
}

#endif   // __NVM_H__

// <><><><><><><><><><><><><> nvm.h <><><><><><><><><><><><><><><><><><><><><><>
