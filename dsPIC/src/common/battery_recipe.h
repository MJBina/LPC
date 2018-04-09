// <><><><><><><><><><><><><> battery_recipe.h <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//  Copyright(C) 2016 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
//  Battery Recipe required for chargins
//  
//-----------------------------------------------------------------------------

#ifndef __BATTERY_RECIPE_H    // include only once
#define __BATTERY_RECIPE_H

// -------
// headers
// -------
#include "options.h"    // must be first include
#include "analog.h"

// ---------------------------------------------------------------------------
// Battery-Type 
// 	NOTE: The RV-C specification (DC_SOURCE_STATUS_4, DGN 0x1FEC9) provides a 
//	4-bit value for Battery Type.  It also defines the following battery types:
//		0=Flooded, 1=Gel, 2=AGM, 3=Lithium-Iron-Phosphate, 13-29 Reserved for 
//		Vendor-defined proprietary types.
//	A 4-bit value has a maximum of 0x0F == 15!
// ---------------------------------------------------------------------------

typedef enum
{
    BATT_WET = 0,       //  Lead Acid
    BATT_GEL = 1,
    BATT_AGM = 2,
    BATT_LI  = 3,		//	Volta Lithium Ion
} BATT_TYPE_t;


//-----------------------------------------------------------------------------
//  Battery Recipe Temperature Trip Points (in Celcius)
//       > 60°C : Warm Batt to Shutdown
//      <= 55°C : From Shutdown to Warm Batt
//      >= 50°C : From Normal to Warm Batt (bulk and acceptance timers on hold)
//      <= 45°C : From Warm Batt to Normal (bulk and acceptance timers resume)
//-----------------------------------------------------------------------------

#define CHGR_BATT_TEMP_SHUTDOWN_THRES    ((int16_t)(60))  // Celsius
#define CHGR_BATT_TEMP_SHUTDOWN_RECOV    ((int16_t)(55))  // Celsius
#define CHGR_BATT_TEMP_WARM_THRES        ((int16_t)(50))  // Celsius
#define CHGR_BATT_TEMP_WARM_RECOV        ((int16_t)(45))  // Celsius

//-----------------------------------------------------------------------------
//  CV to Float Rate-Of-Change (ROC): 
//	The goal of using Rate-of-Change is to transition from CV to Float state 
//	when the battery is near full charge.
//
//	As the battery becomes charged, the current decreases. Normally, charge 
//	current decreases exponentially with charge. When fully charged the current 
//	is essentially constant, but may vary in magnitude due to parasitic loads,
//	and battery condition.
//	
//	In Charger Mode, the transformer primary current (IMeas) is proportional to
//	the charge current. The "Standard Deviation" (a.k.a. SD) of the charge 
//	current is calculated and used to determine the rate of change.
//
//	There are 2 parameters that affect the transition from CV to Float:
//		cv_roc_sdev_threshold - Float Rate-of-Change Std Dev Threshold (adc)
//		cv_roc_timeout_minutes - Float Rate-of-Change Threshold  (minutes)
//	The basic function is that when the SD is less-than the threshold, then the 
//	timer {counter) will decrement. When the timer decrements to zero, then
//	we transition from CV to Float occurs.
//
//-----------------------------------------------------------------------------

//	NOTE: WACR is updated every 68.27 secs ((64*64)/60)
#define CHGR_BATT_ROC_TIMEOUT		39  //39x68.27/60=44.4min
#define CHGR_BATT_ROC_SDEV_THRES	1

// --------------
// Battery Recipe
// --------------

