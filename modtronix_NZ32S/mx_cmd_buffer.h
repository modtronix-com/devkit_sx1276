/**
 * File:      mx_ascii_cmd_buffer.h
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
#ifndef SRC_MX_CMD_BUFFER_H_
#define SRC_MX_CMD_BUFFER_H_

// This file contains an ASCII command buffer. It is used to store ASCII commands.
//
// Each command ends with a ';' character. CR(0x0a='\r') and LF(0x0d='\n') are converted to ';' characters.
//
// A ASCII command is an ASCII Formatted String, with Escape Sequences (Control Characters).
// It uses 2 upper case characters to represent a single hex character, and can have embedded strings (enclosed
// within single quotation marks = ') and "Control Characters" (Lower case characters 'a' to 'z')
//
// It has the following format:
// HH   = Two upper case characters representing a single byte (hex value).
// XHH  = **Not Implemented Yet** Same as HH
// NDD  = **Not Implemented Yet**  Decimal number. For example "N100" = decimal 100 t0='500'
// c    = **Not Implemented Yet** Lower case character 'a' to 'z' represents a "control character".
// '    = Enclose string with single quotes. A double ' character will not end the string, but represents a
//        single ' character.
// ^^   = Two "escape characters" represents a single '^' character.
// ^x   = Use this format to represent a "control character" that is not lower. Lower case characters do not have
//        to be escaped. Currently not used, but reserved for future use!
//
//
// ===== Examples when NOT using "Escape Character" =====
// 0156A9BC; is decoded as:
//  0x01, 0x56, 0xA9, 0xBC
//
// BC56'Hello'; is decoded as:
//  0xBC, 0x56, H, e, l, l, o
//
// A1'Hi^'; is decoded as:
//  0xA1, H, i, ^
//
// s6A52p; is decoded as("lower case" control characters NOT supported without "Escape Character"):
//  0x6A, 0x52
//
//
// ===== Examples when using "Escape Character" '^' =====
// s6A52p; is decoded as:
//  ^, s, 0x6A, 0x52, ^, p
//
// s50'Hi'p; is decoded as:
//  ^, s, 0x50, H, i, ^, p
//
// 'Hi^'; is decoded as:
//  H, i, ^, ^

#include "mx_buffer_base.h"
#include "mx_circular_buffer.h"


// Debugging //////////////////////////////////////////////////////////////////
// To enable debug output from this file, define MX_DEBUG and DEBUG_ENABLE_MX_CMD_BUFFER before
// including this file.
//
//Defines for MXH_DEBUG - debugging for include file
#if !defined(DEBUG_ENABLE_MX_CMD_BUFFER)
    #define DEBUG_ENABLE_MX_CMD_BUFFER          0
#endif
#if !defined(DEBUG_ENABLE_INFO_MX_CMD_BUFFER)
    #define DEBUG_ENABLE_INFO_MX_CMD_BUFFER     0
#endif

#if !defined(MXH_DEBUG)
    #if defined(MX_DEBUG) && (DEBUG_ENABLE_MX_CMD_BUFFER==1)
        #define MXH_DEBUG MX_DEBUG
    #else
        #define MXH_DEBUG(format, args...) ((void)0)
    #endif
#endif

#if !defined(MXH_DEBUG_INFO)
    #if defined(MX_DEBUG) && (DEBUG_ENABLE_MX_CMD_BUFFER==1) && (DEBUG_ENABLE_INFO_MX_CMD_BUFFER==1)
        #define MXH_DEBUG_INFO MX_DEBUG
    #else
        #define MXH_DEBUG_INFO(format, args...) ((void)0)
    #endif
#endif


/** Templated Circular buffer class
 */
template<uint32_t BufferSize, uint8_t Commands = 16, typename CounterType = uint32_t>
class MxCmdBuffer : public MxBuffer {
public:
    MxCmdBuffer() : _head(0), _tail(0), _full(false),
            _errBufFull(0), _dontSaveCurrentCommand(0), _lastCharWasEOF(0), flags(0)
    {
        //flags.Val = 0;
    }

    ~MxCmdBuffer() {
    }

