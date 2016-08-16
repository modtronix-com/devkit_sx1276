/**
 * File:      mx_default_debug.h
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
#ifndef SRC_MX_DEFAULT_DEBUG_H_
#define SRC_MX_DEFAULT_DEBUG_H_

//This file is used for default debugging. Standard way to use it is:
//
//
//===================================================================
//========= Use in c or cpp file (NOT main.c or main.cpp) ===========
//
//----- Add following code to top of file debugging is required -----
//#define DEBUG_ENABLE            1
//#include "mx_default_debug.h"
//
//-------------- Use MX_DEBUG function to write debug ---------------
//MX_DEBUG("\r\nTest Debug message");
//
//
//
//===================================================================
//=================== Use in main.c or main.cpp =====================
//
//--------------- Add following code to top of main.c ---------------
//#define DEBUG_ENABLE_MAIN       1
//Serial streamDebug(USBTX, USBRX);               //Default is UART3
//Stream* pMxDebug = &streamDebug;                //DON'T EDIT!! This pointer is used by external debug code to write to debug stream
//#include "mx_default_debug.h"
//
//------------- Add following code to bottom of main.c --------------
//void mxDebug(const char *format, ...) {
//#define STRING_STACK_LIMIT 100
//
//    //Process argument list, and write out somewhere
//    va_list arg;
//    va_start(arg, format);
//    int len = vsnprintf(NULL, 0, format, arg);
//    if (len < STRING_STACK_LIMIT) {
//        char temp[STRING_STACK_LIMIT];
//        vsprintf(temp, format, arg);
//        streamDebug.puts(temp);
//    } else {
//        char *temp = new char[len + 1];
//        vsprintf(temp, format, arg);
//        streamDebug.puts(temp);
//        delete[] temp;
//    }
//    va_end(arg);
//
//    //Send to standard stream
//    //va_list args;
//    //va_start(args, format);
//    //vfprintf(stderr, format, args);
//    //va_end(args);
//}


#ifndef WEAK
    #if defined (__ICCARM__)
        #define WEAK     __weak
    #else
        #define WEAK     __attribute__((weak))
    #endif
#endif

#if !defined(MX_DEBUG_IS_POINTER)
#define MX_DEBUG_IS_POINTER 1
#endif

#if !defined(DEBUG_ENABLE)
#define DEBUG_ENABLE 0
#endif

#if !defined(DEBUG_ENABLE_INFO)
#define DEBUG_ENABLE_INFO 0
#endif

#if (DEBUG_ENABLE==0) && (DEBUG_ENABLE_INFO==1)
#error "Can not have DEBUG_ENABLE=0 and DEBUG_ENABLE_INFO=1!"
#endif


///////////////////////////////////////////////////////////////////////////////
// Following is for main file - file that has mxDebug() defined in it
#if defined(DEBUG_ENABLE_MAIN)
    #if (DEBUG_ENABLE_MAIN==1) && !defined(MX_DEBUG_DISABLE)
        #define MX_DEBUG  pMxDebug->printf
        //#define MX_DEBUG(format, args...) ((void)0) //DISABLE Debugging
    #else
        //GCC's CPP has extensions; it allows for macros with a variable number of arguments. We use this extension here to preprocess pmesg away.
        #define MX_DEBUG(format, args...) ((void)0)
    #endif

    #if (DEBUG_ENABLE_MAIN==1) && (DEBUG_ENABLE_INFO_MAIN==1) && !defined(MX_DEBUG_DISABLE)
        #define MX_DEBUG_INFO  pMxDebug->printf
        //#define MX_DEBUG(format, args...) ((void)0) //DISABLE Debugging
    #else
        //GCC's CPP has extensions; it allows for macros with a variable number of arguments. We use this extension here to preprocess pmesg away.
        #define MX_DEBUG_INFO(format, args...) ((void)0)
    #endif

///////////////////////////////////////////////////////////////////////////////
// Following is for any file that uses debugging (except main file that has mxDebug() defined in it)
#else   //#if defined(DEBUG_ENABLE_MAIN)

//IMPORTANT, when enabling debugging, it is very important to note the following:
//- If (MX_DEBUG_IS_POINTER==1), ensure there is global Stream pointer defined somewhere in code to override "pMxDebug = NULL" below!
//- If (MX_DEBUG_IS_POINTER==0), define a mxDebug() function somewhere in code to handle debug output

    #if (DEBUG_ENABLE==1) && !defined(MX_DEBUG_DISABLE)
    //Alternative method in stead of using NULL below. This requires to create derived Stream class in each file we want to use debugging
    //    class modtronixDebugStream : public Stream {int _putc(int value) {return 0;}int _getc() {return 0;}};
    //    static modtronixDebugStream modtronixDebug;
    //    WEAK Stream* pMxDebug = &modtronixDebug;
        #if (MX_DEBUG_IS_POINTER==1)        //More efficient, but only works if pMxDebug is defined in code. Disabled by default!
            WEAK Stream* pMxDebug = NULL;
            #define MX_DEBUG  pMxDebug->printf
        #else
            WEAK void mxDebug(const char *format, ...) {}
            #define MX_DEBUG mxDebug
        #endif
    #else
        //GCC's CPP has extensions; it allows for macros with a variable number of arguments. We use this extension here to preprocess pmesg away.
        #define MX_DEBUG(format, args...) ((void)0)
    #endif

    #if (DEBUG_ENABLE==1) && (DEBUG_ENABLE_INFO==1) && !defined(MX_DEBUG_DISABLE)
        #if (MX_DEBUG_IS_POINTER==1)        //More efficient, but only works if pMxDebug is defined in code. Disabled by default!
            #define MX_DEBUG_INFO   pMxDebug->printf
        #else
            #define MX_DEBUG_INFO   mxDebug
        #endif
    #else
        //GCC's CPP has extensions; it allows for macros with a variable number of arguments. We use this extension here to preprocess pmesg away.
        #define MX_DEBUG_INFO(format, args...) ((void)0)
    #endif

#endif  //#if defined(DEBUG_ENABLE_MAIN)
#endif /* SRC_MX_DEFAULT_DEBUG_H_ */
