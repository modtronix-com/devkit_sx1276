/**
 * File:      app_helpers.cpp
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
#define DEBUG_ENABLE_INFO       0
#include "mx_default_debug.h"

#include "app_helpers.h"
#include "app_defs.h"            //Application defines, must be first include after debugging includes/defines(in main.cpp)
#include "mx_config.h"
#include "nz32s.h"



// VARIABLES //////////////////////////////////////////////////////////////////
const uint8_t   mxconfDescAppConfig[] = MXCONF_DESC_AppConfig;
const uint8_t   mxconfDescRadioConfig[] = MXCONF_DESC_RadioConfig;



/**
 * Decodes an ASCII formatted command string, and writes result to given destination buffer.
 *
 * The given string is processed until any of the following characters are found:
 * - NULL end of string
 * - ';' End of command character
 * - CR(0x0a='\r') or LF(0x0d='\n') characters
 *
 * A ASCII command is an ASCII Formatted String, with Escape Sequences (Control Characters).
 * It uses 2 upper case characters to represent a single hex character, and can have embedded strings (enclosed
 * within single quotation marks = ') and "Control Characters" (Lower case characters 'a' to 'z')
 *
 * It has the following format:
 * HH   = Two upper case characters representing a single byte (hex value).
 * XHH  = **Not Implemented Yet** Same as HH
 * NDD  = **Not Implemented Yet**  Decimal number. For example "N100" = decimal 100 t0='500'
 * c    = **Not Implemented Yet** Lower case character 'a' to 'z' represents a "control character".
 * '    = Enclose string with single quotes. A double ' character will not end the string, but represents a
 *        single ' character.
 * ^^   = Two "escape characters" represents a single '^' character.
 * ^x   = Use this format to represent a "control character" that is not lower. Lower case characters do not have
 *        to be escaped. Currently not used, but reserved for future use!
 *
 * ===== Examples when NOT using "Escape Character" =====
 * 0156A9BC; is decoded as:
 *  0x01, 0x56, 0xA9, 0xBC
 *
 * BC56'Hello'; is decoded as:
 *  0xBC, 0x56, H, e, l, l, o
 *
 * A1'Hi^'; is decoded as:
 *  0xA1, H, i, ^
 *
 * s6A52p; is decoded as("lower case" control characters NOT supported without "Escape Character"):
 *  0x6A, 0x52
 *
 *
 * ===== Examples when using "Escape Character" '^' =====
 * s6A52p; is decoded as:
 *  ^, s, 0x6A, 0x52, ^, p
 *
 * s50'Hi'p; is decoded as:
 *  ^, s, 0x50, H, i, ^, p
 *
 * 'Hi^'; is decoded as:
 *  H, i, ^, ^
 *
 *
 * @param pDst Destination buffer.
 *
 * @param bufSize Maximum size to write to destination buffer
 *
 * @param pSrc Source NULL terminated string.
 *
 * @param escChar If 0, escape processing is NOT used. Else, it gives the escape character.
 *      Recommended "Escape Character" is '^'
 *
 * @return Returns number of bytes added to buffer.
 *         Returns 0 if destination buffer did not have enough space to add string. In this
 *         case, nothing is added to the buffer.
 */
