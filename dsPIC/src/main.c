// <><><><><><><><><><><><><> main.c <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//  Copyright(C) 2017 - Sensata Technologies, Inc.  All rights reserved.
//
//  $Workfile: main.c $
//  $Author: clark_dailey $
//  $Date: 2018-04-02 10:25:39-05:00 $
//  $Revision: 439 $ 
//
//  LPC Main Start and Initialization
//
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
//     M I C R O P R O C E S S O R    C O N F I G    S E T T I N G S
//-----------------------------------------------------------------------------

#ifndef WIN32

// FBS
#pragma config BWRP  = WRPROTECT_OFF  // Boot Segment Write Protect (Boot Segment may be written)
#pragma config BSS   = NO_FLASH       // Boot Segment Program Flash Code Protection (No Boot program Flash segment)
#pragma config RBS   = NO_RAM         // Boot Segment RAM Protection (No Boot RAM)

// FSS
#pragma config SWRP  = WRPROTECT_OFF  // Secure Segment Program Write Protect (Secure Segment may be written)
#pragma config SSS   = NO_FLASH       // Secure Segment Program Flash Code Protection (No Secure Segment)
#pragma config RSS   = NO_RAM         // Secure Segment Data RAM Protection (No Secure RAM)

// FGS
#pragma config GWRP  = OFF            // General Code Segment Write Protect (User program memory is not write-protected)
#pragma config GSS   = OFF            // General Segment Code Protection (User program memory is not code-protected)

// FOSCSEL
#pragma config FNOSC = PRIPLL         // Oscillator Mode (Primary Oscillator (XT, HS, EC) w/ PLL)
#pragma config IESO  = OFF            // Two-speed Oscillator Start-Up Enable (Start up with user-selected oscillator)

// FOSC
#pragma config POSCMD   = XT          // Primary Oscillator Source (XT Oscillator Mode)
#pragma config OSCIOFNC = OFF         // OSC2 Pin Function (OSC2 pin has clock out function)
#pragma config FCKSM    = CSDCMD      // Clock Switching and Monitor (Both Clock Switching and Fail-Safe Clock Monitor are disabled)

// FWDT  PS23768:PR128 ~116 seconds;
//          PS16:PR32  ~7 milliseconds 116x(8/32768)x(32/128) = 0.0071  
#pragma config WDTPOST  = PS8         // Watchdog Timer Postscaler (1:N)
#pragma config WDTPRE   = PR32        // WDT Prescaler (1:N)
#pragma config WINDIS   = OFF         // Watchdog Timer Window (Watchdog Timer in Non-Window mode)
#pragma config FWDTEN   = OFF         // Watchdog Timer Enable (Watchdog timer always enabled)

// FPOR
#pragma config FPWRT    = PWR1        // POR Timer Value (Disabled)
#pragma config LPOL     = ON          // Low-side PWM Output Polarity (Active High)
#pragma config HPOL     = ON          // Motor Control PWM High Side Polarity bit (PWM module high side output pins have active-high output polarity)
#pragma config PWMPIN   = ON          // Motor Control PWM Module Pin Mode bit (PWM module pins controlled by PORT register at device Reset)

// FICD
#pragma config ICS      = PGD1        // Comm Channel Select (Communicate on PGC1/EMUC1 and PGD1/EMUD1)
#pragma config JTAGEN   = OFF         // JTAG Port Enable (JTAG is Disabled)

#endif // WIN32


// -------
// headers
// -------
#include "options.h"    // must be first include
#include "hw.h"
#include "timer3.h"
#include "dsPIC_serial.h"
#include "tasker.h"
#include "bootloader.h"
#include "ui.h"
#include "pwm.h"
#include "device.h"
#include "inverter.h"
#include "converter.h" 
#include "nvm.h"

// ----------------------------------------
//  Conditional Compile Flags for debugging
// ----------------------------------------

// comment out flag if not used
#define REDUCE_AC_QUAL_TIME 1   // reduces the A/C qual time (for testing only)
#define DEBUG_SHOW_CFG      1

// automaticlaly remove all debugging
#if OPTION_NO_CONDITIONAL_DBG
 #undef REDUCE_AC_QUAL_TIME
 #undef DEBUG_SHOW_CFG
#endif

// milliseconds when A/C qual time is reduced for testing
#define REDUCED_AC_QUAL_TIME_MSECS  (2000)