    /** Adds a byte to the current command in the buffer. This function checks for the "End of Command" character,
     * and if found, increments the "command count" register indicating how many commands this buffer has.
     *
     * If the buffer becomes full before the "End of Command" character is reached, all bytes added for current
     * command are removed. And, all following bytes added until the next "End of Command" character will be ignored.
     *
     * The checkBufferFullError() function can be used to check if this function caused an error, and command was
     * not added to buffer.
     *
     * @param data Data to be pushed to the buffer
     *
     * @return true if character added to buffer, else false. Important to note that all characters added to
     *      current command CAN BE REMOVED if buffer gets full before "End of Command" added to buffer!
     */
    bool put(const uint8_t& data) {
        uint8_t c = data;

        //Check if buffer full! If so:
        // - All data for this command added till now is lost
        // - All future data added for this command is ignored, until next "End of command" character is received
        if (isFull() && (_dontSaveCurrentCommand==false)) {
            _dontSaveCurrentCommand = true;
            _errBufFull = true;
            MXH_DEBUG("\r\nBuffer full, cmd LOST!");

            //Remove all character added for this command. Restore head to last "End of Command" pointer
            if (cmdEndsBuf.isEmpty() == false) {
                //Restore head to byte following last "End of Command" pointer
                CounterType oldHead;
                oldHead = ((cmdEndsBuf.peekLastAdded()+1) % BufferSize);
                //Ensure buffer not still full
                if (oldHead != _head) {
                    _head = oldHead;
                    _full = false;
                }
            }
            //If no commands in buffer, set head=tail
            else {
                _head = _tail;
                _full = false;
            }
        }

        //Check if "End of Command" byte. A command string is terminated with a ';', CR(0x0a='\r') or LF(0x0d='\n') character
        if ((c == ';') || (c == 0x0d) || (c == 0x0a)) {
            if (flags.bits.replaceCrLfWithEoc) {
                c = ';';    //Change to "end of command" character
            }

            //If last character was also an "End of Command" character, ignore this one
            if (_lastCharWasEOF == true) {
                MXH_DEBUG_INFO("\r\nMultiple EOC");
                return false;   //Nothing added
            }

            _lastCharWasEOF = true; //Remember this was an "End of Command" character

            //Current command is now finished, so reset _dontSaveCurrentCommand
            if (_dontSaveCurrentCommand == true) {
                _dontSaveCurrentCommand = false;
                return false;   //Nothing added
            }

            //Add pointer to "end of command" character
            cmdEndsBuf.put(_head);
            //End of command character will be added to buffer below at current _head pointer
            MXH_DEBUG_INFO("\r\nAdded Cmd, EOC=%d", _head);
        }
        else {
            _lastCharWasEOF = false;
        }

        if (_dontSaveCurrentCommand) {
            return false;
        }

        //Add byte to buffer
        _pool[_head++] = c;
        _head %= BufferSize;
        if (_head == _tail) {
            _full = true;
        }

        return true;
    }

    /** Adds given NULL terminated string to the buffer. This function checks for the "End of Command" character,
     * and if found, increments the "command count" register indicating how many commands this buffer has.
     *
     * If the buffer becomes full before the "End of Command" character is reached, all bytes added for current
     * command are removed. And, all following bytes added until the next "End of Command" character will be ignored.
     *
     * The checkBufferFullError() function can be used to check if this function caused an error, and command was
     * not added to buffer.
     *
     * @param buf Source buffer containing array to add to buffer
     * @param bufSize Size of array to add to buffer
     *
     * @return Returns true if something added to buffer, else false. Important to note that all characters added to
     *      current command CAN BE REMOVED if buffer gets full before "End of Command" added to buffer!
     */
    bool put(const char* str) {
        bool retVal = 0;

        //DO NOT DO this check here! This check MUST be done by put() function, because if buffer becomes full before
        //"End of Command" character received, the put() function will remove all current command characters added!
        //if (isFull() == true) {
        //    return false;
        //}

        //Add whole string to buffer. DO NOT check isFull() in next line, MUST be checked in put() below!!!!
        while((*str != 0) /*&& (isFull()==false)*/) {
            retVal = retVal | put((uint8_t)(*str++));
        }
        return retVal;
    }


    /** Adds given array to the buffer. This function checks for the "End of Command" character,
     * and if found, increments the "command count" register indicating how many commands this buffer has.
     *
     * If the buffer becomes full before the "End of Command" character is reached, all bytes added for current
     * command are removed. And, all following bytes added until the next "End of Command" character will be ignored.
     *
     * The checkBufferFullError() function can be used to check if this function caused an error, and command was
     * not added to buffer.

     * @param buf Source buffer containing array to add to buffer
     * @param bufSize Size of array to add to buffer
     *
     * @return Returns true if something added to buffer, else false. Important to note that all characters added to
     *      current command CAN BE REMOVED if buffer gets full before "End of Command" added to buffer!
     */
    bool putArray(uint8_t* buf, uint16_t bufSize) {
        bool retVal = 0;
        int i;

        //DO NOT DO this check here! This check MUST be done by put() function, because if buffer becomes full before
        //"End of Command" character received, the put() function will remove all current command characters added!
        //if (getFree() < bufSize) {
        //    return 0;
        //}

        for(i=0; i<bufSize; i++) {
            retVal = retVal | put(buf[i]);
        }
        return retVal;
    }

