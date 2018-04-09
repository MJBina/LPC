// <><><><><><><><><><><><><> batt_temp.c <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//  Copyright(C) 2016 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------

// -------
// headers
// --------
#include "options.h"    // must be first include
#include "analog.h"
#include "batt_temp.h"
#include "device.h"
#include "rv_can.h"
#include "rvc_types.h"


//-----------------------------------------------------------------------------
//                        A L L     M O D E L S
//-----------------------------------------------------------------------------

// Evaluate Battery Temperature Reading and detect problems
void BattTemp_Evaluate()
{
	if(!Device.config.batt_temp_sense_present)
	{
		Device.status.batt_temp           = BATT_TEMP_NOMINAL;
		Device.error.batt_temp_snsr_open  = 0; 
		Device.error.batt_temp_snsr_short = 0; 
	}
	else
	{
        Device.error.batt_temp_snsr_open  = (BattTempLongAvg() >= BATT_TEMP_SNSR_OPEN_THRES)  ? 1 : 0; 
        Device.error.batt_temp_snsr_short = (BattTempLongAvg() <= BATT_TEMP_SNSR_SHORT_THRES) ? 1 : 0;
        Device.status.batt_temp           = BattTemp_celsius();
    }
}


//-----------------------------------------------------------------------------
//  Magnum Battery Temperature sensor interface
//
//	BATTERY TEMPERATURE COMPENSATION FACTOR
//
//	NOTE: RV-C provides for a "Temperature Compensation Constant" in mV/°K.  
//	Refer to CHARGER_CONFIGURATION_STATUS_3, DGN # 0x01FECC.
//	However, this is normally experessed as a rate per-cell, and I have not 
//	found a configuration parameter describing the number of cells in the battery.
//
//	Temp-Comp for a 6-cell) 12V battery = 6 * 5.04mv/°C = 30.24 mV/°C
//-----------------------------------------------------------------------------

// ------------------------------
// File Conditionally Compiled
// ------------------------------
#if (defined(OPTION_CHARGE_LION)) 
  #ifndef WIN32
	#warning Temperature compensation should not be used for Li-Ion
  #endif
	// --------------------------------------------------------
	//	NOTE: Some Li-ion packs may experience a temperature rise of about 
	//	5�C (9�F) when reaching full charge. This could be due to the protection 
	//	circuitry and/or elevated internal resistance. Discontinue using the 
	//	battery or charger if the temperature rises more than 10ºC (18ºF) under 
	//	moderate charging speeds.
	// --------------------------------------------------------
    int16_t BattTemp_celsius(void)
    {
        return(23); // TODO return room temperature for now
    }
#else

// --------------------------------------------------------
//  Normal operation - using the Magnum temperature sensor
// --------------------------------------------------------

//	Temperature Compensation is based upon the the number of cells comprising 
//	the battery.  The look-up table below is based upon a 6-cell battery. 
//	'NUM_CELLS_SCALE' defined below, adjusts the
//	the offsets for the appropriate number of cells.
#define _NUM_CELLS	(6)

#if IS_DCIN_12V		//	12V Battery has 6-cells, scaling is 1X
    #define NUM_CELLS_SCALE 	(1)
#elif IS_DCIN_24V
    #define NUM_CELLS_SCALE 	(2)
#elif IS_DCIN_48V
    #define NUM_CELLS_SCALE 	(4)
#elif IS_DCIN_51V
    #define NUM_CELLS_SCALE 	(51.0/12.0)
#endif

//-----------------------------------------------------------------------------
#pragma pack(1)  // structure packing on byte alignment
typedef struct 
{
    ADC10_RDG_t temp_a2d;
    int16_t 	temp_celsius;
    int16_t		vbatt_adc_ofst;
} BATT_TEMP_CONV_t;
#pragma pack()  // restore packing setting

