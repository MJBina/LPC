#include <stdint.h>


/*****************************************************************************
* Function name	: int_sqrt16
* Returns       : uint16_t - result of SQRT function
* Arguments     : uint16_t - value to return the SQRT of
* Description   : This function return the integer SQRT of the passed value
* Notes         : 
*****************************************************************************/
uint16_t int_sqrt16(uint16_t val)
{
    register uint32_t op, res, one;

    op = val;
    res = 0;

    /* "one" starts at the highest power of four <= than the argument. */
    one = 0x4000;  /* second-to-top bit set */
    while (one > op) one >>= 2;

    while (one != 0) {
        if (op >= res + one) {
            op = op - (res + one);
            res = res +  2 * one;
        }
        res >>= 1;
        one >>= 2;
    }
    return(res);

}

/*****************************************************************************
* Function name	: int_sqrt32
* Returns       : uint32_t - result of SQRT function
* Arguments     : uint32_t - value to return the SQRT of
* Description   : This function return the integer SQRT of the passed value
* Notes         : 
*****************************************************************************/
uint32_t int_sqrt32(uint32_t val)
{
    register uint32_t op, res, one;

    op = val;
    res = 0;

    /* "one" starts at the highest power of four <= than the argument. */
    one = 0x40000000;  /* second-to-top bit set */
    while (one > op) one >>= 2;

    while (one != 0) {
        if (op >= res + one) {
            op = op - (res + one);
            res = res +  2 * one;
        }
        res >>= 1;
        one >>= 2;
    }
    return(res);

}