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

// Heartbeat (робот -> сервер), мс. Каждые 15 с робот шлёт текстовый фрейм
// "HB", чтобы роутер/Wi-Fi не сбрасывал простаивающую TCP-сессию (NAT
// умирает за ~2 минуты без трафика). Сервер только логирует HB и НЕ обрывает
// соединение при его отсутствии.
#ifndef WS_HEARTBEAT_INTERVAL_MS
#define WS_HEARTBEAT_INTERVAL_MS 15000u
#endif

// Параметры захвата звука с микрофона.
#ifndef MIC_SAMPLE_RATE
#define MIC_SAMPLE_RATE 16000u
#endif

// Длина аудиочанка (секунды), которым запись отправляется на сервер.
// Непрерывный стриминг во время записи создаёт помехи I2S на CoreS3
// (скрипы/свисты), поэтому звук копится локально и уходит пакетами.
// 1 с PCM = 32 КБ — пакеты меньше, отправка из I2S-коллбека короче.
#ifndef MIC_AUDIO_CHUNK_SECONDS
#define MIC_AUDIO_CHUNK_SECONDS 1u
#endif

#ifndef MIC_CHANNELS
#define MIC_CHANNELS 1u
#endif

// Кадр захвата = 320 сэмплов (20 мс @16 кГц) — фиксированный кадр PCM.
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

// ---------------------------------------------------------------------------
// VAD: робот сам начинает запись по резкому росту шума и завершает по тишине
// или по истечении VAD_MAX_SEGMENT_MS, отправляя серверу RECORD:start/stop.
// ---------------------------------------------------------------------------

// Минимальный порог RMS для детекции речи (int16-шкала). Подобран под
// фоновый шум комнаты: постоянный фон ~700-1300 не должен запускать запись.
#ifndef VAD_MIN_THRESHOLD
#define VAD_MIN_THRESHOLD 1400.0f
#endif

// Скорость адаптации оценки фонового шума (0..1, кадр 20 мс).
#ifndef VAD_NOISE_ADAPT
#define VAD_NOISE_ADAPT 0.03f
#endif

// Кадров подряд выше порога, чтобы начать запись. 5 кадров (100 мс)
// отсекают одиночные щелчки и короткие всплески шума.
#ifndef VAD_START_HANGOVER_FRAMES
#define VAD_START_HANGOVER_FRAMES 5u
#endif

// Тишина (мс) после последней речи — завершение фразы. 1200 мс держат
// паузы/дыхание внутри фразы, чтобы сегмент не рвался на старт/стоп.
#ifndef VAD_SILENCE_MS
#define VAD_SILENCE_MS 1200u
#endif

// Максимальная длина фразы, мс (принудительный стоп).
#ifndef VAD_MAX_SEGMENT_MS
#define VAD_MAX_SEGMENT_MS 5000u
#endif

// Таймаут простоя при воспроизведении аудио (мс): если от сервера больше
// не приходит ни одного PCM-чанка (в т.ч. нулевого маркера конца), робот
// сам освобождает I2S-динамик и возвращает микрофон к прослушиванию (VAD).
#ifndef PLAYBACK_IDLE_TIMEOUT_MS
#define PLAYBACK_IDLE_TIMEOUT_MS 6000u
#endif

#endif  // CONFIG_H_