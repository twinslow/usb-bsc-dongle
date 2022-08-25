#include <Arduino.h>
#include <unity.h>

#include "ReceiveEngine.h"

#define TXD_PIN 3
#define CTS_PIN 8

void test_ReceiveEngine_constructor(void) {
    ReceiveEngine eng(TXD_PIN, CTS_PIN);

    TEST_ASSERT_EQUAL(portInputRegister(digitalPinToPort(TXD_PIN)), eng.getTxdPort());
    TEST_ASSERT_EQUAL(digitalPinToBitMask(TXD_PIN), eng.getTxdBitMask());
    TEST_ASSERT_EQUAL((uint8_t)~digitalPinToBitMask(TXD_PIN), eng.getTxdBitNotMask());
    TEST_ASSERT_EQUAL((uint8_t) CTS_PIN, eng.getCtsPin());
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

    eng.processBit();

    TEST_ASSERT_EQUAL(RECEIVE_STATE_OUT_OF_SYNC, eng.receiveState);
    TEST_ASSERT_FALSE(eng.getInCharSync());

    eng.setBitBuffer(0x32);     // Sync bit pattern
    eng.processBit();

    TEST_ASSERT_EQUAL(RECEIVE_STATE_IDLE, eng.receiveState);
    TEST_ASSERT_TRUE(eng.getInCharSync());
}

void test_ReceiveEngine_processBit_STX_ETX(void) {
    ReceiveEngine eng(TXD_PIN, CTS_PIN);
    DataBuffer * db;

    eng.startReceiving();

    eng.setBitBuffer(0x11);
    eng.processBit();

    eng.setBitBuffer(0x32);     // Sync bit pattern
    eng.processBit();
    TEST_ASSERT_EQUAL(RECEIVE_STATE_IDLE, eng.receiveState);
    TEST_ASSERT_TRUE(eng.getInCharSync());
    db = eng.getDataBuffer();

    TEST_ASSERT_EQUAL(0x32, db->get(0));
    TEST_ASSERT_EQUAL(1, db->getLength());

    eng.setBitBuffer(0x02);     // STX
    eng.processBit();

    TEST_ASSERT_EQUAL(RECEIVE_STATE_DATA, eng.receiveState);
    TEST_ASSERT_EQUAL(2, db->getLength());
    TEST_ASSERT_EQUAL(0x02, db->get(1));

    eng.setBitBuffer(0xC1);     // 'A'
    eng.processBit();
    TEST_ASSERT_EQUAL(RECEIVE_STATE_DATA, eng.receiveState);
    TEST_ASSERT_EQUAL(3, db->getLength());
    TEST_ASSERT_EQUAL(0xC1, db->get(2));

    eng.setBitBuffer(0xC2);     // 'B'
    eng.processBit();
    TEST_ASSERT_EQUAL(RECEIVE_STATE_DATA, eng.receiveState);
    TEST_ASSERT_EQUAL(4, db->getLength());
    TEST_ASSERT_EQUAL(0xC2, db->get(3));

    eng.setBitBuffer(0x03);     // ETX
    eng.processBit();
    TEST_ASSERT_EQUAL(RECEIVE_STATE_BCC1, eng.receiveState);
    TEST_ASSERT_EQUAL(5, db->getLength());
    TEST_ASSERT_EQUAL(0x03, db->get(4));

    eng.setBitBuffer(0x44);     // first BCC byte
    eng.processBit();
    TEST_ASSERT_EQUAL(RECEIVE_STATE_BCC2, eng.receiveState);
    TEST_ASSERT_EQUAL(6, db->getLength());
    TEST_ASSERT_EQUAL(0x44, db->get(5));

    eng.setBitBuffer(0x55);     // second BCC byte
    eng.processBit();
    TEST_ASSERT_EQUAL(RECEIVE_STATE_PAD, eng.receiveState);
    TEST_ASSERT_EQUAL(7, db->getLength());
    TEST_ASSERT_EQUAL(0x55, db->get(6));

    eng.setBitBuffer(0xFF);     // Pad byte
    eng.processBit();
    TEST_ASSERT_EQUAL(RECEIVE_STATE_IDLE, eng.receiveState);
    TEST_ASSERT_EQUAL(8, db->getLength());
    TEST_ASSERT_EQUAL(0xFF, db->get(7));

    TEST_ASSERT_TRUE(eng.isFrameComplete());
}

