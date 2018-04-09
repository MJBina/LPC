// <><><><><><><><><><><><><> J1939.h <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//	Copyright(C) 2012 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
// 	J1939 protocol definitions
//	
//-----------------------------------------------------------------------------

#ifndef __J1939_H_	// avoid repeated including
#define __J1939_H_

// --------
// headers
// --------
#include "options.h"    // must be first include
#include "dsPIC33_CAN.h"	// J1939 relies on CAN bus

// -------------------------
// J1939 Default Priorities
// -------------------------
#define J1939_DEFAULT_PRIORITY		0x06
#define J1939_CONTROL_PRIORITY		0x03
#define J1939_INFO_PRIORITY			0x06
#define J1939_PROPRIETARY_PRIORITY	0x06
#define J1939_REQUEST_PRIORITY		0x06
#define J1939_ACK_PRIORITY			0x06
#define J1939_TP_CM_PRIORITY		0x07
#define J1939_TP_DT_PRIORITY		0x07

// ------------------------
// J1939 Defined Addresses
// ------------------------
#define J1939_GLOBAL_ADDRESS		0xFF // 255
#define J1939_NULL_ADDRESS			0xFE // 254  not yet claimed


// -------------------------------------------------------------------------------------------------------------
//  CA  - Controller Application (has a unique address and name)
//  ECU - Electronic Control Unit (may contain more than one CA)
//  BAM - Broadcast Announcement Message
//  PGN - Parameter Group Number  
//  SPN - Suspect Parameter Number
//  SA  - Source Address
//  DA  - Destination Address
//  PDU - Protocol Data Unit
//
//    +----------+-----------+----------+------------+--------------+----------------+
//    | Priority | XDatapage | DataPage | PDU Format | PDU Specific | Source Address |
//    +----------+-----------+----------+------------+--------------+----------------+
//    |   3 bits |   1 bit   |   1 bit  |    8 bits  |   8 bits     |     8 bits     |
//    +----------+-----------+----------+------------+--------------+----------------+
//
//   PDU format < 240 (peer-to-peer)
//        PDU specific contains the target address. Global (255) can also be used as target address.
//        Then the parameter group is aimed at all devices. 
//        In this case, the PGN is formed only from PDU format.
//   PDU format >= 240 (broadcast), 
//        PDU format together with the Group Extension in the PDU specific field 
//        forms the PGN of the transmitted parameter group.
//
//  There are two types of Parameter Group Numbers:
//    Global PGNs identify parameter groups that are sent to all (broadcast). 
//        Here the PDU Format, PDU Specific, Data Page and Extended Data Page are
//        used for identification of the corresponding Parameter Group. 
//        On global PGNs the PDU Format is 240 or greater and the PDU Specific field is a Group Extension.
//    Specific PGNs are for parameter groups that are sent to particular devices (peer-to-peer).
//        Here the PDU Format, Data Page and Extended Data Pare are used for 
//        identification of the corresponding Parameter Group. 
//        The PDU Format is 239 or less and the PDU Specific field is set to 0.
//
// -------------------------------------------------------------------------------------------------------------


// ------------------------------------------------
// Some J1939 PDU Formats, Control Bytes, and PGN's
// ------------------------------------------------

#define J1939_ACK_CONTROL_BYTE				0
#define J1939_NACK_CONTROL_BYTE				1
#define J1939_ACCESS_DENIED_CONTROL_BYTE	2
#define J1939_CANNOT_RESPOND_CONTROL_BYTE	3

// DGN less than 0xF000 it is target specific rather than entire network
#define J1939_RTS_CONTROL_BYTE				0x10	// Request to Send control byte of CM message
#define J1939_CTS_CONTROL_BYTE				0x11	// Clear to Send control byte of CM message
#define J1939_EOMACK_CONTROL_BYTE			0x13	// End of Message control byte of CM message
#define J1939_BAM_CONTROL_BYTE				0x20	// BAM control byte of CM message
#define J1939_CONNABORT_CONTROL_BYTE		0xFF	// Connection Abort control byte of CM message

#define J1939_PGN0_REQ_ADDRESS_CLAIM		0x00
#define J1939_PGN1_REQ_ADDRESS_CLAIM		0xEA    // 234
#define J1939_PGN2_REQ_ADDRESS_CLAIM		0x00

