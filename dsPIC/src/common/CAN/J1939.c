// <><><><><><><><><><><><><> J1939.c <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//	Copyright(C) 2015 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
//	J1939 CAN bus layer implementation
//	
//-----------------------------------------------------------------------------

// -------
// headers
// -------
#include "options.h"    // must be first include
#include "config.h"
#include "dsPIC33_CAN.h"
#include "inverter_cmds.h"
#include "J1939.h"
#include "nvm.h"
#include "rv_can.h"


// ---------
// Constants
// ---------
#define ADDRESS_CLAIM_TX   1
#define ADDRESS_CLAIM_RX   2
#define STARTUP_INVERTER_ADDRESS  RVC_DEFAULT_INVERTER_ADDRESS1

// -------------
// Global state
// -------------
J1939_STATE  g_J1939;  // global state for J1939

// ----------------------------------------------------------------------
// initialize CAN message for J1939
void J1939_InitMsg(CAN_MSG* msg)
{
	memset(msg,0,sizeof(CAN_MSG));
	msg->frameType = CAN_FRAME_EXT;
	msg->msgType   = CAN_MSG_DATA;
}

// ----------------------------------------------------------------------
// set this inverter address
void J1939_SetInvAddress(uint8_t address)
{
    if (address == g_J1939.MyInvAddress) return; // already set
	LOG(SS_J1939, SV_INFO,"SetInvAddr=%u", address);
    g_J1939.MyInvAddress        = address;
    g_can.MyID = (g_can.MyID & 0xFFFFFF00) | address;   // LSB is CA address
    can_SetID(g_can.MyID);  // set the CAN hardware address
}

// ----------------------------------------------------------------------
// set DGN into CAN message
void J1939_SetDGN(CAN_MSG* msg, CAN_DGN dgn)
{
    dgn = dgn & 0x1FFFF;  // includes data-page
    msg->jid.ID            = dgn << 8;
    msg->jid.SourceAddress = g_J1939.MyInvAddress;
    msg->jid.Priority      = g_J1939.TxPriority;
}

// ----------------------------------------------------------------------
// CompareName for determining address priority
// 
// This routine compares the passed in array data NAME with the CA's
// current NAME as stored in CA_Name. 
// 
// Parameters:	uint8_t *		Array of NAME bytes
// Return: -1  - CA_Name is less than the data
// 		 	0  - CA_Name is equal to the data
// 			1  - CA_Name is greater than the data
// ----------------------------------------------------------------------
static signed char CompareName( uint8_t *OtherName )
{
	int  i;

    for (i=7; i>=0; i--)  // must compare MSB first
    {
        if (g_J1939.CA_Name[i] == OtherName[i]) continue;
        if (g_J1939.CA_Name[i] <  OtherName[i]) 
        {
            LOG(SS_J1939, SV_INFO, "CA-NAME: ours < theirs");
            return(-1); // our name is less than the other
        }
        LOG(SS_J1939, SV_INFO, "CA-NAME: ours > theirs");
        return(1); // our name is greater than other
    } // for
    LOG(SS_J1939, SV_INFO, "CA-NAME: ours == theirs");
	return(0);   // they are the same
}

// ----------------------------------------------------------------------
// Copies the CA's NAME into the callers buffer
// ----------------------------------------------------------------------
static void CopyName(unsigned char* buf)
{
    int i;
	for (i=0; i<MAX_CAN_DATA; i++)
	{
		buf[i] = g_J1939.CA_Name[i];
	}
}

// ----------------------------------------------------------------------
// new address to claim; can be modified
// returns: 0=dont try to claim, 1=try to claim
static int CA_RecalculateAddress(uint8_t* newAddress)
{
    // TODO calculate new address if needed
    return(1);
}

// ----------------------------------------------------------------------
// Transmit our address to the CAN bus
// ----------------------------------------------------------------------
static void J1939_TxAddressClaim()
{
    CAN_DGN  dgn = (J1939_PF_ADDRESS_CLAIMED<<8) | J1939_GLOBAL_ADDRESS;
    J1939_SendMessage(dgn, sizeof(g_J1939.CA_Name), g_J1939.CA_Name);

	if (((g_J1939.MyInvAddress & 0x80) == 0) ||   // Addresses 0-127
		((g_J1939.MyInvAddress & 0xF8) == 0xF8))  // Addresses 248-253 (254,255 illegal)
	{
		g_J1939.CannotClaimAddress = 0;
		// Set up filter to receive messages sent to this address
		J1939_SetInvAddress( g_J1939.MyInvAddress );
        LOG(SS_J1939, SV_INFO, "TxAddrClaim=%u", g_J1939.MyInvAddress);
	}
	else
	{
		// We don't have a proprietary address, so we need to wait.
 		g_J1939.WaitingForAddressClaimContention = 1;
		g_J1939.ContentionWaitTime = 0;
        LOG(SS_J1939, SV_INFO, "TxAddrClaim waiting for contention");
	}
}

