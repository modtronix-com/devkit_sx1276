/**
 * File:      mx_buffer_base.h
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
#ifndef SRC_MX_BUFFER_BASE_H_
#define SRC_MX_BUFFER_BASE_H_


class MxBuffer {

public:
    //static const uint16_t VALUE_OFFSET_UNKNOWN = 0xffff;    /* Offset of 'value' part of "name=value" command unknown */
    static const uint16_t VALUE_OFFSET_UNKNOWN = -1;
};



#endif /* SRC_MX_BUFFER_BASE_H_ */
