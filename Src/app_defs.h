/**
 * File:      app_defs.h
 *
 * Author:    Modtronix Engineering - www.modtronix.com
 *
 * Description: Single include file for all source files in this project. This file included:
 *              - All include files used in project
 *              - Global defines and data
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
#ifndef MX_APP_DEFS_H_
#define MX_APP_DEFS_H_

#include "inair.h"
#include "mx_buffer_base.h"
#include "mx_circular_buffer.h"

#if !defined(WEAK)
#if defined (__ICCARM__)
    #define WEAK     __weak
#else
    #define WEAK     __attribute__((weak))
#endif
#endif

#if !defined(PACKED)
#if defined (__ICCARM__)
    #define PACKED   __packed
#else
    #define PACKED   __attribute__((packed))
#endif
#endif



///////////////////////////////////////////////////////////////////////////////
// Defines ////////////////////////////////////////////////////////////////////
//#define I2C1_SPEED              200000  //Speed for I2C Bus1 200kbits/sec
#define I2C1_SPEED              100000  //Speed for I2C Bus1 100kbits/sec



///////////////////////////////////////////////////////////////////////////////
// USB Defines ////////////////////////////////////////////////////////////////
#define MX_ENABLE_USB           0
#define RX_BUF_USB_SIZE         256         // Size of the USB receive buffer
#define RX_BUF_USB_COUNTERTYPE  uint32_t    // Type used for counters in the USB receive buffer
#define RX_BUF_USB_COMMANDS     16          // Number of commands the USB receive buffer can store

#define TX_BUF_USB_SIZE         256         // Size of the USB receive buffer
#define TX_BUF_USB_COUNTERTYPE  uint32_t    // Type used for counters in the USB receive buffer
#define TX_BUF_USB_COMMANDS     16          // Number of commands the USB receive buffer can store


///////////////////////////////////////////////////////////////////////////////
// Radio Defines //////////////////////////////////////////////////////////////
#define PING_PERIOD_MS          2000
#define RADIO_RXBUF_SIZE        32      // Define the payload size here
#define RADIO_TXBUF_SIZE        32

#define DISABLE_RESET_RADIO_USB_TIMERS
#define RESET_TIMEOUT_RADIO     30  //CPU will reset if no Ratio TX or RX for this period (in seconds)
#define RESET_TIMEOUT_USB_CMD   30  //CPU will reset if no USB command processed for this period (in seconds)

//Receive listeners
#define RX_LISTENER_APP         0x01
#define RX_LISTENER_USB         0x02
//#define RX_LISTENER_DISP      0x04
#if(MX_ENABLE_USB==1)
    #define RX_LISTENER_MASK    (RX_LISTENER_APP|RX_LISTENER_USB)
#else
    #define RX_LISTENER_MASK    (RX_LISTENER_APP)
#endif

//Number of Radios used. Is always 1, except if there are multiple LoRa radios present.
#define RADIO_COUNT         1
#define RADIO_COUNT_CHAR    ('0'+RADIO_COUNT)

//Radio Default values
#define RF_FREQUENCY                                918800000 // 918.8 kHz
#define TX_OUTPUT_POWER                             14        // 14 dBm. Max 14 for inAir4 and inAir9, and 20(17) for inAir9B
#define TX_OUTPUT_POWER_BOOST                       17        // 17 dBm. Default power for hardware using Boost output, like inAir9B
#define LORA_BANDWIDTH                              LORA_BW_500000
#define LORA_SPREADING_FACTOR                       12      // SF7..SF12
#define LORA_CODINGRATE                             1       // 1=4/5, 2=4/6, 3=4/7, 4=4/8
#define LORA_PREAMBLE_LENGTH                        8       // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT                         5       // Receive Timeout in Symbols(used for SX1276 chip). TimeOut = SymbTimeout * Tsym
#define RX_TIMEOUT_VALUE                            5000000 // Receive Timeout in us = 5 Sec. Used by state machine(not internal SX1276 chip timeout)
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false   // true = No Header(Implicit header mode), false = Has header(Explicit header mode)
#define LORA_FHSS_ENABLED                           false
#define LORA_NB_SYMB_HOP                            4
#define LORA_IQ_INVERSION_ON                        false
#define LORA_CRC_ENABLED                            true

enum RX_STATUS {
    RX_STATUS_OK = 0,
    RX_STATUS_TIMEOUT,
    RX_STATUS_ERR_CRC
};

enum RX_MODE {
    RX_MODE_SINGLE = 0,
    RX_MODE_CONTINUOUS_SINGLE,
    RX_MODE_CONTINUOUS
};

enum TX_MODE {
    TX_MODE_IDLE_AFTER_TXION = 0,
    TX_MODE_RX_AFTER_TXION
};

/**
 * Radio mode. If multiple radios, STOPPED, MASTER & SLAVE mode is ONLY used for first ratio(index 0)
 */
