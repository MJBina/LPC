// <><><><><><><><><><><><><> log.h <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//	Copyright(C) 2015 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
//  Logging routines output to serial port for debugging purposes
//	
//-----------------------------------------------------------------------------

#ifndef __LOG_H__    // include only once
#define __LOG_H__

// -------
// headers
// -------
#include "options.h"    // must be first include

// --------------------------
// Conditional Debug Compiles
// --------------------------

// comment out flag to not use
//#define DEBUG_BENCHMARK   1  // time LOG functions

// -----------------------------
// Auto Removal for build types
// -----------------------------
#if OPTION_NO_CONDITIONAL_DBG
  #undef DEBUG_BENCHMARK
#endif


// ------------------------------------------------------------------------
//  Safe Logging can be used to log info from interrupts (time delayed)
//  SLOG(SUBSYS, SEVERITY, TEXT, i1, i2, i3, i4);  // log 4 integer values
// 
//  need to define OPTION_SAFE_LOGGING in project
//  call SLOG_DUMP() periodically to dump queue
// ------------------------------------------------------------------------

// ------------------------------------------------------------------------
// CAUTION! logging floating values is time consuming
//    LOG(SS_SYS, SV_INFO, "vbat=%.3f", (float)vbatt);  // take 12 msecs
//  
//  Much faster to convert to millivolts and print as int32
//    LOG(SS_SYS, SV_INFO, "mvbat=%ld", VOLT_MVOLT(vbatt));
// ------------------------------------------------------------------------


// ----------
// subsystems
// ----------
typedef enum 
{
    SS_SYS = 0,     // system
    SS_CAN,         // CAN bus
    SS_CFG,         // configuration
    SS_CHG,         // charger
    SS_INV,         // inverter
    SS_J1939,       // J1939 CAN bus
    SS_SEN,         // Sensata commands
    SS_PWM,         // pulse-width-modulation
    SS_RVC,         // RV-CAN
    SS_TASKER,      // task runner
    SS_DAC,         // digital/analog converter
    SS_INVCMD,		// inverter command
    SS_CHGCMD,		// charger  command
    SS_NVM,         // non-volatile memory
    SS_ANA,         // analog
	SS_UI,			// User Interface (NP LCD)
    NUM_LOG_SUBSYS_t
} LOG_SUBSYS_t;

// --------
// severity
// --------
typedef enum 
{
    LOG_ALL = 0,	// log everything; dont use in LOG() macro; use in SetSeverity
    SV_INFO,		// informational
    SV_DBG, 		// debug
    SV_WARN,		// warning
    SV_ERR,			// error
    LOG_DISABLE		// disable; dont use in LOG() macro; use in SetSeverity
} LOG_SEVERITY_t;

#ifdef WIN32
    #define LOG
    #define LOG_CONFIG()			                _log_Config()
    #define LOG_DISABLE(SUBSYS)		                _log_Severity(SUBSYS, LOG_DISABLE)
    #define LOG_SEVERITY(SUBSYS, SEVERITY)	        _log_Severity(SUBSYS, SEVERITY)
    #define LOG_SEVERITY_ALL(SEVERITY)		        _log_SeverityAll(SEVERITY)
    #define LOGX(SUBSYS, SEVERITY, text, data, len)	_logx(SUBSYS, SEVERITY, text, data, len)
    #define LOG_TIMER_START()                       _log_TimerStart()
    #define LOG_TIMER_END()                         _log_TimerEnd()
    // safe logging
    #define SLOG(SUBSYS, SEVERITY, text, i1, i2, i3, i4)  _log_Safe(SUBSYS,SEVERITY,text,i1,i2,i3,i4)
    #define SLOG_DUMP()                                   _log_Safe_DumpQueue()
#else

//  Use "options.h" for enabling and disabling logging
#ifdef OPTION_NO_LOGGING
    // logging disabled; do nothing;   need brackets to maintain proper program flow
    #define LOG(SUBSYS, SEVERITY, format, ...)         { } 
    #define LOG_CONFIG()                               { }
    #define LOG_DISABLE(SUBSYS)                        { }
    #define LOG_SEVERITY(SUBSYS, SEVERITY)             { }
    #define LOG_SEVERITY_ALL(SEVERITY)                 { }
    #define LOGX(SUBSYS, SEVERITY, text, data, len)    { }
    #define LOG_TIMER_START()                          { }
    #define LOG_TIMER_END()                            { }
    // safe logging
    #define SLOG(SUBSYS, SEVERITY, text, i1,i2,i3,i4)  { }
    #define SLOG_DUMP()                                { }
#else
    // logging enabled; map to function calls
    #define LOG(SUBSYS, SEVERITY, format, ...)	    _log(GetSysTicks(), SUBSYS, SEVERITY, format, ##__VA_ARGS__)
    #define LOG_CONFIG()			                _log_Config()
    #define LOG_DISABLE(SUBSYS)		                _log_Severity(SUBSYS, LOG_DISABLE)
    #define LOG_SEVERITY(SUBSYS, SEVERITY)	        _log_Severity(SUBSYS, SEVERITY)
    #define LOG_SEVERITY_ALL(SEVERITY)		        _log_SeverityAll(SEVERITY)
    #define LOGX(SUBSYS, SEVERITY, text, data, len)	_logx(SUBSYS, SEVERITY, text, data, len)
    #define LOG_TIMER_START()                       _log_TimerStart()
    #define LOG_TIMER_END()                         _log_TimerEnd()
  // safe logging
  #ifdef OPTION_SAFE_LOGGING
    #define SLOG(SUBSYS, SEVERITY, text, i1, i2, i3, i4)  _log_Safe(SUBSYS,SEVERITY,text,i1,i2,i3,i4)
    #define SLOG_DUMP()                                   _log_Safe_DumpQueue()
  #else
    #define SLOG(SUBSYS, SEVERITY, text, i1, i2, i3, i4)  { }
    #define SLOG_DUMP()                                   { }
  #endif

#endif
#endif // WIN32

#define LOG_ENABLE      LOG_SEVERITY
#define LOG_ENABLE_ALL  LOG_SEVERITY_ALL


// -----------------------
// Timing of LOG routines
// -----------------------
#ifdef DEBUG_BENCHMARK
 extern uint8_t g_serial_DiscardData; // 0=normal, 1=dont transmit data (test mode)
 void _log_TimingTest(void);
#endif


// --------------------
// Function Prototyping
// --------------------
// Don't call these directly; LOG macros calls these
void _log(SYSTICKS, LOG_SUBSYS_t, LOG_SEVERITY_t, char *, ...);
void _log_Config(void);
void _log_Severity(LOG_SUBSYS_t, LOG_SEVERITY_t);
void _log_SeverityAll(LOG_SEVERITY_t severity);
void _logx(LOG_SUBSYS_t subsys, LOG_SEVERITY_t severity, char* preText, uint8_t *data, int16_t len);
void _log_Safe(LOG_SUBSYS_t subsys, LOG_SEVERITY_t severity, char* msg, int v1, int v2, int v3, int v4);
void _log_Safe_DumpQueue(void);
void _log_TimerStart(void);
void _log_TimerEnd(void);

#endif  //  __LOG_H__

// <><><><><><><><><><><><><> log.h <><><><><><><><><><><><><><><><><><><><><><>
