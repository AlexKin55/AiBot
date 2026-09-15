#ifndef TEST_CONFIG_H_
#define TEST_CONFIG_H_

// Параметры, используемые только тестами (tests/*.cpp).
// Основной конфиг прошивки подключается автоматически.
#include "config/config.h"

// Короткий таймаут для негативного сценария Wi-Fi (неверные креды), мс.
#ifndef WIFI_TEST_WRONG_TIMEOUT_MS
#define WIFI_TEST_WRONG_TIMEOUT_MS 3000u
#endif

// Таймауты ожидания событий WebSocket в тесте, мс.
#ifndef WS_TEST_CONNECT_TIMEOUT_MS
#define WS_TEST_CONNECT_TIMEOUT_MS 15000u
#endif

#ifndef WS_TEST_RECV_TIMEOUT_MS
#define WS_TEST_RECV_TIMEOUT_MS 5000u
#endif

// Пределы углов поворота головы (пан-тилт), градусы (целые).
#ifndef MOVEMENT_MIN_ANGLE
#define MOVEMENT_MIN_ANGLE (-180)
#endif

#ifndef MOVEMENT_MAX_ANGLE
#define MOVEMENT_MAX_ANGLE 180
#endif

// Параметры воспроизведения звука в тесте.
#ifndef SOUND_SAMPLE_RATE
#define SOUND_SAMPLE_RATE 16000u
#endif

#ifndef SOUND_BEEP_HZ
#define SOUND_BEEP_HZ 880u
#endif

#ifndef SOUND_BEEP_MS
#define SOUND_BEEP_MS 200u
#endif

#ifndef SOUND_BEEP_COUNT
#define SOUND_BEEP_COUNT 3u
#endif

// Длительность записи голоса в объединённом тесте звука/микрофона, мс.
#ifndef MIC_TEST_RECORD_MS
#define MIC_TEST_RECORD_MS 5000u
#endif

// Длительность вывода видеопотока с камеры на экран в тесте, мс.
#ifndef CAMERA_TEST_DURATION_MS
#define CAMERA_TEST_DURATION_MS 8000u
#endif

// Время удержания каждого цвета светодиодов в тесте, мс.
#ifndef LED_TEST_COLOR_MS
#define LED_TEST_COLOR_MS 500u
#endif

#endif  // TEST_CONFIG_H_