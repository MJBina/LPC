// <><><><><><><><><><><><><> log.c <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//  Copyright(C) 2015 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
//  This file defines the CAN bus layer routines
//    and data structures for the dsPIC33 
//  
//-----------------------------------------------------------------------------

// -------
// headers
// -------
#include "options.h"    // must be first include
#include "hw.h"
#include "dsPIC_serial.h"
#include <stdarg.h>

//------------------------------------------------------------------------------
//        S C O P E    L O G G I N G    R O U T I N E S
//------------------------------------------------------------------------------

// output a series of "num" pulses on RB6 (PICKit3 signal) for monitoring on scope
// not usable from an interrupt
#if !defined(OPTION_NO_CONDITIONAL_DBG)
void RB6_NumToScope(uint8_t num)
{
    int i,j;
    if (num < 1) return;
    for (j=0; j<15; j++) GetSysTicks(); // time delay  450 nsec
    for(i=0; i<num; i++)
    {
        RB6_SET(1);
        for (j=0; j<5; j++) GetSysTicks(); // 150 nsec
        RB6_SET(0);
        for (j=0; j<4; j++) GetSysTicks(); // time delay 120 nsec
    } // for
}

//------------------------------------------------------------------------------
// output a series of "num" pulses on RB7 (PICKit3 signal) for monitoring on scope
// not usable from an interrupt
void RB7_NumToScope(uint8_t num)
{
    int i,j;
    if (num < 1) return;
    for (j=0; j<15; j++) GetSysTicks(); // time delay; 450 nsec
    for(i=0; i<num; i++)
    {
        RB7_SET(1);
        for (j=0; j<5; j++) GetSysTicks(); // time delay 150 nsec
        RB7_SET(0);
        for (j=0; j<4; j++) GetSysTicks(); // time delay 120 nsec
    } // for
}
#endif // !OPTION_NO_CONDITIONAL_DBG


#ifndef OPTION_NO_LOGGING

// -------------------------
// Conditional Compile Flags
// -------------------------
//#define SHOW_ALL_LOG_TIMES   1 // show all log times; else only show exceeded times

// -----------------
// local global data
// -----------------
static LOG_SEVERITY_t _Severity[NUM_LOG_SUBSYS_t];

#define SUBSYS_NAME_LEN  4
const char _SubsysStr[NUM_LOG_SUBSYS_t][SUBSYS_NAME_LEN+1] =
{
    {"SYS"},
    {"CAN"},
    {"CFG"},
    {"CHG"},
    {"INV"},
    {"J19"},
    {"SEN"},
    {"PWM"},
    {"RVC"},
    {"TSK"},
    {"UC "},
    {"->I"},
    {"->C"},
    {"NVM"},
    {"ANA"},
    {"UI "},
};

#define SEVERITY_NAME_LEN  2
const char _SeverityStr[NUM_LOG_SUBSYS_t][SEVERITY_NAME_LEN+1] =
{
    {"A"}, // all
    {" "}, // info
    {"D"}, // debug
    {"W"}, // warning
    {"E"}, // error
    {"N"}, // none
};

//------------------------------------------------------------------------------
//              L O G    T I M I N G     O V E R R U N
//------------------------------------------------------------------------------

#define LOG_TIMER_EXCEEDED   4 // milliseconds (less than to be ok)

SYSTICKS g_logTimerStart;

//------------------------------------------------------------------------------
// start the timer
// dont call directly; use LOG_TIMER_START()
void _log_TimerStart(void)
{
    g_logTimerStart = GetSysTicks();
}

//------------------------------------------------------------------------------
// end the timer and check time overflow
// dont call directly; use LOG_TIMER_END()
void _log_TimerEnd(void)
{
    SYSTICKS nowTicks = GetSysTicks();
    uint32_t msecs    = nowTicks - g_logTimerStart;
    if (msecs >= LOG_TIMER_EXCEEDED)
    {
        _log(nowTicks, SS_SYS, SV_ERR, " ***  LogTimer Exceeded %lu msec ***", msecs);
    }
  #ifdef SHOW_ALL_LOG_TIMES
	else
	{
        _log(nowTicks, SS_SYS, SV_INFO, "LogTime %lu msecs", msecs);
	}
  #endif // SHOW_ALL_LOG_TIMES
}

//------------------------------------------------------------------------------
//  log_Config must be called at startup to initialize the log.
//------------------------------------------------------------------------------