void test_ReceiveEngine_processBit_SOH_STX_ETX(void) {
    ReceiveEngine eng(TXD_PIN, CTS_PIN);
    DataBuffer * db;

    eng.startReceiving();

    eng.setBitBuffer(0x11);
    eng.processBit();

    eng.setBitBuffer(0x32);     // Sync bit pattern
    eng.processBit();
    TEST_ASSERT_EQUAL(RECEIVE_STATE_IDLE, eng.receiveState);
    TEST_ASSERT_TRUE(eng.getInCharSync());
    db = eng.getDataBuffer();

    TEST_ASSERT_EQUAL(0x32, db->get(0));
    TEST_ASSERT_EQUAL(1, db->getLength());

    eng.setBitBuffer(0x01);     // SOH
    eng.processBit();

    TEST_ASSERT_EQUAL(RECEIVE_STATE_DATA, eng.receiveState);
    TEST_ASSERT_EQUAL(2, db->getLength());
    TEST_ASSERT_EQUAL(0x01, db->get(1));

    eng.setBitBuffer(0xF1);     // '1'
    eng.processBit();
    TEST_ASSERT_EQUAL(RECEIVE_STATE_DATA, eng.receiveState);
    TEST_ASSERT_EQUAL(3, db->getLength());
    TEST_ASSERT_EQUAL(0xF1, db->get(2));

    eng.setBitBuffer(0x02);     // STX
    eng.processBit();

    TEST_ASSERT_EQUAL(RECEIVE_STATE_DATA, eng.receiveState);
    TEST_ASSERT_EQUAL(4, db->getLength());
    TEST_ASSERT_EQUAL(0x02, db->get(3));

    eng.setBitBuffer(0xC1);     // 'A'
    eng.processBit();
    TEST_ASSERT_EQUAL(RECEIVE_STATE_DATA, eng.receiveState);
    TEST_ASSERT_EQUAL(5, db->getLength());
    TEST_ASSERT_EQUAL(0xC1, db->get(4));

    eng.setBitBuffer(0xC2);     // 'B'
    eng.processBit();
    TEST_ASSERT_EQUAL(RECEIVE_STATE_DATA, eng.receiveState);
    TEST_ASSERT_EQUAL(6, db->getLength());
    TEST_ASSERT_EQUAL(0xC2, db->get(5));

    eng.setBitBuffer(0x26);     // ETB
    eng.processBit();
    TEST_ASSERT_EQUAL(RECEIVE_STATE_BCC1, eng.receiveState);
    TEST_ASSERT_EQUAL(7, db->getLength());
    TEST_ASSERT_EQUAL(0x26, db->get(6));

    eng.setBitBuffer(0x44);     // first BCC byte
    eng.processBit();
    TEST_ASSERT_EQUAL(RECEIVE_STATE_BCC2, eng.receiveState);
    TEST_ASSERT_EQUAL(8, db->getLength());
    TEST_ASSERT_EQUAL(0x44, db->get(7));

    eng.setBitBuffer(0x55);     // second BCC byte
    eng.processBit();
    TEST_ASSERT_EQUAL(RECEIVE_STATE_PAD, eng.receiveState);
    TEST_ASSERT_EQUAL(9, db->getLength());
    TEST_ASSERT_EQUAL(0x55, db->get(8));

    eng.setBitBuffer(0xFF);     // Pad byte
    eng.processBit();
    TEST_ASSERT_EQUAL(RECEIVE_STATE_IDLE, eng.receiveState);
    TEST_ASSERT_EQUAL(10, db->getLength());
    TEST_ASSERT_EQUAL(0xFF, db->get(9));

    TEST_ASSERT_TRUE(eng.isFrameComplete());
}