// Look-up table to convert adc value to temperature
static const BATT_TEMP_CONV_t BattTempTable[] =
{
	//	This table was created from the temperature data that Gary supplied, 
	//	by doing a piece-wise linear fit to create the intermediate A2D data 
	//	points for the temperature data.  Then calculating the Voltage offset.

	// adc, tempC,   temp compensation offset
	{  65,  105,  (int16_t)(VBATT_SLOPE*(-2.40)*NUM_CELLS_SCALE) }, 
	{  67,  104,  (int16_t)(VBATT_SLOPE*(-2.37)*NUM_CELLS_SCALE) }, 
	{  70,  103,  (int16_t)(VBATT_SLOPE*(-2.34)*NUM_CELLS_SCALE) }, 
	{  72,  102,  (int16_t)(VBATT_SLOPE*(-2.31)*NUM_CELLS_SCALE) }, 
	{  75,  101,  (int16_t)(VBATT_SLOPE*(-2.28)*NUM_CELLS_SCALE) }, 
	{  77,  100,  (int16_t)(VBATT_SLOPE*(-2.25)*NUM_CELLS_SCALE) }, 
	{  80,   99,  (int16_t)(VBATT_SLOPE*(-2.22)*NUM_CELLS_SCALE) }, 
	{  82,   98,  (int16_t)(VBATT_SLOPE*(-2.19)*NUM_CELLS_SCALE) }, 
	{  85,   97,  (int16_t)(VBATT_SLOPE*(-2.16)*NUM_CELLS_SCALE) }, 
	{  87,   96,  (int16_t)(VBATT_SLOPE*(-2.13)*NUM_CELLS_SCALE) }, 
	{  90,   95,  (int16_t)(VBATT_SLOPE*(-2.10)*NUM_CELLS_SCALE) }, 
	{  92,   94,  (int16_t)(VBATT_SLOPE*(-2.07)*NUM_CELLS_SCALE) }, 
	{  95,   93,  (int16_t)(VBATT_SLOPE*(-2.04)*NUM_CELLS_SCALE) }, 
	{  97,   92,  (int16_t)(VBATT_SLOPE*(-2.01)*NUM_CELLS_SCALE) }, 
	{  99,   91,  (int16_t)(VBATT_SLOPE*(-1.98)*NUM_CELLS_SCALE) }, 
	{ 102,   90,  (int16_t)(VBATT_SLOPE*(-1.95)*NUM_CELLS_SCALE) }, 
	{ 104,   89,  (int16_t)(VBATT_SLOPE*(-1.92)*NUM_CELLS_SCALE) }, 
	{ 107,   88,  (int16_t)(VBATT_SLOPE*(-1.89)*NUM_CELLS_SCALE) }, 
	{ 109,   87,  (int16_t)(VBATT_SLOPE*(-1.86)*NUM_CELLS_SCALE) }, 
	{ 112,   86,  (int16_t)(VBATT_SLOPE*(-1.83)*NUM_CELLS_SCALE) }, 
	{ 114,   85,  (int16_t)(VBATT_SLOPE*(-1.80)*NUM_CELLS_SCALE) }, 
	{ 117,   84,  (int16_t)(VBATT_SLOPE*(-1.77)*NUM_CELLS_SCALE) }, 
	{ 119,   83,  (int16_t)(VBATT_SLOPE*(-1.74)*NUM_CELLS_SCALE) }, 
	{ 122,   82,  (int16_t)(VBATT_SLOPE*(-1.71)*NUM_CELLS_SCALE) }, 
	{ 124,   81,  (int16_t)(VBATT_SLOPE*(-1.68)*NUM_CELLS_SCALE) }, 
	{ 127,   80,  (int16_t)(VBATT_SLOPE*(-1.65)*NUM_CELLS_SCALE) }, 
	{ 130,   79,  (int16_t)(VBATT_SLOPE*(-1.62)*NUM_CELLS_SCALE) }, 
	{ 135,   78,  (int16_t)(VBATT_SLOPE*(-1.59)*NUM_CELLS_SCALE) }, 
	{ 139,   77,  (int16_t)(VBATT_SLOPE*(-1.56)*NUM_CELLS_SCALE) }, 
	{ 144,   76,  (int16_t)(VBATT_SLOPE*(-1.53)*NUM_CELLS_SCALE) }, 
	{ 148,   75,  (int16_t)(VBATT_SLOPE*(-1.50)*NUM_CELLS_SCALE) }, 
	{ 152,   74,  (int16_t)(VBATT_SLOPE*(-1.47)*NUM_CELLS_SCALE) }, 
	{ 157,   73,  (int16_t)(VBATT_SLOPE*(-1.44)*NUM_CELLS_SCALE) }, 
	{ 161,   72,  (int16_t)(VBATT_SLOPE*(-1.41)*NUM_CELLS_SCALE) }, 
	{ 166,   71,  (int16_t)(VBATT_SLOPE*(-1.38)*NUM_CELLS_SCALE) }, 
	{ 170,   70,  (int16_t)(VBATT_SLOPE*(-1.35)*NUM_CELLS_SCALE) }, 
	{ 175,   69,  (int16_t)(VBATT_SLOPE*(-1.32)*NUM_CELLS_SCALE) }, 
	{ 179,   68,  (int16_t)(VBATT_SLOPE*(-1.29)*NUM_CELLS_SCALE) }, 
	{ 184,   67,  (int16_t)(VBATT_SLOPE*(-1.26)*NUM_CELLS_SCALE) }, 
	{ 188,   66,  (int16_t)(VBATT_SLOPE*(-1.23)*NUM_CELLS_SCALE) }, 
	{ 193,   65,  (int16_t)(VBATT_SLOPE*(-1.20)*NUM_CELLS_SCALE) }, 
	{ 198,   64,  (int16_t)(VBATT_SLOPE*(-1.17)*NUM_CELLS_SCALE) }, 
	{ 204,   63,  (int16_t)(VBATT_SLOPE*(-1.14)*NUM_CELLS_SCALE) }, 
	{ 210,   62,  (int16_t)(VBATT_SLOPE*(-1.11)*NUM_CELLS_SCALE) }, 
	{ 216,   61,  (int16_t)(VBATT_SLOPE*(-1.08)*NUM_CELLS_SCALE) }, 
	{ 222,   60,  (int16_t)(VBATT_SLOPE*(-1.05)*NUM_CELLS_SCALE) }, 
	{ 228,   59,  (int16_t)(VBATT_SLOPE*(-1.02)*NUM_CELLS_SCALE) }, 
	{ 235,   58,  (int16_t)(VBATT_SLOPE*(-0.99)*NUM_CELLS_SCALE) }, 
	{ 241,   57,  (int16_t)(VBATT_SLOPE*(-0.96)*NUM_CELLS_SCALE) }, 
	{ 248,   56,  (int16_t)(VBATT_SLOPE*(-0.93)*NUM_CELLS_SCALE) }, 
	{ 254,   55,  (int16_t)(VBATT_SLOPE*(-0.90)*NUM_CELLS_SCALE) }, 
	{ 262,   54,  (int16_t)(VBATT_SLOPE*(-0.87)*NUM_CELLS_SCALE) }, 
	{ 269,   53,  (int16_t)(VBATT_SLOPE*(-0.84)*NUM_CELLS_SCALE) }, 
	{ 277,   52,  (int16_t)(VBATT_SLOPE*(-0.81)*NUM_CELLS_SCALE) }, 
	{ 284,   51,  (int16_t)(VBATT_SLOPE*(-0.78)*NUM_CELLS_SCALE) }, 
	{ 292,   50,  (int16_t)(VBATT_SLOPE*(-0.75)*NUM_CELLS_SCALE) }, 
	{ 300,   49,  (int16_t)(VBATT_SLOPE*(-0.72)*NUM_CELLS_SCALE) }, 
	{ 308,   48,  (int16_t)(VBATT_SLOPE*(-0.69)*NUM_CELLS_SCALE) }, 
	{ 317,   47,  (int16_t)(VBATT_SLOPE*(-0.66)*NUM_CELLS_SCALE) }, 
	{ 325,   46,  (int16_t)(VBATT_SLOPE*(-0.63)*NUM_CELLS_SCALE) }, 
	{ 333,   45,  (int16_t)(VBATT_SLOPE*(-0.60)*NUM_CELLS_SCALE) }, 
	{ 342,   44,  (int16_t)(VBATT_SLOPE*(-0.57)*NUM_CELLS_SCALE) }, 
	{ 351,   43,  (int16_t)(VBATT_SLOPE*(-0.54)*NUM_CELLS_SCALE) }, 
	{ 360,   42,  (int16_t)(VBATT_SLOPE*(-0.51)*NUM_CELLS_SCALE) }, 
	{ 369,   41,  (int16_t)(VBATT_SLOPE*(-0.48)*NUM_CELLS_SCALE) }, 
	{ 379,   40,  (int16_t)(VBATT_SLOPE*(-0.45)*NUM_CELLS_SCALE) }, 
	{ 388,   39,  (int16_t)(VBATT_SLOPE*(-0.42)*NUM_CELLS_SCALE) }, 
	{ 398,   38,  (int16_t)(VBATT_SLOPE*(-0.39)*NUM_CELLS_SCALE) }, 
	{ 408,   37,  (int16_t)(VBATT_SLOPE*(-0.36)*NUM_CELLS_SCALE) }, 
	{ 418,   36,  (int16_t)(VBATT_SLOPE*(-0.33)*NUM_CELLS_SCALE) }, 
	{ 428,   35,  (int16_t)(VBATT_SLOPE*(-0.30)*NUM_CELLS_SCALE) }, 
	{ 438,   34,  (int16_t)(VBATT_SLOPE*(-0.27)*NUM_CELLS_SCALE) }, 
	{ 448,   33,  (int16_t)(VBATT_SLOPE*(-0.24)*NUM_CELLS_SCALE) }, 
	{ 459,   32,  (int16_t)(VBATT_SLOPE*(-0.21)*NUM_CELLS_SCALE) }, 
	{ 470,   31,  (int16_t)(VBATT_SLOPE*(-0.18)*NUM_CELLS_SCALE) }, 
	{ 481,   30,  (int16_t)(VBATT_SLOPE*(-0.15)*NUM_CELLS_SCALE) }, 
	{ 491,   29,  (int16_t)(VBATT_SLOPE*(-0.12)*NUM_CELLS_SCALE) }, 
	{ 502,   28,  (int16_t)(VBATT_SLOPE*(-0.09)*NUM_CELLS_SCALE) }, 
	{ 513,   27,  (int16_t)(VBATT_SLOPE*(-0.06)*NUM_CELLS_SCALE) }, 
	{ 522,   26,  (int16_t)(VBATT_SLOPE*(-0.03)*NUM_CELLS_SCALE) }, 
	{ 528,   25,  (int16_t)(VBATT_SLOPE*( 0.00)*NUM_CELLS_SCALE) }, 
	{ 539,   24,  (int16_t)(VBATT_SLOPE*( 0.03)*NUM_CELLS_SCALE) }, 
	{ 551,   23,  (int16_t)(VBATT_SLOPE*( 0.06)*NUM_CELLS_SCALE) }, 
	{ 562,   22,  (int16_t)(VBATT_SLOPE*( 0.09)*NUM_CELLS_SCALE) }, 
	{ 574,   21,  (int16_t)(VBATT_SLOPE*( 0.12)*NUM_CELLS_SCALE) }, 
	{ 585,   20,  (int16_t)(VBATT_SLOPE*( 0.15)*NUM_CELLS_SCALE) }, 
	{ 597,   19,  (int16_t)(VBATT_SLOPE*( 0.18)*NUM_CELLS_SCALE) }, 
	{ 608,   18,  (int16_t)(VBATT_SLOPE*( 0.21)*NUM_CELLS_SCALE) }, 
	{ 619,   17,  (int16_t)(VBATT_SLOPE*( 0.24)*NUM_CELLS_SCALE) }, 
	{ 631,   16,  (int16_t)(VBATT_SLOPE*( 0.27)*NUM_CELLS_SCALE) }, 
	{ 642,   15,  (int16_t)(VBATT_SLOPE*( 0.30)*NUM_CELLS_SCALE) }, 
	{ 654,   14,  (int16_t)(VBATT_SLOPE*( 0.33)*NUM_CELLS_SCALE) }, 
	{ 665,   13,  (int16_t)(VBATT_SLOPE*( 0.36)*NUM_CELLS_SCALE) }, 
	{ 677,   12,  (int16_t)(VBATT_SLOPE*( 0.39)*NUM_CELLS_SCALE) }, 
	{ 688,   11,  (int16_t)(VBATT_SLOPE*( 0.42)*NUM_CELLS_SCALE) }, 
	{ 699,   10,  (int16_t)(VBATT_SLOPE*( 0.45)*NUM_CELLS_SCALE) }, 
	{ 710,    9,  (int16_t)(VBATT_SLOPE*( 0.48)*NUM_CELLS_SCALE) }, 
	{ 720,    8,  (int16_t)(VBATT_SLOPE*( 0.51)*NUM_CELLS_SCALE) }, 
	{ 730,    7,  (int16_t)(VBATT_SLOPE*( 0.54)*NUM_CELLS_SCALE) }, 
	{ 740,    6,  (int16_t)(VBATT_SLOPE*( 0.57)*NUM_CELLS_SCALE) }, 
	{ 750,    5,  (int16_t)(VBATT_SLOPE*( 0.60)*NUM_CELLS_SCALE) }, 
	{ 760,    4,  (int16_t)(VBATT_SLOPE*( 0.63)*NUM_CELLS_SCALE) }, 
	{ 770,    3,  (int16_t)(VBATT_SLOPE*( 0.66)*NUM_CELLS_SCALE) }, 
	{ 778,    2,  (int16_t)(VBATT_SLOPE*( 0.69)*NUM_CELLS_SCALE) }, 
	{ 787,    1,  (int16_t)(VBATT_SLOPE*( 0.72)*NUM_CELLS_SCALE) }, 
	{ 795,    0,  (int16_t)(VBATT_SLOPE*( 0.75)*NUM_CELLS_SCALE) }, 
	{ 803,   -1,  (int16_t)(VBATT_SLOPE*( 0.78)*NUM_CELLS_SCALE) },  
	{ 811,   -2,  (int16_t)(VBATT_SLOPE*( 0.81)*NUM_CELLS_SCALE) },  
	{ 819,   -3,  (int16_t)(VBATT_SLOPE*( 0.84)*NUM_CELLS_SCALE) },  
	{ 827,   -4,  (int16_t)(VBATT_SLOPE*( 0.87)*NUM_CELLS_SCALE) },  
	{ 836,   -5,  (int16_t)(VBATT_SLOPE*( 0.90)*NUM_CELLS_SCALE) },  
	{ 844,   -6,  (int16_t)(VBATT_SLOPE*( 0.93)*NUM_CELLS_SCALE) },  
	{ 852,   -7,  (int16_t)(VBATT_SLOPE*( 0.96)*NUM_CELLS_SCALE) },  
	{ 860,   -8,  (int16_t)(VBATT_SLOPE*( 0.99)*NUM_CELLS_SCALE) },  
	{ 868,   -9,  (int16_t)(VBATT_SLOPE*( 1.02)*NUM_CELLS_SCALE) },  
	{ 876,  -10,  (int16_t)(VBATT_SLOPE*( 1.05)*NUM_CELLS_SCALE) },  
	{ 884,  -11,  (int16_t)(VBATT_SLOPE*( 1.08)*NUM_CELLS_SCALE) },  
	{ 892,  -12,  (int16_t)(VBATT_SLOPE*( 1.11)*NUM_CELLS_SCALE) },  
	{ 900,  -13,  (int16_t)(VBATT_SLOPE*( 1.14)*NUM_CELLS_SCALE) },  
	{ 905,  -14,  (int16_t)(VBATT_SLOPE*( 1.17)*NUM_CELLS_SCALE) },  
	{ 910,  -15,  (int16_t)(VBATT_SLOPE*( 1.20)*NUM_CELLS_SCALE) },  
	{ 914,  -16,  (int16_t)(VBATT_SLOPE*( 1.23)*NUM_CELLS_SCALE) },  
	{ 918,  -17,  (int16_t)(VBATT_SLOPE*( 1.26)*NUM_CELLS_SCALE) },  
	{ 922,  -18,  (int16_t)(VBATT_SLOPE*( 1.29)*NUM_CELLS_SCALE) },  
	{ 927,  -19,  (int16_t)(VBATT_SLOPE*( 1.32)*NUM_CELLS_SCALE) },  
	{ 931,  -20,  (int16_t)(VBATT_SLOPE*( 1.35)*NUM_CELLS_SCALE) },  
	{ 935,  -21,  (int16_t)(VBATT_SLOPE*( 1.38)*NUM_CELLS_SCALE) },  
	{ 940,  -22,  (int16_t)(VBATT_SLOPE*( 1.41)*NUM_CELLS_SCALE) },  
	{ 944,  -23,  (int16_t)(VBATT_SLOPE*( 1.44)*NUM_CELLS_SCALE) },  
	{ 948,  -24,  (int16_t)(VBATT_SLOPE*( 1.47)*NUM_CELLS_SCALE) },  
	{ 952,  -25,  (int16_t)(VBATT_SLOPE*( 1.50)*NUM_CELLS_SCALE) },  
	{ 957,  -26,  (int16_t)(VBATT_SLOPE*( 1.53)*NUM_CELLS_SCALE) },  
	{ 961,  -27,  (int16_t)(VBATT_SLOPE*( 1.56)*NUM_CELLS_SCALE) }   
};

