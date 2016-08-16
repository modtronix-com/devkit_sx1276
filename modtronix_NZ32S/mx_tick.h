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
#ifndef MODTRONIX_NZ32S_MX_TICK_H_
#define MODTRONIX_NZ32S_MX_TICK_H_

#define MODTRONIX_NZ32S_MX_TICK_INC MxTick::increment()

/** A general purpose milli-second and second timer.
 *
 * Because the read_ms() function return a 32 bit value, it can be used to time up
 * to a maximum of 2^31-1 millseconds = 596 Hours = 24.8 Days
 *
 * Example:
 * @code
 * // Count the time to toggle a LED
 *
 * #include "mbed.h"
 * #include "mx_tick.h"
 *
 * MxTimer mxTmr;
 * int tmrLED = 0;
 * DigitalOut led(LED1);
 *
int main() {

    //Main loop
    while(1) {
        //Flash LED
        if (mxTmr.read_ms() >= tmrLED) {
            tmrLED += 1000;         //Wait 1000mS before next LED toggle
            led = !led;
        }
    }
}
 * @endcode
 */
class MxTick {

public:
    MxTick(bool autoInc = true);

    static inline void increment() {
        tickMs++;
        if (--countMs == 0) {
            countMs = 1000;
            tickSec++;
        }
    }


    /** Get the current 32-bit milli-second tick value.
     * Because the read_ms() function return a 32 bit value, it can be used to time up
     * to a maximum of 2^31-1 millseconds = 596 Hours = 24.8 Days
     *
     * This function is thread save!
     *
     * @return The tick value in milli-seconds
     */
    static inline int read_ms() {
        return (int)tickMs;
    }

    /** Get the current 32-bit "10 milli-second" tick value. It is incremented each 10ms.
     * Because the read_ms() function return a 32 bit value, it can be used to time up
     * to a maximum of 2^31-1 * 10 millseconds = 5960 Hours = 248 Days
     *
     * This function is NOT thread save. Takes multiple cycles to calculate value.
     *
     * Note this function takes MUCH longer than read_ms() and read_sec().
     *
     * @return The tick value in 10 x milli-seconds
     */
    static int read_ms10() {
        //For example, if tickMs was 456789, we only want to use the 78 part.
        //The 456 is seconds, and we get that from tickSec, so don't use it
        //The 9 is 1xms, and don't need that
        // 456789 / 10 = 45678
        // 45678 % 100 = 89
        return (int)( ((tickMs/10)%100) + (tickSec*100) );
    }

    /** Get the current 32-bit "100 milli-second" tick value. It is incremented each 100ms.
     * Because the read_ms() function return a 32 bit value, it can be used to time up
     * to a maximum of 2^31-1 * 100 millseconds = 59600 Hours = 2480 Days = 6.79 Years
     *
     * This function is NOT thread save. Takes multiple cycles to calculate value.
     *
     * Note this function takes MUCH longer than read_ms() and read_sec().
     *
     * @return The tick value in 100 x milli-seconds
     */
    static int read_ms100() {
        //For example, if tickMs was 456789, we only want to use the 7 part(100 x Ms).
        //The 456 is seconds, and we get that from tickSec, so don't use it
        //The 8 is 1xMs, the 7 is 10xMs, and we don't need them
        // 456789 / 100 = 4567
        // 45678 % 10 = 7
        return (int)( ((tickMs/100)%10) + (tickSec*10) );
    }

    /** Get the current 32-bit second tick value.
     * Because the read_sec() function return a 32 bit value, it can be used to time up
     * to a maximum of 2^31-1 seconds = 596,523 Hours = 24855 Days = 68 Years
     *
     * This function is thread save!
     *
     * @return The tick value in seconds
     */
    static inline int read_sec() {
        return (int)tickSec;
    }

protected:
    static uint16_t countMs;
    static int32_t  tickSec;
    static int32_t  tickMs;
    static bool     running;
};



#endif /* MODTRONIX_NZ32S_MX_TICK_H_ */
