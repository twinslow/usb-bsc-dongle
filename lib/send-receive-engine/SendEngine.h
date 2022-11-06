#ifndef SendEngine_h
#define SendEngine_h

#include <Arduino.h>
#include "DataBuffer.h"

#include "mcu_abstraction.h"

#define SEND_STATE_OFF                1
#define SEND_STATE_IDLE               2
#define SEND_STATE_XMIT               3

class SendEngine {
    public:
        SendEngine(gpio_pin_t rxdPin);
        virtual ~SendEngine();
        void sendBit(void);
        volatile uint8_t xmitState = SEND_STATE_IDLE;
        int addByte(int data);
        void clearBuffer(void);

        virtual void startSending();
        virtual void stopSending();
        virtual void stopSendingOnIdle();
        virtual void waitForSendIdle();

        pgpio_port_t getRxdPort();
        gpio_port_t getRxdBitMask();
        gpio_port_t getRxdBitNotMask();

        uint8_t getBitBuffer();
        uint8_t getBitBufferLength();
        int getRemainingDataToBeSent(void);
        uint8_t _savedBitBuffer;
        DataBuffer & getDataBuffer(void);
        uint8_t lastBitSent;

    private:
        DataBuffer           _sendDataBuffer;
        uint8_t              _sendBitBuffer;
        uint8_t              _sendBitBufferLength;
        volatile uint8_t     _stopOnIdle;

        volatile pgpio_port_t  _RXD_PORT;
        gpio_port_t            _RXD_BIT;
        gpio_port_t            _RXD_BITMASK;
        int               _dataByte1, _dataByte2;

        int addOutputByte(uint8_t data);
};

#endif
