// WebSocket-события и обработка команд протокола:
// движение, эмоции, светодиоды, аудиотрансляция с микрофона.

#include "commands.h"

#include <Arduino.h>

#include <cstring>
#include <string>

#include "app.h"
#include "app_config.h"
#include "leds/leds.h"
#include "microfon/microfon.h"
#include "move/move.h"
#include "protocol/protocol.h"
#include "screen/screen.h"
#include "state_machine.h"

namespace {

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

// Callback захвата микрофона: упаковывает PCM в бинарный WS-фрейм
// [0]=kAudioFrameType, [1]=kAudioCodecPcm, [2..]=int16 LE и отправляет клиенту.
void onAudioSamples(const int16_t* data, size_t samples)
{
    const size_t payload = 2 + samples * sizeof(int16_t);
    if (gAudioFrame.size() < payload)
    {
        gAudioFrame.resize(payload);
    }
    uint8_t* out = gAudioFrame.data();
    out[0] = kAudioFrameType;
    out[1] = kAudioCodecPcm;
    std::memcpy(out + 2, data, samples * sizeof(int16_t));
    gWs.sendBinary(out, payload);
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
            gWs.sendText(protocol::pong(static_cast<uint32_t>(millis())));
            break;

        case protocol::CommandType::Status:
            gWs.sendText(protocol::statusOnline());
            break;

        case protocol::CommandType::Emotion:
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

        case protocol::CommandType::Move:
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

        case protocol::CommandType::Led:
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

        case protocol::CommandType::Audio:
            if (!cmd.valid)
            {
                sendResponse(cmd, false, "invalid audio");
                break;
            }
            if (cmd.audioStart)
            {
                if (!gMic.isRunning())
                {
                    EspMicrophone::Config cfg;
                    cfg.sampleRate = MIC_SAMPLE_RATE;
                    cfg.channels = MIC_CHANNELS;
                    cfg.frameSamples = MIC_FRAME_SAMPLES;
                    cfg.noiseGateEnabled = true;
                    cfg.noiseGateThreshold = MIC_NOISE_THRESHOLD;
                    cfg.hangoverFrames = MIC_HANGOVER_FRAMES;
                    if (gMic.begin(cfg) && gMic.start(onAudioSamples))
                    {
                        Serial.println("[audio] mic streaming started");
                        sendResponse(cmd, true);
                    }
                    else
                    {
                        sendResponse(cmd, false, "mic start failed");
                    }
                }
                else
                {
                    sendResponse(cmd, true);  // уже транслируем
                }
            }
            else
            {
                gMic.stop();
                Serial.println("[audio] mic streaming stopped");
                sendResponse(cmd, true);
            }
            break;

        case protocol::CommandType::Unknown:
        default:
            sendResponse(cmd, false, "unknown command");
            break;
    }
}

void onWsMessage(const uint8_t* data, size_t size, bool binary)
{
    if (binary)
    {
        // Входящий аудиопоток для воспроизведения:
        // [0]=kAudioFrameType, [1]=кодек, [2..]=данные.
        if (size >= 2 && data[0] == kAudioFrameType)
        {
            if (data[1] == kAudioCodecPcm)
            {
                // Динамик и микрофон делят I2S на CoreS3: перед воспроизведением
                // останавливаем трансляцию с микрофона, если она идёт.
                if (gMic.isRunning())
                {
                    gMic.stop();
                    Serial.println("[audio] mic stopped (playback)");
                }
                const size_t samples = (size - 2) / sizeof(int16_t);
                gSound.playSample(reinterpret_cast<const int16_t*>(data + 2), samples);
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