#include <Arduino.h>
#include "ReceiveEngine.h"
#include "bsc_protocol.h"

ReceiveEngine::ReceiveEngine(uint8_t txdPin, uint8_t ctsPin) {

    _TXD_PORT       = portInputRegister(digitalPinToPort(txdPin));
    _TXD_BIT        = digitalPinToBitMask(txdPin);
    _TXD_BITMASK    = ~digitalPinToBitMask(txdPin);

    receiveState = RECEIVE_STATE_OUT_OF_SYNC;

    _inCharSync = false;
    _previousByteDLE = false;
    _frameComplete = false;
    _receiveBitCounter = 0;
    _ctsPin = ctsPin;

    _inputBitBuffer = 0;        // This is not really required, however it is useful for
                                // unit testing that we clear it initially.

}

uint8_t *ReceiveEngine::getTxdPort(void)
{
    return _TXD_PORT;
}

uint8_t ReceiveEngine::getTxdBitMask(void) {
    return _TXD_BIT;
}

uint8_t ReceiveEngine::getTxdBitNotMask(void) {
    return _TXD_BITMASK;
}

uint8_t ReceiveEngine::getBitBuffer(void) {
    return _inputBitBuffer;
}

uint8_t ReceiveEngine::getCtsPin(void) {
    return _ctsPin;
}

uint8_t ReceiveEngine::getInCharSync(void) {
    return _inCharSync;
}

void ReceiveEngine::setBitBuffer(uint8_t data) {
    _inputBitBuffer = data;
    _receiveBitCounter = 8;
}

DataBuffer * ReceiveEngine::getDataBuffer(void) {
    return &_receiveDataBuffer;
}

bool ReceiveEngine::isFrameComplete(void) {
    return _frameComplete;
}

/*
 * Messages (responses) from the tribuatary station could be this ...
 *
 * SYN SYN ACK0|1 PAD
 * SYN SYN NAK PAD
 * SYN SYN SOH % R STX CU CU DV DV SS0 SS1 ETX BCC1 BCC2 PAD      -- Status message
 * SYN SYN SOH % / STX TEXT... ETX|ETB BCC1 BCC2  PAD              -- Test message
 * SYN SYN STX CU CU DV DV TEXT... ETX|ETB BCC1 BCC2 PAD          -- Read modified response
 * SYN SYN DLE STX CU CU DV DV TEXT... DLE ETX|ETB BCC1 BCC2 PAD  -- Read partition
 * RVI
 * WACK
 *
 * EBCDIC % = 0x6C
 * EBCDIC R = 0xD9
 * EBCDIC / = 0x61
 */

// This routine is timing critical. It needs to be invoked before de-asserting the DTE-transmit
// clock line. The processBit() routine can be invoked afterwards and is not timing critical.
void ReceiveEngine::getBit(uint8_t inputBit) {
    inputBit &= 0x01;
    _inputBitBuffer <<= 1; // Shift to left
    _inputBitBuffer |= inputBit;   // Setting most significant bit when line was high.
    _receiveBitCounter++;
}

// This function is for testing ... routine is timing critical. It needs to be invoked before de-asserting the DTE-transmit
// clock line. The processBit() routine can be invoked afterwards and is not timing critical.
void ReceiveEngine::getBit(void) {
    uint8_t inputBit;
    // Read value from port
    inputBit = (*_TXD_PORT & _TXD_BIT) ? 0x01 : 0x00;
    // Save in buffer
    getBit(inputBit);
}

