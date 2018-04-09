// <><><><><><><><><><><><><> task_dev.c <><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//  Copyright(C) 2018 - Sensata Technologies, Inc.  All rights reserved.
//
//  $Workfile: task_dev.c $
//  $Author: A1021170 $
//  $Date: 2018-04-03 16:56:15-05:00 $
//  $Revision: 15 $ 
//
//  LPC Device Task
//
//-----------------------------------------------------------------------------

// -------
// headers
// -------
#include "options.h"    // must be first include
#include "hw.h"
#include "dsPIC_serial.h"
#include "analog.h"
#include "device.h"
#include "charger.h"

// ----------------
// Input Debouncing
// ----------------
typedef union
{
	struct
	{
		unsigned all_flags	: 3;   // used to compare all bits
		unsigned unused     : 13;
	};
	struct
	{
		unsigned aux_input	: 1;
		unsigned pushbutton	: 1;
		unsigned remote_on	: 1;
		unsigned unused2 	: 13;
	};
} DEBOUNCER_t;
	

// -----------
// Local Data
// -----------

// input debouncing
static int16_t      _push_button_state = 0;
static DEBOUNCER_t  _debouncer_last;   // last   input debounce states
static DEBOUNCER_t  _debouncer_now;    // current input debounce states


//-----------------------------------------------------------------------------
//                      V R E G   1 5
//-----------------------------------------------------------------------------
//	Check the Housekeeping Supply
//	Note that Device error vreg15_invalid is an ERROR flag. Errors WILL 
//	affect (halt or prevent) the operation of the Inverter or Charger.
//-----------------------------------------------------------------------------
	
// check the state of the 15 volt regulator
void dev_CheckVReg15(int8_t reset)
{
	#define VREG_QUAL_COUNT		((int16_t)(20))	//	milliseconds
	#define VREG_SHUTDOWN_COUNT	((int16_t)(2))	//	milliseconds

	static int16_t vreg15_counter = 0;
    static int16_t vreg15_state   = 0;
    
    
    if(reset)
    {
        vreg15_counter = 0;
        vreg15_state   = 0;
        Device.error.vreg15_invalid = 0;
    }
    
  #if defined(OPTION_UL_TEST_CONFIG)
	Device.error.vreg15_invalid = 0;
  #else

    // execute state machine
	switch (vreg15_state)
	{
	default :
	case 0:  // Initialize to Invalid
	    // ------------------------------------------------------------------
		//	Initialize assuming that VReg15 is low (NOT valid). Then wait for 
		//	VReg15 to become valid.
	    // ------------------------------------------------------------------
		Device.error.vreg15_invalid = 1;
		vreg15_counter = VREG_QUAL_COUNT;
		vreg15_state = 1;
		//	fall-thru
	
	case 1:	// wait for VReg15 to get valid.
	    // ------------------------------------------------------------------
		//	VReg15 must be valid (qualified) for a period of time (20-mSec).
		//	While VReg15 is low (invalid) keep resetting the timer. Let the 
		//	timer run when VReg15 is valid. When the timer expires, indicate 
		//	that VReg15 is valid (invalid = false) and move to the next state
	    // ------------------------------------------------------------------
		Device.error.vreg15_invalid = 1;
		if(VReg15Raw() < VREG15_LOW_SHUTDOWN)
        {
			vreg15_counter = VREG_QUAL_COUNT; // reset the counter 
        }
		else if (--vreg15_counter <= 0)
		{
			Device.error.vreg15_invalid = 0;
			vreg15_counter = VREG_QUAL_COUNT;
			vreg15_state++;
		}
		break;
	
	case 2: //  VReg15 IS valid 
		Device.error.vreg15_invalid = 0;
		if(VReg15Raw() > VREG15_LOW_SHUTDOWN)
        {
			vreg15_counter = VREG_QUAL_COUNT; // reset the counter 
        }
		else if (--vreg15_counter <= 0)
		{
			Device.error.vreg15_invalid = 1;
            vreg15_counter = VREG_QUAL_COUNT;
			vreg15_state++;
		}
		break;

	case 3:
	    // ---------------------------------------------------------------------
		//	We are latched off.  Exit this state if one of the following occurs: 
        //   1) This state machine is reset - when INV_REQUEST (E6) is toggled 
        //      off/on.
        //   2) AC-Line is valid (in this state) AND VReg15 is valid (qualified) 
        //      for a period of time (20-mSec).
	    // ---------------------------------------------------------------------
		Device.error.vreg15_invalid = 1;
		if(VReg15Raw() < VREG15_LOW_SHUTDOWN)
        {
			vreg15_counter = VREG_QUAL_COUNT; // reset the counter 
        }
		else if (--vreg15_counter <= 0)
		{
            vreg15_counter = 0;
            //  NOTE: Allow the the counter to run normally, but control the 
            //  flag and transition are based upon AC-Line Qualified.
            if(IsAcLineQualified())
            {
                Device.error.vreg15_invalid = 0;
                vreg15_counter = VREG_QUAL_COUNT;
                vreg15_state--;
            }
		}
		break;
	}
  #endif // OPTION_UL_TEST_CONFIG
}


