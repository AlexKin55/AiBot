#ifndef LEDS_H_
#define LEDS_H_

#include <cstddef>
#include <cstdint>

// Управление светодиодами робота (RGB-лампочки на голове / встроенный
// светодиод) через M5Unified LED_Class (M5.Led).
class EspLeds
{
    public:
    // Инициализирует светодиоды.
    bool begin();
    // Число доступных светодиодов.
    size_t count() const;

    // Устанавливает яркость (0-255).
    void setBrightness(uint8_t brightness);

    // Зажигает все светодиоды указанным цветом.
    void setColor(uint8_t red, uint8_t green, uint8_t blue);
    // Зажигает один светодиод по индексу указанным цветом.
    void setColor(size_t index, uint8_t red, uint8_t green, uint8_t blue);

    // Гасит все светодиоды.
    void off();
};

#endif  // LEDS_H_