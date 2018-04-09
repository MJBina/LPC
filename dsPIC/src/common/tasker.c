// <><><><><><><><><><><><><> tasker.c <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//	Copyright(C) 2015 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
//  Background Task Queuing
//
//-----------------------------------------------------------------------------

// -------
// headers
// -------
#include "options.h"    // must be first include
#include "tasker.h"
#include "dsPIC33_CAN.h"
#include "dspic_serial.h"

// ---------------------------
// Conditional Debug Compiles
// ---------------------------

// comment out flag to not use it
#define  DEBUG_SHOW_TASK_OVERTIME   1 // show task timer over msg run if > MAX_TASK_MSECS


// -----------------------------
// Auto Removal for build types
// -----------------------------
#if OPTION_NO_CONDITIONAL_DBG
  #undef DEBUG_SHOW_TASK_OVERTIME
#endif	

// ---------
// Constants
// ---------
#define TASK_NAME_LEN   (6)
#define MAX_TASK_MSECS  (2)  // max task time allowed before reporting error

// ------------------------
// queue info for each task
// ------------------------
#pragma pack(1)  // structure packing on byte alignment
typedef struct 
{
	char       id[TASK_NAME_LEN];   // name of the task; null terminated
	FUNC_PTR_t func;    // pointer to the function to execute
	int8_t     ready;	// flag indicating task should be run
  #ifdef  ENABLE_TASK_TIMING
    TASK_TIMING_t timing; // timing info
  #endif // ENABLE_TASK_TIMING

} TASK_QUEUE_t;
#pragma pack()  // restore packing setting

// ----------------
// task stats info
// ----------------
#define LEN_LOG_STR  (80)
#pragma pack(1)  // structure packing on byte alignment
typedef struct 
{
    TASK_TIMING_t timing; // timing info
	// calculations
	uint32_t   tavg;    // average tsum / count
	uint32_t   usecs;	// microseconds used during period
	uint32_t   msecs;	// milliseconds used during period (usec/1000)
	float      percent; // percent 0-100
	char       resultsStr[LEN_LOG_STR]; // results string
	uint16_t   resultsLen; // strlen(resultsStr)

} TASK_STATS_t;
#pragma pack()  // restore packing setting

// -----------
// prototyping
// -----------
void T7Config(void);
void T7Start(void);
void T8Config(void);
void T8Start(void);
void task_BenchMarkTimers(void);
void CalcTaskStats(TASK_STATS_t* stats);
void FormatTaskResults(TASK_STATS_t* stats, char* name);

// ----------------
// global task data
// ----------------

TASK_ID_t         _task_idle = -1;
TASK_QUEUE_t      g_task_queue[TASK_QUEUE_SIZE];
TRAP_ERROR_CODE_t g_trapLast    = TRAP_NO_ERROR;  // last trap that has occurred
uint32_t          g_trapLocLast = 0;              // address where last trap occured
SYSTICKS          g_taskTicks   = 0;

#ifdef  ENABLE_TASK_TIMING
  uint32_t          g_usecsTotal; // sum of all tasks and isr times for timing period
  // interrupt timing
  // timing
  TASK_TIMING_t   g_pwmTiming;  // running pwm timing
  TASK_TIMING_t   g_fanTiming;  // running fan timing
  TASK_TIMING_t   g_dmaTiming;  // running dam timing
  TASK_STATS_t    g_pwmStats;   // pwm timing stats for timing period
  TASK_STATS_t    g_fanStats;   // fan timing stats for timing period
  TASK_STATS_t    g_dmaStats;   // dma timing stats for timing period
  TASK_STATS_t    g_admStats;   // non timed tasks (left over to fill out timing period)
  TASK_STATS_t    g_task_stats[TASK_QUEUE_SIZE];
#endif // ENABLE_TASK_TIMING

//-----------------------------------------------------------------------------
// idle task; receives time when other tasks are not running
void TASK_idle(void)
{
}

