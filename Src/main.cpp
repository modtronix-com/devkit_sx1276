/**
 * File:      main.cpp
 *
 * Author:    Modtronix Engineering - www.modtronix.com
 *
 * Description:
 * Source code for Modtronix DEVKIT-SX1276!
 *
 * !!!!! Important DEBUGGING note !!!!!
 *
 *
 * ===== Debugging =====
 * To do software debugging via IDE, the following define must be set to 1 in "modtronix_config.h"!
 * #define  NZ32S_USE_A13_A14  1
 *
 * This will cause debug information to be written out on the serial port. When using a programmer with
 * USB to Serial Port converter, this debug information can be viewed on a standard serial terminal.
 *
 * ===== Build Release Firmware =====
 * To build firmware for release, Ensure following define is in "modtronix_config.h" file:
 * #define MX_DEBUG_DISABLE
 *
 * Next, enable one of the DEVKIT_FOR_INAIR9_xx defines in "app_def.h" file. For example, for 915MHz devkit
 * using inAir9 module:
 * #define DEVKIT_FOR_INAIR9_915
 *
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
#define THIS_IS_MAIN_CPP
#include "main.h"


// DEFINES ////////////////////////////////////////////////////////////////////
//#define         HAS_WATCHDOG

// VARIABLES //////////////////////////////////////////////////////////////////
InterruptIn     pwrInt(PC_10);
bool            pwrIntEn = false;
bool            goToSleep = false;  //If set, will do a controlled power down and go to sleep
int             tmrPwrInt = 0;
DigitalInOut    led1Pwr(PC_10);     //LED1 and power button. 1=LED on. 1=Button pressed.
DigitalOut      led2(PC_11);        //LED2, 1=LED on
DigitalOut      led3(PC_12);        //LED3, 0=LED on
DigitalOut      pinBuzzer(PC_9);    //Buzzer, 1=Buzzer on
AppConfig       appConfig;
AppData         appData;
RadioConfig     radioConfig[RADIO_COUNT];
RadioData       radioData[RADIO_COUNT];
InAir*          pRadios[RADIO_COUNT];
I2C             i2cBus1(PB_9, PB_8);
uint8_t         currRadio;
bool            i2cBus1OK = true;
static MxTick   mxTick;
int             tmrLED = 0;
int             tmrSendPing = 0;
const uint8_t   pingMsg[] = "pInG";
const uint8_t   pongMsg[] = "pOnG";
#if !defined(DISABLE_RESET_RADIO_USB_TIMERS) && (MX_ENABLE_USB==1)
    int tmrSecLastUsbCmd = 0;   //Timer value last time USB message was received
    int tmrSecLastTxRx = 0;     //Timer value since last Radio transmit or receive
#endif


// External GLOBAL VARIABLES //////////////////////////////////////////////////
#if ((MX_ENABLE_USB==1))
extern MxCmdBuffer <RX_BUF_USB_SIZE, RX_BUF_USB_COMMANDS, RX_BUF_USB_COUNTERTYPE> rxBufUsb;
extern MxCmdBuffer <TX_BUF_USB_SIZE, TX_BUF_USB_COMMANDS, TX_BUF_USB_COUNTERTYPE> txBufUsb;
#endif


// Function Prototypes ////////////////////////////////////////////////////////
void blinkLED(bool reset);
void processRxDataApp(uint8_t iRadio);
bool initializeRadio(uint8_t iRadio);
void radioTask(uint8_t iRadio);
void setRadioMode(uint8_t newMode, uint8_t iRadio);
#if ((MX_ENABLE_USB==1))
void processUsbCmds(void);
void processRxDataUSB(uint8_t iRadio);
#endif


void pwrIntISR() {
    if(pwrIntEn) {
        //Debounce 500ms
        if (mxTick.read_ms() >= tmrPwrInt) {
            tmrPwrInt += 1000;
            if (goToSleep == false) {
                goToSleep = true;
            }
            //System is reset in main code after we wake up
            //else {
            //    NVIC_SystemReset();
            //}
        }
    }
}

/** Main function */
int main() {
    uint8_t iRadio;
    volatile uint8_t dummy;
#if !defined(DISABLE_OLED)
    int tmrMenu = 0;
#endif

    //Turn all LEDs and buzzer on PT01NZ board off
    led1Pwr = 0;
    led2 = 0;
    led3 = 1;
    pinBuzzer = 0;

    currRadio = 0;
    appData.flags.Val = 0;

    //Configure shared LED1 & Power pin as interrupt input.
    led1Pwr.input();
    pwrIntEn = true;    //Enable code in pwrInt ISR. Required because ISR is also active when port is set as output!
    pwrInt.rise(pwrIntISR);

    memset(&radioConfig, 0, sizeof(radioConfig));

    //streamDebug.baud(230400);       //Set UART speed used for debugging
    //streamDebug.baud(19200);        //Set UART speed used for debugging
    streamDebug.baud(57600);        //Set UART speed used for debugging
    i2cBus1.frequency(I2C1_SPEED);  //Set I2C bus speed

    NZ32S::clear_led1();            //Initialize System LED = off

    NZ32S::enable_fast_charging();  //Enable fast charging

    #if defined(HAS_WATCHDOG)
    if (NZ32S::watchdog_caused_reset(true)) {
        MX_DEBUG("\r\nWatchdog Reset!");
    }

    //Start Watchdog Timer
    NZ32S::watchdog_start(4000);    //Set watchdog timer to 4 seconds
    #endif

    #if ((MX_ENABLE_USB==1))
        mx_usbcdc_init();
        rxBufUsb.enableReplaceCrLfWithEoc();
        txBufUsb.disableReplaceCrLfWithEoc();
    #endif

    //Load AppConfig from EEPROM. If EEPROM contains invalid data, NOTHING is loaded, and default values are used.
    //Default values are defined in AppConfig constructor
    loadAppConfig(MXCONF_ID_AppConfig, &appConfig);
    appData.flags.bits.dirtyConf = true;      //Cause AppConfig values to be used to update settings below

    //Initialize Radios
    for(iRadio=0; iRadio < RADIO_COUNT; iRadio++) {
        pRadios[iRadio] = NULL;

        restoreRadioConfigDefaults(&radioConfig[iRadio]); //Load default values

        //Try and determine the board type by the defined channel and power
        #if (RF_FREQUENCY < RF_MID_BAND_THRESH)
            radioConfig[iRadio].boardType = BOARD_INAIR4; //Use BOARD_INAIR4, it is a 14dBm output board
        #elif (TX_OUTPUT_POWER > 14)
            radioConfig[iRadio].boardType = BOARD_INAIR9B; //Use BOARD_INAIR9B, it has TX connected to PA Boost
        #else
            radioConfig[iRadio].boardType = BOARD_INAIR9; //Use BOARD_INAIR9, it is a 14dBm output
        #endif

        //Load RadioConfig from EEPROM. If EEPROM contains invalid data, NOTHING is loaded, and default values are used.
        if(loadRadioConfig(MXCONF_ID_RadioConfig0 + iRadio, &radioConfig[iRadio]) == true)
        {
            MX_DEBUG("\r\nRadio %d Initialized from EEPROM", iRadio);
        }

        radioData[iRadio].tmrRadio = 0;
        radioData[iRadio].smRadio = IDLE;
        radioData[iRadio].rxLen = 0;
        radioData[iRadio].rxListeners = 0;
        radioData[iRadio].rxErrCnt = 0;
        radioData[iRadio].rxErrCnt = 0;
        radioData[iRadio].mode = RADIO_MODE_STOPPED;
    }

    #if !defined(DISABLE_OLED)
    wait_ms(100);       //Startup Delay before initializing OLED
    mx_display_init();  //Initialize OLED Display and menu
    #endif

    //Main system loop
    while (1) {

        //This code is executed if the "Power" button is pressed. Puts the CPU to sleep.
        //A second press of the power button will wake the CPU, and reset the system.
        if(goToSleep) {
            //Disable all IO Ports
            led1Pwr = 0;
            led2 = 0;
            led3 = 1;
            pinBuzzer = 0;
            NZ32S::clear_led1();

            mx_display_on_off(0);

            //Put all radios to sleep
            for(iRadio=0; iRadio < RADIO_COUNT; iRadio++) {
                pRadios[iRadio]->Sleep();
            }

            wait_ms(200);

            {
                GPIO_InitTypeDef GPIO_InitStructure;

                //Configure all GPIO port pins in Analog Input mode (floating input trigger OFF)
                //EXCEPT the power button = PC_10
                GPIO_InitStructure.Pin = GPIO_PIN_All;
                GPIO_InitStructure.Mode = GPIO_MODE_ANALOG;
                GPIO_InitStructure.Pull = GPIO_NOPULL;
                HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);
                HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);
                HAL_GPIO_Init(GPIOD, &GPIO_InitStructure);
                HAL_GPIO_Init(GPIOH, &GPIO_InitStructure);
                //Exclude power button PC_10
                GPIO_InitStructure.Pin = GPIO_PIN_All & (~GPIO_PIN_10);
                HAL_GPIO_Init(GPIOC, &GPIO_InitStructure);
            }


            //__HAL_DBGMCU_FREEZE_WWDG();

            //////// Mote code begin ///////
            // Disable HAL tick interrupt
        #if(1)
            TIM_HandleTypeDef TimMasterHandle;
            //The following is the deepsleep code for the TARGET_MOTE_L152RC in sleep.c. It have less power then deepsleep();
            TimMasterHandle.Instance = TIM5;
            __HAL_TIM_DISABLE_IT(&TimMasterHandle, TIM_IT_CC2);

            // Select the regulator state in Stop mode: Set PDDS and LPSDSR bit according to PWR_Regulator value
            MODIFY_REG(PWR->CR, (PWR_CR_PDDS | PWR_CR_LPSDSR), PWR_LOWPOWERREGULATOR_ON);

            // Set SLEEPDEEP bit of Cortex System Control Register
            SET_BIT(SCB->SCR, ((uint32_t)SCB_SCR_SLEEPDEEP_Msk));

            // Request Wait For Interrupt
            __WFI();
            __NOP();
            __NOP();
            __NOP();
            // Reset SLEEPDEEP bit of Cortex System Control Register
            CLEAR_BIT(SCB->SCR, ((uint32_t)SCB_SCR_SLEEPDEEP_Msk));
            // After wake-up from STOP reconfigure the PLL
            SetSysClock();

            // Enable HAL tick interrupt
            __HAL_TIM_ENABLE_IT(&TimMasterHandle, TIM_IT_CC2);
            //////// Mote code end /////////
        #else
            deepsleep();
        #endif

            //After we wake up, reset system
            NVIC_SystemReset();
        }

