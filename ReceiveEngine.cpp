#include <Arduino.h>
#include "ReceiveEngine.h"

ReceiveEngine::ReceiveEngine(uint8_t txdPin) {

    _TXD_PORT       = portInputRegister(digitalPinToPort(txdPin));
    _TXD_BIT        = digitalPinToBitMask(txdPin); 
    _TXD_BITMASK    = ~digitalPinToBitMask(txdPin); 

    _receiveState = RECEIVE_STATE_IDLE;   
}

void ReceiveEngine::getBit(void) {
  uint8_t inputBit;

  inputBit = (*_TXD_PORT & _TXD_BIT) ? 0x80 : 0x00;
  _inputBitBuffer >>= 1; // Shift to right
  _inputBitBuffer |= inputBit;   // Setting most significant bit when line was high.  
}
