// <><><><><><><><><><><><><> dsPIC33_CAN.c <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//  Copyright(C) 2015 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
//  CAN bus layer implementation for dsPIC33 microprocessor
//  
//-----------------------------------------------------------------------------

// -------
// headers
// -------
#include "options.h"    // must be first include
#include "dsPIC33_CAN.h"
#include "rv_can.h"
#include "tasker.h"
#include "nvm.h"

// -------------------
// access global data
// -------------------
extern TASK_ID_t _task_can;
                                                           
// ------------
// CAN Bus data
// ------------
CAN_STATE g_can = { 0 };

// -----------
// Prototyping
// -----------
void J1939_Config(void); // Dont include J1939.h to avoid recursion
void J1939_Start(void);  // Dont include J1939.h to avoid recursion

// ----------------
// CAN DMA buffers
// ----------------

#define  NUM_OF_ECAN_BUFFERS   (32)
#define  ECAN_MSG_BUF_LENGTH   (4)
typedef  uint16_t   ECANMSGBUF[ECAN_MSG_BUF_LENGTH][CAN_DMA_WORDS];
volatile ECANMSGBUF g_canTxDmaBuf __attribute__((space(dma),aligned(ECAN_MSG_BUF_LENGTH * sizeof(DMA_WORD))));
volatile ECANMSGBUF g_canRxDmaBuf __attribute__((space(dma),aligned(ECAN_MSG_BUF_LENGTH * sizeof(DMA_WORD))));

// increment queue pointer and handle wrap around
#define INC_QUEUE_PTR(PTR, QUEUE_SIZE)  { PTR++; if (PTR >= QUEUE_SIZE) PTR = 0; }

// ------------------------------------------------------------------------------------
// CAN Bus Interrupt   ~10 usec
void __attribute__((interrupt, no_auto_psv))_C1Interrupt(void)
{
    // check transmit interrupts
    if (C1INTFbits.TBIF)
    {
        // transmit interrupt
        C1INTFbits.TBIF = 0;
    }

    // check receive interrupts
    if (C1INTFbits.RBIF)
    {
        // receive interrupt
        // check to see if buffer 1 is full
        if(C1RXFUL1bits.RXFUL1)
        {
            // transfer data from dma buffer 1 to circular buffer
            // cant use memcpy here
            g_can.RxQueue[g_can.RxHead].word[0] = g_canRxDmaBuf[0][0];
            g_can.RxQueue[g_can.RxHead].word[1] = g_canRxDmaBuf[0][1];
            g_can.RxQueue[g_can.RxHead].word[2] = g_canRxDmaBuf[0][2];
            g_can.RxQueue[g_can.RxHead].word[3] = g_canRxDmaBuf[0][3];
            g_can.RxQueue[g_can.RxHead].word[4] = g_canRxDmaBuf[0][4];
            g_can.RxQueue[g_can.RxHead].word[5] = g_canRxDmaBuf[0][5];
            g_can.RxQueue[g_can.RxHead].word[6] = g_canRxDmaBuf[0][6];
            g_can.RxQueue[g_can.RxHead].word[7] = g_canRxDmaBuf[0][7];

            INC_QUEUE_PTR(g_can.RxHead, CAN_RX_QUEUE_SIZE);
            C1RXFUL1bits.RXFUL1=0;      // no longer full
            
            task_MarkAsReady(_task_can);
        }
        C1INTFbits.RBIF = 0;
    }
    IFS2bits.C1IF = 0;  // end of interrut
}

//------------------------------------------------------------------------------
//    DMA interrupt handlers
//------------------------------------------------------------------------------

// never called
void __attribute__((interrupt, no_auto_psv)) _DMA1Interrupt(void)
{
    IFS0bits.DMA1IF = 0; // Clear the DMA1 Interrupt Flag;
}

// never called
void __attribute__((interrupt, no_auto_psv)) _DMA2Interrupt(void)
{
    IFS1bits.DMA2IF = 0; // Clear the DMA2 Interrupt Flag;
}

// ------------------------------------------------------------------------------------
// can_SetMode
// 
// This routine sets the ECAN module to a specific mode.  It requests the
// mode, and then waits until the mode is actually set.
// 
// Parameters:  unsigned char   ECAN module mode.  Must be either
//                              ECAN_CONFIG_MODE or ECAN_NORMAL_MODE
// Return:      None
// ------------------------------------------------------------------------------------
void can_SetMode( unsigned char Mode )
{
    C1CTRL1bits.REQOP = Mode;
    while (C1CTRL1bits.OPMODE != Mode);
}

