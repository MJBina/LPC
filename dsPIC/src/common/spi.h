// <><><><><><><><><><><><><> spi.h <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//  Copyright(C) 2015 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
//  SPI Remote LCD Communications Driver
//
//-----------------------------------------------------------------------------

#ifndef __SPI_H    // include only once
#define __SPI_H

// -------
// headers
// -------
#include "options.h"    // must be first include


// ---------------------------------------------------------------------
//             L E G A C Y     S P I     P R O T O C O L
// ---------------------------------------------------------------------
//
//            Device to Display SPI Packet
//        +---------------------+----------+---+
//        | 32 ASCII Characters | CheckSum | : |
//        +---------------------+----------+---+
//         0      * * *       31     32      33    
//
// ---------------------------------------------------------------------
#define SPI_LEGACY_SYNC_BYTE   (':')   // sync byte
#define SPI_LEGACY_MSG_LEN     (32)    // 2 lines of 16 characters each
#define SPI_LEGACY_PKT_BYTES   (34)    // #bytes including sync byte
// checksum is the sum of 32 ASCII characters

// offset into LcdBuf; top bits are turned on as indicators
#define LCD_HANDSHAKE_BIT_INDEX   (15)  // indicates to the LCD that the device is enhanced    
#define LCD_INV_BIT_INDEX         (28)  // indicates invertering
#define LCD_CHG_BIT_INDEX         (29)  // indicates if in charger mode (AC line is qualified)
#define LCD_EXIT_BIT_INDEX        (30)  // command LCD to exit if in Info or Settings mode     
#define LCD_RED_BIT_INDEX         (31)  // turn LCD red LED on/off                             

// ---------------------------------------------------------------------
//
//           Display to Device SPI Packet (one byte packet)
//
//   Status Byte   
//     bit 0 = 0
//	   bit 1 = LSB for ChgrLimitIndex. (1=0A, 2=5A,...7=28A).
//	   bit 2 =	 * * *
//	   bit 3 = MSB for ChgrLimitIndex. 
//     bit 4 = Equalization Mode bit. (1 = Equalization Mode). NOT SUPPORTED
//     bit 5 = 0
//     bit 6 = 0
//     bit 7 = 0
//
// ---------------------------------------------------------------------
#define  TX_BYTE_EQUALIZE  0x10 // Equalization Mode

#define IS_POSSIBLE_LEGACY_BCR(byte)   ((((byte) & 0xE0)==0)?1:0)


// ---------------------------------------------------------------------
//          M E N U I N G     L C D    S P I     P R O T O C O L
// ---------------------------------------------------------------------
// CAUTION! these packets use the same format as the CAN sub-system
//            to allow leveraging that code base
// ---------------------------------------------------------------------

// SPI Transport Protocol Level 0
// these byte patterns were chosen because they are ignored by legacy inverter firmware
#define SPI_TXWORD_SYNC_BYTE   (0x81)  // slave sends back two bytes for every payload byte; first is sync byte
#define SPI_RMT2_IDLE_BYTE     (0xE1)  // sent back by new remote if no data to send

// SPI Command Protocol Level 1
#define SPI_SYNC1    (0xA5)   // sync byte #1
#define SPI_SYNC2    (0xB6)   // sync byte #2
#define SPI_NPARMS    (8)

#define SPI_SYNC_WORD_HI   (0xE1)  // slave sends back two bytes for every payload byte; fist is sync byte

// SPI Packet Header
#pragma pack(1)  // must not be padded since this is data received on the wire
typedef struct SPI_HDR_t
{
    uint8_t  sync1;             // SPI_SYNC1
    uint8_t  cmdNum;            // SPI_CMD number
    uint8_t  ndata;             // number of bytes of payload data (255 max)
    uint8_t  parms[SPI_NPARMS]; // parameter bytes (same as CAN sub-system)
    uint8_t  hdr_cs;            // header check sum; sum of bytes sync1->parms[7]
    uint8_t  sync2;             // SPI_SYNC2
    // optional data sent (only if ndata>0)
    // uint8_t data[ndata];    // payload bytes (if any)
    // uint8_t data_cs;        // data checksum; sum of bytes of data[]
} SPI_HDR;  // 13 bytes
STRUCT_SIZE_CHECK(SPI_HDR, 13);
#pragma pack()  // restore packing setting

