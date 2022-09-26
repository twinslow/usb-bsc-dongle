#include <stdlib.h>
#include <stdio.h>

#include <Arduino.h>

#include "CommandProcessor.h"

CommandProcessor::CommandProcessor(
    SendEngine * sEng,
    ReceiveEngine * rEng,
    SyncControl * syncCntrl  ) {
    //Serial.println(F("CommandProcessor constructor starting."));

    this->sendEngine = sEng;
    this->receiveEngine = rEng;
    this->syncControl = syncCntrl;

    this->lastDataReceivedTime = millis();
    //Serial.println(F("CommandProcessor constructor complete."));
}

CommandProcessor::~CommandProcessor() {
}

void CommandProcessor::enableDebug(bool v) {
    this->debugEnabled = v;
}

void CommandProcessor::setNewCommandMode(uint8_t newCommandMode) {
    this->switchCommandModeRequired = true;
    this->newCommandMode = newCommandMode;
}

bool CommandProcessor::isSwitchCommandModeRequired() {
    return this->switchCommandModeRequired;
}

uint8_t CommandProcessor::getNewCommandMode() {
    return this->newCommandMode;
}

void CommandProcessor::injectSerial(Serial_ *serialInstance) {
    this->useSerial = serialInstance;
}

unsigned long CommandProcessor::getAndProcessCommand() {
    return 0;
}

void CommandProcessor::sendDebugToHost(char * str) {}
void CommandProcessor::sendDebugToHost(const char * str) {}

CommandProcessorBinary::CommandProcessorBinary(
    SendEngine * sEng,
    ReceiveEngine * rEng,
    SyncControl * syncCntrl  ) : CommandProcessor(sEng, rEng, syncControl) {
    //Serial.println(F("CommandProcessorBinary constructor complete."));
}

CommandProcessorBinary::~CommandProcessorBinary() {
}

void CommandProcessorBinary::sendDebugToHost(char * str) {
    sendDebug(str);
}

void CommandProcessorBinary::sendDebugToHost(const char * str) {
    sendDebug(str);
}


void CommandProcessorBinary::putCommand(int code, int length) {
    this->commandCode = code;
    this->commandDataLength = length;
}






void CommandProcessorBinary::getCommand() {
    int cmd;
    int cmdlen;

    cmd = this->serialRead();
    this->lastDataReceivedTime = millis();
    if ( cmd >= 0 ) {
        cmdlen = this->serialRead();
        cmdlen <<= 8;
        cmdlen |= this->serialRead();
    }

    this->commandCode = cmd;
    this->commandDataLength = cmdlen;
}

int CommandProcessorBinary::getCommandCode() {
    return this->commandCode;
}

int CommandProcessorBinary::getCommandDataLength() {
    return this->commandDataLength;
}


void CommandProcessorBinary::copyCommandDataToSender() {
    int i;
    int data;
    int cmdlen = this->commandDataLength;

    this->sendEngine->clearBuffer();
    this->sendEngine->addByte(BSC_CONTROL_PAD);
    this->sendEngine->addByte(BSC_CONTROL_LEADING_PAD);
    this->sendEngine->addByte(BSC_CONTROL_LEADING_PAD);
    this->sendEngine->addByte(BSC_CONTROL_SYN);
    for (i = 0; i < cmdlen; i++) {
        data = this->serialRead();
        if (data >= 0) {
            // sprintf(printbuff, "     Adding byte %d to send-buffer", data);
            // sendDebug(printbuff);
            sendEngine->addByte(data);
        }
    }
    sendEngine->addByte(BSC_CONTROL_PAD);
}


