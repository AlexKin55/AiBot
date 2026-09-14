#ifndef TEST_UTIL_H_
#define TEST_UTIL_H_

#include <Arduino.h>

// Общий подсчёт результатов тестов (общий на все файлы tests/*.cpp).
inline int& testPassed() { static int v = 0; return v; }
inline int& testFailed() { static int v = 0; return v; }

// Сообщает результат одного сценария и учитывает его в общем итоге.
inline void report(bool ok, const char* name)
{
    if (ok)
    {
        Serial.printf("[TEST] PASS: %s\n", name);
        ++testPassed();
    }
    else
    {
        Serial.printf("[TEST] FAIL: %s\n", name);
        ++testFailed();
    }
}

#endif  // TEST_UTIL_H_