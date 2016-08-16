/**
 * File:      app_display.cpp
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

#include "app_defs.h"            //Application defines, must be first include after debugging includes/defines(in main.cpp)
#include "app_helpers.h"
#include "mx_config.h"
#include "nz32s.h"
#include "mx_ssd1306.h"
#include "im4oled.h"

#if !defined(DISABLE_OLED)


// External GLOBAL VARIABLES //////////////////////////////////////////////////
extern AppConfig       appConfig;
extern AppData         appData;
extern bool            i2cBus1OK;
extern I2C             i2cBus1;
extern RadioConfig     radioConfig[RADIO_COUNT];
extern RadioData       radioData[RADIO_COUNT];


// VARIABLES //////////////////////////////////////////////////////////////////
bool            hasOLED;
int             tmrDisplayOff;
int             tmrUpdateBatt;
uint16_t        percentBatt;    //Battery percentage
#if !defined(DISABLE_OLED)
MxSSD1306_I2C   oled(0x78, i2cBus1, 64);
    #if (IM4OLED_VIA_PT01NZ==1)
    Im4OLED         im4Oled(PC_2, PC_1); //OK & Star, Up & Down
    #else
    Im4OLED         im4Oled(PC_1, PC_12, PC_2, PC_10); //OK, Star, Up, Down
    #endif
#endif
static const uint8_t arrBwChar[][5] = {"7.8", "10.4", "15.6", "20.8", "31.2", "41.7", "62.5", "125", "250", "500"};




// Defines ////////////////////////////////////////////////////////////////////
//Top menu states
enum MENU1_STATES {
    MENU1_HOME = 0,
    MENU1_SELECT,
    MENU1_SETTINGS,
    MENU1_CONFIG_RADIO,
    MENU1_RESET_COUNTERS,
    MENU1_MAX
};

//Level 2 menu states for MENU1_CONFIG_RADIO
enum MENU2_CONFIG_RADIO_STATES {
    MENU2_CFGRADIO_MAIN = 0,
    MENU2_CFGRADIO_BOARD,
    MENU2_CFGRADIO_FREQ,
    MENU2_CFGRADIO_BW,
    MENU2_CFGRADIO_SF,
    MENU2_CFGRADIO_RESTORE_DEFAULTS,
    MENU2_CFGRADIO_MAX
};

//Level 2 menu states for MENU1_CONFIG_RADIO
enum MENU2_SETTINGS_STATES {
    MENU2_SETTINGS_MAIN = 0,
    MENU2_SETTINGS_DISPLAY_OFF_TIME,
    MENU2_SETTINGS_DISPLAY_BRIGHTNESS,
    MENU2_SETTINGS_RESTORE_DEFAULTS,
    MENU2_SETTINGS_MAX
};


// Function Prototypes ////////////////////////////////////////////////////////
void checkFrequency(uint8_t radioId);
void selectMenuBlinkEq(uint8_t posEq, uint8_t row, int& tmrDisplay, bool& blinkOn);
void selectMenuUpdate(uint8_t& selectMenuCurrRow, uint8_t selectMenuRows, uint8_t yFirstGt, int& tmrDisplay, bool& blinkOn);
extern void blinkLED(bool reset);
extern void setRadioMode(uint8_t newMode, uint8_t iRadio);
void updateBattLevel(void);



void mx_display_init(void) {

    tmrUpdateBatt = 0;

    //Initialize Auto Off timer
    tmrDisplayOff = MxTick::read_ms()
            + ((appConfig.displayAutoOff<10?10:appConfig.displayAutoOff)*1000); //Set auto off at startup to 30 seconds

    //Configure OLED Keypad
    im4Oled.setDelayTillRepeat(800);    //Delay 800ms until repeat starts
    im4Oled.setRepeatPeriod(100);       //100ms between repeats

    //Only create Adafruit_SSD1306_I2c object here - after startup delay!
    #if !defined(DISABLE_OLED)
    {
        hasOLED = true;
        if (oled.init() != 0) {
            i2cBus1OK = false;
            MX_DEBUG("\r\nERR: OLED Init!");
            hasOLED = false;
        }
    }
    #endif
}


/**
 * Update display with values defined in AppConfig
 */
void mx_display_update(void) {
    oled.setContrast(appConfig.displayBrigtness==1?10:255);
}

/**
 * Turn display on or off. When off, it consumes about 10uA accoring to Sparkfun SSD1306 datasheet
 */
void mx_display_on_off(bool on) {
    oled.displayOn(on);
}

void mx_display_task(void) {
    //Check if bus error occured
    if(hasOLED && i2cBus1OK) {
        if (oled.display() != 0) {
            i2cBus1OK = false;
            MX_DEBUG("\r\nERR: OLED Bus!");
        }
    }

    if (MxTick::read_ms() > tmrUpdateBatt) {
        tmrUpdateBatt += 1000;  //Update in 1 second again
        updateBattLevel();
    }
}


/**
 * Call every 10mS
 */