// ------------------------------------------------------------------------------------
//            8 Mhz         7.37 Mhz
// BRP	Mult  K-Baud        K-Baud
//  19   40   100           62.5       
//  18   38   105.2631579   65.78947368
//  17   36   111.1111111   69.44444444
//  16   34   117.6470588   73.52941176
//  15   32   125           78.125     
//  14   30   133.3333333   83.33333333
//  13   28   142.8571429   89.28571429
//  12   26   153.8461538   96.15384615
//  11   24   166.6666667   104.1666667
//  10   22   181.8181818   113.6363636
//   9   20   200           125        
//   8   18   222.2222222   138.8888889
//   7   16   250           156.25     
//   6   14   285.7142857   178.5714286
//   5   12   333.3333333   208.3333333
//   4   10   400           250        
//   3    8   500           312.5      
//   2    6   666.6666667   416.6666667
//   1    4   1000          625        
//   0    2   2000          1250       
// ------------------------------------------------------------------------------------

// set CAN baud rate
void can_SetBaudRate(CAN_BAUD baudRate)
{
    uint8_t brpValue = 7; // 8 Mhz crystal
    switch(baudRate)
    {
    default:
    case CAN_BAUD_250K:   brpValue = 7;  LOG(SS_CAN, SV_INFO, "Set BaudRate 250kb" );break;
    case CAN_BAUD_500K:   brpValue = 3;  LOG(SS_CAN, SV_INFO, "Set BaudRate 500kb" );break;
    case CAN_BAUD_1000K:  brpValue = 1;  LOG(SS_CAN, SV_INFO, "Set BaudRate 1000kb");break;
    } // switch
    can_SetMode(ECAN_MODE_CONFIGURE);
    C1CFG1bits.BRP = brpValue;
    can_SetMode(ECAN_MODE_NORMAL);   // back to Normal mode.
}

// -------------------- ----------------------------------------------------------------
// returns 0=ok, 1=bad value; ignored
int16_t can_SetInstance(CAN_INST instance)
{
    if (instance < MIN_CAN_INSTANCE || instance > MAX_CAN_INSTANCE) return(1);
    if (instance == g_can.MyInstance) return(0);  // already set
    g_can.instance     = instance;
    g_can.MyInstance   = instance;
    g_nvm.settings.can.instance = instance;
    nvm_SetDirty();
    LOG(SS_CAN, SV_INFO, "Set CAN Instance=%u", (int)instance);
    return(0);
}

