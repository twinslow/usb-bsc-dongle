#include <Arduino.h>
#include <unity.h>

#include "CommandProcessor.h"
#include "mock_Serial.h"

#define RXD_PIN 9

class MockSendEngine : public SendEngine {
    public:
        MockSendEngine(uint8_t rxdPin) : SendEngine(rxdPin) {}

        // Stop these methods from doing anything
        virtual void startSending() {};
        virtual void stopSending() {};
        virtual void stopSendingOnIdle() {};
        virtual void waitForSendIdle() {};
};

class MockReceiveEngine : public ReceiveEngine {
    public:
        MockReceiveEngine(uint8_t txdPin, uint8_t ctsPin) :
            ReceiveEngine(txdPin, ctsPin)
        {
        }

        virtual void startReceiving() {
        }
        virtual int waitReceivedFrameComplete(int timeout) {
            return 100;     // This is the time in millseconds to receive the frame
        }
        // char tempBuffer[10];
        virtual DataBuffer * getSavedFrame() {
            return _receiveDataBuffer;
        }
        void loadMockFrame(uint8_t *data, int len) {
            // itoa(len, tempBuffer, 10);
            // TEST_MESSAGE(tempBuffer);
            _receiveDataBuffer->loadData(len, data);
        }
};

class MockSyncControl : public SyncControl {
    public:
        MockSyncControl() : SyncControl(NULL) {}
        virtual void deviceReset() {}
};

MockSendEngine testSendEngine(1);
MockReceiveEngine testReceiveEngine(2,3);
MockSyncControl testSyncControl;

void test_CommandProcessor_constructor(void) {
    CommandProcessorBinary cmdproc(&testSendEngine, &testReceiveEngine, &testSyncControl);

    TEST_ASSERT_EQUAL((void *)&testSendEngine, (void *)cmdproc.sendEngine);
//    TEST_ASSERT_EQUAL((void *)static_cast<ReceiveEngine>(&testReceiveEngine), (void *)cmdproc.receiveEngine);
}

void test_CommandProcessor_getCommand(void) {
    CommandProcessorBinary cmdproc(&testSendEngine, &testReceiveEngine, &testSyncControl);
    cmdproc.enableDebug(false);

    MockSerial.reset();
    cmdproc.injectSerial(&MockSerial);

    byte dummyData[] = {CMD_WRITE, 0x00, 0x03, 0x32, 0x32, 0x37};
    MockSerial.setReadBuffer(dummyData, 6);

    cmdproc.getCommand();

    TEST_ASSERT_EQUAL(CMD_WRITE, cmdproc.getCommandCode());
    TEST_ASSERT_EQUAL(3, cmdproc.getCommandDataLength());
}

void test_CommandProcessor_sendResponse(void) {
    CommandProcessorBinary cmdproc(&testSendEngine, &testReceiveEngine, &testSyncControl);
    cmdproc.enableDebug(false);

    MockSerial.reset();
    cmdproc.injectSerial(&MockSerial);

    byte dummyData[] = {0x32, 0x32, 0x37};
    cmdproc.sendResponse(0x22, 3, dummyData);

    TEST_ASSERT_EQUAL(6, MockSerial.writePtr);
    TEST_ASSERT_EQUAL(0x22, MockSerial.writeBuffer[0]);
    TEST_ASSERT_EQUAL(0x0,  MockSerial.writeBuffer[1]);
    TEST_ASSERT_EQUAL(0x03, MockSerial.writeBuffer[2]);
    TEST_ASSERT_EQUAL(0x32, MockSerial.writeBuffer[3]);
    TEST_ASSERT_EQUAL(0x32, MockSerial.writeBuffer[4]);
    TEST_ASSERT_EQUAL(0x37, MockSerial.writeBuffer[5]);

    MockSerial.reset();

    byte dummyData2[] = {0x32, 0x32};
    cmdproc.sendResponse(0x22, 2, dummyData2);

    TEST_ASSERT_EQUAL(5, MockSerial.writePtr);
    TEST_ASSERT_EQUAL(0x22, MockSerial.writeBuffer[0]);
    TEST_ASSERT_EQUAL(0x0,  MockSerial.writeBuffer[1]);
    TEST_ASSERT_EQUAL(0x02, MockSerial.writeBuffer[2]);
    TEST_ASSERT_EQUAL(0x32, MockSerial.writeBuffer[3]);
    TEST_ASSERT_EQUAL(0x32, MockSerial.writeBuffer[4]);

}

