// <><><><><><><><><><><><><> spi.c <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//  Copyright(C) 2015 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
//  SPI Remote LCD Communications Driver
//
//-----------------------------------------------------------------------------

// -------
// headers
// -------
#include "options.h"    // must be first include
#include "hw.h"
#include "spi.h"
#include "nvm.h"
#include "sensata_can.h"
#include "rv_can.h"
#include "inverter_cmds.h"
#include "charger_cmds.h"

// ---------
// Constants
// ---------
#define SPI_RX_BUF_LEN     (2*SPI_MAX_PKT_LEN)  // sizeof of receive  buffer (120 bytes) 
#define SPI_TX_BUF_LEN     (2*SPI_MAX_PKT_LEN)  // sizeof of transmit buffer (120 bytes)
#define BCR_DEBOUNCE_CNTS  (10)                 // consecutive counts need to detect legacy BCR command

// -----------
// Structures
// -----------
// SPI Driver Data
#pragma pack(1)
typedef struct SPI_DATA_t
{
    uint8_t  isRmtEnhanced;          // 0=legacy, 1=remote display uses enhanced menuing protocol
    uint8_t  handshakeDone;          // 0=no, 1=handshake with LCD complete
    uint8_t  handshakeCnt;           // count legacy BCR

    // circular receive buffer
    uint8_t  RxBuf[SPI_RX_BUF_LEN];  // circular receive buffer
    uint16_t RxPutIn;                // index into RxBuf[] to put next byte received from remote
    uint16_t RxTakeOut;              // index into RxBuf[] to take out next byte via getter

    // circular transmit buffer      
    uint8_t  TxBuf[SPI_TX_BUF_LEN];  // circular transmit buffer
    uint16_t TxPutIn;                // index into TxBuf[] to put next byte via setter
    uint16_t TxTakeOut;              // index into TxBuf[] to take out next byte for transmission

    // high level command protocol
    uint16_t RxCmdBytes;             // number of bytes in RxCmd[]
    uint8_t  RxCmd[SPI_MAX_PKT_LEN]; // linear command receive buffer
    uint16_t RxCmdBytesNeeded;       // total bytes need in RxCmd[] to fill packet

    // Branch Circuit Rating debouncing
    uint8_t  LastBcrByte;            // last BCR byte receive from remote display
    uint8_t  BcrCntr;                // BCR debounce counter

    // sync word state
    uint8_t  SyncWordState;          // 0=idle, 1=rx sync, 2=rx data
    uint8_t  EmptyTxCnts;            // number of consequtive non=transmit counts (milliseconds)

} SPI_DATA;
#pragma pack() // restore packing


// -------
// Macros
// -------
#define IS_RXBUF_EMPTY()  (g_spi.RxPutIn == g_spi.RxTakeOut?1:0)
#define IS_TXBUF_EMPTY()  (g_spi.TxPutIn == g_spi.TxTakeOut?1:0)

// ------------
// Prototyping
// ------------
static void spi_processRxPktCmd(SPI_HDR* hdr, uint8_t* data);
static void spi_processBcrByte(uint8_t bcrByte);

// ----
// Data
// ----
SPI_DATA g_spi;                 // SPI driver data

//-----------------------------------------------------------------------------
//  spi_Config()
//-----------------------------------------------------------------------------

void spi_Config(void)
{
    // initialize data
    memset(&g_spi,0,sizeof(g_spi));

  #ifndef WIN32

    SPI2CON1bits.MSTEN = 1; 	//  SPI module in MASTER mode
    SPI2CON1bits.MODE16 = 0;    //  Assure that we are in 8-bit mode

	//	SPIFSD: SPIx Frame Sync Pulse Direction Control bit
	//		1 = Frame sync pulse input (slave)
	//		0 = Frame sync pulse output (master)
    SPI2CON2bits.SPIFSD = 0;
	
    //  Want SPI clock (SCK1) = 156.25Kbps, with FCY = 29491200
    //  Equation 20-1:  FSCK = FCY/(Pri Presc * Sec Presc)
    //      (Pri Presc * Sec Presc) = FCY/FSCK = 188.74368
    //  if Pri Presc = 16:1 then Sec Presc = 12:1
    //
    //  PPRE (Primary Prescale bits) and SPRE (Secondary Prescale bits) values 
	//	were determined experimentally:
    //   PPRE   00 = 64:1       SPRE    000 = 8:1   100 = 4:1
    //          01 = 16:1               001 = 7:1   101 = 3:1
    //          10 = 4:1                010 = 6:1   110 = 2:1
    //          11 = 1:1                011 = 5:1   111 = 1:1

    SPI2CON1bits.PPRE = 0b00;	//  64:1
//  SPI2CON1bits.SPRE = 0b101;  //   3:1		orig
//  SPI2CON1bits.SPRE = 0b011;  //   5:1		3.8 uSec
    SPI2CON1bits.SPRE = 0b000;  //   8:1		6.5 uSec

    // init interrupts
    SPI2BUF = 0;                // clear

#endif // WIN32
}

