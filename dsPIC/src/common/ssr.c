// <><><><><><><><><><><><><> ssr.c <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//  Copyright(C) 2016 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
//  Solid State Relay - Volta specific
//
//  This function is intended to address a High Voltage spike on the DC 
//  power bus. (This may result from the Volta Battery pack suddenly 
//  disconnecting itself from the DC Bus while being charged.)
//
//-----------------------------------------------------------------------------

// -------
// headers
// -------
#include "options.h"    // must be first include
#include "analog.h"
#include "ssr.h"
#include "tasker.h"

#if defined(OPTION_SSR) // exit here if no ssr option


// ------------
// Global Data
// ------------
int8_t ssr_TriggerOccurred = 0;
int8_t ssr_Shutdown = 0;
int8_t ssr_Failure = 0;

// ------------
// Prototyping
// ------------
extern void TASK_MainControl(void);


//-----------------------------------------------------------------------------
void ssr_Reset(void)
{
    ssr_TriggerOccurred = 0;
    ssr_Shutdown = 0;
    ssr_Failure = 0;
}

//-----------------------------------------------------------------------------
//  ssr_Detect() is called from the ADC interrupt, as soon as the DC voltage 
//  value has been converted.
//
//  The event is qualified by being in Inverter mode - more specifically, the
//  charge relay must NOT be CLOSED.
//
//  If the DC voltage exceeds the threshold, then we respond as follows:
//   1) Reconfigure the Inverter ISR (special ISR for SSR-mode)
//   2) Enable the SSR output - This will connect the SSR accross the secondary
//      (AC-side) of the transformer.
//   3) Start the Inverter ISR.
//   4) Configure the MainControl state machine appropriately
//
//-----------------------------------------------------------------------------

void ssr_Detect(void)
{
    TASK_ID_t taskid_mainc = -1;
    
    if(VBattRaw() > SSR_VDC_CLAMP_THRES)
    {
        taskid_mainc = task_AddToQueue(TASK_MainControl, "mainc");
        if(!ssr_TriggerOccurred)
            task_MarkAsReadyNow(taskid_mainc);
        else
            task_MarkAsReady(taskid_mainc);
        ssr_TriggerOccurred = 1;
    }
}

#endif // OPTION_SSR

// <><><><><><><><><><><><><> ssr.c <><><><><><><><><><><><><><><><><><><><><><>
 