// ----------------------------------------------------------------------
// Address Claim message has been received and
// this CA must either defend or give up its address.
// ----------------------------------------------------------------------
static void J1939_RxAddressClaim()
{
    LOG(SS_J1939, SV_INFO, "RxAddressClaim");
    g_J1939.OneMessage.frameType       = CAN_FRAME_EXT;
	g_J1939.OneMessage.jid.ID          = 0;
	g_J1939.OneMessage.jid.Priority    = J1939_CONTROL_PRIORITY;
	g_J1939.OneMessage.jid.PDUFormat   = J1939_PF_ADDRESS_CLAIMED;
	g_J1939.OneMessage.jid.PDUSpecific = J1939_GLOBAL_ADDRESS;
    g_J1939.OneMessage.dataLength      = MAX_CAN_DATA;

	if (g_J1939.OneMessage.jid.SourceAddress != g_J1939.MyInvAddress)
		return;

	if (CompareName( g_J1939.OneMessage.data ) != -1) // Our CA_Name is not less
	{
		if (CA_RecalculateAddress( &g_J1939.MyInvAddress )==0)
        {
    		// Send Cannot Claim Address message
    		CopyName(g_J1939.OneMessage.data);
    		g_J1939.OneMessage.jid.SourceAddress = J1939_NULL_ADDRESS;
    		can_TxEnqueue( &g_J1939.OneMessage );
    
    		// Set up filter to receive messages sent to the global address
    		J1939_SetInvAddress( J1939_GLOBAL_ADDRESS );
    
    		g_J1939.CannotClaimAddress = 1;
    		g_J1939.WaitingForAddressClaimContention = 0;
    		return;
        }
	}

	// Send Address Claim message
	CopyName(g_J1939.OneMessage.data);
	g_J1939.OneMessage.jid.SourceAddress = g_J1939.MyInvAddress;
	can_TxEnqueue( &g_J1939.OneMessage );

	if (((g_J1939.MyInvAddress & 0x80) == 0) ||			// Addresses 0-127
		((g_J1939.MyInvAddress & 0xF8) == 0xF8))		// Addresses 248-253 (254,255 illegal)
	{
		g_J1939.CannotClaimAddress = 0;

        // Set up filter to receive messages sent to this address
		J1939_SetInvAddress( g_J1939.MyInvAddress );
	}
	else
	{
		// We don't have a proprietary address, so we need to wait.
 		g_J1939.WaitingForAddressClaimContention = 1;
		g_J1939.ContentionWaitTime = 0;
	}
}

// ----------------------------------------------------------------------
// generate CA_Name for the device
// build Controller Application (CA) name
void J1939_BuildCtrlAppName(uint32_t serialNumber,uint16_t mfgCode,uint8_t instance)
{
    serialNumber = serialNumber & 0xFFFFF;  // 20 bits
    mfgCode      = mfgCode      & 0xFFF;    // 12 bits
    instance     = instance     & 0x7;      //  3 bits
    g_J1939.CA_Name[0] = (uint8_t)((serialNumber      ));
    g_J1939.CA_Name[1] = (uint8_t)((serialNumber >>  8));
    g_J1939.CA_Name[2] = (uint8_t)((serialNumber >> 16)|(mfgCode&0xF)<<4);
    g_J1939.CA_Name[3] = (uint8_t)(mfgCode>>4);
    g_J1939.CA_Name[4] = (uint8_t)(instance);
    g_J1939.CA_Name[5] = 0;
    g_J1939.CA_Name[6] = 0;
    g_J1939.CA_Name[7] = 0x80;  // dynamic address capable
	LOGX(SS_J1939, SV_INFO, "CA-NAME: ", g_J1939.CA_Name, sizeof(g_J1939.CA_Name));
}