#pragma pack(1)  // structure packing on byte alignment
typedef struct
{
    BATT_TYPE_t batt_type;                  // Battery Type
    int16_t     is_custom;                  // 0=no, 1=custom battery recipe (allows changing values)
    int16_t     cc_max_amp_hours;           // Constant-Current Timeout	        (amp-hours)
    int16_t     cv_timeout_minutes;	        // Constant-Voltage Timeout (long)  (minutes)
    int16_t     cv_roc_timeout_minutes;     // Float Rate-of-Change Threshold   (minutes)
    int16_t		float_timeout_minutes;	    // Float Timeout 				   	(minutes)
    int16_t		eq_timeout_minutes;		    // Equalization Timeout (duration)  (minutes)
    ADC10_RDG_t eq_volts_max;        	    // Equalization Maximum Voltage     (adc)   
    ADC10_RDG_t eq_vsetpoint;        	    // Equalization Setpoint Voltage    (adc) (-1 = disabled)
    ADC10_RDG_t cv_volts_max;               // CV Maximum Voltage    		    (adc)   
    ADC10_RDG_t cv_vsetpoint;               // CV Setpoint Voltage              (adc)
    ADC10_RDG_t float_volts_max;            // Float Maximum Voltage 		    (adc)
    ADC10_RDG_t float_vsetpoint;            // Float Setpoint Voltage 			(adc)
    ADC10_RDG_t float_vthreshold;           // Float (init) Threshold Voltage   (adc)
    int16_t 	batt_temp_warm_recov;       // Warm Batt to Normal   (Celsius)
    int16_t 	batt_temp_warm_thres;       // Normal to Warm Batt   (Celsius)
    int16_t 	batt_temp_shutdown_recov;   // Shutdown to Warm Batt (Celsius)
    int16_t 	batt_temp_shutdown_thres;   // Warm Batt to Shutdown (Celsius)
    int16_t     cv_roc_sdev_threshold;	    // cv rate-of-change Std Dev Threshold (adc)
} BATTERY_RECIPE_t;		// 24-bytes
#pragma pack()  // restore packing setting

// -----------------------------------------------------------------------------
// Battery Recipe Initializers
// -----------------------------------------------------------------------------


//	The default nominal battery temperature, for temperature 
#define CHGR_BATT_TEMP_NOMINAL   (25)

#define CHGR_BATT_WET_CV_TIMEOUT_MINUTES 	  (6*60)  // 6-hours
#define CHGR_BATT_AGM_CV_TIMEOUT_MINUTES 	  (6*60)  // 6-hours
#define CHGR_BATT_GEL_CV_TIMEOUT_MINUTES 	  (6*60)  // 6-hours
#define CHGR_BATT_LI_CV_TIMEOUT_MINUTES 	  (10)    // 10-minutes	(same as FW0057.B)
#define CHGR_BATT_DEFAULT_CV_TIMEOUT_MINUTES  (2*60)  // 2-hours
#define CHGR_BATT_FLOAT_TIMEOUT_MINUTES       (4*60)  // 4-hours
#define CHGR_BATT_EQ_TIMEOUT_MINUTES 	      (4*60)  // 4-hours
#define CHGR_BATT_EQ_TIMEOUT_INVALID	      (-1)    // signed int16

// -----------------------------------------------------------------------------
//	These parameters are specific to the battery voltage
// -----------------------------------------------------------------------------

#define BATT_RECIPE_NOT_USED_ADC_VALUE      (MAX_A2D_VALUE+1)

