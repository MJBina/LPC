// <><><><><><><><><><><><><> models.h <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//  Copyright(C) 2015 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
//  This header defines the firmware versions,
//  and is included by options.h header file.
//
//  A firmware version is assigned a FW00nn number
//  Not all models are buildable via a drop down list,
//  but can be easily built by have a project set the appropriate MODEL_nnn.
//
//-----------------------------------------------------------------------------

//****************************************************************************
//                 U N A S S I G N E D     V E R S I O N S
//****************************************************************************

// --------------------------------------
//               12 DC     
// --------------------------------------

#if defined(MODEL_12DC51C)
    // mandatory
    #define BUILD_DATE       "2017-11-09-A"
    #define BUILD_TYPE       BUILD_DEV
    #define FW_ID_STRING     "12DC51C"
    #define MODEL_NO_STRING  "12DC51C"
    #define OPTION_DEV       DEV_CONVERTER
    #define OPTION_PCB       PCB_NP
    #define OPTION_PCB_REV   PCB_REV_NP_1
    #define OPTION_DCIN      DCIN_12V
    #define OPTION_CAL       CAL_12NP24
    // optional
    #define OPTION_CAN_BAUD_500K
    #define OPTION_IGNORE_OVERLOAD
    #define OPTION_HAS_CHARGER
    #define OPTION_CHARGE_LION
    #define OPTION_HAS_CHGR_EN_SWITCH
    #define OPTION_DCDC_DCOUT_VOLTS    DCDC_DCOUT_51V
    #define OPTION_DCDC_DFLT_SETPOINT  (58.000)  // cut over to constant voltage from constant current

// --------------------------------------
//               48 DC     
// --------------------------------------

#elif defined(MODEL_48DC12)
    // mandatory
    #define BUILD_DATE       "2017-11-09-A"
    #define BUILD_TYPE       BUILD_DEV
    #define FW_ID_STRING     "48DC12"
    #define MODEL_NO_STRING  "48DC12"
    #define OPTION_DEV       DEV_CONVERTER
    #define OPTION_PCB       PCB_NP
    #define OPTION_PCB_REV   PCB_REV_NP_1
    #define OPTION_DCIN      DCIN_48V
    #define OPTION_CAL       CAL_48NPXX
    // optional
    #define OPTION_CAN_BAUD_500K
    #define OPTION_DCDC_DCOUT_VOLTS    DCDC_DCOUT_12V
    #define OPTION_DCDC_DFLT_SETPOINT  (14.400)
    #define OPTION_HAS_CLAMP

 #elif defined(MODEL_48DC24)
    // mandatory
    #define BUILD_DATE       "2017-11-09-A"
    #define BUILD_TYPE       BUILD_DEV
    #define FW_ID_STRING     "48DC24"
    #define MODEL_NO_STRING  "48DC24"
    #define OPTION_DEV       DEV_CONVERTER
    #define OPTION_PCB       PCB_NP
    #define OPTION_PCB_REV   PCB_REV_NP_1
    #define OPTION_DCIN      DCIN_48V
    #define OPTION_CAL       CAL_48NPXX
    // optional
    #define OPTION_CAN_BAUD_500K
    #define OPTION_DCDC_DCOUT_VOLTS    DCDC_DCOUT_24V
    #define OPTION_DCDC_DFLT_SETPOINT  (28.800)
    #define OPTION_HAS_CLAMP
    
// --------------------------------------
//           Bidirectional DC     
// --------------------------------------

 #elif defined(MODEL_48BDC24)
    // mandatory
    #define BUILD_DATE       "2017-11-09-A"
    #define BUILD_TYPE       BUILD_DEV
    #define FW_ID_STRING     "48BDC24"
    #define MODEL_NO_STRING  "48BDC24"
    #define OPTION_DEV       DEV_CONVERTER
    #define OPTION_PCB       PCB_BDC
    #define OPTION_PCB_REV   PCB_REV_BDC_1
    #define OPTION_DCIN      DCIN_48V
    #define OPTION_CAL       CAL_48NPXX
    // optional
    #define OPTION_CAN_BAUD_500K
    #define OPTION_DCDC_DCOUT_VOLTS    DCDC_DCOUT_24V
    #define OPTION_DCDC_DFLT_SETPOINT  (28.800)
    #define OPTION_HAS_CLAMP