//  CAUTION! dont call this directly
void _log_Config(void)
{
    int i;

    for (i=0; i<NUM_LOG_SUBSYS_t; i++)
    {
        _Severity[i] = LOG_DISABLE;
    }
}

//------------------------------------------------------------------------------
//  log_Severity can be called anytime to set the threshold for log messages.
//------------------------------------------------------------------------------

//  CAUTION! dont call this directly
void _log_Severity(LOG_SUBSYS_t subsys, LOG_SEVERITY_t severity)
{
    _Severity[subsys] = severity;
}

//------------------------------------------------------------------------------
// set all subsystems with severity level
//  CAUTION! dont call this directly
void _log_SeverityAll(LOG_SEVERITY_t severity)
{
    int i;

    for (i=0; i<NUM_LOG_SUBSYS_t; i++)
    {
        _Severity[i] = severity;
    }
}

#define HDR_FORMAT   "%08lX %s %s "

//------------------------------------------------------------------------------
//  _log is called using the macro 'LOG' to write debug messages to the serial 
//  port.  The specific subsystems must be enabled.
//------------------------------------------------------------------------------

//  CAUTION! dont call this directly
void _log(SYSTICKS ticks, LOG_SUBSYS_t subsys, LOG_SEVERITY_t severity, char * fmt, ...)
{
    char buf[120];  // keep stack usage as small as possible
    uint16_t len, lenh, avail;
    va_list args;

    if (severity < _Severity[subsys]) return;   // dont log if severity level not met

    va_start (args, fmt);

    // fill header
    lenh = sprintf(buf,HDR_FORMAT, ticks, _SubsysStr[subsys], _SeverityStr[severity]);

    // fill message portion and trailer
    len = vsprintf(&buf[lenh], fmt, args) + lenh;
    memcpy(&buf[len],"\r\n", 2);  // new line
    len += 2;   // new line length

    // get bytes available in transmitter buffer
    avail = _serial_TxFreeSpace();
    if (len > avail)
    {
        // no room in the inn
        if (avail > 0)
            _serial_putc('~'); // missed message indicator
    }
    else
    {
        // send message
        _serial_putbuf((unsigned char*)buf, len);
    }

    va_end (args);
}

//------------------------------------------------------------------------------
//  logs data in hex format
//------------------------------------------------------------------------------

//  CAUTION! dont call this directly; Let the LOGX macro do it
void _logx(LOG_SUBSYS_t subsys, LOG_SEVERITY_t severity, char* preText, uint8_t *data, int16_t len)
{
    #define BYTES_PER_LINE  (16)
    uint16_t i,thisRow=0, lenh;
    char buf[30];

    if (len < 1) return;
    
    if (severity >= _Severity[subsys])
    {
        // header
        lenh = sprintf(buf,HDR_FORMAT, GetSysTicks(), _SubsysStr[subsys], _SeverityStr[severity]);
        _serial_putbuf((unsigned char*)buf,lenh);   

        // pretext if any
        if (preText) _serial_puts(preText);   

        // data
        for (i=0; i<len; i++)
        {
            ui8tox(buf,data[i]);
            buf[2] = ' ';
            buf[3] = '\0';
            _serial_putbuf((unsigned char*)buf,3);
            thisRow++;
            if (thisRow < BYTES_PER_LINE) continue;
            // new line
            _serial_puts("\r\n          ");
            thisRow =0;

        } // for

        // trailer
        _serial_putbuf((unsigned char*)"\r\n",2);   // new line
    }
}

