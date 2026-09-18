# AIBot — голосовой робот-ассистент на M5Stack CoreS3

Робот (ESP32-S3 / M5Stack CoreS3) подключается к серверу **AiServer**
(`~/Work/AiServer`) по WebSocket и работает как голосовой ассистент:

- **слушает микрофон постоянно** и по резкому росту шума (голос) сам начинает
  запись — VAD-детекция речи на борту (никаких кнопок и команд с сервера);
- кодирует микрофон в **Opus** (16 кГц/моно, кадры 20 мс) и передаёт чанками
  (~2 с) на сервер;
- сервер сохраняет запись, распознаёт её в **Yandex Cloud** (STT → YandexGPT →
  TTS) и возвращает озвученный ответ тоже в **Opus**;
- робот декодирует ответ в фоновой задаче и произносит его на динамике.

Аудиоканал полностью на Opus (codec 2) в обе стороны — экономия трафика
~64× против PCM. PCM в WebSocket-канале не используется.

## Возможности

- **VAD-запись**: робот сам определяет начало фразы по уровню шума
  (адаптивный порог), завершает по тишине (~0.9 с) или таймауту сегмента (5 с);
- **Opus-кодирование** микрофона (libopus, SILK WB 16 кГц, ~19–20 кбит/с);
- **WebSocket-клиент** с авто-переподключением;
- **голосовой ответ**: декодирование Opus-чанка в фоновой задаче с большим
  стеком, проигрывание одним непрерывным буфером;
- **эмоции на экране** (M5Stack-Avatar): `neutral | happy | angry | sad |
  doubt | sleepy`;
- **движение головы** (пан-тилт): `MOVE:left/right/up/down:<deg>` и `MOVE:center`;
- **LED-подсветка**: `LED:<r>,<g>,<b>`;
- интеграционные тесты модулей на железе и через протокол (с сервером).

## Структура проекта

```
AiBot/
├── platformio.ini          # окружения PlatformIO (приложение + тесты)
├── config/
│   ├── config.h            # параметры прошивки (Wi-Fi, WS, микрофон, VAD)
│   └── test_config.h       # параметры тестовой прошивки
├── src/
│   ├── aibot/              # приложение: state machine, команды, main.cpp
│   │   ├── main.cpp        # setup()/loop(): инициализация + tick
│   │   ├── app.h           # глобальные подсистемы и аудио-глобалы
│   │   ├── state_machine.{h,cpp}  # Wi-Fi -> WebSocket -> ready
│   │   └── commands.{h,cpp}       # VAD, захват микрофона, обработка команд
│   ├── wifi/               # подключение к точке доступа (авто-реконнект)
│   ├── websocket/          # WebSocket-клиент (подключение, события)
│   ├── protocol/           # парсер текстовых команд протокола
│   ├── move/               # пан-тилт (сервоприводы)
│   ├── screen/             # экран + аватар с эмоциями
│   ├── sound/              # динамик (PCM/тоны)
│   ├── microfon/           # I2S-микрофон (кооперативная остановка, RMS)
│   ├── opus/               # обёртки OpusEncoderWrapper/OpusDecoderWrapper
│   ├── camera/             # камера (резерв)
│   └── leds/               # LED-подсветка (резерв)
├── tests/                  # тестовая прошивка (tests/tests.cpp + модули)
├── scripts/
│   ├── build.sh            # сборка (pio run -e m5stack-cores3)
│   ├── programming.sh      # сборка + прошивка в устройство (upload)
│   ├── monitor.sh          # монитор последовательного порта
│   ├── test_build.sh       # сборка тестового окружения
│   └── test_programming.sh # прошивка тестовой прошивки
├── docs/
│   └── protocol.md         # спецификация протокола WebSocket
└── README.md
```

## Как это работает

### VAD-цикл записи (робот — инициатор)

1. В состоянии `ready` робот постоянно слушает микрофон (без шумового шлюза —
   полный поток, чтобы не резать тихие согласные) и оценивает уровень RMS.
2. Фон адаптируется в тишине; порог детекции = `max(VAD_MIN_THRESHOLD,
   noise_floor × 1.6)`.
3. При голосе (RMS ≥ порога, `VAD_START_HANGOVER_FRAMES` кадров подряд) робот
   шлёт серверу текст `RECORD:start` и начинает копить PCM-чанки.
4. Завершение фразы — тишина `VAD_SILENCE_MS` (900 мс) или принудительно по
   `VAD_MAX_SEGMENT_MS` (5 с): робот шлёт `RECORD:stop` и досылает последний
   неполный чанк.

```
робот  → RECORD:start                    (по голосу)
робот  → [PCM-чанк]  [1][1][pcm int16 LE]            (каждые ~2 с)
робот  → RECORD:stop                     (тишина/таймаут)
сервер → [PCM-чанк]  [1][1][pcm int16 LE]            (озвучка ответа)
```

### Обработка звука

- **Запись**: микрофон → PCM 16 кГц моно (int16 LE) → чанк-буфер
  (`MIC_AUDIO_CHUNK_SECONDS` = 2 с) → один бинарный WebSocket-фрейм
  `[тип=1][кодек=1][pcm]`. Сжатия нет — сервер передаёт этот же PCM
  в Yandex STT (raw LINEAR16_PCM).
- **Воспроизведение**: входящий PCM-чанк ставится в FreeRTOS-очередь
  `gPlayQ`; задача `playbackTask` (стек 32 КБ, приоритет 2) играет весь
  чанк одним вызовом `playSample` (без «щелчков»). Перед воспроизведением
  микрофон останавливается (I2S делят динамик и микрофон).