#if IS_DCIN_12V

    #define RESTART_BATT_THRES_FLOAT        (VBATT_VOLTS_ADC(12.60))
	#define RESTART_BATT_THRES_CV           (VBATT_VOLTS_ADC(12.00))

	#define CHGR_BATT_WET_EQ_VOLTS_MAX 		(VBATT_VOLTS_ADC(16.25))
	#define CHGR_BATT_WET_EQ_VSETPOINT   	(VBATT_VOLTS_ADC(15.50))
	#define CHGR_BATT_WET_CV_VOLTS_MAX 		(VBATT_VOLTS_ADC(15.35))
	#define CHGR_BATT_WET_CV_VSETPOINT      (VBATT_VOLTS_ADC(14.60))
	#define CHGR_BATT_WET_FLOAT_VOLTS_MAX	(VBATT_VOLTS_ADC(14.15))
	#define CHGR_BATT_WET_FLOAT_VSETPOINT   (VBATT_VOLTS_ADC(13.40))
	#define CHGR_BATT_WET_FLOAT_THRESHOLD   (VBATT_VOLTS_ADC(12.80))

	#define CHGR_BATT_GEL_EQ_VOLTS_MAX 		(VBATT_VOLTS_ADC(14.85))	//	t.b.d.
	#define CHGR_BATT_GEL_EQ_VSETPOINT   	(VBATT_VOLTS_ADC(14.10))	//	t.b.d.
	#define CHGR_BATT_GEL_CV_VOLTS_MAX 		(VBATT_VOLTS_ADC(14.85))      
	#define CHGR_BATT_GEL_CV_VSETPOINT      (VBATT_VOLTS_ADC(14.10))      
	#define CHGR_BATT_GEL_FLOAT_VOLTS_MAX   (VBATT_VOLTS_ADC(14.35))      
	#define CHGR_BATT_GEL_FLOAT_VSETPOINT   (VBATT_VOLTS_ADC(13.60))
	#define CHGR_BATT_GEL_FLOAT_THRESHOLD   (VBATT_VOLTS_ADC(12.80))

	#define CHGR_BATT_AGM_EQ_VOLTS_MAX 		(VBATT_VOLTS_ADC(15.25))	//	t.b.d.
	#define CHGR_BATT_AGM_EQ_VSETPOINT   	(VBATT_VOLTS_ADC(14.50))	//	t.b.d.
	#define CHGR_BATT_AGM_CV_VOLTS_MAX      (VBATT_VOLTS_ADC(15.25))
	#define CHGR_BATT_AGM_CV_VSETPOINT      (VBATT_VOLTS_ADC(14.50))
	#define CHGR_BATT_AGM_FLOAT_VOLTS_MAX   (VBATT_VOLTS_ADC(14.25))
	#define CHGR_BATT_AGM_FLOAT_VSETPOINT   (VBATT_VOLTS_ADC(13.50))
	#define CHGR_BATT_AGM_FLOAT_THRESHOLD   (VBATT_VOLTS_ADC(12.80))

	// 12V LIION NOT SUPPORTED!!!!
	#define CHGR_BATT_LI_EQ_VOLTS_MAX   	(BATT_RECIPE_NOT_USED_ADC_VALUE)
	#define CHGR_BATT_LI_EQ_VSETPOINT     	(BATT_RECIPE_NOT_USED_ADC_VALUE)
	#define CHGR_BATT_LI_CV_VOLTS_MAX   	(BATT_RECIPE_NOT_USED_ADC_VALUE)
	#define CHGR_BATT_LI_CV_VSETPOINT       (BATT_RECIPE_NOT_USED_ADC_VALUE)
	#define CHGR_BATT_LI_FLOAT_VOLTS_MAX    (BATT_RECIPE_NOT_USED_ADC_VALUE)
	#define CHGR_BATT_LI_FLOAT_VSETPOINT    (BATT_RECIPE_NOT_USED_ADC_VALUE)
	#define CHGR_BATT_LI_FLOAT_THRESHOLD    (BATT_RECIPE_NOT_USED_ADC_VALUE)
	
  #elif IS_DCIN_24V

	#define RESTART_BATT_THRES_FLOAT        (VBATT_VOLTS_ADC(25.20))
	#define RESTART_BATT_THRES_CV           (VBATT_VOLTS_ADC(24.00))

	#define CHGR_BATT_WET_EQ_VOLTS_MAX 		(VBATT_VOLTS_ADC(2 * 16.25))
	#define CHGR_BATT_WET_EQ_VSETPOINT   	(VBATT_VOLTS_ADC(2 * 15.50))
	#define CHGR_BATT_WET_CV_VOLTS_MAX 		(VBATT_VOLTS_ADC(2 * 15.35))
	#define CHGR_BATT_WET_CV_VSETPOINT      (VBATT_VOLTS_ADC(2 * 14.60))
	#define CHGR_BATT_WET_FLOAT_VOLTS_MAX	(VBATT_VOLTS_ADC(2 * 14.15))
	#define CHGR_BATT_WET_FLOAT_VSETPOINT   (VBATT_VOLTS_ADC(2 * 13.40))
	#define CHGR_BATT_WET_FLOAT_THRESHOLD   (VBATT_VOLTS_ADC(2 * 12.80))

	#define CHGR_BATT_GEL_EQ_VOLTS_MAX 		(VBATT_VOLTS_ADC(2 * 14.85))	//	t.b.d.
	#define CHGR_BATT_GEL_EQ_VSETPOINT   	(VBATT_VOLTS_ADC(2 * 14.10))	//	t.b.d.
	#define CHGR_BATT_GEL_CV_VOLTS_MAX 		(VBATT_VOLTS_ADC(2 * 14.85))      
	#define CHGR_BATT_GEL_CV_VSETPOINT      (VBATT_VOLTS_ADC(2 * 14.10))      
	#define CHGR_BATT_GEL_FLOAT_VOLTS_MAX   (VBATT_VOLTS_ADC(2 * 14.35))      
	#define CHGR_BATT_GEL_FLOAT_VSETPOINT   (VBATT_VOLTS_ADC(2 * 13.60))
	#define CHGR_BATT_GEL_FLOAT_THRESHOLD   (VBATT_VOLTS_ADC(2 * 12.80))

	#define CHGR_BATT_AGM_EQ_VOLTS_MAX 		(VBATT_VOLTS_ADC(2 * 15.25))	//	t.b.d.
	#define CHGR_BATT_AGM_EQ_VSETPOINT   	(VBATT_VOLTS_ADC(2 * 14.50))	//	t.b.d.
	#define CHGR_BATT_AGM_CV_VOLTS_MAX      (VBATT_VOLTS_ADC(2 * 15.25))
	#define CHGR_BATT_AGM_CV_VSETPOINT      (VBATT_VOLTS_ADC(2 * 14.50))
	#define CHGR_BATT_AGM_FLOAT_VOLTS_MAX   (VBATT_VOLTS_ADC(2 * 14.25))
	#define CHGR_BATT_AGM_FLOAT_VSETPOINT   (VBATT_VOLTS_ADC(2 * 13.50))
	#define CHGR_BATT_AGM_FLOAT_THRESHOLD   (VBATT_VOLTS_ADC(2 * 12.80))

	// 24V LIION NOT SUPPORTED!!!!
	#define CHGR_BATT_LI_EQ_VOLTS_MAX       (BATT_RECIPE_NOT_USED_ADC_VALUE)
	#define CHGR_BATT_LI_EQ_VSETPOINT       (BATT_RECIPE_NOT_USED_ADC_VALUE)
	#define CHGR_BATT_LI_CV_VOLTS_MAX       (BATT_RECIPE_NOT_USED_ADC_VALUE)
	#define CHGR_BATT_LI_CV_VSETPOINT       (BATT_RECIPE_NOT_USED_ADC_VALUE)
	#define CHGR_BATT_LI_FLOAT_VOLTS_MAX    (BATT_RECIPE_NOT_USED_ADC_VALUE)
	#define CHGR_BATT_LI_FLOAT_VSETPOINT    (BATT_RECIPE_NOT_USED_ADC_VALUE)
	#define CHGR_BATT_LI_FLOAT_THRESHOLD    (BATT_RECIPE_NOT_USED_ADC_VALUE)
	
  #elif IS_DCIN_48V

    #define RESTART_BATT_THRES_FLOAT        (VBATT_VOLTS_ADC(55.50))	//	t.b.d.
	#define RESTART_BATT_THRES_CV	        (VBATT_VOLTS_ADC(53.70))	//	t.b.d.

	#define CHGR_BATT_WET_EQ_VOLTS_MAX 		(VBATT_VOLTS_ADC(4 * 16.25))
	#define CHGR_BATT_WET_EQ_VSETPOINT   	(VBATT_VOLTS_ADC(4 * 15.50))
	#define CHGR_BATT_WET_CV_VOLTS_MAX 		(VBATT_VOLTS_ADC(4 * 15.35))
	#define CHGR_BATT_WET_CV_VSETPOINT      (VBATT_VOLTS_ADC(4 * 14.60))
	#define CHGR_BATT_WET_FLOAT_VOLTS_MAX	(VBATT_VOLTS_ADC(4 * 14.15))
	#define CHGR_BATT_WET_FLOAT_VSETPOINT   (VBATT_VOLTS_ADC(4 * 13.40))
	#define CHGR_BATT_WET_FLOAT_THRESHOLD   (VBATT_VOLTS_ADC(4 * 12.80))

	#define CHGR_BATT_GEL_EQ_VOLTS_MAX 		(VBATT_VOLTS_ADC(4 * 14.85))	//	t.b.d.
	#define CHGR_BATT_GEL_EQ_VSETPOINT   	(VBATT_VOLTS_ADC(4 * 14.10))	//	t.b.d.
	#define CHGR_BATT_GEL_CV_VOLTS_MAX 		(VBATT_VOLTS_ADC(4 * 14.85))      
	#define CHGR_BATT_GEL_CV_VSETPOINT      (VBATT_VOLTS_ADC(4 * 14.10))      
	#define CHGR_BATT_GEL_FLOAT_VOLTS_MAX   (VBATT_VOLTS_ADC(4 * 14.35))      
	#define CHGR_BATT_GEL_FLOAT_VSETPOINT   (VBATT_VOLTS_ADC(4 * 13.60))
	#define CHGR_BATT_GEL_FLOAT_THRESHOLD   (VBATT_VOLTS_ADC(4 * 12.80))

	#define CHGR_BATT_AGM_EQ_VOLTS_MAX 		(VBATT_VOLTS_ADC(4 * 15.25))	//	t.b.d.
	#define CHGR_BATT_AGM_EQ_VSETPOINT   	(VBATT_VOLTS_ADC(4 * 14.50))	//	t.b.d.
	#define CHGR_BATT_AGM_CV_VOLTS_MAX      (VBATT_VOLTS_ADC(4 * 15.25))
	#define CHGR_BATT_AGM_CV_VSETPOINT      (VBATT_VOLTS_ADC(4 * 14.50))
	#define CHGR_BATT_AGM_FLOAT_VOLTS_MAX   (VBATT_VOLTS_ADC(4 * 14.25))
	#define CHGR_BATT_AGM_FLOAT_VSETPOINT   (VBATT_VOLTS_ADC(4 * 13.50))
	#define CHGR_BATT_AGM_FLOAT_THRESHOLD   (VBATT_VOLTS_ADC(4 * 12.80))
	
	// [Volta] 48V LiIon - Not all values are used.  
	//	48V is used for testing because DC Power Supply goes to 60 VDC max
	#define CHGR_BATT_LI_EQ_VOLTS_MAX       (BATT_RECIPE_NOT_USED_ADC_VALUE) 
	#define CHGR_BATT_LI_EQ_VSETPOINT       (BATT_RECIPE_NOT_USED_ADC_VALUE)
	#define CHGR_BATT_LI_CV_VOLTS_MAX       (BATT_RECIPE_NOT_USED_ADC_VALUE)
	#define CHGR_BATT_LI_CV_VSETPOINT       (VBATT_VOLTS_ADC((58.2)*(48.0/51.0)))
	#define CHGR_BATT_LI_FLOAT_VOLTS_MAX    (BATT_RECIPE_NOT_USED_ADC_VALUE)
	#define CHGR_BATT_LI_FLOAT_VSETPOINT    (BATT_RECIPE_NOT_USED_ADC_VALUE)
	#define CHGR_BATT_LI_FLOAT_THRESHOLD    (BATT_RECIPE_NOT_USED_ADC_VALUE)
	
  #elif IS_DCIN_51V

    #define RESTART_BATT_THRES_FLOAT        (VBATT_VOLTS_ADC(59.60))	//	t.b.d.  
	#define RESTART_BATT_THRES_CV           (VBATT_VOLTS_ADC(57.00))	//	t.b.d

	// 51V WET NOT SUPPORTED!!!!
	#define CHGR_BATT_WET_EQ_VOLTS_MAX      (BATT_RECIPE_NOT_USED_ADC_VALUE)
	#define CHGR_BATT_WET_EQ_VSETPOINT      (BATT_RECIPE_NOT_USED_ADC_VALUE)
	#define CHGR_BATT_WET_CV_VOLTS_MAX      (BATT_RECIPE_NOT_USED_ADC_VALUE)
	#define CHGR_BATT_WET_CV_VSETPOINT      (BATT_RECIPE_NOT_USED_ADC_VALUE)
	#define CHGR_BATT_WET_FLOAT_VOLTS_MAX   (BATT_RECIPE_NOT_USED_ADC_VALUE)
	#define CHGR_BATT_WET_FLOAT_VSETPOINT   (BATT_RECIPE_NOT_USED_ADC_VALUE)
	#define CHGR_BATT_WET_FLOAT_THRESHOLD   (BATT_RECIPE_NOT_USED_ADC_VALUE)

	// 51V GEL NOT SUPPORTED!!!!
	#define CHGR_BATT_GEL_EQ_VOLTS_MAX      (BATT_RECIPE_NOT_USED_ADC_VALUE)
	#define CHGR_BATT_GEL_EQ_VSETPOINT      (BATT_RECIPE_NOT_USED_ADC_VALUE)
	#define CHGR_BATT_GEL_CV_VOLTS_MAX      (BATT_RECIPE_NOT_USED_ADC_VALUE)
	#define CHGR_BATT_GEL_CV_VSETPOINT      (BATT_RECIPE_NOT_USED_ADC_VALUE)
	#define CHGR_BATT_GEL_FLOAT_VOLTS_MAX   (BATT_RECIPE_NOT_USED_ADC_VALUE)
	#define CHGR_BATT_GEL_FLOAT_VSETPOINT   (BATT_RECIPE_NOT_USED_ADC_VALUE)
	#define CHGR_BATT_GEL_FLOAT_THRESHOLD   (BATT_RECIPE_NOT_USED_ADC_VALUE)

	// 51V AGM NOT SUPPORTED!!!!
	#define CHGR_BATT_AGM_EQ_VOLTS_MAX      (BATT_RECIPE_NOT_USED_ADC_VALUE)
	#define CHGR_BATT_AGM_EQ_VSETPOINT      (BATT_RECIPE_NOT_USED_ADC_VALUE)
	#define CHGR_BATT_AGM_CV_VOLTS_MAX      (BATT_RECIPE_NOT_USED_ADC_VALUE)
	#define CHGR_BATT_AGM_CV_VSETPOINT      (BATT_RECIPE_NOT_USED_ADC_VALUE)
	#define CHGR_BATT_AGM_FLOAT_VOLTS_MAX   (BATT_RECIPE_NOT_USED_ADC_VALUE)
	#define CHGR_BATT_AGM_FLOAT_VSETPOINT   (BATT_RECIPE_NOT_USED_ADC_VALUE)
	#define CHGR_BATT_AGM_FLOAT_THRESHOLD   (BATT_RECIPE_NOT_USED_ADC_VALUE)

	// [Volta] 51V LiIon battery recipe
	#define CHGR_BATT_LI_EQ_VOLTS_MAX       (BATT_RECIPE_NOT_USED_ADC_VALUE)
	#define CHGR_BATT_LI_EQ_VSETPOINT       (BATT_RECIPE_NOT_USED_ADC_VALUE)
	#define CHGR_BATT_LI_CV_VOLTS_MAX       (BATT_RECIPE_NOT_USED_ADC_VALUE)
	#define CHGR_BATT_LI_CV_VSETPOINT       (VBATT_VOLTS_ADC(58.2))
	#define CHGR_BATT_LI_FLOAT_VOLTS_MAX    (BATT_RECIPE_NOT_USED_ADC_VALUE)
	#define CHGR_BATT_LI_FLOAT_VSETPOINT    (BATT_RECIPE_NOT_USED_ADC_VALUE)
	#define CHGR_BATT_LI_FLOAT_THRESHOLD    (BATT_RECIPE_NOT_USED_ADC_VALUE)
	
