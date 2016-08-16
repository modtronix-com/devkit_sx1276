/**
 * File:      im4oled.h
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
#include "im4oled_default_config.h"

class Im4OLED {
public:
    /** Constructor
     */
#if (IM4OLED_VIA_PT01NZ==0)
    Im4OLED(PinName pinOK, PinName pinStar, PinName pinUp, PinName pinDown);
#else
    Im4OLED(PinName pinOKStar, PinName pinUpDown);
#endif

    /**
     * Returns true if any button was pressed, and is available via a getXxxBtnFalling() function
     */
    bool wasAnyBtnPressed(void);

    /**
     * Reset any button that was pressed, and is available via a getXxxBtnFalling() function
     * @return True if something was reset, else false
     */
    bool resetAllFalling(void);

    /**
     * Return the state of OK button
     * @return false if button released, else true
     */
    bool getOkBtn(void);

    /**
     * Return the state of Star button
     * @return false if button released, else true
     */
    bool getStarBtn(void);

    /**
     * Return the state of Up button
     * @return false if button released, else true
     */
    bool getUpBtn(void);

    /**
     * Return the state of Down button
     * @return false if button released, else true
     */
    bool getDownBtn(void);

//    uint8_t getOkBtnRissing();
//    uint8_t getStarBtnRissing();
//    uint8_t getUpBtnRissing();
//    uint8_t getDownBtnRissing();

    /**
     * Return number of times the given button was pressed, and resets it to 0.
     * @param btnID The button ID, is a Im4OLED::BTN_ID_xxx define
     */
    uint8_t getBtnFalling(uint16_t btnID);

    /**
     * Return number of times the OK button was pressed, and resets it to 0.
     */
    uint8_t getOkBtnFalling(void);

    /**
     * Return number of times the Star button was pressed, and resets it to 0.
     */
    uint8_t getStarBtnFalling(void);

    /**
     * Return number of times the Up button was pressed, and resets it to 0.
     */
    uint8_t getUpBtnFalling(void);

    /**
     * Return number of times the Down button was pressed, and resets it to 0.
     */
    uint8_t getDownBtnFalling(void);

    /** Get number of repeat "BtnFalling" events for current button held down
     * @return Number indicating how many times current button has been automatically repeated
     */
    uint16_t getRepeatCount(void) {
        return repeatCount;
    }

    /** Set period between automatic button repeats in milliseconds.
     * Given value will be rounded down to nearest 10ms. For example, 128 will be rounded to 120ms.
     *
     * @param period Period in milliseconds between button repeats.
     */
    void setRepeatPeriod(uint16_t period) {
        repeatPeriod = period/10;
    }

    /** Set initial delay in milliseconds until the automatic button repeats start.
     * Given value will be rounded down to nearest 10ms. For example, 128 will be rounded to 120ms.
     *
     * @param period Period in milliseconds between button repeats.
     */
    void setDelayTillRepeat(uint16_t period) {
        delayTillRepeat = period/10;
    }

public:
    enum ButtonId {
        BTN_ID_OK=0,
        BTN_ID_STAR,
        BTN_ID_UP,
        BTN_ID_DOWN,
        BTN_ID_MAX
    };

private :
    //Objects
    Ticker      _ticker;

#if (IM4OLED_VIA_PT01NZ==1)
    DigitalIn   btnOKStar;
    DigitalIn   btnUpDown;
#else
    DigitalIn   btnOK;
    DigitalIn   btnStar;
    DigitalIn   btnUp;
    DigitalIn   btnDown;
#endif

private :
    struct ButtonFlags {
        union {
            struct {
                uint8_t fallingLatch    :1;
                uint8_t risingLatch     :1;
            } bit;
            uint8_t    Val;
        } flags;
    };

#define IM4OLED_BUTTONS 4
    //Structures
    struct ButtonFlags arrBtnFlags[IM4OLED_BUTTONS];
    uint8_t     arrButtons[IM4OLED_BUTTONS];            //Debounced button count. Contains 0 if button released, and BTN_SAMPLES if button pressed
    uint8_t     arrBtnFallingCnt[IM4OLED_BUTTONS];      //Counts number of times button was pressed

    uint8_t     delayTillRepeat;    //Delay until repeating starts - a value in 10ms
    uint8_t     repeatPeriod;       //Delay between button repeats - a value in 10ms
    uint8_t     repeatBtnId;        //Button ID (BTN_ID_xx) of current button pressed, or 0xff if none
    uint16_t    repeatCount;        //Count how many times current pressed button(repeatBtnId) has repeated
    int         tmrRepeat;

    // function to take a sample, and update flags
    void _sample(void);
};
