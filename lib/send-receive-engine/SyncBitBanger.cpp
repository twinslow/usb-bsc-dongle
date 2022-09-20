#include "SyncBitBanger.h"

SyncBitBanger * SyncBitBanger::syncBitBangerInstance = NULL;
unsigned int SyncBitBanger::interruptCallCount = 0;

// Static interrupt routine
void SyncBitBanger::serialDriverInterruptRoutine(void) {
    static uint8_t clockPhase = 0;

    switch(clockPhase) {
        case 0:
            // Put the output data pin in the correct state
            syncBitBangerInstance->sendEngine->sendBit();
            clockPhase++;
            break;

        case 1:
            // Set the output clock lines to high
            syncBitBangerInstance->interruptAssertClockLines();
            clockPhase++;
            break;

        case 2:
            // Read the state of the input data pin
            syncBitBangerInstance->receiveEngine->getBit();
            clockPhase++;
            // This is the call that may take some cycles to execute.
            // Every 8th bit it is going to process the received byte and
            // look at the byte value and set the state machine appropriately.
            syncBitBangerInstance->receiveEngine->processBit();
            break;

        case 3:
            // Set the output clock lines to low.
            syncBitBangerInstance->interruptDeassertClockLines();
            clockPhase = 0;
            break;
    }
//    periodCounter++;
//    if ( periodCounter > oneSecondPeriodCount ) {
//        interruptEverySecond();
//        periodCounter = 0;
//    }
}

SyncBitBanger::SyncBitBanger() {
    this->sendEngine = NULL;
    this->receiveEngine = NULL;

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

    dsrReady = false;
};

SyncBitBanger::~SyncBitBanger() {
    if ( this->sendEngine )
        delete this->sendEngine;

    if ( this->receiveEngine )
        delete this->receiveEngine;
}

void SyncBitBanger::setDsrNotReady()
{
        digitalWrite(dsrPin, HIGH); // Active low
        digitalWrite(cdPin, HIGH); // Active low
        digitalWrite(ctsPin, HIGH); // Active low
        digitalWrite(rxdPin, HIGH); // Idle state for the data line we are sending on.

        digitalWrite(txclkPin, LOW);
        digitalWrite(rxclkPin, LOW);
        dsrReady = false;
}

void SyncBitBanger::setDsrReady()
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

void SyncBitBanger::deviceReset() {
    setDsrNotReady();
    delay(2000);
    setDsrReady();
}



void SyncBitBanger::setupPins() {
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

}


void SyncBitBanger::interruptEverySecond() {

}

void SyncBitBanger::init() {
    this->setupPins();

    // Setup these for fast access to output pins.
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

    this->setDsrNotReady();
    delay(1000);

    syncBitBangerInstance = this;
    interruptCallCount = 0;

    Serial.print(F("DEBUG: Setting up interrupt routine with interval of "));
    Serial.print(interruptPeriod);
    Serial.println(F(" micro seconds."));

    Timer1.initialize(interruptPeriod); // 52 us for 9600 bps.
    Timer1.attachInterrupt(serialDriverInterruptRoutine);

    delay(1000);
    unsigned int callCount = interruptCallCount;
    Serial.print(F("After one second the interrupt routine has been called "));
    Serial.print(callCount);
    Serial.println(F(" times."));

    Serial.print(F("DEBUG: RXCLK_PORT          = 0x"));
    Serial.println((unsigned int)RXCLK_PORT, 16);
    Serial.print(F("DEBUG: RXCLK_BIT           = 0x"));
    Serial.println((unsigned int)RXCLK_BIT, 16);
    Serial.print(F("DEBUG: RXCLK_BITMASK       = 0x"));
    Serial.println((unsigned int)RXCLK_BITMASK, 16);
    Serial.print(F("DEBUG: TXCLK_PORT          = 0x"));
    Serial.println((unsigned int)TXCLK_PORT, 16);
    Serial.print(F("DEBUG: TXCLK_BIT           = 0x"));
    Serial.println((unsigned int)TXCLK_BIT, 16);
    Serial.print(F("DEBUG: TXCLK_BITMASK       = 0x"));
    Serial.println((unsigned int)TXCLK_BITMASK, 16);
}

SyncControl::SyncControl(SyncBitBanger * instance) {
    this->bitBangerInstance = instance;
}

void SyncControl::deviceReset() {
    this->bitBangerInstance->deviceReset();
}