void test_CommandProcessor_process_write(void) {
    CommandProcessorBinary cmdproc(&testSendEngine, &testReceiveEngine, &testSyncControl);
    cmdproc.enableDebug(false); // Important ... turn off the debug msgs that would
                                // otherwise be sent to our mock serial instance.

    MockSerial.reset();
    cmdproc.injectSerial(&MockSerial);

    byte dummyData[] = {CMD_WRITE, 0x00, 0x03, 0x32, 0x32, 0x37};
    MockSerial.setReadBuffer(dummyData, 6);

    cmdproc.process();

    TEST_ASSERT_EQUAL(CMD_WRITE, cmdproc.getCommandCode());
    TEST_ASSERT_EQUAL(3, cmdproc.getCommandDataLength());

    // Test what was copied to the SendEngine data buffer
    TEST_ASSERT_EQUAL(8, testSendEngine.getDataBuffer().getLength());
    TEST_ASSERT_EQUAL(0xFF, testSendEngine.getDataBuffer().get(0));
    TEST_ASSERT_EQUAL(0x55, testSendEngine.getDataBuffer().get(1));
    TEST_ASSERT_EQUAL(0x55, testSendEngine.getDataBuffer().get(2));
    TEST_ASSERT_EQUAL(0x32, testSendEngine.getDataBuffer().get(3));
    TEST_ASSERT_EQUAL(0x32, testSendEngine.getDataBuffer().get(4));
    TEST_ASSERT_EQUAL(0x32, testSendEngine.getDataBuffer().get(5));
    TEST_ASSERT_EQUAL(0x37, testSendEngine.getDataBuffer().get(6));
    TEST_ASSERT_EQUAL(0xFF, testSendEngine.getDataBuffer().get(7));

    // Test what was (mocked) written out on the serial port
    TEST_ASSERT_EQUAL(3, MockSerial.writePtr);
    TEST_ASSERT_EQUAL(CMD_WRITE|CMD_RESPONSE_MASK, MockSerial.writeBuffer[0]);
    TEST_ASSERT_EQUAL(0, MockSerial.writeBuffer[1]);
    TEST_ASSERT_EQUAL(0, MockSerial.writeBuffer[2]);
}

void test_CommandProcessor_process_read(void) {
    CommandProcessorBinary cmdproc(&testSendEngine, &testReceiveEngine, &testSyncControl);
    cmdproc.enableDebug(false); // Important ... turn off the debug msgs that would
                                // otherwise be sent to our mock serial instance.

    byte dummyReceivedFrame[] = { 0x32, 0x10, 0x70 };
    testReceiveEngine.loadMockFrame(dummyReceivedFrame, 3 /*sizeof(dummyReceivedFrame)*/);
    DataBuffer *frameBack = testReceiveEngine.getSavedFrame();
    TEST_ASSERT_EQUAL(3, frameBack->getLength());

    TEST_MESSAGE("Setting up input serial command (READ) data");
    MockSerial.reset();
    cmdproc.injectSerial(&MockSerial);

    byte dummyData[] = {CMD_READ, 0x00, 0x00};
    MockSerial.setReadBuffer(dummyData, sizeof(dummyData));

    TEST_MESSAGE("Calling process()");
    cmdproc.process();
    TEST_MESSAGE("Returned from process()");

    TEST_ASSERT_EQUAL(CMD_READ, cmdproc.getCommandCode());
    TEST_ASSERT_EQUAL(0, cmdproc.getCommandDataLength());

    // Test what was (mocked) written out on the serial port
    TEST_ASSERT_EQUAL(6, MockSerial.writePtr);
    TEST_ASSERT_EQUAL(CMD_READ|CMD_RESPONSE_MASK, MockSerial.writeBuffer[0]);
    TEST_ASSERT_EQUAL(0, MockSerial.writeBuffer[1]);
    TEST_ASSERT_EQUAL(3, MockSerial.writeBuffer[2]);
    TEST_ASSERT_EQUAL(0x32, MockSerial.writeBuffer[3]);
    TEST_ASSERT_EQUAL(0x10, MockSerial.writeBuffer[4]);
    TEST_ASSERT_EQUAL(0x70, MockSerial.writeBuffer[5]);
}