//-----------------------------------------------------------------------------
//	initialize tasker sub-system
//-----------------------------------------------------------------------------
void task_Config(void)
{
    int16_t ix;
    
    // zero the memory
    memset(&g_task_queue,0,sizeof(g_task_queue));
   
    for(ix=0; ix<TASK_QUEUE_SIZE; ix++)
    {
        memset(g_task_queue[ix].id, 0, TASK_NAME_LEN);
        g_task_queue[ix].func  = NULL;
        g_task_queue[ix].ready = 0;
    }

  #ifdef  ENABLE_TASK_TIMING
    g_usecsTotal = 0;
    // timing
    memset(&g_pwmTiming, 0, sizeof(g_pwmTiming));
    memset(&g_fanTiming, 0, sizeof(g_fanTiming));
    memset(&g_dmaTiming, 0, sizeof(g_dmaTiming));
    // stats
    memset(&g_pwmStats,  0, sizeof(g_pwmStats ));
    memset(&g_fanStats,  0, sizeof(g_fanStats ));
    memset(&g_dmaStats,  0, sizeof(g_dmaStats ));
    memset(&g_admStats,  0, sizeof(g_admStats ));

    task_ResetTimers();
  #endif // ENABLE_TASK_TIMING
 

    _task_idle = task_AddToQueue(TASK_idle, "idle ");
}

//-----------------------------------------------------------------------------
//  task_AddToQueue(FUNC_PTR_t func, char * str)
//  A function to add a task to the task queue
//    - passed a pointer to a text string to identify the task when 'dumped'
//    - returns the index of the task in task queue
//    - returns -1 if an error occurred adding task, i.e queue full/too small
//    - if the task is already in the queue, return its index
//-----------------------------------------------------------------------------

TASK_ID_t task_AddToQueue(FUNC_PTR_t taskFunc, char* name)
{
    int16_t ix=0;
    
    for (ix = 0; ix < TASK_QUEUE_SIZE; ix++)
    {
        if (g_task_queue[ix].func == taskFunc)
            return(ix);
       
        if (g_task_queue[ix].func == NULL)
        {
            int len = strlen(name);
            if (len >= TASK_NAME_LEN) len = TASK_NAME_LEN-1;
            memcpy(g_task_queue[ix].id, name, len);
            g_task_queue[ix].func = taskFunc;
            return(ix);
        }
    }
	LOG(SS_SYS, SV_ERR, "task_AddToQueue - out of memory");
    return(-1);
}

// -----------------------------
// timer8 counts to microseconds
// -----------------------------
#define T8_TO_USEC(T8COUNTS)   ((16*((uint32_t)T8COUNTS))/10)


TRAP_ERROR_CODE_t task_GetLastErrorCode(void)
{
    return(g_trapLast);
}

//-----------------------------------------------------------------------------
// start task timers
void task_Start(void)
{
  #ifdef  ENABLE_TASK_TIMING
    // start timers
	T7Config();
	T8Config();
	T7Start();
	T8Start();
//  task_BenchMarkTimers();
  #endif // ENABLE_TASK_TIMING
}

//-----------------------------------------------------------------------------
//  task_MarkAsReady()
//  A function to mark a queued task for execution
//    - passed a handle to identify the task
//-----------------------------------------------------------------------------

void task_MarkAsReady(TASK_ID_t taskNo) 
{ 
    if(taskNo < TASK_QUEUE_SIZE)
    {
        g_task_queue[taskNo].ready = 1;  // mark this task as ready to run
    }
}

//	task_index always points to the next task to run
static int16_t _task_index = 0;

//-----------------------------------------------------------------------------
void task_MarkAsReadyNow(TASK_ID_t taskNo) 
{ 
    if(taskNo < TASK_QUEUE_SIZE)
    {
        g_task_queue[taskNo].ready = 1;  // mark this task as ready to run
        _task_index = taskNo;            // make this task run next   
    }
}

//-----------------------------------------------------------------------------
//	run a task if ready to run
//-----------------------------------------------------------------------------

