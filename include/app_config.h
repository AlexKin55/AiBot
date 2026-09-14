#ifndef APP_CONFIG_H_
#define APP_CONFIG_H_

#include <cstdint>

// Параметры точки доступа для теста Wi-Fi на реальном железе.
// Все значения можно переопределить флагами сборки (-DWIFI_SSID="..." и т.п.)
// без правки кода.
#ifndef WIFI_SSID
#define WIFI_SSID "MGTS_GPON_AEDE"
#endif

#ifndef WIFI_PASS
#define WIFI_PASS "GxhpbRa7"
#endif

// Таймаут одной попытки подключения к точке доступа, мс.
#ifndef WIFI_CONNECT_TIMEOUT_MS
#define WIFI_CONNECT_TIMEOUT_MS 20000u
#endif

// Задержка между повторными попытками подключения при потере связи, мс.
#ifndef WIFI_RECONNECT_DELAY_MS
#define WIFI_RECONNECT_DELAY_MS 5000u
#endif

// Короткий таймаут для негативного сценария (заведомо неверные креды), мс.
#ifndef WIFI_TEST_WRONG_TIMEOUT_MS
#define WIFI_TEST_WRONG_TIMEOUT_MS 3000u
#endif

// WebSocket-сервер для теста send/recv.
#ifndef WS_HOST
#define WS_HOST "192.168.1.8"
#endif

#ifndef WS_PORT
#define WS_PORT 9001u
#endif

#ifndef WS_PATH
#define WS_PATH "/"
#endif

// Период отправки периодического статус-сообщения на сервер, мс (0 = откл.).
#ifndef WS_HEARTBEAT_INTERVAL_MS
#define WS_HEARTBEAT_INTERVAL_MS 10000u
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

// Параметры воспроизведения звука в динамиках.
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

// Параметры захвата звука с микрофона.
#ifndef MIC_SAMPLE_RATE
#define MIC_SAMPLE_RATE 16000u
#endif

#ifndef MIC_CHANNELS
#define MIC_CHANNELS 1u
#endif

#ifndef MIC_FRAME_SAMPLES
#define MIC_FRAME_SAMPLES 512u
#endif

// Шумовой шлюз: порог RMS, при превышении которого звук передаётся клиенту.
#ifndef MIC_NOISE_THRESHOLD
#define MIC_NOISE_THRESHOLD 100.0f
#endif

// Сколько кадров ещё передавать после того, как уровень упал ниже порога.
#ifndef MIC_HANGOVER_FRAMES
#define MIC_HANGOVER_FRAMES 4u
#endif

// Длительность записи голоса в объединённом тесте звука/микрофона, мс.
#ifndef MIC_TEST_RECORD_MS
#define MIC_TEST_RECORD_MS 3000u
#endif

// Длительность вывода видеопотока с камеры на экран в тесте, мс.
#ifndef CAMERA_TEST_DURATION_MS
#define CAMERA_TEST_DURATION_MS 8000u
#endif

// Время удержания каждого цвета светодиодов в тесте, мс.
#ifndef LED_TEST_COLOR_MS
#define LED_TEST_COLOR_MS 500u
#endif

#endif  // APP_CONFIG_H_