#include "microfon/microfon.h"

#include <algorithm>
#include <cmath>

#include "log/log.h"

EspMicrophone::~EspMicrophone()
{
    stop();
}

bool EspMicrophone::begin(const Config& config)
{
    config_ = config;
    sampleRate_ = config.sampleRate;
    channels_ = config.channels;
    buffer_.resize(config.frameSamples);
    prev_.resize(config.frameSamples);

    // Настройки из рабочего примера (см. example/mic_m5.cpp):
    //   - усиление задаётся config.magnification: без него сигнал с микрофона
    //     близок к нулю, а слишком большое (128) клиппит речь вблизи;
    //   - на CoreS3 микрофон и динамик делят I2S-шину и кодеки (ES7210 /
    //     AW88298). Mic.begin() на занятой динамиком шине падает
    //     («register I2S object to platform failed») и дальше крашится,
    //     поэтому сначала освобождаем динамик.
    auto micCfg = M5.Mic.config();
    micCfg.sample_rate = config.sampleRate;
    micCfg.stereo = false;
    micCfg.magnification = config.magnification;
    micCfg.over_sampling = config.overSampling;
    M5.Mic.config(micCfg);
    if (M5.Speaker.isRunning())
    {
        M5.Speaker.end();
        vTaskDelay(pdMS_TO_TICKS(20));  // дать драйверу освободить порт
    }
    const bool ok = M5.Mic.begin();
    LOG_I("[mic] begin ok=%d rate=%u ch=%u frame=%u mag=%u os=%u\n",
          ok, static_cast<unsigned>(config.sampleRate),
          config.channels, static_cast<unsigned>(config.frameSamples),
          static_cast<unsigned>(config.magnification),
          static_cast<unsigned>(config.overSampling));
    return ok;
}

bool EspMicrophone::setNoiseLevelCallback(NoiseLevelCallback cb)
{
    noiseCb_ = std::move(cb);
    return true;
}

bool EspMicrophone::start(AudioCallback cb)
{
    if (running_)
        return true;

    // Перед захватом микрофона освобождаем шину от динамика и заводим
    // микрофон (M5.Mic.begin() сам переключает кодеки CoreS3 на ES7210).
    if (M5.Speaker.isRunning())
    {
        M5.Speaker.end();
        vTaskDelay(pdMS_TO_TICKS(20));  // дать драйверу освободить порт
    }
    M5.Mic.begin();

    cb_ = std::move(cb);
    havePrev_ = false;
    running_ = true;

    // M5.Mic.record() требует большой стек задачи (в рабочем проекте 32 КБ).
    if (xTaskCreatePinnedToCore(recordTask, "mic", 32768, this, 1, &task_, 0) != pdPASS)
    {
        running_ = false;
        return false;
    }
    return true;
}

void EspMicrophone::stop()
{
    running_ = false;
    // Кооперативная остановка: задача может находиться внутри блокирующего
    // M5.Mic.record() (ожидание DMA-буфера, ~1 кадр = 32 мс). Убивать её через
    // vTaskDelete нельзя — она оставляет внутренние очереди M5Unified/ESP-IDF
    // в неконсистентном состоянии. Ждём, пока задача выйдет сама и обнулит task_.
    unsigned tries = 0;
    while (task_ != nullptr && tries < 200)
    {
        vTaskDelay(1);
        ++tries;
    }
    if (task_ != nullptr)
    {
        // Резервный путь на случай зависания (обычно не срабатывает).
        vTaskDelete(task_);
        task_ = nullptr;
    }
    // Освобождаем I2S-шину для динамика (переключение кодека на AW88298
    // произойдёт в EspSound::ensureReady() при следующей озвучке).
    M5.Mic.end();
}

bool EspMicrophone::isRunning() const
{
    return running_;
}

uint32_t EspMicrophone::sampleRate() const
{
    return sampleRate_;
}

uint16_t EspMicrophone::channels() const
{
    return channels_;
}

void EspMicrophone::recordTask(void* arg)
{
    auto* self = static_cast<EspMicrophone*>(arg);
    self->recordLoop();
    // recordLoop() завершается самоудалением задачи (не возвращается).
}

void EspMicrophone::recordLoop()
{
    bool active = false;
    uint16_t hangover = 0;
    uint32_t dbgTick = 0;

    while (running_)
    {
        // Mic::record() возвращает bool: при успехе буфер заполняется целиком.
        const bool ok =
            M5.Mic.record(buffer_.data(), buffer_.size(), config_.sampleRate);
        if (!ok)
        {
            vTaskDelay(1);
            continue;
        }
        const size_t read = buffer_.size();

        const float rms = computeRms(buffer_.data(), read);

        // Периодическая диагностика уровня сигнала.
        if (++dbgTick % 50 == 0)
        {
            int16_t peak = 0;
            for (size_t i = 0; i < read; ++i)
            {
                const int16_t a = buffer_[i] < 0 ? static_cast<int16_t>(-buffer_[i])
                                                 : buffer_[i];
                if (a > peak)
                    peak = a;
            }
            LOG_D("[mic] rms=%d peak=%d gate_thr=%d\n", static_cast<int>(rms),
                  peak, static_cast<int>(config_.noiseGateThreshold));
        }

        if (noiseCb_)
        {
            noiseCb_(rms);
        }

        const bool above =
            (!config_.noiseGateEnabled) || (rms >= config_.noiseGateThreshold);

        if (above)
        {
            if (!active)
            {
                // Начало речи: досылаем предыдущий кадр.
                if (havePrev_ && cb_)
                {
                    cb_(prev_.data(), read);
                }
                active = true;
            }
            hangover = config_.hangoverFrames;
        }
        else if (active && hangover > 0)
        {
            --hangover;
        }
        else if (active)
        {
            active = false;
        }

        if (active && cb_)
        {
            cb_(buffer_.data(), read);
        }

        if (read <= prev_.size())
        {
            std::copy(buffer_.begin(), buffer_.begin() + read, prev_.begin());
        }
        havePrev_ = (read > 0);

        // Уступаем CPU, чтобы не блокировать сторожевой таймер.
        vTaskDelay(1);
    }

    // Задача завершается кооперативно: stop() ждёт task_ == nullptr, а затем
    // вызывает M5.Mic.end(). Самоудаление обязано идти после выхода из
    // record(), чтобы не ломать очереди M5Unified.
    task_ = nullptr;
    vTaskDelete(nullptr);
}

float EspMicrophone::computeRms(const int16_t* data, size_t samples)
{
    uint64_t sum = 0;
    for (size_t i = 0; i < samples; ++i)
    {
        const int32_t s = data[i];
        sum += static_cast<uint64_t>(s * s);
    }
    return std::sqrt(static_cast<float>(sum) / samples);
}