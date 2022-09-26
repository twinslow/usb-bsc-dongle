#ifndef CommandProcessor_h
#define CommandProcessor_h

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <Arduino.h>
#include "DataBuffer.h"

#include "SyncBitBanger.h"
#include "SendEngine.h"
#include "ReceiveEngine.h"

#include "bsc_protocol.h"

#define RESP_BIT    0x80
#define ERROR_BIT   0x40

#define CMD_UNKNOWN 0x00
#define CMD_WRITE   0x01
#define CMD_READ    0x02
#define CMD_WRITE_READ    0x03
#define CMD_DEBUG   0x09
#define CMD_RESET   0x0F

#define CMD_RESPONSE_MASK       0x80
#define CMD_RESPONSE_TIMEOUT    0x10
#define CMD_RESPONSE_ERROR      0x70

#define CMD_TEXTMODE '0'    // 0x30

#define HOST_CMD_MODE_TEXT      1
#define HOST_CMD_MODE_BINARY    2

#define RECEIVE_TIMEOUT     2000


/**
 * @brief Process commands from the connected device, host program or terminal
 *
 */
class CommandProcessor {
    public:
        CommandProcessor(
            SendEngine * sEng,
            ReceiveEngine * rEng,
            SyncControl * syncControl);

        virtual ~CommandProcessor();

        SendEngine      *sendEngine;
        ReceiveEngine   *receiveEngine;
        SyncControl     *syncControl;

        virtual unsigned long getAndProcessCommand();

        void enableDebug(bool v);
        bool isSwitchCommandModeRequired();
        uint8_t getNewCommandMode();

        // This call allows an instance (normally a mock instance) of Serial_
        // to be injected for unit testing.
        void injectSerial(Serial_ *serialInstance);

        char printbuff[80];

        virtual void sendDebugToHost(char * str);
        virtual void sendDebugToHost(const char * str);

    protected:

        // Use the standard Serial instance unless injected with something
        // else using injectSerial() method.
        Serial_ * useSerial = &Serial;

        unsigned long   lastDataReceivedTime;

        bool    debugEnabled = true;
        bool    switchCommandModeRequired = false;
        uint8_t newCommandMode;

        void setNewCommandMode(uint8_t newCommandMode);

        inline int serialRead(void) {
            int val = -1;
            while ( this->useSerial && val < 0 ) {
                val = this->useSerial->read();
                if ( val < 0 )
                    delay(1);
            }
            return val;
        }
};


class CommandProcessorBinary : public CommandProcessor {
    public:
        CommandProcessorBinary(
            SendEngine * sEng,
            ReceiveEngine * rEng,
            SyncControl * syncControl);

        virtual ~CommandProcessorBinary();

        void putCommand(int code,  int length);

        virtual unsigned long getAndProcessCommand();

        inline void sendResponse(int msgCode) {
            this->useSerial->write(msgCode);
            this->useSerial->write((byte)0);
            this->useSerial->write((byte)0);
        }
        inline void sendResponse(int msgCode, int len, void * data) {
            int x;
            char * bytedata = (char *)data;

            this->useSerial->write(msgCode);
            this->useSerial->write(len>>8 & 0xff);
            this->useSerial->write(len & 0xff);
            for ( x=0; x<len; x++ )
                this->useSerial->write(bytedata[x]);
        }
        inline void sendResponse(int msgCode, int val) {
            this->useSerial->write(msgCode);
            this->useSerial->write((byte)0);
            this->useSerial->write((byte)2);
            this->useSerial->write(val>>8 & 0xff);
            this->useSerial->write(val & 0xff);
        }
        inline void sendDebug(char *str) {
            if ( !this->debugEnabled )
                return;

            int len;
            len = strlen(str);
            sendResponse(CMD_DEBUG|CMD_RESPONSE_MASK, len, (void *)str);
        }
        inline void sendDebug(const char *str) {
            if ( !this->debugEnabled )
                return;

            int len;
            len = strlen(str);
            sendResponse(CMD_DEBUG|CMD_RESPONSE_MASK, len, (void *)str);
        }
        int  getCommandCode();
        int  getCommandDataLength();

        void getCommand();
        void copyCommandDataToSender();
        void process();

        virtual void sendDebugToHost(char * str);
        virtual void sendDebugToHost(const char * str);

    private:
        int     commandCode;
        int     commandDataLength;

};

/**
 * @brief Process text commands from the connected terminal
 *
 */
class CommandProcessorText : public CommandProcessor {
    public:
        CommandProcessorText(
            SendEngine * sEng,
            ReceiveEngine * rEng,
            SyncControl * syncControl);

        virtual ~CommandProcessorText();

        virtual unsigned long getAndProcessCommand();
        int getCommandParamHexadecimal();
        int addressCuPoll;
        int addressCuSelect;
        int addressDevice;
        bool addressSet = false;

    protected:
        int commandBuffLength = 80;
        char commandBuffer[80];

        int readCommand();

        inline void sendResponse(char *str) {
            this->useSerial->println(str);
        }
        inline void sendResponse(const char *str) {
            this->useSerial->println(str);
        }
        inline void sendDebug(char *str) {
            if ( !this->debugEnabled )
                return;

            this->useSerial->print("DEBUG: ");
            this->useSerial->println(str);
        }
        inline void sendDebug(const char *str) {
            if ( !this->debugEnabled )
                return;
            sendDebug( (char *) str);
        }

        virtual void sendDebugToHost(char * str);
        virtual void sendDebugToHost(const char * str);

    private:
        void execReset(void);
        void execSelect(void);
        void execPoll(void);
        void execWrite(void);
        void execRead(void);
};

#define TXT_CMD_POLL    1
#define TXT_CMD_WRITE   2
#define TXT_CMD_ADDR    3
#define TXT_CMD_DEBUG   9
#define TXT_CMD_BIN     10
#define TXT_CMD_HELP    12
#define TXT_CMD_MEM     13
#define TXT_CMD_RESET   15







/**
 * @brief A facade front end driver for the command processor. Drives the real command
 * processor depending on binary or text command mode.
 *
 */
class CommandProcessorFrontEnd {
    public:
        CommandProcessorFrontEnd(SyncBitBanger * bitBanger);
        ~CommandProcessorFrontEnd(void);

        void enableDebug(bool v) {
            this->debugEnabled = v;
        }

        unsigned long getAndProcessCommand();

        char * getPrintbuff() {
            return cmdProcessor->printbuff;
        }

        inline void sendDebugToHost(char * str) {
            cmdProcessor->sendDebugToHost(str);
        }



    private:
        SyncBitBanger * syncBitBanger;
        bool debugEnabled = true;
        CommandProcessor * cmdProcessor;
        SyncControl * syncControl;
};




#endif