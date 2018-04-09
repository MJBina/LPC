// <><><><><><><><><><><><><> task_temp.c <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//  Copyright(C) 2017 - Sensata Technologies, Inc.  All rights reserved.
//
//  $Workfile: task_temp.c $
//  $Author: clark_dailey $
//  $Date: 2017-12-21 12:13:29-06:00 $
//  $Revision: 4 $ 
//
//  LPC Temperature Sensor task
//
//-----------------------------------------------------------------------------

// -------
// headers
// -------
#include "options.h"    // must be first include
#include "hw.h"
#include "device.h"
#include "inverter.h"
#include "batt_temp.h"

//-----------------------------------------------------------------------------
//  Temperature sensors are common to both the inverter and the charger,
//  so they are tested independent of operating mode.
//
//  High-Temp is tested on a one-millisecond interval, hence the High-Temp 
//  Count value is measured in Milliseconds.
//
//  High-temp is sensed by a thermal (pop) switch. The output of this switch 
//  will disable the drivers when the high-temp switch is active. This will 
//  cause a feedback fault. 
//
//  We need to be able to recover from a high-temp condition and automatically
//  restart. Therefore, we need to clear the feedback fault. If a feedback 
//  fault does exist, it will be caught again.
//-----------------------------------------------------------------------------

void TASK_TempSensors(void)
{
	#define HIGH_TEMP_DEBOUNCE_MSEC	(250)   // temp sensors debounce time (milliseconds)
	static int16_t  hs_msec_cntr   = 0;     // heat sink   over-temp debounce down-counter
	static int16_t  xfrm_msec_cntr = 0;     // transformer over-temp debounce down-counter
	
	//	Heatsink and Transformer Over-Temperature: This is a thermal (bi-metal) 
	//	switch. It has been shown that the contacts may bounce, (Ref: email 
	//	from Gary 2017-04-10) which can result in damage to the system hardware 
	//	if the transformer is being driven. Firmware must stop driving the FETs
	//	as soon as the switch opens.
	//	If the switch has opened, debounce the switch for 100mSec before the
	//	over-temp condition is cleared.
	
	// check heat sink over-temp 
    if (HTSK_HTEMP_ACTIVE()) 
    {
		// heat-sink is over-temp
        hs_msec_cntr = HIGH_TEMP_DEBOUNCE_MSEC;
		Device.error.hs_temp = 1;
    }
    else if(--hs_msec_cntr <= 0)
    {
		// heat sink not over-temp for debounce msecs
        hs_msec_cntr = 0;
        Device.error.hs_temp = 0;
    }

	// check transformer over-temp
    if (XFMR_HTEMP_ACTIVE()) 
    {
		// transformer is over-temp
        xfrm_msec_cntr = HIGH_TEMP_DEBOUNCE_MSEC;
		Device.error.xfmr_ot = 1;
    }
    else if(--xfrm_msec_cntr <= 0)
    {
		// transformer is not over-temp for debounce msecs
        xfrm_msec_cntr = 0;
		Device.error.xfmr_ot = 0;
    }

	//	Clear the Feedback fault:
	//	If either XFMR_HTEMP or HTSK_HTEMP are active the FET drivers are 
	//	disabled and Firmware has no control of the FETs.  This may appear 
	//	as a Feedback-related error since adjustment of the PWM output does 
	//	not produce an observable change.  
	//	LPC HARDWARE DETAILS: 
	//		'XFMR_HTEMP' is connected to 'LOW1 SIDE' driver shutdown.
	//		'HTSK_HTEMP' is connected to 'LOW2 SIDE' driver shutdown.
    if(IsDevOverTemp())
	{
		Inv.error.feedback_problem = 0; 
	}
	
	// Battery Temperature Sensor
    BattTemp_Evaluate();
}
 
// <><><><><><><><><><><><><> task_temp.c <><><><><><><><><><><><><><><><><><><><><><>