// -------------------------------------------------------------------------
//              A / C    L I N E     Q U A L I F I C A T I O N
// -------------------------------------------------------------------------
//	The AC-Line Valid qualification timer is incorporated into the 
//	flag ac_line_qualified.  It will be set when the AC_LINE_VALID input is
//	active for the specified period of time.
// -------------------------------------------------------------------------

// check the state of the A/C power line
static void CheckAcLineState()
{
	static int16_t ac_line_state = 0; // 0=low, 1=high, 2=qualified
	
    // read the hardware line
	Device.status.ac_line_valid = AC_LINE_VALID();

    // execute state machine
	switch(ac_line_state)
	{
	default:
		ac_line_state = 0;
        // fall thru
		
	case 0: // AC_LINE_VALID is low, wait for it to go high.
		Device.status.ac_line_qual_timer_msec = Device.config.ac_line_qual_delay;
		Device.status.ac_line_qualified = 0;

		if (IsAcLineValid()) 
		{	
			ac_line_state = 1;
    		LOG(SS_SYS, SV_INFO, "AC Line qual delay = %u msec", Device.config.ac_line_qual_delay);
		}
		break;
		
	case 1: // 	AC_LINE_VALID is high, wait for timer elapsed, then qualified.
			//	Else if AC_LINE_VALID goes low before timer elapses, then retry.
		if (!IsAcLineValid()) 
		{
			ac_line_state = 0;
		}
		else if (--Device.status.ac_line_qual_timer_msec <= 0) 
		{
			Device.status.ac_line_qualified = 1;
			ac_line_state = 2;
		}
		break;
		
	case 2: // AC-Line is qualified, monitor AC_LINE_VALID
		if (!IsAcLineValid()) 
		{
			Device.status.ac_line_qualified = 0;
			ac_line_state = 0;
		}
		break;
	} // switch ac_line_state
}

//-----------------------------------------------------------------------------
//                 D E B O U N C E     I N P U T S 
//-----------------------------------------------------------------------------
//	AUX_INPUT_ACTIVE()  = INV_I-LOCK 					= RD3 (dsPIC pin-51)	
//	PUSHBUTTON_ACTIVE() = INV_REQUEST = ~REMOTE_ON/OFF  = RD4 (dsPIC pin-52)
//	REMOTE_ON_ACTIVE()  = REMOTE_ON = INV-ON/OFF		= RD6 (dsPIC pin 54)
//-----------------------------------------------------------------------------

