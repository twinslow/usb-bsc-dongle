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

    _sendBitBufferLength = 0; 

    // Initialize the state engine to be idle.
    xmitState = SEND_STATE_OFF;
}

void SendEngine::sendBit() {

    if ( xmitState == SEND_STATE_OFF ) 
      return;

    // If nothing in bit buffer, then fetch character from sendDataBuffer. 
    if ( _sendBitBufferLength == 0 ) {
        if ( _sendDataBuffer.read(&_sendBitBuffer) < 0 ) {
            // There was nothing in the data buffer, so we going to send an
            // idle character.
            _sendBitBuffer = BSC_CONTROL_IDLE;
            xmitState = SEND_STATE_IDLE;      
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


int SendEngine::addByte(int data) {
    return addOutputByte( (uint8_t)data );
}

void SendEngine::startSending() {
    xmitState = SEND_STATE_XMIT;
}

void SendEngine::stopSending() {
    xmitState = SEND_STATE_OFF;
}

void SendEngine::waitForSendIdle() {
    while ( xmitState != SEND_STATE_IDLE ); 
}