void test_ReceiveEngine_processBit_ACK0(void) {
    ReceiveEngine eng(TXD_PIN, CTS_PIN);
    DataBuffer * db;

    db = eng.getDataBuffer();

    eng.startReceiving();

    eng.setBitBuffer(0x11);
    eng.processBit();

    eng.setBitBuffer(0x32);     // Sync bit pattern
    eng.processBit();
    TEST_ASSERT_EQUAL(RECEIVE_STATE_IDLE, eng.receiveState);
    TEST_ASSERT_TRUE(eng.getInCharSync());

    TEST_ASSERT_EQUAL(0x32, db->get(0));
    TEST_ASSERT_EQUAL(1, db->getLength());

    eng.setBitBuffer(0x10);     // DLE
    eng.processBit();

    TEST_ASSERT_EQUAL(RECEIVE_STATE_IDLE, eng.receiveState);
    TEST_ASSERT_EQUAL(1, db->getLength());  // length Unchanged

    eng.setBitBuffer(0x70);     // ACK0
    eng.processBit();
    TEST_ASSERT_EQUAL(RECEIVE_STATE_IDLE, eng.receiveState);
    TEST_ASSERT_EQUAL(3, db->getLength());
    TEST_ASSERT_EQUAL(0x10, db->get(1));
    TEST_ASSERT_EQUAL(0x70, db->get(2));

    TEST_ASSERT_TRUE(eng.isFrameComplete());
}

void test_ReceiveEngine_processBit_ACK1(void) {
    ReceiveEngine eng(TXD_PIN, CTS_PIN);
    DataBuffer * db;

    db = eng.getDataBuffer();

    eng.startReceiving();

    eng.setBitBuffer(0x11);
    eng.processBit();

    eng.setBitBuffer(0x32);     // Sync bit pattern
    eng.processBit();
    TEST_ASSERT_EQUAL(RECEIVE_STATE_IDLE, eng.receiveState);
    TEST_ASSERT_TRUE(eng.getInCharSync());

    TEST_ASSERT_EQUAL(0x32, db->get(0));
    TEST_ASSERT_EQUAL(1, db->getLength());

    eng.setBitBuffer(0x10);     // DLE
    eng.processBit();

    TEST_ASSERT_EQUAL(RECEIVE_STATE_IDLE, eng.receiveState);
    TEST_ASSERT_EQUAL(1, db->getLength());  // length Unchanged

    eng.setBitBuffer(0x61);     // ACK1
    eng.processBit();
    TEST_ASSERT_EQUAL(RECEIVE_STATE_IDLE, eng.receiveState);
    TEST_ASSERT_EQUAL(3, db->getLength());
    TEST_ASSERT_EQUAL(0x10, db->get(1));
    TEST_ASSERT_EQUAL(0x61, db->get(2));

    TEST_ASSERT_TRUE(eng.isFrameComplete());
}

void test_ReceiveEngine_processBit_NAK(void) {
    ReceiveEngine eng(TXD_PIN, CTS_PIN);
    DataBuffer * db;

    db = eng.getDataBuffer();

    eng.startReceiving();

    eng.setBitBuffer(0x11);
    eng.processBit();

    eng.setBitBuffer(0x32);     // Sync bit pattern
    eng.processBit();
    TEST_ASSERT_EQUAL(RECEIVE_STATE_IDLE, eng.receiveState);
    TEST_ASSERT_TRUE(eng.getInCharSync());

    TEST_ASSERT_EQUAL(0x32, db->get(0));
    TEST_ASSERT_EQUAL(1, db->getLength());

    eng.setBitBuffer(0x3D);     // NAK
    eng.processBit();

    TEST_ASSERT_EQUAL(RECEIVE_STATE_IDLE, eng.receiveState);
    TEST_ASSERT_EQUAL(2, db->getLength());
    TEST_ASSERT_EQUAL(0x3D, db->get(1));

    TEST_ASSERT_TRUE(eng.isFrameComplete());
}