// -----------
// Global Data
// -----------
DEVICE_STRUCT_t  Device;		// device info
CONVERTER_t      Conv;			// converter data
uint16_t         RCON_saved;	// contains power on error flags
char             _sysShutDown;  // 0=run, 1=shutdown system


//-----------------------------------------------------------------------------
// Setting the data direction register (TRISx) to 0 makes that pin an output.
// Setting it to a 1 makes it an input. All port pins are defined as inputs after a reset.
// Pin numbers identified for the dsPIC33FJ64MC506A, 64-pin TQFP
// Signal Names are identified on the schematic 170134, Rev L2
//-----------------------------------------------------------------------------

// intitialize the hardware
static void _HwConfig(void)
{
    RCON_saved = RCON;  // save power on error flags

    // Initialize the System Clock
    //  The dsPIC33FJ64MC506A is capable of 40MIPS.  
    CLKDIVbits.PLLPRE = 00000;          //  Input/2 (default)

    //-----------------------------------------------------------------------------
    //  8.0000MHz Crystal
    //  PLLDIV<8:0>: PLL Feedback Divisor bits (also denoted as PLL multiplier)
    //  NOTE: PLLDIV Values are offset by 2 - 0b000 = /2
    //      000000000 =  2
    //      000000001 =  3
    //      ...
    //      000100110 = 40 MIPS ( 0b100110 == 38 == (40-2)) 
    //      000110000 = 50 (default)
    //-----------------------------------------------------------------------------

    PLLFBDbits.PLLDIV = (40-2);     // 40MIPS with 8MHz Crystal

    CLKDIVbits.PLLPOST = 00;        // oscillator output /2

    //   Wait until the Main Oscillator PLL is locked.
    while(0 == OSCCONbits.LOCK);

    RCONbits.WDTO = 0;  // clear watch-dog time out flag
    
    //-----------------------------------------------------------------------------
    // Initialize Port-B
    // Port-B pins are multiplexed with the Analog input pins. Initialization
    // of the Analog input pins is done in the ADC Configuration function.
    //  RB0/AN0     pin-16  analog  E10_SCALED
    //  RB1/AN1     pin-15  analog  V_AC
    //  RB2/AN2     pin-14  analog  OVL
    //  RB3/AN3     pin-13  analog  BATT_TEMP
    //  RB4/AN4     pin-12  analog  I_MEASURE_CL
    //  RB5/AN5     pin-11  analog  I_LOAD_MGMT
    //-----------------------------------------------------------------------------
    
    //  RB6/AN6/PGC pin-17  J6B in-circuit programmer
    TRISBbits.TRISB6 = 0;   //  DEBUG Output
    AD1PCFGLbits.PCFG6 = 1;
    AD2PCFGLbits.PCFG6 = 1;
    
    //  RB7/AN7/PGD pin-18  J6C in-circuit programmer
    TRISBbits.TRISB7 = 0;   //  DEBUG Output
    AD1PCFGLbits.PCFG7 = 1;
    AD2PCFGLbits.PCFG7 = 1;
    
    //  RB8/AN8     pin-21  analog  15V_RAIL_RESET
    TRISBbits.TRISB9 = 0;       //  RB9/AN9     pin-22  DEBUG TP15
    AD1PCFGLbits.PCFG9 = 1;     //  1 = Analog input pin in Digital mode
    AD2PCFGLbits.PCFG9 = 1;     
    //  RB10/AN10   pin-23 analog  XFMR_T-SENSE
    //  RB11/AN11   pin-24 <unused>
    //  RB12/AN12   pin-27 <unused>
    //  RB13/AN13   pin-28 <unused>

  	TRISBbits.TRISB14 = 0;   	//	RB14/AN14   pin-29 BIAS_BOOSTING
  	AD1PCFGLbits.PCFG14 = 1;    //  1 = Analog input pin in Digital mode
  	AD2PCFGLbits.PCFG14 = 1;     
	
    //  RB15/AN15   pin-30 <unused>
    
    // Port-C
    TRISCbits.TRISC13 = 1;      //  RC13        pin-47 input    AC-LINE-SYNC
    TRISCbits.TRISC14 = 0;      //  RC14        pin-48 output   DEBUG TP23
	
    // Port-D
    TRISDbits.TRISD0 = 1;       //  RD0/OC1     pin-46 <unused>
    TRISDbits.TRISD1 = 0;       //  RD1/OC2     pin-49 output   RD1 (STACK)
    TRISDbits.TRISD2 = 1;       //  RD2/OC3     pin-50 input    RD2 (STACK)
    TRISDbits.TRISD3 = 1;       //  RD3/OC4     pin-51 input    INV_I-LOCK
    TRISDbits.TRISD4 = 1;       //  RD4/CN13    pin-52 input    REMOTE_ONOFF (requires weak pull-up)
    CNPU1bits.CN13PUE = 1;      //  enable weak pull-ups on pin-52 <INV_REQUEST>
    TRISDbits.TRISD5 = 0;       //  RD5         pin-53 output   KEEP_ALIVE_PS
    TRISDbits.TRISD6 = 1;       //  RD6         pin-54 input    INV-ON/OFF      
    TRISDbits.TRISD7 = 0;       //  RD7         pin-55 output   DEBUG TP18
  	TRISDbits.TRISD8 = 0;       //  RD8         pin-42 output   BIAS_BOOSTING
    TRISDbits.TRISD9 = 1;       //  RD9         pin-43 input    120V_LINE_VALID
    TRISDbits.TRISD10 = 1;      //  RD10        pin-44 input    XFMR_HTEMP
    TRISDbits.TRISD11 = 1;      //  RD11        pin-45 input    HTSK_TEMP
    
    // Port-E
    TRISEbits.TRISE0 = 0;       //  RE0/PWM1L   pin-60 output   DRIVE_L1
    TRISEbits.TRISE1 = 0;       //  RE1/PWM1H   pin-61 output   DRIVE_H1
    TRISEbits.TRISE2 = 0;       //  RE2/PWM2L   pin-62 output   DRIVE_L2
    TRISEbits.TRISE3 = 0;       //  RE3/PWM2H   pin-63 output   DRIVE_H2
    TRISEbits.TRISE4 = 0;       //  RE4/PWM3L   pin-64 output   RMT_FAULT_RD_LED
    TRISEbits.TRISE5 = 0;       //  RE5/PWM3H   pin-1 output    FAN_CONTROL
    TRISEbits.TRISE6 = 0;       //  RE6/PWM4L   pin-2 output    STATUS_GRN_LED
    TRISEbits.TRISE7 = 0;       //  RE7/PWM4H   pin-3 output    STATUS_RD_LED
    
    // Port-F
    TRISFbits.TRISF0 = 0;       //  RF0/C1TX    pin-59 output   CAN_TX_DATA
    TRISFbits.TRISF1 = 0;       //  RF1/C1RX    pin-58 output   CAN_RX_DATA
    TRISFbits.TRISF2 = 1;       //  RF2/U1RX/SD1    pin-34 input    U1RX
    TRISFbits.TRISF3 = 0;       //  RF3/U1TX/SD0    pin-33 output   U1TX
    TRISFbits.TRISF4 = 1;       //  RF4/CN17/U2RX   pin-31 i/o      SDA2 (NVM)
    TRISFbits.TRISF5 = 0;       //  RF5/CN18/U2TX   pin-32 output   SCL2 (NVM)
//    TRISFbits.TRISF6 = 0;       //  RF6/SCK/INT0    pin-35 output   SSR_DRV
    TRISFbits.TRISF6 = 1;       //  RF6/SCK/INT0    pin-35 INPUT for Kirk's testing
    
    // Port-G
    TRISGbits.TRISG3 = 1;       //  RG3         pin-36  i/o     SDA1 (DAC)
    TRISGbits.TRISG2 = 0;       //  RG2         pin-37  output  SCL1 (DAC)
    
    TRISGbits.TRISG6 = 1;       //  RG6/CN8     pin-4   <unused> 
    TRISGbits.TRISG7 = 1;       //  RG7/CN9     pin-5   <unused>
    TRISGbits.TRISG8 = 1;       //  RG8/CN10    pin-6   <unused>
    TRISGbits.TRISG9 = 0;       //  RG9/CN11    pin-8   output LINE_XFER-CHG

    //-----------------------------------------------------------------------------
    //  INTERRUPT PRIORITIES
    //  For additional detail, refer to the 'dsPIC33FJXXXMCX06 Family Data Sheet'
    //  Microchip doc #70594d,  section 7.1 and TABLE 7.1.1, Interrupt Vector
    //  Table.
    //
    //  Natural order priority is determined by the position of an interrupt
    //  in the vector table, and only affects interrupt operation when multiple
    //  interrupts with the same user-assigned priority become pending at the
    //  same time.
    //
    //  User-assignable priority levels start at 0 as the lowest priority
    //  (disabled) and level 7 as the highest priority.  If more than one
    //  interrupt is user-assigned to the same priority, then natural order
    //  priority will be used within that group.
    //
    //      Timer1 - General Purpose 1-mSec interrupt.
    //      Timer2 - Input Capture - Temperature sensor pulse duty-cycle
    //      Timer3 - Used to trigger ADC conversions.
    //      Timer4 - [LPC Only] Inverter Mode: OVL Signal Gating   //  TODO: ?
    //      Timer5 - [LPC Only] Inverter Mode: OVL Signal Gating   //  TODO: ?
    //      Timer6 - DEBUG: used to toggle test pin with state information
    //      Timer7 - DEBUG: used to toggle test pin with state information
    //-----------------------------------------------------------------------------

//  IPC0bits.INT0IP = 7;    //  Extern. Int. 0  natural = 0 (highest)
    IPC7bits.T5IP = 6;      //  Timer 5         natural = 28
    IPC6bits.T4IP = 5;      //  Timer 4         natural = 27
//  IPC1bits.T2IP = 6;      //  Timer 2         natural = 7
    IPC2bits.T3IP = 4;      //  Timer 3         natural = 8
//  IPC2bits.ADIP = 4;      //  A/D Converter   natural = 13
    IPC14bits.PWMIP = 3;    //  PWM Module      natural = 57
//  Timer1, SPI and UART (U1RX and U1TX) should be lowest priority
    IPC0bits.T1IP = 1;      //  Timer 1         natural = 3

    #ifdef OPTION_UART2
        IPC7bits.U2RXIP = 1;    //  U2RX            natural = 30
        IPC7bits.U2TXIP = 1;    //  U2TX            natural = 31
    #else
        IPC2bits.U1RXIP = 1;    //  U1RX            natural = 11
        IPC3bits.U1TXIP = 1;    //  U1TX            natural = 12
    #endif
}

