#include <Arduino.h>
#include <unity.h>
#include "DataBuffer.h"

extern void test_CommandProcessor();

void setUp(void) {

}

void tearDown(void) {

}



void setup() {
    delay(4000);

    UNITY_BEGIN();
}

void loop() {
    test_CommandProcessor();
    UNITY_END();
    while(1);
}