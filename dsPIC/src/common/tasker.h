// <><><><><><><><><><><><><> tasker.h <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//	Copyright(C) 2016 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
//  Background Task Queuing
//
//  How to use the tasker:
//    A task is added to the task list by calling task_AddToQueue(). 
//    A task is scheduled/flagged to be run by calling task_MarkAsReady().
//    Both of the above will be performed within main().
//
//-----------------------------------------------------------------------------

#ifndef _TASKER_H   // include only once
#define _TASKER_H

// -------
// headers
// -------
#include "options.h"    // must be first include

// -------------------------------------
// conditional compiles for task timing
// -------------------------------------
//#define ENABLE_TASK_TIMING  1 // enable task and interrupt timing

// ----------
// constants
// ----------
#define TASK_QUEUE_SIZE	  (20)
#define TASK_ISRS          (3)
#define TASK_TIMING_PERIOD  (10000)  // task timing period (milliseconds)

// -----------------
// trap error codes
// -----------------
typedef enum
{
    TRAP_NO_ERROR = 0,
    TRAP_OSC_ERR = 1,
    TRAP_ADDR_ERR,
    TRAP_STACK_ERR,
    TRAP_MATH_ERR,
    TRAP_DMA_ERR,
    TRAP_ALT_OSC_ERR,
    TRAP_ALT_ADDR_ERR,
    TRAP_ALT_STACK_ERR,
    TRAP_ALT_MATH_ERR,
    TRAP_ALT_DMA_ERR
} TRAP_ERROR_CODE_t;

#if !defined NULL
  #define NULL ((void *)0)
#endif


// -----------------------------
// tracking info for task timing
// -----------------------------
#pragma pack(1)  // structure packing on byte alignment
typedef struct 
{
    // times are counted as Timer clocks
    uint32_t count; // number of interrupts
    uint16_t tmin;  // min duration time in isr
    uint16_t tmax;  // max duration time in isr
    uint32_t tsum;  // time sum for isr during timing period
} TASK_TIMING_t;
#pragma pack()  // restore packing setting


// --------------
// data structure
// --------------
typedef void (* FUNC_PTR_t)(void);   // function pointer
typedef int16_t TASK_ID_t;


// --------------------
// Function Prototyping
// --------------------

extern TASK_ID_t task_AddToQueue(FUNC_PTR_t taskFunc, char * name);
extern void task_Config(void);
extern void task_DumpDiags(void);
extern void task_Execute(void);
extern void task_MarkAsReady(TASK_ID_t taskNo);
extern void task_MarkAsReadyNow(TASK_ID_t taskNo);
extern void task_Start(void);
extern TRAP_ERROR_CODE_t task_GetLastErrorCode(void);
extern uint32_t getErrLoc(void); // assembly code for retrieving address location

// ------------------
// access global data
// ------------------
extern TASK_ID_t  _task_idle;   // handle id for the idle task

// isr timing
#ifdef ENABLE_TASK_TIMING
  extern TASK_TIMING_t  g_pwmTiming;
  extern TASK_TIMING_t  g_fanTiming;
  extern TASK_TIMING_t  g_dmaTiming;
#endif

// exernals from traps.c(since no traps.h)
extern const char* g_trapName[]; // list of trap names
extern uint8_t     g_trapErr;    // last trap error code (0=none)
extern uint32_t    g_trapErrLoc; // for address error trap, address where occured
      
#endif	//	_TASKER_H

// <><><><><><><><><><><><><> tasker.h <><><><><><><><><><><><><><><><><><><><><><>
