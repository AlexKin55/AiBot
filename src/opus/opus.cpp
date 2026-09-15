#include "opus/opus.h"

#include <Arduino.h>

namespace {

constexpr opus_int32 kSampleRate = 16000;
constexpr int kChannels = 1;
constexpr int kFrameSize = 320;  // 20 мс @16 кГц
constexpr int kAppVoip = 2048;   // OPUS_APPLICATION_VOIP
constexpr int kMaxPacketBytes = 1024;  // запас на худший случай

}  // namespace

OpusEncoderWrapper::~OpusEncoderWrapper()
{
    end();
}

bool OpusEncoderWrapper::begin()
{
    if (encoder_ != nullptr)
    {
        return true;
    }
    int err = 0;
    encoder_ = opus_encoder_create(kSampleRate, kChannels, kAppVoip, &err);
    if (encoder_ == nullptr)
    {
        Serial.printf("[opus] encoder create failed err=%d\n", err);
        return false;
    }
    Serial.println("[opus] encoder ready (16 kHz/mono, 20 ms)");
    return true;
}

int OpusEncoderWrapper::encode(const int16_t* pcm, size_t samples, uint8_t* out,
                               size_t maxOut)
{
    if (encoder_ == nullptr || pcm == nullptr || out == nullptr)
    {
        return -1;
    }
    if (samples != static_cast<size_t>(kFrameSize))
    {
        // Опус принимает только кадры фиксированной длины (2.5/5/10/20/40/60 мс).
        return -1;
    }
    const opus_int32 maxBytes =
        (maxOut < static_cast<size_t>(kMaxPacketBytes))
            ? static_cast<opus_int32>(maxOut)
            : kMaxPacketBytes;
    return opus_encode(encoder_, pcm, kFrameSize, out, maxBytes);
}

void OpusEncoderWrapper::end()
{
    if (encoder_ != nullptr)
    {
        opus_encoder_destroy(encoder_);
        encoder_ = nullptr;
    }
}

OpusDecoderWrapper::~OpusDecoderWrapper()
{
    end();
}

bool OpusDecoderWrapper::begin()
{
    if (decoder_ != nullptr)
    {
        return true;
    }
    int err = 0;
    decoder_ = opus_decoder_create(kSampleRate, kChannels, &err);
    if (decoder_ == nullptr)
    {
        Serial.printf("[opus] decoder create failed err=%d\n", err);
        return false;
    }
    Serial.println("[opus] decoder ready (16 kHz/mono)");
    return true;
}

int OpusDecoderWrapper::decode(const uint8_t* data, size_t len, int16_t* pcm,
                               size_t maxSamples)
{
    if (decoder_ == nullptr || data == nullptr || pcm == nullptr)
    {
        return -1;
    }
    const int frameSize =
        (maxSamples < static_cast<size_t>(kFrameSize))
            ? static_cast<int>(maxSamples)
            : kFrameSize;
    return opus_decode(decoder_, data, static_cast<opus_int32>(len), pcm,
                       frameSize, 0);
}

void OpusDecoderWrapper::end()
{
    if (decoder_ != nullptr)
    {
        opus_decoder_destroy(decoder_);
        decoder_ = nullptr;
    }
}