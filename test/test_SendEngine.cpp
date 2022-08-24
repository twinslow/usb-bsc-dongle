#include <Arduino.h>
#include <unity.h>

#include "SendEngine.h"

#define RXD_PIN 9

void test_SendEngine_constructor(void) {
    SendEngine eng(RXD_PIN);
    uint8_t dataByte;

    TEST_ASSERT_EQUAL(portOutputRegister(digitalPinToPort(RXD_PIN)), eng.getRxdPort());
    TEST_ASSERT_EQUAL(digitalPinToBitMask(RXD_PIN), eng.getRxdBitMask());
    TEST_ASSERT_EQUAL((uint8_t)~digitalPinToBitMask(RXD_PIN), eng.getRxdBitNotMask());

}

void test_SendEngine_sendBit(void) {
    SendEngine eng(RXD_PIN);
    uint8_t dataByte;
    volatile uint8_t *port;
    uint8_t pinMask;
    DataBuffer db;

    // Sanity tests
    dataByte = 0x69;
    TEST_ASSERT_EQUAL(0b01101001, dataByte);
    dataByte = dataByte >> 1;
    TEST_ASSERT_EQUAL(0b00110100, dataByte);

    port = portOutputRegister(digitalPinToPort(RXD_PIN));
    pinMask = digitalPinToBitMask(RXD_PIN);

    TEST_ASSERT_EQUAL(1, eng.addByte(0x69));  // 01101001
    TEST_ASSERT_EQUAL(2, eng.addByte(0x31));  // 00110001

    eng.startSending();
    TEST_ASSERT_EQUAL( SEND_STATE_XMIT, eng.xmitState );

    TEST_ASSERT_EQUAL( 0, eng.getBitBufferLength());
    db = eng.getDataBuffer();
    TEST_ASSERT_EQUAL( 2, db.getLength() );
    TEST_ASSERT_EQUAL( 0x69, db.get(0) );
    TEST_ASSERT_EQUAL( 0x31, db.get(1) );
    TEST_ASSERT_EQUAL( 0, db.getPos() );

    eng.sendBit();
    TEST_ASSERT_EQUAL( 1, eng.lastBitSent );
    TEST_ASSERT_EQUAL( 7, eng.getBitBufferLength());

    db = eng.getDataBuffer();
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

}

/*
void test_DataBuffer_write_read_get(void) {
    DataBuffer buff;
    uint8_t inByte, outByte;

    inByte = 'A';
    TEST_ASSERT_EQUAL(1, buff.write(inByte));
    TEST_ASSERT_EQUAL(1, buff.getLength());
    TEST_ASSERT_EQUAL(0x41, buff.readLast());

    inByte = 'B';
    TEST_ASSERT_EQUAL(2, buff.write(inByte));
    TEST_ASSERT_EQUAL(2, buff.getLength());
    TEST_ASSERT_EQUAL(0x42, buff.readLast());

    TEST_ASSERT_EQUAL(0x41, buff.get(0));
    TEST_ASSERT_EQUAL(0x42, buff.get(1));
    TEST_ASSERT_EQUAL(-1, buff.get(2));

    // Read back ...
    TEST_ASSERT_EQUAL(0, buff.read(&outByte));
    TEST_ASSERT_EQUAL(0x41, outByte);
    TEST_ASSERT_EQUAL(2, buff.getLength());

    TEST_ASSERT_EQUAL(1, buff.read(&outByte));
    TEST_ASSERT_EQUAL(0x42, outByte);
    TEST_ASSERT_EQUAL(2, buff.getLength());

    // Read beyond end
    TEST_ASSERT_EQUAL(-1, buff.read(&outByte));

    // Write another ...
    inByte = 'C';
    TEST_ASSERT_EQUAL(3, buff.write(inByte));
    TEST_ASSERT_EQUAL(3, buff.getLength());
    TEST_ASSERT_EQUAL(0x43, buff.readLast());

    TEST_ASSERT_EQUAL(2, buff.read(&outByte));
    TEST_ASSERT_EQUAL(0x43, outByte);
    TEST_ASSERT_EQUAL(3, buff.getLength());

    // Read beyond end
    TEST_ASSERT_EQUAL(-1, buff.read(&outByte));
}

void test_DataBuffer_clear(void) {
    DataBuffer buff;
    uint8_t inByte, outByte;

    inByte = 'A';
    TEST_ASSERT_EQUAL(1, buff.write(inByte));
    TEST_ASSERT_EQUAL(1, buff.getLength());
    TEST_ASSERT_EQUAL(0x41, buff.readLast());

    buff.clear();

    TEST_ASSERT_EQUAL(0, buff.getLength());
    TEST_ASSERT_EQUAL(-1, buff.read(&outByte));
    TEST_ASSERT_EQUAL(-1, buff.get(0));
    TEST_ASSERT_EQUAL(-1, buff.readLast());
}

*/

void test_SendEngine() {
    RUN_TEST(test_SendEngine_constructor);
    RUN_TEST(test_SendEngine_sendBit);
}