#define NUM_BATT_TEMP_CONV  ARRAY_LENGTH(BattTempTable)

//-----------------------------------------------------------------------------
// find the index into BattTempTable[index] corresponding to adc value
// uses binary search

uint16_t BattTemp_Index(uint16_t adc)
{
    uint16_t first, last, middle;
    
    first = 0;
    if (adc <= BattTempTable[first].temp_a2d) return(0);
    last = NUM_BATT_TEMP_CONV - 1;
    if (adc >= BattTempTable[last].temp_a2d) return(last);
    middle = (first+last)/2;
    
    // binary saarch
    while (first < last)
    {
        if (BattTempTable[middle].temp_a2d < adc)
        {
            first = middle + 1;    
        }
        else if (BattTempTable[middle].temp_a2d == adc)
        {
           	return(middle);
        }
        else
        {
            last = middle - 1;
        }
        
        middle = (first + last)/2;
    } // while

    if (first && BattTempTable[first].temp_a2d > adc)
       	first--; // use the smaller value
    return(first);   
}

//-----------------------------------------------------------------------------
// returns the Battery Temperature in Celsius based upon long average
int16_t BattTemp_celsius(void)
{
    int16_t ix;
	
	if(!IsCycleAvgValid()) return(BATT_TEMP_NOMINAL);
	ix = BattTemp_Index(BattTempLongAvg());
    return(BattTempTable[ix].temp_celsius);
}