//-----------------------------------------------------------------------------
void spi_Start(void)
{
    volatile uint16_t rx_data;

    //  The SPIxBUF must be read before new data is completely shifted in
    rx_data = SPI2BUF;          // Read byte from buffer to make it empty/ready
    SPI2STATbits.SPIROV = 0;    // Clear the Buffer Overflow flag
    SPI2STATbits.SPIEN  = 1;    // Enable the SPI module
}

//-----------------------------------------------------------------------------
void spi_Stop(void)
{
    SPI2STATbits.SPIROV = 0;    // Clear the Buffer Overflow flag
    SPI2STATbits.SPIEN  = 0;    // Disable the SPI module
}

//-----------------------------------------------------------------------------
// return number of bytes available in transmit buffer
int16_t spi_TxFreeSpace()
{
    if (IS_TXBUF_EMPTY()) return(SPI_TX_BUF_LEN-1);   // one less than full buffer
    if (g_spi.TxPutIn > g_spi.TxTakeOut)
        return(SPI_TX_BUF_LEN - g_spi.TxPutIn + g_spi.TxTakeOut - 1);
    return(g_spi.TxTakeOut - g_spi.TxPutIn - 1);
}

//-----------------------------------------------------------------------------
// put data to spi transmitter
// returns: 0=ok, 1=no room
int spi_PutBuf(uint8_t* data, int16_t ndata)
{
    uint16_t n = spi_TxFreeSpace();
    if (n < ndata)
    {
     // LOG(SS_UI, SV_ERR, "spi_PutBuf: No Room: Avail=%u < Need=%u ", n, ndata);
        return(1); // not enough room
    }

    // entire data will fit in transmit buffer
    n = SPI_TX_BUF_LEN - g_spi.TxPutIn; // calc space available to end of buffer
    if (g_spi.TxTakeOut < 1) n--;   // special case; dont let buffer totally fill
    if (ndata <= n)
    {
        // single chunk at end of buffer
        memcpy(&g_spi.TxBuf[g_spi.TxPutIn], data, ndata);
        g_spi.TxPutIn += ndata;
        if (g_spi.TxPutIn >= SPI_TX_BUF_LEN) g_spi.TxPutIn = 0; // wrap
    }
    else
    {
        // two chunks needed, due to buffer wrap
        memcpy(&g_spi.TxBuf[g_spi.TxPutIn], data, n); // end chunk of buffer
        g_spi.TxPutIn = ndata - n;
        memcpy(&g_spi.TxBuf[0], &data[n], g_spi.TxPutIn); // beginning chunk of buffer
    }
    return(0);
}

