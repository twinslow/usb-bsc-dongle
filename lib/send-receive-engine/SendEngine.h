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
        void clearBuffer(void);
        void startSending();
        void stopSending();
        void stopSendingOnIdle();
        void waitForSendIdle();
        uint8_t *getRxdPort();
        uint8_t getRxdBitMask();
        uint8_t getRxdBitNotMask();
        uint8_t getBitBuffer();
        uint8_t getBitBufferLength();
        int getRemainingDataToBeSend(void);
        uint8_t _savedBitBuffer;
        DataBuffer getDataBuffer(void);
        uint8_t lastBitSent;

    private:
        DataBuffer           _sendDataBuffer;
        uint8_t              _sendBitBuffer;
        uint8_t              _sendBitBufferLength;
        volatile uint8_t     _stopOnIdle;

        volatile uint8_t *_RXD_PORT;
        uint8_t           _RXD_BIT;
        uint8_t           _RXD_BITMASK;
        int               _dataByte1, _dataByte2;

        int addOutputByte(uint8_t data);
};

#endif