//------------------------------------------------------------------------------
// returns the battery voltage compensation in A2D counts. 
// This is to be added to the setpoint value (in A2D counts).
ADC10_RDG_t BattTemp_VBattCompA2d(void)
{
    int16_t ix;

	if(!IsCycleAvgValid()) return(0);
    if(!Device.config.batt_temp_sense_present) return(0); // no temp sensor; no temp comp
	ix = BattTemp_Index(BattTempLongAvg());
	return(BattTempTable[ix].vbatt_adc_ofst);
}

//------------------------------------------------------------------------------
// returns 0=ok, else count of errors
/*
int16_t BattTemp_UnitTest()
{
	uint16_t ix, i;
	int16_t  rc=0;

	ix = BattTemp_Index(0);
	if (ix != 0)
		{ rc++; LOG(SS_SYS, SV_ERR, "BattTemp_Index(0) Failed" ); }
	ix = BattTemp_Index(63);
	if (ix != 0)
		{ rc++; LOG(SS_SYS, SV_ERR, "BattTemp_Index(63) Failed" ); }
	ix = BattTemp_Index(64);
	if (ix != 0)
		{ rc++; LOG(SS_SYS, SV_ERR, "BattTemp_Index(64) Failed" ); }
	ix = BattTemp_Index(65);
	if (ix != 0)
		{ rc++; LOG(SS_SYS, SV_ERR, "BattTemp_Index(65) Failed" ); }
	ix = BattTemp_Index(960);
	if (ix != NUM_BATT_TEMP_CONV-2) 
		{ rc++; LOG(SS_SYS, SV_ERR, "BattTemp_Index(960) Failed" ); }

	// try all exact values
	for (i=0; i<NUM_BATT_TEMP_CONV; i++)
	{
		ix = BattTemp_Index(BattTempTable[i].temp_a2d);
		if (ix != i)
			{ rc++; LOG(SS_SYS, SV_ERR, "BattTemp_Index(i) Failed" ); }
	} // for

	// try all values+1
	for (i=0; i<NUM_BATT_TEMP_CONV; i++)
	{
		ix = BattTemp_Index(BattTempTable[i].temp_a2d+1);
		if (ix != i)
			{ rc++; LOG(SS_SYS, SV_ERR, "BattTemp_Index(i+1) Failed" ); }
	} // for

	return(rc);
}
*/

#endif	// OPTION_CHARGE_LION

// <><><><><><><><><><><><><> batt_temp.c <><><><><><><><><><><><><><><><><><><><><><>
