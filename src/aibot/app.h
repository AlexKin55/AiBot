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

// Бинарный фрейм аудио: byte[0]=тип, byte[1]=кодек, далее PCM int16.
extern const uint8_t kAudioFrameType;
extern const uint8_t kAudioCodecPcm;
extern std::vector<uint8_t> gAudioFrame;

// Callback захвата микрофона (реализован в commands.cpp).
void onAudioSamples(const int16_t* data, size_t samples);

#endif  // AIBOT_APP_H_