// This function is called once-per-millisecond
static void DebounceInputs()
{
	#define DEBOUNCE_COUNT  (20)  	// Milliseconds that signals must be stable.
	
    static int16_t debounce_counter = 0;
	DEBOUNCER_t curr;
	
    // read inputs
	curr.aux_input	= AUX_INPUT_ACTIVE();
	curr.pushbutton = PUSHBUTTON_ACTIVE(); 
	curr.remote_on  = REMOTE_ON_ACTIVE();
	
    // any change from last time through
	if(_debouncer_last.all_flags != curr.all_flags)
	{   
        // change occurred
		_debouncer_last = curr;
		debounce_counter = 0; // reset counter
	}
	else if(++debounce_counter > DEBOUNCE_COUNT)
	{
        // no change in required time; use new settings
		_debouncer_now = _debouncer_last;
		debounce_counter = DEBOUNCE_COUNT;
	}
	Device.status.remote_on = _debouncer_now.remote_on;
    Device.status.aux_input = _debouncer_now.aux_input;
}

//-----------------------------------------------------------------------------
//	    I N V E R T E R   R E Q U E S T   P R O C E S S I N G
//-----------------------------------------------------------------------------
//	This code sets the value of Device.status.inv_request
//
//	Inverter Request is set according to the state of the Pushbutton 
//	and remote inputs and their functional configurations
//
//	The 'Pushbutton' and the 'Remote On/Off' (Purple-Wire) control the
//	Inverter. These inputs do interact. Those interactions are handled
//	here. The output of this processing is the state of the inv_request
//	flag.  inv_request is true if the inverter should be running, else
//	it is false.
//
//	If the Remote Mode is 'Snap' then the Pushbutton is disabled. 
//	Else, inv_request toggles each time the Pushbutton is released.
//	If the Remote Mode is 'Momentary' its function will parallel the 
//	Pushbutton - inv_request toggles each time the contacts are opened.
//-----------------------------------------------------------------------------

static void InverterRequestProcess()
{
	if(!Device.config.inv_enabled)
	{
		Device.status.inv_request = 0;
        return;
	}
	
	if(!Device.config.pushbutton_enabled)
		_debouncer_now.pushbutton = 0;

    // execute state machine
	switch(Device.config.remote_mode)
	{
	case REMOTE_DISABLED :
		//  Pushbutton state machine: 
		switch (_push_button_state) 
		{
		default :
		case 0 :
			//  Wait for pushbutton to be pressed
			if (_debouncer_now.pushbutton) 
				_push_button_state = 1;
			break;
			
		case 1 :
			//  Wait for pushbutton to be released
			if (!_debouncer_now.pushbutton) 
			{
				//	Toggle each time the button is released.
				Device.status.inv_request ^= 1;
				_push_button_state = 0;
			}
			break;
		}
		break;

	case REMOTE_SNAP :
		//	Pushbutton disabled; set inv_request based upon Remote On/Off
		Device.status.inv_request = IsRemoteOn() ? 1:0;
		break;
		
	case REMOTE_MOMENTARY :
		//	Don't allow states to change if the inverter is disabled.
		if(!IsInvEnabled())
		{
			break;
		}
		//  Pushbutton state machine: 
		switch (_push_button_state) 
		{
		default :
			serial_putc('?');

		case 0 :
			//  Wait for either pushbutton or remote to be pressed(active)
			Device.status.inv_request = 0;
			if (_debouncer_now.pushbutton || _debouncer_now.remote_on) 
				_push_button_state = 1;
			break;
			
		case 1 :
			//  Wait for both pushbutton and remote to be released(inactive)
			Device.status.inv_request = 1;
			if (!_debouncer_now.pushbutton && !IsRemoteOn()) _push_button_state = 2;
			break;

		case 2 :
			//  Wait for either pushbutton or remote to be pressed(active)
			Device.status.inv_request = 1;
			if (_debouncer_now.pushbutton || IsRemoteOn()) _push_button_state = 3;
			break;
		
		case 3 :
			//  Wait for both pushbutton and remote to be released(inactive)
			Device.status.inv_request = 0;
			if (!_debouncer_now.pushbutton && !IsRemoteOn()) _push_button_state = 0;
			break;
		}
		break;

	case REMOTE_CUSTOM :
		//	1) E6 (Purple-Wire) will have priority.  
		//	 1a) If E6 is high: unit will be on, pressing the pushbutton 
		//		 has no effect.
		//	 1b) If E6 is low and unit is off, pressing the Pushbutton will 
		//		 turn the unit ON.
		//	 1c) If E6 is low and unit is ON, pressing the Pushbutton will 
		//		 turn the unit off.
		//	2) When unit is off, it will be in low-power state.
		//  3) Auxiliary tab is not connected.
		
		if(IsRemoteOn()) 
		{
			//	If E6 is high: unit will be on
			Device.status.inv_request = 1;
			_push_button_state = 0;
		}
		else	
		{
			//	If E6 is low 
			switch (_push_button_state) 
			{
			default :
			case 0 :
				//	Initial state is unit off (assumed)
				//	pressing the Pushbutton will turn the unit ON
				Device.status.inv_request = 0;
				if (_debouncer_now.pushbutton)
					_push_button_state++; 
				break;
				
			case 1 :
				//	wait for pushbutton to be released (inactive)
				Device.status.inv_request = 1;
				if (!_debouncer_now.pushbutton)
					_push_button_state++;
				break;
				
			case 2 :
				//	pressing the Pushbutton will turn the unit off
				Device.status.inv_request = 1;
				if (_debouncer_now.pushbutton)
					_push_button_state++; 
				break;
				
			case 3 :
				//	wait for pushbutton to be released (inactive)
				Device.status.inv_request = 0;
				if (!_debouncer_now.pushbutton)
					_push_button_state = 0;
				break;
            } // switch _push_button_state
        }
		break;
    } // switch Device.config.remote_mode
}

