#ifndef CONFIG_H_
#define CONFIG_H_

#include <cstdint>

// ---------------------------------------------------------------------------
// Конфигурация прошивки робота (приложение src/aibot).
// Все значения можно переопределить флагами сборки (-DWIFI_SSID="..." и т.п.)
// без правки кода. Параметры тестов вынесены в config/test_config.h.
// ---------------------------------------------------------------------------

// Точка доступа.
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

// WebSocket-сервер, к которому подключается робот.
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

// Параметры захвата звука с микрофона.
#ifndef MIC_SAMPLE_RATE
#define MIC_SAMPLE_RATE 16000u
#endif

// Длина аудиочанка (секунды), которым запись отправляется на сервер.
// Непрерывный стриминг во время записи создаёт помехи I2S на CoreS3
// (скрипы/свисты), поэтому звук копится локально и уходит пакетами.
#ifndef MIC_AUDIO_CHUNK_SECONDS
#define MIC_AUDIO_CHUNK_SECONDS 2u
#endif

#ifndef MIC_CHANNELS
#define MIC_CHANNELS 1u
#endif

// Кадр захвата = 320 сэмплов (20 мс @16 кГц) — фиксированный кадр Opus.
#ifndef MIC_FRAME_SAMPLES
#define MIC_FRAME_SAMPLES 320u
#endif

// Шумовой шлюз: порог RMS, при превышении которого звук передаётся клиенту.
#ifndef MIC_NOISE_THRESHOLD
#define MIC_NOISE_THRESHOLD 100.0f
#endif

// Сколько кадров ещё передавать после того, как уровень упал ниже порога.
#ifndef MIC_HANGOVER_FRAMES
#define MIC_HANGOVER_FRAMES 4u
#endif

#endif  // CONFIG_H_