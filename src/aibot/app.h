#ifndef AIBOT_APP_H_
#define AIBOT_APP_H_

#include <cstddef>
#include <cstdint>
#include <vector>

#include "leds/leds.h"
#include "microfon/microfon.h"
#include "move/move.h"
#include "screen/screen.h"
#include "sound/sound.h"
#include "websocket/websocket.h"
#include "wifi/wifi.h"

// Глобальные подсистемы робота (определены в main.cpp).
extern EspWifiManager gWifi;
extern EspWebsocketClient gWs;
extern EspScreen gScreen;
extern EspMovement gMove;
extern EspSound gSound;
extern EspLeds gLeds;
extern EspMicrophone gMic;

// Бинарный фрейм аудио: byte[0]=тип, byte[1]=кодек, далее данные.
// Единственный кодек = 1: сырые PCM-сэмплы (int16 LE, 16 кГц/моно).
extern const uint8_t kAudioFrameType;
extern const uint8_t kAudioCodecPcm;
extern std::vector<uint8_t> gAudioFrame;

// Захват микрофона (VAD-сегмент): PCM-байты копятся в чанк-буфер, который
// отправляется пакетами по MIC_AUDIO_CHUNK_SECONDS. Непрерывный стриминг по
// Wi-Fi во время записи даёт помехи I2S на CoreS3 (скрипы/свисты), поэтому
// звук копится локально и уходит пакетами. По RECORD:stop отправляется
// последний неполный чанк.
extern std::vector<uint8_t> gAudioChunk;    // сырые PCM-байты (int16 LE)
extern size_t gAudioChunkMaxBytes;          // лимит чанка, байт
extern unsigned gAudioChunkPackets;         // кадров в текущем чанке
extern unsigned gAudioChunkMaxPackets;      // лимит кадров в чанке
extern bool gAudioCapturing;                // идёт накопление

#endif  // AIBOT_APP_H_