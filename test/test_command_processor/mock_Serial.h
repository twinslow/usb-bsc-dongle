#ifndef mock_Serial_h
#define mock_Serial_h

#include <Arduino.h>

class MockSerial_ : public Serial_ {
    public:
        byte   readBuffer[128];
        byte   writeBuffer[128];

        int    readLen, readPtr, writePtr;

        MockSerial_() {
            readLen = 0;
            readPtr = 0;
            writePtr = 0;
        }
        void reset() {
            readLen = 0;
            readPtr = 0;
            writePtr = 0;
        }
        virtual int read(void) {
            if ( readPtr >= readLen )
                return -1;

            return readBuffer[readPtr++];
        }
        virtual size_t write(uint8_t data) {
            if ( writePtr >= 128 )
                return 0;

            writeBuffer[writePtr++] = data;
            return 1;
        }
        virtual size_t write(const uint8_t * data, size_t len) {
            int x, written;
            for (x=0; x<(int)len; x++) {
                written = write(data[x]);
                if ( written < 1 )
                    return x;
            }
            return len;
        }

        void setReadBuffer(byte *data, int len) {
            int x;
            for (x=0; x<len && x<128; x++) {
                readBuffer[x] = data[x];
            }
            readPtr = 0;
            readLen = len;
        }
};

MockSerial_ MockSerial;

#endif