void task_Execute(void)
{
  #ifdef  ENABLE_TASK_TIMING
	uint16_t exec_time;
  #endif

    // check for trap hit
    if (g_trapErr != 0)
    {
        // capture as last
        g_trapLast    = g_trapErr;
        g_trapLocLast = g_trapErrLoc;
        // clear to allow new trap to hit
        g_trapErr     = 0;  // clear trap flag
        g_trapErrLoc  = 0;  // clear trap address
        switch (g_trapLast)
        {
        default:
            LOG(SS_TASKER, SV_ERR, "Trap Hit: %s @ %08lX", g_trapName[g_trapLast], g_trapLocLast);
            break;

        case TRAP_DMA_ERR:
        case TRAP_ALT_DMA_ERR:
            can_Config();
            can_Start();
            LOG(SS_TASKER, SV_ERR, "Trap Hit: %s @ %08lX; Reset CAN Hardware", g_trapName[g_trapLast], g_trapLocLast);
            break;
        } // switch
    }
   
    // check if task is ready to run
	if (g_task_queue[_task_index].ready)
	{
     #ifdef DEBUG_SHOW_TASK_OVERTIME
        SYSTICKS msecs, startTicks = GetSysTicks(); // start timing task
     #endif
		g_task_queue[_task_index].ready = 0; // clear asap so as not to miss a request

        // capture idle time
     #ifdef  ENABLE_TASK_TIMING
		exec_time = TMR8;
		g_task_queue[_task_idle].timing.count++;
		g_task_queue[_task_idle].timing.tsum += exec_time;
		if (exec_time < g_task_queue[_task_idle].timing.tmin) g_task_queue[_task_idle].timing.tmin = exec_time;
		if (exec_time > g_task_queue[_task_idle].timing.tmax) g_task_queue[_task_idle].timing.tmax = exec_time;

        TMR8 = 0; // start task timer
     #endif

        // run the task
		g_task_queue[_task_index].func();

     #ifdef  ENABLE_TASK_TIMING
		exec_time = TMR8;  // capture task time
        TMR8 = 0;   // start idle timer again

        // save stats for task
		g_task_queue[_task_index].timing.count++;
		g_task_queue[_task_index].timing.tsum += exec_time;
		if (exec_time < g_task_queue[_task_index].timing.tmin) g_task_queue[_task_index].timing.tmin = exec_time;
		if (exec_time > g_task_queue[_task_index].timing.tmax) g_task_queue[_task_index].timing.tmax = exec_time;
     #endif // ENABLE_TASK_TIMING

     #ifdef DEBUG_SHOW_TASK_OVERTIME
        msecs = GetSysTicks() - startTicks;
        if (msecs > MAX_TASK_MSECS)
            LOG(SS_SYS, SV_ERR, "Task:%s took %lu msecs", g_task_queue[_task_index].id, msecs);
     #endif // DEBUG_SHOW_TASK_OVERTIME
	}
	
    // increment and wrap
	if (++_task_index >= TASK_QUEUE_SIZE)
	{
		_task_index = 0;
	}

  #ifdef  ENABLE_TASK_TIMING
	task_DumpDiags();
  #endif
 }

//-----------------------------------------------------------------------------
//                       T A S K    T I M I N G
//-----------------------------------------------------------------------------
//  Timer7 measures interrupt time
//  Timer8 measures foreground tasks
//    20 Mhz with 8:1 divisor = 6.25 Mhz
//-----------------------------------------------------------------------------

#ifdef  ENABLE_TASK_TIMING


//-----------------------------------------------------------------------------
//	reset task timers
//-----------------------------------------------------------------------------

void task_ResetTiming(TASK_TIMING_t* timing)
{
    memset(timing,0,sizeof(TASK_TIMING_t));
    timing->tmin  = 0xFFFF;
}

//-----------------------------------------------------------------------------
void task_ResetTimers(void)
{
	int16_t ix; 

    // reset isr counters
    task_ResetTiming(&g_pwmTiming);
    task_ResetTiming(&g_fanTiming);
    task_ResetTiming(&g_dmaTiming);

    // reset tasks counters
	for(ix=0; ix<TASK_QUEUE_SIZE; ix++) 	
	{
        if (g_task_queue[ix].func == NULL) continue;
        task_ResetTiming(&g_task_queue[ix].timing);
    } // for

    // zero isr timers
    TMR7 = 0;
    TMR8 = 0;
}

//-----------------------------------------------------------------------------
#define TICKS_PER_USEC   (6.25)   // timer ticks per microsecond