// ------------------------------------------------------------------------------------
// configure CAN bus hardware
void can_Config(void)
{
    g_can.MyID       = g_can.address;
    g_can.MyInstance = g_can.instance;
    can_SetBaudRate(g_can.baud);
    rvcan_BuildProductId();
 
  #ifndef WIN32

    // disable can interrupts to allow resetting
    C1INTEbits.RBIE = 0;
    IEC2bits.C1IE = 0;

    // drive the CAN STANDBY driver pin low
    _TRISF1 = 0;
    _TRISF0 = 1;

    // need to put in configure mode to set registers
    can_SetMode(ECAN_MODE_CONFIGURE);

    // 8 Mhz crystal
    C1CTRL1bits.WIN     = 0b0;
    C1CTRL1bits.CANCKS  = 0b1;

    // Set up the CAN module for 250kbps speed with 10 Tq per bit.
    C1CFG1bits.SJW      = 0b01;
    C1CFG2bits.SEG1PH   = 0b010;
    C1CFG2bits.SEG2PH   = 0b010;
    C1CFG2bits.SEG2PHTS = 0b1;
    C1CFG2bits.PRSEG    = 0b010;
    C1CFG2bits.SAM      = 0b1;
    C1CFG2bits.WAKFIL   = 0b0;
    C1FCTRLbits.DMABS   = 0b110;
    C1FCTRLbits.FSA     = 0b11111;

    // set up the CAN DMA1 for the Transmit Buffer
    DMA1CONbits.SIZE  = 0b0;
    DMA1CONbits.DIR   = 0b1;
    DMA1CONbits.AMODE = 0b10;
    DMA1CONbits.MODE  = 0b00;
    DMA1REQ = 70;
    DMA1CNT = 7;
    DMA1PAD = (volatile uint16_t)&C1TXD;
    DMA1STA = __builtin_dmaoffset(&g_canTxDmaBuf);

    C1TR01CONbits.TXEN0  = 0b1;         // Buffer 0 is the Transmit Buffer
    C1TR01CONbits.TX0PRI = 0b11;        // transmit buffer priority

    DMA1CONbits.CHEN = 0b1;

    //  Initialise the DMA channel 2 for ECAN Rx using peripheral indirect 
    //  addressing mode
    //  normal operation, word operation and select as Rx to peripheral 
    DMA2CON = 0x0020;
    // setup the address of the peripheral ECAN1 (C1RXD) 
    DMA2PAD = (volatile uint16_t)&C1RXD;
    // Set the data block transfer size of 8 
    DMA2CNT = 7;
    // automatic DMA Rx initiation by DMA request 
    DMA2REQ = 0x0022;
    DMA2STA = __builtin_dmaoffset(&g_canRxDmaBuf);
    // enable the channel 
    DMA2CONbits.CHEN=1;

    // 4 CAN Messages to be buffered in DMA RAM 
    C1FCTRLbits.DMABS=0;

    // Filter configuration 
    // enable window to access the filter configuration registers 
    C1CTRL1bits.WIN = 1;
    // select acceptance mask 0 filter 0 buffer 1 
    C1FMSKSEL1bits.F0MSK = 0;

    // id for filter
    C1RXF0SID           = (g_can.MyID<<5);          // set id bits 0-10
    C1RXF0EID           = (g_can.MyID>>11);         // set id bits 11-26
    C1RXF0SIDbits.EID16 = (g_can.MyID>>27)&1?1:0;   // set id bit  27
    C1RXF0SIDbits.EID17 = (g_can.MyID>>28)&1?1:0;   // set id bit  28

    // match id bits
    C1RXM0SID = 0; // xFFE0;    // match bottom 11 bits, MIDE=allow both
    C1RXM0EID = 0;

    // acceptance filter to use buffer 1 for incoming messages 
    C1BUFPNT1bits.F0BP = 1;
    // enable filter 0 
    C1FEN1bits.FLTEN0 = 1;
    // clear window bit to access ECAN control registers 
    C1CTRL1bits.WIN = 0;

    // ECAN1, Buffer 1 is a Receive Buffer 
    C1TR01CONbits.TXEN1 = 0;

    // clear the buffer and overflow flags 
    C1RXFUL1=C1RXFUL2=C1RXOVF1=C1RXOVF2=0x0000;

  #endif // WIN32

    // back to Normal mode.
    can_SetMode(ECAN_MODE_NORMAL);

    LOG(SS_CAN, SV_INFO, "Config DMA1-Tx, DMA2-Rx");

    J1939_Config();   // config J1939 layer
    
    _task_can = task_AddToQueue(TASK_can_Driver, "can  "); 
}

// ------------------------------------------------------------------------------------
// set the device's CAN ID
void can_SetID(CAN_ID id)
{
    g_can.MyID = id;

    // id for filter
    can_SetMode(ECAN_MODE_CONFIGURE);
    C1RXF0SID           = (g_can.MyID<<5);          // set id bits 0-10
    C1RXF0EID           = (g_can.MyID>>11);         // set id bits 11-26
    C1RXF0SIDbits.EID16 = (g_can.MyID>>27)&1?1:0;   // set id bit  27
    C1RXF0SIDbits.EID17 = (g_can.MyID>>28)&1?1:0;   // set id bit  28
    can_SetMode(ECAN_MODE_NORMAL);

    LOG(SS_CAN, SV_INFO, "SetID=%lu", id);
}

// ------------------------------------------------------------------------------------
// start CAN bus interrupts
void can_Start(void)
{
    // CAN RX interrupt enable - 'double arm' since 2-level nested interrupt
    C1INTEbits.RBIE = 1;
    IEC2bits.C1IE = 1;

    LOG(SS_CAN, SV_INFO, "Start");

    J1939_Start();   // start J1939 layer
}

// ------------------------------------------------------------------------------------
// stop CAN bus interrupts
void can_Stop(void)
{
    C1INTEbits.RBIE = 0;
    IEC2bits.C1IE = 0;

//  LOG("Stop CAN\r\n");
}