// --------------------------------------------------------------
//       S A F E    L O G G I N G  (can call from interrupts)
// --------------------------------------------------------------
#if defined(OPTION_SAFE_LOGGING) || defined(WIN32)

  // circular buffer for saving to memory
  #define SAFE_LOG_RECORDS     200  // size of circular buffer
  #define SAFE_LOG_MSG_LEN     16   // max length of text message (will be truncated)

  // info for one safe log record
  #pragma pack(1)  // structure packing on byte alignment
  typedef struct safe_log_record_s
  {
      SYSTICKS       ticks;
      LOG_SUBSYS_t   subsys;
      LOG_SEVERITY_t severity;
      char           msg[SAFE_LOG_MSG_LEN];
      int            v1;
      int            v2;
      int            v3;
      int            v4;
  } SLOG_REC;
  #pragma pack()  // restore packing setting

  // safe log queue
  #pragma pack(1)  // structure packing on byte alignment
  typedef struct safe_log_q
  {
      int      nextIn;   // next value to fill
      int      nextOut;  // next value to take out
      SLOG_REC records[SAFE_LOG_RECORDS];
  } SLOG_Q;
  #pragma pack()  // restore packing setting
  SLOG_Q g_slog_q = { 0 };

  // safe logging function;
  // DONT CALL DIRECTLY; use SLOG(...) macro to map it
  void _log_Safe(LOG_SUBSYS_t subsys, LOG_SEVERITY_t severity, char* msg, int v1, int v2, int v3, int v4)
  {
      int n;
      // acquire the memory slot safely (disable interrupts)
      __builtin_disi(0x3FF);
      n = g_slog_q.nextIn;  // slot to fill
      if (++g_slog_q.nextIn >= SAFE_LOG_RECORDS) g_slog_q.nextIn = 0; // advance pointer and wrap
      __builtin_disi(0x1); // enable ints for one more cycle

      // fill the acquired slot with the data
      g_slog_q.records[n].ticks    = GetSysTicks(); // capture time
      g_slog_q.records[n].subsys   = subsys;
      g_slog_q.records[n].severity = severity;
      strncpy(g_slog_q.records[n].msg, msg, SAFE_LOG_MSG_LEN);
      g_slog_q.records[n].msg[SAFE_LOG_MSG_LEN-1] = 0; // null term
      g_slog_q.records[n].v1 = v1;
      g_slog_q.records[n].v2 = v2;
      g_slog_q.records[n].v3 = v3;
      g_slog_q.records[n].v4 = v4;
  }

  // dump safe log queue
  // MUST call this periodically to dump queue
  // DONT call directory; call SLOG_DUMP() macro
  #define MAX_SAFE_DUMP_MSECS  (3)  // max time allowed for dumping safe logging per main loop
  void _log_Safe_DumpQueue()
  {
      SYSTICKS startTicks = GetSysTicks();
      while (_serial_TxFreeSpace() > 100 && g_slog_q.nextIn != g_slog_q.nextOut)
      {
          int n = g_slog_q.nextOut;
          _log(g_slog_q.records[n].ticks, g_slog_q.records[n].subsys, g_slog_q.records[n].severity,
              "%s,%u,%u,%u,%u",
              g_slog_q.records[n].msg,
              g_slog_q.records[n].v1,
              g_slog_q.records[n].v2,
              g_slog_q.records[n].v3,
              g_slog_q.records[n].v4 );
          if (++g_slog_q.nextOut >= SAFE_LOG_RECORDS) g_slog_q.nextOut = 0;
          if (IsTimedOut(MAX_SAFE_DUMP_MSECS, startTicks)) break; // dont consume too much time doing this
      } // while
  }

#endif // OPTION_SAFE_LOGGING

// --------------------------------------------------------------
// Time Logging Routines
#ifdef DEBUG_BENCHMARK
 uint8_t g_serial_DiscardData = 0;  // 0=normal, 1=dont transmit data (test mode)
 #include "analog.h"
 #include "charger.h"

