// -----------------------------------------------------------------------------
//
//  Copyright(C) 2018 - Sensata Technologies, Inc.  All rights reserved.
//
//  $Workfile: signal_capture.c $
//  $Author: A1021170 $
//  $Date: 2018-01-16 10:57:50-06:00 $
//  $Revision: 1 $ 

//	This file was created to avoid unnecessary clutter in other source files. 
//	This code was originally included in charger_isr.c, and other files to 
//	facilitate debugging.
//
//	This tool was developed for use with the MPLABx debugger.  The intended use
//	is to insert calls to sigcap_arm() and sigcap_trigger() into source to 
//	capture an event.  The event being one or more signals.  The signals are 
//	stored in arrays.  The array data can be retrieved from the MPLABx debugger.
//	
//	"Retrieved" comprises exporting from MPLAB as a .CSV file and then importing
//	the data into MS Excel for analysis.
//
//	Because this is a DEBUG tool, it provides a mechanism for disabling it such 
//	that it has no impact on the code.
//
//	The intended implementation of this code is to simply include the '.c' file 
//	in the code.  This is ugly!  However, it is expected that this code be used
//	only temporarily while debugging.  It is not intended that this code be 
//	used in production firmware.
//
// =============================================================================


#include "analog.h"
#include "inline.h"

//	#define SIGCAP_NO_STORAGE
#undef	SIGCAP_NO_STORAGE


#ifdef SIGCAP_NO_STORAGE

//	do-nothing functions

INLINE void sigcap_arm(void) {};

INLINE void sigcap_reset(void)	{};

INLINE void sigcap_save_data(void)	{};

INLINE void sigcap_trigger(void)	{};


#else

#pragma message "Signal Capture functions included in this build"

#define DEBUG_ARRAY_SIZE    400
//int16_t debug_Duty[DEBUG_ARRAY_SIZE];
int16_t debug_IMeas[DEBUG_ARRAY_SIZE];
int16_t debug_IMeasAvg[DEBUG_ARRAY_SIZE];
//int16_t debug_VAC[DEBUG_ARRAY_SIZE];

#define DEBUG_DELAY_CYCLES  2 * 30
int16_t debug_array_ix = 0;
int16_t debug_trigger = 0;
int8_t  debug_arm = 0;
int16_t debug_cycle_count = 0;


INLINE void sigcap_reset(void)
{
	debug_arm = 1;
	debug_trigger = 0;
	debug_cycle_count = 0;
	debug_array_ix = 0;
}


INLINE void sigcap_arm(void)
{
 	debug_arm = 1;
}


// -----------------------------------------------------------------------------
// 	The trigger is meant for the MPLAB debugger.  Set a breakpoint to trigger 
//	when the value of debug_trigger changes from zero to one.
//	This function is meant to be inserted into the code for use with a type-2
//	storage type (see description).
//
INLINE void sigcap_trigger(void)
{
 	debug_trigger = 1;		//	Set MPLAB breakpoint here!!!
}

// 	Type-1: Store data when armed, trigger when array(s) full.
//	Type-2: Continuously over-write data when armed, stop when triggered.
#define SIGCAP_STORAGE_TYPE		1

INLINE void sigcap_save_data(void)
{
	if(debug_arm)
	{
	  #if(1 == SIGCAP_STORAGE_TYPE)
		// 	Type-1: Store data when armed, trigger when array(s) full.
		//  Capturing the ADC data to an array so that we can use the 
		//  debugger to export the captured data for analysis
		if(debug_array_ix <= (DEBUG_ARRAY_SIZE))
		{
			//	This is the data being saved.
			debug_IMeas[debug_array_ix] = IMeasRaw();
			debug_IMeasAvg[debug_array_ix] = IMeasCycleAvg();
			//  debug_Duty[debug_array_ix] = tmp_dc;
			//  debug_VAC[debug_array_ix] = VacRaw();
		
			debug_array_ix++;
		}
		else
		{
			//  trigger here after array is full
			//	Set a Breakpoint on the line below.
			debug_trigger = 1;	//	Set MPLAB breakpoint here!!!
			debug_array_ix = 0;
		}
		
	  #elif(2 == SIGCAP_STORAGE_TYPE)
		//	Type-2: Continuously over-write data when armed, stop when triggered.
		if(debug_trigger)
		{
			debug_trigger = 0;	//	[OPTION] Set MPLAB breakpoint here!!!
			debug_arm = 0;
		}
		else
		{
			//	This is the data being saved.
			debug_IMeas[debug_array_ix] = IMeasRaw();
			debug_IMeasAvg[debug_array_ix] = IMeasCycleAvg();
			//  debug_Duty[debug_array_ix] = tmp_dc;
			//  debug_VAC[debug_array_ix] = VacRaw();

			if(++debug_array_ix >= (DEBUG_ARRAY_SIZE))
			{
				debug_array_ix = 0;
			}
		}
		
	  #endif	//	SIGCAP_STORAGE_TYPE
	}
}

#endif	//	SIGCAP_NO_STORAGE

// EOF