// ------------------------------------------------------------------------------------
// unpack DMA data to CAN message
static void can_UnpackDma(CAN_DMA *dma, CAN_MSG* can)
{
    uint16_t word0, word1, word2, word3;
    uint16_t ide=0;
    uint16_t rtr=0;
    CAN_ID   id=0;

    // extract for speed
    word0 = dma->word[0];
    word1 = dma->word[1];
    word2 = dma->word[2];
    word3 = dma->word[3];

    // read word 0 to see the message type 
    ide = word0 & 0x0001;

    // check to see what type of message it is 
    if (ide==0)
    {
        // message is standard identifier (11 bit)
        can->jid.ID    = (word0 & 0x1FFC) >> 2;
        rtr            = word0 & 0x0002;
        can->frameType = CAN_FRAME_STD;
    }
    else
    {
        // mesage is extended identifier (29 bit) 
        id             = word0 & 0x1FFC;
        can->jid.ID    = id << 16;
        id             = word1 & 0x0FFF;
        can->jid.ID    = can->jid.ID+(id << 6);
        id             = (word2 & 0xFC00) >> 10;
        can->jid.ID    = can->jid.ID+id;
        rtr            = word2 & 0x0200;
        can->frameType = CAN_FRAME_EXT;
    }

    // check to see what type of message it is 
    if (rtr==1)
    {
        // Remote Transmission Request message 
        can->msgType = CAN_MSG_RTR;
    }
    else
    {
        // normal message 
        // extract for speed
        uint16_t word4, word5, word6;
        word4 = dma->word[4];
        word5 = dma->word[5];
        word6 = dma->word[6];

        can->msgType    = CAN_MSG_DATA;
        can->dataLength = (uint8_t)(word2 & 0x0F);
        can->data[0]    = (uint8_t)(word3     );
        can->data[1]    = (uint8_t)(word3 >> 8);
        can->data[2]    = (uint8_t)(word4     );
        can->data[3]    = (uint8_t)(word4 >> 8);
        can->data[4]    = (uint8_t)(word5     );
        can->data[5]    = (uint8_t)(word5 >> 8);
        can->data[6]    = (uint8_t)(word6     );
        can->data[7]    = (uint8_t)(word6 >> 8);
    } // else

//  LOG(SS_CAN, SV_INFO, "UnpackDma msgType=%u ide=%u rtr=%u", can->msgType, ide, rtr);
}

// ------------------------------------------------------------------------------------
// pack CAN message into DMA data
static void can_PackDma(CAN_MSG* can, CAN_DMA *dma)
{
    unsigned long word0=0;
    unsigned long word1=0;
    unsigned long word2=0;
    
    // check to see if the message has an extended ID
    if (can->frameType == CAN_FRAME_EXT)
    {
        word0 = (can->jid.ID & 0x1FFC0000) >> 16;   // get the extended message id EID28..18        
        word0 = word0+0x0003;                       // set the SRR and IDE bit
        word1 = (can->jid.ID & 0x0003FFC0) >> 6;    // the the value of EID17..6
        word2 = (can->jid.ID & 0x0000003F) << 10;   // get the value of EID5..0 for word 2
    }   
    else
    {
        word0 = ((can->jid.ID & 0x000007FF) << 2); // get the SID
        LOG(SS_CAN, SV_ERR, "11-bit CAN is Wrong");
    }
    if (can->msgType == CAN_MSG_RTR)
    {       
        // RTR message
        if (can->frameType == CAN_FRAME_EXT)
            word2 = word2 | 0x0200;
        else
            word0 = word0 | 0x0002; 
                                
        dma->word[0] = word0;
        dma->word[1] = word1;
        dma->word[2] = word2;
    }
    else
    {
        // normal message
        word2 = word2+(can->dataLength & 0x0F);
        dma->word[0] = word0;
        dma->word[1] = word1;
        dma->word[2] = word2;
        dma->word[3] = ((can->data[1] << 8) + can->data[0]);
        dma->word[4] = ((can->data[3] << 8) + can->data[2]);
        dma->word[5] = ((can->data[5] << 8) + can->data[4]);
        dma->word[6] = ((can->data[7] << 8) + can->data[6]);
    }

//  LOG(SS_CAN, SV_INFO, "PackDma msgType=%u frmType=%u", can->msgType, can->frameType);
}