uint16_t decodeAsciiCmd(uint8_t* pDst, uint16_t destSize, const uint8_t* pSrc, uint8_t escChar) {
    uint8_t     c;
    uint8_t     msb=0;
    uint16_t    dstIdx=0;

    union {
        struct {
            unsigned char inSq : 1;             //We are inside a "Quoted String"
            unsigned char sqFoundInSq : 1;      //Single quote found inside "Quoted String"
            unsigned char firstHexChar : 1;     //First hex character found
            unsigned char putEsc : 1;           //Put a backslash character
            unsigned char notEnoughSpace : 1;   //Not enought space in destination buffer
        } bits;
        uint8_t val;
    } flags;
    flags.val = 0;

    //Do some checks
    if ((destSize==0)) {
        return 0;
    }

    //Read bytes from source
    while (1) {
        //Get next character.
        c = *pSrc++;

        //End of source string
        if ((c == 0) || (c==';')) {
            break;
        }

        //Process read byte
        //Previous char was a ' inside a '. Can be escape sequence to put a single quote, or end of "Quoted String"
        if (flags.bits.sqFoundInSq == true) {
            //flags2.bits.dqFoundInDq = false;   //Don't clear, is needed below!

            //It was the end of the "Quoted String". Clear flag. c contains next char, will be processed below
            if (c != '\'') {
                flags.bits.inSq = false;
                flags.bits.sqFoundInSq = false;
            }
            //else, it was an escape sequence to put a single quote. Will be processed below in if (flags2.bits.dqFoundInDq == true) {}
        }

        //Second byte of hex character expected
        if (flags.bits.firstHexChar == true) {
            flags.bits.firstHexChar = false;

            //Second hex char must be now
            c = MxHelpers::ascii_hex_nibble_to_byte(c);

            //If both bytes were not hex encoded characters, ERROR!
            if ((c == 0xff) || (msb == 0xff)) {
                //DEBUG_PUT_STR(DEBUG_LEVEL_WARNING, "\ncbufPAECStr Hex Err!");
                return 0; //Nothing added to destination, or removed from source buffers
            }

            //Get hex number represented by last two hex characters received
            c = ((msb << 4) | c);

            //c already contains correct character (hex code), will be added to buffer below
        }
        //Inside "Quoted String"
        else if (flags.bits.inSq == true) {
            //Single quote found inside current "Quoted String". Can be escape sequence to put a single single quote,
            //or end of "Quoted String"
            if (c == '\'') {
                //Previous char was a ' inside a '. This is another ' = an escaped single quote (two '' = single '), leave c as is, will be added to buffer below.
                if (flags.bits.sqFoundInSq == true) {
                    flags.bits.sqFoundInSq = false;
                    //c already contains correct character ('), will be added to buffer below
                } else {
                    flags.bits.sqFoundInSq = true;
                    continue;               //Don't put c, continue!
                }
            }
            //c already contains correct character, will be added to buffer below
        }
        //First character of two byte hex code
        else if ((c <= 'F') && (c >= '0')) {
            //This is the MSB of a hex code (in 2 character ASCII HEX format)
            flags.bits.firstHexChar = true;
            msb = MxHelpers::ascii_hex_nibble_to_byte(c);
            continue;              //Don't put first byte of hex code, continue!
        }
        //Found a single quote
        else if (c == '\'') {
            //DEBUG_PUT_STR(DEBUG_LEVEL_INFO, "\n\' Found");
            flags.bits.inSq = true;
            continue;               //Don't put ', continue!
        }
        //Found a control character (lower case character)
        else if ((c <= 'z') && (c >= 'a')) {
            //DEBUG_PUT_STR(DEBUG_LEVEL_INFO, "\nc Found");

            //Control character not supported!
            if (escChar ==0 ) {
                continue;
            }

            flags.bits.putEsc = true; //Cause an "Escape" character to precede the control character

            //We have to write a "escape character" followed by the current "control character" to the TX buffer.
            //c already contains the "control character", and flags2.bits.putEsc will cause an "escape character" to be put before it.
        } else {
            //Invalid char, continue
            continue;
        }

        //If it the Escape Character, it must be escaped. Do NOT escape if inside "Single quotes"
        if ((escChar!=0) && (c==escChar)) {
            flags.bits.putEsc = true; //Cause an "Escape" character to precede the control character
        }

        //Must escape character be written to destination?
        if (flags.bits.putEsc == true) {
            flags.bits.putEsc = false;
            pDst[dstIdx++] = escChar;
            if (dstIdx >= destSize) {
                flags.bits.notEnoughSpace = true;
                break;
            }
        }

        //Write current char to tx buffer
        pDst[dstIdx++] = c;

        //If no more place for writing, set error (putAvailable=-1) and break out of loop
        if (dstIdx >= destSize) {
            flags.bits.notEnoughSpace = true;
            break;
        }
    }

    //NULL terminate end of destination
    if(flags.bits.notEnoughSpace) {
        return 0;
    }

    return dstIdx;
}


