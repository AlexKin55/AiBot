#ifndef CAMERA_H_
#define CAMERA_H_

#include <cstddef>
#include <cstdint>
#include <vector>

// Захват видеопотока с камеры робота (GC0308 / M5Stack CoreS3).
// Используется драйвер esp32-camera из SDK.
class EspCamera
{
    public:
    struct Config
    {
        uint16_t width = 320;   // запрашиваемая ширина кадра
        uint16_t height = 240;  // запрашиваемая высота кадра
    };

    // Кадр RGB565, захваченный с камеры. data указывает на внутренний буфер
    // модуля и действителен до следующего вызова grabFrame().
    struct Frame
    {
        const void* data = nullptr;
        uint16_t width = 0;
        uint16_t height = 0;
    };

    bool begin(const Config& config);
    bool isActive() const;
    // Копирует очередной готовый кадр (RGB565) во внутренний буфер и отдаёт
    // на него указатель. Возвращает false, если кадр ещё не готов.
    bool grabFrame(Frame& frame);
    void end();

    private:
    std::vector<uint16_t> rgbBuffer_;
    bool active_ = false;
};

#endif  // CAMERA_H_