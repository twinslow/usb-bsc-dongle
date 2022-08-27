#include <Arduino.h>
#include <unity.h>

#include "SendEngine.h"

#define RXD_PIN 9

void test_SendEngine_constructor(void) {
    SendEngine eng(RXD_PIN);

    TEST_ASSERT_EQUAL(portOutputRegister(digitalPinToPort(RXD_PIN)), eng.getRxdPort());
    TEST_ASSERT_EQUAL(digitalPinToBitMask(RXD_PIN), eng.getRxdBitMask());
    TEST_ASSERT_EQUAL((uint8_t)~digitalPinToBitMask(RXD_PIN), eng.getRxdBitNotMask());
}

void test_SendEngine_sendBit(void) {
    SendEngine eng(RXD_PIN);
    uint8_t dataByte;
    DataBuffer & db = eng.getDataBuffer();

    // Sanity tests
    dataByte = 0x69;
    TEST_ASSERT_EQUAL(0b01101001, dataByte);
    dataByte = dataByte >> 1;
    TEST_ASSERT_EQUAL(0b00110100, dataByte);

    TEST_ASSERT_EQUAL(1, eng.addByte(0x69));  // 01101001
    TEST_ASSERT_EQUAL(2, eng.addByte(0x31));  // 00110001

    eng.startSending();
    TEST_ASSERT_EQUAL( SEND_STATE_XMIT, eng.xmitState );

    TEST_ASSERT_EQUAL( 0, eng.getBitBufferLength());
    // db = eng.getDataBuffer();
    TEST_ASSERT_EQUAL( 2, db.getLength() );
    TEST_ASSERT_EQUAL( 0x69, db.get(0) );
    TEST_ASSERT_EQUAL( 0x31, db.get(1) );
    TEST_ASSERT_EQUAL( 0, db.getPos() );

    TEST_ASSERT_EQUAL(2, eng.getRemainingDataToBeSent());

    eng.sendBit();
    TEST_ASSERT_EQUAL( 1, eng.lastBitSent );
    TEST_ASSERT_EQUAL( 7, eng.getBitBufferLength());

    // db = eng.getDataBuffer();
    TEST_ASSERT_EQUAL( 2, db.getLength() );
    TEST_ASSERT_EQUAL( 0x69, db.get(0) );
    TEST_ASSERT_EQUAL( 0x31, db.get(1) );
    TEST_ASSERT_EQUAL( 1, db.getPos() );

    TEST_ASSERT_EQUAL( SEND_STATE_XMIT, eng.xmitState );

    TEST_ASSERT_EQUAL( 0b01101001, eng._savedBitBuffer );
    TEST_ASSERT_EQUAL( 0b00110100, eng.getBitBuffer() );

    eng.sendBit();
    TEST_ASSERT_EQUAL( 0, eng.lastBitSent );
    TEST_ASSERT_EQUAL( 6, eng.getBitBufferLength());
    TEST_ASSERT_EQUAL( 0b00011010, eng.getBitBuffer() );

    eng.sendBit();
    TEST_ASSERT_EQUAL( 0, eng.lastBitSent );
    eng.sendBit();
    TEST_ASSERT_EQUAL( 1, eng.lastBitSent );
    eng.sendBit();
    TEST_ASSERT_EQUAL( 0, eng.lastBitSent );
    eng.sendBit();
    TEST_ASSERT_EQUAL( 1, eng.lastBitSent );
    eng.sendBit();
    TEST_ASSERT_EQUAL( 1, eng.lastBitSent );
    eng.sendBit();
    TEST_ASSERT_EQUAL( 0, eng.lastBitSent );
    TEST_ASSERT_EQUAL( 0, eng.getBitBufferLength());
    TEST_ASSERT_EQUAL( 0b00000000, eng.getBitBuffer() );

    TEST_ASSERT_EQUAL(1, eng.getRemainingDataToBeSent());

    eng.sendBit();
    TEST_ASSERT_EQUAL( 1, eng.lastBitSent );
    TEST_ASSERT_EQUAL( 7, eng.getBitBufferLength());
    TEST_ASSERT_EQUAL( 0b00011000, eng.getBitBuffer() );
    eng.sendBit();
    TEST_ASSERT_EQUAL( 0, eng.lastBitSent );
    TEST_ASSERT_EQUAL( 6, eng.getBitBufferLength());
    TEST_ASSERT_EQUAL( 0b00001100, eng.getBitBuffer() );
    eng.sendBit();
    TEST_ASSERT_EQUAL( 0, eng.lastBitSent );
    eng.sendBit();
    TEST_ASSERT_EQUAL( 0, eng.lastBitSent );
    eng.sendBit();
    TEST_ASSERT_EQUAL( 1, eng.lastBitSent );
    eng.sendBit();
    TEST_ASSERT_EQUAL( 1, eng.lastBitSent );
    eng.sendBit();
    TEST_ASSERT_EQUAL( 0, eng.lastBitSent );
    eng.sendBit();
    TEST_ASSERT_EQUAL( 0, eng.lastBitSent );

    TEST_ASSERT_EQUAL(0, eng.getRemainingDataToBeSent());

    // Idle/SYN being sent as no more data in buffer.
    eng.sendBit();
    TEST_ASSERT_EQUAL( 0, eng.lastBitSent );
    eng.sendBit();
    TEST_ASSERT_EQUAL( 1, eng.lastBitSent );
    eng.sendBit();
    TEST_ASSERT_EQUAL( 0, eng.lastBitSent );
    eng.sendBit();
    TEST_ASSERT_EQUAL( 0, eng.lastBitSent );
    eng.sendBit();
    TEST_ASSERT_EQUAL( 1, eng.lastBitSent );
    eng.sendBit();
    TEST_ASSERT_EQUAL( 1, eng.lastBitSent );
    eng.sendBit();
    TEST_ASSERT_EQUAL( 0, eng.lastBitSent );
    eng.sendBit();
    TEST_ASSERT_EQUAL( 0, eng.lastBitSent );
 }

