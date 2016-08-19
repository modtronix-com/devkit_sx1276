/**
 * File:      nz32s.cpp
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
#define THIS_IS_NZ32S_CPP

#include "mbed.h"
#include <stdarg.h>
#include <stdio.h>

#define DEBUG_ENABLE            1
#include "mx_default_debug.h"

//Includes that use debugging - Include AFTER debug defines/includes above! It uses them for debugging!
#define DEBUG_ENABLE_NZ32S          0
#define DEBUG_ENABLE_INFO_NZ32S     0
#include "nz32s.h"


// DEFINES ////////////////////////////////////////////////////////////////////


// GLOBAL VARIABLES ///////////////////////////////////////////////////////////


// STATIC OBJECTS /////////////////////////////////////////////////////////////
DigitalOut NZ32S::led1(LED1);
DigitalIn  NZ32S::btn1(USER_BUTTON, PullDown);
#if (NZ32S_USE_WWDG==1)
WWDG_HandleTypeDef NZ32S::hwwdg;    //Windowed Watchdog Timer, is stopped during low power mode
#else
IWDG_HandleTypeDef NZ32S::hiwdg;    //Independent Watchdog Timer, is NOT stopped during low power mode!
#endif
#if (NZ32S_USE_A13_A14 == 1)
DigitalInOut NZ32S::enableFastCharge(PA_14, PIN_INPUT, PullNone, 0);
#endif


// Function Prototypes ////////////////////////////////////////////////////////


/** Constructor
 */
NZ32S::NZ32S() {
    led1 = 0;   //User LED off
}


/** Get the battery voltage.
 */
uint16_t NZ32S::get_batt_mv() {
    MX_DEBUG("\r\nnz32s_getBattMV()");

    //uint16_t getBattMV(void) {
    //    float fval = 0;
    //
    //#if (NZ32S_USE_A13_A14 == 1)
    ////    uint16_t meas;
    ////
    ////    //Measure Vbatt. It doesn't seem to make lots of a difference if we disable the DC/DC converter!
    ////    ctrVBatt = 0;
    ////    ctrVBatt.output();
    ////    wait_ms(150);
    ////    meas = ainVBatt.read_u16(); // Converts and read the analog input value
    ////    ctrVBatt.input();
    ////    //fval = ((float)meas / (float)0xffff) * 6600;  //6600 = 3300*2, because resistor divider = 2
    ////    fval = meas * ((float)6600.0 / (float)65535.0);
    //#endif
    //
    //    return (uint16_t) fval;
    return 0;
}


/**
 * Get Vusb or 5V supply voltage in millivolts.
 */
uint16_t NZ32S::get_supply_mv(void) {
    //    float fval;
    //    uint16_t meas;
    //
    //    //Following voltages were measured at input of ADC:
    //    //
    //    //----- Vusb=0V  &  5V supply=0V -----
    //    //Value is typically 4mV
    //    //
    //    //----- Vusb=5V  &  5V supply=0V -----
    //    //When only VUSB is supplied, the value is typically 1.95V
    //    //
    //    //----- Vusb=0V  &  5V supply=5V -----
    //    //When only VUSB is supplied, the value is typically 1.95V
    //    //
    //    //The reason this circuit does not work, is because of the reverse voltage of the diodes. It is given
    //    //as about 40 to 80uA. With a resistor value of 470K, this will put 5V on other side of diode.
    //    //Thus, no matter if 5V is at Vusb, Vsupply, or both, the read value will always be 1.95V.
    //    //To solve problem, resistors must be lowered to value where 80uA will no longer give more than 5V
    //    //voltage drop = 62k. Using two 47k resistors should work.
    //
    //    //Measure Vusb/5V sense input
    //    meas = ainVSense.read_u16(); // Converts and read the analog input value
    //    fval = ((float) meas / (float) 0xffff) * 3300;
    //
    //    return (uint16_t) fval;
    return 0;
}


/**
 * Reset I2C bus
 */
void NZ32S::i2c_reset(uint8_t busNumber, PinName sda, PinName scl) {
#if defined(STM32) || defined(STM32L1)
    i2c_t objI2C;

    if (busNumber == 1) {
        objI2C.i2c = I2C_1;
    }
    else if (busNumber == 2) {
        objI2C.i2c = I2C_2;
    }

    objI2C.slave = 0;
    i2c_init(&objI2C, sda, scl);
    //i2c_reset(&objI2C);       //i2c_init() above calls i2c_reset()
    //pc.printf("\r\nI2C1 Reset");

    MX_DEBUG("\r\nI2C1 Reset");

#endif  //#if defined(STM32) || defined(STM32L1)
}

void NZ32S::print_reset_cause(bool clearFlags) {
#if defined(STM32) || defined(STM32L1)
    if ( __HAL_RCC_GET_FLAG(PWR_FLAG_SB))
        MX_DEBUG("\r\nSystem resumed from STANDBY mode");

    if ( __HAL_RCC_GET_FLAG(RCC_FLAG_SFTRST))
        MX_DEBUG("\r\nSoftware Reset");

    if ( __HAL_RCC_GET_FLAG(RCC_FLAG_PORRST))
        MX_DEBUG("\r\nPower-On-Reset");

    if ( __HAL_RCC_GET_FLAG(RCC_FLAG_PINRST)) // Always set, test other cases first
        MX_DEBUG("\r\nExternal Pin Reset");

    if ( __HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST) != RESET)
        MX_DEBUG("\r\nWatchdog Reset");

    if ( __HAL_RCC_GET_FLAG(RCC_FLAG_WWDGRST) != RESET)
        MX_DEBUG("\r\nWindow Watchdog Reset");

    if ( __HAL_RCC_GET_FLAG(RCC_FLAG_LPWRRST) != RESET)
        MX_DEBUG("\r\nLow Power Reset");

//    if ( __HAL_RCC_GET_FLAG(RCC_FLAG_BORRST) != RESET) // F4 Usually set with POR
//        MX_DEBUG("\r\nBrown-Out Reset");

    if (clearFlags) {
        __HAL_RCC_CLEAR_RESET_FLAGS(); // The flags cleared after use
    }
#endif  //#if defined(STM32) || defined(STM32L1)
}

