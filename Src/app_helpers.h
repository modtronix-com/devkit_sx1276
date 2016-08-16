/**
 * File:      app_display.h
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
#ifndef APP_HELPERS_H_
#define APP_HELPERS_H_

#include "app_defs.h"            //Application defines, must be first include after debugging includes/defines(in main.cpp)

/**
 * Restore default values for given RadioConfig structure
 */
void restoreRadioConfigDefaults(RadioConfig* pRadioConfig);

/**
 * Load AppConfig from EEPROM
 * @return True if OK, else false
 */
bool loadAppConfig(uint8_t id, AppConfig* appConf);

/**
 * Save AppConfig
 * @return True if OK, else false
 */
bool saveAppConfig(AppConfig* appConf);


/**
 * Load RadioConfig from EEPROM
 * @return True if OK, else false
 */
bool loadRadioConfig(uint8_t id, RadioConfig* radioConf);

/**
 * Save RadioConfig
 * @return True if OK, else false
 */
bool saveRadioConfig(uint8_t id, RadioConfig* radioConf);


uint16_t decodeAsciiCmd(uint8_t* pDst, uint16_t destSize, const uint8_t* pSrc, uint8_t escChar = 0);


#endif /* APP_HELPERS_H_ */
