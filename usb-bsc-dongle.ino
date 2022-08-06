/*
 * Pin assignments
 * ---------------
 * 
 * Arduino DB25 connector is DCE wired. Pins are always named from the perspective 
 * of the DTE. In other words, for a DCE, RXD is an output pin.
 * 
 * Signal     DCE-Pin     Source     Max-232      Arduino Pin Wire color
 * 
 * TXCLK .    15 .        DCE .      A-T1 .       16          (green)       
 * 
 * RXCLK      17 .        DCE .      A-T2 .       14          (blue)      VER 
 * 
 * DTR .      20 .        DTE .      A-R1 .       10          (black)
 * 
 * TXCLK-DTE  24 .        DTE .      A-R2 .       15 .        (gray)
 * 
 * CD .       8 .         DCE .      B-T1 .       5 .         (brown)
 * 
 * RI .       22 .        DCE .      No-Connect . 
 * 
 * DSR .      6 .         DCE .      B-T2 .       6 .         (white)
 * 
 * CTS .      5           DCE        C-T1 .       8 .         (red)
 * 
 * RTS .      4 .         DTE .      C-R1 .       2  .        (purple)
 * 
 * TXD .      2 .         DTE .      C-R2 .       3  NRZI     (yellow)     VER
 * 
 * RXD .      3 .         DCE .      C-T2 .       9  NRZI .   (orange)     VER
 * 
 * Ground .   1 .                    No-connect
 *
 * Pin   Leonardo 
 *       Port
 * ----- --------       
 * 0     PD2
 * 1     PD3
 * 2     PD1
 * 3     PD0
 * 4     PD4
 * 5     PD6
 * 6     PD7
 * 7     PE6
 * 
 * 8     PB4
 * 9     PB5
 * 10    PB6
 * 11    PB7
 * 12    PD6
 * 13    PC7
 * 
 * SDA   PD1
 * SCL   PD0
 * 
 * A0    PF7
 * A1    PF6
 * A2    PF5
 * A3    PF4
 * A4    PF1
 * A5    PF0
 * 
 * 
 * 
 * 
 * MAX232 Pins
 * -----------
 * 
 *         -----------------
 *    C1+  | 1   |--|   16 |  VCC
 *    VS+  | 2          15 |  GND
 *    C1-  | 3          14 |  T1OUT
 *    C2+  | 4          13 |  R1IN
 *    C2-  | 5          12 |  R1OUT
 *    VS-  | 6          11 |  T1IN
 *  T2OUT  | 7          10 |  T2IN
 *  R2IN   | 8           9 |  R2OUT
 *         -----------------
 * 
 * 
 * 
 */

#include <TimerOne.h>
#include "bsc_protocol.h"

#include "DataBuffer.h"
#include "SendEngine.h"
#include "ReceiveEngine.h"



uint8_t interruptCycleState;

#define CYCLE_STATE_STARTBIT 0
#define CYCLE_STATE_MIDBIT   1

uint8_t txdPin, rxdPin, debugDataPin, rxclkPin, txclkPin, dteclkPin;
uint8_t ctsPin, rtsPin, dsrPin, dtrPin,  cdPin, riPin;

long    bitRate;

volatile uint8_t *RXCLK_PORT;
uint8_t RXCLK_BIT;
uint8_t RXCLK_BITMASK;

volatile uint8_t *TXCLK_PORT;
uint8_t TXCLK_BIT;
uint8_t TXCLK_BITMASK;

SendEngine      *sendEngine;
ReceiveEngine   *receiveEngine;

long interruptPeriod;
long oneSecondPeriodCount;
long periodCounter;

