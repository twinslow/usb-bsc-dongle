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

#include <Arduino.h>
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

#define CMD_WRITE   0x01
#define CMD_READ    0x02
#define CMD_POLL    0x05
#define CMD_DEBUG   0x09
#define CMD_READERROR 0x82
#define CMD_RESET   0x20
#define CMD_TEXTMODE 'T'

void sendDebug(char *str) {
    int len,x;
    len = strlen(str);
    Serial.write(CMD_DEBUG);
    Serial.write(len>>8 & 0xff);
    Serial.write(len & 0xff);
    for ( x=0; x<len; x++ )
        Serial.write(str[x]);
}

uint8_t dsrReady = false;

void setDsrNotReady()
{
        digitalWrite(dsrPin, HIGH); // Active low
        digitalWrite(cdPin, HIGH); // Active low
        digitalWrite(ctsPin, HIGH); // Active low
        digitalWrite(rxdPin, HIGH); // Idle state for the data line we are sending on.
        dsrReady = false;
}

void setDsrReady()
{
    if ( !dsrReady ) {
        digitalWrite(dsrPin, LOW);    // Active low
        delay(500);
        // This is going to be my temp ring indicator.
        digitalWrite(cdPin, LOW); //Ring!
        delay(2000);
        digitalWrite(cdPin, HIGH); // Silent
        delay(4000);
        digitalWrite(cdPin, LOW); // Ring! short ring because we going to pretend an answer.
        delay(500);
        digitalWrite(cdPin, HIGH); // Silent after answer
        //
        // ctsPin now also drives CD (temp)
        digitalWrite(ctsPin, LOW);
        delay(500);
        digitalWrite(rxdPin, HIGH);    // Idle state for the data line we are sending on.
        dsrReady = true;
    }
}

void deviceReset() {
    setDsrNotReady();
    delay(2000);
    setDsrReady();
}

unsigned long getAndProcessCommand() {
    int cmd, cmdlen, data, i;
    unsigned long lastDataReceivedTime;
    char printbuff[256];

    //setDsrReady();
    cmd = Serial.read();
    lastDataReceivedTime = millis();
    if ( cmd >= 0 ) {
        cmdlen = Serial.read();
        cmdlen <<= 8;
        cmdlen |= Serial.read();
    }
    sprintf(printbuff, "Got command: %d, length %d", cmd, cmdlen);
    sendDebug(printbuff);

    //setDsrReady();
    DataBufferReadOnly * frame;
    switch(cmd) {

        case CMD_RESET:
            sprintf(printbuff, "Executing command RESET");
            sendDebug(printbuff);
            deviceReset();
            break;

        case CMD_WRITE:
            sprintf(printbuff, "Executing command WRITE");
            sendDebug(printbuff);
            sendEngine->clearBuffer();
            sendEngine->addByte(BSC_CONTROL_LEADING_PAD);
            sendEngine->addByte(BSC_CONTROL_LEADING_PAD);
            sendEngine->addByte(BSC_CONTROL_SYN);
            for (i = 0; i < cmdlen; i++) {
                data = Serial.read();
                if (data >= 0) {
                    sprintf(printbuff, "     Adding byte %d to send-buffer", data);
                    sendDebug(printbuff);
                    sendEngine->addByte(data);
                }
            }
            sprintf(printbuff, "About to start sending %d bytes of data for WRITE",
                sendEngine->getRemainingDataToBeSent());
            sendDebug(printbuff);

            sendEngine->startSending();
            sendEngine->stopSendingOnIdle();
            sendEngine->waitForSendIdle();

            //delay(1000);
            sprintf(printbuff, "WRITE command completed with %d bytes of data remaining to be sent",
                    sendEngine->getRemainingDataToBeSent());
            sendDebug(printbuff);
            break;

        case CMD_POLL:
            sprintf(printbuff, "Executing command POLL");
            sendDebug(printbuff);
            sendEngine->clearBuffer();
            sendEngine->addByte(BSC_CONTROL_LEADING_PAD);
            sendEngine->addByte(BSC_CONTROL_LEADING_PAD);
            sendEngine->addByte(BSC_CONTROL_SYN);
            for (i = 0; i < cmdlen; i++) {
                data = Serial.read();
                if (data >= 0) {
                    sendEngine->addByte(data);
                }
            }
            sendEngine->startSending();
            sendEngine->stopSendingOnIdle();
            sendEngine->waitForSendIdle();

            sendDebug("POLL command completed");
            break;

        case CMD_READ:
            receiveEngine->startReceiving();
            //sendEngine->stopSending();
            sprintf(printbuff, "Executing command READ");
            sendDebug(printbuff);

            if ( receiveEngine->waitReceivedFrameComplete(2000) < 0 ) {
                sendDebug("READ command timeout");
                Serial.write(CMD_READERROR);
                Serial.write((byte)0);
                Serial.write((byte)0);
            } else {
                frame = receiveEngine->getSavedFrame();

                sprintf(printbuff, "Got response -- sending frame of length %d back to host", frame->getLength() );
                sendDebug(printbuff);

                // Get data and send back to host.
                Serial.write(CMD_WRITE);
                Serial.write( (frame->getLength() >> 8) & 0xff );
                Serial.write( frame->getLength() & 0xff );
                for ( i=0; i<frame->getLength(); i++) {
                    Serial.write(frame->get(i));
                }

                sendDebug("READ command completed");
            }
            break;

        default:
            sprintf(printbuff, "Unrecognized command code %d", cmd);
            sendDebug(printbuff);
            break;
    }
    return lastDataReceivedTime;
}

