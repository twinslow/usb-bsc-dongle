#include <Arduino.h>
#include <unity.h>
#include "DataBuffer.h"

extern void test_DataBuffer();
extern void test_SendEngine();

void setUp(void) {

}

void tearDown(void) {

}


void setup() {
    delay(3000);

    UNITY_BEGIN();
}

void loop() {
    test_DataBuffer();
    test_SendEngine();
    UNITY_END();
    while(1);
}