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
// Кодек 1 = сырой PCM int16 LE; кодек 2 = Opus-пакеты [u16le len][opus ...].
extern const uint8_t kAudioFrameType;
extern const uint8_t kAudioCodecPcm;
extern const uint8_t kAudioCodecOpus;
extern std::vector<uint8_t> gAudioFrame;

// Захват микрофона (команда AUDIO:start): PCM кодируется в Opus (20 мс) и
// копится в чанк-буфер, который отправляется пакетами по
// MIC_AUDIO_CHUNK_SECONDS. Непрерывный стриминг по Wi-Fi во время записи
// даёт помехи I2S на CoreS3 (скрипы/свисты), а Opus ещё и в ~10 раз меньше
// трафика. По AUDIO:stop отправляется последний неполный чанк.
extern std::vector<uint8_t> gAudioChunk;    // упакованные Opus-пакеты
extern size_t gAudioChunkMaxBytes;          // лимит чанка, байт
extern unsigned gAudioChunkPackets;         // пакетов в текущем чанке
extern unsigned gAudioChunkMaxPackets;      // лимит пакетов в чанке
extern bool gAudioCapturing;                // идёт накопление

#endif  // AIBOT_APP_H_