// ----------------------------------------------------------------------
void J1939_Config()
{
    LOG(SS_J1939, SV_INFO, "Config");

    memset(&g_J1939,0,sizeof(g_J1939));

	// Initialize global variables;
    g_J1939.ContentionWaitTime = 1;
    g_J1939.TxPriority = J1939_DEFAULT_PRIORITY;

    // could use serial number of device
    J1939_BuildCtrlAppName(0, RVC_MFG_Sensata, g_can.MyInstance);

    // can be changed dynamically later on
	J1939_SetInvAddress(g_nvm.settings.can.address);
}

// ----------------------------------------------------------------------
void J1939_Start()
{
    LOG(SS_J1939, SV_INFO, "Start");
	J1939_TxAddressClaim();
}

// ----------------------------------------------------------------------
// send address request message
void J1939_SendAddressRequest(uint8_t desiredAddress)
{
    CAN_DATA data[3];
    CAN_LEN  ndata = 3;
    CAN_DGN  dgn   = (J1939_PF_REQUEST_DGN<<8) | desiredAddress;

    LOG(SS_J1939, SV_INFO, "SendAddrReq addr=%y", desiredAddress);
    J1939_SendMessage(dgn, ndata, data);
}

// ----------------------------------------------------------------------
// send the address that we claimed
void J1939_SendAddressClaimed()
{
    LOGX(SS_J1939, SV_INFO, "SendAddressClaimed", g_J1939.CA_Name, 8);
    J1939_SendMessage(J1939_PF_ADDRESS_CLAIMED<<8, 8, g_J1939.CA_Name);
}

// ----------------------------------------------------------------------
// returns 0=ok, 1=too many bytes
int J1939_SendMultiPacketMessage(CAN_DGN dgn, uint16_t dataLen, CAN_DATA* data)
{
    const int PACKET_BYTES = 7; // maximum bytes per multipacket
	CAN_MSG can;
    int i, n, npackets, nfullpackets, nleft;

    if (dataLen < 1) return(0);     // nothing to send
	if (dataLen > 1785) return(1);	// per spec

    LOG(SS_J1939, SV_INFO, "TxMultipacket DGN=%lX n=%u", dgn, dataLen);

    // generate initial packet
    nleft = dataLen;
    npackets = (dataLen + (PACKET_BYTES-1))/PACKET_BYTES;

    J1939_InitMsg(&can);
    J1939_SetDGN(&can, J1939_DGN_INITIAL_MULTI_PACKET);
	can.dataLength = 8;
	can.data[0]    = 0x20;  // per spec
	can.data[1]    = (uint8_t)(dataLen   );  // LSB
	can.data[2]    = (uint8_t)(dataLen>>8);  // MSB
	can.data[3]    = (uint8_t)npackets;
	can.data[5]    = (uint8_t)(dgn      );   // LSB
	can.data[6]    = (uint8_t)(dgn >>  8);
	can.data[7]    = (uint8_t)(dgn >> 16);   // MSB
    // transmit initial packet
   	can_TxEnqueue(&can);

    // subsequent full packets
    nfullpackets = dataLen/PACKET_BYTES;
    J1939_SetDGN(&can, J1939_DGN_SUBSEQUENT_MULTI_PACKET);
    i = 0;
    for (n=1; n<=nfullpackets; n++, i+=PACKET_BYTES)
    {
    	can.data[0] = (char)n;
    	memcpy(&can.data[1], &data[i], PACKET_BYTES);	// copy the data bytes
    	can_TxEnqueue(&can);
    }

    // remaining partial packet (if any)
    if (npackets == nfullpackets) return(0);
   	can.data[0] = (char)npackets;
    n = dataLen - (nfullpackets*PACKET_BYTES);   // remaining bytes
   	memcpy(&can.data[1], &data[i], n);
   	can_TxEnqueue(&can);

    return(0);
}

// ----------------------------------------------------------------------
// set transmit priority; priority used when transmitting CAN message
void J1939_SetPriority(uint8_t priority)
{
    g_J1939.TxPriority = priority & 0x7;    // 3 bits
}

// ----------------------------------------------------------------------
// base J1939 message sender
void J1939_SendMessage(CAN_DGN dgn, CAN_LEN dataLen, CAN_DATA* data)
{
	CAN_MSG can;

	if (dataLen > MAX_CAN_DATA) dataLen = MAX_CAN_DATA;	// dont crash buffer
    J1939_InitMsg(&can);
    J1939_SetDGN (&can, dgn);
	can.dataLength = dataLen;
	memcpy(can.data, data, dataLen); // copy the data bytes
	can_TxEnqueue(&can);
}

