#include <Arduino.h>
#include <unity.h>

#include "ReceiveEngine.h"

#define TXD_PIN 3
#define CTS_PIN 8

struct {
    unsigned long startTime;
    unsigned long endTime;
    long callCount;
    unsigned long totalCallTime;
    unsigned long minCallTime;
    unsigned long maxCallTime;
    char printbuff[256];
} functionTiming;

void runProcessBit(ReceiveEngine &eng) {
    unsigned long callTime;

    functionTiming.startTime = micros();
    eng.processBit();
    functionTiming.endTime = micros();
    callTime = functionTiming.endTime - functionTiming.startTime;
    functionTiming.callCount++;
    functionTiming.totalCallTime += callTime;
    if ( functionTiming.minCallTime == 0 )
        functionTiming.minCallTime = callTime;
    else
        functionTiming.minCallTime = min(functionTiming.minCallTime, callTime);
    functionTiming.maxCallTime = max(functionTiming.maxCallTime, callTime);

}


#define TIME_FUNCTION(func) \
    runTimeFunction(&(func));

void resetFunctionTime() {
    functionTiming.callCount = 0;
    functionTiming.totalCallTime = 0;
    functionTiming.minCallTime = 0;
    functionTiming.maxCallTime = 0;
}

void reportFunctionTime(char *name) {
    // Serial.print("Average time for function ");
    // Serial.print(name);
    // Serial.print(" was ");
    // Serial.print(functionTiming.totalCallTime / functionTiming.callCount);
    // Serial.println(" microseconds.");
    sprintf(functionTiming.printbuff,
        "Average time for function %s was %lu microseconds over %ld calls. Max call time was %lu microseconds and min call time was %lu microseconds.",
        name,
        (functionTiming.totalCallTime / functionTiming.callCount),
        functionTiming.callCount,
        functionTiming.maxCallTime, functionTiming.minCallTime);
    TEST_MESSAGE(functionTiming.printbuff);
}

void test_ReceiveEngine_constructor(void) {
    ReceiveEngine eng(TXD_PIN, CTS_PIN);

    TEST_ASSERT_EQUAL(portInputRegister(digitalPinToPort(TXD_PIN)), eng.getTxdPort());
    TEST_ASSERT_EQUAL(digitalPinToBitMask(TXD_PIN), eng.getTxdBitMask());
    TEST_ASSERT_EQUAL((uint8_t)~digitalPinToBitMask(TXD_PIN), eng.getTxdBitNotMask());
    TEST_ASSERT_EQUAL((uint8_t) CTS_PIN, eng.getCtsPin());

    TEST_ASSERT_EQUAL(0, eng.getDataBuffer()->getLength());
}

void test_ReceiveEngine_getBit(void) {
    ReceiveEngine eng(TXD_PIN, CTS_PIN);

    eng.getBit(0x01);
    TEST_ASSERT_EQUAL(0b00000001, eng.getBitBuffer());
    eng.getBit(0x00);
    TEST_ASSERT_EQUAL(0b00000010, eng.getBitBuffer());
    eng.getBit(0x00);
    TEST_ASSERT_EQUAL(0b00000100, eng.getBitBuffer());
    eng.getBit(0x01);
    TEST_ASSERT_EQUAL(0b00001001, eng.getBitBuffer());
    eng.getBit(0x01);
    TEST_ASSERT_EQUAL(0b00010011, eng.getBitBuffer());
    eng.getBit(0x00);
    TEST_ASSERT_EQUAL(0b00100110, eng.getBitBuffer());
}

void test_ReceiveEngine_processBit_outOfSync(void) {
    ReceiveEngine eng(TXD_PIN, CTS_PIN);

    eng.setBitBuffer(0x11);
    eng.receiveState = RECEIVE_STATE_OUT_OF_SYNC;

    runProcessBit(eng);

    TEST_ASSERT_EQUAL(RECEIVE_STATE_OUT_OF_SYNC, eng.receiveState);
    TEST_ASSERT_FALSE(eng.getInCharSync());

    eng.setBitBuffer(0x32);     // Sync bit pattern
    runProcessBit(eng);

    TEST_ASSERT_EQUAL(RECEIVE_STATE_IDLE, eng.receiveState);
    TEST_ASSERT_TRUE(eng.getInCharSync());
}