#endif

#define CHGR_WET_CC_TIMEOUT_AMP_HOURS	((int16_t)(6 * IDC_CHG_MAX_AMPS))

#define CHARGER_WET_BATTERY_RECIPE  \
    {                               \
    BATT_WET,                       \
    0,                              \
    CHGR_WET_CC_TIMEOUT_AMP_HOURS,  \
    CHGR_BATT_WET_CV_TIMEOUT_MINUTES, \
    CHGR_BATT_ROC_TIMEOUT,          \
	CHGR_BATT_FLOAT_TIMEOUT_MINUTES, \
	CHGR_BATT_EQ_TIMEOUT_MINUTES,	\
    CHGR_BATT_WET_EQ_VOLTS_MAX,   	\
    CHGR_BATT_WET_EQ_VSETPOINT,  	\
    CHGR_BATT_WET_CV_VOLTS_MAX,   	\
    CHGR_BATT_WET_CV_VSETPOINT,  	\
    CHGR_BATT_WET_FLOAT_VOLTS_MAX,	\
    CHGR_BATT_WET_FLOAT_VSETPOINT,  \
    CHGR_BATT_WET_FLOAT_THRESHOLD,  	\
    CHGR_BATT_TEMP_WARM_RECOV,      \
    CHGR_BATT_TEMP_WARM_THRES,      \
    CHGR_BATT_TEMP_SHUTDOWN_RECOV,  \
    CHGR_BATT_TEMP_SHUTDOWN_THRES,  \
	CHGR_BATT_ROC_SDEV_THRES,		\
}