enum RADIO_MODE {
    RADIO_MODE_STOPPED = 0,
    RADIO_MODE_MASTER,
    RADIO_MODE_SLAVE,
    RADIO_MODE_ERR_NORADIO, //Error - No radio. Error code 2
    RADIO_MODE_MAX
};
#define RADIO_MODE_ERR_FIRST    RADIO_MODE_ERR_NORADIO



///////////////////////////////////////////////////////////////////////////////
// Display Defines ////////////////////////////////////////////////////////////
//#define DISABLE_OLED            // Disable OLED
#define DISPLAY_CHAR_WIDTH   5
#define DISPLAY_CHAR_HEIGHT  7



///////////////////////////////////////////////////////////////////////////////
// Macros /////////////////////////////////////////////////////////////////////
#define OFFSETOF_MSB(strc, mem) ((offsetof(strc, mem)>>8)&0xff)
#define OFFSETOF_LSB(strc, mem) (offsetof(strc, mem)&0xff)

/**
 * Size in bytes from first to last member. Includes size of both members. For example, if there are two members, and
 * they are uing32_t, size will be 2*4 = 8
 */
#define SIZE_IN_BYTES(strc, firstMem, lastMem) (offsetof(strc,lastMem) - offsetof(strc,firstMem) + sizeof(strc::lastMem))



///////////////////////////////////////////////////////////////////////////////
// Structs and Unions /////////////////////////////////////////////////////////
typedef struct AppData_ {
    union flags_ {
        struct {
            uint32_t    dirtyConf           :1; //Set when AppConfig changes. Used & Cleared by main App
            uint32_t    dirtyConfDisp       :1; //Set when AppConfig changes. Used & Cleared by Display
            uint32_t    displayOff          :1; //Display is currently off
        } bits;
        uint32_t Val;
        //Constructors
        flags_()            : Val(0) {}
        flags_(uint32_t v)  : Val(v) {}
    } PACKED flags;

    AppData_() :
        flags(0),
        rxCountPingPong(0)
    {}

    uint16_t    rxCountPingPong;        //Ping-Pong valid receive count
    uint16_t    rxErrCountPingPong;     //Ping-Pong error count, is incremented for timeout or faulty received packet(addressed to us)
    uint16_t    oldRxCountPingPong;     //Value of "rxCountPingPong" when last PING message was sent
} PACKED AppData;

typedef struct RadioData_ {
    union flags_ {
        struct {
            uint32_t dirtyConf          :1; //Set when RadioConfig changes. Used & Cleared by main App
            uint32_t dirtyConfDisp      :1; //Set when RadioConfig changes. Used & Cleared by Display
            uint32_t initialized        :1; //The radio has been initialized
            uint32_t noRadio            :1; //No radio detected
            uint32_t noRadioMsgUSB      :1; //Send "No Radio" message via USB
            uint32_t rxMsgLost          :1; //Set if OnRxDone is called, and previous received msg not processed yet(rxLen!=0)
        } bits;
        uint32_t    Val;

        //Constructors
        flags_()            : Val(0) {}
        flags_(uint32_t v)  : Val(v) {}
    } flags;

    int         tmrRadio;
    MxCircularBuffer<uint8_t, RADIO_TXBUF_SIZE> txBuf;
    uint8_t     smRadio;        //Radio State Machine state
    uint8_t     rxStatus;       //Receive status, is a RX_STATUS_xxx define.
    uint8_t     rxListeners;    //Contains RX_LISTENER_xx flags. Test with RX_LISTENER_MASK to see if any pending
    uint8_t     rxBuf[RADIO_RXBUF_SIZE];
    uint16_t    rxLen;

    uint16_t    rxErrCnt;       //Faulty packets = Timeouts and CRC errors. When it increments, error type is available in rxStatus
    uint16_t    rxCount;        //Count valid received packets. Packets NOT addressed to us, or with errors are NOT counted!

    int16_t     RssiValue;      //RSSI value of last reception, or 0xfff if receive timeout
    int16_t     RssiValueSlave; //RSSI value of last reception, or 0xfff if receive timeout
    int8_t      SnrValue;       //Signal to noise ratio

    uint8_t     mode;           //Radio mode, is a RADIO_MODE_XXX define
} RadioData;





