// <><><><><><><><><><><><><> pid.h <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//  Copyright(C) 2015 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
//  Propotional-Integral-Differentila feedback control loop
//
//-----------------------------------------------------------------------------

#ifndef __PID_H    // include only once
#define __PID_H

// -------
// headers
// -------
#include "options.h"    // must be first include

// ----------
// PID object
// ----------
#pragma pack(1)  // structure packing on byte alignment
typedef struct
{
    // PID tuning values
    int16_t KP;         // proportional
    int16_t KI;         // integral    
    int16_t KD;         // differential
    int16_t ILIM;       // integral limit
    int16_t DLIM;       // differential limit
    int16_t deadband;   // +/- counts from setpoint to ignore integral
    int16_t KFF;        // feed forward gain
    int16_t FF_deadband;// feed forward deadband (+/- adc counts)

    // user inputs
    int16_t setpoint;   // setpoint value       
    int16_t input;      // current control value
    int16_t reset;      // 0=no, 1=reset hysteresis terms

    // internal
    int16_t error;      // setpoint - input 
    int16_t pterm;      // proportional term
    int16_t iterm;      // integral     term
    int16_t dterm;      // differential term
    int16_t last_input; // last input value 
    int16_t ffterm;     // feed forward term

    // output
    int16_t output;     // new output value

} PID;
#pragma pack()  // restore packing setting

// --------------------
// Function Prototyping
// --------------------

// --------------------------------------------------------------------------------
// absolute value
INLINE int16_t iabs(int16_t value)
{ 
    if (value >= 0) return(value);
    return(-value);
}

// --------------------------------------------------------------------------------
INLINE void pid_SetPoint(PID* pid, int16_t setpoint)
{ 
    pid->setpoint = setpoint;
}

// --------------------------------------------------------------------------------
INLINE void pid_SetTuning(PID* pid, int16_t KP, int16_t KI, int16_t KD, int16_t ILIM, int16_t DLIM)
{ 
    pid->KP   = KP;
    pid->KI   = KI;
    pid->KD   = KD;
    pid->ILIM = ILIM;
    pid->DLIM = DLIM;
}

// --------------------------------------------------------------------------------
INLINE int16_t ClipInt32(int32_t value)
{
    #define MAX_INT16  (32766)
    if (value >  MAX_INT16) return( MAX_INT16);
    if (value < -MAX_INT16) return(-MAX_INT16);
    return(value);
}

// --------------------------------------------------------------------------------
// reset PID hysteresis terms
// should do this when ever going discontinuous
INLINE void pid_Reset(PID* pid)
{
    pid->reset = 1;
}

// --------------------------------------------------------------------------------
// needs to be called on periodic time basis
// returns output value
INLINE float pid_Compute(PID* pid, int16_t input)
{
    // save input
    pid->last_input = pid->input; // save as last input
    pid->input      = input;      // save new input

    // calculate error
    pid->error = pid->setpoint - input;   // required - actual

    // calculate proportional term
    pid->pterm = ClipInt32((int32_t)pid->KP * (int32_t)pid->error);

    // calculate integral term
    pid->iterm = ClipInt32((int32_t)pid->iterm + (int32_t)pid->KI * (int32_t)pid->error);
    if (pid->iterm >  pid->ILIM) pid->iterm =  pid->ILIM;
    if (pid->iterm < -pid->ILIM) pid->iterm = -pid->ILIM;
    if (iabs(pid->input - pid->setpoint) <= pid->deadband)
        pid->iterm = 0;  // apply deadband to integral term

    // calculate differential term
    pid->dterm = ClipInt32(pid->KD * (int32_t)(pid->last_input - pid->input));
    if (pid->dterm >  pid->DLIM) pid->dterm =  pid->DLIM;
    if (pid->dterm < -pid->DLIM) pid->dterm = -pid->DLIM;

    // ignore hysteresis terms i and d
    if (pid->reset)
    {
        pid->dterm = 0;
        pid->iterm = 0;
        pid->reset = 0; // clear reset flag
    }

    // calculate output as sum of components
    pid->output = ClipInt32((int32_t)pid->pterm + (int32_t)pid->iterm + (int32_t)pid->dterm);
    return(pid->output);
}

#endif  //  __PID_H

// <><><><><><><><><><><><><> pid.h <><><><><><><><><><><><><><><><><><><><><><>
