#include <Arduino.h>
#include "SendEngine.h"
#include "bsc_protocol.h"

// Pins are named with respect to the DTE. As we are implementing a DCE interface, our
// RXD pin is being used to send data (the DTE is receiving on this pin).

SendEngine::SendEngine(uint8_t rxdPin) {
    _RXD_PORT       = portOutputRegister(digitalPinToPort(rxdPin));
    _RXD_BIT        = digitalPinToBitMask(rxdPin); 
    _RXD_BITMASK    = ~digitalPinToBitMask(rxdPin); 

    // Initialize/clear data buffers
    _sendDataBuffer.clear();

    // Load the bit buffer with the idle character
    _sendBitBuffer = BSC_CONTROL_IDLE;
    _sendBitBufferLength = 8; 

    // Initialize the state engine to be idle.
    xmitState = SEND_STATE_IDLE;
}

void SendEngine::sendBit() {

    // If nothing in bit buffer, then fetch character from sendDataBuffer. 
    if ( _sendBitBufferLength == 0 ) {
        if ( _sendDataBuffer.read(&_sendBitBuffer) < 0 ) {
            // There was nothing in the data buffer, so we going to send an
            // idle character.
            _sendBitBuffer = BSC_CONTROL_IDLE;      
        }
        _sendBitBufferLength = 8;
    }
      
    // Set pin high or low.
    if ( _sendBitBuffer & 0x01 )
        *_RXD_PORT |= _RXD_BIT;
    else
        *_RXD_PORT &= _RXD_BITMASK;

    _sendBitBuffer >>= 1;         // Shift bits in send buffer
    _sendBitBufferLength--;        
}

int SendEngine::addOutputByte(uint8_t data) {
    return _sendDataBuffer.write(data);
}

/*
What could be coming in ...

    PAD SYN SYN SOH xx xx STX yy yy yy ETX BCC1 BCC2
    PAD SYN SYN SOH xx xx STX yy yy yy ETB BCC1 BCC2
    PAD SYN SYN STX yy yy yy ETB BCC1 BCC2
    PAD SYN SYN DLE STX yy yy yy DLE ETX BCC1 BCC2
    PAD SYN SYN DLE STX yy yy yy DLE ETB BCC1 BCC2
    
*/

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

int SendEngine::addByte(int data) {
    int newXmitState;
    
    _dataByte2 = data;
    newXmitState = xmitStateMachine(_dataByte1, _dataByte2);
    _dataByte1 = _dataByte2;
    
    if ( xmitState != SEND_STATE_IDLE )
        return addOutputByte( (uint8_t)data );

    return 0;
}
