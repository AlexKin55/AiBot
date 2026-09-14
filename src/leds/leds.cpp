#include "leds/leds.h"

#include <M5Unified.h>

bool EspLeds::begin()
{
    if (!M5.Display.width())
    {
        auto cfg = M5.config();
        M5.begin(cfg);
    }
    return M5.Led.begin();
}

size_t EspLeds::count() const
{
    return M5.Led.getCount();
}

void EspLeds::setBrightness(uint8_t brightness)
{
    M5.Led.setBrightness(brightness);
}

void EspLeds::setColor(uint8_t red, uint8_t green, uint8_t blue)
{
    M5.Led.setAllColor(red, green, blue);
}

void EspLeds::setColor(size_t index, uint8_t red, uint8_t green, uint8_t blue)
{
    M5.Led.setColor(index, red, green, blue);
}

void EspLeds::off()
{
    M5.Led.setAllColor(0, 0, 0);
}