void task_DumpDiags(void)
{
	static uint8_t dumpState = 0; // 0=wait for time to dump, else machine states
	static uint8_t ix        = 0; // index into arrays for iterating

	switch (dumpState)
	{
	case 0: // wait for time to dump
	    if (!IsTimedOut(TASK_TIMING_PERIOD, g_taskTicks)) return; // not ready yet

        // copy isr timing data to stats
        memcpy(&g_pwmStats.timing, &g_pwmTiming, sizeof(TASK_TIMING_t));
        memcpy(&g_fanStats.timing, &g_fanTiming, sizeof(TASK_TIMING_t));
        memcpy(&g_dmaStats.timing, &g_dmaTiming, sizeof(TASK_TIMING_t));

		// copy task timing data to stats
		for (ix=0; ix<TASK_QUEUE_SIZE; ix++) 
	    {
            if (g_task_queue[ix].func == NULL) continue;
            memcpy(&g_task_stats[ix].timing, &g_task_queue[ix].timing, sizeof(TASK_TIMING_t));
	    } // for

        // advance to next step
		dumpState++;
        break;

	case 1: // reset timers to allow timing for next period
        task_ResetTimers();
        g_taskTicks = GetSysTicks(); // reset timer

        // timers are now running for next period
		g_usecsTotal = 0; // clear total usec sum

        // advance to next step
		dumpState++;
        ix = 0;
		break;

	case 2: // calculate task stats; one per pass
        if (g_task_queue[ix].func != NULL)
            CalcTaskStats(&g_task_stats[ix]);
		if (++ix < TASK_QUEUE_SIZE) break;

        // advance to next step
		dumpState++;
        ix = 0;
		break;

    case 3: // calc isr stats; one per pass
        switch (ix)
        {
        case 0: CalcTaskStats(&g_pwmStats);  break;
        case 1: CalcTaskStats(&g_fanStats);  break;
        case 2: CalcTaskStats(&g_dmaStats);  break;
        case 3: // admin is remainder
            g_admStats.usecs = g_usecsTotal > (TASK_TIMING_PERIOD*1000L) ? 0 : ((TASK_TIMING_PERIOD*1000L) - g_usecsTotal);
            g_admStats.msecs = g_admStats.usecs/1000;
            break;
        } // switch;
        if (++ix <= TASK_ISRS) break;

        // advance to next step
		dumpState++;
        ix = 0;
		break;

	case 4: // calculate task percentages; one per pass
        if (g_task_queue[ix].func != NULL)
            g_task_stats[ix].percent = ((float)g_task_stats[ix].usecs*100)/((float)TASK_TIMING_PERIOD*1000);
		if (++ix < TASK_QUEUE_SIZE) break;

        // advance to next step
		dumpState++;
        ix = 0;
		break;

	case 5:  // calc isr percentages
        switch (ix)
        {
        case 0: g_pwmStats.percent = ((float)g_pwmStats.usecs*100)/((float)TASK_TIMING_PERIOD*1000); break;
        case 1: g_fanStats.percent = ((float)g_fanStats.usecs*100)/((float)TASK_TIMING_PERIOD*1000); break;
        case 2: g_dmaStats.percent = ((float)g_dmaStats.usecs*100)/((float)TASK_TIMING_PERIOD*1000); break;
        case 3: g_admStats.percent = ((float)g_admStats.usecs*100)/((float)TASK_TIMING_PERIOD*1000); break;
        } // switch
        if (++ix <= TASK_ISRS) break;

        // advance to next step
		dumpState++;
		ix = 0;
		break;

	case 6: // format task results: one per pass
        if (g_task_queue[ix].func != NULL)
            FormatTaskResults(&g_task_stats[ix], g_task_queue[ix].id);
		if (++ix < TASK_QUEUE_SIZE) break;

        // advance to next step
		dumpState++;
		ix = 0;
        break;

	case 7: // format isr results: one per pass
        switch (ix)
        {
        case 0: FormatTaskResults(&g_pwmStats, "pwm"  );  break;
        case 1: FormatTaskResults(&g_fanStats, "fan"  );  break;
        case 2: FormatTaskResults(&g_dmaStats, "dma"  );  break;
        case 3: // format noTask results here since we have time
		    g_admStats.resultsLen = 
              sprintf(g_admStats.resultsStr, "%-5s                                     %7lu %6lu  %4.1f\r\n" , 
                "admin",  g_admStats.usecs,  g_admStats.msecs,  (double)g_admStats.percent);  // Microchip sprintf uses doubles for %f
        } // switch
        if (++ix <= TASK_ISRS) break;

        // advance to next step
		dumpState++;
		ix = 0;
		break;		

    // use direct serial port output for speed
	case 8: // dump isr results
		_serial_puts("\n\rISR----Counts----TmrSum---Min---Max---Avg---uSecs--mSecs-Percent (10 secs)\r\n"); // TASK_TIMING_PERIOD
		_serial_putbuf((uint8_t*)g_dmaStats.resultsStr, g_dmaStats.resultsLen);
		_serial_putbuf((uint8_t*)g_pwmStats.resultsStr, g_pwmStats.resultsLen);
		_serial_putbuf((uint8_t*)g_fanStats.resultsStr, g_fanStats.resultsLen);
		_serial_putbuf((uint8_t*)g_admStats.resultsStr, g_admStats.resultsLen);
		_serial_putbuf((uint8_t*)"\n\r", 2);

        // advance to next step
		dumpState++;
		ix = 0;
		break;		

	case 9: // dump task results
		_serial_puts("\n\rTASK---Counts----TmrSum---Min---Max---Avg---uSecs--mSecs-Percent\r\n"); // TASK_TIMING_PERIOD
		for (ix=0; ix<TASK_QUEUE_SIZE; ix++) 
		{
	        if (g_task_queue[ix].func == NULL) continue;
			_serial_putbuf((uint8_t*)g_task_stats[ix].resultsStr, g_task_stats[ix].resultsLen);
	    } // for
     // LOG(SS_TASKER, SV_INFO, "LastTrap=%u LastTrapLoc=%08lX", (unsigned int)g_trapLast, g_trapLocLast);
		_serial_putbuf((uint8_t*)"\n\r", 2);

        // start over again
		dumpState = 0;
		break;
	} // switch
}

