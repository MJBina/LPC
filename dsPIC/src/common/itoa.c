// <><><><><><><><><><><><><> itoa.c <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//	Copyright(C) 2015 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
// Binary to ASCII string conversions
//
// These conversion routines are provided for increased speed over
// standard library routines such as sprintf() and itoa()
// Speed is important to keep the state machine runnning smoothly.
//
//-----------------------------------------------------------------------------

// -------
// headers
// -------
#include "options.h"    // must be first include
#include "itoa.h"

//-----------------------------------------------------------------------------
//                    I N T 3 2    T O    A S C I I 
//-----------------------------------------------------------------------------

// convert uint32 to string
// returns buf so function can be called in place
char* ui32toa(char* buf, uint32_t uval)
{
	char prbuf[12];
	int16_t i,j;
	char c;

	// generate the string, but in reverse order
	i = j = 0;
	do
	{
		c = uval % 10;
		uval /= 10;
		if (c >= 10)
			c += 'A'-'0'-10;
		c += '0';
		prbuf[i++] = c;
	} while(uval != 0);

	// output in correct order
	buf[i--] = 0;	// null terminate
	while (i>=0)
	{
		buf[i--] = prbuf[j++];
	}
	return(buf);
}

//-----------------------------------------------------------------------------
// convert int32 to null terminated string
// returns buf so function can be called in place
char* i32toa(char* buf, int32_t ival)
{
	char* cp = buf;

	if (ival < 0)
    {
		*buf++ = '-';
		ival = -ival;
	}
	ui32toa(buf, ival);
	return(cp);
}

//-----------------------------------------------------------------------------
//                    I N T 1 6    T O    A S C I I 
//-----------------------------------------------------------------------------

// convert uint16 to null terminated string
// returns buf so function can be called in place
char* ui16toa(char* buf, uint16_t uval)
{
	char prbuf[7];
	int16_t i,j;
	char c;

	// generate the string, but in reverse order
	i = j = 0;
	do
	{
		c = uval % 10;
		uval /= 10;
		if (c >= 10)
			c += 'A'-'0'-10;
		c += '0';
		prbuf[i++] = c;
	} while(uval != 0);

	// output in correct order
	buf[i--] = 0;	// null terminate
	while (i>=0)
	{
		buf[i--] = prbuf[j++];
	}
	return(buf);
}

//-----------------------------------------------------------------------------
// convert int16 to null terminated string
// returns buf so function can be called in place
char* i16toa(char* buf, int16_t ival)
{
	char* cp = buf;

	if (ival < 0)
    {
		*buf++ = '-';
		ival = -ival;
	}
	ui16toa(buf, ival);
	return(cp);
}

//-----------------------------------------------------------------------------
//                    I N T 1 6    T O    A S C I I 
//                        (No Null Terminator)
//-----------------------------------------------------------------------------

// convert uint16 to string (not null terminated)
// returns buf so function can be called in place
char* ui16toa2(char* buf, uint16_t ival)
{
	char* cp = buf; // save pointer for returning to caller
    char leading_space;
    int16_t digit, divisor;
    int8_t i;

	if (ival == 0)
	{
		*buf = '0';
		return(cp);
	}
	
    // if it's negative, note that and take the absolute value
    if (ival < 0) {
        ival = -ival;
        *buf++ = '-';
    }

    divisor = 10000;
    leading_space = 1;
    for (i=0; i<5; i++)
    {
        digit = ival/divisor;
        if(digit > 0)
        {
            leading_space = 0;
        }
        if(0 == leading_space)
        {
            *buf++ = '0' + digit;
        }
        ival -= (digit * divisor);
        divisor /= 10;
    } // for
	return(cp);

    /* not fixed width as required
	char prbuf[7];
	int16_t i,j;
	char c;

	// generate the string, but in reverse order
	i = j = 0;
	do
	{
		c = uval % 10;
		uval /= 10;
		if (c >= 10)
			c += 'A'-'0'-10;
		c += '0';
		prbuf[i++] = c;
	} while(uval != 0);

	// output in correct order
	i--;	// null terminate
	while (i>=0)
	{
		buf[i--] = prbuf[j++];
	}
	return(buf);
    */
}