// ------------------------------------------------------------------------------------
// returns Data Group Number of a can message
uint32_t can_DGN(CAN_MSG* msg)
{
    uint32_t dgn = (msg->jid.ID>>8) & 0x1FFFF;  // includes data-page

    /* 
    LOG(SS_CAN, SV_INFO, "DGN=%08lX P=%02X DP=%02X PUDF=%02X PUDS=%02X S=%02X",
        dgn, 
        (int)msg->jid.Priority,
        (int)msg->jid.DataPage,
        (int)msg->jid.PDUFormat,
        (int)msg->jid.PDUSpecific,
        (int)msg->jid.SourceAddress);
    */

    return(dgn);
}

// ------------------------------------------------------------------------------------
// used to dump can message contents to console for debugging
void can_DumpMsg(CAN_MSG* canMsg)
{
    int8_t buf[30];
    int i,n;

    // build up hex data string for printing
    n = canMsg->dataLength;
    if (n>MAX_CAN_DATA) n=MAX_CAN_DATA; // dont crash buffer
    memset(buf,0,sizeof(buf));
    for (i=0; i<n; i++)
    {
        sprintf(&buf[i*3],"%02X ", (unsigned)canMsg->data[i]);
    }

    // dump can data
    LOG(SS_CAN, SV_INFO, "%s ID=%08lx DGN=%05lx L=%u: %s",
        canMsg->frameType==CAN_FRAME_EXT?"29-Bit":"11-Bit",
        canMsg->jid.ID,
        can_DGN(canMsg),
        canMsg->dataLength,
        buf);
}

// ------------------------------------------------------------------------------------
// dispatch can messageas to appropriate handlers
static void can_CmdDispatcher(CAN_MSG* canMsg)
{
    if (canMsg->frameType == CAN_FRAME_EXT && canMsg->msgType == CAN_MSG_DATA)
    {
        if (J1939_Dispatcher(canMsg)) return; // handled
        if (rvcan_Dispatcher(canMsg)) return; // handled
    }

//  LOGX(SS_CAN, SV_ERR, "Rx Ignored", canMsg->data, canMsg->dataLength);
}

// ------------------------------------------------------------------------------------
// returns: 0=no, 1=yes
int16_t can_IsTxQueueEmpty(void)
{
    return(g_can.TxHead == g_can.TxTail ? 1 : 0);
}

// ------------------------------------------------------------------------------------
// returns: 0=no, 1=yes
int16_t can_IsRxQueueEmpty(void)
{
    return(g_can.RxHead == g_can.RxTail ? 1 : 0);
}

// ------------------------------------------------------------------------------------
// returns: 0=no, 1=yes
int16_t can_IsTxQueueFull(void)
{
    // if incrementing head makes pointers equal, then it is full
    uint8_t head = g_can.TxHead;    // use temporary
    INC_QUEUE_PTR(head, CAN_TX_QUEUE_SIZE);
    return (head == g_can.TxTail ? 1 : 0);
}

// ------------------------------------------------------------------------------------
// returns: 0=no, 1=yes
int16_t can_IsRxQueueFull(void)
{
    // if incrementing head makes pointers equal, then it is full
    uint8_t head = g_can.RxHead;    // use temporary
    INC_QUEUE_PTR(head, CAN_RX_QUEUE_SIZE);
    return (head == g_can.RxTail ? 1 : 0);
}

// ------------------------------------------------------------------------------------
// can transmitter driver
void can_TxDriver()
{
    // process any messages in the tranmit queue
    if (!can_IsTxQueueEmpty() && C1TR01CONbits.TXREQ0 != 1)  // can not be transmitting
    {
        // copy data to dma buffer
        g_canTxDmaBuf[0][0] = g_can.TxQueue[g_can.TxTail].word[0];
        g_canTxDmaBuf[0][1] = g_can.TxQueue[g_can.TxTail].word[1];
        g_canTxDmaBuf[0][2] = g_can.TxQueue[g_can.TxTail].word[2];
        g_canTxDmaBuf[0][3] = g_can.TxQueue[g_can.TxTail].word[3];
        g_canTxDmaBuf[0][4] = g_can.TxQueue[g_can.TxTail].word[4];
        g_canTxDmaBuf[0][5] = g_can.TxQueue[g_can.TxTail].word[5];
        g_canTxDmaBuf[0][6] = g_can.TxQueue[g_can.TxTail].word[6];
        g_canTxDmaBuf[0][7] = g_can.TxQueue[g_can.TxTail].word[7];

        INC_QUEUE_PTR(g_can.TxTail, CAN_TX_QUEUE_SIZE);
        C1TR01CONbits.TXREQ0 = 0x1;     // transmit it

        g_can.rvcTxFrameCount++;    // one more packet transmitted
    }
}

