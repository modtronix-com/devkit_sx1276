/**
 * File:      mx_usb_cdc.cpp
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

//Enable debugging in include files if required - Include AFTER debug defines/includes above! It uses them for debugging!
//#define DEBUG_ENABLE_MX_CMD_BUFFER          1
//#define DEBUG_ENABLE_INFO_MX_CMD_BUFFER     1
#include "app_defs.h"    //Application defines, must be first include after debugging includes/defines
#include "mx_usb_cdc.h"
#include "mx_cmd_buffer.h"
#include "usb_device.h"
#include "usbd_cdc_if.h"


// Defines ////////////////////////////////////////////////////////////////////
#define TEMP_BUF_SIZE   TX_BUF_USB_SIZE


// VARIABLES //////////////////////////////////////////////////////////////////
uint8_t tempBuf[TEMP_BUF_SIZE];     //Temporary buffer



// GLOBAL VARIABLES ///////////////////////////////////////////////////////////
MxCmdBuffer <RX_BUF_USB_SIZE, RX_BUF_USB_COMMANDS, RX_BUF_USB_COUNTERTYPE> rxBufUsb;
MxCmdBuffer <TX_BUF_USB_SIZE, TX_BUF_USB_COMMANDS, TX_BUF_USB_COUNTERTYPE> txBufUsb;


/** Initialization
 */
void mx_usbcdc_init(void) {
    MX_DEBUG("\r\nHello from mx_usbcdc_init");

    //Required for USB on the NZ32-SC151 without 1k5 pull-up resistor on USB D+.
    __SYSCFG_CLK_ENABLE();

    //USB Init
    MX_USB_DEVICE_Init();
}


void mx_usbcdc_receive(uint8_t* Buf, uint32_t *Len) {
    uint16_t i;

    for (i = 0; i < *Len; i++) {
        rxBufUsb.put(Buf[i]);
    }
}



/**
 * Task, call from while loop
 */
void mx_usbcdc_task() {
    static int lenRead;
    static uint8_t usbTransmitAttempts = 0;
    int cmdLen;

    if(usbTransmitAttempts == 0) {
        if (txBufUsb.hasCommand()) {
            cmdLen = txBufUsb.getCommandLength();

            //Get current command in buffer. Is NOT removed!
            lenRead = txBufUsb.peekArray(tempBuf, TEMP_BUF_SIZE, cmdLen);

            //Check if space to add ';' terminator
            if(lenRead >= TEMP_BUF_SIZE) {
                MX_DEBUG("\r\nmx_usbcdc_task() ERR1");
                //Remove command from buffer - too big for buffer. Can not be sent
                txBufUsb.removeCommand();
                return;
            }

            //Add ';' command termination
            //tempBuf[lenRead++] = ';';
            tempBuf[lenRead++] = '\r';

            usbTransmitAttempts = 100;
        }
    }

    if(usbTransmitAttempts != 0) {

        //Try to transmit
        if (CDC_Transmit_FS(tempBuf, lenRead) != USBD_OK) {
            //Error trying to transmit

            //If still more attempts left, try again later
            if (--usbTransmitAttempts!=0) {
                return; //Return, and try again later
            }
            else {
                MX_DEBUG("\r\nCDC_Transmit_FS() ERR");
            }
        }

        usbTransmitAttempts = 0;

        //Remove command from buffer
        txBufUsb.removeCommand();
    }


//    //Check if command in buffer
//    if (rxBufUsb.hasCommand()) {
//        uint8_t c;
//        int i;
//        uint16_t len = rxBufUsb.getCommandLength();
//        MX_DEBUG("\r\nR Cmd(%d)=", len);
//
//        for(int i=0; i<len; i++) {
//            MX_DEBUG("%c", rxBufUsb.peekAt(i));
//        }
//
//        //if ()
//
//        if(rxBufUsb.peek()=='t') {
//
//        }
//        else if((len>3) && (rxBufUsb.peek()=='b') && (rxBufUsb.peekAt(1)=='w')) {
//
//        }
//        else if(rxBufUsb.peek()=='f') {
//
//        }
//        else if(rxBufUsb.peek()=='s') {
//
//        }
//
//
//
//        rxBufUsb.removeCommand();
//    }

//    static uint8_t oldCmdCount=0;
//    //Check if command in buffer
//    if (rxBufUsb.hasCommand()) {
//        if (oldCmdCount != rxBufUsb.getCommandCount()) {
//            oldCmdCount = rxBufUsb.getCommandCount();
//            MX_DEBUG("\r\nCmdCount=%d", rxBufUsb.getCommandCount());
//        }
//    }
}

