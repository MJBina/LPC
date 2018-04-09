// <><><><><><><><><><><><><> pwm.h <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//	Copyright(C) 2015 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
//  Pulse-Width-Modulation driver
//
//-----------------------------------------------------------------------------

#ifndef __PWM_H    // include only once
#define __PWM_H

// ------
// enums
// ------
typedef enum 
{
    PWM_CONFIG_DEFAULT,
    PWM_CONFIG_BOOT_CHARGE,
    PWM_CONFIG_CONVERTER,
    PWM_CONFIG_INVERTER,
    PWM_CONFIG_CHARGER,
} PWM_CONFIG_t;

typedef enum 
{
    PWM_ISRCMD_NULL = 0,
    PWM_ISRCMD_RESET = 1,
    PWM_ISRCMD_L1_ON,
    PWM_ISRCMD_L1_OFF,
} PWM_ISRCMD_t;


// --------------------
// Function Prototyping
// --------------------

extern void pwm_DefaultIsr(void);
extern void pwm_Config(PWM_CONFIG_t config, void (*isr_func)(void));
extern void pwm_Start(void);
extern void pwm_Stop(void);

#endif	//	__PWM_H

// <><><><><><><><><><><><><> pwm.h <><><><><><><><><><><><><><><><><><><><><><>
