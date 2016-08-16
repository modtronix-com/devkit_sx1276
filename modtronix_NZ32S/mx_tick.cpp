/**
 * File:      mx_timer.cpp
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
#include "mx_tick.h"

uint16_t    MxTick::countMs     = 1000;
int32_t     MxTick::tickSec     = 0;
int32_t     MxTick::tickMs      = 0;
bool        MxTick::running     = 0;

MxTick::MxTick(bool autoInc) {
    if(running==false) {
        running = true;
        if(autoInc == true) {
            static Ticker tcr;
            tcr.attach_us(&MxTick::increment, 1000);
        }
    }
}