    /** Gets and object from the buffer. Ensure buffer is NOT empty before calling this function!
     *
     * @return Read data
     */
    uint8_t get() {
        if (!isEmpty()) {
            uint8_t retData;
            retData = _pool[_tail++];
            _tail %= BufferSize;
            _full = false;
            return retData;
        }
        return 0;
    }

    /** Gets and object from the buffer. Returns true if OK, else false
     *
     * @param data Variable to put read data into
     * @return True if the buffer is not empty and data contains a transaction, false otherwise
     */
    bool getAndCheck(uint8_t& data) {
        if (!isEmpty()) {
            data = _pool[_tail++];
            _tail %= BufferSize;
            _full = false;
            return true;
        }
        return false;
    }


    /** Gets and object from the buffer, but do NOT remove it. Ensure buffer is NOT empty before calling
     * this function! If buffer is empty, will return an undefined value.
     *
     * @return Read data
     */
    uint8_t peek() {
        return _pool[_tail];
    }

    /** Gets an object from the buffer at given offset, but do NOT remove it. Given offset is a value from
     * 0 to n. Ensure buffer has as many objects as the offset requested! For example, if buffer has 5 objects
     * available, given offset can be a value from 0 to 4.
     *
     * @param offset Offset of requested object. A value from 0-n, where (n+1) = available objects = getAvailable()
     * @return Object at given offset
     */
    uint8_t peekAt(CounterType offset) {
        return _pool[(offset+_tail)%BufferSize];
    }

    /** Gets the last object added to the buffer, but do NOT remove it.
     *
     * @return Object at given offset
     */
    uint8_t peekLastAdded() {
        return _pool[(_head-1)%BufferSize];
    }

    /** Gets and array of given size, and write it to given buffer.
     * Nothing is removed from buffer
     *
     * @param buf Destination buffer to array to
     * @param bufSize Maximum size to write to destination buffer
     * @param lenReq Requested length
     *
     *
     * @return Number of bytes written to given buffer
     */
    uint16_t peekArray(uint8_t* buf, uint16_t bufSize, CounterType lenReq) {
        uint16_t lenWritten=0;
        CounterType currTail;
        currTail = _tail;

        //Some checks
        if (isEmpty() || (bufSize==0) || (lenReq==0)) {
            return 0;
        }

        do  {
            buf[lenWritten++] = _pool[currTail];
            currTail = ((currTail+1)%BufferSize);
        } while((lenWritten<bufSize) && (lenWritten<lenReq));


        return lenWritten;
    }

    /** Check if the buffer has a complete command
     *
     * @return True if the buffer has a command, false if not
     */
    bool hasCommand() {
        return !cmdEndsBuf.isEmpty();
    }

    /** Get length of next command in buffer, excluding "end of command" character! For example,
     * the command "r=100;" will return 5.
     * @return Length of next command in buffer
     */
    uint8_t getCommandLength() {
        CounterType offsetEOC;

        if (cmdEndsBuf.isEmpty()) {
            return 0;
        }

        //Get offset of "end of command" character of current command in buffer
        offsetEOC = cmdEndsBuf.peek();

        return ((offsetEOC-_tail) % BufferSize);
    }

    /** Get number of commands waiting in buffer
     * @return Number of commands waiting in buffer
     */
    uint8_t getCommandsAvailable() {
        return cmdEndsBuf.getAvailable();
    }

