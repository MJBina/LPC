// <><><><><><><><><><><><><> uimsg.h <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//  Copyright(C) 2015 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
//  User Interface Message Formatting
//
//-----------------------------------------------------------------------------

#ifndef __UIMSG_H // include only once
#define __UIMSG_H

// --------
// Defines
// --------
#define LEN_UI_MSG  (32)   // 2 lines of 16 characters each

// ---------------
// UI Tricolor LED
// ---------------
typedef enum 
{
    UI_LED_OFF = 0,
    UI_LED_RED = 1,
    UI_LED_GRN = 2,
    UI_LED_YEL = 3 // Red + Green gives Amber
} UI_LED_COLOR;


// ------------------------
// tricolor LED flash codes
// ------------------------
#pragma pack(1)  // structure packing on byte alignment
typedef struct
{
    int16_t count;  // blink count
    int8_t  red;    // 0=off, 1=red on
    int8_t  grn;    // 0=off, 1=green on
                    // amber if both red and green on
} UI_LED_FLASH_CODE;
#pragma pack()  // restore packing setting

// --------------------------
// units to string conversion
// --------------------------
typedef void (*UIMSG_FUNC)(char* buf);  // prototype for message patching function 

#pragma pack(1)  // structure packing on byte alignment
typedef struct
{
    int16_t     pos;    // position in the text string to patch with value (zero base)
    UIMSG_FUNC  func;   // function pointer to convert unit to string value (NULL if none)
} UIMSG_SUBSTR;
#pragma pack()  // restore packing setting

// ----------
// ui message
// ----------
#pragma pack(1)  // structure packing on byte alignment
typedef struct 
{
    const char*   lcd_text;  // text string to render
    UIMSG_SUBSTR  sub_str0;  // sub-string 1
    UIMSG_SUBSTR  sub_str1;  // sub-string 1
    UIMSG_SUBSTR  sub_str2;  // sub-string 2
    UI_LED_COLOR  led_color; // LED color
    int16_t       led_count; // LED blink counts
    uint8_t       lcd_red;   // 0=off, 1=remote red led on  (LCD control)
    uint8_t       lcd_exit;  // 0=no,  1=exit settings mode (LCD control)
} UIMSG;
#pragma pack()  // restore packing setting


#endif  //  __UIMSG_H

// <><><><><><><><><><><><><> uimsg.h <><><><><><><><><><><><><><><><><><><><><><>
