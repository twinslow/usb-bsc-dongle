#ifndef _DataBuffer_h
#define _DataBuffer_h 1

//#define DATA_BUFFER_DEBUG

#include <Arduino.h>

#define DATABUFF_MAX_DATA   300

class DataBufferReadOnly {
    public:
        inline DataBufferReadOnly() {
            _readPos = 0;
            _allocSize = 0;
            _len = 0;
        }

        inline DataBufferReadOnly(int size) {
            _readPos = 0;
            _allocSize = size;
            _len = 0;
            if ( _allocSize > 0 )
                _buff = (uint8_t *)malloc(_allocSize);
        }

        inline DataBufferReadOnly(int size, uint8_t *data) {
            _readPos = 0;
            _allocSize = size;
            _len = size;
            if ( _allocSize > 0 ) {
                _buff = (uint8_t *)malloc(_allocSize);
                memcpy((void *)_buff, data, _allocSize);
            }
        }


        inline DataBufferReadOnly(const DataBufferReadOnly & dbsrc) {
            _readPos = dbsrc._readPos;
            _allocSize = dbsrc._allocSize;
            _len = dbsrc._len;

            if ( _allocSize > 0 ) {
                _buff = (uint8_t *)malloc(_allocSize);
                memcpy((void *)_buff, (void *)dbsrc._buff, _len);
            }
        }

        inline ~DataBufferReadOnly() {
            if ( _allocSize > 0 )
                free((void *)_buff);
        }

        void loadData(uint8_t * data);
        void loadData(int size, uint8_t * data);
        int read(uint8_t *data);
        int get(int idx);
        int getPos();
        int getLength();
        void * getData();

    protected:
        int _allocSize;
        volatile int _readPos;
        volatile int _len;
        volatile uint8_t * _buff;
};

class DataBuffer : public DataBufferReadOnly  {
    public:
        DataBuffer(void);
        DataBuffer(const DataBuffer & dbsrc);
        ~DataBuffer(void);

        inline void clear()
        {
            _complete = 0;
            _len = 0;
            _readPos = 0;
        }

        inline int write(uint8_t data)
        {
            int len = _len;
            if ( len == DATABUFF_MAX_DATA - 1 )
                return -1;     // Fail, buffer full.

            _buff[len++] = data;
            _len = len;
            return (_len);
        }

        inline DataBufferReadOnly * newReadOnlyCopy(void) {
            DataBufferReadOnly * ro;
            ro = new DataBufferReadOnly(_len, (uint8_t *)_buff);
            return ro;
        }

        int readLast(void);
        int setComplete();
        int isComplete();

    private:
        volatile uint8_t _complete;
};

#endif