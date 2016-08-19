/**
 * File:      modtronix_config.h
 *
 * Description:
 * This file provides configuration defines for Modtronix Libraries.
 */
#ifndef _MODTRONIX_CONFIG_H_
#define _MODTRONIX_CONFIG_H_



/////////////////////////////////////////////////
//////////////// Debugging //////////////////////

//Disable all debugging! Uncomment this to build firmware for release
//#define MX_DEBUG_DISABLE



/////////////////////////////////////////////////
//////////// modtronix_im4OLED //////////////////

// IM4OLED is integrated on PT01NZ board
#define IM4OLED_VIA_PT01NZ      1

// Uncomment this to turn off the builtin splash
#define OLED_HAS_SPLASH         0

//Don't use std::vector. This saves a lot of Flash space. Also, std::vector uses heap which is not always desirable.
//When NOT using std::vector, OLED_HEIGHT and OLED_WIDTH are required.
#define OLED_USE_VECTOR         0

// Used when std::vector not used. In this case, buffer has to be assigned statically.
#define OLED_HEIGHT             64
#define OLED_WIDTH              128

// Uncomment this to enable all functionality
#define GFX_ENABLE_ABSTRACTS    0

// Uncomment this to enable only runtime font scaling, without all the rest of the Abstracts
#define GFX_SIZEABLE_TEXT       1



/////////////////////////////////////////////////
////////////// modtronix_inAir //////////////////
#if !defined(INAIR_DIO0_IS_INTERRUPT)
#define INAIR_DIO0_IS_INTERRUPT     0
#endif

#if !defined(INAIR_DIO1_IS_INTERRUPT)
#define INAIR_DIO1_IS_INTERRUPT     0
#endif

#if !defined(INAIR_DIO2_IS_INTERRUPT)
#define INAIR_DIO2_IS_INTERRUPT     0
#endif

#if !defined(INAIR_DIO3_IS_INTERRUPT)
#define INAIR_DIO3_IS_INTERRUPT     0
#endif



/////////////////////////////////////////////////
////////////// modtronix_NZ32S //////////////////

//Set to 0 to disable A13 and A14 from being used(are SWD serial program/debug ping). Must be done if
//debugging is going to be used. These pins are used by ST-Link for programming debugging, but also used
//for battery level monitoring, and enabling fast battery charge. Setting this define to 0 will disable these
//functions, but will enable programming and debugging via SWD (ST-Link)
#define     NZ32S_USE_A13_A14    1



#endif