void ReceiveEngine::processBit(void) {

    // When we are out of sync (no char/byte sync on the line ...
    if ( receiveState == RECEIVE_STATE_OUT_OF_SYNC ) {

        // When are out of sync then we constantly look for the sync bit pattern.
        if ( _inputBitBuffer == BSC_CONTROL_SYN ) {
            _inCharSync = true;
            receiveState = RECEIVE_STATE_IDLE;
            _receiveDataBuffer.write(_inputBitBuffer);
            _receiveBitCounter = 0;
        }
        // We are done
        return;
    }

    // We must be in character sync on the line. So, we only need to look
    // at complete bytes ... e.g. when _receiveBitCounter == 8.
    if ( _receiveBitCounter != 8 )
        return;


    _latestByte = _inputBitBuffer;
    _receiveBitCounter = 0;

    if ( receiveState == RECEIVE_STATE_IDLE ||
         receiveState == RECEIVE_STATE_DATA ) {

        // If latest character is a SYNC/IDLE then just discard.
        if ( _latestByte == BSC_CONTROL_SYN )
            return;

        // If latest character is a DLE then set the flag for next time.
        if ( _latestByte == BSC_CONTROL_DLE ) {
            _previousByteDLE = true;
            return;
        }

        // if latest character STX and previous character was DLE then we are going
        // into transparent mode.
        if ( _previousByteDLE && _latestByte == BSC_CONTROL_STX ) {
            receiveState = RECEIVE_STATE_TRANSPARENT_DATA;
            _receiveDataBuffer.write(BSC_CONTROL_DLE);
            _receiveDataBuffer.write(BSC_CONTROL_STX);
            _previousByteDLE = false;
            return;
        }

        if ( _latestByte == BSC_CONTROL_STX ||
             _latestByte == BSC_CONTROL_SOH ) {
             receiveState = RECEIVE_STATE_DATA;
             _receiveDataBuffer.write(_latestByte);
            _previousByteDLE = false;
             return;
        }


    }

    if ( receiveState == RECEIVE_STATE_IDLE ) {

        if ( _previousByteDLE &&
             ( _latestByte == BSC_CONTROL_ACK0 || _latestByte == BSC_CONTROL_ACK1 ) ) {
            // We got a SYN DLE ACK0 or SYN DLE ACK1 sequence
            _receiveDataBuffer.write(BSC_CONTROL_DLE);
            _receiveDataBuffer.write(_latestByte);
            _frameComplete = true;
            return;
        }

        _previousByteDLE = false;

        if ( _latestByte == BSC_CONTROL_SYN &&
             _receiveDataBuffer.readLast() == BSC_CONTROL_SYN ) {
            // If we a SYN character, following a prior SYN then we can just discard it.
            // character in buffer.
            return;
        }

        if ( _latestByte == BSC_CONTROL_EOT ||
             _latestByte == BSC_CONTROL_NAK ) {
            // We got a SYN EOT or SYN NAK sequence
            _receiveDataBuffer.write(_latestByte);
            _frameComplete = true;
            return;
        }
    }

    if ( receiveState == RECEIVE_STATE_PAD ) {
        _receiveDataBuffer.write(_latestByte);
        receiveState = RECEIVE_STATE_IDLE;
        return;
    }

    if ( receiveState == RECEIVE_STATE_DATA ) {

        if ( _latestByte == BSC_CONTROL_SYN ) {
            // Ignore SYN chars sent as an idle character during data transmission.
            return;
        }

        // Do we have a character terminating the block...
        if ( _latestByte == BSC_CONTROL_ETB ||
             _latestByte == BSC_CONTROL_ETX ) {
            _receiveDataBuffer.write(_latestByte);
            receiveState = RECEIVE_STATE_BCC1;
            return;
        }

        // Must be data ...
        _receiveDataBuffer.write(_latestByte);
        return;
    }

    if ( receiveState == RECEIVE_STATE_BCC1 ) {
        _receiveDataBuffer.write(_latestByte);
        receiveState = RECEIVE_STATE_BCC2;
        return;
    }

    if ( receiveState == RECEIVE_STATE_BCC2 ) {
        _receiveDataBuffer.write(_latestByte);
        receiveState = RECEIVE_STATE_PAD;
        _frameComplete = true;
        return;
    }

    if ( receiveState == RECEIVE_STATE_TRANSPARENT_DATA ) {

        // If we received two DLE's sequence, then we add the second
        // DLE to the buffer, discarding the first.
        if ( _previousByteDLE && _latestByte == BSC_CONTROL_DLE ) {
            _receiveDataBuffer.write(BSC_CONTROL_DLE);
            _previousByteDLE = false;
            return;
        }

        if ( _previousByteDLE && _latestByte == BSC_CONTROL_ENQ ) {
            _receiveDataBuffer.write(BSC_CONTROL_DLE);
            _receiveDataBuffer.write(_latestByte);
            _frameComplete = true;
            _previousByteDLE = false;
            return;
        }

        if ( _previousByteDLE && (
              _latestByte == BSC_CONTROL_ITB ||
              _latestByte == BSC_CONTROL_ETX ||
              _latestByte == BSC_CONTROL_ETB ) ) {
            _receiveDataBuffer.write(BSC_CONTROL_DLE);
            _receiveDataBuffer.write(_latestByte);
            receiveState = RECEIVE_STATE_BCC1;
            return;
        }

        // If latest character is a DLE then set the flag for next time.
        if ( _latestByte == BSC_CONTROL_DLE ) {
            _previousByteDLE = true;
            return;
        }

        // Otherwise ... just add this byte to the receive buffer.
        _receiveDataBuffer.write(_latestByte);
        return;
    }

}

