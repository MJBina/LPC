// <><><><><><><><><><><><><> dsPIC33_CAN.h <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//  Copyright(C) 2015 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
//  CAN bus layer data structures for the dsPIC33
//  
//-----------------------------------------------------------------------------

#ifndef __DSPIC33_CAN_H__   // avoid repeated including
#define __DSPIC33_CAN_H__

// -------
// headers
// -------
#include "options.h"    // must be first include

// -----------
// Basic types
// -----------
typedef uint32_t  CAN_ID;   // device id;  11 bit or 29 bit
typedef uint32_t  CAN_DGN;  // data group number
typedef uint8_t   CAN_LEN;  // CAN data length
typedef uint8_t   CAN_DATA; // CAN data byte
typedef uint8_t   CAN_INST; // CAN instance
typedef uint8_t   CAN_QPTR; // CAN queue pointer
typedef uint8_t   CAN_BAUD; // CAN baud
typedef uint16_t  DMA_WORD; // Caution! dma memory is accessed as 16-bit words
// guarantee proper sizes
STRUCT_SIZE_CHECK(CAN_DATA, 1);
STRUCT_SIZE_CHECK(CAN_LEN , 1);
STRUCT_SIZE_CHECK(CAN_ID  , 4);
STRUCT_SIZE_CHECK(CAN_DGN , 4);
STRUCT_SIZE_CHECK(CAN_QPTR, 1);
STRUCT_SIZE_CHECK(CAN_INST, 1);
STRUCT_SIZE_CHECK(DMA_WORD, 2);


// -----------
// Max / Mins
// -----------
#define MAX_CAN_DATA      (8)  // maximum CAN data bytes in message
#define MIN_CAN_INSTANCE  (1)  // minimum CAN instance
#define MAX_CAN_INSTANCE  (15) // maximum CAN instance RVC spec has 13

// -------------------
// Enhanced CAN modes
// -------------------
#define ECAN_MODE_NORMAL        0
#define ECAN_MODE_DISABLE       1
#define ECAN_MODE_LOOPBACK      2
#define ECAN_MODE_LISTEN        3
#define ECAN_MODE_CONFIGURE     4
#define ECAN_MODE_LISTEN_ALL    7

// -----------------
// CAN bus baud rate
// -----------------
#define CAN_BAUD_250K  0  // default
#define CAN_BAUD_500K  1
#define CAN_BAUD_1000K 2


// -----------------------------
//  CAN Message Circular Buffers
// -----------------------------
#define CAN_TX_QUEUE_SIZE   32  // number of CAN messages in transmit queue
#define CAN_RX_QUEUE_SIZE   32  // number of CAN messages in receive  queue


// --------------
// CAN Device ID
// --------------

#pragma pack(1)  // structure packing on byte alignment
typedef union 
{
    struct  // access the id as a J1939 packet; only 29 bit allowed
    {
        uint8_t     SourceAddress;  // byte 3
        uint8_t     PDUSpecific;    // byte 2  DGN Lo byte
        uint8_t     PDUFormat;      // byte 1  DGN Hi byte

        // msb
        uint8_t     DataPage    : 1;    // RVC sets to 1
        uint8_t     Reserved    : 1;    // 
        uint8_t     Priority    : 3;    // 0=highest, 7=lowest
        uint8_t     Zero        : 3;    // always 0
    };
    CAN_ID  ID; // can device id 
} JCAN_ID;  // 4 bytes
STRUCT_SIZE_CHECK(JCAN_ID,4);
#pragma pack()  // restore packing setting

// ------------
// CAN message
// ------------

// frame types
#define CAN_FRAME_STD   'S' // Standard message 11 bit id
#define CAN_FRAME_EXT   'X' // Extended message 29 bit id 
                        
// message types        
#define CAN_MSG_DATA    'D' // data message
#define CAN_MSG_RTR     'R' // Remote Transmission Request message 

#pragma pack(1)  // structure packing on byte alignment
typedef struct
{
    JCAN_ID jid;            // id address
    uint8_t frameType;      // CAN_FRAME_STD or CAN_FRAME_EXT
    uint8_t msgType;        // CAN_MSG_DATA  or CAN_MSG_RTR
    uint8_t align;          // used for alignment to 16 bytes
    CAN_LEN dataLength;     // number of valid bytes in data[]
    CAN_DATA    data[MAX_CAN_DATA]; // data payload
} CAN_MSG;  // 16 bytes
#pragma pack()  // restore packing setting


// ---------------------------------
// CAN data placed in DMA buffers
// is required in this packed format
// ---------------------------------
#define CAN_DMA_WORDS   8   // number of 16-bit words in CAN DMA message