//------------------------------------------------------------------------------
//                        D E B U G G I N G
//------------------------------------------------------------------------------

// dump device configration to serial port for debuggin
void _show_device_config(void)
{
  #ifdef DEBUG_SHOW_CFG
	const char* aux_str[4] = 
	{
		"DISABLED"	,	//	0
		"RV"		,	//	1     		
		"UTILITY"	,	//	2 		
		"CONTROL"		//	3		
	};
	const char* rm_str[4] =
	{
        "DISABLED"	,	//	0,
        "SNAP"	 	,	//	1,
        "MOMENTARY" ,   //	2,
        "CUSTOM"        //  3	
    };
	
	static uint16_t last_cfg_flags = 0;

    // test if state has changed
	if(last_cfg_flags == Device.config.all_flags) return;
	last_cfg_flags = Device.config.all_flags;
	
	LOG(SS_SYS, SV_INFO, "Device Config: 0x%04X ", Device.config.all_flags);
	if(Device.config.inv_enabled)           LOG(SS_SYS, SV_INFO, " inv_enabled" );
	if(Device.config.chgr_enabled)          LOG(SS_SYS, SV_INFO, " chgr_enabled" );
	if(Device.config.pass_thru_enabled)   	LOG(SS_SYS, SV_INFO, " pass_thru_enabled" );
	if(Device.config.tmr_shutdown_enabled)  LOG(SS_SYS, SV_INFO, " tmr_shutdown_enabled" );
//	if(Device.config.pushbutton_enabled)    LOG(SS_SYS, SV_INFO, " pushbutton_enabled" );
	if(Device.config.batt_temp_sense_present) LOG(SS_SYS, SV_INFO, " batt_temp_sense_present" );

	LOG(SS_SYS, SV_INFO, " pushbutton  = %s",  Device.config.pushbutton_enabled?"ENABLED":"DISABLED"); 
	LOG(SS_SYS, SV_INFO, " aux_mode    = %s", aux_str[Device.config.aux_mode] ); 
	LOG(SS_SYS, SV_INFO, " remote_mode = %s", rm_str[Device.config.remote_mode] );			
  #endif // DEBUG_SHOW_CFG
}

           
// ------------
// Prototyping
// ------------
void TASK_TempSensors(void);
void TASK_devio_Driver(void);
void TASK_MainControl(void);
void TASK_devio_Driver(void);
void TASK_ui_Driver(void);   
void TASK_inv_Driver(void);  
void TASK_can_Driver(void);  
void TASK_chgr_Driver(void); 
void TASK_nvm_Driver(void);  
void TASK_TempSensors(void); 
void TASK_fan_Driver(void);  

