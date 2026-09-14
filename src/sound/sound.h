#ifndef SOUND_H_
#define SOUND_H_

#include <cstddef>
#include <cstdint>

// Воспроизведение звука через динамики робота (M5Unified Speaker, I2S).
class EspSound
{
    public:
    struct Config
    {
        uint32_t sampleRate = 16000;  // Гц
        uint16_t channels = 1;
    };

    // Инициализирует динамик с конфигурацией по умолчанию.
    bool begin();
    // Инициализирует динамик с заданной конфигурацией.
    bool begin(const Config& config);
    // Включён ли динамик.
    bool isEnabled() const;
    // Освобождает динамик. На CoreS3 нужно вызывать перед захватом
    // микрофоном, т.к. динамик и микрофон делят I2S-ресурсы.
    void end();
    // Останавливает текущее воспроизведение.
    void stop();

    // Проигрывает тон заданной частоты (Гц) и длительности (мс).
    bool playTone(uint16_t frequencyHz, uint16_t durationMs);
    // Проигрывает PCM-сэмпл (int16 моно) с частотой конфигурации.
    bool playSample(const int16_t* data, size_t samples);

    private:
    Config config_;
    bool enabled_ = false;
};

#endif  // SOUND_H_