void test_ReceiveEngine_processBit_STX_ETX(void) {
    ReceiveEngine eng(TXD_PIN, CTS_PIN);

    eng.startReceiving();

    eng.setBitBuffer(0x11);
    runProcessBit(eng);

    eng.setBitBuffer(0x32);     // Sync bit pattern
    runProcessBit(eng);
    TEST_ASSERT_EQUAL(RECEIVE_STATE_IDLE, eng.receiveState);
    TEST_ASSERT_TRUE(eng.getInCharSync());

    TEST_ASSERT_EQUAL(0x32, eng.getFrameDataByte(0));
    TEST_ASSERT_EQUAL(1, eng.getFrameLength());

    eng.setBitBuffer(0x02);     // STX
    runProcessBit(eng);

    TEST_ASSERT_EQUAL(RECEIVE_STATE_DATA, eng.receiveState);
    TEST_ASSERT_EQUAL(2, eng.getFrameLength());
    TEST_ASSERT_EQUAL(0x02, eng.getFrameDataByte(1));

    eng.setBitBuffer(0xC1);     // 'A'
    runProcessBit(eng);
    TEST_ASSERT_EQUAL(RECEIVE_STATE_DATA, eng.receiveState);
    TEST_ASSERT_EQUAL(3, eng.getFrameLength());
    TEST_ASSERT_EQUAL(0xC1, eng.getFrameDataByte(2));

    eng.setBitBuffer(0xC2);     // 'B'
    runProcessBit(eng);
    TEST_ASSERT_EQUAL(RECEIVE_STATE_DATA, eng.receiveState);
    TEST_ASSERT_EQUAL(4, eng.getFrameLength());
    TEST_ASSERT_EQUAL(0xC2, eng.getFrameDataByte(3));
    TEST_ASSERT_FALSE(eng.isFrameComplete());

    eng.setBitBuffer(0x03);     // ETX
    runProcessBit(eng);
    TEST_ASSERT_EQUAL(RECEIVE_STATE_BCC1, eng.receiveState);
    TEST_ASSERT_EQUAL(5, eng.getFrameLength());
    TEST_ASSERT_EQUAL(0x03, eng.getFrameDataByte(4));
    TEST_ASSERT_FALSE(eng.isFrameComplete());

    eng.setBitBuffer(0x44);     // first BCC byte
    runProcessBit(eng);
    TEST_ASSERT_FALSE(eng.isFrameComplete());
    TEST_ASSERT_EQUAL(RECEIVE_STATE_BCC2, eng.receiveState);
    TEST_ASSERT_EQUAL(6, eng.getFrameLength());
    TEST_ASSERT_EQUAL(0x44, eng.getFrameDataByte(5));

    eng.setBitBuffer(0x55);     // second BCC byte
    runProcessBit(eng);
    TEST_ASSERT_FALSE(eng.isFrameComplete());
    TEST_ASSERT_EQUAL(RECEIVE_STATE_PAD, eng.receiveState);
    TEST_ASSERT_EQUAL(7, eng.getFrameLength());
    TEST_ASSERT_EQUAL(0x55, eng.getFrameDataByte(6));

    eng.setBitBuffer(0xFF);     // Pad byte
    runProcessBit(eng);
    TEST_ASSERT_EQUAL(RECEIVE_STATE_IDLE, eng.receiveState);
    TEST_ASSERT_TRUE(eng.isFrameComplete());

    DataBufferReadOnly * completedFrame = eng.getSavedFrame();
    TEST_ASSERT_EQUAL(8, completedFrame->getLength());
    TEST_ASSERT_EQUAL(0xFF, completedFrame->get(7));

    // This should no longer be true!
    TEST_ASSERT_FALSE(eng.isFrameComplete());

    // Buffer should be clear
    TEST_ASSERT_EQUAL(0, eng.getFrameLength());

    // Start receiving the next frame ...
    eng.setBitBuffer(0x32);     // Sync for next frame
    runProcessBit(eng);
    eng.setBitBuffer(0x02);     // STX
    runProcessBit(eng);

    TEST_ASSERT_EQUAL(2, eng.getFrameLength());
    TEST_ASSERT_EQUAL(0x32, eng.getFrameDataByte(0));
    TEST_ASSERT_EQUAL(0x02, eng.getFrameDataByte(1));
}

