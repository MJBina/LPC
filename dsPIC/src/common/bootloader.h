// <><><><><><><><><><><><><> bootloader.h <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//  Copyright(C) 2013 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
//  Bootloader Sub-system for dsPIC33F Inverters
//    Work is not complete and has been put on hold
//
//-----------------------------------------------------------------------------

#ifndef _BOOTLOADER_H_    // include only once
#define _BOOTLOADER_H_


// -----------------------------------------------------------------------------
//                    Flash Program Memory Map
//                         dsPIC33FJ128 
// -----------------------------------------------------------------------------
//
//        Flash           Memory                 Flash Page is 0x400 words
//       Address         Contents
//       (Words)
//               +-------------------------+
//        0000h  |       reset address     |<--------------- Power on reset
//        0002h  |     jump instruction    |---->-----+
//               +-------------------------+          |
//        0004h  |    Interrupt Vector     |--->---+  |
//               |       Table (IVT)       |       |  |
//               +-------------------------+       |  |
//        0100h  |        reserved         |       |  |
//               |                         |       |  |
//               +-------------------------+       |  |
//        0104h  |   Alternate Interrupt   |-->-+  |  |        
//               |   Vector Table (AIVT)   |    |  |  |
//               +-------------------------+    |  |  |        
//        0200h  |      Manufacturing      |    |  |  |
//               |         ROM Data        |    |  |  |     B O O T L O A D E R  
//               |       (write once)      |    |  |  |                          
//               +-------------------------+    |  |  |         (16 pages)       
//        0300h  |   Non-Volatile Memory   |    |  |  |
//               |   (NVM) ROM Defaults    |    |  |  |
//               |      (write once)       |    |  |  |
//               +-------------------------+    |  |  |
//        0400h  |    __boot_entry:0       |    |  |  |
//               |        boot(0)          |    |  |  |
//               +-------------------------+    |  |  |
//        0402h  |    __boot_entry:1       |<------------- jumped to from Firmware
//               |     EnterBootloader()   |    |  |  |
//               +-------------------------+    |  |  |
//        0404h  |     Bootloader Code     |    |  |  |
//               |         Segment         |    |  |  |
//               |      (15 Kbytes)        |    |  |  |
//               |                         |    |  |  |
//               | +--------------------+  |    |  |  |
//               | | __DefaultInterrupt |<------+  |  |
//               | |        ISR         |  |       |  | 
//               | |    (Reflector)     |<---------+  |
//               | +--------------------+  |          |
//               |                         |          |
//               |                __reset  |<---------+
//               |                         |
//      ---------+-------------------------+---------------------------------------
//        4000h  |      reset address      |
//        4002h  |    jump instruction     |---+
//               +-------------------------+   |
//        4004h  |         Firmware        |<-------- Reflected from
//               |           IVT           |   |        __DefaultInterrupt ISR
//               +-------------------------+   |
//        4100h  |      Firmware Length    |   |
//               |                         |   |
//               +-------------------------+   |
//        4102h  |    Firmware Checksum    |   |
//               +-------------------------+   |
//        4103h  |   Is Flashed Indicator  |   |
//               +-------------------------+   |
//        4104h  |        Firmware         |<--------  Reflected from          
//               |          AIVT           |   |         __DefaultInterrupt ISR
//               +-------------------------+   |
//        4200h  |     Field Upgradable    |   |   
//               |         Firmware        |<--+
//               |           Code          |       
//               |                         |             F I R M W A R E
//               |                         |                          
//               |                         |                (70 pages)  
//       15800h  |                         |
//               +-------------------------+
//                        no memory
//               +-------------------------+
//       3FFFFh  |  Device Cfg Registers   |       F U S E    R E G I S T E R S
//               +-------------------------+
//
//
// -----------------------------------------------------------------------------

// ------------------------------
// Flash Program Memory Addresses
// ------------------------------
// Int Vector Table           0x0004-0x00FE
// Alternate IVT              0x0104-0x01FE
// Sensata Mfg Data           0x0200-0x02FF
// Sensata NVM ROM Defaults   0x0300-0x03FF
#define  BL_BASE_ADDRESS      0x400    // bootloader base address
#define  BL_FW_ENTRY1         0x402    // bootloader entry 1 from firmware
#define  FW_BASE_ADDRESS      0x4000   // firwmare   base address
#define  FW_LENGTH_ADDRESS    0x4100   // firmware length (two u16)
#define  FW_CHECKSUM_ADDRESS  0x4102   // firmware checksum (u16)
#define  FW_IS_FLASHED_FLAG   0x4103   // NP uses self-modifying code to detect when reflashed
#define  FW_CODE_ADDRESS      0x4200   // firwmare code address; start of checksum
#define  FUSE_REG_ADDRESS     0x3FFFF  // words

// -------------------
// program flash pages
// -------------------
#define  FLASH_PAGE_SIZE    0x400   // 1024 bytes
#define  FLASH_PAGES_TOTAL  86      // total flash memory pages
#define  FLASH_PAGES_BL     (FW_BASE_ADDRESS/FLASH_PAGE_SIZE)     // 16 pages
#define  FLASH_PAGES_FW     (FLASH_PAGES_TOTAL - FLASH_PAGES_BL)  // 70 pages

// --------------------------------
// flash memory gets erased to FF's
// --------------------------------
#define  ERASED_WORD_VALUE  0xFFFF  // flash memory word erased value
#define  ERASED_BYTE_VALUE    0xFF  // flash memory byte erased value (phantom byte; on an odd address)

// ------------------
// bootloader linkage
// ------------------
#ifdef WIN32
  #define JumpBootloader()   // do nothing
#else
  #define JumpBootloader()   asm("goto 0x402")  // __boot_entry:1 ;  does not return
#endif



#endif // _BOOTLOADER_H_

// <><><><><><><><><><><><><> bootloader.h <><><><><><><><><><><><><><><><><><><><><><>
