/**
 * File:      mx_helpers.h
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
#ifndef MX_HELPERS_H_
#define MX_HELPERS_H_


// Debugging //////////////////////////////////////////////////////////////////
// To enable debug output from this file, define MX_DEBUG and DEBUG_ENABLE_MX_HELPERS before
// including this file.
//
//Defines for MXH_DEBUG - debugging for include file
#if !defined(DEBUG_ENABLE_MX_HELPERS)
    #define DEBUG_ENABLE_MX_HELPERS          0
#endif
#if !defined(DEBUG_ENABLE_INFO_MX_HELPERS)
    #define DEBUG_ENABLE_INFO_MX_HELPERS     0
#endif

#if !defined(MXH_DEBUG)
    #if defined(MX_DEBUG) && (DEBUG_ENABLE_MX_HELPERS==1)
        #define MXH_DEBUG MX_DEBUG
    #else
        #define MXH_DEBUG(format, args...) ((void)0)
    #endif
#endif

#if !defined(MXH_DEBUG_INFO)
    #if defined(MX_DEBUG) && (DEBUG_ENABLE_MX_HELPERS==1) && (DEBUG_ENABLE_INFO_MX_HELPERS==1)
        #define MXH_DEBUG_INFO MX_DEBUG
    #else
        #define MXH_DEBUG_INFO(format, args...) ((void)0)
    #endif
#endif


typedef union
{
    uint16_t    Val;
    uint8_t     v[2] __packed;
    struct
    {
        uint8_t LB;
        uint8_t HB;
    } byte;
} WORD_VAL;


#define GET_U8_AT0(val)  MxHelpers::get_u8_at0(val)
#define GET_U8_AT1(val)  MxHelpers::get_u8_at1(val)
#define GET_U8_AT2(val)  MxHelpers::get_u8_at2(val)
#define GET_U8_AT3(val)  MxHelpers::get_u8_at3(val)

#define SET_U8_AT0(val)  MxHelpers::set_u8_at0(val)
#define SET_U8_AT1(val)  MxHelpers::set_u8_at1(val)
#define SET_U8_AT2(val)  MxHelpers::set_u8_at2(val)
#define SET_U8_AT3(val)  MxHelpers::set_u8_at3(val)


class MxHelpers {
public:
    /** Get first(LSB) 8-bit byte(uint8_t) of given 16-bit Word
     * @return Returns the lower(LSB) byte of given value
     */
    static inline uint8_t get_u8_at0(uint16_t val) {
        return ((uint8_t)((val)&0xFF));
    }


    /** Get second(MSB) 8-bit byte(uint8_t) of given 16-bit Word.
     * @return Returns the upper(MSB) byte of given value
     */
    static inline uint8_t get_u8_at1(uint16_t val) {
        return ((uint8_t)(((val)>>8)&0xFF));
    }


    /** Get first(LSB) 8-bit byte(uint8_t) of given 32-bit Word
     * @return Returns the lower(LSB) byte of given value = byte at bit offset 0 to 7
     */
    static inline uint8_t get_u8_at0(uint32_t val) {
        return ((uint8_t)((val)&0xFF));
    }


    /** Get second 8-bit byte(uint8_t) of given 32-bit Word
     * @return Returns the second byte of given value = byte at bit offset 8 to 15
     */
    static inline uint8_t get_u8_at1(uint32_t val) {
        return ((uint8_t)(((val)>>8)&0xFF));
    }


    /** Get third 8-bit byte(uint8_t) of given 32-bit Word
     * @return Returns the third byte of given value = byte at bit offset 16 to 23
     */
    static inline uint8_t get_u8_at2(uint32_t val) {
        return ((uint8_t)(((val)>>16)&0xFF));
    }


    /** Get forth(MSB) 8-bit byte(uint8_t) of given 32-bit Word
     * @return Returns the MSB byte of given value = byte at bit offset 24 to 31
     */
    static inline uint8_t get_u8_at3(uint32_t val) {
        return ((uint8_t)(((val)>>24)&0xFF));
    }


    /** Write given byte to first(LSB) 8-bit byte(uint8_t) of given 16-bit Word
     * @param src Source 8-bit byte to write to destination
     * @param dest Destination
     */
    static inline void set_u8_at0(uint8_t src, uint16_t dest) {
        ((uint8_t*)&dest)[0] = src;
    }


    /** Write given byte to MSB 8-bit byte(uint8_t) of given 16-bit Word
     * @param src Source 8-bit byte to write to destination
     * @param dest Destination
     */
    static inline void set_u8_at1(uint8_t src, uint16_t dest) {
        ((uint8_t*)&dest)[1] = src;
    }


    /** Write given byte to first(LSB) 8-bit byte(uint8_t) of given 32-bit Word
     * @param src Source 8-bit byte to write to destination
     * @param dest Destination
     */
    static inline void set_u8_at0(uint8_t src, uint32_t dest) {
        ((uint8_t*)&dest)[0] = src;
    }


    /** Write given byte to second 8-bit byte(uint8_t) of given 32-bit Word
     * @param src Source 8-bit byte to write to destination
     * @param dest Destination
     */
    static inline void set_u8_at1(uint8_t src, uint32_t dest) {
        ((uint8_t*)&dest)[1] = src;
    }


    /** Write given byte to third 8-bit byte(uint8_t) of given 32-bit Word
     * @param src Source 8-bit byte to write to destination
     * @param dest Destination
     */
    static inline void set_u8_at2(uint8_t src, uint32_t dest) {
        ((uint8_t*)&dest)[2] = src;
    }


    /** Write given byte to last(MSB) 8-bit byte(uint8_t) of given 32-bit Word
     * @param src Source 8-bit byte to write to destination
     * @param dest Destination
     */
    static inline void set_u8_at3(uint8_t src, uint32_t dest) {
        ((uint8_t*)&dest)[3] = src;
    }


    /** Get the first fractional part of float. For example, will return 5 for 123.56
     * @return Returns first fractional part of float. For example, will return 5 for 123.56
     */
    static inline uint8_t get_frac1(float val) {
        return (uint8_t)(( val - ((float)((uint32_t)val)) )*10);
    }


    /**
     * Converts a "2 character ASCII hex" string to a single packed byte.
     *
     * @param asciiChars - WORD_VAL where
     *          asciiChars.v[0] is the ASCII value for the lower nibble and
     *          asciiChars.v[1] is the ASCII value for the upper nibble.
     *          Each must range from '0'-'9', 'A'-'F', or 'a'-'f'.
     *
     * @return Returns byte representation of given "2 character ASCII byte"
     */
    static uint8_t ascii_hex_to_byte(WORD_VAL asciiChars);


    /**
     * Get the hex value for the given hex encoded character (0-9, a-f).
     * The returned value will be from 0 - 15. If the given byte is not a hex encoded
     * character, 0xff is returned!
     *
     * @param c A hex encoded character, value values are 0-9, A-F and a-f
     *
     * @return Returns a value from 0 to 15 representing the hex value of given parameter. Or, 0xff if error.
     */
    static uint8_t ascii_hex_nibble_to_byte(uint8_t c);

    /**
     * Converts a byte to a "2-character uppercase ASCII hex" value. The 2 bytes
     * are returned in the bytes of the returned WORD_VAL.
     * For example, nzByteToAsciiHex(0xAE) will return 'E' in the LSB, and 'A' in
     * the MSB of the returned uint16_t.
     *
     * @param b The byte containing the nibble to convert
     *
     * @return The LSB of the returned word contains the ASCII value for the lower nibble.
     *         The MSB of the returned word contains the ASCII value for the upper nibble.
     */
    static inline uint16_t byte_to_ascii_hex(uint8_t b) {
        WORD_VAL w;
        w.byte.LB = low_nibble_to_ascii_hex(b);
        w.byte.HB = high_nibble_to_ascii_hex(b);
        return w.Val;
    }

    /**
     * Converts a byte to a "2-character uppercase ASCII hex" string.
     * For example, nzByteToAsciiHex(0xAE) will return the string "AE" (LSB='E').
     * Will always return 2 characters, for example 0x5 will return "05".
     * A NULL termination is NOT added to the back of the returned 2 characters.
     *
     * @param b The byte containing the nibble to convert
     *
     * @param dst Pointer to buffer to write the result to. Will always write 2
     *      characters, without a NULL termination.
     */
    static inline void byte_to_ascii_hex_str(uint8_t b, char* dst) {
        dst[0] = high_nibble_to_ascii_hex(b);
        dst[1] = low_nibble_to_ascii_hex(b);
    }

    /**
     * Converts the upper nibble of a binary value to a hexadecimal ASCII byte.
     * For example, nzHighNibbleToAsciiHex(0xAE) will return 'A'.
     * Same as Microchip "btohexa_high()" function
     *
     * @param b The byte containing the nibble to convert
     *
     * @return The converted ASCII hex character for the given nibble
     */
    static inline uint8_t high_nibble_to_ascii_hex(uint8_t b) {
        b >>= 4;
        return (b > 0x9u) ? b + 'A' - 10 : b + '0';
    }

    /**
     * Converts the lower nibble of a binary value to a hexadecimal ASCII byte.
     * For example, nzLowNibbleToAsciiHex(0xAE) will return 'E'.
     * Same as Microchip "btohexa_low()" function
     *
     * @param b The byte containing the nibble to convert
     *
     * @return The converted ASCII hex character for the given nibble
     */
    static inline uint8_t low_nibble_to_ascii_hex(uint8_t b) {
        b &= 0x0F;
        return (b > 9u) ? b + 'A' - 10 : b + '0';
    }


    /** Counts number of set bits
     *
     * @param val: Given parameter to count bit's that are 1
     * @return Number of set bits
     */
    static inline uint8_t count_ones(uint32_t val) {
        return __builtin_popcount(val);     //Use GCC built in functions, see http://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html
    }

    /**
     * Counts number of bits in given array that are set to 1
     *
     * @param p: Pointer to array whos bits we must count
     * @param size: Number of bytes in array we must count bits of
     */
    static inline uint8_t count_ones_arr(uint8_t* p, uint8_t size) {
        uint8_t offset;
        uint8_t x;
        uint8_t count = 0;

        for (offset = 0; offset < size; offset++) {
            x = *p++;
            for (count = 0; x != 0; x >>= 1) {
                if (x & 0x01)
                    count++;
            }
        }
        return count;
    }


    /** Count leading zeros
     * This function counts the number of leading zeros of a data value.
     *
     * @param val Value to count the leading zeros
     * @return number of leading zeros in value
     */
    static inline uint8_t count_leading_zeros(uint32_t val) {
        return __CLZ(val);
        //return  __builtin_clz(val);     //Use GCC built in functions, see http://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html
    }

    /** Count trailing zeros
     * This function counts the number of trailing zeros of a data value.
     * See https://en.wikipedia.org/wiki/Find_first_set
     *
     * @param val Value to count the trailing zeros
     * @return number of trailing zeros in value
     */
    static inline uint8_t count_trailing_zeros(uint32_t val) {
        //return  __builtin_ctz(val);     //Use GCC built in functions, see http://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html
        return (__CLZ(__RBIT(val)));
    }

    /** Count leading ones
     * This function counts the number of leading ones of a data value.
     * See https://en.wikipedia.org/wiki/Find_first_set
     *
     * @param val Value to count the leading ones
     * @return number of leading ones in value
     */
    static inline uint8_t count_leading_ones(uint32_t val) {
        return __CLZ(~val);
        //return  __builtin_clz(~val);     //Use GCC built in functions, see http://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html
    }

    /** Count trailing ones
     * This function counts the number of trailing ones of a data value.
     * See https://en.wikipedia.org/wiki/Find_first_set
     *
     * @param val Value to count the leading ones
     * @return number of leading ones in value
     */
    static inline uint8_t count_trailing_ones(uint32_t val) {
        //return  __builtin_ctz(~val);     //Use GCC built in functions, see http://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html
        return (__CLZ(__RBIT(~val)));
    }

    /** Reverse bit order of value
     * This function reverses the bit order of the given value.
     *
     * @param val Value to reverse bits of
     * @return Reversed value
     */
    static inline uint32_t reverse_bits(uint32_t val) {
        return __RBIT(val);
    }


    /**
     * Get the value of the given decimal string. String value can have
     * leading 0s. Maximum value is 65,535. Following possible leading format specifiers are
     * checked for:
     * - x = Upper Case Hex value to follow
     *
     * Some examples:
     * "255" will return 0x00FF
     * "x010" will return 0x0010
     *
     * @param str String containing hex value
     *
     * @param retFlags A value returned from this function.
     *        - bit 0-3 is the number of bytes parsed by this function
     *        - bits 4-7 are reserved for flags, like if the parsed value is a byte
     *          or word for example.
     *
     * @return Parsed word value
     */
    static uint16_t cvt_ascii_dec_to_uint16(const char* str, uint8_t* retFlags);


    /**
     * Get the value of the given Upper Case Hex, or decimal string. String value can have
     * leading 0s. Maximum value is 0xFFFF. Following possible leading format specifiers are
     * checked for:
     * - d = Decimal value to follow
     *
     * Some examples:
     * "A100" will return 0xA100
     * "d010" will return 0xA
     *
     * @param str String containing hex value
     *
     * @param retFlags A value returned from this function.
     *        - bit 0-3 is the number of bytes parsed by this function
     *        - bits 4-7 are reserved for flags, like if the parsed value is a byte
     *          or word for example.
     *
     * @return Parsed word value
     */
    static uint16_t cvt_ascii_hex_to_uint16(const char* str, uint8_t* retFlags);


    /**
     * Converts a 16-bit unsigned integer to a null-terminated decimal string.
     *
     * @param val - The number to be converted
     *
     * @param buf - Pointer in which to store the converted string
     *
     * @param ret - Returns length of string written to buf. Does NOT include NULL terminator added
     */
    static uint8_t cvt_uint16_to_ascii_str(uint16_t val, uint8_t* buf);


    /**
     * Converts a 16-bit signed integer to a null-terminated decimal string.
     *
     * @param val - The number to be converted
     *
     * @param buf - Pointer in which to store the converted string
     *
     * @param ret - Returns length of string written to buf. Does NOT include NULL terminator added
     */
    static inline uint8_t cvt_int16_to_ascii_str(int16_t val, uint8_t* buf) {
        uint8_t retVal = 0;
        if(val < 0) {
            *buf = '-';
            buf++;
            retVal++;
        }
        return retVal + cvt_uint16_to_ascii_str(abs(val), buf);
    }


    /** Rotate Right with Extend (32 bit)
     * This function moves each bit of a bitstring right by one bit. The carry input is shifted
     * in at the left end of the bitstring.
     *
     * @param value  Value to rotate
     * @return Rotated value
     */
    static inline uint32_t rotate_right(int32_t val) {
        return __RRX(val);
    }

    //Protected Data
protected:

};

#if defined(MXH_DEBUG)
    #undef MXH_DEBUG
#endif
#if defined(MXH_DEBUG_INFO)
    #undef MXH_DEBUG_INFO
#endif

#endif /* MX_HELPERS_H_ */