    /** If current command has "name=value" format, the 'name' part is returned.
     * Else, the whole command is returned(excluding a possible trailing '=' character).
     * Returned string is NULL terminated by default.
     * Nothing is removed from the buffer!
     *
     * If true is returned in the "isNameValue" parameter, this is a "name=value" command, with a name part of at
     * lease 1 character long. The offset of the 'value' part will be the returned
     *
     * If false is returned in the "isNameValue" parameter, there is no 'value' part of at least 1 character. There could however
     * still be a '=' character with no 'value' following it. Any possible trailing '=' character is removed string returned in 'buf'.
     *
     * @param buf Destination buffer to write string to
     *
     * @param bufSize Maximum size to write to destination buffer
     *
     * @param isNameValue Returns true if a '=' character was found -AND- at least 1 char following it. This indicates
     *        this is a "name=value" command, and has a value part(of at least 1 character) following the '='.
     *        First character of 'value' is value returned by this function + 1!
     *
     * @return Size in bytes of string returned in 'buf' parameter(is 'name' if isNameValue=true).
     *         If true is returned in 'isNameValue', 'buf' contains the 'name' part of a 'name=value' command. This returned offset points to '='.
     *         If false is returned in 'isNameValue', 'buf' contains the whole command string(excluding possible trailing '=' character).
     */
    uint16_t getCommandName(uint8_t* buf, uint16_t bufSize, bool& isNameValue, bool nullTerminate = true) {
        CounterType offsetEOC;
        CounterType currTail;
        uint16_t nameLen = 0;
        isNameValue=false;

        //Some checks
        if (cmdEndsBuf.isEmpty() || (bufSize==0) || (_pool[_tail]=='=')) {
            return 0;
        }
        offsetEOC = cmdEndsBuf.peek();
        currTail = _tail;

        //Copy current command string to given buffer, until '=' reached
        do {
            buf[nameLen++] = _pool[currTail];
            currTail = ((currTail+1)%BufferSize);
            //If next character is '='
            if(_pool[currTail] == '=') {
                //Check at least 1 character following '='
                //Get pointer of character following '=', and ensure it is not "end of command"(offsetEOC). Meaning there is
                //still at least 1 more character following '='
                if(((currTail+1)%BufferSize) != offsetEOC) {
                    isNameValue = true;
                }
                break;
            }
        } while((currTail!=offsetEOC) && (nameLen<bufSize));


        if (nullTerminate) {
            if(nameLen<bufSize) {
                buf[nameLen] = 0;
            }
            buf[bufSize-1] = 0; //Null terminate last position of buffer in case it filled up
        }

        return nameLen;
    }

    /** For commands with "name=value" format, returns the 'value' part.
     * Returned string is NULL terminated by default.
     * Nothing is removed from the buffer!
     *
     * To optimize this function, call getCommandName() before calling this function. The getCommandName() return value(+1)
     * can be used as 'offset' parameter to this function. This will save this function from searching for '=' character.
     *
     *
     * @param buf Destination buffer to write string to
     * @param bufSize Maximum size to write to destination buffer
     * @param offset Gives offset of value string if known. This can be value(+1) returned by
     *        getCommandName() function. Use -1 if unknown.
     *
     * @return Size in bytes of returned string
     */
    uint16_t getCommandValue(uint8_t* buf, uint16_t bufSize, uint16_t offset = -1, bool nullTerminate = true) {
        CounterType offsetEOC;
        CounterType currTail;
        uint16_t valueLen = 0;
        bool foundEq=false;     //'=' character found

        //Some checks
        if (cmdEndsBuf.isEmpty() || (bufSize==0)) {
            return 0;
        }
        offsetEOC = cmdEndsBuf.peek();  //offset of "end of command" character

        currTail = _tail;

        //If offset was given, it will point to first character of value string
        if (offset != -1) {
            //Check offset point somewhere inside current command! Where ((offsetEOC-_tail) % BufferSize) = command length
            if (offset < ((offsetEOC-_tail)%BufferSize)) {
                currTail = ((currTail + offset)%BufferSize);    //Add offset to tail

                //If given offset was for first character of 'value', it will NOT point to '='. It will be next character.
                if(_pool[currTail] != '=') {
                    //Points to character after '=', indicate '=' has already been found.
                    foundEq=true;
                }
                //ELSE, assume offset for for '=' character. It will be found in step below
            }
            else {
                //currTail = _tail; //Ignore given offset, and search whole command for '='
                MXH_DEBUG("\r\ngetCommandValue() Offset error!");
            }
        }

        do {
            if (foundEq) {
                buf[valueLen++] = _pool[currTail];
            }
            else {
                if(_pool[currTail] == '=') {
                    foundEq=true;
                }
            }
            currTail = ((currTail+1)%BufferSize);
        } while((currTail != offsetEOC) && (valueLen<bufSize));

        if (nullTerminate) {
            if(valueLen<bufSize) {
                buf[valueLen] = 0;
            }
            buf[bufSize-1] = 0; //Null terminate last position of buffer in case it filled up
        }

        return valueLen;
    }

