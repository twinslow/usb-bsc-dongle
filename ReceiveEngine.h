#ifndef ReceiveEngine_h
#define ReceiveEngine_h

#include <Arduino.h>
#include "DataBuffer.h"

#define RECEIVE_STATE_OUT_OF_SYNC       0
#define RECEIVE_STATE_IDLE              1
#define RECEIVE_STATE_DATA              2
#define RECEIVE_STATE_TRANSPARENT_DATA  3
#define RECEIVE_STATE_BCC1              4
#define RECEIVE_STATE_BCC2              5



class ReceiveEngine {
    public:
        ReceiveEngine(uint8_t txdPin, uint8_t ctsPin);    // Transmit pin of the DTE ... our receive pin.
        void getBit(void);  
        void processBit(void);
        void startReceiving(void);
        void waitReceivedFrameComplete(void);
        int getFrameLength(void);
        int getFrameDataByte(int idx);

    private:
        DataBuffer           _receiveDataBuffer;
        uint8_t              _inputBitBuffer;
        uint8_t              _receiveBitCounter;
        uint8_t              _latestByte;
        uint8_t              _previousByteDLE;
        volatile uint8_t     _frameComplete;
        volatile uint8_t     _inCharSync;

        volatile uint8_t *_TXD_PORT;
        uint8_t           _TXD_BIT;
        uint8_t           _TXD_BITMASK;
        uint8_t           _ctsPin;
        
        uint8_t     _receiveState;
};

#endif