void ReceiveEngine::startReceiving() {
    digitalWrite(_ctsPin, LOW);
    _inCharSync = false;
    receiveState = RECEIVE_STATE_OUT_OF_SYNC;
}

void ReceiveEngine::waitReceivedFrameComplete() {
    while ( !_frameComplete );
}

int ReceiveEngine::getFrameLength() {
    return _receiveDataBuffer.getLength();
}

int ReceiveEngine::getFrameDataByte(int idx) {
    return _receiveDataBuffer.get(idx);
}

/*
#define SEND_STATE_IDLE               1
#define SEND_STATE_XMIT               2
#define SEND_STATE_TRANSPARENT_XMIT   3
#define SEND_STATE_BCC1               4
#define SEND_STATE_BCC2               5

#define SEND_STATE_EOT                10





uint8_t SendEngine::xmitStateMachine(int data1, int data2) {


    switch(xmitState) {
        case SEND_STATE_IDLE:
            if ( data1 == BSC_CONTROL_SYN && data2 == BSC_CONTROL_EOT ) {
                xmitState = SEND_STATE_EOT;
                return xmitState;
            } else if ( data1 == BSC_CONTROL_SYN &&
                 ( data2 == BSC_CONTROL_SOH || data2 == BSC_CONTROL_STX ) ) {
                xmitState = SEND_STATE_XMIT;
                return xmitState;
            } else if ( data1 == BSC_CONTROL_DLE && data2 == BSC_CONTROL_STX ) {
                xmitState = SEND_STATE_TRANSPARENT_XMIT;
                return xmitState;
            }
            break;

        case SEND_STATE_XMIT:
            if ( data1 == BSC_CONTROL_DLE && data2 == BSC_CONTROL_STX ) {
                xmitState = SEND_STATE_TRANSPARENT_XMIT;
                return xmitState;
            } else if ( data2 == BSC_CONTROL_ENQ ) {
                xmitState = SEND_STATE_IDLE;       // Transmission is being terminated.
                                                  // Receiving station should send NAK
                return xmitState;
            } else if ( data2 == BSC_CONTROL_ETX ||
                      data2 == BSC_CONTROL_ETB ) {
                xmitState = SEND_STATE_BCC1;
                return xmitState;
            }
            break;

        case SEND_STATE_TRANSPARENT_XMIT:
            if ( data1 == BSC_CONTROL_DLE ) {
                if ( data2 == BSC_CONTROL_ENQ ) {
                    xmitState = SEND_STATE_IDLE;
                    return xmitState;
                } else if ( data2 == BSC_CONTROL_ETB ||
                            data2 == BSC_CONTROL_ITB ||
                            data2 == BSC_CONTROL_ETX ) {
                    xmitState = SEND_STATE_BCC1;
                    return xmitState;
                }
            }
            break;

        case SEND_STATE_BCC1:
            xmitState = SEND_STATE_BCC2;
            return xmitState;

        case SEND_STATE_BCC2:
            xmitState = SEND_STATE_IDLE;
            return xmitState;

        default:
            break;
    }

    return 0;     // state has not changed.
}



 */
