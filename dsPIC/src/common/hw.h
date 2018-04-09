// <><><><><><><><><><><><><> hw.h <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//  Copyright(C) 2013 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
//  Micro processor Hardware Specific defines such as PINS
//
//-----------------------------------------------------------------------------

#ifndef _HW_H_    // include only once
#define _HW_H_

// -------
// headers
// -------
#include "options.h"    // must be first include


#if IS_PCB_NP || IS_PCB_BDC

  // -------------------------------------------------------------------------- 
  //                            NP or BDC
  // -------------------------------------------------------------------------- 
  #if(defined(OPTION_HAS_CHGR_EN_SWITCH))
    #define CHARGER_HW_ENABLE_ACTIVE()      ((0==PORTBbits.RB3)?1:0)  // 0=switch is open, 1=switch is closed
  #else
  	#define CHARGER_HW_ENABLE_ACTIVE()        (1)   // simulate switch on (closed)
  #endif

    // -------------------------------------------
    // Programming Header pins used for debugging
    //
    //   O  O  O  O  O  O
    //      |  |        |
    //      |  |        +--- PICKit3 Pin 1 (V)
    //      |  +-- RB7
    //      + --- RB6                  ^
    // -------------------------------------------

    // programming header  J6B
    #define IS_RB6_ON()        	    (1==LATBbits.LATB6)
    #define RB6_SET(onoff)          (LATBbits.LATB6 = ((onoff)?1:0))
    #define RB6_TOGGLE()            RB6_SET(!IS_RB6_ON())
    #define DEBUG_RB6_LO()          RB6_SET(0)
    #define DEBUG_RB6_HI()          RB6_SET(1)

    // programming header  J6C
    #define IS_RB7_ON()        	    (1==LATBbits.LATB7)
    #define RB7_SET(onoff)          (LATBbits.LATB7 = ((onoff)?1:0))
    #define RB7_TOGGLE()            RB7_SET(!IS_RB7_ON())
    #define DEBUG_RB7_LO()          RB7_SET(0)
    #define DEBUG_RB7_HI()          RB7_SET(1)

	//  RB12 (pin-27)               J2F header
    #define IS_RB12_ON()           	(1==LATBbits.LATB12)
    #define RB12_SET(onoff)        	(LATBbits.LATB12 = ((onoff)?1:0))
    #define RB12_TOGGLE()           RB12_SET(!IS_RB12_ON())
	
    //  RB13 (pin-28)               J2G header
    #define IS_RB13_ON()           	(1==LATBbits.LATB13)
    #define RB13_SET(onoff)        	(LATBbits.LATB13 = ((onoff)?1:0))
    #define RB13_TOGGLE()           RB13_SET(!IS_RB13_ON())
	
    //  RB14 (pin-29) STATUS_RED_LED (0=inactive)
    #define IS_LED_RED_ON()        	(1==LATBbits.LATB14)
    #define LED_RED_SET(onoff)     	(LATBbits.LATB14 = ((onoff)?1:0))
    #define LED_RED_ON()            LED_RED_SET(1)
    #define LED_RED_OFF()           LED_RED_SET(0)
    #define LED_RED_TOGGLE()        LED_RED_SET(!IS_LED_RED_ON())

    //  RB15/AN15 (pin-30)  XFMR_HTEMP
    #define XFMR_OT_ACTIVE()        (1==PORTBbits.RB15)

    //  RD6 (pin-54)        FAN_SWITCH
    #define FAN_SWITCH_ACTIVE()     (1==PORTDbits.RD6)

    //  RD9/INT2 (pin-43)
    #define OVL_A_ACTIVE()          (0==PORTDbits.RD9)  // FET deck #1

    //  RD8/INT1 (pin-42)
    #define OVL_B_ACTIVE()          (0==PORTDbits.RD8)  // FET deck #2

    //  RD4/CN13 (pin-52)   <INV_REQUEST>
    #define REMOTE_ON_ACTIVE()      (0==PORTDbits.RD4)
    
    //  RF6 (pin-35)        120V_LINE_VALID
    #if IS_DEV_CONVERTER
       #define AC_LINE_VALID()         (0) // dc charger has no ac line
    #else
     #ifdef OPTION_HAS_CHARGER
       #define AC_LINE_VALID()         (1==PORTFbits.RF6)    // has charger; read hardware
     #else
       #define AC_LINE_VALID()         (0) // no charger; ac line is never valid
     #endif
    #endif

    //  RC13 (pin-47)
    #if IS_PCB_BDC
       #define AC_LINE_PHASE()         (0) // not available
    #else
       #define AC_LINE_PHASE()         (1==PORTCbits.RC13)
    #endif

    //  RC14 (pin-48)       TP36 
    #define IS_TP36_ON()            (1==LATCbits.LATC14)
    #define TP36_SET(onoff)         (LATCbits.LATC14 = ((onoff)?1:0))
    #define DEBUG_TP36_HI()          TP36_SET(1)
    #define DEBUG_TP36_LO()          TP36_SET(0)
    #define TP36_TOGGLE()            TP36_SET(!IS_TP36_ON())

    //  RD5 (pin-53) SSR_DRIVER
    #define IS_SSR_DRIVE_ON()       (1==LATDbits.LATD5)
    #define SSR_DRIVE_ON()       	(LATDbits.LATD5 = 1)
    #define SSR_DRIVE_OFF()      	(LATDbits.LATD5 = 0)
    #define SSR_DRIVE_SET(onoff)   	(LATDbits.LATD5 = ((onoff)?1:0))

    //  RD7 (pin-55) 120V_CHARGE_RELAY
    #define IS_CHARGE_RELAY_ON()    (1==LATDbits.LATD7)
    #define CHARGE_RELAY_ON()       (LATDbits.LATD7 = 1)
    #define CHARGE_RELAY_OFF()      (LATDbits.LATD7 = 0)
    
    //  RD10 (pin-44)               J2B header
  #if IS_PCB_BDC
    #define IS_RD10_ON()           (0) // not available on BDC
    #define RD10_SET(onoff)        
    #define RD10_TOGGLE()          
  #else
    #define IS_RD10_ON()           (1==LATDbits.LATD10)
    #define RD10_SET(onoff)        (LATDbits.LATD10 = ((onoff)?1:0))
    #define RD10_TOGGLE()           RD10_SET(!IS_RD10_ON())
  #endif
	
    // STATUS_GRN_LED
  #if IS_PCB_BDC
    #define LED_GRN_PIN            LATDbits.LATD0 // RD0 (pin-46) STATUS_GRN_LED (0=inactive)
  #else
    #define LED_GRN_PIN            LATEbits.LATE4 // RE4 (pin-64) STATUS_GRN_LED (0=inactive)
  #endif
    #define IS_LED_GRN_ON()        (1==LED_GRN_PIN)
    #define LED_GRN_SET(onoff)     (LED_GRN_PIN = ((onoff)?1:0))
    #define LED_GRN_ON()            LED_GRN_SET(1)
    #define LED_GRN_OFF()           LED_GRN_SET(0)
    #define LED_GRN_TOGGLE()        LED_GRN_SET(!IS_LED_GRN_ON())

	//	RE6	(pin-02) TP50
  #if !IS_PCB_BDC
    #define IS_TP50_ON()            (1==LATEbits.LATE6)
    #define TP50_SET(onoff)         (LATEbits.LATE6 = ((onoff)?1:0))
    #define DEBUG_TP50_HI()          TP50_SET(1)
    #define DEBUG_TP50_LO()          TP50_SET(0)
    #define TP50_TOGGLE()            TP50_SET(!IS_TP50_ON())
  #endif
	
	//	RE7	(pin-02) TP51
  #if !IS_PCB_BDC
    #define IS_TP51_ON()            (1==LATEbits.LATE7)
    #define TP51_SET(onoff)         (LATEbits.LATE7 = ((onoff)?1:0))
    #define DEBUG_TP51_HI()          TP51_SET(1)
    #define DEBUG_TP51_LO()          TP51_SET(0)
    #define TP51_TOGGLE()            TP51_SET(!IS_TP51_ON())
  #endif
	
	//	RB10 (pin-23) TP52
    #define IS_TP52_ON()            (1==LATBbits.LATB10)
    #define TP52_SET(onoff)         (LATBbits.LATB10 = ((onoff)?1:0))
    #define DEBUG_TP52_HI()          TP52_SET(1)
    #define DEBUG_TP52_LO()          TP52_SET(0)
    #define TP52_TOGGLE()            TP52_SET(!IS_TP52_ON())
	
    //  RD3 pin-51  KEEP_ALIVE
    #define IS_KEEP_ALIVE_ON()      (1==LATDbits.LATD3)
    #define KEEP_ALIVE_ON()         (LATDbits.LATD3 = 1)
    #define KEEP_ALIVE_OFF()        (LATDbits.LATD3 = 0)

    //  RG9  (pin-8)  TRANS_RELAY
    #define IS_TRANS_RELAY_ON()     (1==LATGbits.LATG9)
    #define TRANS_RELAY_ON()        (LATGbits.LATG9 = 1)
    #define TRANS_RELAY_OFF()       (LATGbits.LATG9 = 0)

    // Fan Control
  #if IS_PCB_BDC
    #define FAN_CTRL_ON()           (LATFbits.LATF6 = 1)
    #define FAN_CTRL_OFF()          (LATFbits.LATF6 = 0)
  #else
    #define FAN_CTRL_ON()           (LATEbits.LATE5 = 1)
    #define FAN_CTRL_OFF()          (LATEbits.LATE5 = 0)
  #endif

