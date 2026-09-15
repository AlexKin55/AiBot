// WebSocket-события и обработка команд протокола:
// движение, эмоции, светодиоды, аудиотрансляция с микрофона.

#include "commands.h"

#include <Arduino.h>

#include <freertos/queue.h>

#include <cstring>
#include <string>

#include "app.h"
#include "config/config.h"
#include "leds/leds.h"
#include "microfon/microfon.h"
#include "move/move.h"
#include "opus/opus.h"
#include "protocol/protocol.h"
#include "screen/screen.h"
#include "state_machine.h"

namespace {

// Opus-энкодер записи. Декодер живёт внутри задачи playbackTask (см. ниже).
OpusEncoderWrapper gOpusEnc;
std::vector<uint8_t> gOpusPkt;   // буфер одного Opus-пакета

Emotion parseEmotion(const std::string& name)
{
    if (name == "happy") return Emotion::Happy;
    if (name == "angry") return Emotion::Angry;
    if (name == "sad") return Emotion::Sad;
    if (name == "doubt") return Emotion::Doubt;
    if (name == "sleepy") return Emotion::Sleepy;
    return Emotion::Neutral;
}

void sendResponse(const protocol::Command& cmd, bool ok, const char* reason = nullptr)
{
    gWs.sendText(protocol::response(cmd, ok, reason));
}

// Отправляет накопленный чанк как бинарный фрейм:
// [0]=kAudioFrameType, [1]=kAudioCodecOpus, [2..]= последовательность
// Opus-пакетов в формате [u16le длина][данные пакета].
void sendAudioChunk()
{
    if (gAudioChunk.empty())
    {
        return;
    }
    const size_t payload = 2 + gAudioChunk.size();
    if (gAudioFrame.size() < payload)
    {
        gAudioFrame.resize(payload);
    }
    uint8_t* out = gAudioFrame.data();
    out[0] = kAudioFrameType;
    out[1] = kAudioCodecOpus;
    std::memcpy(out + 2, gAudioChunk.data(), gAudioChunk.size());
    gWs.sendBinary(out, payload);
}

// Callback захвата микрофона: кодирует каждый PCM-кадр (20 мс) в Opus-пакет
// и копит пакеты в чанк-буфер. Чанк отправляется по достижении
// MIC_AUDIO_CHUNK_SECONDS: непрерывный стриминг по Wi-Fi во время записи даёт
// помехи I2S на CoreS3, а Opus дополнительно сокращает трафик в ~10 раз.
// Входящее аудио для воспроизведения. opus_decode требует большого стека
// (в loopTask 32 КБ происходило переполнение), поэтому чанки ставятся
// в очередь и декодируются отдельной задачей с собственным большим стеком.
struct PlaybackMsg
{
    std::vector<uint8_t> data;  // тело фрейма: [u16le len][opus ...]
};

QueueHandle_t gPlayQ = nullptr;

void playbackTask(void* /*arg*/)
{
    OpusDecoderWrapper dec;
    std::vector<int16_t> pcm(MIC_FRAME_SAMPLES);
    // Весь чанк в PCM: playRaw ждёт окончания текущего звука, поэтому вызов
    // на каждый 20-мс пакет давал серию щелчков («перезарядка дробовика»).
    // Чанк 2 с = 64 КБ PCM, декодируется за ~100-200 мс в фоновой задаче.
    std::vector<int16_t> out;
    out.reserve(static_cast<size_t>(MIC_SAMPLE_RATE) *
                    MIC_AUDIO_CHUNK_SECONDS +
                MIC_FRAME_SAMPLES);
    while (true)
    {
        PlaybackMsg* msg = nullptr;
        if (xQueueReceive(gPlayQ, &msg, portMAX_DELAY) != pdTRUE ||
            msg == nullptr)
        {
            continue;
        }
        if (!dec.begin())
        {
            delete msg;
            continue;
        }
        const uint8_t* data = msg->data.data();
        const size_t size = msg->data.size();
        out.clear();
        size_t pos = 0;
        while (pos + sizeof(uint16_t) <= size)
        {
            const uint16_t pktLen = static_cast<uint16_t>(
                data[pos] | (data[pos + 1] << 8));
            pos += sizeof(uint16_t);
            if (pktLen == 0 || pos + pktLen > size)
            {
                break;
            }
            const int n = dec.decode(data + pos, pktLen, pcm.data(),
                                     pcm.size());
            if (n > 0)
            {
                out.insert(out.end(), pcm.begin(), pcm.begin() + n);
            }
            pos += pktLen;
        }
        if (!out.empty())
        {
            // Один непрерывный вызов вместо сотен мелких.
            gSound.playSample(out.data(), out.size());
            Serial.printf("[audio] played opus chunk: %u samples (%.2f s)\n",
                          static_cast<unsigned>(out.size()),
                          static_cast<double>(out.size()) / MIC_SAMPLE_RATE);
        }
        delete msg;
    }
}

// Ставит входящий Opus-чанк в очередь декодирования (иначе очередь полна —
// чанк отбрасывается).
void enqueuePlayback(const uint8_t* data, size_t size)
{
    if (gPlayQ == nullptr)
    {
        gPlayQ = xQueueCreate(4, sizeof(PlaybackMsg*));
        if (gPlayQ != nullptr)
        {
            // Стек 48 КБ: libopus (SILK/Hybrid) + playSample. Приоритет 2 —
            // как советует M5Unified, чтобы не было шумов в выводе динамика.
            xTaskCreatePinnedToCore(playbackTask, "playopus", 49152, nullptr,
                                    2, nullptr, 0);
        }
    }
    if (gPlayQ == nullptr)
    {
        return;
    }
    auto* msg = new PlaybackMsg();
    msg->data.assign(data, data + size);
    if (xQueueSend(gPlayQ, &msg, 0) != pdTRUE)
    {
        delete msg;  // очередь занята — пропускаем (не накапливаем)
    }
}

void onAudioSamples(const int16_t* data, size_t samples)
{
    if (!gAudioCapturing)
    {
        return;
    }
    const int encSize =
        gOpusEnc.encode(data, samples, gOpusPkt.data(), gOpusPkt.size());
    if (encSize <= 0)
    {
        return;
    }
    const uint16_t pktLen = static_cast<uint16_t>(encSize);
    const uint8_t* lenBytes = reinterpret_cast<const uint8_t*>(&pktLen);
    if (gAudioChunk.size() + sizeof(uint16_t) + encSize > gAudioChunkMaxBytes ||
        gAudioChunkPackets >= gAudioChunkMaxPackets)
    {
        sendAudioChunk();
        gAudioChunk.clear();
        gAudioChunkPackets = 0;
    }
    gAudioChunk.insert(gAudioChunk.end(), lenBytes, lenBytes + sizeof(uint16_t));
    gAudioChunk.insert(gAudioChunk.end(), gOpusPkt.begin(),
                       gOpusPkt.begin() + encSize);
    ++gAudioChunkPackets;
}

void onWsConnected()
{
    Serial.println("[ws] connected to server");
    if (gState == AppState::kConnecting)
    {
        transition(AppState::kReady);
    }
    // Короткий сигнал успешного подключения.
    gSound.playTone(880, 120);
}

void onWsDisconnected(uint16_t code, const std::string& reason)
{
    Serial.printf("[ws] disconnected code=%u reason=\"%s\"\n", code, reason.c_str());
    if (gState == AppState::kReady || gState == AppState::kConnecting)
    {
        transition(AppState::kWifiLost);
    }
}

// Выполняет команду протокола и отправляет подтверждение/ошибку.
void handleCommand(const protocol::Command& cmd)
{
    switch (cmd.type)
    {
        case protocol::CommandType::Ping:
        {
            gWs.sendText(protocol::pong(static_cast<uint32_t>(millis())));
            break;
        }
        case protocol::CommandType::Status:
        {
            gWs.sendText(protocol::statusOnline());
            break;
        }
        case protocol::CommandType::Emotion:
        {
            if (cmd.valid)
            {
                gScreen.setEmotion(parseEmotion(cmd.emotion));
                sendResponse(cmd, true);
            }
            else
            {
                sendResponse(cmd, false, "unknown emotion");
            }
            break;
        }
        case protocol::CommandType::Move:
        {
            if (!cmd.valid)
            {
                sendResponse(cmd, false, "invalid move");
                break;
            }
            if (cmd.moveAxis == "center") gMove.center();
            else if (cmd.moveAxis == "left") gMove.turnLeft(cmd.moveDegrees);
            else if (cmd.moveAxis == "right") gMove.turnRight(cmd.moveDegrees);
            else if (cmd.moveAxis == "up") gMove.turnUp(cmd.moveDegrees);
            else if (cmd.moveAxis == "down") gMove.turnDown(cmd.moveDegrees);
            sendResponse(cmd, true);
            break;
        }
        case protocol::CommandType::Led:
        {
        /*
            if (cmd.valid)
            {
                gLeds.setColor(cmd.ledR, cmd.ledG, cmd.ledB);
                sendResponse(cmd, true);
            }
            else
            {
                sendResponse(cmd, false, "invalid color");
            }
            break;
        */
            Serial.println("[led] not implemented");
        }
        case protocol::CommandType::Audio:
        {
            if (!cmd.valid)
            {
                sendResponse(cmd, false, "invalid audio");
                break;
            }
            if (cmd.audioStart)
            {
                if (!gMic.isRunning())
                {
                    // Захват звука: пишем непрерывно без шумового шлюза (как в
                    // рабочем примере mic_m5.cpp), иначе тихие согласные и края
                    // фраз вырезаются и запись становится неразборчивой. VAD
                    // (шумовой шлюз) при необходимости включается отдельно.
                    EspMicrophone::Config cfg;
                    cfg.sampleRate = MIC_SAMPLE_RATE;
                    cfg.channels = MIC_CHANNELS;
                    cfg.frameSamples = MIC_FRAME_SAMPLES;
                    cfg.noiseGateEnabled = false;
                    cfg.noiseGateThreshold = MIC_NOISE_THRESHOLD;
                    cfg.hangoverFrames = MIC_HANGOVER_FRAMES;

                    // Чанк-буфер: лимит пакетов = CHUNK_SECONDS записи,
                    // байтовый лимит — страховка от раздувания пакетов.
                    gAudioChunkMaxPackets =
                        MIC_SAMPLE_RATE * MIC_AUDIO_CHUNK_SECONDS / MIC_FRAME_SAMPLES;
                    gAudioChunkMaxBytes = 64u * 1024u;
                    gAudioChunk.clear();
                    gAudioChunkPackets = 0;
                    gAudioCapturing = true;

                    gOpusPkt.resize(1024);
                    if (!gOpusEnc.begin())
                    {
                        gAudioCapturing = false;
                        sendResponse(cmd, false, "opus encoder failed");
                        break;
                    }

                    if (gMic.begin(cfg) && gMic.start(onAudioSamples))
                    {
                        Serial.printf("[audio] capture started (opus, chunk %.1f s)\n",
                                      static_cast<double>(MIC_AUDIO_CHUNK_SECONDS));
                        sendResponse(cmd, true);
                    }
                    else
                    {
                        gAudioCapturing = false;
                        sendResponse(cmd, false, "mic start failed");
                    }
                }
                else
                {
                    sendResponse(cmd, true);  // уже захватываем
                }
            }
            else
            {
                gMic.stop();  // кооперативная остановка задачи захвата
                gAudioCapturing = false;
                Serial.println("[audio] capture stopped");
                sendResponse(cmd, true);
                // Последний неполный чанк отправляем серверу после ACK.
                sendAudioChunk();
                gAudioChunk.clear();
                gAudioChunkPackets = 0;
            }
            break;
        }
        case protocol::CommandType::Unknown:
        default:
        {
            sendResponse(cmd, false, "unknown command");
            break;
        }
    }
}

void onWsMessage(const uint8_t* data, size_t size, bool binary)
{
    if (binary)
    {
        // Входящий аудиопоток для воспроизведения:
        // [0]=kAudioFrameType, [1]=кодек, [2..]=данные.
        // Кодек 1: сырой PCM int16; кодек 2: Opus-пакеты [u16le len][opus].
        if (size >= 2 && data[0] == kAudioFrameType)
        {
            // Динамик и микрофон делят I2S на CoreS3: перед воспроизведением
            // останавливаем трансляцию с микрофона, если она идёт.
            if (gMic.isRunning())
            {
                gMic.stop();
                Serial.println("[audio] mic stopped (playback)");
            }

            if (data[1] == kAudioCodecPcm)
            {
                const size_t samples = (size - 2) / sizeof(int16_t);
                gSound.playSample(reinterpret_cast<const int16_t*>(data + 2), samples);
            }
            else if (data[1] == kAudioCodecOpus)
            {
                // Декодирование в отдельной задаче с большим стеком
                // (opus_decode в loopTask переполняет его стек).
                enqueuePlayback(data + 2, size - 2);
            }
            else
            {
                Serial.printf("[audio] unknown codec %u\n", data[1]);
            }
            return;
        }
        Serial.printf("[ws] binary frame %u bytes\n", static_cast<unsigned>(size));
        return;
    }

    // Робот слушает WebSocket и выполняет команды протокола.
    const protocol::Command cmd =
        protocol::parse(reinterpret_cast<const char*>(data), size);
    Serial.printf("[ws] text << %.*s\n", static_cast<int>(size),
                  reinterpret_cast<const char*>(data));
    handleCommand(cmd);
}

}  // namespace

void setupCommands()
{
    gWs.setConnectedCallback(onWsConnected);
    gWs.setDisconnectedCallback(onWsDisconnected);
    gWs.setMessageCallback(onWsMessage);
}