void test_CommandProcessor_process_write_read(void) {
    CommandProcessorBinary cmdproc(&testSendEngine, &testReceiveEngine, &testSyncControl);
    cmdproc.enableDebug(false); // Important ... turn off the debug msgs that would
                                // otherwise be sent to our mock serial instance.

    MockSerial.reset();
    cmdproc.injectSerial(&MockSerial);

    byte dummyData[] = {CMD_WRITE_READ, 0x00, 0x03, 0x32, 0x32, 0x37};
    MockSerial.setReadBuffer(dummyData, 6);

    cmdproc.process();

    TEST_ASSERT_EQUAL(CMD_WRITE_READ, cmdproc.getCommandCode());
    TEST_ASSERT_EQUAL(3, cmdproc.getCommandDataLength());

    // Test what was copied to the SendEngine data buffer
    TEST_ASSERT_EQUAL(8, testSendEngine.getDataBuffer().getLength());
    TEST_ASSERT_EQUAL(0xFF, testSendEngine.getDataBuffer().get(0));
    TEST_ASSERT_EQUAL(0x55, testSendEngine.getDataBuffer().get(1));
    TEST_ASSERT_EQUAL(0x55, testSendEngine.getDataBuffer().get(2));
    TEST_ASSERT_EQUAL(0x32, testSendEngine.getDataBuffer().get(3));
    TEST_ASSERT_EQUAL(0x32, testSendEngine.getDataBuffer().get(4));
    TEST_ASSERT_EQUAL(0x32, testSendEngine.getDataBuffer().get(5));
    TEST_ASSERT_EQUAL(0x37, testSendEngine.getDataBuffer().get(6));
    TEST_ASSERT_EQUAL(0xFF, testSendEngine.getDataBuffer().get(7));

    byte dummyReceivedFrame[] = { 0x32, 0x10, 0x70 };
    testReceiveEngine.loadMockFrame(dummyReceivedFrame, 3 /*sizeof(dummyReceivedFrame)*/);
    DataBuffer *frameBack = testReceiveEngine.getSavedFrame();
    TEST_ASSERT_EQUAL(3, frameBack->getLength());

    // Test what was (mocked) written out on the serial port
    TEST_ASSERT_EQUAL(6, MockSerial.writePtr);
    TEST_ASSERT_EQUAL(CMD_WRITE_READ|CMD_RESPONSE_MASK, MockSerial.writeBuffer[0]);
    TEST_ASSERT_EQUAL(0, MockSerial.writeBuffer[1]);
    TEST_ASSERT_EQUAL(3, MockSerial.writeBuffer[2]);
    TEST_ASSERT_EQUAL(0x32, MockSerial.writeBuffer[3]);
    TEST_ASSERT_EQUAL(0x10, MockSerial.writeBuffer[4]);
    TEST_ASSERT_EQUAL(0x70, MockSerial.writeBuffer[5]);

}


void test_CommandProcessor() {
    RUN_TEST(test_CommandProcessor_constructor);
    RUN_TEST(test_CommandProcessor_getCommand);
    RUN_TEST(test_CommandProcessor_sendResponse);
    RUN_TEST(test_CommandProcessor_process_write);
    RUN_TEST(test_CommandProcessor_process_read);
    RUN_TEST(test_CommandProcessor_process_write_read);
}