/**
 * File:      mx_helpers.cpp
 *
 * Author:    Modtronix Engineering - www.modtronix.com
 *
 * Description:
 *
 * Software License Agreement:
 * This software has been written or modified by Modtronix Engineering. The code
 * may be modified and can be used free of charge for commercial and non commercial
 * applications. If this is modified software, any license conditions from original
 * software also apply. Any redistribution must include reference to 'Modtronix
 * Engineering' and web link(www.modtronix.com) in the file header.
 *
 * THIS SOFTWARE IS PROVIDED IN AN 'AS IS' CONDITION. NO WARRANTIES, WHETHER EXPRESS,
 * IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE
 * COMPANY SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
 * CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 */
#include "mbed.h"

#define DEBUG_ENABLE            1
#include "mx_default_debug.h"

//Includes that use debugging - Include AFTER debug defines/includes above! It uses them for debugging!
#include "mx_helpers.h"

uint8_t MxHelpers::ascii_hex_to_byte(WORD_VAL asciiChars) {
    // Convert lowercase to uppercase
    if (asciiChars.v[1] > 'F')
        asciiChars.v[1] -= 'a' - 'A';
    if (asciiChars.v[0] > 'F')
        asciiChars.v[0] -= 'a' - 'A';

    // Convert 0-9, A-F to 0x0-0xF
    if (asciiChars.v[1] > '9')
        asciiChars.v[1] -= 'A' - 10;
    else
        asciiChars.v[1] -= '0';

    if (asciiChars.v[0] > '9')
        asciiChars.v[0] -= 'A' - 10;
    else
        asciiChars.v[0] -= '0';

    // Concatenate
    return (asciiChars.v[1] << 4) | asciiChars.v[0];
}

uint8_t MxHelpers::ascii_hex_nibble_to_byte(uint8_t c) {
    if ((c <= '9') && (c >= '0'))
        return (c - '0');
    else if ((c <= 'F') && (c >= 'A'))
        return (c - 55);
    else if ((c <= 'f') && (c >= 'a'))
        return (c - 87);

    return 0xff;    //Indicate given byte was NOT alpha numerical
}


uint16_t MxHelpers::cvt_ascii_dec_to_uint16(const char* str, uint8_t* retFlags) {
    uint16_t val = 0;   //Returned value
    uint8_t ret = 0;   //Return retFlags value
    bool isHex = 0;
    const char* savedStrPtr;

    savedStrPtr = str;

    //MX_DEBUG("\nparseHexDecWord()");

    //Does the string contain a Upper Case Hex value
    if (*str == 'x') {
        str++;
        isHex = true;
        //MX_DEBUG(" -d");
    }

    //Parse all '0'-'9', and 'A'-'F' bytes
    while ((((*str >= '0') && (*str <= '9'))
            || ((*str >= 'A') && (*str <= 'F')))) {
        if (isHex) {
            val = (val << 4)
                    + ((*str <= '9') ? (*str - '0') : (*str - 'A' + 10));
        } else {
            val = (val * 10) + ((*str) - '0');
        }
        str++;
    }

    //Get the number of bytes parsed
    ret = str - savedStrPtr;
    if (retFlags != NULL) {
        *retFlags = ret;
    }

    return val;
}


uint16_t MxHelpers::cvt_ascii_hex_to_uint16(const char* str, uint8_t* retFlags) {
    uint16_t retVal = 0;    //Returned value
    uint8_t ret = 0;        //Return retFlags value
    bool isDecimal = 0;
    const char* savedStrPtr;

    savedStrPtr = str;

    //MX_DEBUG("\nparseHexDecWord()");

    //Does the string contain a decimal value
    if (*str == 'd') {
        str++;
        isDecimal = true;
        //MX_DEBUG(" -d");
    }

    //Parse all '0'-'9', and 'A'-'F' bytes
    while ((((*str >= '0') && (*str <= '9'))
            || ((*str >= 'A') && (*str <= 'F')))) {
        if (isDecimal) {
            retVal = (retVal * 10) + ((*str) - '0');
        } else {
            retVal = (retVal << 4)
                    + ((*str <= '9') ? (*str - '0') : (*str - 'A' + 10));
        }
        str++;
    }

    //Get the number of bytes parsed
    ret = str - savedStrPtr;
    if (retFlags != NULL) {
        *retFlags = ret;
    }

    return retVal;
}


uint8_t MxHelpers::cvt_uint16_to_ascii_str(uint16_t val, uint8_t* buf) {
    uint8_t i;
    uint8_t retVal = 0;
    uint16_t digit;
    uint16_t divisor;
    bool printed = false;
    //const uint16_t NZ_UITOA_DIV[4] = { 1, 10, 100, 1000 };
    //static const uint16_t NZ_UITOA_DIV[4] = { 1, 10, 100, 1000 };

    if (val) {
        divisor = 10000;
        for (i = 5; i != 0; i--) {
            digit = val / divisor;
            if ((digit!=0) || printed) {
                *buf++ = '0' + digit;
                val -= digit * divisor;
                printed = true;
                retVal++;
            }
            divisor = divisor / 10;

            //Alternative method
            //divisor = NZ_UITOA_DIV[i - 2];

            //Alternative method
            //if (i==5)
            //    divisor = 1000;
            //else if (i==4)
            //    divisor = 100;
            //else if (i==3)
            //    divisor = 10;
            //else
            //    divisor = 1;
        }
    } else {
        *buf++ = '0';
        retVal++;
    }

    *buf = '\0';

    return retVal;
}




