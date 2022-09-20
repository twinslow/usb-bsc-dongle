#ifndef SyncBitBanger_h
#define SyncBitBanger_h

#include <stdlib.h>
#include <stdio.h>

#include <Arduino.h>
#include <TimerOne.h>

#include "SendEngine.h"
#include "ReceiveEngine.h"

#include "bsc_protocol.h"

#define CYCLE_STATE_STARTBIT 0
#define CYCLE_STATE_MIDBIT   1


class SyncBitBanger {
    public:

        int txdPin, rxdPin, debugDataPin, rxclkPin, txclkPin, dteclkPin;
        int ctsPin, rtsPin, dsrPin, dtrPin, cdPin, riPin;
        long    bitRate;
        uint8_t dsrReady;

        SendEngine      *sendEngine;
        ReceiveEngine   *receiveEngine;

        SyncBitBanger();
        ~SyncBitBanger();
        void init();
        void setDsrNotReady();
        void setDsrReady();
        void deviceReset();

        // Interrupt stuff must be static

        static SyncBitBanger *syncBitBangerInstance;
        static void serialDriverInterruptRoutine(void);

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
        void interruptEverySecond();

    private:

        volatile uint8_t *RXCLK_PORT;
        uint8_t RXCLK_BIT;
        uint8_t RXCLK_BITMASK;

        volatile uint8_t *TXCLK_PORT;
        uint8_t TXCLK_BIT;
        uint8_t TXCLK_BITMASK;

        long interruptPeriod;
        long oneSecondPeriodCount;
        long periodCounter;

        void setupPins();



};

class SyncControl {
    public:
        SyncControl(SyncBitBanger *bitBangerInstance);
        virtual ~SyncControl() {};
        virtual void deviceReset();
    private:
        SyncBitBanger * bitBangerInstance;
};


#endif