#elif IS_PCB_LPC
    // -------------------------------------------------------------------------- 
	//                             L P C
    // -------------------------------------------------------------------------- 
	//	[2016-11-11] The latest version of the LP/LPC fiberglass is 170165 Rev-A
	//	There are a few minor differences from the prior version.  
	//	170165 Rev-A superceeds prior hardware.
    // -------------------------------------------------------------------------- 

    #define AC_LINE_PHASE()     (0)     //  NOT available

    // -------------------------------------------
    // Programming Header pins used for debugging
    //
    //   O  O  O  O  O  O
    //      |  |        |
    //      |  |        +--- PICKit3 Pin 1 (V)
    //      |  +-- RB7
    //      + --- RB6                  ^
    // -------------------------------------------

    // programming header  J6B
    #define IS_RB6_ON()        	(1==LATBbits.LATB6)
    #define RB6_SET(onoff)      (LATBbits.LATB6 = ((onoff)?1:0))
    #define RB6_TOGGLE()        RB6_SET(!IS_RB6_ON())
    #define DEBUG_RB6_LO()      RB6_SET(0)
    #define DEBUG_RB6_HI()      RB6_SET(1)

    // programming header  J6C
    #define IS_RB7_ON()        	(1==LATBbits.LATB7)
    #define RB7_SET(onoff)      (LATBbits.LATB7 = ((onoff)?1:0))
    #define RB7_TOGGLE()        RB7_SET(!IS_RB7_ON())
    #define DEBUG_RB7_LO()      RB7_SET(0)
    #define DEBUG_RB7_HI()      RB7_SET(1)

    #define DEBUG_TP15_LO()     (LATBbits.LATB9 = 0)	
    #define DEBUG_TP15_HI()     (LATBbits.LATB9 = 1)
	
    #define BIAS_BOOSTING_ACTIVE()      (LATBbits.LATB14 = 1)
    #define BIAS_BOOSTING_INACTIVE()    (LATBbits.LATB14 = 0)
    
    #define DEBUG_TP23_LO()     (LATCbits.LATC14 = 0)
    #define DEBUG_TP23_HI()     (LATCbits.LATC14 = 1)
    
    #define DEBUG_TP22_LO()     (LATDbits.LATD1 = 0)
    #define DEBUG_TP22_HI()     (LATDbits.LATD1 = 1)

    #define AUX_INPUT_ACTIVE()  (1==PORTDbits.RD3)
    
    #define PUSHBUTTON_ACTIVE() (0==PORTDbits.RD4)
    
    #define KEEP_ALIVE_ON()     (LATDbits.LATD5 = 1)
    #define KEEP_ALIVE_OFF()    (LATDbits.LATD5 = 0)
    
    #define REMOTE_ON_ACTIVE() 	(1==PORTDbits.RD6)

    #define DEBUG_TP18_LO()     (LATDbits.LATD7 = 0)
    #define DEBUG_TP18_HI()     (LATDbits.LATD7 = 1)

  #ifdef OPTION_HAS_CHARGER
    #define AC_LINE_VALID()     (1==PORTDbits.RD9)    // read hardware
  #else
    #define AC_LINE_VALID()     (0) // no charger: ac line is never valid
  #endif

    #define XFMR_HTEMP_ACTIVE() (1==PORTDbits.RD10)

    #define HTSK_HTEMP_ACTIVE() (1==PORTDbits.RD11)
                         
    #define LED_RMT_FAULT_ON()  (LATEbits.LATE4 = 1)
    #define LED_RMT_FAULT_OFF() (LATEbits.LATE4 = 0)

    #define FAN_CTRL_ON()       (LATEbits.LATE5 = 1)
    #define FAN_CTRL_OFF()      (LATEbits.LATE5 = 0)

    #define LED_GRN_PIN         (LATEbits.LATE6)
    #define LED_GRN_SET(onoff)  (LED_GRN_PIN = (onoff)?1:0)
    #define LED_GRN_TOGGLE()     LED_GRN_SET(!LED_GRN_PIN)
    #define LED_GRN_ON()         LED_GRN_SET(1)
    #define LED_GRN_OFF()        LED_GRN_SET(0)

    #define LED_RED_PIN         (LATEbits.LATE7)
    #define LED_RED_SET(onoff)  (LED_RED_PIN = (onoff)?1:0)
    #define LED_RED_TOGGLE()     LED_RED_SET(!LED_RED_PIN)
    #define LED_RED_ON()         LED_RED_SET(1)
    #define LED_RED_OFF()        LED_RED_SET(0)

  #ifdef OPTION_HAS_CHARGER
    #define IS_XFER_CHG_RELAY_ON()  (1==LATGbits.LATG9)
    #define XFER_CHG_RELAY_ON()  (LATGbits.LATG9 = 1)
    #define XFER_CHG_RELAY_OFF() (LATGbits.LATG9 = 0)
  #else
    #define IS_XFER_CHG_RELAY_ON()	(0)
    #define XFER_CHG_RELAY_ON()
    #define XFER_CHG_RELAY_OFF()
  #endif
	
#else
    #warning  OPTION_PCB_... is not defined in file "hw.h"!

#endif  //  IS_PCB_ type    

// -------------------------
// Scope Debugging Routines
// -------------------------
// not usable from an interrupt
void RB6_NumToScope(uint8_t num);
void RB7_NumToScope(uint8_t num);

// ------------------
// Common hw defines
// ------------------

//  Calculations relating to the system clock functions
//  Fcy == MIPS == (Fosc * PLLx)/4      4 Q-clocks per instruction
#define FCY 40000000

#define CPU_Reboot()  { asm("RESET"); } // need to shutdown any hardware first?

#endif // _HW_H_

// <><><><><><><><><><><><><> hw.h <><><><><><><><><><><><><><><><><><><><><><>