//-----------------------------------------------------------------------------
// convert int16 to string (not null terminated)
// returns buf so function can be called in place
char* i16toa2(char* buf, int16_t ival)
{
	char* cp = buf;

	if (ival < 0)
    {
		*buf++ = '-';
		ival = -ival;
	}
	ui16toa(buf, ival);
	return(cp);
}

//-----------------------------------------------------------------------------
//                  F L O A T    T O    A S C I I
//-----------------------------------------------------------------------------
#define MAX_DECIMAL_PLACES  (6)  // 32 bit integer has 10 digits

// convert float value to null terminated string
// returns buf so function can be called in place
char* ftoa2(char* buf, float fval, uint16_t places)
{
    const float fmults[MAX_DECIMAL_PLACES+1] = { 1.e0, 1.e1, 1.e2, 1.e3, 1.e4, 1.e5, 1.e6 };
	char  prbuf[20];
    uint32_t uval;
	int16_t  i=0,j=0;
	char c;

    fval += (float)0.05; // round up for display purposes

    if (places == 0)
    {
        return(i32toa(buf, (int32_t)fval));
    }

    // handle sign
    if (fval < 0) 
    {
        buf[j++] = '-'; // negative sign
        fval = -fval; // make positive
    }

    // calc decimal place multiplier
    if (places > MAX_DECIMAL_PLACES) places = MAX_DECIMAL_PLACES;
    uval = (int32_t)(fval*fmults[places]);

	// generate the string, but in reverse order without decimal place
	do
	{
		c = uval % 10;
		uval /= 10;
		if (c >= 10)
			c += 'A'-'0'-10;
		c += '0';
		prbuf[i++] = c;
	} while(uval != 0);

	// output in correct order integer portion
	i--;  // last index
    while (i>=places)
	{
		buf[j++] = prbuf[i--];
	}
	// output decimal place
	buf[j++] = '.';
	// output fractional portion
	while (i>=0)
	{
		buf[j++] = prbuf[i--];
	}
	buf[j++] = 0;	// null terminate
	return(buf);
}

//-----------------------------------------------------------------------------
//            I N T E G E R    T O    A S C I I    H E X
//-----------------------------------------------------------------------------