// byte offsets (zero based)  compiler does not support offsetof()
#define SPI_SYNC1_OFFSET   0
#define SPI_CMDNUM_OFFSET  1
#define SPI_NDATA_OFFSET   2
#define SPI_HDR_CS_OFFSET  11
#define SPI_SYNC2_OFFSET   12
#define SPI_HDR_BYTES      sizeof(SPI_HDR)  // 13

// ----------------------
// LCD to Device Packets
// ----------------------
#define SPI_CMD_GET_FIELD_REQ   0x01 // request  to get field
   // ndata = 0
   // parm[0]=0
   // parm[1]=field Num (lo byte)  SENFLD_nnn defined in sensata_can.h
   // parm[2]=field Num (hi byte)
   // parm[3]=0
   // parm[4]=0
   // parm[5]=0
   // parm[6]=0
   // parm[7]=0

#define SPI_CMD_SET_FIELD_REQ   0x02 // request  to set field
   // ndata = 0
   // parm[0]=0
   // parm[1]=field Num (lo byte)  SENFLD_nnn defined in sensata_can.h
   // parm[2]=field Num (hi byte)
   // parm[3]=field type (0=none, 1=int16, 2=float, 3=int32, 4=string, 5=Q16
   // parm[4]=value LSB
   // parm[5]=  * * *
   // parm[6]=  * * *
   // parm[7]=value MSB

#define SPI_CMD_REBOOT_DEVICE   0x03 // request to reboot device
   // ndata = 0
   // parm[0]=0
   // parm[1]=0
   // parm[2]=0
   // parm[3]=0
   // parm[4]=0
   // parm[5]=0
   // parm[6]=0
   // parm[7]=0

#define SPI_CMD_FACTORY_DEFAULTS (0x04) // request device to set to factory defaults
   // ndata = 0
   // parm[0]=0
   // parm[1]=0
   // parm[2]=0
   // parm[3]=0
   // parm[4]=0
   // parm[5]=0
   // parm[6]=0
   // parm[7]=0

#define SPI_CMD_INV_ENABLE_DISABLE (0x05) // request enable/disable inverter
   // ndata = 0
   // parm[0]=0=disable, 1=enable
   // parm[1]=0
   // parm[2]=0
   // parm[3]=0
   // parm[4]=0
   // parm[5]=0
   // parm[6]=0
   // parm[7]=0

// ----------------------
// Device to LCD Packets
// ----------------------
#define SPI_CMD_GET_FIELD_RSP   0x81 // response to get field request
   // ndata = 0
   // parm[0]=0
   // parm[1]=field Num (lo byte)  SENFLD_nnn defined in sensata_can.h
   // parm[2]=field Num (hi byte)
   // parm[3]=field type (0=none, 1=int16, 2=float, 3=int32, 4=string, 5=Q16
   // parm[4]=value LSB
   // parm[5]=  * * *
   // parm[6]=  * * *
   // parm[7]=value MSB

#define SPI_CMD_SET_FIELD_RSP   0x82 // response to set field request (echo data)
   // ndata = 0
   // parm[0]=0
   // parm[1]=field Num (lo byte)  SENFLD_nnn defined in sensata_can.h
   // parm[2]=field Num (hi byte)
   // parm[3]=field type (0=none, 1=int16, 2=float, 3=int32, 4=string, 5=Q16
   // parm[4]=value LSB
   // parm[5]=  * * *
   // parm[6]=  * * *
   // parm[7]=value MSB

// longest SPI message to expect or send
#define SPI_MAX_PKT_LEN   (60)   // sizeof(SPI_HDR) + SENFLD_MAX_BYTES  // 13 + 41 = 54

// --------------------
// Function Prototyping
// --------------------
uint8_t IsRmtEnhanced(void);
void    spi_ClearHandshake(void);
void    spi_Config(void);
void    spi_Start(void);
void    spi_Stop(void);
int     spi_PutBuf(uint8_t* buf, int16_t len);
int     spi_PutLcdMsg(uint8_t* data, int16_t ndata, uint8_t led_red, uint8_t lcd_exit);
void    spi_TxByte(uint8_t tx_byte);
uint8_t spi_CalcChecksum(uint8_t* buf, uint16_t nbytes);
void    TASK_spi_Driver(void);


#endif  //  __SPI_H

// <><><><><><><><><><><><><> spi.h <><><><><><><><><><><><><><><><><><><><><><>