// ---------
// task ids
// ---------
TASK_ID_t _task_mainc = -1;
TASK_ID_t _task_devio = -1;
TASK_ID_t _task_ui    = -1;
TASK_ID_t _task_inv   = -1;
TASK_ID_t _task_chg   = -1;
TASK_ID_t _task_nvm   = -1; 
TASK_ID_t _task_temp  = -1; 
TASK_ID_t _task_fan   = -1; 
TASK_ID_t _task_1sec  = -1;  
TASK_ID_t _task_scomm = -1;  
TASK_ID_t _task_can   = -1; 

// keep alive needs to be turned on ASAP so the power will continue
#define _KEEP_ALIVE_ON_RIGHT_NOW()   {TRISDbits.TRISD5 = 0; LATDbits.LATD5 = 1;}

//-----------------------------------------------------------------------------
// it all starts here
int main(void)
{
    static int16_t MilliSecTickCount = 0;
    static int16_t OneSecTickCount = 0;
	
    _KEEP_ALIVE_ON_RIGHT_NOW(); // ASAP or we will die for lack of power
    _HwConfig();

    //  Serial Needs Timer1 to be running
    _T1Config();
    _T1Start();
    
    // start Serial early to allow debugging startup code
    serial_Config();
    serial_Start();

    // turn on all logging during boot time
    LOG_CONFIG();
    LOG_SEVERITY_ALL(LOG_ALL);

    // indicate watchdog reset if it happened
    if (RCON_saved & 0x10)
	  #if defined(OPTION_NO_LOGGING)
		serial_printf("\n\rWatchdog Reset Detected\n\r");
	  #else
        LOG(SS_SYS, SV_ERR, "Watchdog Reset Detected");
	  #endif

    // show initializing
  #if defined(OPTION_NO_LOGGING)
    serial_printf(       "%s %s %s %s initializing\n\r", MODEL_NO_STRING, FW_ID_STRING, BUILD_TYPE_STR, BUILD_DATE);
  #else                                                                               
    LOG(SS_SYS, SV_INFO, "%s %s %s %s initializing",     MODEL_NO_STRING, FW_ID_STRING, BUILD_TYPE_STR, BUILD_DATE);
  #endif

    // need nvm driver to load configuration early on
    nvm_Config();	// 	nvm_Config loads NVM ROM settings into RAM active settings
    nvm_Start();	//	nvm_Start monitors active settings for changes

	// not configurable LPC hardware options
	Device.status.chgr_enable_time = 1; // set only at this point
	Device.status.chgr_enable_jmpr = 1; // set only at this point
    
    //  Initialize Timer-Shutdown Enabled (implementation of 'On-Startup')
    Device.status.tmr_shutdown_enabled = Device.config.tmr_shutdown_enabled;
    
    // configure devices
    an_Config();
    T3_Config();
    pwm_Config(PWM_CONFIG_DEFAULT, &pwm_DefaultIsr);
    ui_Config();
    can_Config();

    an_Start();
    pwm_Start();
    ui_Start();
    can_Start();

    // show fw version
  #if defined(OPTION_NO_LOGGING)
    serial_printf(       "%s %s %s %s Init complete\n\r", MODEL_NO_STRING, FW_ID_STRING, BUILD_TYPE_STR, BUILD_DATE);
  #else
    LOG(SS_SYS, SV_INFO, "%s %s %s %s Init complete",     MODEL_NO_STRING, FW_ID_STRING, BUILD_TYPE_STR, BUILD_DATE);
  #endif
    TASK_devio_Driver();  // initialize logging of enables.

    _show_device_config();
  
  #if !defined(OPTION_SERIAL_IFC)
    // turn on errors
    LOG_SEVERITY_ALL(SV_WARN);
//  LOG_SEVERITY_ALL(LOG_ALL);

    // enable long term logging now
    LOG_ENABLE(SS_CHG,   LOG_ALL);
//  LOG_ENABLE(SS_CHGCMD,LOG_ALL);
    LOG_ENABLE(SS_INV,   LOG_ALL);
    LOG_ENABLE(SS_INVCMD, LOG_ALL);
    LOG_ENABLE(SS_SYS,   LOG_ALL);
//  LOG_ENABLE(SS_NVM,   LOG_ALL);
//  LOG_ENABLE(SS_ANA,   LOG_ALL);
    LOG_ENABLE(SS_UI,    LOG_ALL);
//  LOG_ENABLE(SS_RVC,   LOG_ALL);
//  LOG_ENABLE(SS_PWM,   LOG_ALL);
//  LOG_ENABLE(SS_TASKER,LOG_ALL);
  #endif

  #if IS_BUILD_DVT || IS_BUILD_RELEASE
    LOG_SEVERITY_ALL(SV_WARN);  // warnings and errors only
  #endif

    // fill the queue with task info
    _task_mainc = task_AddToQueue(TASK_MainControl      , "mainc");
    _task_devio = task_AddToQueue(TASK_devio_Driver     , "devio");
    _task_ui    = task_AddToQueue(TASK_ui_Driver        , "ui"   );
    _task_inv   = task_AddToQueue(TASK_inv_Driver       , "inv"  );
    _task_can   = task_AddToQueue(TASK_can_Driver       , "can"  );
    _task_chg   = task_AddToQueue(TASK_chgr_Driver      , "chg"  );
    _task_nvm   = task_AddToQueue(TASK_nvm_Driver       , "nvm"  ); 
    _task_temp  = task_AddToQueue(TASK_TempSensors      , "temp" ); 
    _task_fan   = task_AddToQueue(TASK_fan_Driver       , "fan"  ); 

    _T1TickCount = 0;
    _sysShutDown = 0;    // don't shutdown until commanded

    // put serial debugging into run mode (non-blocking)
    serial_RunMode();

    // enable watchdog timer running at 7 msec
    ClrWdt();
    RCONbits.SWDTEN = 1;

    // preload nvm settings into working memory
	Device.status.inv_enabled 		= Device.config.inv_enabled;
	Device.status.chgr_enable_can 	= Device.config.chgr_enabled;
	Inv.status.load_sense_enabled 	= Inv.config.load_sense_enabled;

  #ifdef REDUCE_AC_QUAL_TIME   // reduce A/C line qual time for testing
    Device.config.ac_line_qual_delay = REDUCED_AC_QUAL_TIME_MSECS;
    g_nvm.settings.dev.ac_line_qual_delay = Device.config.ac_line_qual_delay;
    nvm_SetDirty();  // need to save to NVM
    LOG(SS_SYS, SV_WARN, "A/C Qual Time Reduced (for testing) to %u msecs", Device.config.ac_line_qual_delay);
  #endif

    // run loop
    while(!_sysShutDown)
    {
        if (_T1TickCount > 0)
        {
            _T1TickCount--;

            //  Clear the watchdog timer
            ClrWdt();

            // let the tasks run
            task_MarkAsReady(_task_devio);
            task_MarkAsReady(_task_inv);  
            task_MarkAsReady(_task_chg);  
            task_MarkAsReady(_task_mainc);  
            task_MarkAsReady(_task_ui);   
            task_MarkAsReady(_task_can);  
            task_MarkAsReady(_task_nvm);
            task_MarkAsReady(_task_temp);            
            task_MarkAsReady(_task_fan);  

            // check the once second timer
            if(++MilliSecTickCount >= 1000)
            {
                MilliSecTickCount = 0;
				
                // once-per-second tasks
                if(++OneSecTickCount > 60)
                {
                    // once-per-minute tasks
                    OneSecTickCount = 0;
                  #ifdef OPTION_HAS_CHARGER
					chgr_TimerOneMinuteUpdate();
                  #endif
                }
            }
        }
        task_Execute();

    } // while
    return(0);
}

// <><><><><><><><><><><><><> main.c <><><><><><><><><><><><><><><><><><><><><><>