// ------------------------------------------------------------------------------------
// can receiver driver
static void can_RxDriver()
{
    // process any messages in receive queue
    if (!can_IsRxQueueEmpty())
    {
        CAN_MSG canMsg;
        can_UnpackDma(&g_can.RxQueue[g_can.RxTail], &canMsg);
        INC_QUEUE_PTR(g_can.RxTail, CAN_RX_QUEUE_SIZE);
        g_can.rvcRxFrameCount++;    // one more packet received
        can_CmdDispatcher(&canMsg);
    }
}

// ------------------------------------------------------------------------------------
// place a can message in the transmit queue
// returns: 0=ok, 1=tx queue is full
int16_t can_TxEnqueue(CAN_MSG* msg)
{
    if (can_IsTxQueueFull())
    {
     // LOG(SS_CAN, SV_ERR, "Tx Queue is Full");
        return(1);
    }
    can_PackDma(msg, &g_can.TxQueue[g_can.TxHead]); // put in the next queue slot
    INC_QUEUE_PTR(g_can.TxHead, CAN_TX_QUEUE_SIZE); // advance to next entry in queue
    // wake up handler to transmit
    task_MarkAsReady(_task_can);

//  LOG(SS_CAN, SV_INFO, "TxEnqueue msgType=%u frmType=%u", msg->msgType, msg->frameType);
//  can_DumpMsg(msg);

    return(0);
}

// ------------------------------------------------------------------------------------
#define BROADCAST_EVENT_MSEC  (2000)

// drive the CAN state machine
void TASK_can_Driver(void)
{
    static SYSTICKS s_lastTick = 0;
    static SYSTICKS s_ticks    = 0;
    static int      s_sendItem = 0;
    SYSTICKS nowTick = GetSysTicks();

    if (IsTimedOut(BROADCAST_EVENT_MSEC,s_ticks))
    {
        s_ticks = nowTick;    // reset timer
        s_sendItem = 1;
    }

    // send the can messages out on different timer ticks
    if (can_IsTxQueueFull()) s_sendItem = 0; // dont send if full
    switch (s_sendItem)
    {
    default: s_sendItem=0;                                    break;
    case  1: s_sendItem++; rvcan_Send_DM_RV();                break;
    case  2: s_sendItem++; rvcan_SendDcSourceStatus1();       break;
    case  3: s_sendItem++; rvcan_SendDcSourceStatus2();       break;
    case  4: s_sendItem++; rvcan_SendInverterStatus();        break;
    case  5: s_sendItem++; rvcan_SendInverterAcStatus1();     break;
    case  6: s_sendItem++; rvcan_SendInverterAcStatus3();     break;
    case  7: s_sendItem++; rvcan_SendInverterDcStatus();      break;
    case  8: s_sendItem++; rvcan_SendInverterConfigStatus1(); break;
 #ifdef OPTION_HAS_CHARGER
    case  9: s_sendItem++; rvcan_SendChargerStatus();         break;
    case 10: s_sendItem++; rvcan_SendChargerConfigStatus();   break;
    case 11: s_sendItem++; rvcan_SendChargerConfigStatus2();  break;
    case 12: s_sendItem++; rvcan_SendChargerAcStatus1();      break;
    case 13: s_sendItem++; rvcan_SendChargerAcStatus2();      break;
    case 14: s_sendItem++; rvcan_SendChargerEqualStatus();    break;
 #endif
    } // switch

    // keep the transmitter pumping
    can_TxDriver();

    // keep the receiver pumping
    can_RxDriver();

    if (nowTick >= s_lastTick)
    {
        J1939_Poll(nowTick-s_lastTick);
    }
    s_lastTick = nowTick;

    // enqueue task again if pending jobs
    if (!can_IsTxQueueEmpty() || !can_IsRxQueueEmpty()) 
    {
        // wake up to handle
        task_MarkAsReady(_task_can);
    }
}

// <><><><><><><><><><><><><> dsPIC33_CAN.c <><><><><><><><><><><><><><><><><><><><><><>