#define CHGR_GEL_CC_TIMEOUT_AMP_HOURS	((int16_t)(6 * IDC_CHG_MAX_AMPS))

#define CHARGER_GEL_BATTERY_RECIPE  \
    {                               \
    BATT_GEL,                       \
    0,                              \
    CHGR_GEL_CC_TIMEOUT_AMP_HOURS,	\
    CHGR_BATT_GEL_CV_TIMEOUT_MINUTES, \
    CHGR_BATT_ROC_TIMEOUT,          \
	CHGR_BATT_FLOAT_TIMEOUT_MINUTES, \
	CHGR_BATT_EQ_TIMEOUT_INVALID,	\
    CHGR_BATT_GEL_EQ_VOLTS_MAX,   	\
    CHGR_BATT_GEL_EQ_VSETPOINT,     \
    CHGR_BATT_GEL_CV_VOLTS_MAX,   	\
    CHGR_BATT_GEL_CV_VSETPOINT,  	\
    CHGR_BATT_GEL_FLOAT_VOLTS_MAX,  \
    CHGR_BATT_GEL_FLOAT_VSETPOINT,  \
    CHGR_BATT_GEL_FLOAT_THRESHOLD,  \
    CHGR_BATT_TEMP_WARM_RECOV,      \
    CHGR_BATT_TEMP_WARM_THRES,      \
    CHGR_BATT_TEMP_SHUTDOWN_RECOV,  \
    CHGR_BATT_TEMP_SHUTDOWN_THRES,  \
	CHGR_BATT_ROC_SDEV_THRES,		\
}

