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
    M5.Speaker.playRaw(data, samples, config_.sampleRate, config_.channels);
    return true;
}