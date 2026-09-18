// Прошивка робота AIBot: точка входа.
//
// Логика вынесена в файлы рядом:
//   app.h            - глобальные подсистемы (определены здесь)
//   state_machine.*  - state machine, подключение Wi-Fi
//   commands.*       - WebSocket-события и выполнение команд протокола

#include <Arduino.h>
#include <WiFi.h>
#include <esp_system.h>

#include <vector>

#include "app.h"
#include "commands.h"
#include "screen/screen.h"
#include "state_machine.h"

// Определение глобальных подсистем (см. app.h).
EspWifiManager gWifi;
EspWebsocketClient gWs;
EspScreen gScreen;
EspMovement gMove;
EspSound gSound;
EspLeds gLeds;
EspMicrophone gMic;

const uint8_t kAudioFrameType = 1;
const uint8_t kAudioCodecPcm = 1;
std::vector<uint8_t> gAudioFrame;

// Захват микрофона (см. app.h): чанк-буфер сырых PCM-байтов (int16 LE).
std::vector<uint8_t> gAudioChunk;
size_t gAudioChunkMaxBytes = 0;
unsigned gAudioChunkPackets = 0;
unsigned gAudioChunkMaxPackets = 0;
bool gAudioCapturing = false;

void setup()
{
    Serial.begin(115200);
    delay(300);

    Serial.println();
    Serial.println("[app] AIBot firmware started");
    Serial.printf("[app] chip model: %s\n", ESP.getChipModel());

    // Диагностика: причина последней перезагрузки (краш/watchdog/brownout).
    const char* reset_reason = "other";
    switch (esp_reset_reason())
    {
        case ESP_RST_POWERON: reset_reason = "power-on"; break;
        case ESP_RST_SW: reset_reason = "software reset"; break;
        case ESP_RST_PANIC: reset_reason = "panic (crash)"; break;
        case ESP_RST_INT_WDT: reset_reason = "interrupt watchdog"; break;
        case ESP_RST_TASK_WDT: reset_reason = "task watchdog"; break;
        case ESP_RST_WDT: reset_reason = "watchdog"; break;
        case ESP_RST_BROWNOUT: reset_reason = "brownout"; break;
        default: break;
    }
    Serial.printf("[app] last reset reason: %s\n", reset_reason);

    // Не даём Wi-Fi-стеку уходить в глубокий сон.
    WiFi.setSleep(false);

    // Экран/аватар.
    if (gScreen.begin())
    {
        gScreen.setEmotion(Emotion::Neutral);
        Serial.println("[screen] avatar ready");
    }
    else
    {
        Serial.println("[screen] avatar init FAILED");
    }

    // Пан-тилт приводы.
    if (gMove.begin())
    {
        gMove.center();
        Serial.println("[move] pan/tilt ready");
    }
    else
    {
        Serial.println("[move] servos init FAILED");
    }

    // Динамик.
    if (gSound.begin())
    {
        Serial.println("[sound] speaker ready");
    }
    else
    {
        Serial.println("[sound] speaker init FAILED");
    }

    // Светодиоды: короткая зелёная вспышка при старте.
    if (gLeds.begin())
    {
        gLeds.setBrightness(40);
        gLeds.setColor(0, 255, 0);
        delay(200);
        gLeds.off();
    }

    // WebSocket-обработчики (команды протокола).
    setupCommands();
}

void loop()
{
    tickStateMachine();
    tickAudio();  // VAD: прослушивание микрофона и автозапись речи
    delay(10);
}