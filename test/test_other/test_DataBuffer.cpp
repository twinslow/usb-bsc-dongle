#include <Arduino.h>
#include <unity.h>
#include "DataBuffer.h"

void test_DataBufferReadOnly_constructor() {
    DataBufferReadOnly buff(20);
    uint8_t dataByte;

    TEST_ASSERT_EQUAL(0, buff.getLength());
    TEST_ASSERT_EQUAL(-1, buff.read(&dataByte));
    TEST_ASSERT_EQUAL(-1, buff.get(0));
    TEST_ASSERT_EQUAL(-1, buff.get(1));
}

void test_DataBufferReadOnly_copy_constructor() {
    DataBufferReadOnly buff(6);

    TEST_ASSERT_EQUAL(0, buff.getLength());

    buff.loadData((uint8_t *)"ABCDEF");
    TEST_ASSERT_EQUAL(6, buff.getLength());

    DataBufferReadOnly copybuff = buff;
    TEST_ASSERT_EQUAL(6, copybuff.getLength());
    TEST_ASSERT_EQUAL('A', copybuff.get(0));
    TEST_ASSERT_EQUAL('B', copybuff.get(1));
    TEST_ASSERT_EQUAL('C', copybuff.get(2));
    TEST_ASSERT_EQUAL('D', copybuff.get(3));
    TEST_ASSERT_EQUAL('E', copybuff.get(4));
    TEST_ASSERT_EQUAL('F', copybuff.get(5));
}

void test_DataBufferReadOnly_loadData_read_get() {
    DataBufferReadOnly buff(3);
    uint8_t dataByte;

    buff.loadData((uint8_t *)"ABC");

    TEST_ASSERT_EQUAL('A', buff.get(0));
    TEST_ASSERT_EQUAL('B', buff.get(1));
    TEST_ASSERT_EQUAL('C', buff.get(2));
    TEST_ASSERT_EQUAL(-1, buff.get(3));

    TEST_ASSERT_EQUAL(3, buff.getLength());
    TEST_ASSERT_EQUAL(0, buff.read(&dataByte));
    TEST_ASSERT_EQUAL('A', dataByte);

    TEST_ASSERT_EQUAL(1, buff.read(&dataByte));
    TEST_ASSERT_EQUAL('B', dataByte);

    TEST_ASSERT_EQUAL(2, buff.read(&dataByte));
    TEST_ASSERT_EQUAL('C', dataByte);

    TEST_ASSERT_EQUAL(-1, buff.read(&dataByte));

}

void test_DataBuffer_copy_constructor(void) {
    DataBuffer buff;

    buff.loadData(6, (uint8_t *)"ABCDEF");
    TEST_ASSERT_EQUAL(6, buff.getLength());
    TEST_ASSERT_EQUAL('A', buff.get(0));
    TEST_ASSERT_EQUAL('B', buff.get(1));
    TEST_ASSERT_EQUAL('C', buff.get(2));
    TEST_ASSERT_EQUAL('D', buff.get(3));
    TEST_ASSERT_EQUAL('E', buff.get(4));
    TEST_ASSERT_EQUAL('F', buff.get(5));

    // Testing initial state of DataBuffer object.

    DataBuffer copybuff = buff;
    TEST_ASSERT_EQUAL(6, copybuff.getLength());
    TEST_ASSERT_EQUAL('A', copybuff.get(0));
    TEST_ASSERT_EQUAL('B', copybuff.get(1));
    TEST_ASSERT_EQUAL('C', copybuff.get(2));
    TEST_ASSERT_EQUAL('D', copybuff.get(3));
    TEST_ASSERT_EQUAL('E', copybuff.get(4));
    TEST_ASSERT_EQUAL('F', copybuff.get(5));
}

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

void test_DataBuffer_makeReadOnlyCopy(void) {
    DataBuffer buff;
    uint8_t inByte;

    inByte = 'A';
    TEST_ASSERT_EQUAL(1, buff.write(inByte));
    inByte = 'B';
    TEST_ASSERT_EQUAL(2, buff.write(inByte));
    inByte = 'C';
    TEST_ASSERT_EQUAL(3, buff.write(inByte));

    DataBufferReadOnly * ro = buff.newReadOnlyCopy();

    TEST_ASSERT_EQUAL(3, ro->getLength() );
    TEST_ASSERT_EQUAL('A', ro->get(0));
    TEST_ASSERT_EQUAL('B', ro->get(1));
    TEST_ASSERT_EQUAL('C', ro->get(2));
    TEST_ASSERT_EQUAL(-1, ro->get(3));
}

void test_DataBuffer_max_length(void) {
    DataBuffer buff;

    int returnedLen;
    for (int x = 1; x <= DATABUFF_MAX_DATA; x++ ) {
        returnedLen = buff.write('Z');
        TEST_ASSERT_EQUAL(x, returnedLen);
    }

    TEST_ASSERT_EQUAL(DATABUFF_MAX_DATA, buff.getLength());
    returnedLen = buff.write('A');
    TEST_ASSERT_EQUAL(-1, returnedLen);
    TEST_ASSERT_EQUAL(DATABUFF_MAX_DATA, buff.getLength());

}

void test_DataBuffer() {
    RUN_TEST(test_DataBufferReadOnly_constructor);
    RUN_TEST(test_DataBufferReadOnly_copy_constructor);
    RUN_TEST(test_DataBufferReadOnly_loadData_read_get);
    RUN_TEST(test_DataBuffer_constructor);
    RUN_TEST(test_DataBuffer_copy_constructor);
    RUN_TEST(test_DataBuffer_write_read_get);
    RUN_TEST(test_DataBuffer_clear);
    RUN_TEST(test_DataBuffer_makeReadOnlyCopy);
    RUN_TEST(test_DataBuffer_max_length);
}