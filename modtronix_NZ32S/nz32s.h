/**
 * File:      nz32s.h
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
#ifndef NZ32S_H_
#define NZ32S_H_

#include "nz32s_default_config.h"
#include "mx_tick.h"
#include "mx_helpers.h"
#include "mx_circular_buffer.h"
#include "mx_cmd_buffer.h"


// Debugging //////////////////////////////////////////////////////////////////
// To enable debug output from this file, define MX_DEBUG and DEBUG_ENABLE_NZ32S before
// including this file.
//
//Defines for MXH_DEBUG - debugging for include file
#if !defined(DEBUG_ENABLE_NZ32S)
    #define DEBUG_ENABLE_NZ32S          0
#endif
#if !defined(DEBUG_ENABLE_INFO_NZ32S)
    #define DEBUG_ENABLE_INFO_NZ32S     0
#endif

#if !defined(MXH_DEBUG)
    #if defined(MX_DEBUG) && (DEBUG_ENABLE_NZ32S==1)
        #define MXH_DEBUG MX_DEBUG
    #else
        #define MXH_DEBUG(format, args...) ((void)0)
    #endif
#endif

#if !defined(MXH_DEBUG_INFO)
    #if defined(MX_DEBUG) && (DEBUG_ENABLE_NZ32S==1) && (DEBUG_ENABLE_INFO_NZ32S==1)
        #define MXH_DEBUG_INFO MX_DEBUG
    #else
        #define MXH_DEBUG_INFO(format, args...) ((void)0)
    #endif
#endif


#ifdef __cplusplus
extern "C" {
#endif

#ifndef WEAK
    #if defined (__ICCARM__)
        #define WEAK     __weak
    #else
        #define WEAK     __attribute__((weak))
    #endif
#endif


class NZ32S {
public:
    /** Constructor
     */
    NZ32S();

    /** Toggle System LED1
    */
    static inline bool toggle_led1(void) {
        led1 = !led1;
        return (bool)led1.read();
    }

    /** Set System LED1
     */
    static inline void set_led1(void) {
        led1 = 1;
    }

    /** Set System LED1
     */
    static inline void clear_led1(void) {
        led1 = 0;
    }

    /** Set System LED1
     */
    static inline void write_led1(bool val) {
        led1 = val;
    }

    /** Get state of LED2
     */
    static inline bool get_led1(void) {
        return (bool)led1.read();
    }

    /** Get state of button1. Returns 1 when pressed
     */
    static inline bool get_btn1(void) {
        return (bool)btn1.read();
    }

    /** Enable fast charging
     * This function will enable fast charging of the battery. Ensure solder jumper labeled
     * "Fast Charge" (J13 on NZ32-SC151) on the back of the board is made!
     */
    static inline void enable_fast_charging(void) {
        #if (NZ32S_USE_A13_A14 == 1)
        enableFastCharge.output();
        enableFastCharge = 0; //Enable fast charging
        #endif
    }


    /** Disable fast charging
     */
    static inline void disable_fast_charging(void) {
        #if (NZ32S_USE_A13_A14 == 1)
        NZ32S::enableFastCharge.input();
        NZ32S::enableFastCharge.mode(PullNone);
        #endif
    }

    /** Get the battery voltage.
     */
    static uint16_t get_batt_mv();

    /**
     * Get Vusb or 5V supply voltage in millivolts.
     */
    static uint16_t get_supply_mv(void);

    /**
     * Reset I2C bus
     */
    static void i2c_reset(uint8_t busNumber, PinName sda, PinName scl);

    /** Initializes and start the IWDG Watchdog timer with given timerout.
     * After calling this function, watchdog_refresh() function must be called before the
     * timeout expires, else the MCU will be reset.
     *
     * @param timeout The timeout in ms. Must be a value from 1 to 32000 (1ms to 32 seconds)
     */
    static inline void watchdog_start(uint32_t timeout) {
#if (NZ32S_USE_WWDG==1)
        //WWDG clock counter = (PCLK1 (32MHz)/4096)/8) = 976 Hz (~1024 us)
        //WWDG Window value = 80 means that the WWDG counter should be refreshed only
        //when the counter is below 80 (and greater than 64/0x40) otherwise a reset will
        //be generated.
        //WWDG Counter value = 127, WWDG timeout = ~1024 us * 64 = 65.57 ms */
        //
        //Min and Max for given Prescaler and WDGTB
        //For Prescaler=1 and WDGTB=0 is:
        // - Min TimeOut = 128uS, Max Timeout = 8.19ms
        //For Prescaler=2 and WDGTB=1 is:
        // - Min TimeOut = 256uS, Max Timeout = 16.38ms
        //For Prescaler=4 and WDGTB=2 is:
        // - Min TimeOut = 512uS, Max Timeout = 32.76ms
        //For Prescaler=8 and WDGTB=3 is:
        // - Min TimeOut = 1024uS, Max Timeout = 65.54ms
        NZ32S::hwwdg.Instance = WWDG;

        MXH_DEBUG("\r\nwatchdog_start() called!");

        //The refresh window is:between 48.1ms (~1024 * (127-80)) and 65.57 ms (~1024 * 64)
        NZ32S::hwwdg.Init.Prescaler = WWDG_PRESCALER_8;
        NZ32S::hwwdg.Init.Window    = 80;
        NZ32S::hwwdg.Init.Counter   = 127;

        HAL_WWDG_Init(&NZ32S::hwwdg);

        //Start the watchdog timer
        HAL_WWDG_Start(&NZ32S::hwwdg);
#else
        //Watchdog frequency is always 32 kHz
        //Prescaler: Min_Value = 4 and Max_Value = 256
        //Reload: Min_Data = 0 and Max_Data = 0x0FFF(4000)
        //TimeOut in seconds = (Reload * Prescaler) / Freq.
        //MinTimeOut = (4 * 1) / 32000 = 0.000125 seconds (125 microseconds)
        //MaxTimeOut = (256 * 4096) / 32000 = 32.768 seconds
        NZ32S::hiwdg.Instance = IWDG;

        MXH_DEBUG("\r\nwatchdog_start() called!");

        //For values below 4000, use IWDG_PRESCALER_32. This will cause timeout period to be
        //equal to 'timeout' value in ms
        if(timeout < 4000) {
            NZ32S::hiwdg.Init.Prescaler = IWDG_PRESCALER_32;
            NZ32S::hiwdg.Init.Reload = timeout;    //Timeout = (32 * timeout) / 32000 = (timeout) ms
        }
        else {
            NZ32S::hiwdg.Init.Prescaler = IWDG_PRESCALER_256;
            NZ32S::hiwdg.Init.Reload = timeout/8;  //Timeout = (256 * (timeout/8)) / 32000
        }
        HAL_IWDG_Init(&NZ32S::hiwdg);

        //Start the watchdog timer
        HAL_IWDG_Start(&NZ32S::hiwdg);
#endif
    }


    /** Refreshes the IWDG
     */
    static inline void watchdog_refresh(void) {
#if (NZ32S_USE_WWDG==1)
        //Refresh WWDG: update counter value to 127, the refresh window is:
        //between 48.1ms (~1024 * (127-80)) and 65.57 ms (~1024 * 64) */
        HAL_WWDG_Refresh(&NZ32S::hwwdg, 127);
#else
        HAL_IWDG_Refresh(&NZ32S::hiwdg);
#endif
    }


    /** Return true if last reset was caused by watchdog timer
     *
     * @param clearFlags If set, will clear ALL reset flags! Note that not only the
     * watchdog flag, but ALL reset flags located in RCC_CSR register are cleared!
     */
    static inline bool watchdog_caused_reset(bool clearFlags) {
        bool retVal;
        retVal = __HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST)==0?false:true;
        if (clearFlags==true) {
            __HAL_RCC_CLEAR_RESET_FLAGS(); // The flags cleared after use
        }
        return retVal;
    }

    /** Print the cause of last reset
     *
     * @param clearFlags If set, will clear ALL reset flags! Note that not only the
     * watchdog flag, but ALL reset flags located in RCC_CSR register are cleared!
     */
    static inline void print_reset_cause(bool clearFlags);

public:
    #if (NZ32S_USE_WWDG==1)
    static WWDG_HandleTypeDef hwwdg;   //Windowed Watchdog Timer, is stopped during low power mode
    #else
    static IWDG_HandleTypeDef hiwdg;   //Independent Watchdog Timer, is NOT stopped during low power mode!
    #endif

    static DigitalOut led1;
    static DigitalIn  btn1;

    #if (NZ32S_USE_A13_A14 == 1)
    static DigitalInOut enableFastCharge;
    #endif
};

#ifdef __cplusplus
};
#endif

#if defined(MXH_DEBUG)
    #undef MXH_DEBUG
#endif
#if defined(MXH_DEBUG_INFO)
    #undef MXH_DEBUG_INFO
#endif

#endif //NZ32S_H_
