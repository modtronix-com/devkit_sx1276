/**
 * File:      mx_app_config.cpp
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

#define DEBUG_ENABLE            1
#define DEBUG_ENABLE_INFO       1
#include "mx_default_debug.h"

//Enable debugging in include files if required - Include AFTER debug defines/includes above! It uses them for debugging!
#define DEBUG_ENABLE_MX_APP_CONFIG          1
#define DEBUG_ENABLE_INFO_MX_APP_CONFIG     1
#include "app_defs.h"    //Application defines, must be first include after debugging includes/defines
#include "mx_config.h"

//extern const uint16_t mxconfIdAdr[];

//const uint16_t mxconfIdAdr[] = {
//        MXCONF_ADR_ID0,
//        MXCONF_ADR_ID1,
//        MXCONF_ADR_ID2,
//        MXCONF_ADR_ID3,
//        MXCONF_ADR_ID4,
//        MXCONF_ADR_ID5
//};

//#define MXCONF_GET_OFFSET(id)  MxConf::getOffset(id)
//#define MXCONF_GET_OFFSET(id)  (mxconfIdAdr[id])


// Function Prototypes ////////////////////////////////////////////////////////
bool readHeader(uint8_t id, MxConfHdr* pHdr, uint32_t address, uint16_t sizeStruct);
bool eepSaveAndConfirm(uint32_t adr, uint8_t value);
bool updateAllStructures(void);

/**
 * Used for Debug only! Print size of structures.
 */
void infoConfigStructs(void) {
    // Print size of AppConfig - ensure it is 64 bytes long!
    MX_DEBUG("\r\nStruct AppConfig Size=%d", sizeof(AppConfig));
    // Debug output used to ensure bytes are packed. Should output "startIncNbr=0 structID=1 structSize=2 structVer=4"
//    MX_DEBUG("\r\nstartIncNbr=%d structID=%d structSize=%d structVer=%d", offsetof(AppConfig, startIncNbr),
//            offsetof(AppConfig, structID), offsetof(AppConfig, structSize), offsetof(AppConfig, structVer));

    // Print size of AppConfig - ensure it is 64 bytes long!
    MX_DEBUG("\r\nStruct RadioConfig Size=%d", sizeof(RadioConfig));
    // Debug output used to ensure bytes are packed. Should output "startIncNbr=0 structID=1 structSize=2 structVer=4"
//    MX_DEBUG("\r\nstartIncNbr=%d structID=%d structSize=%d structVer=%d", offsetof(RadioConfig, startIncNbr),
//            offsetof(RadioConfig, structID), offsetof(RadioConfig, structSize), offsetof(RadioConfig, structVer));
}


