#ifndef ReceiveEngine_h
#define ReceiveEngine_h

#include <Arduino.h>
#include "DataBuffer.h"

#define RECEIVE_STATE_IDLE    0
#define RECEIVE_STATE_DATA    1

class ReceiveEngine {
    public:
        ReceiveEngine(uint8_t txdPin);
        void getBit(void);  

    private:
        DataBuffer  _receiveDataBuffer();
        uint8_t     _inputBitBuffer;

        volatile uint8_t *_TXD_PORT;
        uint8_t           _TXD_BIT;
        uint8_t           _TXD_BITMASK;
        
        uint8_t     _receiveState;
};

#endif
