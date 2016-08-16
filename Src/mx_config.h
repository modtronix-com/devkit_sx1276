/**
 * File:      mx_config.h
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
#ifndef MX_APP_CONFIG_H_
#define MX_APP_CONFIG_H_


// Function Prototypes ////////////////////////////////////////////////////////
void infoConfigStructs(void);

#define MXCONF_DESC_TYPE_1BIT   0xB1
#define MXCONF_DESC_TYPE_8BIT   0xB2

//Type: 0=1bit, 1=8bit, 2=16bit, 3=32bit
//Offset:
typedef struct MxConfDataDesc_ {
    uint8_t     type;               //0xB1=1bit, 0xB2=8bit - Other possible NOT SUPPORTED types: 0xB3=16bit, 0xB4=32bit
    uint8_t     size;               //Size in 'type'. For example, if type is '1bit', will be number of bit. If size of 8bit, will be number of 8bits...
    uint16_t    offset;             //Bits 0-11 = Offset(max 4096). Bits 12-15 is MSB is size
} PACKED MxConfDataDesc;

typedef struct MxConfHdr_ {
    uint8_t     startIncNbr;        //Incrementing number at start of structure. endIncNbr must be equal to startIncNbr+1
    uint8_t     structSize;         //Structure Size in MXCONF_SIZE_BASE(16) multiple - This is the size reserved in Non Volatile Memory(1=16, 2=32...)
    uint16_t    dataSize;           //Size of data part of this structure. Reserved space = structSize - dataSize. (Maybe use MSB 2bits for structSize)
    uint8_t     magicNumber;        //Magic number, is always 0xA5 if more "MxConfig Structures" to follow, or 0xA4 if this is the last one
    uint8_t     structID;           //Structure ID = 1(Radio 0), 2, 3, 4 or 5(Radio 5)
} PACKED MxConfHdr;

//typedef struct MxConfFtr_ {
//    uint8_t     endIncNbr;          //Incrementing number at end of structure. Must be equal to startIncNbr+1
//} PACKED MxConfFtr;


/**
 * Read "MxConfig Structure" with given ID from Non Volatile Memory. Copies it to given destination.
 * Only 'sizeData' bytes are copied to destination.
 *
 * @param id "MxConfig Structure" ID
 * @param pDest Pointer to destination
 * @param sizeData Size of used data in structure. This is NOT the size that is reserved in EEPROM, which is larger.
 * @param adrOffset Offset in Non Volatile memory where structure is located
 * @param sizeStruct Size of structure in Non Volatile memory
 * @param pDataDsc First byte is number of MxConfDataDesc structures, followed by array of MxConfDataDesc[]
 *
 * @return True if OK, else false
 */
bool mxconf_read_struct(uint8_t id, uint8_t* pDest, uint16_t sizeData, uint16_t adrOffset, uint16_t sizeStruct, uint8_t* pDataDsc = 0);

/**
 * Save given "MxConfig Structure".
 * @param id "MxConfig Structure" ID
 * @param pSrc Pointer to source structure
 * @param sizeData Size of used data in structure. This is NOT the size that is reserved in EEPROM, which is larger.
 * @param adrOffset Offset in Non Volatile memory where structure is located
 * @param sizeStruct Size of structure in Non Volatile memory
 * @param pDataDsc First byte is number of MxConfDataDesc structures, followed by array of MxConfDataDesc[]
 * @return True if OK, else false
 */
bool mxconf_save_struct(uint8_t id, uint8_t* pSrc, uint16_t sizeData, uint16_t adrOffset, uint16_t sizeStruct, uint8_t* pDataDsc = 0);


#endif /* MX_APP_CONFIG_H_ */