const char HexLookup[] = {'0', '1', '2', '3', '4', '5', '6', '7', 
                          '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

//-----------------------------------------------------------------------------
// convert uint32_t to null terminated hex string
// returns buf so function can be called in place
char* ui32tox(char* buf, uint32_t uval)
{
    buf[0] = HexLookup[(uval >> 28) & 0x000F];
    buf[1] = HexLookup[(uval >> 24) & 0x000F];
    buf[2] = HexLookup[(uval >> 20) & 0x000F];
    buf[3] = HexLookup[(uval >> 16) & 0x000F];
    buf[4] = HexLookup[(uval >> 12) & 0x000F];
    buf[5] = HexLookup[(uval >>  8) & 0x000F];
    buf[6] = HexLookup[(uval >>  4) & 0x000F];
    buf[7] = HexLookup[(uval      ) & 0x000F];
    buf[8] = 0;    // null terminate string
    return(buf);
}

//-----------------------------------------------------------------------------
// convert uint16_t to null terminated hex string
// returns buf so function can be called in place
char* ui16tox(char* buf, uint16_t uval)
{
    buf[0] = HexLookup[(uval >> 12) & 0x000F];
    buf[1] = HexLookup[(uval >>  8) & 0x000F];
    buf[2] = HexLookup[(uval >>  4) & 0x000F];
    buf[3] = HexLookup[(uval      ) & 0x000F];
    buf[4] = 0;    // null terminate string
    return(buf);
}

//-----------------------------------------------------------------------------
// convert uint8_t to null terminated hex string
// returns buf so function can be called in place
char* ui8tox(char* buf, uint8_t uval)
{
    buf[0] = HexLookup[(uval >>  4) & 0x0F];
    buf[1] = HexLookup[(uval      ) & 0x0F];
    buf[2] = 0;    // null terminate string
    return(buf);
}

//-----------------------------------------------------------------------------
// returns number of errors
int itoa_UnitTest()
{
	#define  GARBAGE_BYTE   (0x78)  // <0x80 to prevent char conversion error
	#define CLEAR()  memset(buf, GARBAGE_BYTE, sizeof(buf))  // fill with garbage
    char buf[20];
    int nerrors = 0;

    CLEAR(); if (memcmp( ui32toa(buf,                 0),         "0\0",2) !=0) nerrors++;
    CLEAR(); if (memcmp(  i32toa(buf,                 0),         "0\0",2) !=0) nerrors++;
    CLEAR(); if (memcmp( ui16toa(buf,                 0),         "0\0",2) !=0) nerrors++;
    CLEAR(); if (memcmp(  i16toa(buf,                 0),         "0\0",2) !=0) nerrors++;
    CLEAR(); if (memcmp(ui16toa2(buf,                 0),         "0",  1) !=0) nerrors++; if (buf[1] != GARBAGE_BYTE) nerrors++;
    CLEAR(); if (memcmp( i16toa2(buf,                 0),         "0",  1) !=0) nerrors++; if (buf[1] != GARBAGE_BYTE) nerrors++;
    CLEAR(); if (memcmp( ui32tox(buf,                 0),  "00000000\0",9) !=0) nerrors++;
    CLEAR(); if (memcmp( ui16tox(buf,                 0),      "0000\0",5) !=0) nerrors++;
    CLEAR(); if (memcmp(  ui8tox(buf,                 0),        "00\0",3) !=0) nerrors++;
    CLEAR(); if (memcmp(   ftoa2(buf,               0,0),         "0\0",2) !=0) nerrors++;
                                    
    CLEAR(); if (memcmp( ui32toa(buf,  12345678        ),  "12345678\0", 9) !=0) nerrors++;
    CLEAR(); if (memcmp(  i32toa(buf, -98765432        ), "-98765432\0",10) !=0) nerrors++;
    CLEAR(); if (memcmp( ui16toa(buf,  32768           ),     "32768\0", 6) !=0) nerrors++;
    CLEAR(); if (memcmp(  i16toa(buf, -32766           ),    "-32766\0", 7) !=0) nerrors++;
    CLEAR(); if (memcmp(ui16toa2(buf,  32765           ),     "32765",   5) !=0) nerrors++; if (buf[5] != GARBAGE_BYTE) nerrors++;
    CLEAR(); if (memcmp( i16toa2(buf, -2               ),        "-2",   2) !=0) nerrors++; if (buf[2] != GARBAGE_BYTE) nerrors++;
    CLEAR(); if (memcmp( ui32tox(buf,  0x12305678      ),  "12305678\0", 9) !=0) nerrors++;
    CLEAR(); if (memcmp( ui16tox(buf,  0x4320          ),      "4320\0", 5) !=0) nerrors++;
    CLEAR(); if (memcmp(  ui8tox(buf,  0xCD            ),        "CD\0", 3) !=0) nerrors++;
    CLEAR(); if (memcmp(   ftoa2(buf,  1234,          0),      "1234\0", 5) !=0) nerrors++;
    CLEAR(); if (memcmp(   ftoa2(buf, (float)-9.8765, 4),   "-9.8765\0", 8) !=0) nerrors++;
    CLEAR(); if (memcmp(   ftoa2(buf, (float) 3.14159,5),   "3.14159\0", 8) !=0) nerrors++;

	return(nerrors);
}

// <><><><><><><><><><><><><> itoa.c <><><><><><><><><><><><><><><><><><><><><><>