//-----------------------------------------------------------------------------
// put Lcd message to spi transmitter buffer using legacy protocol
// returns: 0=ok, 1=no room
int spi_PutLcdMsg(uint8_t* data, int16_t ndata, uint8_t lcd_red, uint8_t lcd_exit)
{
    uint8_t pkt[SPI_LEGACY_PKT_BYTES];

    // build the packet
    // load the ASCII message
    if (ndata>SPI_LEGACY_MSG_LEN) ndata = SPI_LEGACY_MSG_LEN;
    memcpy(pkt,data,ndata);
    if (ndata < SPI_LEGACY_MSG_LEN)
        memset((uint8_t*)&pkt[ndata],' ',SPI_LEGACY_MSG_LEN-ndata);

    // turn on handshake bit during detection mode
    if (!g_spi.handshakeDone || IsRmtEnhanced())  // enhance remote always needs
    {
        pkt[LCD_HANDSHAKE_BIT_INDEX] |= 0x80;  // set top bit
    }

    // set the LCD control bits for enhanced display
    if (IsRmtEnhanced())
    {
        uint8_t inv_mode = IsInvActive();
        uint8_t chg_mode = IsAcLineQualified();
        // encode LCD control bits into message; stripped out by menuing LCD
        if (inv_mode) pkt[LCD_INV_BIT_INDEX ] |= 0x80;  // set top bit
        if (chg_mode) pkt[LCD_CHG_BIT_INDEX ] |= 0x80;  // set top bit
        if (lcd_exit) pkt[LCD_EXIT_BIT_INDEX] |= 0x80;  // set top bit
        if (lcd_red ) pkt[LCD_RED_BIT_INDEX ] |= 0x80;  // set top bit
        // LOG(SS_UI, SV_INFO, "LCD_Red=%u %02X  LCD_exit=%u %02X  LCD_chg=%u %02X", 
        //   (unsigned)lcd_red,  (unsigned)pkt[LCD_RED_BIT_INDEX ],
        //   (unsigned)lcd_exit, (unsigned)pkt[LCD_EXIT_BIT_INDEX],
        //   (unsigned)chg_mode, (unsigned)pkt[LCD_CHG_BIT_INDEX ]);
    }

    // load checksum
    pkt[SPI_LEGACY_MSG_LEN] = spi_CalcChecksum(pkt,SPI_LEGACY_MSG_LEN);
    // load sync byte
    pkt[SPI_LEGACY_MSG_LEN+1] = SPI_LEGACY_SYNC_BYTE;

    return(spi_PutBuf(pkt, SPI_LEGACY_PKT_BYTES));
}

//-----------------------------------------------------------------------------
// load packet header with info for transporting
void spi_FillPktHdr(SPI_HDR* hdr, uint8_t cmdNum, uint8_t* parms, uint8_t nparms, uint8_t ndata)
{
    // validate
    if (nparms > SPI_NPARMS) nparms = SPI_NPARMS; // dont crash buffer

    // zero
    memset((uint8_t*)hdr,0,sizeof(SPI_HDR));

    // fill data fields
    hdr->sync1  = SPI_SYNC1;
    hdr->cmdNum = cmdNum;
    hdr->ndata  = ndata;
    if (nparms>0) memcpy(hdr->parms, parms, nparms); // copy parms if any
    hdr->sync2  = SPI_SYNC2;
    hdr->hdr_cs = spi_CalcChecksum((uint8_t*)hdr, SPI_HDR_CS_OFFSET);
}


//-----------------------------------------------------------------------------
// send a command packet to the remote
// returns: 0=ok, 1=no room
int spi_PutPktCmd(uint8_t cmdNum, uint8_t* parms, int16_t nparms, uint8_t* data, uint8_t ndata)
{
    SPI_HDR hdr;
    int16_t    needed, avail;

    // check for enough buffer space
    avail  = spi_TxFreeSpace();
    needed = sizeof(SPI_HDR) + ndata + 1;  // header + data + cs
    if (avail < needed)
    {
     // LOG(SS_UI, SV_ERR, "spi_PutPktCmd: No Room: Avail=%u < Need=%u ", avail, needed);
        return(1);
    }

    // build the packet header
    spi_FillPktHdr(&hdr, cmdNum, parms, nparms, ndata);

    // send the header
    if (spi_PutBuf((uint8_t*)&hdr, sizeof(hdr))) return(1);

    // send data bytes if requested
    if (ndata)
    {
        // calculate data checksum
        uint8_t cs = spi_CalcChecksum(data, ndata);
        // send data
        if (spi_PutBuf(data, ndata)) return(1);
        // send data checksum
        if (spi_PutBuf(&cs,1)) return(1);
    }

    avail = sizeof(hdr) + ndata;
 // LOGX(SS_UI, SV_INFO, "SPI TX PKT ", (uint8_t*)&hdr, avail);
    return(0);
}

