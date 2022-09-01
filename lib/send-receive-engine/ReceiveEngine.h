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
#define RECEIVE_STATE_PAD               6



class ReceiveEngine {
    public:
        ReceiveEngine(uint8_t txdPin, uint8_t ctsPin);    // Transmit pin of the DTE ... our receive pin.
        ~ReceiveEngine(void);
        void getBit(uint8_t val);
        void getBit(void);
        void setBit(uint8_t bit);
        void processBit(void);
        void startReceiving(void);
        void stopReceiving(void);
        int waitReceivedFrameComplete(int timeoutMs);
        int getFrameLength(void);
        int getFrameDataByte(int idx);
        uint8_t *getTxdPort(void);
        uint8_t getTxdBitMask(void);
        uint8_t getTxdBitNotMask(void);
        uint8_t getBitBuffer(void);
        uint8_t getCtsPin(void);
        void setBitBuffer(uint8_t data);
        uint8_t getInCharSync(void);
        uint8_t receiveState;
        DataBuffer * getDataBuffer(void);
        bool isFrameComplete(void);
        DataBufferReadOnly * getSavedFrame(void);

    private:
        DataBuffer           _receiveDataBuffer;
        uint8_t              _inputBitBuffer;
        uint8_t              _receiveBitCounter;
        uint8_t              _latestByte;
        uint8_t              _previousByteDLE;
        volatile uint8_t     _frameComplete;
        volatile uint8_t     _inCharSync;
        DataBufferReadOnly * _savedFrame;
        inline void                 frameComplete(void);

        volatile uint8_t *_TXD_PORT;
        uint8_t           _TXD_BIT;
        uint8_t           _TXD_BITMASK;
        uint8_t           _ctsPin;


};

#endif
