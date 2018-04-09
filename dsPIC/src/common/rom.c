// <><><><><><><><><><><><><> rom.c <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//  Copyright(C) 2016 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
//  ROM values are stored here
//
//-----------------------------------------------------------------------------

//	TODO: Refactor allocation of ROM Defaults
#define ALLOCATE_SPACE_ROM_DEFAULTS

// -------
// headers
// -------
#include "options.h"    // must be first include
#include "charger.h"
#include "converter.h"
#include "device.h"
#include "dsPIC33_CAN.h"
#include "inverter.h"

#undef ALLOCATE_SPACE_ROM_DEFAULTS

// <><><><><><><><><><><><><> rom.c <><><><><><><><><><><><><><><><><><><><><><>
