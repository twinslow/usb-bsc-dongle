#ifndef _MCU_ABSTRACTION_H
#define _MCU_ABSTRACTION_H

#include "Arduino.h"

#if defined(ARDUINO_BLUE_PILL)
//------------------------------- BLUE Pill --------------------------------

#define SerialClass USBSerial
#define SerialInstance &SerialUSB
typedef uint32_t gpio_pin_t;
typedef uint32_t gpio_port_t;             // STM32 ports are 16 bits wide
typedef gpio_port_t volatile * pgpio_port_t;       // Pointer to port (address -- 32 bits)

#else
//------------------------------- OTHER ARDUINO BOARDS --------------------------------
#define SerialClass Serial_
#define SerialInstance &Serial
typedef uint8_t gpio_pin_t;
typedef uint8_t gpio_port_t;
typedef gpio_port_t * pgpio_port_t;       // Pointer to port (address -- 16 bits)

#endif


#endif
