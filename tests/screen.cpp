// Тест вывода эмоций на экран (аватар M5Stack-Avatar) на реальном железе.
//
// Сценарии: инициализация экрана, установка каждой из эмоций (проверяется по
// внутреннему состоянию getEmotion()) и работа речевого пузыря. После каждого
// переключения делается небольшая пауза, чтобы анимация лица успела
// отрисоваться — визуально эмоции можно наблюдать на дисплее робота.

#include <Arduino.h>

#include "screen/screen.h"
#include "test_util.h"

namespace {

EspScreen gScreen;

void testSetEmotion(Emotion e, const char* name)
{
    gScreen.setEmotion(e);
    report(gScreen.getEmotion() == e, name);
    // Даём аватару время отрисовать выражение.
    delay(400);
}

}  // namespace

void runScreenTests()
{
    Serial.println("[TEST] === Screen (emotion) tests on real hardware ===");

    report(gScreen.begin(), "screen begin()");

    testSetEmotion(Emotion::Neutral, "emotion Neutral");
    testSetEmotion(Emotion::Happy, "emotion Happy");
    testSetEmotion(Emotion::Angry, "emotion Angry");
    testSetEmotion(Emotion::Sad, "emotion Sad");
    testSetEmotion(Emotion::Doubt, "emotion Doubt");
    testSetEmotion(Emotion::Sleepy, "emotion Sleepy");

    // Речевой пузырь: показать текст и очистить.
    gScreen.setSpeechText("Hello!");
    delay(400);
    gScreen.clearSpeechText();
    delay(200);

    // Возвращаем нейтральное выражение.
    gScreen.setEmotion(Emotion::Neutral);
}