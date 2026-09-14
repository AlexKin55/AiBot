// Точка входа тестовой прошивки. Все наборы тестов выполняются один раз
// в setup(), результаты читаются через Serial Monitor. loop() остаётся в idle.

#include <Arduino.h>

#include "test_util.h"

void runWifiTests();
void runWebsocketTests();
void runMoveTests();
void runScreenTests();
void runAudioTests();
void runVideoTests();
void runLedsTests();

void setup()
{
    Serial.begin(115200);
    delay(300);

    Serial.println();
    Serial.println("[tests] Test firmware started (M5Stack CoreS3)");

    runWifiTests();
    runWebsocketTests();
    //runMoveTests();
    runScreenTests();
    //runAudioTests();
    //runVideoTests();
    //runLedsTests();

    const int total = testPassed() + testFailed();
    Serial.printf("[TEST] FINAL RESULT: passed=%d failed=%d total=%d\n",
                  testPassed(), testFailed(), total);
    Serial.printf("[TEST] %s\n", testFailed() == 0 ? "ALL PASSED" : "HAS FAILURES");
}

void loop()
{
    delay(1000);
}