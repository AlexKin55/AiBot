// Объединённый тест звука (динамик) и микрофона на реальном железе.
//
// Сценарий: робот пикает N раз, затем пользователь говорит ~3 секунды —
// модуль микрофона записывает звук, и динамик повторяет записанное.
// На CoreS3 динамик и микрофон делят I2S, поэтому между этапами динамик
// освобождается (sound.end()/begin()), а микрофон отключается (mic.stop()).

#include <Arduino.h>

#include <algorithm>
#include <vector>

#include "app_config.h"
#include "microfon/microfon.h"
#include "sound/sound.h"
#include "test_util.h"

namespace {

EspSound gSound;
EspMicrophone gMic;

}  // namespace

void runAudioTests()
{
    Serial.println("[TEST] === Sound + Microphone test on real hardware ===");

    // 1. Динамик: пикаем несколько раз.
    report(gSound.begin(), "sound begin()");
    bool beepOk = true;
    for (unsigned i = 0; i < SOUND_BEEP_COUNT; ++i)
    {
        if (!gSound.playTone(SOUND_BEEP_HZ, SOUND_BEEP_MS))
        {
            beepOk = false;
        }
        delay(SOUND_BEEP_MS + 150u);
    }
    report(beepOk, "sound beeps played");
    gSound.end();  // освобождаем динамик перед микрофоном

    // 2. Микрофон: запись голоса (без шумового шлюза — пишем все кадры).
    EspMicrophone::Config cfg;
    cfg.sampleRate = MIC_SAMPLE_RATE;
    cfg.channels = MIC_CHANNELS;
    cfg.frameSamples = MIC_FRAME_SAMPLES;
    cfg.noiseGateEnabled = false;

    std::vector<int16_t> rec;
    rec.reserve(MIC_SAMPLE_RATE * 4u);  // запас на ~4 секунды

    report(gMic.begin(cfg), "mic begin()");
    report(gMic.start([&rec](const int16_t* data, size_t samples) {
        rec.insert(rec.end(), data, data + samples);
    }), "mic start()");

    // Пользователь говорит в микрофон.
    delay(MIC_TEST_RECORD_MS);

    gMic.stop();
    report(!gMic.isRunning(), "mic stopped");

    const double seconds = static_cast<double>(rec.size()) / MIC_SAMPLE_RATE;
    Serial.printf("[audio] recorded %u samples (%.2f s)\n",
                  static_cast<unsigned>(rec.size()), seconds);
    report(!rec.empty(), "microphone recorded samples");

    // 3. Динамик: повторяем записанное.
    if (!rec.empty())
    {
        report(gSound.begin(), "sound re-enabled for playback");
        bool playOk = true;
        size_t played = 0;
        while (played < rec.size())
        {
            const size_t chunk = std::min<size_t>(1024u, rec.size() - played);
            if (!gSound.playSample(&rec[played], chunk))
            {
                playOk = false;
            }
            played += chunk;
            delay(2);
        }
        report(playOk, "sound replayed recorded audio");
        delay(300);
        gSound.end();
    }
}