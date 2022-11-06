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
//#include <TimerOne.h>

#include "bsc_protocol.h"

#include "DataBuffer.h"
#include "SendEngine.h"
#include "ReceiveEngine.h"
#include "CommandProcessor.h"
#include "SyncBitBanger.h"

//CommandProcessorBinary * commandProcessor;
SyncBitBanger * syncBitBanger;
//SyncControl * syncControl;
CommandProcessorFrontEnd * commandProcFE;

#define printbuff commandProcFE->getPrintbuff()
#define sendDebug(x) commandProcFE->sendDebugToHost(x)

void loop() {
    static unsigned long lastDataReceivedTime = millis();

    if ( Serial ) {
        lastDataReceivedTime = commandProcFE->getAndProcessCommand();
    }
}


void setup() {
    volatile uint8_t *port;
    uint8_t mask1, mask2;
    char debugbuff[80];


    // Open serial communications and wait for port to open:
    Serial.begin(57600);        // This should be faster than the DCE speed being implemented.
                                // The synchronous DCE can send and receive faster than the input async port, if
                                // this is a real serial link and not a native USB device.
                                // For USB native serial port on Leonardo the speed is irrelevant -- as it
                                // native USB.
    while (!Serial) {
      ; // wait for serial port to connect. Needed for native USB port only
    }

    pinMode(LED_BUILTIN, OUTPUT);
    syncBitBanger = new SyncBitBanger();
    syncBitBanger->init();
    commandProcFE = new CommandProcessorFrontEnd(syncBitBanger);
    syncBitBanger->setDsrReady();
    // port = portOutputRegister(digitalPinToPort(syncBitBanger->txclkPin));
    // mask1 = digitalPinToBitMask(syncBitBanger->txclkPin);
    // mask2  = ~digitalPinToBitMask(syncBitBanger->txclkPin);
    return;
}


//--------------------------- Retired code ----------------------------
/*
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
*/


/*
#define CMD_WRITE   0x01
#define CMD_READ    0x02
#define CMD_POLL    0x05
#define CMD_DEBUG   0x09
#define CMD_READERROR 0x82
#define CMD_RESET   0x20
#define CMD_TEXTMODE 'T'
*/

/*
uint8_t dsrReady = false;

void setDsrNotReady()
{
        digitalWrite(dsrPin, HIGH); // Active low
        digitalWrite(cdPin, HIGH); // Active low
        digitalWrite(ctsPin, HIGH); // Active low
        digitalWrite(rxdPin, HIGH); // Idle state for the data line we are sending on.

        digitalWrite(txclkPin, LOW);
        digitalWrite(rxclkPin, LOW);
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

        digitalWrite(txclkPin, LOW);
        digitalWrite(rxclkPin, LOW);
    }
}

void deviceReset() {
    setDsrNotReady();
    delay(2000);
    setDsrReady();
}


*/

/*

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
            sendEngine->addByte(BSC_CONTROL_PAD);
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
            sendEngine->addByte(BSC_CONTROL_PAD);
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
*/
/*
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
*/

