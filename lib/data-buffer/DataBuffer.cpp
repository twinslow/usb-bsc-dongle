#include <Arduino.h>

#include "DataBuffer.h"

void DataBufferReadOnly::loadData(uint8_t * data) {
#ifdef DATA_BUFFER_DEBUG
    Serial.println("DataBufferReadOnly.loadData(data) started");
#endif

    if ( _allocSize > 0 )
        memcpy((void *)_buff, data, _allocSize);
    _len = _allocSize;
    _readPos = 0;
}

void DataBufferReadOnly::loadData(int size, uint8_t * data) {
#ifdef DATA_BUFFER_DEBUG
    Serial.println("DataBufferReadOnly.loadData(size, data) started");
#endif
    _len = min(size, _allocSize);
    if ( _allocSize > 0 )
        memcpy((void *)_buff, data, min(size, _allocSize));
    _readPos = 0;
}

int DataBufferReadOnly::read(uint8_t *data)
{
    if ( _len == 0 || _readPos >= _len )
        return -1;     // Fail, buffer empty or reached end.

    *data = _buff[_readPos];
    return _readPos++;      // Note deliberate post increment.
}

int DataBufferReadOnly::get(int idx)
{
    if ( idx >= _len )
        return -1;     // Fail, buffer empty / beyond end.

    return _buff[idx];  // Return the byte at idx
}

int DataBufferReadOnly::getLength(void) {
    return _len;
}

int DataBufferReadOnly::getPos(void) {
    return _readPos;
}

void * DataBufferReadOnly::getData(void) {
    return (void *)_buff;
}

//-----------------------------------------------------------------------------------

DataBuffer::DataBuffer() : DataBufferReadOnly(DATABUFF_MAX_DATA) {
#ifdef DATA_BUFFER_DEBUG
    Serial.println("DataBuffer ... constructor called");
#endif
    _complete = 0;
}

DataBuffer::DataBuffer(const DataBuffer & dbsrc) : DataBufferReadOnly(dbsrc) {
#ifdef DATA_BUFFER_DEBUG
    Serial.println("DataBuffer ... copy constructor called");
#endif
    _complete = 0;
    this->loadData(dbsrc._len, (uint8_t *)dbsrc._buff);
}

// DataBuffer::DataBuffer & operator=(const DataBuffer & dbsrc)
//     _complete = 0;
// }

DataBuffer::~DataBuffer() {
#ifdef DATA_BUFFER_DEBUG
    Serial.println("DataBuffer ... destructor called");
#endif
}



int DataBuffer::readLast(void)
{
    if ( _len == 0 )
        return -1;     // Fail, buffer empty.

    return _buff[_len - 1];  // Return the last byte written
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