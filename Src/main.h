/**
 * File:      main.h
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
#ifndef __MAIN_H__
#define __MAIN_H__

#include "mbed.h"

#if defined(THIS_IS_MAIN_CPP)
#define DEBUG_ENABLE_MAIN       0
#define DEBUG_ENABLE_INFO_MAIN  0
Serial streamDebug(USBTX, USBRX);   //Default is UART3
Stream* pMxDebug = &streamDebug;    //DON'T EDIT!! This pointer is used by external debug code to write to debug stream
#include "mx_default_debug.h"

#include "app_defs.h"            //Application defines, must be first include after debugging includes/defines(in main.cpp)
#include "app_helpers.h"
#include "mx_config.h"
#include "nz32s.h"
#include "mx_circular_buffer.h"
#include "mx_usb_cdc.h"
#include "mx_ssd1306.h"
#include "app_display.h"
#include "inair.h"
#include "im4oled.h"
#endif  //#if defined(THIS_IS_MAIN_CPP)


/*
 * Callback functions prototypes
 */

/*!
 * @brief Function to be executed on Radio Tx Done event
 */
void OnTxDone(uint8_t radioID);

/*!
 * @brief Function to be executed on Radio Rx Done event
 */
void OnRxDone(uint8_t radioID, uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr);

/*!
 * @brief Function executed on Radio Tx Timeout event
 */
void OnTxTimeout(uint8_t radioID);

/*!
 * @brief Function executed on Radio Rx Timeout event
 */
void OnRxTimeout(uint8_t radioID);

/*!
 * @brief Function executed on Radio Rx Error event
 */
void OnRxError(uint8_t radioID);

/*!
 * @brief Function executed on Radio Fhss Change Channel event
 */
void OnFhssChangeChannel(uint8_t radioID, uint8_t channelIndex );

/*!
 * @brief Function executed on CAD Done event
 */
void OnCadDone(uint8_t radioID, bool channelActivityDetected );

static inline void OnTxDone0(void) {OnTxDone(0);}
static inline void OnRxDone0(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {OnRxDone(0, payload, size, rssi, snr);}
static inline void OnTxTimeout0(void) {OnTxTimeout(0);}
static inline void OnRxTimeout0(void) {OnRxTimeout(0);}
static inline void OnRxError0(void) {OnRxError(0);}

static inline void OnTxDone1(void) {OnTxDone(1);}
static inline void OnRxDone1(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {OnRxDone(1, payload, size, rssi, snr);}
static inline void OnTxTimeout1(void) {OnTxTimeout(1);}
static inline void OnRxTimeout1(void) {OnRxTimeout(1);}
static inline void OnRxError1(void) {OnRxError(1);}

static inline void OnTxDone2(void) {OnTxDone(2);}
static inline void OnRxDone2(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {OnRxDone(2, payload, size, rssi, snr);}
static inline void OnTxTimeout2(void) {OnTxTimeout(2);}
static inline void OnRxTimeout2(void) {OnRxTimeout(2);}
static inline void OnRxError2(void) {OnRxError(2);}


#endif // __MAIN_H__