void test_ReceiveEngine_processBit_SYN_EOT(void) {
    ReceiveEngine eng(TXD_PIN, CTS_PIN);
    DataBuffer * db;

    db = eng.getDataBuffer();

    eng.startReceiving();

    eng.setBitBuffer(0x11);
    eng.processBit();

    eng.setBitBuffer(0x32);     // Sync bit pattern
    eng.processBit();
    TEST_ASSERT_EQUAL(RECEIVE_STATE_IDLE, eng.receiveState);
    TEST_ASSERT_TRUE(eng.getInCharSync());

    TEST_ASSERT_EQUAL(0x32, db->get(0));
    TEST_ASSERT_EQUAL(1, db->getLength());

    eng.setBitBuffer(0x37);     // EOT
    eng.processBit();

    TEST_ASSERT_EQUAL(RECEIVE_STATE_IDLE, eng.receiveState);
    TEST_ASSERT_EQUAL(2, db->getLength());
    TEST_ASSERT_EQUAL(0x37, db->get(1));

    TEST_ASSERT_TRUE(eng.isFrameComplete());
}

void test_ReceiveEngine_processBit_Status_Msg(void) {
    ReceiveEngine eng(TXD_PIN, CTS_PIN);
    DataBuffer * db;

    db = eng.getDataBuffer();

    eng.startReceiving();

    eng.setBitBuffer(0x11);     // Something junk
    eng.processBit();

    eng.setBitBuffer(0x32);     // Sync bit pattern
    eng.processBit();
    TEST_ASSERT_EQUAL(RECEIVE_STATE_IDLE, eng.receiveState);
    TEST_ASSERT_TRUE(eng.getInCharSync());

    TEST_ASSERT_EQUAL(0x32, db->get(0));
    TEST_ASSERT_EQUAL(1, db->getLength());

    eng.setBitBuffer(0x32);     // 2nd sync ... discarded
    eng.processBit();
    TEST_ASSERT_EQUAL(1, db->getLength());

    eng.setBitBuffer(0x01);     // SOH
    eng.processBit();
    TEST_ASSERT_EQUAL(2, db->getLength());
    TEST_ASSERT_EQUAL(0x01, db->get(1));

    eng.setBitBuffer(0x6C);     // %
    eng.processBit();
    TEST_ASSERT_EQUAL(3, db->getLength());
    TEST_ASSERT_EQUAL(0x6C, db->get(2));

    eng.setBitBuffer(0xD9);     // R
    eng.processBit();
    TEST_ASSERT_EQUAL(4, db->getLength());
    TEST_ASSERT_EQUAL(0xD9, db->get(3));

    eng.setBitBuffer(0x02);     // STX
    eng.processBit();
    TEST_ASSERT_EQUAL(5, db->getLength());
    TEST_ASSERT_EQUAL(0x02, db->get(4));

    eng.setBitBuffer(0xC1);     // CU-ADDR-CHAR 'A', dev # 1
    eng.processBit();
    eng.setBitBuffer(0xC1);     // CU-ADDR-CHAR 'A', dev # 1
    eng.processBit();
    eng.setBitBuffer(0x40);     // DEV-ADDR-CHAR ' ', dev # 0
    eng.processBit();
    eng.setBitBuffer(0x40);     // DEV-ADDR-CHAR ' ', dev # 0
    eng.processBit();
    eng.setBitBuffer(0xC9);     // invented SS byte
    eng.processBit();
    eng.setBitBuffer(0xC9);     // invented SS byte
    eng.processBit();
    eng.setBitBuffer(0xC9);     // invented SS byte
    eng.processBit();
    TEST_ASSERT_EQUAL(12, db->getLength());

    eng.setBitBuffer(0x03);     // ETX
    eng.processBit();
    TEST_ASSERT_EQUAL(13, db->getLength());
    TEST_ASSERT_EQUAL(0x03, db->get(12));

    eng.setBitBuffer(0xF1);     // BCC1
    eng.processBit();
    TEST_ASSERT_EQUAL(14, db->getLength());
    TEST_ASSERT_EQUAL(0xF1, db->get(13));

    eng.setBitBuffer(0xF2);     // BCC2
    eng.processBit();
    TEST_ASSERT_EQUAL(15, db->getLength());
    TEST_ASSERT_EQUAL(0xF2, db->get(14));

    eng.setBitBuffer(0xFF);     // PAD
    eng.processBit();
    TEST_ASSERT_EQUAL(16, db->getLength());
    TEST_ASSERT_EQUAL(0xFF, db->get(15));

    TEST_ASSERT_TRUE(eng.isFrameComplete());
}

