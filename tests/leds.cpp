// Тест светодиодов робота: последовательно зажигаем лампочки разными цветами.
//
// После begin() проверяется наличие светодиодов, затем по очереди включаются
// красный, зелёный, синий, жёлтый, голубой, пурпурный, белый и в конце всё
// гасится. Каждый цвет удерживается LED_TEST_COLOR_MS — визуально можно
// наблюдать смену подсветки на голове робота.

#include <Arduino.h>

#include "app_config.h"
#include "leds/leds.h"
#include "test_util.h"

namespace {

EspLeds gLeds;

void showColor(const char* name, uint8_t r, uint8_t g, uint8_t b)
{
    gLeds.setColor(r, g, b);
    Serial.printf("[leds] color: %s (r=%u g=%u b=%u)\n", name, r, g, b);
    report(true, name);
    delay(LED_TEST_COLOR_MS);
}

}  // namespace

void runLedsTests()
{
    Serial.println("[TEST] === LED tests on real hardware ===");

    report(gLeds.begin(), "leds begin()");
    report(gLeds.count() > 0, "leds count > 0");

    gLeds.setBrightness(40);

    showColor("led Red",    255, 0,   0);
    showColor("led Green",  0,   255, 0);
    showColor("led Blue",   0,   0,   255);
    showColor("led Yellow", 255, 255, 0);
    showColor("led Cyan",   0,   255, 255);
    showColor("led Magenta", 255, 0, 255);
    showColor("led White",  255, 255, 255);

    // Гасим светодиоды.
    gLeds.off();
    Serial.println("[leds] off");
    delay(200);
}