#define J1939_PGN0_COMMANDED_ADDRESS		0xD8    // 216 (PDU Specific)
#define J1939_PGN1_COMMANDED_ADDRESS		0xFE    // 254
#define J1939_PGN2_COMMANDED_ADDRESS		0x00

#define J1939_PF_CANNOT_CLAIM_ADDRESS		0xEE	// With null address

// ----------------------
// J1939 PF (PDU Formats)
// ----------------------
#define J1939_PF_REQUEST2					0xC9    // 201
#define J1939_PF_TRANSFER					0xCA    // 202
#define J1939_PF_REQUEST_REPETITION_RATE	0xCC    // 204
#define J1939_PF_TIME_DATE_ADJUST           0xD5    // 213
#define J1939_PF_RESET                      0xDE    // 222
#define J1939_PF_VT_TO_NODE                 0xE6    // 230
#define J1939_PF_MODE_TO_VT                 0xE7    // 231
#define J1939_PF_ACK                        0xE8    // 232
#define J1939_PF_REQUEST_DGN                0xEA    // 234
#define J1939_PF_TP_DATA_TRANSFER           0xEB    // 235
#define J1939_PF_TP_CONNECT_MGMT            0xEC    // 236
#define J1939_PF_NETWORK_LAYER              0xED    // 237
#define J1939_PF_ADDRESS_CLAIMED            0xEE    // 238
#define J1939_PF_PROPRIETARY_A              0xEF    // 239
#define J1939_PF_PROPRIETARY_B				0xFF    // 240

// ---------
// J1939 DGN
// ---------
#define J1939_DGN_INITIAL_MULTI_PACKET      0x0ECFF    // initial multi-packet
#define J1939_DGN_SUBSEQUENT_MULTI_PACKET   0x0EBFF    // subsequent multi-packet
#define J1939_DGN_ACK                     	0x0E800	   // 


// ------------------------------------------------------
//  S T A N D A R D    A C K N O W L E D G M E N T S
// ------------------------------------------------------
#define J1939_ACK 				0
#define J1939_NAK 				1


// -----------
// State Data
// -----------
// global data for J1939 module
#pragma pack(1)  // structure packing on byte alignement
typedef struct 
{
	uint8_t		CA_Name[MAX_CAN_DATA];  // controller application name
	uint8_t		CommandedAddressSource;
	uint8_t 	CommandedAddressName[MAX_CAN_DATA];
	uint32_t 	ContentionWaitTime;
	uint8_t 	MyInvAddress;  // J1939 inverter address
	uint8_t 	MyChgrAddress; // J1939 charger  address
	uint8_t 	TxPriority;	// message transmit priority

	// flags
	uint16_t	CannotClaimAddress;
	uint16_t	WaitingForAddressClaimContention;
	uint16_t	GettingCommandedAddress;
	uint16_t	GotFirstDataPacket;
	uint16_t	ReceivedMessagesDropped;

	// working message
	CAN_MSG		OneMessage;

} J1939_STATE;
#pragma pack()  // restore packing setting


// -----------
// Prototyping
// -----------
void J1939_Config(void);
void J1939_Start(void);
void J1939_InitMsg(CAN_MSG* msg);
void J1939_SetInvAddress(uint8_t address);
void J1939_SetDGN (CAN_MSG* msg, CAN_DGN dgn);
void J1939_BuildCtrlAppName(uint32_t serialNumber,uint16_t mfgCode,uint8_t instance);
void J1939_SetPriority(uint8_t priority);
void J1939_SendMessage(CAN_DGN dgn, CAN_LEN dataLen, CAN_DATA* data);
void J1939_SendAckNak(uint8_t code, CAN_DGN ackedDGN);
void J1939_SendAck(CAN_DGN ackedDGN);
void J1939_SendNak(CAN_DGN nackedDGN);
void J1939_SendAddressRequest(uint8_t desiredAddress);
void J1939_SendRequestForDGN(uint32_t dgn, uint8_t destAddress);
void J1939_SendAddressClaimed(void);
int  J1939_SendMultiPacketMessage(CAN_DGN dgn, uint16_t dataLen, CAN_DATA* data);

#endif	// __J1939_H_

// <><><><><><><><><><><><><> J1939.h <><><><><><><><><><><><><><><><><><><><><><>