//-----------------------------------------------------------------------------
void CalcTaskStats(TASK_STATS_t* stats)
{
    stats->tavg  = stats->timing.count ? stats->timing.tsum/stats->timing.count : 0;
    stats->usecs = (uint32_t)((float)stats->timing.tsum/(float)TICKS_PER_USEC);
    stats->msecs = stats->usecs/1000;
    g_usecsTotal += stats->usecs; // add to total
}

//-----------------------------------------------------------------------------
void FormatTaskResults(TASK_STATS_t* stats, char* name)
{
    stats->resultsLen = sprintf(stats->resultsStr,  "%-5s%8lu%10lu %5u %5u %5lu %7lu %6lu  %4.1f\r\n", 
    	    name,               stats->timing.count, stats->timing.tsum,
		    stats->timing.tmin, stats->timing.tmax,  stats->tavg,
            stats->usecs,       stats->msecs,        (double)stats->percent ); // Microchip sprintf uses doubles for %f
}

//-----------------------------------------------------------------------------
//                           T I M E R  7
//-----------------------------------------------------------------------------

void T7Config(void)
{
  #ifndef WIN32
    T7CONbits.TON = 0;     // Disable Timer

    // TCS: Timer Clock Source Select bit
    //      1 = External clock from pin TxCK
    //      0 = Internal clock (Tcy)
    T7CONbits.TCS = 0;

    T7CONbits.TGATE = 0;  // Disable Gated Timer mode

    // TCKPS<1:0>: Timer Input Clock Prescale Select bits
    //      11 = 1:256 prescale value
    //      10 = 1:64 prescale value
    //      01 = 1:8 prescale value
    //      00 = 1:1 prescale value
    T7CONbits.TCKPS = 0b1; // 1:8

    //  Clear the timer register
    TMR7 = 0;
//  PR7: 16-bit period register
//  PR7 = 5;  //  1-microsecond
    PR7 = 0xFFFF;
  #endif // WIN32
}

//-----------------------------------------------------------------------------
void T7Start(void)
{
    TMR7 = 0;
    T7CONbits.TON = 1;      // turn on timer 7
}

//-----------------------------------------------------------------------------
//  Timer-7 Interrupt ... NOT USED
//-----------------------------------------------------------------------------

void __attribute__((interrupt, no_auto_psv)) _T7Interrupt (void)
{
    IFS3bits.T7IF = 0;  // end-of-interrupt
}

//-----------------------------------------------------------------------------
//                           T I M E R  8
//-----------------------------------------------------------------------------