void mx_menu_task(void) {
    float fVal;

#define MENU_MASK    0x0fff     //Use bottom 14 bits for menu state
#define MENU_ENTRY   0x8000     //Bit 16 indicates if menu entry has been processed
#define MENU_EXIT    0x4000     //Bit 15 indicates if menu exit has been processed
#define MENU_REDRAW  0x2000     //Bit 14 = Redraw

    static uint16_t     smMenu1 = 0;    //Top level menu state machine
    static uint16_t     smMenu2 = 0;    //Second level menu state machine

#if !defined(DISABLE_OLED)
    static uint8_t  selectMenuCurrRow;  //Current selected row index. Each screen has 4 rows. For first screen is 0-3, next 4-7....
    static uint8_t  selectMenuOldRow;   //Cursor position for menu 2
    static uint8_t  selectMenuRows;
    static uint16_t oldRxCntPingPong = 0xffff;
    static uint16_t oldRxErrCnt = 0xffff;
    static uint16_t oldPercentBatt = 0;
    static int      tmrDisplay;
    static bool     blinkOn;
#endif

    //Return if no OLED
    if ((hasOLED==false) || (i2cBus1OK==false)) {
        return;
    }

#if !defined(DISABLE_OLED)

    //If any button pressed, reset timer
    if(im4Oled.wasAnyBtnPressed()) {
        tmrDisplayOff = MxTick::read_ms() + (appConfig.displayAutoOff * 1000);
    }

    //Display currently OFF. Check if any button pressed. If so, CONSUME it, and turn display on
    if(appData.flags.bits.displayOff == true) {
        if(im4Oled.wasAnyBtnPressed()) {
            appData.flags.bits.displayOff = false;
            im4Oled.resetAllFalling();  //Consume any pending buttons
            oled.displayOn(true);       //Turn display on
            MX_DEBUG("\r\nDisplay On");
            return;
        }
    }
    //Display currently ON
    else {
        //Display Auto Off enabled
        if(appConfig.displayAutoOff != 0) {
            //Display off timer expired, turn display off
            if (MxTick::read_ms() >= tmrDisplayOff) {
                appData.flags.bits.displayOff = true;
                MX_DEBUG("\r\nDisplay Off");
                oled.displayOn(false);  //Turn display off
            }
        }
    }

    //If any dirtyConfDisp flags set, display must be redrawn!
    if(appData.flags.bits.dirtyConfDisp || radioData[0].flags.bits.dirtyConfDisp) {
        appData.flags.bits.dirtyConfDisp = false;
        radioData[0].flags.bits.dirtyConfDisp = false;
        smMenu1 &= ~MENU_REDRAW;    //Clear "redraw" flag - cause display to get redrawn!
        smMenu2 &= ~MENU_REDRAW;    //Clear "redraw" flag - cause display to get redrawn!
    }

    //Top level menu
    switch (smMenu1 & MENU_MASK) {
    case MENU1_HOME:
        // HOME Entry /////////////////////////////////////////////////////////
        if ((smMenu1 & MENU_ENTRY) == 0) {
            smMenu1 |= MENU_ENTRY; //Set "entry" flag
        }
        // HOME Redraw ////////////////////////////////////////////////////////
        if ((smMenu1 & MENU_REDRAW) == 0) {
            smMenu1 |= MENU_REDRAW; //Set "entry" flag

            oldPercentBatt = 0xff;  //Force battery level to be updated in next step
            oled.clearDisplay();    //Clear display. Will cause WHOLE display to be redrawn! Use fillRect() to clear only required area if possible!

            //Line 1 - Battery //////////////////
            oled.setTextSize(1);
            oled.fillRect(94, 1, 4, 2, 1); //x, y, w, h
            oled.fillRect(92, 4, 8, 3, 1);
            oled.fillRect(92, 8, 8, 3, 1);
            oled.fillRect(92, 12, 8, 3, 1);
            oled.setTextCursor(104, 5);
            //oled.printf("100%%");

            // Line 1 ///////////////////////////
            oled.setTextSize(2);
            oled.setTextCursor(0, 0);
            if (radioConfig[0].boardType == BOARD_INAIR4)
                oled.printf("inAir4");
            else if (radioConfig[0].boardType == BOARD_INAIR9)
                oled.printf("inAir9");
            else
                oled.printf("inAir9B");
            oled.setTextSize(1);    //Return to default text size


            // Line 2 & 5 ///////////////////////
            oled.setTextCursor(0, 56);  //Line 5
            if (radioData[0].mode == RADIO_MODE_STOPPED) {
                oled.printf("Mode: Stopped       ");
                //Write large text to second line
                oled.setTextSize(2);
                oled.setTextCursor(0, 16);  //Second line(16), first column. First line is 0-13 (14x10 chars)
                if(radioData[0].flags.bits.noRadio) {
                    oled.printf("No Radio! ");
                }
                else {
                    oled.printf("Stopped");
                }
            }
            else if (radioData[0].mode == RADIO_MODE_MASTER) {
                oled.printf("Mode: Master -> Slv%d", appConfig.remotelAdr);
                oled.setTextSize(2);        //Write large text to second line
                oled.setTextCursor(0, 16);  //Second line(16), first column. First line is 0-13 (14x10 chars)
                oled.printf("Master");
            }
            else if (radioData[0].mode >= RADIO_MODE_SLAVE) {
                oled.printf("Mode: Slave %d       ", appConfig.localAdr);
                oled.setTextSize(2);        //Write large text to second line
                oled.setTextCursor(0, 16);  //Second line(16), first column. First line is 0-13 (14x10 chars)
                oled.printf("Slave %d", appConfig.localAdr);
            }
            else if (radioData[0].mode >= RADIO_MODE_ERR_FIRST) {
                oled.printf("Error: Code = %d     ", radioData[0].mode - RADIO_MODE_ERR_FIRST);
                //Write large text to second line
                oled.setTextSize(2);
                oled.setTextCursor(0, 16);  //Second line(16), first column. First line is 0-13 (14x10 chars)
                oled.printf("Error");
            }
            oled.setTextSize(1);    //Return to default text size


            // Line 3 ///////////////////////////
            fVal = ((float)(radioConfig[0].frequency+100))/1000000.0f; //+100 to correct rounding error
            oled.setTextCursor(0, 36);
            oled.printf("F=%d.%d BW=%s SF=%d", (uint)fVal, MxHelpers::get_frac1(fVal),
                    arrBwChar[radioConfig[0].bw], radioConfig[0].sf);

            // Line 4 ///////////////////////////
            //Line 4 - For 3 lines, use 36,46,56 (Could also have 4 lines at 33,41,49 and 57)
            oled.setTextCursor(0, 46);
            oled.printf("RXed OK=0000 Err=0000");
        }

        // HOME Menu /////////////////////////////////////////////////////////
        {
            bool            btnPressed = false;
            static bool     btn1WasReleased=false;
            static uint8_t  btn1Cnt = 0;

            //Debounce and latch system button. Variable btnPressed is set each time button is pressed, app has to reset it!
            if(NZ32S::get_btn1()) {
                if(btn1Cnt != 0) {
                    btn1Cnt--;
                    if(btn1Cnt == 0) {
                        if(btn1WasReleased) {
                            btn1WasReleased=false;
                            btnPressed = true;
                        }
                    }
                }
            }
            else {
                if(btn1Cnt==5) {
                    btn1WasReleased=true;
                }
                else {
                    btn1Cnt++;
                }
            }

            //OK button toggles Stopped/Master/Slave mode
            if ((im4Oled.getOkBtnFalling()!=0) || btnPressed) {
                btnPressed = false;
                if(radioData[0].flags.bits.noRadio) {
                    setRadioMode(RADIO_MODE_STOPPED, 0);
                }
                else {
                    if (radioData[0].mode++ == RADIO_MODE_SLAVE) {
                        radioData[0].mode = RADIO_MODE_STOPPED;
                    }
                    setRadioMode(radioData[0].mode, 0);
                    smMenu1 &= ~MENU_REDRAW;    //Clear "redraw" flag - cause display to get redrawn!
                    blinkLED(true);             //Reset blink system LED. This is done so blink pattern starts immediately
                }
            }
            // Enter Configuration menu
            else if(im4Oled.getStarBtnFalling() != 0) {
                smMenu1 = MENU1_SELECT;
                break;
            }
            // Up button
            //else if(im4Oled.getUpBtnFalling() != 0) {
            //    break;
            //}
            // Down button
            //else if(im4Oled.getDownBtnFalling() != 0) {
            //    break;
            //}

            //Update battery level if it changed
            if(oldPercentBatt != percentBatt) {
                oldPercentBatt = percentBatt;
                oled.setTextCursor(104, 5);
                oled.printf("%d%%", percentBatt);
                if (percentBatt<100) oled.putc(' ');  //Add 1 space after '%' if percentage is only 2 digits long
                if (percentBatt<10)  oled.putc(' ');  //Add 2 spaces after '%' if percentage is only 1 digits long
            }

            //Update Line 2 and 4
            if((oldRxCntPingPong!=appData.rxCountPingPong) || (oldRxErrCnt!=appData.rxErrCountPingPong)) {
                //10+((posEq+1)*(DISPLAY_CHAR_WIDTH+1)),              // X - Editable part is after posEq characters (+1 for space between characters)

                // Line 4 ///////////////////////////
                //Line 4 - For 3 lines, use 36,46,56 (Could also have 4 lines at 33,41,49 and 57)
                oled.setTextCursor(0, 46);
                oled.printf("RXed OK=%04d Err=%04d", appData.rxCountPingPong, appData.rxErrCountPingPong);

                // Line 2 ///////////////////////////
                // Update line 2 for Master and Slave mode if new data received!
                //Clear second line from x=50. No lines should be shorter(end before 50) than that
                oled.fillRect(50, 16, (127-50), 16, 0); //x, y, w, h  - NOT REQUIRED, clearDisplay() called above
                oled.setTextSize(2);        //Write large text to second line
                oled.setTextCursor(0, 16);  //Second line(16), first column. First line is 0-13 (14x10 chars)
                if (radioData[0].mode == RADIO_MODE_MASTER) {
                    if (oldRxErrCnt != appData.rxErrCountPingPong) {
                        if(radioData[0].rxStatus == RX_STATUS_TIMEOUT) {
                            oled.printf("Rx Timeout");
                        }
                        else if(radioData[0].rxStatus == RX_STATUS_ERR_CRC) {
                            oled.printf("CRC Error");
                        }
                        else {
                            oled.printf("Rx Timeout");
                        }
                    }
                    else if (oldRxCntPingPong != appData.rxCountPingPong) {
                        if ((appData.rxCountPingPong & 0x01) == 0) {
                            oled.printf("L=%03d ", abs(radioData[0].RssiValue));
                        } else {
                            oled.printf("L-%03d ", abs(radioData[0].RssiValue));
                        }
                        oled.setTextCursor(68, 16); //Second line(16), column=68pixels. First line is 0-13 (14x10 chars)
                        if ((appData.rxCountPingPong & 0x01) == 0) {
                            oled.printf("R=%03d", abs(radioData[0].RssiValueSlave));
                        } else {
                            oled.printf("R-%03d", abs(radioData[0].RssiValueSlave));
                        }
                    }
                }
                else if (radioData[0].mode >= RADIO_MODE_SLAVE) {
                    if (oldRxCntPingPong != appData.rxCountPingPong) {
                        char c = '\\';
                        oled.printf("Rx - %03d ", abs(radioData[0].RssiValue));
                        switch(appData.rxCountPingPong & 0x03) {
                            case 0: c = '-'; break;
                            case 1: c = '/'; break;
                            case 2: c = '|'; break;
                        }
                        oled.putc(c);
                    }
                }
                oled.setTextSize(1);    //Return to default text size
                oldRxCntPingPong = appData.rxCountPingPong;
                oldRxErrCnt = appData.rxErrCountPingPong;
            }
        }
        break;
    //Select "Setting" or "Radio Config"
    case MENU1_SELECT:
    {
        selectMenuRows = 3; // "Select Menu" has 2 rows

        // Select Menu - State Entry //////////////////////////////////////////
        if ((smMenu1 & MENU_ENTRY) == 0) {
            smMenu1 |= MENU_ENTRY; //Set "entry" flag
            oled.clearDisplay();    //Clear display. Will cause WHOLE display to be redrawn! Use fillRect() to clear only required area if possible!
            //Line 1
            oled.setTextCursor(0, 2);
            oled.printf("Select or * To Return");

            //Line 2
            fVal = ((float)(radioConfig[0].frequency+100))/1000000.0f; //+1 to correct rounding error
            oled.setTextCursor(10, 16); //x=10, leave space for '>' character
            oled.printf("Settings");

            //Line 3
            oled.setTextCursor(10, 27); //x=10, leave space for '>' character
            oled.printf("Configure Radio");

            //Line 4
            oled.setTextCursor(10, 38);
            oled.printf("Reset Counters");

            selectMenuCurrRow = 0;
            blinkOn = true;
            tmrDisplay = MxTick::read_ms();
        }
        // Select Menu ////////////////////////////////////////////////////////
        else {
            //Select current menu
            if(im4Oled.getOkBtnFalling() != 0) {
                if(selectMenuCurrRow==0) {
                    smMenu1 = MENU1_SETTINGS;
                    smMenu2 = MENU2_SETTINGS_MAIN;
                }
                else if(selectMenuCurrRow==1) {
                    smMenu1 = MENU1_CONFIG_RADIO;
                    smMenu2 = MENU2_CFGRADIO_MAIN;
                }
                else if(selectMenuCurrRow==2) {
                    smMenu1 = MENU1_RESET_COUNTERS;
                    smMenu2 = 0xff; //Is no second level menu!
                }
            }
            //Return Home
            else if(im4Oled.getStarBtnFalling() != 0) {
                smMenu1 = MENU1_HOME;
            }
            selectMenuUpdate(selectMenuCurrRow, selectMenuRows, 16, tmrDisplay, blinkOn);
        }
        break;
    } //case MENU1_SELECT:
    case MENU1_SETTINGS:
    case MENU1_CONFIG_RADIO:
    {
        break;
    }
    case MENU1_RESET_COUNTERS:
    {
        if (MxTick::read_ms() >= tmrDisplay) {
            blinkOn = !blinkOn;

            //Line 4
            oled.setTextCursor(0, 38); //x=0, Row 3 = 38
            if(blinkOn) {
                oled.printf(" OK or * to Cancel!");
                tmrDisplay = MxTick::read_ms() + 600;
            }
            else {
                oled.printf("                   ");
                tmrDisplay = MxTick::read_ms() + 200;
            }
        }
        //OK button = Restore Defaults
        if(im4Oled.getOkBtnFalling() != 0) {
            smMenu1 = MENU1_SELECT;
            //Reset counters
            appData.rxCountPingPong = 0;
            appData.rxErrCountPingPong = 0;
            appData.oldRxCountPingPong = 0xff;
        }
        //Start button = return
        else if(im4Oled.getStarBtnFalling() != 0) {
            smMenu1 = MENU1_SELECT;
        }
        break;
    }
    }   //switch (smMenu1 & MENU_MASK) {




    ///////////////////////////////////////////////////////////////////////////
    // Level 2 menu for MENU1_SETTINGS ////////////////////////////////////////
    if ((smMenu1 & MENU_MASK) == MENU1_SETTINGS) {

        //Does "Select Menu" have to be redrawn. Each screen can hold 4 menu items.
        if( ((smMenu2 & MENU_REDRAW) == 0) || ((selectMenuOldRow/4) != (selectMenuCurrRow/4)) ) {
            smMenu2 |= MENU_REDRAW; //Set "redraw" flag

            //First screen
            //if((selectMenuCurrRow/4) == 0) {
            {
                MX_DEBUG("\r\nDraw Screen1");

                oled.clearDisplay();    //Clear display. Will cause WHOLE display to be redrawn! Use fillRect() to clear only required area if possible!
                //Line 1
                oled.setTextCursor(0, 2);
                oled.printf("Select or * To Return");

                //Line 2
                oled.setTextCursor(10, 16);  //x=10, leave space for '>' character
                oled.printf("Disp. Timeout = %03d", appConfig.displayAutoOff);

                //Line 3
                oled.setTextCursor(10, 27); //x=10, leave space for '>' character
                oled.printf("Brightness = %d ", appConfig.displayBrigtness);

                //Line 4
                oled.setTextCursor(10, 38); //x=10, leave space for '>' character
                oled.printf("Restore Defaults");
            }
            //Second Screen
            /*
            else if((selectMenuCurrRow/4) == 1) {
                MX_DEBUG("\r\nDraw Screen2");

                oled.clearDisplay();    //Clear display. Will cause WHOLE display to be redrawn! Use fillRect() to clear only required area if possible!
                //Line 1
                oled.setTextCursor(0, 2);
                oled.printf("Select or * To Return");

                //Line 2
                oled.setTextCursor(10, 16); //x=10, leave space for '>' character
                oled.printf("Some text....");
            }
            */
        }

        switch (smMenu2 & MENU_MASK) {
        // Configure Main Screen //////////////////////////////////////////////
        case MENU2_SETTINGS_MAIN:
            // Configure Main Screen - State Entry ////////////////////////////
            if ((smMenu2 & MENU_ENTRY) == 0) {
                smMenu2 |= MENU_ENTRY;      //Set "entry" flag
                selectMenuRows = 3;         //"Select Menu" has 3 rows.
                selectMenuCurrRow = 0;
                selectMenuOldRow = 0xff;    //Ensure menu screen gets updated
                smMenu2 &= ~MENU_REDRAW;    //Clear "redraw" flag - cause display to get redrawn!
                blinkOn = true;
                tmrDisplay = MxTick::read_ms();
            }
            // Configure Main Screen //////////////////////////////////////////
            else {
                //Select current menu
                if(im4Oled.getOkBtnFalling() != 0) {
                    smMenu2 = MENU2_SETTINGS_DISPLAY_OFF_TIME + selectMenuCurrRow;
                }
                //Return Home
                else if(im4Oled.getStarBtnFalling() != 0) {
                    smMenu1 = MENU1_SELECT;
                }
                selectMenuOldRow = selectMenuCurrRow;
                selectMenuUpdate(selectMenuCurrRow, selectMenuRows, 16, tmrDisplay, blinkOn);
            }
            break;
        // Display Off Time ///////////////////////////////////////////////////
        case MENU2_SETTINGS_DISPLAY_OFF_TIME:
        {
            bool autoOffUpdated = false;
            //Save and Exit Configuration menu
            if( (im4Oled.getOkBtnFalling()!=0) || (im4Oled.getStarBtnFalling()!=0)) {
                smMenu2 = MENU2_SETTINGS_MAIN | MENU_ENTRY; //Return to MENU2_SETTINGS_MAIN.
                saveAppConfig(&appConfig);
                break;
            }
            //Up button
            else if(im4Oled.getUpBtnFalling() != 0) {
                if(appConfig.displayAutoOff<250) {
                    appConfig.displayAutoOff += 10;
                }
                autoOffUpdated = true;
            }
            //Down button
            else if(im4Oled.getDownBtnFalling() != 0) {
                if(appConfig.displayAutoOff>0) {
                    appConfig.displayAutoOff -= 10;
                }
                autoOffUpdated = true;
            }

            if(autoOffUpdated) {
                appData.flags.bits.dirtyConf = true;
                tmrDisplayOff = MxTick::read_ms() + (appConfig.displayAutoOff*1000);
            }

            //Toggle '=' sign before editable part of "Select Menu", it is at position 15.
            selectMenuBlinkEq(15, 1, tmrDisplay, blinkOn);
            break;
        }
        // Display Brightness /////////////////////////////////////////////////
        case MENU2_SETTINGS_DISPLAY_BRIGHTNESS:
            //Save and Exit Configuration menu
            if( (im4Oled.getOkBtnFalling()!=0) || (im4Oled.getStarBtnFalling()!=0)) {
                smMenu2 = MENU2_SETTINGS_MAIN | MENU_ENTRY; //Return to MENU2_SETTINGS_MAIN.
                saveAppConfig(&appConfig);
                break;
            }
            //Up button
            else if(im4Oled.getUpBtnFalling() != 0) {
                if(appConfig.displayBrigtness<2) {
                    appConfig.displayBrigtness += 1;
                }
                appData.flags.bits.dirtyConf = true;
            }
            //Down button
            else if(im4Oled.getDownBtnFalling() != 0) {
                if(appConfig.displayBrigtness>1) {
                    appConfig.displayBrigtness -= 1;
                }
                appData.flags.bits.dirtyConf = true;
            }
            //Check valid value

            //Toggle '=' sign before editable part of "Select Menu", it is at position 12.
            selectMenuBlinkEq(12, 2, tmrDisplay, blinkOn);
            break;
        // Restore Defaults ///////////////////////////////////////////////////
        case MENU2_SETTINGS_RESTORE_DEFAULTS:
            if (MxTick::read_ms() >= tmrDisplay) {
                blinkOn = !blinkOn;

                //Line 4
                oled.setTextCursor(0, 38); //x=0, Row 3 = 38
                if(blinkOn) {
                    oled.printf(" OK or * to Cancel!");
                    tmrDisplay = MxTick::read_ms() + 600;
                }
                else {
                    oled.printf("                   ");
                    tmrDisplay = MxTick::read_ms() + 200;
                }
            }
            //OK button = Restore Defaults
            if(im4Oled.getOkBtnFalling() != 0) {
                smMenu2 = MENU2_SETTINGS_MAIN | MENU_ENTRY; //Return to MENU2_SETTINGS_MAIN. Skip MENU_ENTRY!
                //Restore defaults
                appConfig.displayAutoOff = 0;
                appConfig.displayBrigtness = 2;
                saveAppConfig(&appConfig);
                appData.flags.bits.dirtyConf = true;
                break;
            }
            //Start button = return
            else if(im4Oled.getStarBtnFalling() != 0) {
                smMenu2 = MENU2_SETTINGS_MAIN | MENU_ENTRY;
                break;
            }
            break;
        } //switch (smMenu2 & MENU_MASK)

        if(appData.flags.bits.dirtyConf == true) {
            appData.flags.bits.dirtyConfDisp = true;    //Mark AppData dirty for display
        }
    } //if (smMenu1 == MENU1_SETTINGS_RADIO)




    ///////////////////////////////////////////////////////////////////////////
    // Level 2 menu for MENU1_CONFIG_RADIO ////////////////////////////////////
    if ((smMenu1 & MENU_MASK) == MENU1_CONFIG_RADIO) {

        //Does "Select Menu" have to be redrawn. Each screen can hold 4 menu items.
        if( ((smMenu2 & MENU_REDRAW) == 0) || ((selectMenuOldRow/4) != (selectMenuCurrRow/4)) ) {
            smMenu2 |= MENU_REDRAW; //Set "redraw" flag
            //First screen
            if((selectMenuCurrRow/4) == 0) {
                float fVal;
                MX_DEBUG("\r\nDraw Screen1");

                oled.clearDisplay();    //Clear display. Will cause WHOLE display to be redrawn! Use fillRect() to clear only required area if possible!
                //Line 1
                oled.setTextCursor(0, 2);
                oled.printf("Select or * To Return");

                //Line 2
                oled.setTextCursor(10, 16);  //x=10, leave space for '>' character
                oled.printf("Board = ");
                if (radioConfig[0].boardType == BOARD_INAIR4)
                    oled.printf("inAir4 ");
                else if (radioConfig[0].boardType == BOARD_INAIR9)
                    oled.printf("inAir9 ");
                else
                    oled.printf("inAir9B");

                //Line 3
                fVal = ((float)(radioConfig[0].frequency+100))/1000000.0f; //+100 to correct rounding error
                oled.setTextCursor(10, 27); //x=10, leave space for '>' character
                oled.printf("Freq = %d.%d ", (uint)fVal, MxHelpers::get_frac1(fVal));

                //Line 4
                oled.setTextCursor(10, 38); //x=10, leave space for '>' character
                oled.printf("BW = %s  ", arrBwChar[radioConfig[0].bw]);

                //Line 5
                oled.setTextCursor(10, 49); //x=10, leave space for '>' character
                oled.printf("SF = %d  ", radioConfig[0].sf);
            }
            //Second Screen
            else if((selectMenuCurrRow/4) == 1) {
                MX_DEBUG("\r\nDraw Screen2");

                oled.clearDisplay();    //Clear display. Will cause WHOLE display to be redrawn! Use fillRect() to clear only required area if possible!
                //Line 1
                oled.setTextCursor(0, 2);
                oled.printf("Select or * To Return");

                //Line 2
                oled.setTextCursor(10, 16); //x=10, leave space for '>' character
                oled.printf("Restore Defaults");
            }
        }


        switch (smMenu2 & MENU_MASK) {
        // Configure Main Screen //////////////////////////////////////////////
        case MENU2_CFGRADIO_MAIN:
            // Configure Main Screen - State Entry ////////////////////////////
            if ((smMenu2 & MENU_ENTRY) == 0) {
                smMenu2 |= MENU_ENTRY;      //Set "entry" flag
                selectMenuRows = 5;         //"Select Menu" has 5 rows. First screen has 4, second 1
                selectMenuCurrRow = 0;
                selectMenuOldRow = 0xff;    //Ensure menu screen gets updated
                blinkOn = true;
                tmrDisplay = MxTick::read_ms();
            }
            // Configure Main Screen //////////////////////////////////////////
            else {
                //Select current menu
                if(im4Oled.getOkBtnFalling() != 0) {
                    smMenu2 = MENU2_CFGRADIO_BOARD + selectMenuCurrRow;
                }
                //Return Home
                else if(im4Oled.getStarBtnFalling() != 0) {
                    smMenu1 = MENU1_SELECT;
                }
                selectMenuOldRow = selectMenuCurrRow;
                selectMenuUpdate(selectMenuCurrRow, selectMenuRows, 16, tmrDisplay, blinkOn);
            }
            break;
        // Configure Board ////////////////////////////////////////////////
        case MENU2_CFGRADIO_BOARD:
            //Save and Exit Configuration menu
            if( (im4Oled.getOkBtnFalling()!=0) || (im4Oled.getStarBtnFalling()!=0)) {
                smMenu2 = MENU2_CFGRADIO_MAIN | MENU_ENTRY; //Return to MENU2_CFGRADIO_MAIN. Skip MENU_ENTRY!
                saveRadioConfig(MXCONF_ID_RadioConfig0, &radioConfig[0]);
                break;
            }
            //Up button
            else if(im4Oled.getUpBtnFalling() != 0) {
                if (radioConfig[0].boardType < BOARD_INAIR9B) {
                    radioConfig[0].boardType += 1;
                    radioData[0].flags.bits.dirtyConf = true;
                }
            }
            //Down button
            else if(im4Oled.getDownBtnFalling() != 0) {
                if (radioConfig[0].boardType > 0) {
                    radioConfig[0].boardType -= 1;
                    radioData[0].flags.bits.dirtyConf = true;
                }
            }

            //Toggle '=' sign before editable part of "Select Menu", it is at position 7.
            selectMenuBlinkEq(7, 1, tmrDisplay, blinkOn);
            break;
        // Configure Frequency ////////////////////////////////////////////////////
        case MENU2_CFGRADIO_FREQ:
            //Save and Exit Configuration menu
            if( (im4Oled.getOkBtnFalling()!=0) || (im4Oled.getStarBtnFalling()!=0)) {
                smMenu2 = MENU2_CFGRADIO_MAIN | MENU_ENTRY; //Return to MENU2_CFGRADIO_MAIN. Skip MENU_ENTRY!
                saveRadioConfig(MXCONF_ID_RadioConfig0, &radioConfig[0]);
                break;
            }
            //Up button
            else if(im4Oled.getUpBtnFalling() != 0) {
                radioConfig[0].frequency += (im4Oled.getRepeatCount()<=20)?100000:1000000;   //Increment by 0.1 or 1MHz
                radioData[0].flags.bits.dirtyConf = true;
            }
            //Down button
            else if(im4Oled.getDownBtnFalling() != 0) {
                radioConfig[0].frequency -= (im4Oled.getRepeatCount()<=20)?100000:1000000;   //Decrement by 0.1 or 1MHz
                radioData[0].flags.bits.dirtyConf = true;
            }
            checkFrequency(0);  //Ensure new frequency values are OK

            //Toggle '=' sign before editable part of "Select Menu", it is at position 6.
            selectMenuBlinkEq(6, 2, tmrDisplay, blinkOn);
            break;
        // Configure BW ///////////////////////////////////////////////////////////
        case MENU2_CFGRADIO_BW:
            //Save and Exit Configuration menu
            if( (im4Oled.getOkBtnFalling()!=0) || (im4Oled.getStarBtnFalling()!=0)) {
                smMenu2 = MENU2_CFGRADIO_MAIN | MENU_ENTRY; //Return to MENU2_CFGRADIO_MAIN. Skip MENU_ENTRY!
                saveRadioConfig(MXCONF_ID_RadioConfig0, &radioConfig[0]);
                break;
            }
            //Up button
            else if(im4Oled.getUpBtnFalling() != 0) {
                if (radioConfig[0].bw < 9) {
                    radioConfig[0].bw += 1;
                    radioData[0].flags.bits.dirtyConf = true;
                }
            }
            //Down button
            else if(im4Oled.getDownBtnFalling() != 0) {
                if (radioConfig[0].bw > 4) {
                    radioConfig[0].bw -= 1;
                    radioData[0].flags.bits.dirtyConf = true;
                }
            }

            //Toggle '=' sign before editable part of "Select Menu", it is at position 4.
            selectMenuBlinkEq(4, 3, tmrDisplay, blinkOn);
            break;
        // Configure SF ///////////////////////////////////////////////////////////
        case MENU2_CFGRADIO_SF:
            //Save and Exit Configuration menu
            if( (im4Oled.getOkBtnFalling()!=0) || (im4Oled.getStarBtnFalling()!=0)) {
                smMenu2 = MENU2_CFGRADIO_MAIN | MENU_ENTRY; //Return to MENU2_CFGRADIO_MAIN. Skip MENU_ENTRY!
                saveRadioConfig(MXCONF_ID_RadioConfig0, &radioConfig[0]);
                break;
            }
            //Up button
            else if(im4Oled.getUpBtnFalling() != 0) {
                if (radioConfig[0].sf < 12) {
                    radioConfig[0].sf += 1;
                    radioData[0].flags.bits.dirtyConf = true;
                }
            }
            //Down button
            else if(im4Oled.getDownBtnFalling() != 0) {
                if (radioConfig[0].sf > 6) {
                    radioConfig[0].sf -= 1;
                    radioData[0].flags.bits.dirtyConf = true;
                }
            }

            //Toggle '=' sign before editable part of "Select Menu", it is at position 7.
            selectMenuBlinkEq(4, 4, tmrDisplay, blinkOn);
            break;
        // Restore Defaults ///////////////////////////////////////////////////
        case MENU2_CFGRADIO_RESTORE_DEFAULTS:
            if (MxTick::read_ms() >= tmrDisplay) {
                blinkOn = !blinkOn;

                //Line 1
                oled.setTextCursor(0, 16);  //x=0, Row 1 = 16
                if(blinkOn) {
                    oled.printf(" OK or * to Cancel!");
                    tmrDisplay = MxTick::read_ms() + 600;
                }
                else {
                    oled.printf("                   ");
                    tmrDisplay = MxTick::read_ms() + 200;
                }
            }
            //OK button = Restore Defaults
            if(im4Oled.getOkBtnFalling() != 0) {
                smMenu2 = MENU2_CFGRADIO_MAIN | MENU_ENTRY;
                //Restore defaults
                restoreRadioConfigDefaults(&radioConfig[0]);
                setRadioMode(radioData[0].mode, 0);
                radioData[0].flags.bits.dirtyConf = true;
                saveRadioConfig(MXCONF_ID_RadioConfig0, &radioConfig[0]);
                break;
            }
            //Start button = return
            else if(im4Oled.getStarBtnFalling() != 0) {
                smMenu2 = MENU2_CFGRADIO_MAIN | MENU_ENTRY;
                break;
            }
            break;
        } //switch (smMenu2 & MENU_MASK)

        if(radioData[0].flags.bits.dirtyConf == true) {
            radioData[0].flags.bits.dirtyConfDisp = true;   //Mark RadioData[0] dirty for display
        }
    } //if (smMenu1 == MENU1_CONFIG_RADIO)

#endif  //#if !defined(DISABLE_OLED)
}