#pragma pack(1)  // structure packing on byte alignment
typedef struct
{
    DMA_WORD    word[CAN_DMA_WORDS];

    // ------------------------------
    //  DMA Buffer Format:
    //  word0 : 0bUUUx xxxx xxxx xxxx
    //               |____________|||
    //                  SID10:0   SRR IDE(bit 0)
    //  word1 : 0bUUUU xxxx xxxx xxxx
    //                 |____________|
    //                      EID17:6
    //  word2 : 0bxxxx xxx0 UUU0 xxxx
    //            |_____||       |__|
    //            EID5:0 RTR      DLC
    //  word3-word6: data bytes
    //  word7: filter hit code bits
    //
    //  Remote Transmission Request Bit for standard frames
    //  SRR->   "0"  Normal Message
    //          "1"  Message will request remote transmission
    //  Substitute Remote Request Bit for extended frames
    //  SRR->   should always be set to "1" as per CAN specification
    //
    //  Extended  Identifier Bit
    //  IDE->   "0"  Message will transmit standard identifier
    //          "1"  Message will transmit extended identifier
    //
    //  Remote Transmission Request Bit for extended frames
    //  RTR->   "0"  Message transmitted is a normal message
    //          "1"  Message transmitted is a remote message
    //  Don't care for standard frames
    // -------------------------------
} CAN_DMA;  // 16 bytes
STRUCT_SIZE_CHECK(CAN_DMA ,16);
#pragma pack()  // restore packing setting


// --------------------------
// Global CAN Data Structure
// --------------------------
#pragma pack(1)  // structure packing on byte alignment
typedef struct
{
    CAN_ID      MyID;           // this device's CAN ID
    CAN_INST    MyInstance;     // device instance 1-13 Note: 6.20.1 Inverter/Charger have same instance

    // receive queue
    CAN_QPTR    RxHead;     // points to next put in
    CAN_QPTR    RxTail;     // points to next take out
    CAN_DMA     RxQueue[CAN_RX_QUEUE_SIZE];

    // transmit queue
    CAN_QPTR    TxHead;     // points to next put in
    CAN_QPTR    TxTail;     // points to next take out
    CAN_DMA     TxQueue[CAN_TX_QUEUE_SIZE];

    // RVC communication status 1  TODO set these
    uint16_t    rvcRxErrCount;  // The number of errors encountered receiving incoming CAN messages.
    uint16_t    rvcTxErrCount;  // The number of errors encountered transmitting CAN messages.

    // RVC communication status 2
    uint32_t    rvcTxFrameCount;  // The number of can packets transmitted by this node
    uint32_t    rvcRxFrameCount;  // The number of can packets received    by this node

    // RVC communication status 3  TODO set these
    uint16_t    rvcBusOffErr;       // The number of bus-off errors detected.
    uint16_t    rvcRxFramesDropped; // The number of receive  frames dropped.
    uint16_t    rvcTxFramesDropped; // The number of transmit frames dropped. 

    // configuration
    CAN_BAUD    baud;
    uint8_t     address;
    CAN_INST    instance;       // device instance 1-13 Note: 6.20.1 Inverter/Charger have same instance

} CAN_STATE;
#pragma pack()  // restore packing setting


#pragma pack(1)  // structure packing on byte alignment
typedef struct
{
    uint8_t	baud;
    uint8_t address;
    uint8_t instance; 	// device instance 1-13 Note: 6.20.1 Inverter/Charger have same instance
	uint8_t unused;		// an extra byte to make size and even number.
} CAN_CONFIG_t;			// (4-bytes)
#pragma pack()  // restore packing setting

#if !defined(ALLOCATE_SPACE_ROM_DEFAULTS)
	extern const CAN_CONFIG_t CanConfigDefaults;
#else
	const CAN_CONFIG_t CanConfigDefaults = 
	{
	  #ifdef OPTION_CAN_BAUD_500K
		CAN_BAUD_500K,
	  #else
		CAN_BAUD_250K, // can_baud
	  #endif

	  // can_address
	  #ifdef OPTION_DC_CONVERTER
		67,  	// dc+dc converter address
	  #else
		66, 	// inverter
	  #endif

		1, 		// can_instance // device instance 1-13 Note: 6.20.1 Inverter/Charger have same instance
		0		//	<unused>
	};
	
#endif 


// -------------------
// Access global data.
// -------------------
extern CAN_STATE      g_can;


// -------------
//  Prototyping
// -------------
void can_SetBaudRate(CAN_BAUD baudRate);
void can_SetID(CAN_ID id);
void can_SetMode(unsigned char Mode);
void can_Config(void);
void can_Start(void);
void can_Stop(void);
void TASK_can_Driver(void);
void can_DumpMsg(CAN_MSG* canMsg);
int16_t  can_SetInstance(CAN_INST instance);
uint32_t can_DGN(CAN_MSG* msg);
int16_t  can_IsTxQueueFull(void);
int16_t  can_IsRxQueueFull(void);
int16_t  can_IsTxQueueEmpty(void);
int16_t  can_IsRxQueueEmpty(void);
void     can_TxDriver(void);
int16_t  can_TxEnqueue(CAN_MSG* msg);
void     J1939_Poll(unsigned long ElapsedTime);
int16_t  J1939_Dispatcher(CAN_MSG* msg);
uint8_t  J1939_InvAddress(void);
uint8_t  J1939_ChgrAddress(void);
int16_t  rvcan_Dispatcher(CAN_MSG* msg);

#endif  //  __DSPIC33_CAN_H__

// <><><><><><><><><><><><><> dsPIC33_CAN.h <><><><><><><><><><><><><><><><><><><><><><>