void T8Config(void)
{
  #ifndef WIN32
    T8CONbits.TON = 0;  // Disable Timer

    // TCS: Timer Clock Source Select bit
    //      1 = External clock from pin TxCK
    //      0 = Internal clock (Tcy)
    T8CONbits.TCS = 0;

    T8CONbits.TGATE = 0;  // Disable Gated Timer mode

    // TCKPS<1:0>: Timer Input Clock Prescale Select bits
    //      11 = 1:256 prescale value
    //      10 = 1:64 prescale value
    //      01 = 1:8 prescale value
    //      00 = 1:1 prescale value
    T8CONbits.TCKPS = 0b1; // 1:8

    //  Clear the timer register
    TMR8 = 0;
//  PR8: 16-bit period register for Timer
//  PR8 = 5;  //  1-microsecond
    PR8 = 0xFFFF;
  #endif // WIN32
}

//-----------------------------------------------------------------------------
void T8Start(void)
{
    TMR8 = 0;
    T8CONbits.TON = 1; // turn on timer
}

//-----------------------------------------------------------------------------
void __attribute__((interrupt, no_auto_psv)) _T8Interrupt (void) // not used
{
    IFS3bits.T8IF = 0;  // end-of-interrupt
}

//-----------------------------------------------------------------------------
/*
void task_BenchMarkTimers(void)
{
    #define TASK_BM_MSECS  (800)
    uint16_t timerCounts;
    SYSTICKS startTicks = GetSysTicks();

    // benchmark timer 7
    startTicks = GetSysTicks();
    while (startTicks != GetSysTicks()) { } // wait for millisecond to turn
    startTicks = GetSysTicks();
    TMR7 = 0;
    while (!IsTimedOut(TASK_BM_MSECS,startTicks)) { }
    timerCounts = TMR7;
	LOG(SS_TASKER, SV_INFO, "Timer7 %u counts in %u msec", timerCounts, TASK_BM_MSECS);

    // benchmark timer 8
    startTicks = GetSysTicks();
    while (startTicks != GetSysTicks()) { } // wait for millisecond to turn
    startTicks = GetSysTicks();
    TMR8 = 0;
    while (!IsTimedOut(TASK_BM_MSECS,startTicks)) { }
    timerCounts = TMR8;
	LOG(SS_TASKER, SV_INFO, "Timer8 %u counts in %u msec", timerCounts, TASK_BM_MSECS);

    // Timer 59,566    counts in 800 msec @ 1/256  ~ 20 Mhz
}
*/

#endif // ENABLE_TASK_TIMING


//--------------------------------------------------------------------------+
// 2016-03-31:  10 second run in inverter mode;  T8 tick = 1.6 usec         |
//------------------------------------+-------------------------------------+
//             Optimization Off       |         Optimization Level 1        |
//  TASK-----T8SUM--PERCENT    usecs  |  TASK-----T8SUM--PERCENT    usecs   |
//  idle   4213991  72.5    6,742,386 |  idle   5756462  94.8    9,210,339  |
//  mainc   388188   6.7      621,101 |  mainc   104930   1.7      167,888  |
//  can     469500   8.1      751,200 |  can      72035   1.2      115,256  |
//  ana     149887   2.6      239,819 |  ana      30696   0.5       49,114  |
//  devio   104132   1.8      166,611 |  devio    24534   0.4       39,254  |
//  spirx    67371   1.2      107,794 |  spirx    10619   0.2       16,990  |
//  spi      34977   0.6       55,963 |  spi       6308   0.1       10,093  |
//  ui       49745   0.9       79,592 |  ui        5303   0.1        8,485  |
//  inv     223518   3.8      357,629 |  inv      46991   0.8       75,186  |
//  chg      19154   0.3       30,646 |  chg       2791   0.0        4,466  |
//  nvm      46139   0.8       73,822 |  nvm       6465   0.1       10,344  |
//  hs        6137   0.1        9,819 |  hs        1891   0.0        3,026  |
//  temp     43045   0.7       68,872 |  temp      3494   0.1        5,590  |
//  fan         89   0.0          142 |  fan         11   0.0           18  |
//  logan        0   0.0            0 |  logan        0   0.0            0  |
//------------------------------------+-------------------------------------+
//                          9,305,397 |                          9,716,048  |
//------------------------------------+-------------------------------------+