void test_ReceiveEngine_processBit_SOH_STX_ETX(void) {
    ReceiveEngine eng(TXD_PIN, CTS_PIN);

    eng.startReceiving();

    eng.setBitBuffer(0x11);
    runProcessBit(eng);

    eng.setBitBuffer(0x32);     // Sync bit pattern
    runProcessBit(eng);
    TEST_ASSERT_EQUAL(RECEIVE_STATE_IDLE, eng.receiveState);
    TEST_ASSERT_TRUE(eng.getInCharSync());

    TEST_ASSERT_EQUAL(0x32, eng.getFrameDataByte(0));
    TEST_ASSERT_EQUAL(1, eng.getFrameLength());

    eng.setBitBuffer(0x01);     // SOH
    runProcessBit(eng);

    TEST_ASSERT_EQUAL(RECEIVE_STATE_DATA, eng.receiveState);
    TEST_ASSERT_EQUAL(2, eng.getFrameLength());
    TEST_ASSERT_EQUAL(0x01, eng.getFrameDataByte(1));

    eng.setBitBuffer(0xF1);     // '1'
    runProcessBit(eng);
    TEST_ASSERT_EQUAL(RECEIVE_STATE_DATA, eng.receiveState);
    TEST_ASSERT_EQUAL(3, eng.getFrameLength());
    TEST_ASSERT_EQUAL(0xF1, eng.getFrameDataByte(2));

    eng.setBitBuffer(0x02);     // STX
    runProcessBit(eng);

    TEST_ASSERT_EQUAL(RECEIVE_STATE_DATA, eng.receiveState);
    TEST_ASSERT_EQUAL(4, eng.getFrameLength());
    TEST_ASSERT_EQUAL(0x02, eng.getFrameDataByte(3));

    eng.setBitBuffer(0xC1);     // 'A'
    runProcessBit(eng);
    TEST_ASSERT_EQUAL(RECEIVE_STATE_DATA, eng.receiveState);
    TEST_ASSERT_EQUAL(5, eng.getFrameLength());
    TEST_ASSERT_EQUAL(0xC1, eng.getFrameDataByte(4));

    eng.setBitBuffer(0xC2);     // 'B'
    runProcessBit(eng);
    TEST_ASSERT_EQUAL(RECEIVE_STATE_DATA, eng.receiveState);
    TEST_ASSERT_EQUAL(6, eng.getFrameLength());
    TEST_ASSERT_EQUAL(0xC2, eng.getFrameDataByte(5));

    eng.setBitBuffer(0x26);     // ETB
    runProcessBit(eng);
    TEST_ASSERT_EQUAL(RECEIVE_STATE_BCC1, eng.receiveState);
    TEST_ASSERT_EQUAL(7, eng.getFrameLength());
    TEST_ASSERT_EQUAL(0x26, eng.getFrameDataByte(6));

    eng.setBitBuffer(0x44);     // first BCC byte
    runProcessBit(eng);
    TEST_ASSERT_EQUAL(RECEIVE_STATE_BCC2, eng.receiveState);
    TEST_ASSERT_EQUAL(8, eng.getFrameLength());
    TEST_ASSERT_EQUAL(0x44, eng.getFrameDataByte(7));

    eng.setBitBuffer(0x55);     // second BCC byte
    runProcessBit(eng);
    TEST_ASSERT_EQUAL(RECEIVE_STATE_PAD, eng.receiveState);
    TEST_ASSERT_EQUAL(9, eng.getFrameLength());
    TEST_ASSERT_EQUAL(0x55, eng.getFrameDataByte(8));

    eng.setBitBuffer(0xFF);     // Pad byte
    runProcessBit(eng);
    TEST_ASSERT_EQUAL(RECEIVE_STATE_IDLE, eng.receiveState);
    TEST_ASSERT_TRUE(eng.isFrameComplete());

    // Get the saved frame
    DataBufferReadOnly * completedFrame = eng.getSavedFrame();

    TEST_ASSERT_EQUAL(10, completedFrame->getLength());
    TEST_ASSERT_EQUAL(0xFF, completedFrame->get(9));

    TEST_ASSERT_FALSE(eng.isFrameComplete());
}