/**
 * Restore default values for given RadioConfig structure
 */
void restoreRadioConfigDefaults(RadioConfig* pRadioConfig) {
    pRadioConfig->frequency = RF_FREQUENCY;
    pRadioConfig->bw = LORA_BANDWIDTH;
    pRadioConfig->sf = LORA_SPREADING_FACTOR;
    if(pRadioConfig->boardType == BOARD_INAIR9B) {
        pRadioConfig->power = TX_OUTPUT_POWER_BOOST;
    }
    else {
        pRadioConfig->power = TX_OUTPUT_POWER;
    }
    pRadioConfig->preambleLength = LORA_PREAMBLE_LENGTH;
    pRadioConfig->symbolTimeout = LORA_SYMBOL_TIMEOUT;
    pRadioConfig->numberSymHop = LORA_NB_SYMB_HOP;
    pRadioConfig->rxTimeout = RX_TIMEOUT_VALUE;
    pRadioConfig->rxMode = RX_MODE_CONTINUOUS;          //Continues receive mode, Radio stays in receive mode after receiving a message
    pRadioConfig->txMode = TX_MODE_IDLE_AFTER_TXION;    //Transmit Mode - Radio switches to Idle after TXion

    pRadioConfig->conf.lora.codingRate = LORA_CODINGRATE;
    pRadioConfig->conf.lora.fixLength = LORA_FIX_LENGTH_PAYLOAD_ON;
    pRadioConfig->conf.lora.fshhEnable = LORA_FHSS_ENABLED;
    pRadioConfig->conf.lora.iqInversionEnable = LORA_IQ_INVERSION_ON;
    pRadioConfig->conf.lora.crcEnable = LORA_CRC_ENABLED;
}


/**
 * Load AppConfig from EEPROM
 * @return True if OK, else false
 */
bool loadAppConfig(uint8_t id, AppConfig* appConf) {
    return mxconf_read_struct(id,
            (uint8_t*)appConf,
            sizeof(AppConfig),
            MXCONF_ADR_AppConfig,
            MXCONF_SIZE_AppConfig,
            (uint8_t*)mxconfDescAppConfig);
}


/**
 * Save AppConfig
 * @return True if OK, else false
 */
bool saveAppConfig(AppConfig* appConf) {
    return mxconf_save_struct(MXCONF_ID_AppConfig,
            (uint8_t*)appConf,
            sizeof(AppConfig),
            MXCONF_ADR_AppConfig,
            MXCONF_SIZE_AppConfig,
            (uint8_t*)mxconfDescAppConfig);
}


/**
 * Load RadioConfig from EEPROM
 * @return True if OK, else false
 */
bool loadRadioConfig(uint8_t id, RadioConfig* radioConf) {
    return mxconf_read_struct(id,
                    (uint8_t*)radioConf,
                    sizeof(RadioConfig),
                    MXCONF_ADR_RadioConfig0 + ((id-MXCONF_ID_RadioConfig0)*MXCONF_SIZE_RadioConfig),
                    MXCONF_SIZE_RadioConfig,
                    (uint8_t*)mxconfDescRadioConfig);
}

/**
 * Save RadioConfig
 * @return True if OK, else false
 */
bool saveRadioConfig(uint8_t id, RadioConfig* radioConf) {
    return mxconf_save_struct(id,
            (uint8_t*)radioConf,
            sizeof(RadioConfig),
            MXCONF_ADR_RadioConfig0 + ((id-MXCONF_ID_RadioConfig0)*MXCONF_SIZE_RadioConfig),
            MXCONF_SIZE_RadioConfig,
            (uint8_t*)mxconfDescRadioConfig);
}