void test_ReceiveEngine_processBit_Read_Partition1(void) {
    ReceiveEngine eng(TXD_PIN, CTS_PIN);
    DataBuffer * db;

    db = eng.getDataBuffer();

    eng.startReceiving();

    eng.setBitBuffer(0x11);     // Something junk
    eng.processBit();

    eng.setBitBuffer(0x32);     // Sync bit pattern
    eng.processBit();
    TEST_ASSERT_EQUAL(RECEIVE_STATE_IDLE, eng.receiveState);
    TEST_ASSERT_TRUE(eng.getInCharSync());

    TEST_ASSERT_EQUAL(0x32, db->get(0));
    TEST_ASSERT_EQUAL(1, db->getLength());

    eng.setBitBuffer(0x32);     // 2nd sync ... discarded
    eng.processBit();
    TEST_ASSERT_EQUAL(1, db->getLength());

    eng.setBitBuffer(0x10);     // DLE
    eng.processBit();

    eng.setBitBuffer(0x02);     // STX
    eng.processBit();
    TEST_ASSERT_EQUAL(3, db->getLength());
    TEST_ASSERT_EQUAL(0x10, db->get(1));
    TEST_ASSERT_EQUAL(0x02, db->get(2));

    TEST_ASSERT_EQUAL(RECEIVE_STATE_TRANSPARENT_DATA, eng.receiveState);

    eng.setBitBuffer(0xC1);     // CU-ADDR-CHAR 'A', dev # 1
    eng.processBit();
    eng.setBitBuffer(0xC1);     // CU-ADDR-CHAR 'A', dev # 1
    eng.processBit();
    eng.setBitBuffer(0x40);     // DEV-ADDR-CHAR ' ', dev # 0
    eng.processBit();
    eng.setBitBuffer(0x40);     // DEV-ADDR-CHAR ' ', dev # 0
    eng.processBit();
    eng.setBitBuffer(0xC1);     // Message text ...
    eng.processBit();
    eng.setBitBuffer(0xC2);     // invented SS byte
    eng.processBit();
    eng.setBitBuffer(0xC3);     // invented SS byte
    eng.processBit();
    TEST_ASSERT_EQUAL(10, db->getLength());

    eng.setBitBuffer(0x10);     // DLE
    eng.processBit();

    eng.setBitBuffer(0x03);     // ETX
    eng.processBit();
    TEST_ASSERT_EQUAL(12, db->getLength());
    TEST_ASSERT_EQUAL(0x10, db->get(10));
    TEST_ASSERT_EQUAL(0x03, db->get(11));

    eng.setBitBuffer(0xF1);     // BCC1
    eng.processBit();
    TEST_ASSERT_EQUAL(13, db->getLength());
    TEST_ASSERT_EQUAL(0xF1, db->get(12));

    eng.setBitBuffer(0xF2);     // BCC2
    eng.processBit();
    TEST_ASSERT_EQUAL(14, db->getLength());
    TEST_ASSERT_EQUAL(0xF2, db->get(13));

    eng.setBitBuffer(0xFF);     // PAD
    eng.processBit();
    TEST_ASSERT_EQUAL(15, db->getLength());
    TEST_ASSERT_EQUAL(0xFF, db->get(14));

    TEST_ASSERT_EQUAL(RECEIVE_STATE_IDLE, eng.receiveState);
    TEST_ASSERT_TRUE(eng.isFrameComplete());
}



void test_ReceiveEngine() {
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
}