/**
 * Update "Select Menu" blinking '>' cursor. Also update "selectMenuCurrRow" and "tmrDisplay".
 * Each "Select Menu" can have 4 rows with options to select.
 *
 * @param selectMenuCurrRow Current row index, a value from 0 to "selectMenuRows-1". Is the row where the '>' char is blinking.
 *        Each screen can have 4 rows, so first screen will be 0-3, second 4-7...
 * @param selectMenuRows Number of rows. Each screen can have 4 rows.
 * @param Y position of first '>' symbol.
 */
void selectMenuUpdate(uint8_t& selectMenuCurrRow, uint8_t selectMenuRows, uint8_t yFirstGt, int& tmrDisplay, bool& blinkOn) {
    //Up
    if(im4Oled.getUpBtnFalling() != 0) {
        if(selectMenuCurrRow-- == 0) {
            selectMenuCurrRow = selectMenuRows-1;
        }
    }
    //Down
    else if(im4Oled.getDownBtnFalling() != 0) {
        if (++selectMenuCurrRow == selectMenuRows) {
            selectMenuCurrRow = 0;
        }
    }

    if (MxTick::read_ms() >= tmrDisplay) {
        uint8_t rows = selectMenuRows-1;
        if (rows>3)
            rows=3;
        tmrDisplay = MxTick::read_ms() + 250;   //Blink '>' character every 500mS
        //Clear all '>' characters
        for(int i=0; i<=rows; i++) {
            oled.setTextCursor(0, yFirstGt+(i*11));
            if ((i==(selectMenuCurrRow%4)) && (blinkOn))
                oled.printf(">");
            else
                oled.printf(" ");
        }
        blinkOn = !blinkOn;
    }
}