int freeRam () {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

void CommandProcessorBinary::process() {

    getCommand();
    // this->sendResponse(0x8C, freeRam());

    switch(this->commandCode) {
        case CMD_RESET:
            // sendDebug("RESET command starting");
            syncControl->deviceReset();
            sendDebug("RESET command completed");
            sendResponse(RESP_BIT|CMD_RESET);
            break;

        case CMD_DEBUG: {
                int debugValue = this->serialRead();
                this->enableDebug( debugValue ? true : false );
                sendDebug("DEBUG command completed");
                sendResponse(0x8C, freeRam());
                sendResponse(RESP_BIT|CMD_DEBUG);
            }
            break;

        case CMD_TEXTMODE: {
                setNewCommandMode(HOST_CMD_MODE_TEXT);
                sendDebug("TEXTMODE command processed ... mode will be changed");
                sendResponse(RESP_BIT|CMD_TEXTMODE);
            }
            break;

        case CMD_WRITE:
            copyCommandDataToSender();

            sendEngine->startSending();
            sendEngine->stopSendingOnIdle();
            sendEngine->waitForSendIdle();

            sprintf(printbuff, "WRITE command completed with %d bytes of data remaining to be sent",
                    sendEngine->getRemainingDataToBeSent());
            sendDebug(printbuff);

            sendResponse(CMD_WRITE | CMD_RESPONSE_MASK);
            break;

        case CMD_WRITE_READ:
            copyCommandDataToSender();

            sendEngine->startSending();
            sendEngine->stopSendingOnIdle();
            sendEngine->waitForSendIdle();

            sprintf(printbuff, "WRITE_READ command has %d bytes of data remaining to be sent",
                    sendEngine->getRemainingDataToBeSent());
            sendDebug(printbuff);


            receiveEngine->startReceiving();
            sendDebug("Reading response ...");

            if ( receiveEngine->waitReceivedFrameComplete(RECEIVE_TIMEOUT) < 0 ) {
                receiveEngine->getDataBuffer();
                sendResponse(CMD_WRITE_READ | CMD_RESPONSE_MASK | CMD_RESPONSE_TIMEOUT);
            } else {
                DataBuffer * frame = receiveEngine->getSavedFrame();
                sendResponse(CMD_WRITE_READ | CMD_RESPONSE_MASK,
                    frame->getLength(), frame->getData());
            }
            break;

        case CMD_READ:
            receiveEngine->startReceiving();
            sendDebug("Reading response ...");

            if ( receiveEngine->waitReceivedFrameComplete(RECEIVE_TIMEOUT) < 0 ) {
                receiveEngine->getDataBuffer();
                sendResponse(CMD_READ | CMD_RESPONSE_MASK | CMD_RESPONSE_TIMEOUT);
            } else {
                DataBuffer * frame = receiveEngine->getSavedFrame();
                sendResponse(CMD_READ | CMD_RESPONSE_MASK,
                    frame->getLength(), frame->getData());
            }
            break;

        default:
            sprintf(printbuff, "Unrecognized command code %d", this->commandCode);
            sendDebug(printbuff);

            sendResponse(RESP_BIT|ERROR_BIT|(byte)this->commandCode);
            break;
    }
}

unsigned long CommandProcessorBinary::getAndProcessCommand() {
    this->process();
    return this->lastDataReceivedTime;
}

CommandProcessorText::CommandProcessorText(
    SendEngine * sEng,
    ReceiveEngine * rEng,
    SyncControl * syncCntrl  ) : CommandProcessor(sEng, rEng, syncControl) {
    addressCuPoll = 0x40;
    addressCuSelect = 0x60;
    addressDevice = 0x40;
    addressSet = true;
}

CommandProcessorText::~CommandProcessorText() {
}

void CommandProcessorText::sendDebugToHost(char * str) {
    sendDebug(str);
}

void CommandProcessorText::sendDebugToHost(const char * str) {
    sendDebug(str);
}

int CommandProcessorText::readCommand() {
    int x;
    int chr;

    this->useSerial->write("> ");

    // Read characters from serial port until buffer is full (user-error)
    // or until we get a NL char.
    for(x=0; x<commandBuffLength; ) {
        chr = this->serialRead();
        if ( chr == /* carriage return */ 13 ) {
        } else if ( chr == /* new line */ 10 ) {
            commandBuffer[x++] = '\0';
            break;
        } else if ( chr == 8 /* back space */ ) {
            if ( x > 0 )
                x--;
        } else if ( chr >= 0 ) {
            commandBuffer[x++] = toupper(chr);
        }
    }
    if ( x >= commandBuffLength ) {
        this->useSerial->println(F("ERROR: Command too long."));
        return -1;
    }

    char * command = strtok(commandBuffer," ,");
    sprintf(this->printbuff, "Command is '%s'", command);
    sendDebug(this->printbuff);

    if ( !strcmp(command,"POLL") )
        return TXT_CMD_POLL;

    if ( !strcmp(command,"WRITE") )
        return TXT_CMD_WRITE;

    if ( !strcmp(command,"ADDR") )
        return TXT_CMD_ADDR;

    if ( !strcmp(command,"DEBUG") )
        return TXT_CMD_DEBUG;

    if ( !strcmp(command,"BIN") )
        return TXT_CMD_BIN;

    if ( !strcmp(command,"RESET") )
        return TXT_CMD_RESET;

    if ( !strcmp(command,"MEM") )
        return TXT_CMD_MEM;

    if ( !strcmp(command,"HELP") || !strcmp(command,"?") )
        return TXT_CMD_HELP;

    this->useSerial->println(F("ERROR: Invalid command."));
    return -1;
}

int CommandProcessorText::getCommandParamHexadecimal() {

    int hexValue = 0;
    int digit;
    unsigned int i;

    char * parmStr = strtok(NULL, ",");
    for(i=0; i<strlen(parmStr); i++) {
        if ( parmStr[i] == ' ' ||
             parmStr[i] == 'x' )
            continue;

        if ( isdigit(parmStr[i]) )
            digit = parmStr[i] - '0';
        else if ( parmStr[i] >= 'A' && parmStr[i] <= 'F' )
            digit = parmStr[i] - 'A' + 10;
        else {
            this->useSerial->print(F("ERROR: Invalid parameter in command - '"));
            this->useSerial->print(parmStr);
            this->useSerial->println("'");
            hexValue = -1;
            break;
        }
        hexValue = hexValue * 16 + digit;
    }
    if ( this->debugEnabled ) {
        this->useSerial->print(F("DEBUG: Returning param value of "));
        this->useSerial->print(hexValue);
        this->useSerial->print(F(" for string - "));
        this->useSerial->println(parmStr);
    }
    return hexValue;
}

unsigned long CommandProcessorText::getAndProcessCommand() {
    int command;
    int parm1, parm2, parm3;
    // Get command
    command = readCommand();
    if ( command < 0 )
        return millis();

    switch(command) {
        case TXT_CMD_DEBUG:
            parm1 = getCommandParamHexadecimal();
            this->debugEnabled = parm1 ? true : false;
            break;

        case TXT_CMD_ADDR:
            parm1 = getCommandParamHexadecimal();
            parm2 = getCommandParamHexadecimal();
            parm3 = getCommandParamHexadecimal();
            if ( parm1 >= 0 && parm2 >= 0 && parm3 >= 0 ) {
                addressCuPoll = parm1;
                addressCuSelect = parm2;
                addressDevice = parm3;
            }
            this->addressSet = true;
            break;

        case TXT_CMD_POLL:
            if ( !this->addressSet ) {
                this->useSerial->println(F("ERROR: Use ADDR command to set device addr first."));
            } else {
                execReset();
                execPoll();
                execRead();
            }
            break;

        case TXT_CMD_WRITE:
            if ( !this->addressSet ) {
                this->useSerial->println(F("ERROR: Use ADDR command to set device addr first."));
            } else {
                execReset();
                execSelect();
                delay(50);
                execWrite();
                execRead();
                delay(50);
            }
            break;

        case TXT_CMD_MEM:
            this->useSerial->print(F("Memory has "));
            this->useSerial->print(freeRam());
            this->useSerial->println(F(" bytes."));
            break;

        case TXT_CMD_RESET:
            this->useSerial->println(F("Starting RESET."));
            // this->useSerial->print(F("this->syncControl = 0x"));
            // this->useSerial->println((unsigned int)this->syncControl, 16);
            this->syncControl->deviceReset();
            this->useSerial->println(F("RESET complete."));
            break;

        case TXT_CMD_BIN:
            setNewCommandMode(HOST_CMD_MODE_BINARY);
            sendDebug("TEXTMODE command processed ... mode will be changed");
            break;

        case TXT_CMD_HELP:
            this->useSerial->println(F("Commands are --"));
            this->useSerial->println(F("ADDR hex-addr-poll,hex-addr-select,hex-dev-addr"));
            this->useSerial->println(F("POLL"));
            this->useSerial->println(F("WRITE"));
            this->useSerial->println(F("RESET"));
            this->useSerial->println(F("MEM"));
            this->useSerial->println(F("BIN\n"));
            break;

        default:
            this->useSerial->println(F("ERROR: Unrecognized command."));

    }
    return millis();
}



void CommandProcessorText::execReset() {
    // Reset the line ... puts the tributary stations in control mode and listening for
    // a select/poll.
    this->sendEngine->clearBuffer();
    this->sendEngine->addByte(BSC_CONTROL_PAD);
    this->sendEngine->addByte(BSC_CONTROL_LEADING_PAD);
    this->sendEngine->addByte(BSC_CONTROL_LEADING_PAD);
    this->sendEngine->addByte(BSC_CONTROL_SYN);
    this->sendEngine->addByte(BSC_CONTROL_SYN);
    this->sendEngine->addByte(BSC_CONTROL_EOT);
    this->sendEngine->addByte(BSC_CONTROL_PAD);

    sendEngine->startSending();
    sendEngine->stopSendingOnIdle();
    sendEngine->waitForSendIdle();

    sendResponse("Sent EOT to tributary stations");
}

void CommandProcessorText::execPoll() {
    // Send a poll to the device
    this->sendEngine->clearBuffer();
    this->sendEngine->addByte(BSC_CONTROL_PAD);
    this->sendEngine->addByte(BSC_CONTROL_LEADING_PAD);
    this->sendEngine->addByte(BSC_CONTROL_LEADING_PAD);
    this->sendEngine->addByte(BSC_CONTROL_SYN);
    this->sendEngine->addByte(BSC_CONTROL_SYN);

    this->sendEngine->addByte(this->addressCuPoll);
    this->sendEngine->addByte(this->addressCuPoll);
    this->sendEngine->addByte(this->addressDevice);
    this->sendEngine->addByte(this->addressDevice);

    this->sendEngine->addByte(BSC_CONTROL_ENQ);
    this->sendEngine->addByte(BSC_CONTROL_PAD);

    sendEngine->startSending();
    sendEngine->stopSendingOnIdle();
    sendEngine->waitForSendIdle();

    sendResponse("Sent poll to specific station/device");
}

void CommandProcessorText::execSelect() {
    // Send a select to the device
    this->sendEngine->clearBuffer();
    this->sendEngine->addByte(BSC_CONTROL_PAD);
    this->sendEngine->addByte(BSC_CONTROL_LEADING_PAD);
    this->sendEngine->addByte(BSC_CONTROL_LEADING_PAD);
    this->sendEngine->addByte(BSC_CONTROL_SYN);
    this->sendEngine->addByte(BSC_CONTROL_SYN);

    this->sendEngine->addByte(this->addressCuSelect);
    this->sendEngine->addByte(this->addressCuSelect);
    this->sendEngine->addByte(this->addressDevice);
    this->sendEngine->addByte(this->addressDevice);

    this->sendEngine->addByte(BSC_CONTROL_ENQ);
    this->sendEngine->addByte(BSC_CONTROL_PAD);

    sendEngine->startSending();
    sendEngine->stopSendingOnIdle();
    sendEngine->waitForSendIdle();

    sendResponse("Sent select to station/device");
}

void CommandProcessorText::execWrite() {
    // Send a select to the device
    this->sendEngine->clearBuffer();
    this->sendEngine->addByte(BSC_CONTROL_PAD);
    this->sendEngine->addByte(BSC_CONTROL_LEADING_PAD);
    this->sendEngine->addByte(BSC_CONTROL_LEADING_PAD);
    this->sendEngine->addByte(BSC_CONTROL_SYN);
    this->sendEngine->addByte(BSC_CONTROL_SYN);

    this->sendEngine->addByte(BSC_CONTROL_DLE);
    this->sendEngine->addByte(BSC_CONTROL_STX);

    this->sendEngine->addByte(0x27);        // ESC
    this->sendEngine->addByte(0xF5);        // EW
    this->sendEngine->addByte(0x42);        // WCC

    this->sendEngine->addByte(0x11);        // SBA
    this->sendEngine->addByte(0x40);        // 0x000
    this->sendEngine->addByte(0x40);

    this->sendEngine->addByte(0x1D);        // SF
    this->sendEngine->addByte(0x60);
    this->sendEngine->addByte(0xC8);        // HELLO WORLD
    this->sendEngine->addByte(0xC5);
    this->sendEngine->addByte(0xD3);
    this->sendEngine->addByte(0xD3);
    this->sendEngine->addByte(0xD6);
    this->sendEngine->addByte(0x40);
    this->sendEngine->addByte(0xE6);
    this->sendEngine->addByte(0xD6);
    this->sendEngine->addByte(0xD9);
    this->sendEngine->addByte(0xD3);
    this->sendEngine->addByte(0xC4);
    this->sendEngine->addByte(0x40);
    this->sendEngine->addByte(0x40);
    // this->sendEngine->addByte(0x40);

    this->sendEngine->addByte(0x13);        // IC

    this->sendEngine->addByte(BSC_CONTROL_DLE);
    this->sendEngine->addByte(BSC_CONTROL_ETX);
    this->sendEngine->addByte(0x6C);        // BCC
    this->sendEngine->addByte(0x16);

    this->sendEngine->addByte(BSC_CONTROL_PAD);

    sendEngine->startSending();
    sendEngine->stopSendingOnIdle();
    sendEngine->waitForSendIdle();

    sendResponse("Sent EW / HELLO WORLD to station/device");

}

void CommandProcessorText::execRead() {
    receiveEngine->startReceiving();
    sendDebug("Reading response ...");

    if ( receiveEngine->waitReceivedFrameComplete(RECEIVE_TIMEOUT) < 0 ) {
        DataBuffer * frame = receiveEngine->getDataBuffer();
        this->useSerial->print(F("Error: Timeout. We have "));
        this->useSerial->print(frame->getLength());
        this->useSerial->print(F(" bytes of data received, receive state is "));
        this->useSerial->println((int)receiveEngine->receiveState);
        this->useSerial->print(F("Error: _inputBitBuffer = "));
        this->useSerial->println(receiveEngine->_inputBitBuffer, 1);
        sendResponse("Error: Response timeout");
    } else {
        DataBuffer * frame = receiveEngine->getSavedFrame();
        this->useSerial->println(F("Response received, "));
        this->useSerial->print(frame->getLength());
        this->useSerial->println(F(" bytes of received data follows ..."));
        for ( int x = 0; x < frame->getLength(); x++ ) {
            if ( frame->get(x) < 16 )
                this->useSerial->print("0x0");
            else
                this->useSerial->print("0x");
            this->useSerial->print(frame->get(x), 16);
            this->useSerial->print(' ');
        }
        this->useSerial->println(F("**END**"));
    }

}

CommandProcessorFrontEnd::CommandProcessorFrontEnd(SyncBitBanger * bitBanger) {
    this->syncBitBanger = bitBanger;
    //Serial.println(F("Creating SyncControl instance."));
    this->syncControl = new SyncControl(bitBanger);

    //Serial.println(F("Creating CommandProcessorBinary instance."));
    cmdProcessor = new CommandProcessorBinary(
        bitBanger->sendEngine,
        bitBanger->receiveEngine,
        syncControl);
    //Serial.println(F("CommandProcessorFrontEnd constructor complete."));
}

CommandProcessorFrontEnd::~CommandProcessorFrontEnd() {
    delete this->syncControl;
    delete this->cmdProcessor;
}

unsigned long CommandProcessorFrontEnd::getAndProcessCommand() {
    unsigned long lastReceived;

    //this->cmdProcessor->sendDebugToHost("Calling cmdProcessor->getAndProcessCommand()");

    lastReceived = this->cmdProcessor->getAndProcessCommand();
    if ( this->cmdProcessor->isSwitchCommandModeRequired() ) {
        //this->cmdProcessor->sendDebugToHost("Switching command mode.");
        uint8_t newType = this->cmdProcessor->getNewCommandMode();

        switch( newType ) {
            case HOST_CMD_MODE_BINARY:
                //this->cmdProcessor->sendDebugToHost("Switching command mode to binary.");
                delete this->cmdProcessor;
                this->cmdProcessor = new CommandProcessorBinary(
                    this->syncBitBanger->sendEngine,
                    this->syncBitBanger->receiveEngine,
                    this->syncControl);
                break;

            case HOST_CMD_MODE_TEXT:
                //this->cmdProcessor->sendDebugToHost("Switching command mode to text.");
                delete this->cmdProcessor;
                this->cmdProcessor = new CommandProcessorText(
                    this->syncBitBanger->sendEngine,
                    this->syncBitBanger->receiveEngine,
                    this->syncControl);
                break;
        }
    }
    return lastReceived;
}