    /** Search current command for given character
     *
     * @param offset Returns offset of found character. If NULL, this parameter is not used.
     *
     * @return Returns true if found, else false
     */
    bool search(uint8_t c, uint16_t* offsetFound) {
        CounterType offsetEOF;
        CounterType currTail;

        //Get command length
        if (cmdEndsBuf.isEmpty()) {
            return false;
        }
        offsetEOF = cmdEndsBuf.peek();
        currTail = _tail;
        do {
            if(_pool[currTail] == c) {
                if (offsetFound!=NULL) {
                    *offsetFound = (uint16_t)currTail;
                }
                return true;
            }
            currTail = ((currTail+1)%BufferSize);
        } while(currTail != offsetEOF);

//        CounterType i;
//        CounterType cmdLen;
//        cmdLen = (offsetEOF-_tail) % BufferSize;
//        for (i=0; i<cmdLen; i++) {
//            if(_pool[(i+_tail)%BufferSize] == c) {
//            //if(peekAt(i)==c) {
//                offsetFound = (uint16_t)i;
//                return true;
//            }
//        }
        return false;
    }

    /** Check if the buffer is empty
     *
     * @return True if the buffer is empty, false if not
     */
    bool isEmpty() {
        return (_head == _tail) && !_full;
    }

    /** Check if the buffer is full
     *
     * @return True if the buffer is full, false if not
     */
    bool isFull() {
        return _full;
    }

    /** Get number of available bytes in buffer
     * @return Number of available bytes in buffer
     */
    CounterType getAvailable() {
        if (_head != _tail) {
            CounterType avail;
            avail = _head - _tail;
            avail %= BufferSize;
            return avail;
        }

        //Head=Tail. Can be full or empty
        if (_full==false) {
            return 0;
        }
        else {
            return BufferSize;
        }
    }


    /** Get number of free bytes in buffer available for writing data to.
     * @return Number of free bytes in buffer available for writing data to.
     */
    CounterType getFree() {
        CounterType free;
        //Full
        if (_full==true) {
            return 0;
        }
        //Empty
        if(_head == _tail) {
            return BufferSize;
        }
        free = _tail - _head;
        free %= BufferSize;
        return free;
    }

    /** Replaces any LF or CR characters with an "End of Command" character = ';'
     */
    void enableReplaceCrLfWithEoc() {
        flags.bits.replaceCrLfWithEoc = true;
    }

    /** Do NOT replaces LF and CR characters with an "End of Command" character = ';'
     */
    void disableReplaceCrLfWithEoc() {
        flags.bits.replaceCrLfWithEoc = false;
    }

    /** Check if an overwrite error occurred. It resets the error flag if it was set
     * @return True if overwrite error occurred since last time this function was called, else false
     */
    bool checkBufferFullError() {
        bool retVal = _errBufFull;
        _errBufFull = false;
        return retVal;
    }

    /** Reset the buffer
     */
    void reset() {
        _head = 0;
        _tail = 0;
        _full = false;
        _errBufFull = false;
        _dontSaveCurrentCommand = false;
    }

    /** Remove a command from buffer
     */
    void removeCommand() {
        if (cmdEndsBuf.isEmpty()) {
            return;
        }

        //MXH_DEBUG("\r\nRemoving Cmd");

        //Set tail = "end of command" character + 1. This is first character of next command
        _tail = (cmdEndsBuf.get()+1) % BufferSize;
        _full = false;
    }

private:
    uint8_t _pool[BufferSize];
    volatile CounterType _head;
    volatile CounterType _tail;
    volatile bool _full;
    volatile bool _errBufFull;
    volatile bool _dontSaveCurrentCommand;
    volatile bool _lastCharWasEOF;

    union Flags {
        struct {
            uint8_t replaceCrLfWithEoc : 1;         //Replace CR and LF with "End of Command" character = ';'
        } bits;
        uint8_t     Val;
        //Union constructor. Used in initialization list of this class.
        Flags(uint8_t v) : Val(v) {}
    } flags;

public:
    //Buffer for storing "end of command" locations
    MxCircularBuffer <uint16_t, Commands, CounterType> cmdEndsBuf;
    //MxCircularBuffer <uint16_t, Commands, uint16_t> cmdEndsBuf;   //Creates larger code
};

#if defined(MXH_DEBUG)
    #undef MXH_DEBUG
#endif
#if defined(MXH_DEBUG_INFO)
    #undef MXH_DEBUG_INFO
#endif


#endif /* SRC_MX_CMD_BUFFER_H_ */