///////////////////////////////////////////////////////////////////////////////
// "MxConfig Structure" Defines ///////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#define EEPROM_START_ADDRESS            0x08080000
#define MXCONF_ADR_START                EEPROM_START_ADDRESS
#define MXCONF_ADR_LAST                 MXCONF_ADR_RadioConfig4 /* Address of LAST structure "MxConfig Structure" */
#define MXCONF_SIZE_BASE                16

// AppConfig Defines //////////////////////////////////////////////////////////
#define MXCONF_ID_AppConfig             0
#define MXCONF_SIZE_ID0                 64
#define MXCONF_SIZE_AppConfig           MXCONF_SIZE_ID0 /* Size to reserve in EEPROM, must be multiple of MXCONF_SIZE_BASE */
#define MXCONF_ADR_ID0                  0
#define MXCONF_ADR_AppConfig            MXCONF_ADR_ID0

// RadioConfig ////////////////////////////////////////////////////////////////
#define MXCONF_ID_RadioConfig0          1
#define MXCONF_ID_RadioConfig1          2
#define MXCONF_ID_RadioConfig2          3
#define MXCONF_ID_RadioConfig3          4
#define MXCONF_ID_RadioConfig4          5
#define MXCONF_SIZE_RadioConfig         64
#define MXCONF_SIZE_ID1                 MXCONF_SIZE_RadioConfig
#define MXCONF_SIZE_ID2                 MXCONF_SIZE_RadioConfig
#define MXCONF_SIZE_ID3                 MXCONF_SIZE_RadioConfig
#define MXCONF_SIZE_ID4                 MXCONF_SIZE_RadioConfig
#define MXCONF_SIZE_ID5                 MXCONF_SIZE_RadioConfig
#define MXCONF_ADR_ID1                  (MXCONF_ADR_ID0 + MXCONF_SIZE_ID0)
#define MXCONF_ADR_ID2                  (MXCONF_ADR_ID1 + MXCONF_SIZE_ID1)
#define MXCONF_ADR_ID3                  (MXCONF_ADR_ID2 + MXCONF_SIZE_ID2)
#define MXCONF_ADR_ID4                  (MXCONF_ADR_ID3 + MXCONF_SIZE_ID3)
#define MXCONF_ADR_ID5                  (MXCONF_ADR_ID4 + MXCONF_SIZE_ID4)
#define MXCONF_ADR_RadioConfig0         MXCONF_ADR_ID1
#define MXCONF_ADR_RadioConfig1         MXCONF_ADR_ID2
#define MXCONF_ADR_RadioConfig2         MXCONF_ADR_ID3
#define MXCONF_ADR_RadioConfig3         MXCONF_ADR_ID4
#define MXCONF_ADR_RadioConfig4         MXCONF_ADR_ID5


class MxConf {
public:
    /**
     * Get offset of given "MxConfig Structure". Will be 0 for first "MxConfig Structure".
     */
    static inline uint16_t getOffset(uint16_t id) {
        const uint16_t mxconfIdAdr[] = {
                MXCONF_ADR_ID0,
                MXCONF_ADR_ID1,
                MXCONF_ADR_ID2,
                MXCONF_ADR_ID3,
                MXCONF_ADR_ID4,
                MXCONF_ADR_ID5
        };
        return mxconfIdAdr[id];
    }


    /**
     * Get size allocated in EEPROM for given "MxConfig Structure". Note this is larger than the actual
     * size of the "MxConfig Structure" we store there. Has reserved bytes.
     */
    static inline uint16_t getSize(uint16_t id) {
        const uint16_t mxconfIdSize[] = {
                MXCONF_SIZE_ID0,
                MXCONF_SIZE_ID1,
                MXCONF_SIZE_ID2,
                MXCONF_SIZE_ID3,
                MXCONF_SIZE_ID4,
                MXCONF_SIZE_ID5
        };
        return mxconfIdSize[id];
    }
};



/**
 * Application Settings that are saved to EEPROM
 * IMPORTANT!
 * - Only add to end of Structure and "Bit Structures".
 * - Ensure structure size is multiple of 2.
 */