// ----------------------------------------------------------------------
// RVC 3.2.4.3  pg 12
void J1939_SendAckNak(uint8_t code, CAN_DGN ackedDGN)
{
	CAN_LEN  ndata = 8;
	CAN_DATA data[8];

	data[0] = code;
	data[1] = 0xFF;
	data[2] = 0x00;
	data[3] = 0x00;
	data[4] = (ackedDGN      ) & 0xFF;
	data[5] = (ackedDGN >>  8) & 0xFF;
	data[6] = (ackedDGN >> 16) & 0xFF;
	data[7] = (ackedDGN >> 24) & 0xFF;
	J1939_SendMessage(J1939_DGN_ACK, ndata, data);
}

// ----------------------------------------------------------------------
// RVC 3.2.4.3  pg 12
void J1939_SendAck(CAN_DGN ackedDGN)
{
	LOG(SS_J1939, SV_INFO,"SendAck(DGN=%lX)", ackedDGN);

	J1939_SendAckNak(J1939_ACK, ackedDGN);
}

// ----------------------------------------------------------------------
// RVC 3.2.4.3  pg 12
void J1939_SendNak(CAN_DGN nackedDGN)
{
	LOG(SS_J1939, SV_INFO,"SendNak(DGN=%lX)", nackedDGN);

	J1939_SendAckNak(J1939_NAK, nackedDGN);
}

// ----------------------------------------------------------------------
// RVC 3.2.4.2  pg 11
// send request for DGN; destAddress can be RVC_DEST_ADDRESS_ALL
void J1939_SendRequestForDGN(uint32_t dgn, uint8_t destAddress)
{
	CAN_LEN  ndata = 3;
	CAN_DATA data[3];

	data[0] = (dgn      ) & 0xFF;
	data[1] = (dgn >>  8) & 0xFF;
	data[2] = (dgn >> 16) & 0xFF;

	LOGX(SS_J1939, SV_INFO, "SendRequestForDGN: ", data, ndata);

	J1939_SendMessage(((J1939_PGN1_REQ_ADDRESS_CLAIM << 8) | destAddress), ndata, data);
}

// ----------------------------------------------------------------------
// J1939_Poll
// 
// Parameters:	The number of milliseconds that have
// 				passed since the last time this routine was
// 				called.  This number can be approximate,
// 				and to meet spec, should be rounded down.
// ----------------------------------------------------------------------
void J1939_Poll(unsigned long ElapsedTime)
{
	g_J1939.ContentionWaitTime += ElapsedTime;

	if (g_J1939.WaitingForAddressClaimContention && (g_J1939.ContentionWaitTime >= 250000)) // 4+ minutes
	{
		g_J1939.CannotClaimAddress = 0;
		g_J1939.WaitingForAddressClaimContention = 0;

		// Set up filter to receive messages sent to this address.
		J1939_SetInvAddress( g_J1939.MyInvAddress );
	}
}

// ----------------------------------------------------------------------
// return our CAN inverter address
uint8_t J1939_InvAddress()
{
    return(g_J1939.MyInvAddress);
}

// ----------------------------------------------------------------------
// return our CAN charger address
uint8_t J1939_ChgrAddress()
{
    return(g_J1939.MyChgrAddress);
}