void loop() {
    static unsigned long lastDataReceivedTime = millis();

    if ( Serial && Serial.available() ) {
        lastDataReceivedTime = getAndProcessCommand();
    } else if ( Serial && !Serial.available() ) {
        // if ( millis() - lastDataReceivedTime > 10000 ) {
        //     deviceReset();
        // }
    }
}

inline void interruptAssertClockLines() {
        // Assert output clock for data being sent (which is on DTE rxdPin)
        // Assert output clock for data being received (which is on DTE txdPin)
        *RXCLK_PORT |= RXCLK_BIT;
        *TXCLK_PORT |= TXCLK_BIT;
}

inline void interruptDeassertClockLines() {
        // De-assert output clocks
        *RXCLK_PORT &= RXCLK_BITMASK;
        *TXCLK_PORT &= TXCLK_BITMASK;
}

static void interruptEverySecond() {

}


void serialDriverInterruptRoutine(void) {
    static uint8_t clockPhase = 0;
    switch(clockPhase) {
        case 0:
            // Put the output data pin in the correct state
            sendEngine->sendBit();
            clockPhase++;
            break;

        case 1:
            // Set the output clock lines to high
            interruptAssertClockLines();
            clockPhase++;
            break;

        case 2:
            // Read the state of the input data pin
            receiveEngine->getBit();
            clockPhase++;
            // This is the call that may take some cycles to execute.
            // Every 8th bit it is going to process the received byte and
            // look at the byte value and set the state machine appropriately.
            receiveEngine->processBit();
            break;

        case 3:
            // Set the output clock lines to low.
            interruptDeassertClockLines();
            clockPhase = 0;
            break;
    }
//    periodCounter++;
//    if ( periodCounter > oneSecondPeriodCount ) {
//        interruptEverySecond();
//        periodCounter = 0;
//    }
}


void setup() {
    volatile uint8_t *port;
    uint8_t mask1, mask2;
    char printbuff[256];

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
    //bitRate = 50;

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
    receiveEngine = new ReceiveEngine(txdPin, ctsPin);

    // Set our interval timer.
    interruptPeriod = (long)1000000 / bitRate / 4;
    oneSecondPeriodCount = (long)1000000 / interruptPeriod;
    periodCounter = 0;

    setDsrNotReady();
    delay(1000);

    Timer1.initialize( interruptPeriod );  // 52 us for 9600 bps.
    Timer1.attachInterrupt(serialDriverInterruptRoutine);

    digitalWrite(rxdPin, HIGH);


    // Open serial communications and wait for port to open:
    Serial.begin(57600);        // This should be faster than the DCE speed being implemented.
                                // The synchronous DCE can send and receive faster than the input async port, if
                                // this is a real serial link and not a native USB device.
                                // For USB native serial port on Leonardo the speed is irrelevant -- as it
                                // native USB.
    while (!Serial) {
      ; // wait for serial port to connect. Needed for native USB port only
    }

    sprintf(printbuff, "Interrupt period is %ld microseconds", interruptPeriod);
    sendDebug(printbuff);

    setDsrReady();
    sendDebug("Completed setDsrReady() processing.");

    sprintf(printbuff, "txclkPin=%d", txclkPin);
    sendDebug(printbuff);

    port = portOutputRegister(digitalPinToPort(txclkPin));

    sprintf(printbuff, "port=%ld", (long int) port);
    sendDebug(printbuff);

    mask1 = digitalPinToBitMask(txclkPin);
    sprintf(printbuff, "mask1=%d", mask1);
    sendDebug(printbuff);

    mask2  = ~digitalPinToBitMask(txclkPin);
    sprintf(printbuff, "mask2=%d", mask2);
    sendDebug(printbuff);
/*
    while(true) {

        sendEngine->clearBuffer();
        sendEngine->addByte((uint8_t)BSC_CONTROL_PAD);
//        sendEngine->addByte((uint8_t)BSC_CONTROL_PAD);
//        sendEngine->addByte((uint8_t)BSC_CONTROL_PAD);

        for ( i = 0; i < 4; i++ )
            sendEngine->addByte((uint8_t)BSC_CONTROL_SYN);

        sendEngine->addByte((uint8_t)BSC_CONTROL_STX);
        sendEngine->addByte((uint8_t)0xC1);
        sendEngine->addByte((uint8_t)0xC2);
        sendEngine->addByte((uint8_t)0xC3);
        sendEngine->addByte((uint8_t)BSC_CONTROL_ETX);
        sendEngine->addByte((uint8_t)0xC9);
        sendEngine->addByte((uint8_t)0xC8);
        sendEngine->addByte((uint8_t)BSC_CONTROL_PAD);

        sendEngine->startSending();
//        delay(100);
        sendEngine->waitForSendIdle();
        sendEngine->stopSending();

        delay(1000);
    }
*/
    return;

}

