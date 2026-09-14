#ifndef MICROFON_H_
#define MICROFON_H_

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <M5Unified.h>

// Захват звука с микрофона робота (M5.Mic, I2S) в отдельной FreeRTOS-задаче.
class EspMicrophone
{
    public:
    struct Config
    {
        uint32_t sampleRate = 16000;   // частота дискретизации, Гц
        uint16_t channels = 1;         // число каналов
        uint16_t frameSamples = 512;   // сэмплов в одном кадре

        // Шумовой шлюз.
        bool noiseGateEnabled = true;
        float noiseGateThreshold = 100.0f;  // порог по RMS, int16-шкала
        uint16_t hangoverFrames = 4;
    };

    using AudioCallback = std::function<void(const int16_t* data, size_t samples)>;
    using NoiseLevelCallback = std::function<void(float rms)>;

    ~EspMicrophone();

    // Инициализирует микрофон (перед этим гасит динамик — общий I2S на CoreS3).
    bool begin(const Config& config);
    bool setNoiseLevelCallback(NoiseLevelCallback cb);
    // Запускает непрерывный захват с шумовым шлюзом.
    bool start(AudioCallback cb);
    void stop();
    bool isRunning() const;

    uint32_t sampleRate() const;
    uint16_t channels() const;

    private:
    static void recordTask(void* arg);
    void recordLoop();
    static float computeRms(const int16_t* data, size_t samples);

    Config config_;
    AudioCallback cb_;
    NoiseLevelCallback noiseCb_;
    std::vector<int16_t> buffer_;
    std::vector<int16_t> prev_;
    bool havePrev_ = false;
    TaskHandle_t task_ = nullptr;
    volatile bool running_ = false;
    uint32_t sampleRate_ = 0;
    uint16_t channels_ = 1;
};

#endif  // MICROFON_H_