/**
 * Blink '=' sign used in one of the 4 rows of the "Select Menu". Rows are at Y 16, 27, 38 or 49.
 * @param posEq The character position of the equal sign
 * @param row Current row number, a value from 1 to 4.
 */
void selectMenuBlinkEq(uint8_t posEq, uint8_t row, int& tmrDisplay, bool& blinkOn) {
    if (MxTick::read_ms() >= tmrDisplay) {
        blinkOn = !blinkOn;

        //Get position of '=' sign. Text always start at x=10, so "10+" required.
        //11 pixels between rows
        oled.setTextCursor(10+((posEq-1)*(DISPLAY_CHAR_WIDTH+1)), 5+(row*11));
        oled.putc(blinkOn?'=':' ');
        tmrDisplay = MxTick::read_ms() + 250;

        // ----- ALTERNATIVE -----
        //Toggle line under editable part of "Select Menu". First line is at 16.
        // - Keep "Board = " part = 8 character. Starts at x=10.
        // - Remove "inAirxB" part = 7 character.
        /*
        oled.fillRect(
                10+((posEq+1)*(DISPLAY_CHAR_WIDTH+1)),              // X - Editable part is after posEq characters (+1 for space between characters)
                5+DISPLAY_CHAR_HEIGHT+1+(row*11),                   // Y - Get location under editable part. 11 pixels between rows
                (6*(DISPLAY_CHAR_WIDTH+1)),                         // Width is 6 characters (+1 for space between characters). Add 'width' parameter if using this method!
                1, blinkOn?1:0);
        tmrDisplay += blinkOn?200:600;                              //On for 100ms, off for 900ms
        */
    }
}