//  AGM - Absorbative Glass Mat

#define CHGR_AGM_CC_TIMEOUT_AMP_HOURS	((int16_t)(6 * IDC_CHG_MAX_AMPS))

#define CHARGER_AGM_BATTERY_RECIPE  \
    {                               \
    BATT_AGM,                       \
    0,                              \
    CHGR_AGM_CC_TIMEOUT_AMP_HOURS,	\
    CHGR_BATT_AGM_CV_TIMEOUT_MINUTES, \
    CHGR_BATT_ROC_TIMEOUT,          \
	CHGR_BATT_FLOAT_TIMEOUT_MINUTES, \
	CHGR_BATT_EQ_TIMEOUT_INVALID,	\
    CHGR_BATT_AGM_EQ_VOLTS_MAX,     \
    CHGR_BATT_AGM_EQ_VSETPOINT,     \
    CHGR_BATT_AGM_CV_VOLTS_MAX,     \
    CHGR_BATT_AGM_CV_VSETPOINT,     \
    CHGR_BATT_AGM_FLOAT_VOLTS_MAX,  \
    CHGR_BATT_AGM_FLOAT_VSETPOINT,  \
    CHGR_BATT_AGM_FLOAT_THRESHOLD,  \
    CHGR_BATT_TEMP_WARM_RECOV,      \
    CHGR_BATT_TEMP_WARM_THRES,      \
    CHGR_BATT_TEMP_SHUTDOWN_RECOV,  \
    CHGR_BATT_TEMP_SHUTDOWN_THRES,  \
	CHGR_BATT_ROC_SDEV_THRES,		\
}