typedef struct AppConfig_ {
    //Structure Flags
    //IMPORTANT, When adding new flags in future:
    // - Update MXCONF_DESC_AppConfig structure!
    // - Ensure their default values are 0! New firmware might read old struct(without these new flags) from EEPROM with values=0.
    union flags_ {
        struct {
            uint16_t fill:1;        //Fill - remove later when/if flags are used
        } bits;
        uint16_t     Val;

        //Constructors
        flags_()            : Val(0) {}
        flags_(uint16_t v)  : Val(v) {}
    } PACKED flags;

    //Bytes
    uint8_t     displayBrigtness;   //Display brightness, 0=minimum(not off), 0xff=maximum
    uint8_t     displayAutoOff;     //Display auto off time seconds. 0=never turn off

    //When we are SLAVE, is our slave address. Not used when we are MASTER
    uint8_t     localAdr;       //This device's address

    //When we are MASTER, is address of remote slave we are communicating with. Not used when we are SLAVE
    uint8_t     remotelAdr;     //The remote devices

    AppConfig_() :
        flags(0),   //Calls constructor of flags_ union
        displayBrigtness(2),
        displayAutoOff(60),
        localAdr(1),
        remotelAdr(1)
        {}
} PACKED AppConfig;
#define AppConfig_FIRST_MBR     displayBrigtness    //First "non-flag" member of structure
#define AppConfig_LAST_MBR      remotelAdr          //Last "non-flag" member of structure


//Data Description for data contained in AppConfig
//Type(Byte1):      A MXCONF_DESC_TYPE_xBIT define (0xB1=1bit, 0xB2=8bit)
//Size(Byte2):      Size in 'Type'. For example, if type is '1bit', will be number of bits. If size of 8bit, will be number of 8bits variables...
//Offset(Byte3-4):  Bits 0-11 = Offset(max 4096). Bits 12-15 is MSB off size if required
#define MXCONF_DESC_AppConfig {  \
    2, /*Number of MxConfDataDesc structures to follow*/  \
    /*1 - Descriptor for flags(1Bit) = uses 0 bits*/  \
    MXCONF_DESC_TYPE_1BIT, 0,  \
    OFFSETOF_MSB(AppConfig_, flags), OFFSETOF_LSB(AppConfig_, flags),  \
    /*2 - Descriptor for 8-Bit bytes = uses 2 members*/  \
    MXCONF_DESC_TYPE_8BIT, SIZE_IN_BYTES(AppConfig_, AppConfig_FIRST_MBR, AppConfig_LAST_MBR),  \
    OFFSETOF_MSB(AppConfig_, AppConfig_FIRST_MBR), OFFSETOF_LSB(AppConfig_, AppConfig_FIRST_MBR) }



/**
 * Radio Settings that are saved to EEPROM
 * IMPORTANT!
 * - Only add to end of Structure and "Bit Structures".
 * - Ensure structure size is multiple of 2.
 */
typedef struct RadioConfig_ {
    //Structure Flags
    //IMPORTANT, When adding new flags in future:
    // - Update MXCONF_DESC_RadioConfig structure!
    // - Ensure their default values are 0! New firmware might read old struct(without these new flags) from EEPROM with values=0.
    union flags_ {
        struct {
            uint16_t fill:1;            //Fill - remove later when/if flags are used
        } bits;
        uint16_t     Val;

        //Constructors
        flags_()            : Val(0) {}
        flags_(uint16_t v)  : Val(v) {}
    } PACKED flags;

    //Radio Flags
    //IMPORTANT, When adding new flags in future:
    // - Update MXCONF_DESC_RadioConfig structure!
    // - Ensure their default values are 0! New firmware might read old struct(without these new flags) from EEPROM with values=0.
    union conf_ {
        struct {
            uint16_t codingRate:4;      //1=4/5, 2=4/6, 3=4/7,  4=4/8
            uint16_t crcEnable:1;
            uint16_t fixLength:1;       //0=variable(Has header = Explicit header mode), 1=fixed(No Header = Implicit header mode)
            uint16_t fshhEnable:1;
            uint16_t iqInversionEnable:1;
        } lora;
        uint16_t    Val;

        //Constructors
        conf_()             : Val(0) {}
        conf_(uint16_t v)   : Val(v) {}
    } PACKED conf;

    uint8_t     boardType;      //The board type, a BOARD_XXX define
    uint8_t     txMode;         //Transmit Mode. 0=Radio goes Idle after TXion, 1=Radio switched to RX after TXion
    //Receive mode:
    // 0 = Single receive mode, Radio enters Idle mode after receiving a message
    // 1 = Continues receive mode, Radio enters Idle mode after receiving first message
    // 2 = Continues receive mode, Radio stays in receive mode after receiving a message
    uint8_t     rxMode;
    uint8_t     bw;             //bandwidth, a LORA_BW_XX define
                                    //0=7.8 1=10.4 2=15.6 3=20.8 4=31.25 5=41.7 6=62.5 7=125 8=250 9=500 kHz
    uint8_t     sf;             //Spreading factor, a value from 7 to 12
    uint8_t     power;          //Output power in dBm, a value from 1 to 20
    uint8_t     preambleLength; //Length of preamble, default = 8
    uint8_t     numberSymHop;
    //In Single Receive mode, the radio searches for a preamble during a given time window. If a preamble hasn’t been
    //  found at the end of the time window, the chip generates the RxTimeout interrupt and goes back to stand-by mode.
    //  The length of the window (in symbols) is defined by this value, and should be in the range of 4 (minimum time
    //  for the modem to acquire lock on a preamble) up to 1023 symbols. Default is 5. If no preamble is detected during
    //  this window the RxTimeout interrupt is generated and the radio goes back to stand-by mode.
    uint8_t     symbolTimeout;  //Receive Timeout in Symbols(used for SX1276 chip). TimeOut = SymbTimeout * Tsym

    //Fill onto 32-bit boundary
    uint8_t     fillArray2[3];

    uint32_t    frequency;      //Frequency
    uint32_t    rxTimeout;      //RX Timeout in us. Used for "onRxTimeout()" callback, and NOT for SX1276 chip settings! 0=No Timeout.
} PACKED RadioConfig;
#define RadioConfig_FIRST_MBR     boardType     //First "non-flag" member of structure
#define RadioConfig_LAST_MBR      rxTimeout     //Last "non-flag" member of structure