bool mxconf_read_struct(uint8_t id, uint8_t* pDest, uint16_t sizeData, uint16_t adrOffset, uint16_t sizeStruct, uint8_t* pDataDsc) {
    MxConfHdr hdr;
    uint32_t address;
    uint32_t addressData;   //Address to first byte of data

    MX_DEBUG_INFO("\r\nRead Structure!");
    //MX_DEBUG("\r\nMxConf Offset=%d, size=%d", MXCONF_GET_OFFSET(id), MxConf::getSize(id));
    address = EEPROM_START_ADDRESS + adrOffset; //Address of header

    //EEPROM does NOT currently contain valid data.
    if(readHeader(id, &hdr, address, sizeStruct) == false) {
        MX_DEBUG("\r\nMxConf%d Invalid!", id);
        //Save structure again. This should save correct version of structure!
        mxconf_save_struct(id, pDest, sizeData, adrOffset, sizeStruct, pDataDsc);
        return false;
    }

    //TODO - Seems like EEPROM contains old data structure! In this case, we have to rewrite WHOLE eeprom to new structures!
    if(hdr.structSize != (sizeStruct/MXCONF_SIZE_BASE)) {
        updateAllStructures();
    }

    //Get address of first byte of data
    addressData = address + sizeof(MxConfHdr);

    //Read ALL data (TODO - Replace with method below in future)
    MX_DEBUG_INFO("\r\nData=");
    for (uint i=0; i<hdr.dataSize; i++) {
        *pDest = *(__IO uint8_t*)addressData++;
        MX_DEBUG_INFO("%02x ", *pDest);
        pDest++;
    }

    //TODO - Enable this in future if needed. Only have to implement bit copying below, all rest is done
    //Check if EEPRM contains "Data Description". Will follow data
    /*
    uint8_t* pDataDesc;
    MX_DEBUG_INFO("\r\nData=");
    pDataDesc = (uint8_t*)(addressData + hdr.dataSize); //First byte AFTER data = "Data Descriptor"
    if(*pDataDesc != 0) {
        MxConfDataDesc desc;
        uint8_t* pCurrDest;
        uint8_t numDesc = *pDataDesc++;     //Number of "Data Descriptors"
        MX_DEBUG_INFO("\r\nDataDesc Num=%d", numDesc);
        //Read only data defined by "Data Descriptor"
        for (int iDesc=0; iDesc<numDesc; iDesc++) {
            uint i;
            //Read current "Data Descriptor"
            for(i=0; i<sizeof(MxConfDataDesc); i++) {
                ((uint8_t*)(&desc))[i] = *pDataDesc++;
            }

            address = addressData + desc.offset;    //Source address of block of data pointed to by current "Data Descriptor"
            pCurrDest = pDest + desc.offset;        //Destination address of block of data pointed to by current "Data Descriptor"

            //Copy 1-Bit data given by "Data Description"
            if(desc.type == MXCONF_DESC_TYPE_1BIT) {
                //TODO - Copy desc.size number of bits from source to destination
            }
            //Copy 8-Bit data given by "Data Description"
            if(desc.type == MXCONF_DESC_TYPE_8BIT) {
                for(i=0; i<desc.size; i++) {
                    *pCurrDest = *(__IO uint8_t*)address++;
                }
            }
            else {
                MX_DEBUG("\r\nERR: DataDesc Type NOT supported!")
            }
        }
    }
    else {
        //Read ALL data (TODO - Only copy data defined by "Data Descriptor")
        for (uint i=0; i<hdr.dataSize; i++) {
            *pDest = *(__IO uint8_t*)address++;
            MX_DEBUG_INFO("%02x ", *pDest);
            pDest++;

        }
    }
    */

    return true;    //OK
}

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
bool mxconf_save_struct(uint8_t id, uint8_t* pSrc, uint16_t sizeData, uint16_t adrOffset, uint16_t sizeStruct, uint8_t* pDataDsc) {
    uint i;
    uint8_t endIncNbr;
    uint16_t sizeDataDsc = 0;   //Size of "Data Descriptor"
    MxConfHdr hdr;
    HAL_StatusTypeDef status;
    uint32_t address;
    uint8_t* ptr;

    MX_DEBUG_INFO("\r\nSave Struct %d", id);

    #if (DEBUG_ENABLE==1)
    if((sizeStruct%MXCONF_SIZE_BASE) != 0) {
        MX_DEBUG("\r\nERR: sizeStruct invalid!");
        return false;
    }
    #endif

    //Get "Data Descriptor" size
    if(pDataDsc!=0) {
        sizeDataDsc = *pDataDsc;  //Get number of "Data Descriptors"
        if(sizeDataDsc != 0) {
            sizeDataDsc = 1 + (sizeDataDsc * sizeof(MxConfDataDesc));
        }
        MX_DEBUG_INFO("\r\nDataDesc Size=%d", sizeDataDsc);
    }

    //Check if structure size is large enough
    if(sizeStruct < (sizeData + sizeDataDsc)) {
        MX_DEBUG("\r\nsizeStruct TOO SMALL!");
        return false;
    }

    //Print reserved space
    MX_DEBUG_INFO("\r\nSize=%d, Reserve=%d", sizeStruct, sizeStruct-(sizeData+sizeDataDsc));

    address = EEPROM_START_ADDRESS + adrOffset;

    //EEPROM does NOT currently contain valid data.
    if(readHeader(id, &hdr, address, sizeStruct) == false) {
        MX_DEBUG("\r\nInvalid Hdr%d in EEPROM, creating new Hdr!", id);
        hdr.startIncNbr = 0;
    }

    hdr.structSize = sizeStruct/MXCONF_SIZE_BASE;
    hdr.dataSize = sizeData;
    hdr.structID = id;
    hdr.magicNumber = (adrOffset==MXCONF_ADR_LAST) ? 0xA4 : 0xA5;
    #if(DEBUG_ENABLE_INFO==1)
    if(adrOffset==MXCONF_ADR_LAST) {
        MX_DEBUG_INFO("\r\nLast Structure!");
    }
    #endif

    //Increment "Incrementing numbers"
    hdr.startIncNbr++;
    endIncNbr = hdr.startIncNbr;
    endIncNbr++;

    ////Unlock before we can write
    HAL_FLASHEx_DATAEEPROM_Unlock();

    //If code below brings errors, this will clear error flag before attempting to write to EEPROM
    //FLASH_WaitForLastOperation((uint32_t)100); //Wait for last operation for 100ms

    //Check and Print Flash error
    //if(__HAL_FLASH_GET_FLAG(FLASH_FLAG_WRPERR)     != RESET) { MX_DEBUG("\r\nFlErr FLASH_FLAG_WRPERR");}
    //if(__HAL_FLASH_GET_FLAG(FLASH_FLAG_PGAERR)     != RESET) { MX_DEBUG("\r\nFlErr FLASH_FLAG_PGAERR");}
    //if(__HAL_FLASH_GET_FLAG(FLASH_FLAG_SIZERR)     != RESET) { MX_DEBUG("\r\nFlErr FLASH_FLAG_SIZERR");}
    //if(__HAL_FLASH_GET_FLAG(FLASH_FLAG_RDERR)      != RESET) { MX_DEBUG("\r\nFlErr FLASH_FLAG_RDERR");}
    //if(__HAL_FLASH_GET_FLAG(FLASH_FLAG_OPTVERRUSR) != RESET) { MX_DEBUG("\r\nFlErr FLASH_FLAG_OPTVERRUSR");}
    //if(__HAL_FLASH_GET_FLAG(FLASH_FLAG_OPTVERR)    != RESET) { MX_DEBUG("\r\nFlErr FLASH_FLAG_OPTVERR");}

    //Save header
    ptr = (uint8_t*)&hdr;
    for(i=0; i<sizeof(MxConfHdr); i++) {
        if (eepSaveAndConfirm(address++, *ptr) == false) {
            status = HAL_ERROR;
            break;
        }
        ptr++;
    }

    //Save actual structure
    if(status != HAL_OK) {
        MX_DEBUG_INFO("\r\nData=");
        for (i=0; i<sizeData; i++) {
            MX_DEBUG_INFO("%02x ", *pSrc);
            if (eepSaveAndConfirm(address++, *pSrc) == false) {
                status = HAL_ERROR;
                break;
            }
            pSrc++;
        }
    }

    //Save "Data Description" if given.
    if((status != HAL_OK) && (sizeDataDsc!=0)) {
        //address = EEPROM_START_ADDRESS + adrOffset + sizeData;    //address will already have this value = byte following data(sizeData)

        //Get size of "Data Description". First byte points to number of MxConfDataDesc structures to follow
        while(sizeDataDsc-- != 0) {
            //MX_DEBUG("\r\nDD=%d adr=0x%x", *pDataDsc, address);
            if (eepSaveAndConfirm(address++, *pDataDsc) == false) {
                status = HAL_ERROR;
                break;
            }
            pDataDsc++;
        }
    }

    //Save "Incrementing number at end of structure"
    if(status != HAL_OK) {
        address = EEPROM_START_ADDRESS + adrOffset + sizeStruct - 1;
        if (eepSaveAndConfirm(address++, endIncNbr) == false) {
            status = HAL_ERROR;
        }
    }

    HAL_FLASHEx_DATAEEPROM_Lock();      //Lock again

    if (status==HAL_OK) {
        MX_DEBUG("\r\nSaved MxConfig%d to EEPROM", id);
    }

    return (status==HAL_OK)?true:false;    //OK
}

