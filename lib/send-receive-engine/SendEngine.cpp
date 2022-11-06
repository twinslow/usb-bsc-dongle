#include <Arduino.h>
#include "SendEngine.h"
#include "bsc_protocol.h"
#include "mcu_abstraction.h"

// Pins are named with respect to the DTE. As we are implementing a DCE interface, our
// RXD pin is being used to send data (the DTE is receiving on this pin).

SendEngine::SendEngine(gpio_pin_t rxdPin) {
    _RXD_PORT       = portOutputRegister(digitalPinToPort(rxdPin));
    _RXD_BIT        = digitalPinToBitMask(rxdPin);
    _RXD_BITMASK    = ~_RXD_BIT;

    // Initialize/clear data buffers
    _sendDataBuffer.clear();
    _sendBitBufferLength = 0;

    // Initialize the state engine to be idle.
    xmitState = SEND_STATE_OFF;
    _stopOnIdle = false;
}

SendEngine::~SendEngine() {
}

void SendEngine::clearBuffer(void)
{
    // Initialize/clear data buffers
    _sendDataBuffer.clear();
    _sendBitBufferLength = 0;

    // Initialize the state engine to be idle.
    xmitState = SEND_STATE_OFF;
    _stopOnIdle = false;
}

pgpio_port_t SendEngine::getRxdPort(void)
{
    return _RXD_PORT;
}

gpio_port_t SendEngine::getRxdBitMask(void) {
    return _RXD_BIT;
}

gpio_port_t SendEngine::getRxdBitNotMask(void) {
    return _RXD_BITMASK;
}

uint8_t SendEngine::getBitBuffer(void) {
    return _sendBitBuffer;
}

uint8_t SendEngine::getBitBufferLength(void) {
    return _sendBitBufferLength;
}

DataBuffer & SendEngine::getDataBuffer(void) {
    return _sendDataBuffer;
}

void SendEngine::sendBit() {
    int retVal;

    if ( xmitState == SEND_STATE_OFF )
      return;

    // If nothing in bit buffer, then fetch character from sendDataBuffer.
    if ( _sendBitBufferLength == 0 ) {
        retVal = _sendDataBuffer.read(&_sendBitBuffer);
        if ( retVal < 0 ) {
            // There was nothing in the data buffer, so we going to send an
            // idle character.
            _sendBitBuffer = BSC_CONTROL_IDLE;
            if ( _stopOnIdle ) {
                xmitState = SEND_STATE_OFF;
                // Set the output pin high ... idle state.
                *_RXD_PORT |= _RXD_BIT;
                return;
            }
            xmitState = SEND_STATE_IDLE;
        } else {
            xmitState = SEND_STATE_XMIT;
        }
        _sendBitBufferLength = 8;
    }

    // Set pin high or low.
    lastBitSent = _sendBitBuffer & 0x01;
    if ( lastBitSent )
        *_RXD_PORT |= _RXD_BIT;
    else
        *_RXD_PORT &= _RXD_BITMASK;

    _savedBitBuffer = _sendBitBuffer;
    _sendBitBuffer = _sendBitBuffer >> 1;    // Shift bits in send buffer
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
    _stopOnIdle = false;
}

void SendEngine::stopSending() {
    xmitState = SEND_STATE_OFF;
}

void SendEngine::waitForSendIdle() {
    while (xmitState != SEND_STATE_IDLE &&
           xmitState != SEND_STATE_OFF);
}

void SendEngine::stopSendingOnIdle() {
    _stopOnIdle = true;
}

int SendEngine::getRemainingDataToBeSent(void) {
    int remainingDataLength = _sendDataBuffer.getLength() - _sendDataBuffer.getPos();
    return remainingDataLength;
}