void test_SendEngine_startSending(void) {
    SendEngine eng(RXD_PIN);

    TEST_ASSERT_EQUAL(SEND_STATE_OFF, eng.xmitState);
    eng.startSending();
    TEST_ASSERT_EQUAL(SEND_STATE_XMIT, eng.xmitState);
}

void test_SendEngine_stopSending(void) {
    SendEngine eng(RXD_PIN);

    eng.startSending();
    TEST_ASSERT_EQUAL(SEND_STATE_XMIT, eng.xmitState);
    eng.stopSending();
    TEST_ASSERT_EQUAL(SEND_STATE_OFF, eng.xmitState);
}

void test_SendEngine_stopSendingOnIdle1(void) {
    SendEngine eng(RXD_PIN);
    DataBuffer & db = eng.getDataBuffer();

    TEST_ASSERT_EQUAL(1, eng.addByte(0x69));  // 01101001

    eng.startSending();
    TEST_ASSERT_EQUAL( SEND_STATE_XMIT, eng.xmitState );

    TEST_ASSERT_EQUAL(1, eng.getRemainingDataToBeSent());

    eng.sendBit();
    TEST_ASSERT_EQUAL( 1, eng.lastBitSent );
    eng.sendBit();
    TEST_ASSERT_EQUAL( 0, eng.lastBitSent );
    eng.sendBit();
    TEST_ASSERT_EQUAL( 0, eng.lastBitSent );
    eng.sendBit();
    TEST_ASSERT_EQUAL( 1, eng.lastBitSent );
    eng.sendBit();
    TEST_ASSERT_EQUAL( 0, eng.lastBitSent );
    eng.sendBit();
    TEST_ASSERT_EQUAL( 1, eng.lastBitSent );
    eng.sendBit();
    TEST_ASSERT_EQUAL( 1, eng.lastBitSent );
    eng.sendBit();
    TEST_ASSERT_EQUAL( 0, eng.lastBitSent );

    TEST_ASSERT_EQUAL(0, eng.getRemainingDataToBeSent());

    // Idle/SYN being sent as no more data in buffer.
    eng.sendBit();
    TEST_ASSERT_EQUAL( 0, eng.lastBitSent );
    eng.sendBit();

    // We issue the stop sending on idle in the middle of this byte...
    eng.stopSendingOnIdle();
    TEST_ASSERT_EQUAL( 1, eng.lastBitSent );
    eng.sendBit();
    TEST_ASSERT_EQUAL( 0, eng.lastBitSent );
    eng.sendBit();
    TEST_ASSERT_EQUAL( 0, eng.lastBitSent );
    eng.sendBit();
    TEST_ASSERT_EQUAL( 1, eng.lastBitSent );
    eng.sendBit();
    TEST_ASSERT_EQUAL( 1, eng.lastBitSent );
    eng.sendBit();
    TEST_ASSERT_EQUAL( 0, eng.lastBitSent );
    eng.sendBit();
    TEST_ASSERT_EQUAL( 0, eng.lastBitSent );

    // As the last bit was sent, we should now be xmit state of off.
    eng.sendBit();          // One more sendbit, which won't actually do anything other
                            // change the xmitState.
    TEST_ASSERT_EQUAL(SEND_STATE_OFF, eng.xmitState);

}

