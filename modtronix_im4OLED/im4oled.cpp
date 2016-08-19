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
#include "im4oled.h"
#include "mx_tick.h"

#define BTN_SAMPLES 4

static MxTick  mxTick;

#if (IM4OLED_VIA_PT01NZ==0)
/*
 * Constructor
 */
Im4OLED::Im4OLED(PinName pinOK, PinName pinStar, PinName pinUp, PinName pinDown)
    : btnOK(pinOK, PullUp), btnStar(pinStar, PullUp), btnUp(pinUp, PullUp), btnDown(pinDown, PullUp)
{
    delayTillRepeat = 80;   //Repeats start after 800ms
    repeatPeriod = 20;      //200mS between button repeats
    repeatBtnId = 0xff;     //No button currently pressed donw
    repeatCount = 0;
    tmrRepeat = 0;

    // reset all the flags and counters
    memset(&arrButtons, 0, sizeof(arrButtons));
    memset(&arrBtnFallingCnt, 0, sizeof(arrBtnFallingCnt));
    memset(&arrBtnFlags, 0, sizeof(arrBtnFlags));

    // Read pins every 10ms
    _ticker.attach(this, &Im4OLED::_sample, 0.01);
}
#else
/*
 * Constructor
 */
Im4OLED::Im4OLED(PinName pinOKStar, PinName pinUpDown)
    : btnOKStar(pinOKStar, PullUp), btnUpDown(pinUpDown, PullUp)
{
    delayTillRepeat = 80;   //Repeats start after 800ms
    repeatPeriod = 20;      //200mS between button repeats
    repeatBtnId = 0xff;     //No button currently pressed donw
    repeatCount = 0;
    tmrRepeat = 0;

    // reset all the flags and counters
    memset(&arrButtons, 0, sizeof(arrButtons));
    memset(&arrBtnFallingCnt, 0, sizeof(arrBtnFallingCnt));
    memset(&arrBtnFlags, 0, sizeof(arrBtnFlags));

    // Read pins every 10ms
    _ticker.attach(this, &Im4OLED::_sample, 0.01);
}
#endif


void Im4OLED::_sample() {
    bool anyBtnPressed = false;    //Check if any button pressed
    uint16_t i;
    uint8_t val[IM4OLED_BUTTONS];

#if (IM4OLED_VIA_PT01NZ==0)
    val[0] = btnOK.read();
    val[1] = btnStar.read();
    val[2] = btnUp.read();
    val[3] = btnDown.read();
#else
    //All buttons not pressed
    val[0] = val[1] = val[2] = val[3] = 1;

    //Test if OK or Down is pressed. To test this, enable pullups(default) on both
    //pins, and test if pins are pulled low.
    if(btnOKStar.read() == 0) {
        val[0] = 0; //Star pressed
    }
    btnOKStar.mode(PullDown);
    if(btnUpDown.read() == 0) {
        val[3] = 0; //Down pressed
    }
    btnUpDown.mode(PullDown);

    //Test if Star or Up is pressed. To test this, enable pulldown on both
    //pins, and test if pins are pulled high.
    wait_us(5);     //Delay time for pulldown to settle
    if(btnOKStar.read() == 1) {
        val[1] = 0; //OK pressed
    }
    if(btnUpDown.read() == 1) {
        val[2] = 0; //Up pressed
    }

    //Enable default state - pullups on both pins
    btnOKStar.mode(PullUp);
    btnUpDown.mode(PullUp);
#endif

    for(i=0; i<IM4OLED_BUTTONS; i++) {
        // Current button PRESSED /////////////////////////////////////////////
        if(val[i] == 0) {
            if(arrButtons[i] < BTN_SAMPLES) {
                arrButtons[i]++;
            }

            //Debounced Button is pressed
            if(arrButtons[i] == BTN_SAMPLES) {
                anyBtnPressed = true;  //Indicate that a button is pressed

                //This button was already pressed
                if (repeatBtnId == i) {
                    //The initial "Delay till repeat" time has not passed yet
                    if (repeatCount == 0) {
                        //Has the "Delay till repeat" time passed yet
                        if ((mxTick.read_ms10() - tmrRepeat) > delayTillRepeat) {
                            repeatCount++;
                            tmrRepeat = mxTick.read_ms10();
                            arrBtnFlags[i].flags.bit.fallingLatch = 0;  //Simulate a button press
                        }
                    }
                    //Button is repeating
                    else {
                        //Has the "period" time passed yet
                        if ((mxTick.read_ms10() - tmrRepeat) > repeatPeriod) {
                            repeatCount++;
                            tmrRepeat = mxTick.read_ms10();
                            arrBtnFlags[i].flags.bit.fallingLatch = 0;  //Simulate a button press
                        }
                    }
                }
                //This button was NOT pressed till now!
                else {
                    repeatCount = 0;
                    tmrRepeat = mxTick.read_ms10();
                }

                //Button just change from released->pressed
                if(arrBtnFlags[i].flags.bit.fallingLatch == 0) {
                    arrBtnFlags[i].flags.bit.fallingLatch = 1;
                    arrBtnFallingCnt[i]++;
                    repeatBtnId = i;    //Remember pressed button ID
                }
            }
        }
        // Current button RELEASED ////////////////////////////////////////////
        else {
            if (arrButtons[i] > 0) {
                arrButtons[i]--;
            }

            //Debounced Button is released
            if(arrButtons[i] == 0) {
                //Reset fallingLatch
                arrBtnFlags[i].flags.bit.fallingLatch = 0;
            }
        }
    }

    //All buttons released
    if(anyBtnPressed == false) {
        repeatBtnId = 0xff; //Indicate no button pressed
    }
}