#if !defined(DISABLE_RESET_RADIO_USB_TIMERS)
        //Watchdog timer refresh with following conditions:
        // - A Radio reception or transmission occurred less than RESET_TIMEOUT_RADIO seconds ago
        // - A USB Command was processed less than RESET_TIMEOUT_USB_CMD seconds ago
        if ( ((mxTick.read_sec()-tmrSecLastTxRx) < RESET_TIMEOUT_RADIO) && ((mxTick.read_sec()-tmrSecLastUsbCmd) < RESET_TIMEOUT_USB_CMD))
        {
            #if defined(HAS_WATCHDOG)
            NZ32S::watchdog_refresh();
            #endif
        }
#else
        #if defined(HAS_WATCHDOG)
        NZ32S::watchdog_refresh();
        #endif
#endif

        //USB Task
        #if ((MX_ENABLE_USB==1))
        mx_usbcdc_task();
        processUsbCmds();
        #endif

        //Try to fix any I2C error
        if (i2cBus1OK == false) {
            i2cBus1OK = true;
            NZ32S::i2c_reset(1, PB_9, PB_8);
            wait_ms(2);
            i2cBus1.frequency(I2C1_SPEED);  //Set bus speed
            MX_DEBUG("\r\nI2C Bus Error!");
            wait_ms(2);
        }

        //Service menu every 10mS
        #if !defined(DISABLE_OLED)
        if (mxTick.read_ms() >= tmrMenu) {
            tmrMenu += 10; //Every 10mS
            mx_menu_task();  //OLED Display and menu task
        }

        //OLED Display task
        mx_display_task();
        #endif

        //AppConfig changed
        if (appData.flags.bits.dirtyConf == true) {
            appData.flags.bits.dirtyConf = false;
            appData.flags.bits.dirtyConfDisp = true;    //Mark AppData dirty for display
            #if !defined(DISABLE_OLED)
            mx_display_update();
            #endif
        }

        blinkLED(false); //Blink system LED

        //If Master, send PING message
        if (radioData[0].mode == RADIO_MODE_MASTER) {

            if (mxTick.read_ms() >= tmrSendPing) {
                tmrSendPing = mxTick.read_ms() + PING_PERIOD_MS;

                //Check if PONG reply was received for previous PING we sent?
                if(appData.oldRxCountPingPong == appData.rxCountPingPong) {
                    appData.rxErrCountPingPong++;
                }
                appData.oldRxCountPingPong = appData.rxCountPingPong;   //Remember "rxCountPingPong"

                //Send PING message to slave. Message has following format:
                // - Byte0:     Slave address
                // - Byte1-4:   "pInG"
                radioData[0].txBuf.put(appConfig.remotelAdr);
                radioData[0].txBuf.putArray((unsigned char*)pingMsg, 4);
            }
        }

        // Iterate thru all Radios ////////////////////////////////////////////
        // - Check if any received data has to be processed?
        // - Check if RX error, or message lost. If so, print debug message
        // - Configure radio
        // - Initialize Radio
        for(iRadio=0; iRadio < RADIO_COUNT; iRadio++) {
            //If config dirty, set initialized = false. Causes radio to be initialized again
            if (radioData[iRadio].flags.bits.dirtyConf == true) {
                radioData[iRadio].flags.bits.dirtyConf = false;
                radioData[iRadio].flags.bits.initialized = false;
            }

            // Initialize radio
            if (radioData[iRadio].flags.bits.initialized == false) {
                if(initializeRadio(iRadio) == false) {
                    continue;   //If not successful, break out of loop and go to next radio
                }
                radioData[iRadio].flags.bits.initialized = true;
            }

            //Low level radio Task
            pRadios[iRadio]->task();

            //High level radio Task
            radioTask(iRadio);

            //RX message was lost
            if (radioData[iRadio].flags.bits.rxMsgLost == true) {
                radioData[iRadio].flags.bits.rxMsgLost = false;
                MX_DEBUG("\r\nRX%d Msg Lost!", iRadio);
            }

            //Process Received Data - There are "Rx Listeners" that have to process received data.
            if (radioData[iRadio].rxListeners != 0) {
                //Call handlers for all "Rx Listeners"
                processRxDataApp(iRadio);       //Process received data for Application
                #if ((MX_ENABLE_USB==1))
                    processRxDataUSB(iRadio);   //Process received data for USB
                #endif

                //All Rx Listeners have processed receive data(rxListeners=0), reset rxLen to 0. This enables new messages to be received.
                if(radioData[iRadio].rxListeners==0) {
                    radioData[iRadio].rxLen = 0;
                }
            }

            //RX Status
            //if (radioData[iRadio].rxStatus != RX_STATUS_OK) {
            //    MX_DEBUG("\r\nRX%d Err = %d!", iRadio, radioData[iRadio].rxStatus);
            //    radioData[iRadio].rxStatus = 0;
            //}

        } //for(iRadio=0; iRadio < RADIO_COUNT; iRadio++) {

        //dummy = 6;  //Without a command here, program does NOT run???????
    }
}

void OnTxDone(uint8_t radioID) {
    pRadios[radioID]->Sleep();
    radioData[radioID].smRadio = TX_DONE;
}