// call to test timing of logging functions; call before main control loop
void _log_TimingTest()
{
    uint32_t i, nLoops = 10000, usecs;
    SYSTICKS startTicks, endTicks;
    char prbuf[30];
    uint16_t adc   = 123;
    uint16_t i16   = 12345;
    uint32_t i32   = 987654321;
    uint16_t adc1, adc2, adc3, adc4, adc5, adc6, adc7, adc8, adc9, adc10;
    float    fval  = (float)3.141592654;
    float    f1,f2,f3,f4,f5,f6,f7,f8,f9,f10;

    // Timing Test #1
    startTicks = GetSysTicks();
    g_serial_DiscardData = 1;
    for (i=0; i<nLoops; i++) { LOG(SS_SYS, SV_INFO, "LOG Timing Test#1"); }
    endTicks = GetSysTicks();
    g_serial_DiscardData = 0;
    usecs = (1000*(endTicks - startTicks))/nLoops;
    LOG(SS_SYS, SV_INFO, "usec=%lu   LOG Timing Test#1", usecs);
    // usec=83   LOG Timing Test#1

    // Timing Test #2
    startTicks = GetSysTicks();
    g_serial_DiscardData = 1;
    for (i=0; i<nLoops; i++) { LOG(SS_SYS, SV_INFO, "LOG Timing Test#2 i16=%d", i16); }
    endTicks = GetSysTicks();
    g_serial_DiscardData = 0;
    usecs = (1000*(endTicks - startTicks))/nLoops;
    LOG(SS_SYS, SV_INFO, "usec=%lu  LOG Timing Test#2 i16=%%d (%d)", usecs, i16);
    // usec=111  LOG Timing Test#2 i16=%d (12345)

    // Timing Test #3
    startTicks = GetSysTicks();
    g_serial_DiscardData = 1;
    for (i=0; i<nLoops; i++) { LOG(SS_SYS, SV_INFO, "LOG Timing Test#3 i32=%ld", i32); }
    endTicks = GetSysTicks();
    g_serial_DiscardData = 0;
    usecs = (1000*(endTicks - startTicks))/nLoops;
    LOG(SS_SYS, SV_INFO, "usec=%lu  LOG Timing Test#3 i32=%%ld  (%ld)", usecs, i32);
    // usec=224  LOG Timing Test#3 i32=%ld  (987654321)

    // Timing Test #4
    startTicks = GetSysTicks();
    g_serial_DiscardData = 1;
    for (i=0; i<nLoops; i++) { LOG(SS_SYS, SV_INFO, "LOG Timing Test#4 fval=%f", fval); }
    endTicks = GetSysTicks();
    g_serial_DiscardData = 0;
    usecs = (1000*(endTicks - startTicks))/nLoops;
    LOG(SS_SYS, SV_INFO, "usec=%lu  LOG Timing Test#4 fval=%%f  (%f)", usecs, fval);
    // usec=348  LOG Timing Test#4 fval=%f  (3.141593)

    // Timing Test #5
    startTicks = GetSysTicks();
    g_serial_DiscardData = 1;
    for (i=0; i<nLoops; i++) { LOG(SS_SYS, SV_INFO, "LOG Timing Test#5 fval=%2.1f", VBATT_ADC_VOLTS(adc)); }
    endTicks = GetSysTicks();
    g_serial_DiscardData = 0;
    usecs = (1000*(endTicks - startTicks))/nLoops;
    LOG(SS_SYS, SV_INFO, "usec=%lu  LOG Timing Test#5 fval=%%2.1f  (%2.1f)", usecs, VBATT_ADC_VOLTS(adc));
    // usec=211  LOG Timing Test#5 fval=%2.1f  (2.2)

    // Timing Test #6
    startTicks = GetSysTicks();
    g_serial_DiscardData = 1;
    for (i=0; i<nLoops; i++) { LOG(SS_SYS, SV_INFO, "LOG Timing Test#6 i16=%s", i16toa(prbuf,i16)); }
    endTicks = GetSysTicks();
    g_serial_DiscardData = 0;
    usecs = (1000*(endTicks - startTicks))/nLoops;
    LOG(SS_SYS, SV_INFO, "usec=%lu  LOG Timing Test#6 i16=i16toa()  (%s)", usecs, i16toa(prbuf,i16));
    // usec=108  LOG Timing Test#6 i16=i16toa()  (12345)

    // Timing Test #7
    startTicks = GetSysTicks();
    g_serial_DiscardData = 1;
    for (i=0; i<nLoops; i++) { LOG(SS_SYS, SV_INFO, "LOG Timing Test#7 i32=%s", i32toa(prbuf,i32)); }
    endTicks = GetSysTicks();
    g_serial_DiscardData = 0;
    usecs = (1000*(endTicks - startTicks))/nLoops;
    LOG(SS_SYS, SV_INFO, "usec=%lu  LOG Timing Test#7 i32=i32toa()  (%s)", usecs, i32toa(prbuf,i32));
    // usec=313  LOG Timing Test#7 i32=i32toa()  (987654321)

    // Timing Test #8
    startTicks = GetSysTicks();
    g_serial_DiscardData = 1;
    for (i=0; i<nLoops; i++) { LOG(SS_SYS, SV_INFO, "LOG Timing Test#8 fval=%s", ftoa2(prbuf,fval,1)); }
    endTicks = GetSysTicks();
    g_serial_DiscardData = 0;
    usecs = (1000*(endTicks - startTicks))/nLoops;
    LOG(SS_SYS, SV_INFO, "usec=%lu  LOG Timing Test#8 fval=ftoa2()  (%s)", usecs, ftoa2(prbuf,fval,1));
    // usec=161  LOG Timing Test#8 fval=ftoa2()  (3.1)

    // Timing Test #9
    startTicks = GetSysTicks();
    g_serial_DiscardData = 1;
    for (i=0; i<nLoops; i++)
    {
       LOG(SS_CHG, SV_INFO, "Test#9 BattRcp CvSet=%.1f CvVMax=%.1f FltSet=%.1f FltMax=%.1f EqSet=%.1f EqMax=%.1f",
           VBATT_ADC_VOLTS(Chgr.config.battery_recipe.cv_vsetpoint   ) ,  
           VBATT_ADC_VOLTS(Chgr.config.battery_recipe.cv_volts_max   ) ,  
           VBATT_ADC_VOLTS(Chgr.config.battery_recipe.float_vsetpoint) ,
           VBATT_ADC_VOLTS(Chgr.config.battery_recipe.float_volts_max) ,
           VBATT_ADC_VOLTS(Chgr.config.battery_recipe.eq_vsetpoint   ) ,  
           VBATT_ADC_VOLTS(Chgr.config.battery_recipe.eq_volts_max   )   ); 
    } // for
    endTicks = GetSysTicks();
    g_serial_DiscardData = 0;
    usecs = (1000*(endTicks - startTicks))/nLoops;
    LOG(SS_CHG, SV_INFO, "usec=%lu  Test#9 BattRcp CvSet=%.1f CvVMax=%.1f FltSet=%.1f FltMax=%.1f EqSet=%.1f EqMax=%.1f",
           usecs,
           VBATT_ADC_VOLTS(Chgr.config.battery_recipe.cv_vsetpoint   ) ,  
           VBATT_ADC_VOLTS(Chgr.config.battery_recipe.cv_volts_max   ) ,  
           VBATT_ADC_VOLTS(Chgr.config.battery_recipe.float_vsetpoint) ,
           VBATT_ADC_VOLTS(Chgr.config.battery_recipe.float_volts_max) ,
           VBATT_ADC_VOLTS(Chgr.config.battery_recipe.eq_vsetpoint   ) ,  
           VBATT_ADC_VOLTS(Chgr.config.battery_recipe.eq_volts_max   )   ); 
    // usec=862  Test#9 BattRcp CvSet=0.1 CvVMax=0.1 FltSet=0.1 FltMax=0.1 EqSet=0.1 EqMax=0.1

    // Timing Test #10
    adc1 = 4*10;
    adc2 = 4*20;
    adc3 = 4*30;
    adc4 = 4*40;
    adc5 = 4*50;
    f1 = f2 = f3 = f4 = f5 = 0.;
    nLoops = 100000;
    startTicks = GetSysTicks();
    for (i=0; i<nLoops; i++)
    {
        f1 +=    VBATT_ADC_VOLTS(adc1);
        f2 += IMEAS_INV_ADC_AMPS(adc2);
        f3 += IMEAS_CHG_AMPS_ADC(adc3);
        f4 +=     ILINE_AMPS_ADC(adc4);
        f5 +=      VAC_VOLTS_ADC(adc5);
    } // for
    endTicks = GetSysTicks();
    usecs = (1000*(endTicks - startTicks))/nLoops;
    LOG(SS_CHG, SV_INFO, "usec=%lu  Test#9 f1=%.1f f2=%.1f f3=%.1f f4=%.1f f5=%.1f",
                            usecs, f1, f2, f3, f4, f5);
    // usec=30  Test#9 f1=80483.7 f2=243775.0 f3=425663040.0 f4=153643632.0 f5=36845468.0

    //  Timing Results
    // usec=83   LOG Timing Test#1
    // usec=111  LOG Timing Test#2 i16=%d (12345)
    // usec=224  LOG Timing Test#3 i32=%ld  (987654321)
    // usec=348  LOG Timing Test#4 fval=%f  (3.141593)
    // usec=211  LOG Timing Test#5 fval=%2.1f  (2.2)
    // usec=108  LOG Timing Test#6 i16=i16toa()  (12345)
    // usec=313  LOG Timing Test#7 i32=i32toa()  (987654321)
    // usec=161  LOG Timing Test#8 fval=ftoa2()  (3.1)
    // usec=862  Test#9 BattRcp CvSet=0.1 CvVMax=0.1 FltSet=0.1 FltMax=0.1 EqSet=0.1 EqMax=0.1
    // usec=30   Test#10 f1=80483.7 f2=243775.0 f3=425663040.0 f4=153643632.0 f5=36845468.0
}
#endif // DEBUG_BENCHMARK

#endif // OPTION_NO_LOGGING

// <><><><><><><><><><><><><> log.c <><><><><><><><><><><><><><><><><><><><><><>