//-----------------------------------------------------------------------------
//	    C H A R G E R   R E Q U E S T   P R O C E S S I N G
//-----------------------------------------------------------------------------
//	NOTE: This is a little more elaborate than it needs to be for LPC because of
//	limitations of the LPC Transfer Relay hardware. (These limitations were not 
//	fully realized until after this design was in development.) 
//
//	The design implemented here assumes independent control of the Charger and 
//	Transfer Relays, such that it may implement a 'Bypass' mode if/when 
//	supported by hardware. It causes no conflict with the operation of the LPC, 
//	so it is left as-is, for future implementation.
//
//	This logic to determines if the Charger/Transfer Relay and Charger should be
//	active and sets requests accordingly. MainControl processes these requests.
//
//	The status affected by this code is summarized below:
//		Device.status.relay_request = Charger/Transfer relay should be active
//		Device.status.chgr_request = charger should be running
//		
//	chgr_request requires relay_request - it is assumed the Charger/Transfer
//	Relay must be active before the Charger can run.
//	Charger operation will preempt Inverter operation.
//-----------------------------------------------------------------------------
	
static void ChargerRequestProcessing()
{
	if(IsAcLineQualified())
	{
		//	Valid AC-Line has priority - use shore-power when available.
		if(Device.config.pass_thru_enabled)
		{
			Device.status.relay_request = 1;
			
			//	The [LPC] Charger cannot run unless the charge/transfer 
			//	relay is closed.
			if(IsChgrEnabled())
				Device.status.chgr_request = 1;
			else
				Device.status.chgr_request = 0;
		}
	}
	else 
	{
		Device.status.relay_request = 0;
		Device.status.chgr_request = 0;
	}
}

//-----------------------------------------------------------------------------
//	A U X I L I A R Y   I N P U T   P R O C E S S I N G 
//-----------------------------------------------------------------------------
//	The 'Auxiliary' input allows inverter/charger/relay functions to be
//	overridden depending on mode. Brief use case embedded below describe 
//	this functionality and applicability.
//-----------------------------------------------------------------------------

