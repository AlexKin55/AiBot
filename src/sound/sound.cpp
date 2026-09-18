#include "sound/sound.h"

#include <M5Unified.h>

bool EspSound::begin()
{
    return begin(Config{});
}

bool EspSound::begin(const Config& config)
{
    config_ = config;

    if (!M5.Display.width())
    {
        auto cfg = M5.config();
        M5.begin(cfg);
    }
    M5.Power.setExtPower(true);

    auto spkCfg = M5.Speaker.config();
    spkCfg.sample_rate = config_.sampleRate;
    M5.Speaker.config(spkCfg);
    M5.Speaker.setVolume(200);

    enabled_ = M5.Speaker.begin();
    return enabled_;
}

bool EspSound::isEnabled() const
{
    return enabled_;
}

void EspSound::end()
{
    if (enabled_)
    {
        M5.Speaker.end();
        enabled_ = false;
    }
}

void EspSound::stop()
{
    if (enabled_)
    {
        M5.Speaker.stop();
    }
}

bool EspSound::ensureReady()
{
    // Переключение общей I2S-шины CoreS3 на динамик: освобождаем порт от
    // микрофона (если он ещё слушает) и заводим динамик. M5.Speaker.begin()
    // идемпотентен и сам переключает кодек (AW88298).
    if (M5.Mic.isRunning())
    {
        M5.Mic.end();
        vTaskDelay(pdMS_TO_TICKS(20));  // дать драйверу освободить порт
    }
    enabled_ = M5.Speaker.begin();
    return enabled_;
}

bool EspSound::playTone(uint16_t frequencyHz, uint16_t durationMs)
{
    if (!enabled_)
    {
        return false;
    }
    M5.Speaker.tone(frequencyHz, durationMs);
    return true;
}

bool EspSound::playSample(const int16_t* data, size_t samples)
{
    if (!enabled_)
    {
        return false;
    }
    // ВАЖНО: четвёртый параметр playRaw — bool stereo. Число каналов (1) в
    // него передавать нельзя: 1 == true, и моно-PCM играется как стерео
    // (скорость x2, «мышиный» голос). У нас всегда моно.
    //
    // Канал фиксируем (0) и не прерываем текущий звук: все PCM-чанки одной
    // озвучки встают в очередь одного потока воспроизведения без пауз и
    // щелчков на стыках (при channel=-1 каждый playRaw заводил бы новый
    // поток, и чанки накладывались/щёлкали).
    M5.Speaker.playRaw(data, samples, config_.sampleRate, false, 1, 0, false);
    return true;
}