void setup() {

    // RS232 pin names are from the DTE point of view. 
    // Therefore, for example, the DTE transmit data pin is an input to the
    // DCE. Likewise, the DTE receive data pin is an output from the DCE. 
    
    ctsPin = 8;         // Output 
    dsrPin = 6;         // Output
    dtrPin = 10;        // Input
    rtsPin = 2;         // Input
    cdPin  = 5;         // Output
    riPin  = 0;         // Output - not used.
    rxclkPin = 14;      // Output
    txclkPin = 16;      // Output 
    dteclkPin = 15;     // Input tx data clock from DTE -- Not used, but assigned and wired.
    txdPin = 3;         // Input tx data
    rxdPin = 9;         // Output rx data 

    bitRate = 2400;     // Bit rate. 19,200 bps is about the max for 
                        // bit banging the synchronous serial DCE.

    pinMode(ctsPin, OUTPUT);
    pinMode(dsrPin, OUTPUT);
    pinMode(dtrPin, INPUT);
    pinMode(cdPin,OUTPUT);
    //pinMode(riPin, OUTPUT);  // This is not being used because 
                               // we don't have enough output channels
                               // on the 3 MAX232 ICs.

    pinMode(dteclkPin, INPUT);
    
    pinMode(rxdPin, OUTPUT);      // This is the output data to the MAX232
    if (debugDataPin) 
        pinMode(debugDataPin, OUTPUT);   // This is just for logic analyzer debugging.
    pinMode(txdPin, INPUT_PULLUP);
    pinMode(txclkPin, OUTPUT);
    pinMode(rxclkPin, OUTPUT); 

    interruptCycleState = CYCLE_STATE_STARTBIT;

    pinMode(LED_BUILTIN, OUTPUT);

    RXCLK_PORT     = portOutputRegister(digitalPinToPort(rxclkPin));
    RXCLK_BIT      = digitalPinToBitMask(rxclkPin); 
    RXCLK_BITMASK  = ~digitalPinToBitMask(rxclkPin); 

    TXCLK_PORT     = portOutputRegister(digitalPinToPort(txclkPin));
    TXCLK_BIT      = digitalPinToBitMask(txclkPin); 
    TXCLK_BITMASK  = ~digitalPinToBitMask(txclkPin); 

    sendEngine = new SendEngine(rxdPin);
    receiveEngine = new ReceiveEngine(txdPin);
    
    // Set our interval timer. 
    interruptPeriod = (long)1000000 / bitRate / 2;
    oneSecondPeriodCount = 1000000 / interruptPeriod;
    periodCounter = 0;

    digitalWrite(cdPin, LOW);     // Active low
    digitalWrite(dsrPin, LOW);    // Active low  
    digitalWrite(ctsPin, LOW);    // Active low 
    
    Timer1.initialize( interruptPeriod );  // 52 us for 9600 bps.
    Timer1.attachInterrupt(serialDriverInterruptRoutine); 

    digitalWrite(rxdPin, LOW);
    

    // Open serial communications and wait for port to open:
    Serial.begin(57600);        // This should be faster than the DCE speed being implemented.
                                // The synchronous DCE can send and receive faster than the input async port, if
                                // this is a real serial link and not a native USB device. 
                                // For USB native serial port on Leonardo the speed is irrelevant -- as it
                                // native USB. 
//    while (!Serial) {
//      ; // wait for serial port to connect. Needed for native USB port only
//    }

    return;
    
}

void loop() {
    int data;
    uint8_t newXmitState;

    if ( Serial && Serial.available() ) {
        data = Serial.read();
        if ( data > 0 ) {
            //Serial.write(data);
            sendEngine->addByte(data);
        }         
    }

}


static void interruptAssertClockLines() {
        // Assert output clock for data being sent (which is on DTE rxdPin)
        *RXCLK_PORT |= RXCLK_BIT;
        // Assert output clock for data being received (which is on DTE txdPin)
        *TXCLK_PORT |= TXCLK_BIT;
}

static void interruptDeassertClockLines() {
        // De-assert output clocks
        *RXCLK_PORT &= RXCLK_BITMASK;
        *TXCLK_PORT &= TXCLK_BITMASK;  
}

static void interruptEverySecond() {
  
}

void serialDriverInterruptRoutine(void) {
    if ( interruptCycleState == CYCLE_STATE_STARTBIT ) {
        sendEngine->sendBit();
        interruptAssertClockLines();
        interruptCycleState == CYCLE_STATE_MIDBIT;  
    } else {
        receiveEngine->getBit();
        interruptDeassertClockLines();
        interruptCycleState == CYCLE_STATE_STARTBIT;  
    }

    periodCounter++;
    if ( periodCounter > oneSecondPeriodCount ) {
        interruptEverySecond();
        periodCounter = 0;
    }
}