//  [Volta] Lithium-Ion 

#define CHGR_LI_CC_TIMEOUT_AMP_HOURS    (BATT_RECIPE_NOT_USED_ADC_VALUE)

#define CHARGER_LI_BATTERY_RECIPE   \
    {                               \
    BATT_LI,                        \
    0,                              \
    CHGR_LI_CC_TIMEOUT_AMP_HOURS,	\
    CHGR_BATT_LI_CV_TIMEOUT_MINUTES, \
    CHGR_BATT_ROC_TIMEOUT,          \
	CHGR_BATT_FLOAT_TIMEOUT_MINUTES, \
	CHGR_BATT_EQ_TIMEOUT_INVALID,	\
    CHGR_BATT_LI_EQ_VOLTS_MAX,      \
    CHGR_BATT_LI_EQ_VSETPOINT,      \
    CHGR_BATT_LI_CV_VOLTS_MAX,      \
    CHGR_BATT_LI_CV_VSETPOINT,      \
    CHGR_BATT_LI_FLOAT_VOLTS_MAX,   \
    CHGR_BATT_LI_FLOAT_VSETPOINT,   \
    CHGR_BATT_LI_FLOAT_THRESHOLD,   \
    CHGR_BATT_TEMP_WARM_RECOV,      \
    CHGR_BATT_TEMP_WARM_THRES,      \
    CHGR_BATT_TEMP_SHUTDOWN_RECOV,  \
    CHGR_BATT_TEMP_SHUTDOWN_THRES,  \
	CHGR_BATT_ROC_SDEV_THRES,		\
}

// -----------------------------
// Define Default Battery Recipe
// -----------------------------

#if defined(OPTION_CHARGE_LION)
  #define CHARGER_DEFAULT_BATTERY_RECIPE   CHARGER_LI_BATTERY_RECIPE
  #define ChgrConfigBattRecipe_Default	   ChgrConfigBattRecipe_LI
#else
  #define CHARGER_DEFAULT_BATTERY_RECIPE   CHARGER_GEL_BATTERY_RECIPE
  #define ChgrConfigBattRecipe_Default	   ChgrConfigBattRecipe_Wet
#endif

#endif  //  __BATTERY_RECIPE_H

// <><><><><><><><><><><><><> battery_recipe.h <><><><><><><><><><><><><><><><><><><><><><>