bool Im4OLED::wasAnyBtnPressed(void) {
    for(int i=0; i<IM4OLED_BUTTONS; i++) {
        if(arrBtnFallingCnt[i]!=0) {
            return true;
        }
    }
    return false;
}

bool Im4OLED::resetAllFalling(void) {
    bool retVal = false;
    for(int i=0; i<IM4OLED_BUTTONS; i++) {
        if(getBtnFalling(i)!=0) {
            retVal=true;
        }
    }
    return retVal;
}


bool Im4OLED::getOkBtn(void) {
    //return btnOK.read();
    return arrButtons[BTN_ID_OK] == BTN_SAMPLES;
}


bool Im4OLED::getStarBtn(void) {
    return arrButtons[BTN_ID_STAR] == BTN_SAMPLES;
}


bool Im4OLED::getUpBtn(void) {
    return arrButtons[BTN_ID_UP] == BTN_SAMPLES;
}


bool Im4OLED::getDownBtn(void) {
    return arrButtons[BTN_ID_DOWN] == BTN_SAMPLES;
}

uint8_t Im4OLED::getBtnFalling(uint16_t btnID) {
	uint8_t retVal = 0;
    if(arrBtnFallingCnt[btnID]!=0) {
    	retVal = arrBtnFallingCnt[btnID];
        arrBtnFallingCnt[btnID] = 0;
    }
    return retVal;
}

/**
 * Return number of times the OK button was pressed, and resets it to 0.
 */
uint8_t Im4OLED::getOkBtnFalling(void) {
    return getBtnFalling(0);
}

/**
 * Return number of times the Star button was pressed, and resets it to 0.
 */
uint8_t Im4OLED::getStarBtnFalling(void) {
    return getBtnFalling(1);
}

/**
 * Return number of times the Up button was pressed, and resets it to 0.
 */
uint8_t Im4OLED::getUpBtnFalling(void) {
    return getBtnFalling(2);
}

/**
 * Return number of times the Down button was pressed, and resets it to 0.
 */
uint8_t Im4OLED::getDownBtnFalling(void) {
    return getBtnFalling(3);
}


