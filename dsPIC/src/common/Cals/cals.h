// <><><><><><><><><><><><><> cals.h <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//  Copyright(C) 2017 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------

// -------------------------
// Load the target cal file
// -------------------------


#if   (OPTION_CAL == CAL_12LPC)
      #include "Cals\Cal_12LPC.h"

#elif (OPTION_CAL == CAL_12NP18)
      #include "Cals\Cal_12NP18.h"

#elif (OPTION_CAL == CAL_12NP20)
      #include "Cals\Cal_12NP20.h"

#elif (OPTION_CAL == CAL_12NP24)
      #include "Cals\Cal_12NP24.h"

#elif (OPTION_CAL == CAL_12NP30)
      #include "Cals\Cal_12NP30.h"

#elif (OPTION_CAL == CAL_12DC51)
      #include "Cals\Cal_12DC51.h"

#elif (OPTION_CAL == CAL_24NP24)
      #include "Cals\Cal_24NP24.h"

#elif (OPTION_CAL == CAL_24NP36)
      #include "Cals\Cal_24NP36.h"

#elif (OPTION_CAL == CAL_48NPXX)
      #include "Cals\Cal_48NPxx.h"

#elif (OPTION_CAL == CAL_51NPXX)
      #include "Cals\Cal_51NPxx.h"

#elif (OPTION_CAL == CAL_51LPC)
      #include "Cals\Cal_51LPC.h"

#elif (OPTION_CAL == CAL_51DC12)
      #include "Cals\Cal_51DC12.h"

#elif (OPTION_CAL == CAL_51DC24)
      #include "Cals\Cal_51DC24.h"

// --------------------------------------
//           Error Condition     
// --------------------------------------

#else
    #error No Calibration Specified in 'cals.h'
#endif


// -------------------------------------------------------------------
//    S P E C I A L    O V E R - R I D E    C A L I B R A T I O N S
// -------------------------------------------------------------------

#ifdef MODEL_12DC51_FW0065  // Final Tester uses pot for adjustment
    #undef  DCDC_SLOPE
    #undef  DCDC_INTERCEPT
    #define DCDC_SLOPE         (15.607)
    #define DCDC_INTERCEPT     (-1.9803)
#endif

#ifdef MODEL_12DC51C  // up charger   39.95v~654;  62.00v~1017   Rtop=13.5k Rbot=750ohm
    #undef  DCDC_SLOPE
    #undef  DCDC_INTERCEPT
    #define DCDC_SLOPE         (16.499)
    #define DCDC_INTERCEPT     (-5.1772)
#endif

// <><><><><><><><><><><><><> cals.h <><><><><><><><><><><><><><><><><><><><><><>
