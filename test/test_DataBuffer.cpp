#include <Arduino.h>
#include <unity.h>
#include "DataBuffer.h"

void test_DataBuffer_constructor(void) {
    DataBuffer buff;
    uint8_t dataByte;

    // Testing initial state of DataBuffer object.
    TEST_ASSERT_EQUAL(0, buff.getLength());
    TEST_ASSERT_EQUAL(-1, buff.read(&dataByte));
    TEST_ASSERT_EQUAL(-1, buff.get(0));
    TEST_ASSERT_EQUAL(-1, buff.get(1));
    TEST_ASSERT_EQUAL(-1, buff.readLast());
    TEST_ASSERT_EQUAL(0, buff.isComplete());
}

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
    TEST_ASSERT_EQUAL(0, buff.getPos());

    buff.clear();

    TEST_ASSERT_EQUAL(0, buff.getLength());
    TEST_ASSERT_EQUAL(-1, buff.read(&outByte));
    TEST_ASSERT_EQUAL(-1, buff.get(0));
    TEST_ASSERT_EQUAL(-1, buff.readLast());
    TEST_ASSERT_EQUAL(0, buff.getPos());
}


void test_DataBuffer() {
    RUN_TEST(test_DataBuffer_constructor);
    RUN_TEST(test_DataBuffer_write_read_get);
    RUN_TEST(test_DataBuffer_clear);
}