//----------------------------------------------------------------+
// 2017-09-27: IDLE MODE                          6.25 ticks=usec |
//----------------------------------------------------------------+
// ISR----Counts----TmrSum---Min---Max---Avg---uSecs--mSecs-Percent (10 secs)
// dma    230483  13599118    59    60    59 2175859   2175  21.8
// pwm    230482    463964     2     7     2   74234     74   0.7
// fan    230482    257596     1     3     1   41215     41   0.4
// admin                                      266671    266   2.7
//
// TASK---Counts----TmrSum---Min---Max---Avg---uSecs--mSecs-Percent
// idle   185156  39483196     6 39878   213 6317311   6317  63.2
// ana       600    224751   326  1283   374   35960     35   0.4
// hs        103     12701    95   169   123    2032      2   0.0
// can     94601   4746689    25   112    50  759470    759   7.6
// mainc    9983    315905    16   103    31   50544     50   0.5
// devio    9983    322115    16   103    32   51538     51   0.5
// spi      9983    166887     8    94    16   26701     26   0.3
// ui       9983    103702     5   222    10   16592     16   0.2
// inv      9984    196281     9    83    19   31404     31   0.3
// chg      9984     57433     3    76     5    9189      9   0.1
// nvm      9984    104963     5    90    10   16794     16   0.2
// temp     9984    715241    35   122    71  114438    114   1.1
// fan      9984     62802     3    76     6   10048     10   0.1
//----------------------------------------------------------------+


//----------------------------------------------------------------+
// 2017-09-27: INVERTING                          6.25 ticks=usec |
//----------------------------------------------------------------+
// ISR----Counts----TmrSum---Min---Max---Avg---uSecs--mSecs-Percent (10 secs)
// dma    230483  13628506    59    60    59 2180561   2180  21.8
// pwm    230483   7487052    31    38    32 1197928   1197  12.0
// fan    230483    257597     1     3     1   41215     41   0.4
// admin                                           0      0   0.0
// 
// TASK---Counts----TmrSum---Min---Max---Avg---uSecs--mSecs-Percent
// idle   155137  38010251    19  9073   245 6081640   6081  60.8
// ana       600    335999   479   628   559   53759     53   0.5
// hs        103     18175   124   212   176    2908      2   0.0
// can     64661   4489714    25   175    69  718354    718   7.2
// mainc    9974    454232    16   102    45   72677     72   0.7
// devio    9974    431823    16   103    43   69091     69   0.7
// spi      9975    237984     8    87    23   38077     38   0.4
// ui       9975    156910     5  1256    15   25105     25   0.3
// inv      9975    556354    20   106    55   89016     89   0.9
// chg      9975     81176     3    88     8   12988     12   0.1
// nvm      9975    128844     5    92    12   20615     20   0.2
// temp     9975    952503    35   185    95  152400    152   1.5
// fan      9975     89478     3    88     8   14316     14   0.1
//----------------------------------------------------------------+


//----------------------------------------------------------------+
// 2017-09-27: CHARGING                           6.25 ticks=usec |
//----------------------------------------------------------------+
// ISR----Counts----TmrSum---Min---Max---Avg---uSecs--mSecs-Percent (10 secs)
// dma    230482  13630022    59    60    59 2180803   2180  21.8
// pwm    230483   5193876    22    29    22  831020    831   8.3
// fan    230483    257599     1     3     1   41215     41   0.4
// admin                                           0      0   0.0
// 
// TASK---Counts----TmrSum---Min---Max---Avg---uSecs--mSecs-Percent
// idle   161312  37258636    19 62826   230 5961382   5961  59.6
// ana       600    318347   471   604   530   50935     50   0.5
// hs        103     15851   116   194   153    2536      2   0.0
// can     70930   4428130    25   165    62  708500    708   7.1
// mainc    9964    405923    16   104    40   64947     64   0.6
// devio    9964    382865    15   102    38   61258     61   0.6
// spi      9964    223158     8    94    22   35705     35   0.4
// ui       9966    150891     5  1057    15   24142     24   0.2
// inv      9966    226279     9    95    22   36204     36   0.4
// chg      9967   1781912   120 10411   178  285105    285   2.9
// nvm      9968    118726     5    78    11   18996     18   0.2
// temp     9960    849416    35   175    85  135906    135   1.4
// fan      9960     83171     3    80     8   13307     13   0.1
//----------------------------------------------------------------+

                                                                                      
// <><><><><><><><><><><><><> tasker.c <><><><><><><><><><><><><><><><><><><><><><>