//-----------------------------------------------------------------------------
// calculate checksum by summing bytes
uint8_t spi_CalcChecksum(uint8_t* buf, uint16_t nbytes)
{
    uint8_t cs = 0;
    uint16_t i;
    for (i=0; i<nbytes; i++)
    {
        cs += buf[i];
    } // for
    return(cs);
}

//-----------------------------------------------------------------------------
// returns 0=no, 1=remote display uses enhance menuing protocol
uint8_t IsRmtEnhanced()
{
    return(g_spi.isRmtEnhanced);
}

//-----------------------------------------------------------------------------
// start LCD handshake operation
void spi_ClearHandshake()
{
    g_spi.handshakeCnt  = 0;
    g_spi.handshakeDone = 0;
    LOG(SS_UI, SV_INFO, "Start LCD handshake");
}

//-----------------------------------------------------------------------------
//  spi_Driver()
//-----------------------------------------------------------------------------
//  spi_Driver is called repeatedly to transmit data.
//
//	2015-12-17 - The SPI Driver task is scheduled each 1-millisecond. Attempts
//	were made to use interrupts and DMA, for faster transfer times. However, 
//	it was determined that increasing the data rates overwhelmed the display.
//	
//	It was also noticed that at 1-millisecond per character, there are problems 
//	with display stability.  Therefore, after a delay was implemented after 
//	each message has been transmitted, to allow the display time to process 
//	the data.
//
//  spi_Driver uses double-buffering, so that the data will not change while 
//  transmitting.  Data is copied from the global buffer '_LcdBuf' to the local
//  private buffer 'buf' before transmission starts.
//  Application code should call spi_PutBuf to load the global buffer.
//-----------------------------------------------------------------------------

#define EMPTY_TX_THRESHOLD   (10)  // transmit sync byte if idle this long

