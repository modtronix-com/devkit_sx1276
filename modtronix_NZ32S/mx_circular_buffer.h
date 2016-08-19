/**
 * File:      mx_circular_buffer.h
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
#ifndef SRC_MX_CIRCULAR_BUFFER_H_
#define SRC_MX_CIRCULAR_BUFFER_H_

#include "nz32s_default_config.h"
#include "mx_buffer_base.h"

/** Templated Circular buffer class
 */
template<typename T, uint32_t BufferSize, typename CounterType = uint32_t>
class MxCircularBuffer : public MxBuffer {
public:
    MxCircularBuffer() : _head(0), _tail(0), _full(false) {
    }

    ~MxCircularBuffer() {
    }


    /** Adds an object to the buffer. If no space in buffer, this function does nothing.
     * do anything!
     * Use getFree() function prior to this function to ensure buffer has enough free space!
     *
     * @param data Data to be pushed to the buffer
     *
     * @return Returns true if character added to buffer, else false
     */
    bool put(const T& data) {
        if (isFull()) {
            return false;
        }
        _pool[_head++] = data;
        _head %= BufferSize;
        if (_head == _tail) {
            _full = true;
        }
        return true;
    }


    /** Adds given array to the buffer. This function checks if buffer has enough space.
     * If not enough space available, nothing is added, and this function returns 0.
     *
     * @param buf Source buffer containing array to add to buffer
     * @param bufSize Size of array to add to buffer
     *
     * @return Returns true if character added to buffer, else false
     */
    bool putArray(uint8_t* buf, uint16_t bufSize) {
        bool retVal = false;
        int i;

        if (getFree() < bufSize) {
            return false;
        }

        for(i=0; i<bufSize; i++) {
            retVal = retVal | put(buf[i]);
        }
        return retVal;
    }


    /** Gets and remove and object from the buffer. Ensure buffer is NOT empty before calling this function!
     *
     * @return Read data
     */
    T get() {
        if (!isEmpty()) {
            T retData;
            retData = _pool[_tail++];
            _tail %= BufferSize;
            _full = false;
            return retData;
        }
        return 0;
    }


    /** Gets and object from the buffer, but do NOT remove it. Ensure buffer is NOT empty before calling
     * this function! If buffer is empty, will return an undefined value.
     *
     * @return Read data
     */
    T peek() {
        return _pool[_tail];
    }


    /** Gets an object from the buffer at given offset, but do NOT remove it. Given offset is a value from
     * 0 to n. Ensure buffer has as many objects as the offset requested! For example, if buffer has 5 objects
     * available, given offset can be a value from 0 to 4.
     *
     * @param offset Offset of requested object. A value from 0-n, where (n+1) = available objects = getAvailable()
     * @return Object at given offset
     */
    T peekAt(CounterType offset) {
        return _pool[(offset+_tail)%BufferSize];
    }


    /** Gets the last object added to the buffer, but do NOT remove it.
     *
     * @return Object at given offset
     */
    T peekLastAdded() {
        return _pool[(_head-1)%BufferSize];
    }


    /** Gets and object from the buffer. Returns true if OK, else false
     *
     * @param data Variable to put read data into
     * @return True if the buffer is not empty and data contains a transaction, false otherwise
     */
    bool getAndCheck(T& data) {
        if (!isEmpty()) {
            data = _pool[_tail++];
            _tail %= BufferSize;
            _full = false;
            return true;
        }
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


    /** Reset the buffer
     */
    void reset() {
        _head = 0;
        _tail = 0;
        _full = false;
    }



private:
    T _pool[BufferSize];
    volatile CounterType _head;
    volatile CounterType _tail;
    volatile bool _full;
};

#endif /* SRC_MX_CIRCULAR_BUFFER_H_ */