void OnRxDone(uint8_t radioID, uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {
    //For "RX mode" 0 and 1, exit receive mode, and go to Standby/Sleep.
    //If radio was configured for continuous reception, this will cancel it
    if (radioConfig[radioID].rxMode < 2) {
        pRadios[radioID]->Standby();
    }

    //Only add data to buffer if empty. Else, data is lost!
    if(radioData[radioID].rxLen == 0) {
        radioData[radioID].rxStatus = RX_STATUS_OK;
        radioData[radioID].RssiValue = rssi;
        radioData[radioID].SnrValue = snr;
        radioData[radioID].smRadio = RX_DONE;
        memcpy(&radioData[radioID].rxBuf[0], payload, size);
        radioData[radioID].rxLen = size;
    }
    else {
        radioData[radioID].flags.bits.rxMsgLost = 1;
    }
}

void OnTxTimeout(uint8_t radioID) {
    pRadios[radioID]->Sleep();
    radioData[radioID].smRadio = TX_TIMEOUT;
}

/**
 * Seems like this can be called due to 1 of 2 events:
 * - The timeout given for RadioConfig.rxTimeout expired. This uses a MCU timer to detect timeout. Nothing to do
 *   with SX1276 chip timeout
 * - The timeout given for RadioConfig.symbolTimeout expired. This is the timeout set on the SX1276 chip. In single
 *   Receive mode, the radio searches for a preamble during a given time window. If a preamble hasn’t been found at
 *   the end of the time window, the chip generates the RxTimeout.
 */
void OnRxTimeout(uint8_t radioID) {
    //For "RX mode" 0 and 1, exit receive mode, and go to Standby/Sleep
    //If radio was configured for continuous reception, this will cancel it
    if (radioConfig[radioID].rxMode < 2) {
        //pRadios[radioID]->Standby();
        pRadios[radioID]->Sleep();
    }
    radioData[radioID].smRadio = RX_TIMEOUT;
}

void OnRxError(uint8_t radioID) {
    //For "RX mode" 0 and 1, exit receive mode, and go to Standby/Sleep
    //If radio was configured for continuous reception, this will cancel it
    if (radioConfig[radioID].rxMode < 2) {
        //pRadios[radioID]->Standby();
        pRadios[radioID]->Sleep();
    }
    radioData[radioID].smRadio = RX_ERROR;
}

#if ((MX_ENABLE_USB==1))
/**
 * Process any USB Commands
 */
void processUsbCmds() {
    #define MX_NAME_LEN    32
    #define MX_VALUE_LEN   32
    #define MX_BIN_LEN     32
    uint8_t     nameBuf[MX_NAME_LEN];
    uint8_t     valueBuf[MX_VALUE_LEN];
    uint8_t     binBuf[MX_VALUE_LEN];
    uint8_t     iRadio;
    uint8_t     currCmdRadio;       //If the current command has "namex=value" where 'x' is ratio number 0-(number of radios-1)
    uint8_t     trailingNameDig;    //If a trailing digit was removed from 'name', this gives it's value. Else, 0xff.
    uint16_t    nameLen=0;
    uint16_t    valueLen=0;
    uint16_t    binLen=0;
    uint16_t    cmdLen;
    uint32_t    val;
    uint8_t     cmdResponse;        //Assign a CMD_RESPONCE_xx value
    bool        isNameValue;

    enum CMD_RESPONCE {
        CMD_RESPONCE_UNKNOWN = 0,
        CMD_RESPONCE_OK,
        CMD_RESPONCE_NONE
    };

    //////////////////////////////////////////
    //Sent Command - command send by this unit
    //
    //rn=asciiCmd    - Packet received on given transceiver. Where n is a value from 0-(number of radios)
    //      Where asciiCmd is an "Ascii Command" string containing "two character upper case hex" data
    //
    //
    /////////////////////////////////////////////////////////
    //Receive Commands - commands that this unit will process
    //
    //tv=n  - Set transceiver to use for all commands that does not specify tranceiber(t=5566 for example)
    //      Where n is a value from 0-(number of radios)
    //
    //t=asciiCmd    - Transmit Packet on current transceiver (set via previous 'tr' command)
    //tn=asciiCmd   - Same as "t", but n gives transceiver to use. A value from 0 to (Radios-1).
    //      Where asciiCmd is an "Ascii Command" string containing "two character upper case hex" data
    //      If transmission successful, a "rn=tok" message will be sent (n = transceiver ID)
    //      If transmission timeout, a "rn=tto" message will be sent (n = transceiver ID)
    //
    //tm=v  - Set "transmit mode" for current transceiver
    //tmn=v - Same as "tm", but n gives transceiver to use. A value from 0 to (Radios-1).
    //       Valid modes(value of 'v') are:
    //       0 = Radio enters Idle mode after transmitting a message
    //       1 = Radio enters Receive mode after transmitting a message
    //
    //tpw=v  - Set "transmit power" for current transceiver
    //tpwn=v - Same as "tpw", but n gives transceiver to use. A value from 0 to (Radios-1).
    //       Given power values(value of 'v') must be from 0 to 20 (0 to 20dBm)
    //
    //bw=x  - Set bandwidth of current transceiver
    //bwn=x - Same as "bw", but n gives transceiver to use. A value from 0 to (Radios-1).
    //      Bandwidth is given by x is one of the following: 6=62.5kHz, 7=125kHz, 8=250kHz, 9=500kHz (0 to 5 not supported = 7.8kHz to 41.7kHz)
    //
    //crc=x  - Enable or disable CRC
    //crcn=x - Same as "crc", but n gives transceiver to use. A value from 0 to (Radios-1).
    //
    //f=x   - Set frequency of current transceiver (set via previous 'tr' command).
    //fn=x  - Set frequency of given transceiver, where n is 0 to 9
    //      Frequency is given by x, and is a value from 400000 to 980000
    //
    //rx
    //rxn   - Where n is 0-9
    //      Put given ratio in receive mode. Timeout given with previous "Receive Time Out" command(rto=n) is used.
    //      If message received successfully, a "rn=asciiString" message will be sent (n = transceiver ID)
    //      If receive error, a "rn=rer" message will be sent (n = transceiver ID)
    //      If receive timeout, a "rn=rto" message will be sent (n = transceiver ID)
    //
    //rst - Reset this module
    //
    //rm=n  - Set "receive mode" for current transceiver
    //rmn=n - Same as "rm", but n gives transceiver to use. A value from 0 to (Radios-1).
    //      Valid modes are:
    //      0 = Single receive mode, Radio enters Idle mode after receiving a message
    //      1 = Continues receive mode, Radio enters Idle mode after receiving first message
    //      2 = Continues receive mode, Radio stays in receive mode after receiving a message
    //
    //rs  - Request RSSI value of last reception.
    //rsn - Same as "rs", but n gives transceiver to use. A value from 0 to (Radios-1).
    //      Returns the RSSI value for the previous reception for given transceiver.
    //      Return format is "rsn=v", where n is transceiver ID, and v = RSSI value
    //
    //rto=n  - Set "receive timeout" in milli seconds for current transceiver
    //rton=n - Same as "rto", but n gives transceiver to use. A value from 0 to (Radios-1).
    //       Valid modes are:
    //       0 = No timeout, stay in receive mode
    //
    //rsym=n    - Set "receive symbol" value.
    //rsymn=n   - Same as "rsym", but n gives transceiver to use. A value from 0 to (Radios-1).
    //      In Single Receive mode, the radio searches for a preamble during a given time window. If a preamble hasn’t been
    //      found at the end of the time window, the chip generates the RxTimeout interrupt and goes back to stand-by mode.
    //      The length of the window (in symbols) is defined by this value, and should be in the range of 4 (minimum time
    //      for the modem to acquire lock on a preamble) up to 1023 symbols. Default is 5. If no preamble is detected during
    //      this window the RxTimeout interrupt is generated and the radio goes back to stand-by mode.
    //
    //pr=l   - Set "preamble length" in symbols
    //prn=l  - Same as "pr", but n gives transceiver to use. A value from 0 to (Radios-1).
    //      Where l = Preamble length in symbols, 4 to 1000
    //
    //sf=x  - Set spreading factor of current transceiver (set via previous 'tr' command)
    //sfn=x - Set spreading factor of given transceiver, where n is 0 to 9
    //      Spreading factor is given by x and is a value from 7 to 12
    //
    //tvs - Request Status of all Transceiver
    //      Returns status of all transceivers. For each transceiver, will return command will following format:
    //      "tvsn=x", where n is transceiver ID, and x is status:
    //       - 0 = Uninitialized
    //       - 1 = OK
    //
    //Check if command in buffer
    while (rxBufUsb.hasCommand()) {
        currCmdRadio = currRadio;   //Use current radio. Will be overwritten below if "namex=value" pair with radio number
        cmdLen = rxBufUsb.getCommandLength();

        MX_DEBUG_INFO("\r\nCmd(%d): ", cmdLen);
        for(int i=0; i<cmdLen; i++) {
            MX_DEBUG_INFO("%c", rxBufUsb.peekAt(i));
        }

        if (cmdLen < 2) {
            MX_DEBUG_INFO("\r\nCmd too short, removed!");
            rxBufUsb.removeCommand();
            break;
        }

        //Get name=value pair
        nameLen = rxBufUsb.getCommandName(nameBuf, MX_NAME_LEN, isNameValue);
        //This command has 'name=value' format
        if((nameLen>0) && (isNameValue==true)) {
            //At this stage, we know nameLen+1 = first character of 'value'
            valueLen = rxBufUsb.getCommandValue(valueBuf, MX_VALUE_LEN, nameLen+1);
            //if (valueLen != 0) {
            //    MX_DEBUG_INFO("\r\nN='%s' V='%s'", nameBuf, valueBuf);
            //}
        }
        else {
            isNameValue=false;
        }

        //Check if 'name' ends with digit, and if so, remove it.
        //The digit has to be the radio number, a value from 0 to (number of radios-1)
        trailingNameDig = 0xff; //Name does not have a trailing digit
        if((nameLen>1) && (nameBuf[nameLen-1]<RADIO_COUNT_CHAR) && (nameBuf[nameLen-1]>='0')) {
            nameLen--;  //Remove trailing digit
            currCmdRadio    = nameBuf[nameLen]-'0';
            trailingNameDig = currCmdRadio;      //Indicate the trailing digit was removed from 'name'
            //MX_DEBUG_INFO("\r\ncurCmdRadio=%d", currCmdRadio);
            nameBuf[nameLen] = 0;   //Overwrite last byte of name (radio number) with NULL
        }

        cmdResponse = CMD_RESPONCE_UNKNOWN;

        //Process name-value commands
        //NOTE that if 'name' ended with trailing digit (0 - max radio), it is removed and saved in trailingNameDig
        if (isNameValue) {
            if (cmdLen < 4) {
                MX_DEBUG_INFO("\r\nCmd too short, removed!");
                rxBufUsb.removeCommand();
                break;
            }

            //t....... - Command starting with 't'
            if(nameBuf[0]=='t') {
                // ---------- Name-Value COMMAND ----------
                //t=asciiCmd    - Transmit Packet on current transceiver
                //tn=asciiCmd   - Same as "t", but n gives transceiver to use. A value from 0 to (Radios-1).
                //      Where asciiCmd is an "Ascii Command" string containing "two character upper case hex" data
                //      If transmission successful, a "rn=tok" message will be sent (n = transceiver ID)
                //      If transmission timeout, a "rn=tto" message will be sent (n = transceiver ID)
                if(nameLen==1) {
                    cmdResponse = CMD_RESPONCE_NONE;        //This command already send a "rn=.." reply
                    //convert to binary
                    binLen = decodeAsciiCmd(binBuf, MX_BIN_LEN, valueBuf);
                    binBuf[binLen>=MX_BIN_LEN?(MX_BIN_LEN-1):binLen] = 0;   //NULL terminate
                    //MX_DEBUG_INFO("Bin=%s", valueBuf);
                    //MX_DEBUG_INFO("\r\nBin=");
                    //for(int i=0; i<binLen; i++) {
                    //    MX_DEBUG_INFO("%02x,", binBuf[i]);
                    //}

                    //Send value
                    for (int i=0; i<binLen; i++) {
                        radioData[currCmdRadio].txBuf.put(binBuf[i]);
                    }
                    //radioData[currCmdRadio].tmrRadio = timerMain.read_ms() + 10;    //Delay sending for 10ms
                }
                //t. - Command exactly 2 characters long, starting with 't'
                else if(nameLen==2) {
                    // ---------- Name-Value COMMAND ----------
                    //tv=n  - Set current transceiver to use for all commands that does not specify transceiver
                    //      Where n is a value from 0-(number of radios)
                    if(nameBuf[1]=='v') {
                        if((valueBuf[0]<RADIO_COUNT_CHAR) && (valueBuf[0]>='0')) {
                            //Set new current transceiver radio
                            currRadio = valueBuf[0]-'0';
                            cmdResponse = CMD_RESPONCE_OK;
                            MX_DEBUG_INFO("\r\nTrcvr=%d", currRadio);
                        }
                    }
                    // ---------- Name-Value COMMAND ----------
                    //tm=n  - Set "transmit mode" for current transceiver
                    //tmn   - Same as "tm", but n gives transceiver to use. A value from 0 to (Radios-1).
                    //      Valid modes are:
                    //      0 = Radio enters Idle mode after transmitting a message
                    //      1 = Radio enters Receive mode after transmitting a message
                    else if(nameBuf[1]=='m') {
                        if((valueBuf[0]>='0') && (valueBuf[0]<='1')) {
                            cmdResponse = CMD_RESPONCE_OK;
                            MX_DEBUG_INFO("\r\nTx mode=%d", valueBuf[0]-'0');
                            if (radioConfig[currCmdRadio].txMode != valueBuf[0]-'0') {
                                radioConfig[currCmdRadio].txMode = valueBuf[0]-'0';
                                radioData[currCmdRadio].flags.bits.dirtyConf = true;
                            }
                        }
                    }
                }
                //tpw=v  - Set "transmit power" for current transceiver
                //tpwn=v - Same as "tpw", but n gives transceiver to use. A value from 0 to (Radios-1).
                //       Given power values(value of 'v') must be from 0 to 20 (0 to 20dBm)
                else if(strcmp((const char*)&nameBuf[1], "pw") == 0) {
                    uint8_t newVal;
                    newVal = atoi((const char*)&valueBuf[0]);
                    if (newVal <= 20) {
                        cmdResponse = CMD_RESPONCE_OK;
                        MX_DEBUG_INFO("\r\nTX Pwr=%d", newVal);
                        //Set new CRC
                        if (radioConfig[currCmdRadio].power != newVal) {
                            radioConfig[currCmdRadio].power = newVal;
                            radioData[currCmdRadio].flags.bits.dirtyConf = true;
                        }
                    }
                }
            }
            //r....... - Command starting with 'r'
            else if(nameBuf[0]=='r') {
                //r. - Command exactly 2 characters long, starting with 'r'
                if(nameLen==2) {
                    // ---------- Name-Value COMMAND ----------
                    //rm=n  - Set "receive mode" for current transceiver
                    //rmn=n - Same as "rm", but n gives transceiver to use. A value from 0 to (Radios-1).
                    //      Valid modes are:
                    //      0 = Single receive mode, Radio enters Idle mode after receiving a message
                    //      1 = Continues receive mode, Radio enters Idle mode after receiving first message
                    //      2 = Continues receive mode, Radio stays in receive modo after receiving a message
                    if(nameBuf[1]=='m') {
                        int val;
                        val = valueBuf[0]-'0';
                        if((val >= 0) && (val <= 2)) {
                            cmdResponse = CMD_RESPONCE_OK;
                            MX_DEBUG_INFO("\r\nRx mode=%d", val);

                            //Set value in RadioConfig
                            if (radioConfig[currCmdRadio].rxMode != val) {
                                radioConfig[currCmdRadio].rxMode = val;
                                radioData[currCmdRadio].flags.bits.dirtyConf = true;
                            }
                        }
                    }
                }
                //r... - Command longer than 2 characters, starting with 'r'
                else  {
                    // ---------- Name-Value COMMAND ----------
                    //rto=n  - Set "receive timeout" in milli seconds for current transceiver
                    //rton=n - Same as "rto", but n gives transceiver to use. A value from 0 to (Radios-1).
                    //       Valid modes are:
                    //       0 = No timeout, stay in receive mode
                    //if((nameBuf[1]=='t') && (nameBuf[2]=='o')) {
                    if(strcmp((const char*)&nameBuf[1], "to") == 0) {
                        val = atoi((const char *)valueBuf);
                        //Valid value from 0 to 10,000
                        if ((val >= 0) && (val <= 10000)) {
                            cmdResponse = CMD_RESPONCE_OK;
                            MX_DEBUG_INFO("\r\nRx TO=%d ms", val);
                            val = val * 1000;
                            if (radioConfig[currCmdRadio].rxTimeout != val) {
                                radioConfig[currCmdRadio].rxTimeout = val;
                                radioData[currCmdRadio].flags.bits.dirtyConf = true;
                            }
                        }
                        else {
                            MX_DEBUG_INFO("\r\nRx TO ERR!");
                        }
                    }
                    //rsym=n    - Set "receive symbol" value.
                    //rsymn=n   - Same as "rsym", but n gives transceiver to use. A value from 0 to (Radios-1).
                    if(strcmp((const char*)&nameBuf[1], "sym") == 0) {
                        val = atoi((const char *)valueBuf);
                        //Valid value from 4 to 1023
                        if ((val >= 4) && (val <= 1023)) {
                            cmdResponse = CMD_RESPONCE_OK;
                            MX_DEBUG_INFO("\r\nRxSymTO=%d", val);
                            if (radioConfig[currCmdRadio].symbolTimeout != val) {
                                radioConfig[currCmdRadio].symbolTimeout = val;
                                radioData[currCmdRadio].flags.bits.dirtyConf = true;
                            }
                        }
                        else {
                            MX_DEBUG_INFO("\r\nRxSymTO ERR!");
                        }
                    }
                }
            }
            //b....... - Command starting with 'b'
            else if(nameBuf[0]=='b') {
                //b. - Command exactly 2 characters long, starting with 'b'
                if(nameLen==2) {
                    // ---------- Name-Value COMMAND ----------
                    //bw=x      - Set bandwidth of current transceiver (set via previous 'tr' command)
                    //bwn=x     - Same as "bw", but n gives transceiver to use. A value from 0 to (Radios-1).
                    //      Bandwidth is given by x is one of the following: 6=62.5kHz, 7=125kHz, 8=250kHz, 9=500kHz (0 to 5 not supported = 7.8kHz to 41.7kHz)
                    if(nameBuf[1]=='w') {
                        if ((valueLen==1) && (valueBuf[0]>='6') && (valueBuf[0]<='9')) {
                            cmdResponse = CMD_RESPONCE_OK;
                            MX_DEBUG_INFO("\r\nBW=%d", valueBuf[0] - '0');
                            //Set new bandwidth
                            if (radioConfig[currCmdRadio].bw != valueBuf[0]-'0') {
                                radioConfig[currCmdRadio].bw = valueBuf[0]-'0';
                                radioData[currCmdRadio].flags.bits.dirtyConf = true;
                            }
                        }
                    }
                }
            }
            //c....... - Command starting with 'c'
            else if(nameBuf[0]=='c') {
                //c.. - Command exactly 3 characters long, starting with 'c'
                if(nameLen==3) {
                    // ---------- Name-Value COMMAND ----------
                    //crc=x  - Enable or disable CRC
                    //crcn=x - Same as "crc", but n gives transceiver to use. A value from 0 to (Radios-1).
                    if(strcmp((const char*)&nameBuf[1], "rc") == 0) {
                        bool newVal;
                        cmdResponse = CMD_RESPONCE_OK;
                        newVal = valueBuf[0]=='0'?false:true;
                        MX_DEBUG_INFO("\r\nCRC=%d", newVal);
                        //Set new CRC
                        if (radioConfig[currCmdRadio].conf.lora.crcEnable != newVal) {
                            radioConfig[currCmdRadio].conf.lora.crcEnable = newVal;
                            radioData[currCmdRadio].flags.bits.dirtyConf = true;
                        }
                    }
                }
            }
             //p....... - Command starting with 'p'
             else if(nameBuf[0]=='p') {
                 //s. - Command exactly 2 characters long, starting with 's'
                 if(nameLen==2) {
                     // ---------- Name-Value COMMAND ----------
                     //pr=l   - Set "preamble length" in symbols
                     //prn=l  - Same as "pr", but n gives transceiver to use. A value from 0 to (Radios-1).
                     //     WhereValid l = Preamble length in symbols, 4 to 1000
                     if(nameBuf[1]=='r') {
                         val = atoi((const char *)valueBuf);
                         if ((val >= 4) && (val <= 1000)) {
                             cmdResponse = CMD_RESPONCE_OK;
                             MX_DEBUG_INFO("\r\nPre=%d", val);

                             //Set new Preamble
                             if (radioConfig[currCmdRadio].preambleLength != val) {
                                 radioConfig[currCmdRadio].preambleLength = val;
                                 radioData[currCmdRadio].flags.bits.dirtyConf = true;
                             }
                         }
                     }
                 }
             }
            //s....... - Command starting with 's'
            else if(nameBuf[0]=='s') {
                //s. - Command exactly 2 characters long, starting with 's'
                if(nameLen==2) {
                    // ---------- Name-Value COMMAND ----------
                    //sf=x      - Set spreading factor of current transceiver (set via previous 'tr' command)
                    //sfn=x     - Same as "sf", but n gives transceiver to use. A value from 0 to (Radios-1).
                    //      Spreading factor is given by x and is a value from 7 to 12
                    if(nameBuf[1]=='f') {
                        val = atoi((const char *)valueBuf);
                        if ((val >= 7) && (val <= 12)) {
                            cmdResponse = CMD_RESPONCE_OK;
                            MX_DEBUG_INFO("\r\nSF=%d", val);

                            //Set new SF
                            if (radioConfig[currCmdRadio].sf != val) {
                                radioConfig[currCmdRadio].sf = val;
                                radioData[currCmdRadio].flags.bits.dirtyConf = true;
                            }
                        }
                    }
                }
            }
            //f....... - Command starting with 'f'
            else if(nameBuf[0]=='f') {
                // ---------- Name-Value COMMAND ----------
                //f=x       - Set frequency of current transceiver (set via previous 'tr' command).
                //fn=x      - Same as "f", but n gives transceiver to use. A value from 0 to (Radios-1).
                //      Frequency is given by x, and is a value from 400000000 to 950000000 (or short 4000 to 9500)
                if(nameLen==1) {
                    val = atoi((const char *)valueBuf);
                    //Support short format giving MHz/10. For example, 916.7MHz = 9167
                    if (valueLen==4) {
                        val = val * 100000;
                    }
                    if ((val >= 400000000) && (val <= 950000000)) {
                        cmdResponse = CMD_RESPONCE_OK;
                        MX_DEBUG_INFO("\r\nF=%d", val);
                        if (radioConfig[currCmdRadio].frequency != val) {
                            radioConfig[currCmdRadio].frequency = val;
                            radioData[currCmdRadio].flags.bits.dirtyConf = true;
                        }
                    }
                }
                //f. - Command exactly 2 characters long, starting with 'f'
                //else if(nameLen==2) {
                //}
            }
        }   //if (isNameValue)
        //////////////////////////////////////////////////////////////////
        //Command only (NO "value" part)
        //////////////////////////////////////////////////////////////////
        //Command does NOT have "name=value" format. Command is in nameBuf
        //NOTE that if command ended with trailing digit (0 - max radio), it is removed and saved in trailingNameDig
        else {
            //r....... - Command starting with 'r'
            if(nameBuf[0]=='r') {
                //r. - Command exactly 2 characters long, starting with 'r'
                if(nameLen==2) {
                    // ---------- COMMAND ----------
                    //rs  - Request RSSI value of last reception.
                    //rsn - Same as "rs", but n gives transceiver to use. A value from 0 to (Radios-1).
                    //      Returns the RSSI value for the previous reception for given transceiver.
                    //      Return format is "rsn=v", where n is transceiver ID, and v = RSSI value
                    if(nameBuf[1]=='s') {
                        int len = 0;
                        cmdResponse = CMD_RESPONCE_NONE;    //This command already send a reply
                        //Use nameBuf to build reply string
                        nameBuf[0] = 'r';
                        nameBuf[1] = 's';
                        nameBuf[2] = '0' + currCmdRadio;
                        nameBuf[3] = '=';
                        //itoa((int)(radioData[currCmdRadio].RssiValue), (char*)&nameBuf[4], 10);
                        len = MxHelpers::cvt_int16_to_ascii_str(radioData[currCmdRadio].RssiValue, &nameBuf[4]);
                        nameBuf[4+len] = ';';
                        txBufUsb.putArray(nameBuf, len+5);

                        #if (DEBUG_ENABLE_MAIN==1)
                        nameBuf[5+len] = 0;
                        MX_DEBUG_INFO("\r\n%s(%d)", nameBuf, radioData[currCmdRadio].RssiValue);
                        #endif
                    }
                    // ---------- COMMAND ----------
                    //rx  - Put ratio in receive mode. Timeout given with previous "Receive Time Out" command(rto=n) is used.
                    //rxn - Same as "rx", but n gives transceiver to use. A value from 0 to (Radios-1).
                    //      If message received successfully, a "rn=asciiString" message will be sent (n = transceiver ID)
                    //      If receive error, a "rn=rer" message will be sent (n = transceiver ID)
                    //      If receive timeout, a "rn=rto" message will be sent (n = transceiver ID)
                    else if(nameBuf[1]=='x') {
                        InAir* pRadio           = pRadios[currCmdRadio];
                        RadioData* pRadioData   = &radioData[currCmdRadio];
                        cmdResponse = CMD_RESPONCE_NONE;    //This command already send a reply
                        MX_DEBUG_INFO("\r\nRx%d", currCmdRadio);
                        if (pRadio!=NULL) {
                            pRadioData->smRadio = IDLE;
                            pRadio->Rx(radioConfig[currCmdRadio].rxTimeout);    //Continues reception mode
                        }
                        else {
                            MX_DEBUG_INFO(" - NULL!!");
                        }
                    }
                }   //if(nameLen==2)
                // ---------- COMMAND ----------
                //rst   - Run command
                else if(strcmp((const char*)&nameBuf[1], "st") == 0) {
                    cmdResponse = CMD_RESPONCE_OK;
                    MX_DEBUG("\r\nResetting!");
                    wait(1);
                    NVIC_SystemReset();
                }
                // ---------- COMMAND ----------
                //run   - Run command
                else if(strcmp((const char*)&nameBuf[1], "un") == 0) {
                    cmdResponse = CMD_RESPONCE_OK;
                    MX_DEBUG_INFO("\r\nRun");
                }

            }   //if(nameBuf[0]=='r')
            //t...... - Command starting with 't'
            else if(nameBuf[0]=='t') {
                // ---------- COMMAND ----------
                //tvs - Request Status of all Transceiver
                //      Returns status of all transceivers. For each transceiver, will return command will following format:
                //      "tvsn=x", where n is transceiver ID, and x is status:
                //       - 0 = Uninitialized
                //       - 1 = OK
                if(strcmp((const char*)&nameBuf[1], "vs") == 0) {
                    cmdResponse = CMD_RESPONCE_NONE;        //This command already send a reply
                    MX_DEBUG_INFO("\r\nTcvr Stat");

                    for(iRadio=0; iRadio < RADIO_COUNT; iRadio++) {
                        InAir* pRadio               = pRadios[iRadio];
                        //RadioConfig* pRadioConfig = &radioConfig[iRadio];
                        //RadioData* pRadioData     = &radioData[iRadio];

                        txBufUsb.put("tvs");
                        txBufUsb.put('0' + iRadio);
                        if (pRadio!=0) {
                            txBufUsb.put("=1;");
                        }
                        else {
                            txBufUsb.put("=0;");
                        }
                    }
                }
                else if(strcmp((const char*)&nameBuf[1], "est") == 0) {
                    // ---------- COMMAND ----------
                    //test1 - Virtual com port speed test
                    if(trailingNameDig==1) {
                        uint8_t tempBuf[16];
                        cmdResponse = CMD_RESPONCE_NONE;        //This command already send a reply
                        MX_DEBUG_INFO("\r\nTest1");
                        strcpy((char*)tempBuf, "012345678;");
                        for (int i = 0; i<10; i++) {
                        //for (int i = 0; i<1000; i++) {
                            while(txBufUsb.getFree() < 12) {
                                mx_usbcdc_task();
                            }
                            txBufUsb.putArray(tempBuf, 10);
                            mx_usbcdc_task();
                        }
                    }
                    // ---------- COMMAND ----------
                    //test2 - Test watchdog timer
                    else if(trailingNameDig==2) {
                        cmdResponse = CMD_RESPONCE_OK;
                        while(1){};
                    }
                }
            }   //else if(nameBuf[0]=='t')
        }   // else - Command does NOT have "name=value" format

        //If dirty, mark RadioData dirty for display
        if (radioData[currCmdRadio].flags.bits.dirtyConf == true) {
            radioData[currCmdRadio].flags.bits.dirtyConfDisp = true;
        }

        //Unknown command
        if(cmdResponse==CMD_RESPONCE_UNKNOWN) {
            MX_DEBUG("\r\nUnknown command!");
            //MX_DEBUG("\r\nUnknown command %s", cmdBuf);
            txBufUsb.put("uc;");   //Unknown command
        }
        else if(cmdResponse==CMD_RESPONCE_OK) {
            #if !defined(DISABLE_RESET_RADIO_USB_TIMERS)
                tmrSecLastUsbCmd = mxTick.read_sec();   //Save last time a valid USB command processed
            #endif
            txBufUsb.put("ok;");                    //Acknowledge last received command
        }
        else if(cmdResponse==CMD_RESPONCE_NONE) {
            #if !defined(DISABLE_RESET_RADIO_USB_TIMERS)
                tmrSecLastUsbCmd = mxTick.read_sec();   //Save last time a valid USB command processed
            #endif
        }

        rxBufUsb.removeCommand();
        break;
    }   //while (rxBufUsb.hasCommand())
}
#endif  //#if ((MX_ENABLE_USB==1))

/**
 * Blink System LED with combination of long and short flashes
 * @param reset When false, blink system LED. Then true, only reset and return.
 */
void blinkLED(bool reset) {
    #define LONG_FLASH_MS       500     //Period of long flash
    #define SHORT_FLASH_MS      20      //Period of short flash
    #define LED_OFF_LONG_MS     200     //Off time after a long flash
    #define LED_OFF_SHORT_MS    250     //Off time after a short flash
    #define CYCLE_SEPARATION_MS 1500    //Time after long/short cycle till next cycle starts

    static uint8_t oldMode = 0xff;
    //Short & Long flash registers, and Reload Registers
    static uint8_t longFlash;
    static uint8_t longFlashRL;
    static uint8_t shortFlash;
    static uint8_t shortFlashRL;

    if(reset) {
        oldMode=0xff;
        tmrLED = mxTick.read_ms() + 300;    //First flash will happen in 300ms
    }

    if(oldMode != radioData[0].mode) {
        oldMode = radioData[0].mode;

        //Stopped = 2 x Long, 1 x Short flash
        if(oldMode == RADIO_MODE_STOPPED) {
            longFlash = longFlashRL = 2;
            shortFlash = shortFlashRL = 1;
        }
        //Master = 1 x Long, ? x Short flash. Where ? is address of slave we are communicating with.
        else if(oldMode == RADIO_MODE_MASTER) {
            longFlash = longFlashRL = 1;
            shortFlash = shortFlashRL = appConfig.remotelAdr;
        }
        //Slave = 0 x Long, ? x Short flash. Where ? is our slave address
        else if(oldMode == RADIO_MODE_SLAVE) {
            longFlash = longFlashRL = 0;
            shortFlash = shortFlashRL = appConfig.localAdr;
        }
        //Error = 2 x Long, (1+ErrCode) x Short flash
        else if(oldMode >= RADIO_MODE_ERR_FIRST) {
            longFlash = longFlashRL = 2;
            shortFlash = shortFlashRL = 2 + (oldMode - RADIO_MODE_ERR_FIRST);
        }
    }

//    if(reset) {
//        return;
//    }


    //Flash LED
    if (mxTick.read_ms() >= tmrLED) {

        //LED currently OFF
        if(NZ32S::get_led1() == false) {
            //Long flash
            if(longFlash != 0) {
                longFlash--;
                tmrLED = mxTick.read_ms() + LONG_FLASH_MS;
            }
            //Short flash
            else {
                shortFlash--;
                tmrLED = mxTick.read_ms() + SHORT_FLASH_MS;
            }
        }
        //LED currently ON
        else {
            //Reload
            if((shortFlash == 0) && (longFlash == 0)) {
                longFlash = longFlashRL;
                shortFlash = shortFlashRL;

                //If Master, synchronize next cycle to start when we send next PING message
                if(radioData[0].mode == RADIO_MODE_MASTER) {
                    tmrLED = tmrSendPing;
                }
                //Slave = 0 x Long, ? x Short flash. Where ? is our slave address
//                else if(radioData[0].mode == RADIO_MODE_SLAVE) {
//                }
                else {
                    tmrLED = mxTick.read_ms() + CYCLE_SEPARATION_MS;
                }
            }
            else {
                tmrLED = mxTick.read_ms() + ((longFlash==0) ? LED_OFF_SHORT_MS : LED_OFF_LONG_MS);   //LED off time
            }
        }

        NZ32S::toggle_led1();
    }
}

#if ((MX_ENABLE_USB==1))
/**
 * Process received data for USB
 */
void processRxDataUSB(uint8_t iRadio) {
    uint8_t     bufTemp[16];
    RadioData*  pRadioData = &radioData[iRadio];

    //Process received data for USB
    if (pRadioData->rxListeners & RX_LISTENER_USB) {
        MX_DEBUG_INFO("\r\nRx%d %d Bytes", iRadio, pRadioData->rxLen);

        //Copy received data to USB
        //Enough space for data plus 4 bytes: Leading "rn=" and trailing ';'
        if(txBufUsb.getFree() >= (TX_BUF_USB_COUNTERTYPE)((pRadioData->rxLen*2)+4)) {
            bufTemp[0] = 'r';
            bufTemp[1] = iRadio + '0';
            bufTemp[2] = '=';
            txBufUsb.putArray(bufTemp, 3);
            for(int i=0; i<pRadioData->rxLen; i++) {
                MxHelpers::byte_to_ascii_hex_str(pRadioData->rxBuf[i], (char*)bufTemp);
                txBufUsb.putArray(bufTemp, 2);
            }
            txBufUsb.put(';');
        }
        else {
            MX_DEBUG_INFO("\r\nRX_DONE Too little space!");
        }
        pRadioData->rxListeners &= (~RX_LISTENER_USB);  //Clear "Rx Listerner USB" flag
    }
}
#endif  //#if ((MX_ENABLE_USB==1))


/**
 * Process received data for Application
 */
void processRxDataApp(uint8_t iRadio) {
    RadioData*  pRadioData = &radioData[iRadio];

    if (pRadioData->rxListeners & RX_LISTENER_APP) {
        MX_DEBUG_INFO("\r\nRx%d %d Bytes for App", iRadio, pRadioData->rxLen);

        if(pRadioData->rxLen > 2) {
            //If Master, check for reply PONG message. Message has following format:
            // - Byte0:     Slave address
            // - Byte1-4:   "pOnG"
            // - Bytes5-6:  RSSI value slave received our "PING" message at
            if (pRadioData->mode == RADIO_MODE_MASTER) {
                //Is it addressed to us(master address is always 0)
                if (pRadioData->rxBuf[0]==0) {
                    if(memcmp(&pRadioData->rxBuf[1], pongMsg, 4) == 0) {
                        pRadioData->RssiValueSlave = ((uint16_t)pRadioData->rxBuf[5]) + (((uint16_t)pRadioData->rxBuf[6]) << 8);
                        MX_DEBUG("\r\nRXed Pong - RSSI=%d, %d", pRadioData->RssiValue, pRadioData->RssiValueSlave);
                        appData.rxCountPingPong++;  //Increment valid ping-pong message count
                    }
                }
                else {
                    MX_DEBUG_INFO("\r\nMsg not addressed to master!");
                }
            }
            //If slave, check for PING message, and if received, send PONG reply
            // - Byte0:     Slave address
            // - Byte1-4:   "pInG"
            else if (pRadioData->mode == RADIO_MODE_SLAVE) {
                //Is it from current remote slave
                if (pRadioData->rxBuf[0]==appConfig.localAdr) {
                    if(memcmp(&pRadioData->rxBuf[1], pingMsg, 4) == 0) {
                        MX_DEBUG("\r\nRXed Ping - RSSI=%d", pRadioData->RssiValue);
                        appData.rxCountPingPong++;  //Increment valid ping-pong message count
                        //Send PONG message. Message has following format
                        // - Byte0:     0(Master address is always 0)
                        // - Byte1-4:   "pOnG"
                        // - Bytes5-6:  RSSI value slave received "PING" message at
                        pRadioData->txBuf.put(0);
                        pRadioData->txBuf.putArray((unsigned char*)pongMsg, 4);
                        pRadioData->txBuf.put((uint8_t)pRadioData->RssiValue);      //Put LSB of RSSI
                        pRadioData->txBuf.put((uint8_t)(pRadioData->RssiValue>>8)); //Put MSB of RSSI
                    }
                }
                else {
                    MX_DEBUG_INFO("\r\nMsg not addressed to us!");
                }
            }
        }
        pRadioData->rxListeners &= (~RX_LISTENER_APP);  //Clear "Rx Listerner App" flag
    }
}

/**
 * High level radio task.
 *
 * === Transmit ===
 * If any data present in radio txBuf (radioData[].txBuf), it will be transmitted
 *
 * === Receive ===
 * If data received, it is copied to radio rxBuf (radioData[].rxBuf), and rxLen will
 * be updated with length of received message. The application has to process this data, and
 * clear rxLen when done. The receive process will ONLY copy new received data to the buffer
 * if rxLen = 0! If not, received message will be lost!
 */
void radioTask(uint8_t iRadio) {
    InAir* pRadio               = pRadios[iRadio];
    RadioConfig* pRadioConfig = &radioConfig[iRadio];
    RadioData* pRadioData     = &radioData[iRadio];

    ///////////////////////////////////////////////////////////////////
    // Main Radio Task ////////////////////////////////////////////////
    switch (pRadioData->smRadio) {
    //Idle, and not currently waiting for a transmission to finish.
    //This state will check if there is anything in buffer to send.
    case IDLE:
        if (mxTick.read_ms() >= pRadioData->tmrRadio) {
            pRadioData->tmrRadio = mxTick.read_ms();    //Always update tick, else it will expire after a while!

            //Check if there is data to send
            if (pRadioData->txBuf.isEmpty() == false) {
                uint8_t tx[32];
                int i;
                MX_DEBUG_INFO("\r\nTx%d", iRadio);
                int txSize = pRadioData->txBuf.getAvailable();  // TODO - Debug shows txSize of 0 returned!!!!!!!!!!
                for(i=0; i<txSize; i++) {
                    tx[i] = pRadioData->txBuf.get();
                }

                if ((pRadioData->mode!=RADIO_MODE_STOPPED) && (pRadio!=NULL) ) {
                    pRadio->Send(tx, txSize);
                }
                else {
                    MX_DEBUG_INFO("\r\n NOT RUNNING!");
                }
                pRadioData->smRadio = LOWPOWER; //Wait for TX done callback in lowpower mode
            }
        }
        break;
    //CPU Low power state(can put CPU to sleep here), Waiting for a response from Radio(Tx or Rx Done, Timeout...).
    //This state will NOT check if there is any new data to send! Will only leave this state once a radio interrupt occurs.
    //Use IDLE state to enter receive mode, AND check if there is any new data to sent(will leave RX mode and send data)
    case LOWPOWER:
        break;
    //We just got a "transmission finished" notification from the radio chip
    case TX_DONE:
        #if !defined(DISABLE_RESET_RADIO_USB_TIMERS)
            tmrSecLastTxRx = mxTick.read_sec(); //Save last time Radio did a successful transmission
        #endif
        MX_DEBUG_INFO("\r\n> Tx Done");

        //Send TX OK reply
        #if ((MX_ENABLE_USB==1))
        txBufUsb.put('t');
        txBufUsb.put('0' + iRadio);
        txBufUsb.put("=tok;");   //TX OK
        #endif

        //TX mode 0: Go to Idle mode after transmission
        if (pRadioConfig->txMode == TX_MODE_IDLE_AFTER_TXION) {
            pRadioData->smRadio = IDLE;
        }
        //TX mode 1: Go to Receive mode after transmission
        else if (pRadioConfig->txMode == TX_MODE_RX_AFTER_TXION) {
            MX_DEBUG_INFO("\r\n> Rx(%d)", pRadioConfig->rxTimeout);
            pRadioData->smRadio = IDLE;
            pRadio->Rx(pRadioConfig->rxTimeout);    //Continues reception mode
        }
        break;
    case TX_TIMEOUT:
        MX_DEBUG_INFO("\r\nTx Timeout");

        //Send TX Timeout reply
        #if ((MX_ENABLE_USB==1))
        txBufUsb.put('t');
        txBufUsb.put('0' + iRadio);
        txBufUsb.put("=tto;");
        #endif

        pRadioData->smRadio = IDLE; //Return to IDLE mode, check if any new data available to send
        break;
    //We just got a "reception finished" notification from the radio chip
    case RX_DONE:
        #if !defined(DISABLE_RESET_RADIO_USB_TIMERS)
            tmrSecLastTxRx = mxTick.read_sec();     //Save last time Radio had a successful reception
        #endif
        //Set all "RX Listener" flags
        pRadioData->rxListeners |= RX_LISTENER_MASK;
        pRadioData->rxCount++;      //Increment RX Count
        pRadioData->smRadio = IDLE; //Return to IDLE mode, check if any new data available to send
        break;
    case RX_TIMEOUT:
        pRadioData->rxStatus = RX_STATUS_TIMEOUT;
        pRadioData->rxErrCnt++;     //Increment for Timeouts and CRC errors
        MX_DEBUG_INFO("\r\nRx%d Timeout", iRadio);
        //Send RX Timeout reply
        #if ((MX_ENABLE_USB==1))
        txBufUsb.put('r');
        txBufUsb.put('0' + iRadio);
        txBufUsb.put("=rto;");
        #endif

        pRadioData->smRadio = IDLE; //Return to IDLE mode, check if any new data available to send
        break;
    case RX_ERROR:
        pRadioData->rxStatus = RX_STATUS_ERR_CRC;
        pRadioData->rxErrCnt++;     //Increment for Timeouts and CRC errors
        MX_DEBUG_INFO("\r\nRx%d Error", iRadio);
        //Send RX Error reply
        #if ((MX_ENABLE_USB==1))
        txBufUsb.put('r');
        txBufUsb.put('0' + iRadio);
        txBufUsb.put("=rer;");   //TX OK
        #endif

        pRadioData->smRadio = IDLE; //Return to IDLE mode, check if any new data available to send
        break;
    default:
        pRadioData->smRadio = IDLE;
        break;
    }  //switch (pRadioData->smRadio)
}

/**
 * Initialize given radio.
 * @return True if redio was successfully initialized, else false.
 */
bool initializeRadio(uint8_t iRadio) {
    InAir* pRadio               = pRadios[iRadio];
    RadioConfig* pRadioConfig = &radioConfig[iRadio];
    RadioData* pRadioData     = &radioData[iRadio];

    if (mxTick.read_ms() >= pRadioData->tmrRadio) {
        pRadioData->tmrRadio = mxTick.read_ms();    //Always update tick, else it will expire after a while!

        //Create and Initialize instance of SX1276inAir
        if (pRadio == NULL) {
            if(iRadio==0) {
                MX_DEBUG_INFO("\r\nCreating Radio%d", iRadio);
                pRadio = new InAir(OnTxDone0, OnTxTimeout0, OnRxDone0,  OnRxTimeout0, OnRxError0,
                        NULL/*FHSS Change*/, NULL/*CAD Done*/,
                        PB_5/*MOSI*/, PB_4/*MISO*/, PB_3/*SCLK*/, PC_8/*CS*/, PA_9/*RST*/,
                        PB_0/*DIO0*/, PB_1/*DIO1*/, PC_6/*DIO2*/, PA_10/*DIO3*/);
                pRadio->SetBoardType(pRadioConfig->boardType);
                pRadios[iRadio] = pRadio;
            }
            //ENABLE IF MULTIPLE RADIOS
            //else if(iRadio==1) {
            //    MX_DEBUG_INFO("\r\nCreating tr%d", iRadio);
            //    pRadio = new InAir(OnTxDone1, OnTxTimeout1, OnRxDone1,  OnRxTimeout1, OnRxError1,
            //            NULL/*FHSS Change*/, NULL/*CAD Done*/,
            //            PB_5/*MOSI*/, PB_4/*MISO*/, PB_3/*SCLK*/, PA_1/*CS*/, PA_2/*RST*/,
            //            PA_0/*DIO0*/, PC_0/*DIO1*/, PA_5/*DIO2*/, PA_3/*DIO3*/);
            //    pRadio->SetBoardType(pRadioConfig->boardType);
            //    pRadios[iRadio] = pRadio;
            //}
            //else if(iRadio==2) {
            //    MX_DEBUG_INFO("\r\nCreating tr%d", iRadio);
            //    pRadio = new InAir(OnTxDone2, OnTxTimeout2, OnRxDone2,  OnRxTimeout2, OnRxError2,
            //            NULL/*FHSS Change*/, NULL/*CAD Done*/,
            //            PB_5/*MOSI*/, PB_4/*MISO*/, PB_3/*SCLK*/, PA_8/*CS*/, PB_6/*RST*/,
            //            PA_6/*DIO0*/, PA_7/*DIO1*/, PA_4/*DIO2*/, PB_7/*DIO3*/);
            //    pRadio->SetBoardType(pRadioConfig->boardType);
            //    pRadios[iRadio] = pRadio;
            //}
            //Wait 100mS before executing radio functions again
            pRadioData->tmrRadio = mxTick.read_ms() + 100;
            return false;   //Radio NOT initialized, try again later
        }

        //Check if radio is present
        pRadioData->flags.bits.noRadio = false;
        if (pRadio->Read(REG_VERSION) == 0x00) {
            pRadioData->flags.bits.noRadio = true;
            MX_DEBUG_INFO("\r\nNo Radio%d detected!", iRadio);

            #if ((MX_ENABLE_USB==1))
            if (pRadioData->flags.bits.noRadioMsgUSB == 0) {
                pRadioData->flags.bits.noRadioMsgUSB = 1;
                txBufUsb.put("NoRadio\r");   //No Radio
                txBufUsb.put('0' + iRadio);
                txBufUsb.put("\r");
            }
            #endif

            //If radio was already initialized, delete it.
            //delete pRadio;
            //pRadio = NULL;

            //Test for radio again in 500mS
            pRadioData->tmrRadio = mxTick.read_ms() + 500;
            return false;   //Radio NOT initialized, try again later
        }

        //If no display, automatically put in master mode
        #if defined(DISABLE_OLED)
        pRadioData->mode = RADIO_MODE_MASTER;
        #endif

        //Set Channel Frequency
        pRadio->SetChannel(pRadioConfig->frequency);

        if(pRadioConfig->conf.lora.fshhEnable == true) {
            MX_DEBUG("\r\n%d=LORA FHSS Mode", iRadio);
        }
        else {
            MX_DEBUG("\r\n%d=LORA Mode", iRadio);
        }

        pRadio->SetTxConfig(MODEM_LORA, pRadioConfig->power, 0, pRadioConfig->bw,
                pRadioConfig->sf, pRadioConfig->conf.lora.codingRate,
                pRadioConfig->preambleLength, pRadioConfig->conf.lora.fixLength,
                pRadioConfig->conf.lora.crcEnable, pRadioConfig->conf.lora.fshhEnable, pRadioConfig->numberSymHop,
                pRadioConfig->conf.lora.iqInversionEnable, 2000000);

        //Configure receiver as LoRa, with given values:
        // - Payload Length = 0
        pRadio->SetRxConfig(MODEM_LORA, pRadioConfig->bw,
                pRadioConfig->sf, pRadioConfig->conf.lora.codingRate, 0,
                pRadioConfig->preambleLength, pRadioConfig->symbolTimeout,
                pRadioConfig->conf.lora.fixLength, 0, pRadioConfig->conf.lora.crcEnable,
                pRadioConfig->conf.lora.fshhEnable, pRadioConfig->numberSymHop,
                pRadioConfig->conf.lora.iqInversionEnable, pRadioConfig->rxMode==0?0:1);

        pRadioData->smRadio = IDLE;
        return true;    //Radio Initialized!
    }   //if (mxTick.read_ms() >= pRadioData->tmrRadio)

    return false;   //Radio NOT initialized, try again later
}

/**
 * Called when the current mode is changed.
 * @param newMode New mode, is a RADIO_MODE_xxx define
 * @param iRadio Index of radio, a value from 0 - RADIO_COUNT
 */
void setRadioMode(uint8_t newMode, uint8_t iRadio) {
    radioData[iRadio].mode = newMode;

    //Settings that are the same for Master and Slave
    if ((newMode==RADIO_MODE_MASTER) || (newMode==RADIO_MODE_SLAVE)) {
        if(radioConfig[iRadio].boardType == BOARD_INAIR9B) {
            radioConfig[iRadio].power = TX_OUTPUT_POWER_BOOST;
        }
        else {
            radioConfig[iRadio].power = TX_OUTPUT_POWER;
        }
        radioConfig[iRadio].preambleLength = LORA_PREAMBLE_LENGTH;
        radioConfig[iRadio].symbolTimeout = LORA_SYMBOL_TIMEOUT;
        radioConfig[iRadio].numberSymHop = LORA_NB_SYMB_HOP;
        radioConfig[iRadio].rxTimeout = RX_TIMEOUT_VALUE;
        radioConfig[iRadio].rxMode = RX_MODE_CONTINUOUS;        //Continues receive mode, Radio stays in receive mode after receiving a message
        radioConfig[iRadio].conf.lora.codingRate = LORA_CODINGRATE;
        radioConfig[iRadio].conf.lora.fixLength = LORA_FIX_LENGTH_PAYLOAD_ON;
        radioConfig[iRadio].conf.lora.fshhEnable = LORA_FHSS_ENABLED;
        radioConfig[iRadio].conf.lora.iqInversionEnable = LORA_IQ_INVERSION_ON;
        radioConfig[iRadio].conf.lora.crcEnable = LORA_CRC_ENABLED;

        appData.rxCountPingPong = 0;
        appData.rxErrCountPingPong = 0;
        appData.oldRxCountPingPong = 0xff;  //Ensure NOT equal to rxCountPingPong.
    }


    switch (newMode) {
    case RADIO_MODE_STOPPED:
        break;
    case RADIO_MODE_MASTER:
        radioConfig[iRadio].txMode = TX_MODE_RX_AFTER_TXION;    //Transmit Mode - Radio switches to Receive after TXion
        radioConfig[iRadio].rxTimeout = 0;                      //No Timeout. After transmit, we switch to receive mode with no timeout
        break;
    case RADIO_MODE_SLAVE:
        radioConfig[iRadio].txMode = TX_MODE_RX_AFTER_TXION;    //Transmit Mode - Radio switches to Receive after TXion
        //radioConfig[iRadio].txMode = TX_MODE_IDLE_AFTER_TXION;  //Transmit Mode - Radio switches to Idle after TXion
        radioConfig[iRadio].rxTimeout = 0;                      //No Timeout. After transmit, we switch to receive mode with no timeout
        break;
    }

    radioData[iRadio].flags.bits.dirtyConf = true;
}
