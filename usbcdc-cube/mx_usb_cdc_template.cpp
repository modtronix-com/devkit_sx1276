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
#include "mx_usb_cdc.h"
#include "mx_circular_buffer.h"

// USB Includes
#include "usb_device.h"
#include "usbd_cdc_if.h"

//Default implementation. Echos back everything received on CDC. To create custom implementation:
// - Copy this file to src folder, and name it "mx_usb_cdc.cpp" for example
// - Uncomment following line
//#define USE_CUSTOM_MX_USB_CDC

#if defined(USE_CUSTOM_MX_USB_CDC)

#define DEBUG_ENABLE            1
#include "mx_default_debug.h"


// Defines ////////////////////////////////////////////////////////////////////
#define RX_BUF_SIZE  256


// GLOBAL VARIABLES ///////////////////////////////////////////////////////////
MxAsciiCmdBuffer <uint8_t, RX_BUF_SIZE> rxBuf;


// External GLOBAL VARIABLES //////////////////////////////////////////////////
extern DigitalOut led;



/** Initialization
 */
void mx_usbcdc_init(void) {
    //MX_DEBUG("\r\nHello from mx_usbcdc_init");

    //USB Init
    MX_USB_DEVICE_Init();
}


void mx_usbcdc_receive(uint8_t* Buf, uint32_t *Len) {
    uint16_t i;

    for (i = 0; i < *Len; i++) {
        rxBuf.put(Buf[i]);
    }
}

/**
 * Task, call from while loop
 */
void mx_usbcdc_task() {
    uint8_t c;
    while (!rxBuf.isEmpty()) {
        c = rxBuf.get();
        if (CDC_Transmit_FS(&c, 1) == USBD_OK) {
            led = !led;
        } else {
            break;
        }
    }
}

#endif  //#if (USE_CUSTOM_MX_USB_CDC == 0)
