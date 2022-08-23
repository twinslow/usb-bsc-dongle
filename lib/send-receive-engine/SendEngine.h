#ifndef SendEngine_h
#define SendEngine_h

#include <Arduino.h>
#include "DataBuffer.h"

#define SEND_STATE_OFF                1
#define SEND_STATE_IDLE               2
#define SEND_STATE_XMIT               3

class SendEngine {
    public:
        SendEngine(uint8_t rxdPin);
        void sendBit(void);  
        volatile uint8_t xmitState = SEND_STATE_IDLE;
        int addByte(int data);
        void startSending();
        void stopSending();
        void waitForSendIdle();
        
    private:
        DataBuffer  _sendDataBuffer;
        uint8_t     _sendBitBuffer;
        uint8_t     _sendBitBufferLength;

        volatile uint8_t *_RXD_PORT;
        uint8_t           _RXD_BIT;
        uint8_t           _RXD_BITMASK;
        int         _dataByte1, _dataByte2;

        int addOutputByte(uint8_t data);
};

#endif