void test_SendEngine_stopSendingOnIdle2(void) {
    SendEngine eng(RXD_PIN);

    TEST_ASSERT_EQUAL(1, eng.addByte(0x69));  // 01101001

    eng.startSending();
    TEST_ASSERT_EQUAL( SEND_STATE_XMIT, eng.xmitState );

    TEST_ASSERT_EQUAL(1, eng.getRemainingDataToBeSent());

    eng.sendBit();
    TEST_ASSERT_EQUAL( 1, eng.lastBitSent );
    eng.sendBit();
    TEST_ASSERT_EQUAL( 0, eng.lastBitSent );
    eng.sendBit();
    TEST_ASSERT_EQUAL( 0, eng.lastBitSent );
    eng.sendBit();
    TEST_ASSERT_EQUAL( 1, eng.lastBitSent );
    eng.sendBit();
    TEST_ASSERT_EQUAL( 0, eng.lastBitSent );
    eng.sendBit();
    TEST_ASSERT_EQUAL( 1, eng.lastBitSent );
    eng.sendBit();

    // We issue the stop sending on idle in the middle of this byte...
    eng.stopSendingOnIdle();

    TEST_ASSERT_EQUAL( 1, eng.lastBitSent );
    eng.sendBit();
    TEST_ASSERT_EQUAL( 0, eng.lastBitSent );

    TEST_ASSERT_EQUAL(0, eng.getRemainingDataToBeSent());

    // As the last bit was sent, we should now be xmit state of off.
    eng.sendBit();          // One more sendbit, which won't actually do anything other
                            // change the xmitState.

    TEST_ASSERT_EQUAL(SEND_STATE_OFF, eng.xmitState);
 }

void test_SendEngine_clearBuffer(void) {
    SendEngine eng(RXD_PIN);
    DataBuffer & db = eng.getDataBuffer();

    TEST_ASSERT_EQUAL(1, eng.addByte(0x69));  // 01101001

    eng.startSending();
    TEST_ASSERT_EQUAL( SEND_STATE_XMIT, eng.xmitState );

    TEST_ASSERT_EQUAL(1, eng.getRemainingDataToBeSent());

    eng.sendBit();
    TEST_ASSERT_EQUAL( 1, eng.lastBitSent );
    eng.sendBit();
    TEST_ASSERT_EQUAL( 0, eng.lastBitSent );

    eng.clearBuffer();
    TEST_ASSERT_EQUAL( 0, eng.getBitBufferLength() );
    TEST_ASSERT_EQUAL( 0, eng.getRemainingDataToBeSent() );
    TEST_ASSERT_EQUAL( SEND_STATE_OFF, eng.xmitState );

    TEST_ASSERT_EQUAL( 0, db.getPos() );
    TEST_ASSERT_EQUAL( 0, db.getLength() );

 }


void test_SendEngine() {
    RUN_TEST(test_SendEngine_constructor);
    RUN_TEST(test_SendEngine_sendBit);
    RUN_TEST(test_SendEngine_startSending);
    RUN_TEST(test_SendEngine_stopSending);
    RUN_TEST(test_SendEngine_stopSendingOnIdle1);
    RUN_TEST(test_SendEngine_stopSendingOnIdle2);
    RUN_TEST(test_SendEngine_clearBuffer);
}