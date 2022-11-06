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

//#define RECEIVE_ENGINE_DEBUG


class ReceiveEngine {
    public:
        ReceiveEngine(uint8_t txdPin, uint8_t ctsPin);    // Transmit pin of the DTE ... our receive pin.
        virtual ~ReceiveEngine(void);

        // This routine is timing critical. It needs to be invoked before de-asserting the DTE-transmit
        // clock line. The processBit() routine can be invoked afterwards and is not timing critical.
        inline void getBitSet(uint8_t inputBit) {
            _inputBitBuffer >>= 1; // Shift to right
            _inputBitBuffer |= inputBit;   // Setting most significant bit when line was high.
            _receiveBitCounter++;
        }

        // This routine is timing critical. It needs to be invoked before de-asserting the DTE-transmit
        // clock line. The processBit() routine can be invoked afterwards and is not timing critical.
        inline void getBit(void) {
            uint8_t inputBit;
            // Read value from port
            inputBit = (*_TXD_PORT & _TXD_BIT) ? 0x80 : 0x00;
            // Save in buffer
            getBitSet(inputBit);
        }

        // This routine is for testing.
        inline void getBit(uint8_t inputBit) {
            inputBit = inputBit ? 0x80 : 0x00;
            getBitSet(inputBit);
        }

        // void getBit(uint8_t val);
        // void getBit(void);
        void setBit(uint8_t bit);
        void processBit(void);
        virtual void startReceiving(void);
        void stopReceiving(void);
        virtual int waitReceivedFrameComplete(int timeoutMs);
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
        virtual DataBuffer * getSavedFrame(void);
        uint8_t _inputBitBuffer;

    protected:
        // Two data buffers that we alternate for receiving and processing data
        DataBuffer           _dataBuffers[2];
        // The data buffer we are currently using for receiving data
        DataBuffer *         _receiveDataBuffer;
        // The data buffer we have received a complete frame (or poll, ack etc.)
        DataBuffer *         _savedFrame;
        // An indicator as to which data buffer is the current working buffer (pointed
        // to by _receiveDataBuffer).
        uint8_t              _workingDataBuffer = 0;

    private:
        uint8_t              _receiveBitCounter;
        uint8_t              _latestByte;
        uint8_t              _previousByteDLE;
        volatile bool        _frameComplete;
        volatile uint8_t     _inCharSync;
        inline void          frameComplete(void);
        volatile uint32_t *   _TXD_PORT;
        uint8_t              _TXD_BIT;
        uint8_t              _TXD_BITMASK;
        uint8_t              _ctsPin;
};

#endif
