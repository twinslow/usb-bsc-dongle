#ifndef _DataBuffer_h
#define _DataBuffer_h 1

#include <Arduino.h>

#define DATABUFF_MAX_DATA   300

class DataBufferReadOnly {
    public:
        DataBufferReadOnly(int size);
        DataBufferReadOnly(const DataBufferReadOnly & dbsrc);
        ~DataBufferReadOnly();
        void loadData(uint8_t * data);
        void loadData(int size, uint8_t * data);
        int read(uint8_t *data);
        int get(int idx);
        int getPos();
        int getLength();
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
        void clear(void);
        int readLast(void);
        int write(uint8_t data);
        int setComplete();
        int isComplete();
        DataBufferReadOnly * newReadOnlyCopy(void);

    private:
        volatile uint8_t _complete;
};

#endif