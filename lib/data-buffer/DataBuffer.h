#ifndef _DataBuffer_h
#define _DataBuffer_h 1

#include <Arduino.h>

#define DATABUFF_MAX_DATA   300

class DataBuffer {
    public:
        DataBuffer(void);
        void clear(void);
        int read(uint8_t *data);
        int readLast(void);
        int get(int idx);
        int getLength();
        int write(uint8_t data);
        int setComplete();
        int isComplete();

    private:
        volatile int _readPos;
        volatile int _len;
        volatile uint8_t _buff[DATABUFF_MAX_DATA];
        volatile uint8_t _complete;
};

#endif