static void ProcessAuxInput()
{
    // functionality depends upon configured aux mode
	switch(Device.config.aux_mode)
	{
	case AUX_DISABLED:
		//	Don't care - However, Auxiliary input hardware will control the 
		//	Housekeeping supply
		break;
		
	case AUX_RV:
		//	RV USE CASE: Auxiliary input is connected to a 'Master' switch. 
		//	When the Master switch is off, the inverter, charger and relay 
		//	should be off.
		if(!_debouncer_now.aux_input)
		{
			//	Auxiliary input is high(active), override requests.
			Device.status.inv_request   = 0;
			Device.status.chgr_request  = 0;
			Device.status.relay_request = 0;
		}
		break;
		
    // When the Aux tab is set to RV or Utility and AC is qualified,
    // 1) If Aux is high (active): transfer relay should be on, with PWM chrg drive.
	
	case AUX_UTILITY:
		//	UTILITY USE CASE: Goal is to assure that the inverter will not 
		//	automatically start and drain the battery. Charger and relay 
		//	will function normally. Auxiliary input is connected to vehicle 
		//	ignition. 
		if(!_debouncer_now.aux_input)
		{
			//	Auxiliary input is low(inactive), override inv_request
			Device.status.inv_request = 0;
		}
		break;
		
	case AUX_CONTROL:
		//	WIRED-CONTROL USE CASE: This feature was requested by Volta.
		//	The Auxiliary Input controls the charger. chgr_request is cleared 
		//	when the Auxiliary Input is low. However, the Charger/Transfer Relay 
		//	will remain engaged as long as AC is valid.
		
		//	When the Aux tab is set to control and AC is qualified, then the
		//	transfer/charger relay should be on, and:
		//	1) If Aux is low (inactive), then the charger is inactive.
		//	2) If Aux is high (active), then the charger is active

		//	NOTE: Device status relay request and Device status chgr_request
		//	are set in the "CHARGER ENABLE PROCESSING" section above. So, 
		//	here we just want to clear Device status chgr_request if the 
		//	Aux input is low.
		
		if(!_debouncer_now.aux_input)
		{
			Device.status.chgr_request = 0;
		}
		break;
    } // switch aux mode
}

//-----------------------------------------------------------------------------
//  TASK_devio_Driver()
//-----------------------------------------------------------------------------
//  is called once-per-millisecond.
//  debounces digital inputs.
//  implements a state machine to latch the remote-on input.
//-----------------------------------------------------------------------------

void TASK_devio_Driver(void)
{
    //  LPC does not have xfer relay
    Device.status.chgr_relay_active = IS_XFER_CHG_RELAY_ON();
    Device.status.xfer_relay_active = IS_XFER_CHG_RELAY_ON();

    // the order of these is important
    dev_CheckVReg15(0);
    CheckAcLineState();
    DebounceInputs();
    // process the inputs
    InverterRequestProcess();
    ChargerRequestProcessing();
    ProcessAuxInput();

    //  Validate and apply the Branch Circuit Rating.
    if (ChgrCfgBcrAmps() > MAX_BRANCH_CIRCUIT_AMPS)
    {
        Chgr.config.bcr_int8 = MAX_BRANCH_CIRCUIT_AMPS;
    }
    
    if(ChgrCfgBcrAmps() > 0) 
    {
        Device.status.chgr_enable_bcr = 1;
        Chgr.config.bcr_adc = ILINE_AMPS_ADC(ChgrCfgBcrAmps()) - BCR_SAFETY_A2D;
    }
    else
    {
        Device.status.chgr_enable_bcr = 0;
        Chgr.config.bcr_adc = 0;
    }
}

// <><><><><><><><><><><><><> task_dev.c <><><><><><><><><><><><><><><><><><><>