void TASK_spi_Driver(void)
{
    uint8_t  byte,cs,validPacket=0, gotbyte;
    uint16_t index;

    // ----------------------
    // drive the transmitter
    // ----------------------

    if (SPI2STATbits.SPIROV)
    {
        // byte overflow; data is trash
        SPI2STATbits.SPIROV = 0;
        byte = SPI2BUF;
     // LOG(SS_UI, SV_ERR, "SPI RxOverflow Byte=%02X", byte);
    }
    else if (SPI2STATbits.SPIRBF)
    {
        gotbyte=0;
        byte = (uint8_t)SPI2BUF;
     // LOG(SS_UI, SV_INFO, "RxRawByte=%02X", byte);
        switch(g_spi.SyncWordState)
        {
        default:
        case 0: // first byte of word
            switch (byte)
            {
            default:
                if (IS_POSSIBLE_LEGACY_BCR(byte)) gotbyte = 1;
                break;
            case SPI_TXWORD_SYNC_BYTE:  // transmit word; sync byte received; wait for data byte
                g_spi.SyncWordState = 1;
                break;
            case SPI_RMT2_IDLE_BYTE:    // ignore idle bytes
                break;
            } // switch byte
            break;
        case 1: // second byte of word transfer
            gotbyte = 1;
            g_spi.SyncWordState = 0;
            break;
        } // switch sync word state
        if (gotbyte)
        {
            g_spi.RxBuf[g_spi.RxPutIn] = byte;
            g_spi.RxPutIn++;  // advance pointer
            if (g_spi.RxPutIn >= SPI_RX_BUF_LEN) g_spi.RxPutIn = 0; // wrap
         // LOG(SS_UI, SV_INFO, "RxCookedByte=%02X", (unsigned)byte);
        }
    }

    // check if SPI transmitter hardware is ready
    if (!SPI2STATbits.SPITBF)
    {
        // SPI transmitter hardware is ready
        if (IS_TXBUF_EMPTY())
        {
            // nothing to send; send periodically to keep receiver running
            g_spi.EmptyTxCnts++;
            if (g_spi.EmptyTxCnts > EMPTY_TX_THRESHOLD)
            {
                // send legacy sync byte to keep receiver running
                g_spi.EmptyTxCnts = 0;
                SPI2BUF = SPI_LEGACY_SYNC_BYTE;
            }
        }
        else
        {
            // data is in circular transmit buffer; send one byte
            SPI2BUF = g_spi.TxBuf[g_spi.TxTakeOut];
            g_spi.TxTakeOut++;
            if (g_spi.TxTakeOut >= SPI_TX_BUF_LEN) g_spi.TxTakeOut = 0; // wrap
            g_spi.EmptyTxCnts = 0;
        }
    }

    // -------------------
    // drive the receiver
    // -------------------

    // check for any bytes in receiver
    if (IS_RXBUF_EMPTY()) return; // no data

    // retrieve a byte from the receiver
    byte = g_spi.RxBuf[g_spi.RxTakeOut];
    g_spi.RxTakeOut++;
    if (g_spi.RxTakeOut >= SPI_RX_BUF_LEN) g_spi.RxTakeOut = 0; // wrap
    g_spi.RxCmd[g_spi.RxCmdBytes] = byte; // save in command buffer
    g_spi.RxCmdBytes++;

    // evaluate protocol
    switch (g_spi.RxCmdBytes)
    {
    case SPI_SYNC1_OFFSET+1: // SYNC1 byte
        g_spi.RxCmdBytesNeeded = SPI_HDR_BYTES;
        if (byte != SPI_SYNC1)
        {
            g_spi.RxCmdBytes = 0; // reset counter
            spi_processBcrByte(byte);
            break;
        }
        break;

    case SPI_SYNC2_OFFSET+1: // SYNC2 byte
        if (byte != SPI_SYNC2)
        {
            LOGX(SS_UI, SV_ERR, "SPI SYNC2 FAILED ", g_spi.RxCmd, SPI_HDR_BYTES);
            g_spi.RxCmdBytes = 0; // reset counter
            break;
        }
        cs = spi_CalcChecksum(g_spi.RxCmd, SPI_HDR_CS_OFFSET);
        if (cs != g_spi.RxCmd[SPI_HDR_CS_OFFSET])
        {
            LOG (SS_UI, SV_ERR, "SPI HDR CS FAILED calc=%02X rx=%02X",(unsigned)cs, (unsigned)g_spi.RxCmd[SPI_HDR_CS_OFFSET]);
            LOGX(SS_UI, SV_ERR, "SPI HDR CS FAILED ", g_spi.RxCmd, SPI_HDR_BYTES);
            g_spi.RxCmdBytes = 0; // reset counter
            break;
        }
        if (g_spi.RxCmd[SPI_NDATA_OFFSET] == 0)
        {
            validPacket = 1; // process it
            break;
        }
        g_spi.RxCmdBytesNeeded = SPI_HDR_BYTES + g_spi.RxCmd[SPI_NDATA_OFFSET] + 1;
        break;

    default:
        if (g_spi.RxCmdBytes < 3 && byte == SPI_SYNC1) // ignore repeating sync1 bytes
        {
            g_spi.RxCmdBytes = 0; // reset counter
            break;
        }
        if (g_spi.RxCmdBytes < SPI_HDR_BYTES) break;  // need to have at least a full header
        if (g_spi.RxCmdBytes < g_spi.RxCmdBytesNeeded) break; // data payload
        // payload packet
        cs = spi_CalcChecksum((uint8_t*)&g_spi.RxBuf[SPI_HDR_BYTES], g_spi.RxCmd[SPI_NDATA_OFFSET]);
        index = SPI_HDR_BYTES + g_spi.RxCmd[SPI_NDATA_OFFSET];  // offset to payload checksum
        if (cs != g_spi.RxBuf[index])
        {
            // bad payload checksum
            LOG(SS_UI, SV_INFO, "SPI PAYLOAD CS FAILED calc=%02X rx=%02X", cs, g_spi.RxBuf[index]);
            g_spi.RxCmdBytes = 0; // reset counter
            break;
        }
        validPacket = 1; // process it
        break;

    } // switch

    // ---------------------------
    // process packet if received
    // ---------------------------
    if (validPacket)
    {
        // set enhanced LCD flag
        if (!g_spi.isRmtEnhanced)
        {
            g_spi.isRmtEnhanced = 1;
            g_spi.handshakeDone = 1;
            // log it on first time
            LOG(SS_UI, SV_INFO, "Enhance Remote LCD detected");
        }

        // process it
        spi_processRxPktCmd( (SPI_HDR*)&g_spi.RxCmd[0], &g_spi.RxCmd[sizeof(SPI_HDR)] );

        // allow new command
        g_spi.RxCmdBytes = 0;
        g_spi.RxCmdBytesNeeded = SPI_HDR_BYTES;
    }
}