/**
 * Read header
 * @return True if OK, else false
 */
bool readHeader(uint8_t id, MxConfHdr* pHdr, uint32_t address, uint16_t sizeStruct) {
    uint8_t* pEeprom;
    uint8_t* pDst;
    uint8_t endIncNbr;
    //Size of structure in EEPROM. If EEPROM contains data from older firmware, it can be smaller than given sizeStruct!
    uint16_t eepStructSize;

    pEeprom = (uint8_t*)address;
    MX_DEBUG_INFO("\r\nRead Header at 0x%x", address);

    //Get header
    pDst = (uint8_t*)pHdr;
    for(uint i=0; i<sizeof(MxConfHdr); i++) {
        *pDst++ = *pEeprom++;
    }

    //Check magicNumber is 0xA5 of 0xA4
    if((pHdr->magicNumber&0xFE) != 0xA4) {
        MX_DEBUG("\r\nERROR! Header magic number wrong!");
        return false;
    }

    //Confirm size if not larger than the size we have. Could be smaller if this is older version.
    eepStructSize = pHdr->structSize * MXCONF_SIZE_BASE;
    if(eepStructSize > sizeStruct) {
        MX_DEBUG("\r\nERROR! Header size too big!");
        return false;
    }

    //Increment to "Incrementing number at end of structure"
    pEeprom = (uint8_t*)(address + eepStructSize - 1);   //Update pointer to point to last byte of structure
    MX_DEBUG_INFO("\r\nRead endIncNbr at 0x%x", (uint32_t)pEeprom);
    endIncNbr = *pEeprom;

    //Check endIncNbr is 1 larger than startIncNbr
    endIncNbr--;    //Decrement, should now be equal to
    if(endIncNbr != pHdr->startIncNbr) {
        MX_DEBUG("\r\nERROR! IncNbr Error!");
        return false;
    }

    return true;
}

/**
 * Save and confirm given byte to EEPROM
 * @return True if OK, else false
 */
bool eepSaveAndConfirm(uint32_t adr, uint8_t value) {
    HAL_StatusTypeDef status;

    //Check if byte already has requested value
    if ( *(__IO uint8_t*)adr == value) {
        return true;    //Nothing to do, return OK
    }

    //Save byte
    status = HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_BYTE, adr, value);
    if(status != HAL_OK) {
        MX_DEBUG("\r\nEEPROM Save ERR!");
        return false;
    }

    //Confirm byte written correctly
    if ( *(__IO uint8_t*)adr != value) {
        MX_DEBUG("\r\nEEPROM Save ERR!");
        return false;
    }
    return true;
}

/**
 * TODO - This function is called when the EEPROM contains "MxConfig Structures" from an older version
 * of firmware. Typically the structures in EEPROM will be smaller than current structures. The following
 * has to be done:
 * - Read last structure from EEPROM
 * - Write last structure to EEPROM. This will be at a higher address than old structure
 * - Do same for all remaining structures (second last, third last, ...., first)
 */
bool updateAllStructures(void) {
    MX_DEBUG_INFO("\r\nUpdating all EEPROM Structures!");
    return true;
}
