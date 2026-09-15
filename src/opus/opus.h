#ifndef AIBOT_OPUS_H_
#define AIBOT_OPUS_H_

// Обёртки над libopus (sh123/esp32_opus) для потокового кодирования голоса
// 16 кГц/моно в пакеты Opus и обратного декодирования. Кадр 20 мс = 320
// сэмплов (стандарт VoIP), формат пакетов совместим с libopus/opuslib.

#include <cstddef>
#include <cstdint>

// Угловые скобки обязательны: собственный файл называется opus.h, иначе
// кавычки зациклили бы include на самом себе.
#include <opus.h>

// Публичный C-API энкодера/декодера. Порт sh123/esp32_opus содержит
// opus_encoder.c/opus_decoder.c, но не поставляет объявлений в отдельном
// заголовке, поэтому объявляем нужные функции вручную (ABI стабилен).
extern "C" {
struct OpusEncoder;
struct OpusDecoder;

OpusEncoder* opus_encoder_create(opus_int32 Fs, int channels, int application,
                                 int* error);
int opus_encode(OpusEncoder* st, const opus_int16* pcm, int frame_size,
                unsigned char* data, opus_int32 max_data_bytes);
int opus_encoder_ctl(OpusEncoder* st, int request, ...);
void opus_encoder_destroy(OpusEncoder* st);

OpusDecoder* opus_decoder_create(opus_int32 Fs, int channels, int* error);
int opus_decode(OpusDecoder* st, const unsigned char* data, opus_int32 len,
                opus_int16* pcm, int frame_size, int decode_fec);
void opus_decoder_destroy(OpusDecoder* st);
}

class OpusEncoderWrapper
{
    public:
    OpusEncoderWrapper() = default;
    ~OpusEncoderWrapper();

    // Создаёт энкодер 16 кГц/моно для голоса (OPUS_APPLICATION_VOIP).
    bool begin();
    // Кодирует один PCM-кадр (20 мс = 320 сэмплов int16 моно) в Opus-пакет.
    // Возвращает размер пакета в байтах или <=0 при ошибке.
    int encode(const int16_t* pcm, size_t samples, uint8_t* out,
               size_t maxOut);
    // Освобождает ресурсы энкодера.
    void end();

    private:
    OpusEncoder* encoder_ = nullptr;
};

class OpusDecoderWrapper
{
    public:
    OpusDecoderWrapper() = default;
    ~OpusDecoderWrapper();

    // Создаёт декодер 16 кГц/моно.
    bool begin();
    // Декодирует один Opus-пакет в PCM (обычно 320 сэмплов). Возвращает
    // число сэмплов или <=0 при ошибке.
    int decode(const uint8_t* data, size_t len, int16_t* pcm,
               size_t maxSamples);
    // Освобождает ресурсы декодера.
    void end();

    private:
    OpusDecoder* decoder_ = nullptr;
};

#endif  // AIBOT_OPUS_H_