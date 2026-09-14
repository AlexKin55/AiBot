// Тест камеры: захват видеопотока и вывод на экран.
//
// Берём кадры с камеры (EspCamera) и выводим их на дисплей робота через
// интерфейс экрана EspScreen::showFrame() в течение CAMERA_TEST_DURATION_MS.
// Успех проверяется по факту захвата и отображения хотя бы одного кадра.

#include <Arduino.h>

#include "app_config.h"
#include "camera/camera.h"
#include "screen/screen.h"
#include "test_util.h"

namespace {

EspCamera gCam;
EspScreen gScreen;

}  // namespace

void runVideoTests()
{
    Serial.println("[TEST] === Camera/video test on real hardware ===");

    report(gScreen.begin(), "screen begin()");
    report(gCam.begin(EspCamera::Config{}), "camera begin()");

    if (!gCam.isActive())
    {
        report(false, "camera active");
        return;
    }

    // Показываем живой видеопоток несколько секунд.
    const unsigned long start = millis();
    uint32_t frames = 0;
    while ((millis() - start) < CAMERA_TEST_DURATION_MS)
    {
        EspCamera::Frame f;
        if (gCam.grabFrame(f))
        {
            gScreen.showFrame(f.width, f.height, f.data);
            ++frames;
        }
        delay(5);
    }

    Serial.printf("[camera] frames displayed=%u\n", static_cast<unsigned>(frames));
    report(frames > 0, "camera frames displayed on screen");

    gCam.end();
}