## Конфигурация

Все параметры — в [`config/config.h`](config/config.h), переопределяются
флагами сборки (`-DWIFI_SSID="..."` и т.п.) без правки кода.

| Группа | Макрос | По умолчанию | Описание |
|--------|--------|--------------|----------|
| Wi-Fi | `WIFI_SSID` / `WIFI_PASS` | — | точка доступа |
| | `WIFI_CONNECT_TIMEOUT_MS` | 20000 | таймаут подключения, мс |
| | `WIFI_RECONNECT_DELAY_MS` | 5000 | пауза между попытками, мс |
| WebSocket | `WS_HOST` / `WS_PORT` / `WS_PATH` | `192.168.1.8` / `9001` / `/` | адрес AiServer |
| Микрофон | `MIC_SAMPLE_RATE` | 16000 | частота дискретизации, Гц |
| | `MIC_FRAME_SAMPLES` | 320 | кадр 20 мс (фиксированный кадр PCM) |
| | `MIC_CHANNELS` | 1 | каналы |
| | `MIC_AUDIO_CHUNK_SECONDS` | 1 | длина PCM-чанка на сервер, с |
| VAD | `VAD_MIN_THRESHOLD` | 250.0 | мин. порог RMS детекции речи |
| | `VAD_NOISE_ADAPT` | 0.03 | скорость адаптации фона (на кадр) |
| | `VAD_START_HANGOVER_FRAMES` | 2 | кадров подряд выше порога для старта |
| | `VAD_SILENCE_MS` | 900 | тишина после речи → конец фразы |
| | `VAD_MAX_SEGMENT_MS` | 5000 | макс. длина фразы (принудительный стоп) |

## Сборка и прошивка

Требуется PlatformIO ([platformio.ini](platformio.ini), платформа
`espressif32`, плата `m5stack-cores3`, Arduino-фреймворк, C++17).

```bash
# Сборка прошивки робота
./scripts/build.sh
# или pio run -e m5stack-cores3

# Сборка + загрузка в устройство (порт из platformio.ini, upload_port)
./scripts/programming.sh
# PORT=/dev/ttyUSB0 ./scripts/programming.sh   # переопределить порт

# Монитор последовательного порта (лог робота: [vad], [mic], [ws], ...)
./scripts/monitor.sh
```

Логика приложения — `src/aibot/`: `setup()` инициализирует подсистемы и
стартует Wi-Fi/WebSocket, `loop()` каждый тик вызывает
`tickStateMachine()` + `tickAudio()` (VAD).

## Тестирование

### На железе (тестовая прошивка)

Окружение `m5stack-cores3-tests` собирает `tests/tests.cpp` со сценариями
модулей (wifi, websocket, move, screen, audio, ...):

```bash
ENV_NAME=m5stack-cores3-tests ./scripts/build.sh
ENV_NAME=m5stack-cores3-tests ./scripts/programming.sh   # или ./scripts/test_build.sh / test_programming.sh
```

Вывод — `[TEST] PASS/FAIL` и итоговый `FINAL RESULT`.

### Через протокол (интеграционные тесты с AiServer)

`~/Work/AiServer/tests/*.py` имитируют сервер и гоняют робота по протоколу:
`audio.py` ждёт `RECORD:start` по голосу, собирает PCM-чанки до `RECORD:stop`,
сохраняет `.wav`, распознаёт в Yandex и озвучивает ответ роботу.

```bash
cd ~/Work/AiServer
./scripts/run_tests.sh
```

Робот должен работать на **прошивке приложения** (`src/aibot`) и иметь в
`config/config.h` `WS_HOST` = адрес машины, где запущен сервер.

## Протокол

Полная спецификация — [`docs/protocol.md`](docs/protocol.md). Кратко:

- **Робот → сервер (текст):** `RECORD:start` / `RECORD:stop` (VAD),
  `PONG:<ms>` (ответ на PING), `ACK:*`/`ERR:*`, `HB` (heartbeat каждые
  15 с — сервер только логирует, соединение не обрывает).
- **Сервер → робот (текст):** `PING`, `EMOTION:<name>`,
  `MOVE:left|right|up|down:<deg>`, `MOVE:center`, `LED:<r>,<g>,<b>`.
- **Аудио (бинарные фреймы), единственный кодек — PCM:**

```
byte[0]   = 1   (тип: AUDIO)
byte[1]   = 1   (кодек: PCM)
byte[2..] = сырые сэмплы int16 LE (16 кГц, моно)
```

## Интеграция с AiServer

Сервер в `~/Work/AiServer` (FastAPI + WebSocket):

- принимает PCM-чанки робота (codec 1, `Recorder` → WAV) и параллельно
  кормит ими потоковый SpeechKit STT v3 `RecognizeStreaming`
  (raw LINEAR16_PCM);
- по `RECORD:stop` закрывает стрим, получает текст → YandexGPT v3 →
  SpeechKit TTS v3 (PCM) → озвучивает ответ роботу PCM-чанком codec 1;
- `HB` от робота (каждые 15 с) только логирует — соединение не обрывает;
- `GET /ask` — последний текстовый ответ, `GET /health` — состояние.

```bash
cd ~/Work/AiServer
./scripts/run_server.sh     # uvicorn src.server:app --host 0.0.0.0 --port 9001
```

Требования к креденшалам Yandex: env `YANDEX_API_KEY` / `YANDEX_FOLDER_ID`
или секция `yandex` в `config/settings.json`.