void test_ReceiveEngine_processBit_ACK0(void) {
    ReceiveEngine eng(TXD_PIN, CTS_PIN);

    eng.startReceiving();

    eng.setBitBuffer(0x11);
    runProcessBit(eng);

    eng.setBitBuffer(0x32);     // Sync bit pattern
    runProcessBit(eng);
    TEST_ASSERT_EQUAL(RECEIVE_STATE_IDLE, eng.receiveState);
    TEST_ASSERT_TRUE(eng.getInCharSync());

    TEST_ASSERT_EQUAL(0x32, eng.getFrameDataByte(0));
    TEST_ASSERT_EQUAL(1, eng.getFrameLength());

    eng.setBitBuffer(0x10);     // DLE
    runProcessBit(eng);

    TEST_ASSERT_EQUAL(RECEIVE_STATE_IDLE, eng.receiveState);
    TEST_ASSERT_EQUAL(1, eng.getFrameLength());  // length Unchanged

    eng.setBitBuffer(0x70);     // ACK0
    runProcessBit(eng);
    eng.setBitBuffer(0xFF);     // PAD
    runProcessBit(eng);

    TEST_ASSERT_TRUE(eng.isFrameComplete());

    DataBufferReadOnly * completedFrame = eng.getSavedFrame();
    TEST_ASSERT_FALSE(eng.isFrameComplete());

    TEST_ASSERT_EQUAL(RECEIVE_STATE_IDLE, eng.receiveState);
    TEST_ASSERT_EQUAL(4, completedFrame->getLength());
    TEST_ASSERT_EQUAL(0x10, completedFrame->get(1));
    TEST_ASSERT_EQUAL(0x70, completedFrame->get(2));
    TEST_ASSERT_EQUAL(0xFF, completedFrame->get(3));

}

void test_ReceiveEngine_processBit_ACK1(void) {
    ReceiveEngine eng(TXD_PIN, CTS_PIN);

    eng.startReceiving();

    eng.setBitBuffer(0x11);
    runProcessBit(eng);

    eng.setBitBuffer(0x32);     // Sync bit pattern
    runProcessBit(eng);
    TEST_ASSERT_EQUAL(RECEIVE_STATE_IDLE, eng.receiveState);
    TEST_ASSERT_TRUE(eng.getInCharSync());

    TEST_ASSERT_EQUAL(0x32, eng.getFrameDataByte(0));
    TEST_ASSERT_EQUAL(1, eng.getFrameLength());

    eng.setBitBuffer(0x10);     // DLE
    runProcessBit(eng);

    TEST_ASSERT_EQUAL(RECEIVE_STATE_IDLE, eng.receiveState);
    TEST_ASSERT_EQUAL(1, eng.getFrameLength());  // length Unchanged

    eng.setBitBuffer(0x61);     // ACK1
    runProcessBit(eng);
    eng.setBitBuffer(0xFF);     // PAD
    runProcessBit(eng);

    TEST_ASSERT_TRUE(eng.isFrameComplete());

    DataBufferReadOnly * completedFrame = eng.getSavedFrame();
    TEST_ASSERT_FALSE(eng.isFrameComplete());

    TEST_ASSERT_EQUAL(RECEIVE_STATE_IDLE, eng.receiveState);
    TEST_ASSERT_EQUAL(4, completedFrame->getLength());
    TEST_ASSERT_EQUAL(0x10, completedFrame->get(1));
    TEST_ASSERT_EQUAL(0x61, completedFrame->get(2));
    TEST_ASSERT_EQUAL(0xFF, completedFrame->get(3));


}