//Data Description for data contained in RadioConfig
//Type(Byte1):      A MXCONF_DESC_TYPE_xBIT define (0xB1=1bit, 0xB2=8bit)
//Size(Byte2):      Size in 'Type'. For example, if type is '1bit', will be number of bits. If size of 8bit, will be number of 8bits variables...
//Offset(Byte3-4):  Bits 0-11 = Offset(max 4096). Bits 12-15 is MSB is size
#define MXCONF_DESC_RadioConfig {  \
    3, /*Number of MxConfDataDesc structures to follow*/  \
    /*1 - Descriptor for flags(1Bit) = uses 0 bits*/  \
    MXCONF_DESC_TYPE_1BIT, 0,  \
    OFFSETOF_MSB(RadioConfig_, flags), OFFSETOF_LSB(RadioConfig_, flags),  \
    /*2 - Descriptor for flags(1Bit) = uses 8 bits*/  \
    MXCONF_DESC_TYPE_1BIT, 8, \
    OFFSETOF_MSB(RadioConfig_, conf), OFFSETOF_LSB(RadioConfig_, conf),  \
    /*3 - Descriptor for 8-Bit members*/  \
    MXCONF_DESC_TYPE_8BIT, SIZE_IN_BYTES(RadioConfig_, RadioConfig_FIRST_MBR, RadioConfig_LAST_MBR),  \
    OFFSETOF_MSB(RadioConfig_, RadioConfig_FIRST_MBR), OFFSETOF_LSB(RadioConfig_, RadioConfig_FIRST_MBR) }

//Alternative method, uses one more element! Distinguish between 8-Bit and 32-Bit members.
#if(0)
#define MXCONF_DESC_RadioConfig {  \
    4, /*Number of MxConfDataDesc structures to follow*/  \
    /*1 - Descriptor for flags(1-Bit)*/  \
    MXCONF_DESC_TYPE_1BIT, 1,  \
    OFFSETOF_MSB(RadioConfig_, flags), OFFSETOF_LSB(RadioConfig_, flags),  \
    /*2 - Descriptor for flags(1-Bit)*/  \
    MXCONF_DESC_TYPE_1BIT, 8, \
    OFFSETOF_MSB(RadioConfig_, conf), OFFSETOF_LSB(RadioConfig_, conf),  \
    /*3 - Descriptor for 8-Bit members*/  \
    MXCONF_DESC_TYPE_8BIT, SIZE_IN_BYTES(RadioConfig_, boardType, symbolTimeout),  \
    OFFSETOF_MSB(RadioConfig_, boardType), OFFSETOF_LSB(RadioConfig_, boardType),  \
    /*4 - Descriptor for 32-Bit members*/  \
    MXCONF_DESC_TYPE_32BIT, (SIZE_IN_BYTES(RadioConfig_, frequency, rxTimeout)/4),  \
    OFFSETOF_MSB(RadioConfig_, frequency), OFFSETOF_LSB(RadioConfig_, frequency) }
#endif


#endif /* MX_APP_DEFS_H_ */
