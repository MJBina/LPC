// <><><><><><><><><><><><><> fan_ctrl.c <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//  Copyright(C) 2015 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
//  Cooling Fan Control for NP flavor board
//
//-----------------------------------------------------------------------------

// -------
// headers
// -------
#include "options.h"    // must be first include
#include "fan_ctrl.h"

// -------------------------------------------------
// include the appropriate fan based upon the model
// -------------------------------------------------

#if IS_PCB_NP || IS_PCB_BDC
    //  NP and DC Models
    #include "fan_ctrl_np.c"  // PWM operation
    
#elif IS_PCB_LPC
    //  LPC Models
    #include "fan_ctrl_lpc.c"
    
#endif

// <><><><><><><><><><><><><> fan_ctrl.c <><><><><><><><><><><><><><><><><><><><><><>