void test_ReceiveEngine_processBit_NAK(void) {
    ReceiveEngine eng(TXD_PIN, CTS_PIN);

    eng.startReceiving();

    eng.setBitBuffer(0x11);
    runProcessBit(eng);

    eng.setBitBuffer(0x32);     // Sync bit pattern
    runProcessBit(eng);
    TEST_ASSERT_EQUAL(RECEIVE_STATE_IDLE, eng.receiveState);
    TEST_ASSERT_TRUE(eng.getInCharSync());

    TEST_ASSERT_EQUAL(0x32, eng.getFrameDataByte(0));
    TEST_ASSERT_EQUAL(1, eng.getFrameLength());

    eng.setBitBuffer(0x3D);     // NAK
    runProcessBit(eng);

    eng.setBitBuffer(0xFF);     // PAD
    runProcessBit(eng);

    TEST_ASSERT_TRUE(eng.isFrameComplete());

    DataBufferReadOnly * completedFrame = eng.getSavedFrame();
    TEST_ASSERT_FALSE(eng.isFrameComplete());

    TEST_ASSERT_EQUAL(RECEIVE_STATE_IDLE, eng.receiveState);
    TEST_ASSERT_EQUAL(3, completedFrame->getLength());
    TEST_ASSERT_EQUAL(0x3D, completedFrame->get(1));
    TEST_ASSERT_EQUAL(0xFF, completedFrame->get(2));
}

void test_ReceiveEngine_processBit_SYN_EOT(void) {
    ReceiveEngine eng(TXD_PIN, CTS_PIN);

    eng.startReceiving();

    eng.setBitBuffer(0x11);
    runProcessBit(eng);

    eng.setBitBuffer(0x32);     // Sync bit pattern
    runProcessBit(eng);
    TEST_ASSERT_EQUAL(RECEIVE_STATE_IDLE, eng.receiveState);
    TEST_ASSERT_TRUE(eng.getInCharSync());

    TEST_ASSERT_EQUAL(0x32, eng.getFrameDataByte(0));
    TEST_ASSERT_EQUAL(1, eng.getFrameLength());

    eng.setBitBuffer(0x37);     // EOT
    runProcessBit(eng);

    eng.setBitBuffer(0xFF);     // PAD
    runProcessBit(eng);

    TEST_ASSERT_TRUE(eng.isFrameComplete());

    DataBufferReadOnly * completedFrame = eng.getSavedFrame();
    TEST_ASSERT_FALSE(eng.isFrameComplete());

    TEST_ASSERT_EQUAL(RECEIVE_STATE_IDLE, eng.receiveState);
    TEST_ASSERT_EQUAL(3, completedFrame->getLength());
    TEST_ASSERT_EQUAL(0x37, completedFrame->get(1));
    TEST_ASSERT_EQUAL(0xFF, completedFrame->get(2));
}