//****************************************************************************
//           A S S I G N E D    F I R M W A R E    V E R S I O N S
//****************************************************************************
//
//  A firmware version is assigned a FWnnnn number
//
//  Not all models are buildable via the drop down list,
//  but can be built using this process.
//
//  To build one of these releases
//    1) In MPLAB NPxx project
//    2) Select "SpecifyModel" project
//    3) Right click on NPxx navigator pane
//    4) Select Properties
//    5) Hightlight  Conf:[SpecifyModel] XC16 {Global Options}
//    6) Click [...] button "Define common macros"  MODEL_
//    7) Enter "MODEL_nnn" as one of the selections below
//    8) [Apply], then [Ok]
//    9) Run \ Clean and Build Project
//   If you get an error "MODEL_nnn" not set by project,
//   then you have not entered a correct model string below
//   (ie: in the #elif defined(MODEL_nnn) statements)
//
//  The firmware file output from this process is located at:
//     NP\dsPIC\NPxx\dist\SpecifyModel\production\NPxx.production.hex  .
//  Rename it the the appropriate FW00nn.hex fhile.
//
//****************************************************************************

#elif defined(MODEL_51NP36_FW0056)      // FW0056
    #include "Models\FW0056_51NP36.h"

#elif defined(MODEL_51NP36_FW0057)      // FW0057
    #include "Models\FW0057_51NP36.h"
	
#elif defined(MODEL_12LPC15_FW0058)     // FW0058
    #include "Models\FW0058_12LPC15.h"

#elif defined(MODEL_51DC12_FW0059)      // FW0059
    #include "Models\FW0059_51DC12.h"

#elif defined(MODEL_24NP36_FW0060)      // FW0060
    #include "Models\FW0060_24NP36.h"

#elif defined(MODEL_12NP24_FW0061)      // FW0061
    #include "Models\FW0061_12NP24.h"

#elif defined(MODEL_51DC24_FW0062)      // FW0062
    #include "Models\FW0062_51DC24.h"

#elif defined(MODEL_12LP15_FW0063)      // FW0063
    #include "Models\FW0063_12LP15.h"

#elif defined(MODEL_51LPC20_FW0064)     // FW0064
    #include "Models\FW0064_51LPC20.h"

#elif defined(MODEL_12DC51_FW0065)      // FW0065 auto tester
    #include "Models\FW0065_12DC51AT.h"

#elif defined(MODEL_12LP15R_FW0066)     // FW0066
    #include "Models\FW0066_12LP15R.h"
                                        // FW0067 is new LCD firmware

#elif defined(MODEL_12NP18_FW0068)      // FW0068
    #include "Models\FW0068_12NP18.h"

#elif defined(MODEL_12NP24_FW0069)      // FW0069
    #include "Models\FW0069_12NP24.h"

#elif defined(MODEL_12NP30_FW0070)      // FW0070
    #include "Models\FW0070_12NP30.h"

#elif defined(MODEL_24NP24_FW0071)      // FW0071
    #include "Models\FW0071_24NP24.h"

#elif defined(MODEL_12NP20_FW0072)      // FW0072
    #include "Models\FW0072_12NP30.h"

#elif defined(MODEL_24NP36_FW0075)      // FW0075
    #include "Models\FW0075_24NP36.h"

// --------------------------------------
//           Error Condition     
// --------------------------------------

#else
    #error "MODEL_nnn" not set by project and needed by 'models.h'
#endif


// <><><><><><><><><><><><><> models.h <><><><><><><><><><><><><><><><><><><><><><>
