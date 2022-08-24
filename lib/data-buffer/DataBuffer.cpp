#include <Arduino.h>

#include "DataBuffer.h"

DataBuffer::DataBuffer() {
    _complete = 0;
    _len = 0;
    _readPos = 0;
}

void DataBuffer::clear()
{
    _complete = 0;
    _len = 0;
    _readPos = 0;
}

int DataBuffer::read(uint8_t *data)
{
    _readPos;
    if ( _len == 0 || _readPos >= _len )
        return -1;     // Fail, buffer empty or reached end.

    *data = _buff[_readPos];
    return _readPos++;      // Note deliberate post increment.
}

int DataBuffer::readLast(void)
{
    if ( _len == 0 )
        return -1;     // Fail, buffer empty.

    return _buff[_len - 1];  // Return the last byte written
}

int DataBuffer::get(int idx)
{
    if ( idx >= _len )
        return -1;     // Fail, buffer empty / beyond end.

    return _buff[idx];  // Return the byte at idx
}

int DataBuffer::getLength(void) {
    return _len;
}

int DataBuffer::getPos(void) {
    return _readPos;
}

int DataBuffer::write(uint8_t data)
{
    if ( _len == DATABUFF_MAX_DATA - 1 )
        return -1;     // Fail, buffer full.

    _buff[_len] = data;
    return ++(_len);
}

int DataBuffer::setComplete() {
    // Set the complete flag, providing we have data in the buffer.
    if ( _len > 0 )
        _complete = 1;
    return _complete;
}

int DataBuffer::isComplete() {
    return _complete;
}