void test_ReceiveEngine_processBit_Status_Msg(void) {
    ReceiveEngine eng(TXD_PIN, CTS_PIN);
    DataBuffer * db;

    db = eng.getDataBuffer();

    eng.startReceiving();

    eng.setBitBuffer(0x11);     // Something junk
    runProcessBit(eng);

    eng.setBitBuffer(0x32);     // Sync bit pattern
    runProcessBit(eng);
    TEST_ASSERT_EQUAL(RECEIVE_STATE_IDLE, eng.receiveState);
    TEST_ASSERT_TRUE(eng.getInCharSync());

    TEST_ASSERT_EQUAL(0x32, eng.getFrameDataByte(0));
    TEST_ASSERT_EQUAL(1, eng.getFrameLength());

    eng.setBitBuffer(0x32);     // 2nd sync ... discarded
    runProcessBit(eng);
    TEST_ASSERT_EQUAL(1, eng.getFrameLength());

    eng.setBitBuffer(0x01);     // SOH
    runProcessBit(eng);
    TEST_ASSERT_EQUAL(2, eng.getFrameLength());
    TEST_ASSERT_EQUAL(0x01, eng.getFrameDataByte(1));

    eng.setBitBuffer(0x6C);     // %
    runProcessBit(eng);
    TEST_ASSERT_EQUAL(3, eng.getFrameLength());
    TEST_ASSERT_EQUAL(0x6C, eng.getFrameDataByte(2));

    eng.setBitBuffer(0xD9);     // R
    runProcessBit(eng);
    TEST_ASSERT_EQUAL(4, eng.getFrameLength());
    TEST_ASSERT_EQUAL(0xD9, eng.getFrameDataByte(3));

    eng.setBitBuffer(0x02);     // STX
    runProcessBit(eng);
    TEST_ASSERT_EQUAL(5, eng.getFrameLength());
    TEST_ASSERT_EQUAL(0x02, eng.getFrameDataByte(4));

    eng.setBitBuffer(0xC1);     // CU-ADDR-CHAR 'A', dev # 1
    runProcessBit(eng);
    eng.setBitBuffer(0xC1);     // CU-ADDR-CHAR 'A', dev # 1
    runProcessBit(eng);
    eng.setBitBuffer(0x40);     // DEV-ADDR-CHAR ' ', dev # 0
    runProcessBit(eng);
    eng.setBitBuffer(0x40);     // DEV-ADDR-CHAR ' ', dev # 0
    runProcessBit(eng);
    eng.setBitBuffer(0xC9);     // invented SS byte
    runProcessBit(eng);
    eng.setBitBuffer(0xC9);     // invented SS byte
    runProcessBit(eng);
    eng.setBitBuffer(0xC9);     // invented SS byte
    runProcessBit(eng);
    TEST_ASSERT_EQUAL(12, eng.getFrameLength());

    eng.setBitBuffer(0x03);     // ETX
    runProcessBit(eng);
    TEST_ASSERT_EQUAL(13, eng.getFrameLength());
    TEST_ASSERT_EQUAL(0x03, eng.getFrameDataByte(12));

    eng.setBitBuffer(0xF1);     // BCC1
    runProcessBit(eng);
    TEST_ASSERT_EQUAL(14, eng.getFrameLength());
    TEST_ASSERT_EQUAL(0xF1, eng.getFrameDataByte(13));

    eng.setBitBuffer(0xF2);     // BCC2
    runProcessBit(eng);
    TEST_ASSERT_EQUAL(15, eng.getFrameLength());
    TEST_ASSERT_EQUAL(0xF2, eng.getFrameDataByte(14));

    // As a test here, how long does it take to execute a newReadOnlyCopy
    // on the data buffer.
    {
    unsigned long startTime = micros();
    DataBufferReadOnly * rocopy = db->newReadOnlyCopy();
    unsigned long callTime = micros() - startTime;
    delete rocopy;

    sprintf(functionTiming.printbuff,
        "It took %lu microseconds to make a read only copy of the data buffer.",
        callTime);
    TEST_MESSAGE(functionTiming.printbuff);
    }

    {
    unsigned long startTime = micros();
    void * someMemory = malloc(15);
    memcpy(someMemory, "123456789012345", 15);
    unsigned long callTime = micros() - startTime;
    sprintf(functionTiming.printbuff,
        "It took %lu microseconds to malloc and copy 15 bytes of memory.",
        callTime);
    TEST_MESSAGE(functionTiming.printbuff);
    // startTime = micros();
    // memcpy(someMemory, "123456789012345", 15);
    // callTime = micros() - startTime;
    // sprintf(functionTiming.printbuff,
    //     "It took %lu microseconds to copy 15 bytes into that memory.",
    //     callTime);
    // TEST_MESSAGE(functionTiming.printbuff);
    }

    eng.setBitBuffer(0xFF);     // PAD
    runProcessBit(eng);

    DataBufferReadOnly * completedFrame = eng.getSavedFrame();
    TEST_ASSERT_FALSE(eng.isFrameComplete());

    TEST_ASSERT_EQUAL(RECEIVE_STATE_IDLE, eng.receiveState);
    TEST_ASSERT_EQUAL(16, completedFrame->getLength());
    TEST_ASSERT_EQUAL(0xFF, completedFrame->get(15));

}

