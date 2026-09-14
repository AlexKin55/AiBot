// Прошивка робота AIBot: точка входа.
//
// Логика вынесена в файлы рядом:
//   app.h            - глобальные подсистемы (определены здесь)
//   state_machine.*  - state machine, подключение Wi-Fi
//   commands.*       - WebSocket-события и выполнение команд протокола

#include <Arduino.h>
#include <WiFi.h>

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

void setup()
{
    Serial.begin(115200);
    delay(300);

    Serial.println();
    Serial.println("[app] AIBot firmware started");
    Serial.printf("[app] chip model: %s\n", ESP.getChipModel());

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
    delay(10);
}