//-----------------------------------------------------------------------------
// process Branch Circuit Rating byte for legacy protocol
static void spi_processBcrByte(uint8_t bcrByte)
{
 // LOG(SS_UI, SV_INFO, "BCR raw byte=%02X", bcrByte);
    if ((bcrByte & 0xE0) != 0) return; // top 3 bits must be zero for legacy remote

    // debounce the BCR status byte
    if (bcrByte != g_spi.LastBcrByte)
    {
        g_spi.LastBcrByte = bcrByte; // save as last
        g_spi.BcrCntr = 0; // reset debounce counter
        return;
    }

    // same as last time
    g_spi.BcrCntr++;
    if (g_spi.BcrCntr < BCR_DEBOUNCE_CNTS) return;  // not enough counts yet
    if (g_spi.BcrCntr > BCR_DEBOUNCE_CNTS)
    {
        g_spi.BcrCntr = BCR_DEBOUNCE_CNTS+1; // keep it above threshold; dont retrigger
    }
    else
    {
        // new BCR value received; update Branch Circuit Rating of system
        int16_t ix = (int16_t)((bcrByte&0x0E)>>1); // 0=n/a 1=0, 2=5, 3=10, 4=15... 
        if (ix>0)
        {
          #ifdef OPTION_HAS_CHARGER
         // LOG(SS_UI, SV_INFO, "BCR byte=%02X", bcrByte);
        	chgr_SetBranchCircuitAmps(5*(ix-1));
          #endif
        }

        // menuing LCD does not send back BCR byte continuously
        if (!g_spi.handshakeDone)
        {
            g_spi.handshakeCnt++;
            if (g_spi.handshakeCnt > 2) // about 1 seconds
            {
                g_spi.isRmtEnhanced = 0;
                g_spi.handshakeDone = 1;
                LOG(SS_UI, SV_INFO, "Legacy Remote LCD detected");
            }
        }
    }
}

//-----------------------------------------------------------------------------
// process received spi packetized command
static void spi_processRxPktCmd(SPI_HDR* hdr, uint8_t* indata)
{
    CAN_DATA data[SENFLD_MAX_BYTES];
    CAN_LEN  ndata;
    int16_t  rc;

 // LOGX(SS_UI, SV_INFO, "SPI RX PKT ", (uint8_t*)hdr, SPI_HDR_BYTES);

    switch (hdr->cmdNum)
    {
    case SPI_CMD_GET_FIELD_REQ:
        // get requested field info
        rc = sen_GetField(hdr->parms, data, &ndata);
        if (rc != 0) break; 

        // first 8 bytes fit in parms[] array; remainder is payload tacked on the end as data
        if (ndata <= SPI_NPARMS) ndata = 0;
        else                     ndata = ndata - SPI_NPARMS;

        // send the packet
        rc = spi_PutPktCmd(SPI_CMD_GET_FIELD_RSP, data, SPI_NPARMS, &data[SPI_NPARMS], ndata);
        break;

    case SPI_CMD_SET_FIELD_REQ:
        rc = sen_SetField(hdr->parms);
        //echo back response
        rc = spi_PutPktCmd(SPI_CMD_SET_FIELD_RSP, indata, SPI_NPARMS, &data[SPI_NPARMS], 0);
        break;

    case SPI_CMD_REBOOT_DEVICE:
        CPU_Reboot();
        break;

    case SPI_CMD_FACTORY_DEFAULTS:
        nvm_LoadRomSettings();
        nvm_SetDirty(); // save them to nvm
        nvm_ApplySettings();
        LOG(SS_SYS, SV_INFO, "Factory Defaults");
        break;

    case SPI_CMD_INV_ENABLE_DISABLE:
        dev_SetInverterEnabled(hdr->parms[0]);
        if (hdr->parms[0]==0) Device.status.inv_disabled_source = 1;  // LCD
        LOG(SS_SYS, SV_INFO, "SPI %s Inverter", hdr->parms[0]?"Enable":"Disable");
        break;

    } // switch
}

// <><><><><><><><><><><><><> spi.c <><><><><><><><><><><><><><><><><><><><><><>