void test_ReceiveEngine_processBit_Read_Partition1(void) {
    ReceiveEngine eng(TXD_PIN, CTS_PIN);

    eng.startReceiving();

    eng.setBitBuffer(0x11);     // Something junk
    runProcessBit(eng);

    eng.setBitBuffer(0x32);     // Sync bit pattern
    runProcessBit(eng);
    TEST_ASSERT_EQUAL(RECEIVE_STATE_IDLE, eng.receiveState);
    TEST_ASSERT_TRUE(eng.getInCharSync());

    TEST_ASSERT_EQUAL(0x32, eng.getFrameDataByte(0));
    TEST_ASSERT_EQUAL(1, eng.getFrameLength());

    eng.setBitBuffer(0x32);     // 2nd sync ... discarded
    runProcessBit(eng);
    TEST_ASSERT_EQUAL(1, eng.getFrameLength());

    eng.setBitBuffer(0x10);     // DLE
    runProcessBit(eng);

    eng.setBitBuffer(0x02);     // STX
    runProcessBit(eng);
    TEST_ASSERT_EQUAL(3, eng.getFrameLength());
    TEST_ASSERT_EQUAL(0x10, eng.getFrameDataByte(1));
    TEST_ASSERT_EQUAL(0x02, eng.getFrameDataByte(2));

    TEST_ASSERT_EQUAL(RECEIVE_STATE_TRANSPARENT_DATA, eng.receiveState);

    eng.setBitBuffer(0xC1);     // CU-ADDR-CHAR 'A', dev # 1
    runProcessBit(eng);
    eng.setBitBuffer(0xC1);     // CU-ADDR-CHAR 'A', dev # 1
    runProcessBit(eng);
    eng.setBitBuffer(0x40);     // DEV-ADDR-CHAR ' ', dev # 0
    runProcessBit(eng);
    eng.setBitBuffer(0x40);     // DEV-ADDR-CHAR ' ', dev # 0
    runProcessBit(eng);
    eng.setBitBuffer(0xC1);     // Message text ...
    runProcessBit(eng);
    eng.setBitBuffer(0xC2);     // invented SS byte
    runProcessBit(eng);
    eng.setBitBuffer(0xC3);     // invented SS byte
    runProcessBit(eng);
    TEST_ASSERT_EQUAL(10, eng.getFrameLength());

    eng.setBitBuffer(0x10);     // DLE
    runProcessBit(eng);

    eng.setBitBuffer(0x03);     // ETX
    runProcessBit(eng);
    TEST_ASSERT_EQUAL(12, eng.getFrameLength());
    TEST_ASSERT_EQUAL(0x10, eng.getFrameDataByte(10));
    TEST_ASSERT_EQUAL(0x03, eng.getFrameDataByte(11));

    eng.setBitBuffer(0xF1);     // BCC1
    runProcessBit(eng);
    TEST_ASSERT_EQUAL(13, eng.getFrameLength());
    TEST_ASSERT_EQUAL(0xF1, eng.getFrameDataByte(12));

    eng.setBitBuffer(0xF2);     // BCC2
    runProcessBit(eng);
    TEST_ASSERT_EQUAL(14, eng.getFrameLength());
    TEST_ASSERT_EQUAL(0xF2, eng.getFrameDataByte(13));

    eng.setBitBuffer(0xFF);     // PAD
    runProcessBit(eng);

    DataBufferReadOnly * completedFrame = eng.getSavedFrame();
    TEST_ASSERT_FALSE(eng.isFrameComplete());

    TEST_ASSERT_EQUAL(RECEIVE_STATE_IDLE, eng.receiveState);
    TEST_ASSERT_EQUAL(15, completedFrame->getLength());
    TEST_ASSERT_EQUAL(0xFF, completedFrame->get(14));

}



void test_ReceiveEngine() {
    resetFunctionTime();

    RUN_TEST(test_ReceiveEngine_constructor);
    RUN_TEST(test_ReceiveEngine_getBit);
    RUN_TEST(test_ReceiveEngine_processBit_outOfSync);
    RUN_TEST(test_ReceiveEngine_processBit_STX_ETX);
    RUN_TEST(test_ReceiveEngine_processBit_SOH_STX_ETX);
    RUN_TEST(test_ReceiveEngine_processBit_ACK0);
    RUN_TEST(test_ReceiveEngine_processBit_ACK1);
    RUN_TEST(test_ReceiveEngine_processBit_NAK);
    RUN_TEST(test_ReceiveEngine_processBit_SYN_EOT);
    RUN_TEST(test_ReceiveEngine_processBit_Status_Msg);
    RUN_TEST(test_ReceiveEngine_processBit_Read_Partition1);

    reportFunctionTime("eng.processBit()");
}