// ----------------------------------------------------------------------
// check if CAN message is a J1939 house keeping message and process it
// returns: 0=not for us, 1=dispatched
int16_t J1939_Dispatcher(CAN_MSG* msg)
{
	int Loop;
	int rc = 0;	// assume not handled
	uint32_t dgn;

    // must be extended message
    if (msg->frameType != CAN_FRAME_EXT) return(0);

	dgn = can_DGN(msg);
    switch( msg->jid.PDUFormat )
    {
    	case J1939_PF_TP_CONNECT_MGMT:
    		if ((msg->data[0] == J1939_BAM_CONTROL_BYTE) &&
    			(msg->data[5] == J1939_PGN0_COMMANDED_ADDRESS) &&
    			(msg->data[6] == J1939_PGN1_COMMANDED_ADDRESS) &&
    			(msg->data[7] == J1939_PGN2_COMMANDED_ADDRESS))
    		{
                LOG(SS_J1939, SV_INFO, "Rx:J1939_PF_TP_CONNECT_MGMT");
    			g_J1939.GettingCommandedAddress = 1;
    			g_J1939.CommandedAddressSource  = g_J1939.OneMessage.jid.SourceAddress;
            	LOG(SS_J1939, SV_INFO, "Cmd Address=%u", (unsigned)g_J1939.CommandedAddressSource);
                rc = 1; // handled
    		}
    		break;
    
    	case J1939_PF_TP_DATA_TRANSFER:
    		if ((g_J1939.GettingCommandedAddress == 1) &&
    			(g_J1939.CommandedAddressSource == g_J1939.OneMessage.jid.SourceAddress))
    		{  
    			// Commanded Address Handling
    			if ((!g_J1939.GotFirstDataPacket) && (msg->data[0] == 1))
    			{
    				for (Loop=0; Loop<7; Loop++)
    				{
    					g_J1939.CommandedAddressName[Loop] = g_J1939.OneMessage.data[Loop+1];
    				}
    				g_J1939.GotFirstDataPacket = 1;
                    rc = 1; // handled
                	LOG(SS_J1939, SV_INFO, "rx DATA_TRANSFER 1st packet");
    			}
    			else if ((g_J1939.GotFirstDataPacket) && (msg->data[0] == 2))
    			{
    				g_J1939.CommandedAddressName[7] = g_J1939.OneMessage.data[1];
    				J1939_SetInvAddress(g_J1939.OneMessage.data[2]);
    				// Make sure the message is for us; We can change the address.
    				if ((CompareName( g_J1939.CommandedAddressName ) == 0))
    				{
    					J1939_TxAddressClaim();
    				}
    				g_J1939.GotFirstDataPacket = 0;
    				g_J1939.GettingCommandedAddress = 0;
                    rc = 1; // handled
                	LOG(SS_J1939, SV_INFO, "rx DATA_TRANSFER next packet");
    			}
                else
                {
                	LOG(SS_J1939, SV_ERR, "rx DATA_TRANSFER not handled");
                }
    		}
    		break;
    
    	case J1939_PF_REQUEST_DGN:
            if (msg->jid.PDUSpecific == J1939_GLOBAL_ADDRESS)
            {
                // global message
        		if ((msg->data[0]==0x00) && (msg->data[1]==J1939_PF_ADDRESS_CLAIMED) && (msg->data[2]==0x00))
                {
                    if (g_J1939.CannotClaimAddress)
                        J1939_SetInvAddress(J1939_NULL_ADDRESS);
                    J1939_SendAddressClaimed();
                    rc = 1; // handled
                }
            }
            else
            {
                // targeted message
                if (msg->jid.PDUSpecific != g_J1939.MyInvAddress) break;
                // for us
        		if ((msg->data[0]==0xEB) && (msg->data[1]==0xFE) && (msg->data[2]==0x00))
                {
                    rvcan_SendProductID(rvcan_GetProductIdString());
                    rc = 1; // handled
                }
                else
                {
                	LOG(SS_J1939, SV_ERR, "JID=%08lX reqDGN=%02X %02X %02x NOT HANDLED", msg->jid.ID, msg->data[0], msg->data[1], msg->data[2]);
                 // J1939_SendNak(); // TODO
                }
            }
    		break;
    
    	case J1939_PF_ADDRESS_CLAIMED:
            LOG(SS_J1939, SV_INFO, "Rx:J1939_PF_ADDRESS_CLAIMED");
            g_J1939.OneMessage = *msg;   // save a copy
    		J1939_RxAddressClaim();
    		break;
    
    	default:
    		rc = 0;	// not dispatched
    		break;
    } // switch
    return(rc);
}

// ----------------------------------------------------------------------
// test that the bit fields map correctly
// returns: 0=ok, else error
/*
int can_UnitTest()
{
    int errs=0;
    JCAN_ID jid;

    jid.ID = 0; jid.SourceAddress = 0x12;
    if (jid.ID != 0x12) errs++;

    jid.ID = 0; jid.PDUSpecific = 0x34;
    if (jid.ID != 0x3400) errs++;

    jid.ID = 0; jid.PDUFormat = 0x56;
    if (jid.ID != 0x560000) errs++;

    jid.ID = 0; jid.DataPage = 1;
    if (jid.ID != 0x1000000) errs++;

    jid.ID = 0; jid.Reserved = 1;
    if (jid.ID != 0x2000000) errs++;

    jid.ID = 0; jid.Priority = 3;
    if (jid.ID != 0xC000000) errs++;

    jid.ID = 0; jid.Zero = 3;
    if (jid.ID != 0x60000000) errs++;

    if (errs) LOG(SS_J1939, SV_ERROR, "JCAN ID Bitfield Error");

    return(0);
}
*/

// <><><><><><><><><><><><><> J1939.c <><><><><><><><><><><><><><><><><><><><><><>