#ifdef NZ32_ST1L_REV1
AnalogIn        ainVSense(PB_12);
#else
AnalogIn        ainVSense(PC_4);
#endif
AnalogIn        ainVBatt(PC_5);
#if (DONT_USE_A13_A14 == 0)
DigitalInOut    ctrVBatt(PA_13);
#endif

/** Check frequency is valid. If not, set to valid frequency
 */
void checkFrequency(uint8_t radioId) {
    uint32_t fMin, fMax;

    if (radioId >= RADIO_COUNT) {
        return;
    }

    fMin = 860000000;
    fMax = 930000000;
    if (radioConfig[radioId].boardType == BOARD_INAIR4) {
        fMin = 430000000;
        fMax = 435000000;
    }

    if (radioConfig[radioId].frequency < fMin) {
        radioConfig[radioId].frequency = fMin;
    }
    else if (radioConfig[radioId].frequency > fMax) {
        radioConfig[radioId].frequency = fMax;
    }
}

void updateBattLevel(void) {
    #define     SIZE_ARR_BATT_ADC   32      //Must be factor of 2 value (4,8,16,32...)
    //Static variables
    static uint8_t  arrBattAdcInit = 0;
    static uint8_t  putArrBattAdc = 0;
    static uint16_t arrBattAdc[SIZE_ARR_BATT_ADC];

    float           fval = 0;
    uint32_t        averBattAdc;
    uint16_t        mvBatt;       //Battery mv value
    uint16_t        i;

    //Assume voltage decreases linearly from 4.2 to 3.2V
    //Measure Vbatt. It doesn't seem to make lots of a difference if we disable the DC/DC converter!
#if (DONT_USE_A13_A14 == 0)
    ctrVBatt = 0;
    ctrVBatt.output();
#endif
    wait_ms(150);
    arrBattAdc[(putArrBattAdc++) & (SIZE_ARR_BATT_ADC-1)] = ainVBatt.read_u16(); // Converts and read the analog input value
    //First time, fill whole array with read value (first element just got current value in code above)
    if (arrBattAdcInit == 0) {
        arrBattAdcInit = 1;
        for (i=1; i<SIZE_ARR_BATT_ADC; i++) {
            arrBattAdc[i] = arrBattAdc[0];
        }
    }

    averBattAdc = 0;
    for (i=0; i<SIZE_ARR_BATT_ADC; i++) {
        //averBattAdc += arrBattAdc[i];
        averBattAdc = averBattAdc + arrBattAdc[i];
    }

    averBattAdc = averBattAdc / SIZE_ARR_BATT_ADC;
#if (DONT_USE_A13_A14 == 0)
    ctrVBatt.input();
#endif
    //fval = ((float)meas / (float)0xffff) * 6600; //6600 = 3300*2, because resistor divider = 2
    fval = averBattAdc * ((float)6600.0 / (float)65535.0);
    mvBatt = (uint16_t)fval;

    if (mvBatt < 3200) {
        percentBatt = 0;
    }
    else {
        percentBatt = (mvBatt - 3200)/10;  //Convert to value from 0 to 1000, then divide by 10 to get 0-100 percentage
    }

    if (percentBatt > 100)
        percentBatt = 100; //Not more than 100%
}

#endif //#if !defined(DISABLE_OLED)
