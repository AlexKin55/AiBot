# План: Wi-Fi тест на реальном железе (workspace `/home/alex/Work/AiBot`)

## Контекст

- Логика Wi-Fi должна лежать в `src/wifi/`.
- Тестирование на реальном железе — в `tests/` (главный цикл `tests/tests.cpp`,
  сценарии `tests/wifi.cpp`).
- Плата: M5Stack CoreS3, Arduino, PlatformIO.
- За основу взята реализация из реального проекта `Robert/aibot`
  (`lib/wifi/esp_wifi_manager.cpp` + `include/iwifi_manager.h`).

## Целевая структура

```text
AiBot/
├── platformio.ini
├── include/
│   └── app_config.h          # SSID/PASS/таймауты (переопределяемы через -D)
├── src/
│   └── wifi/
│       ├── wifi.h            # IWifiManager (абстракция) + EspWifiManager
│       └── wifi.cpp          # реализация через Arduino <WiFi.h>
└── tests/
    ├── tests.cpp             # setup()/loop() — точка входа тестовой прошивки
    └── wifi.cpp              # сценарии теста Wi-Fi + итоговый отчёт по Serial
```

## Конфигурация сборки (platformio.ini)

Тестовая прошивка берёт точку входа из `tests/`, а модуль `src/wifi` подключается
как библиотека:

```ini
[env:m5stack-cores3]
platform = espressif32
board = m5stack-cores3
framework = arduino

upload_port = /dev/ttyACM0
monitor_speed = 115200
monitor_dtr = 0
monitor_rts = 0

# Главный цикл находится в tests/tests.cpp
src_dir = tests
# src/wifi компилируется как библиотека "wifi"
lib_extra_dirs = src

build_flags =
    -std=gnu++17
    -Iinclude
    -Isrc
    -D ARDUINO_USB_CDC_ON_BOOT=1
    -D ARDUINO_USB_MSC_ON_BOOT=0
    -D CONFIG_ARDUINO_LOOP_STACK_SIZE=32768

build_unflags =
    -std=gnu++11
```

## Модуль Wi-Fi

`src/wifi/wifi.h` — абстракция `IWifiManager` + класс `EspWifiManager`
(`connect(ssid, pass, timeoutMs)`, `isConnected()`).

`src/wifi/wifi.cpp` — реализация на Arduino `<WiFi.h>` (режим STA, ожидание
подключения с таймаутом, возврат `WL_CONNECTED`).

`include/app_config.h` — настраиваемые `WIFI_SSID`, `WIFI_PASS`,
`WIFI_CONNECT_TIMEOUT_MS` (переопределяются флагами `-D`).

## Сценарии теста (tests/wifi.cpp)

Порядок выполнения на реальном железе:

1. `testConnectCorrect()` — подключение с корректными SSID/PASS → ожидается `true`.
2. `testIsConnectedAfterConnect()` — после успешного подключения `isConnected()`
   возвращает `true`; выводится IP и RSSI.
3. `testConnectIdempotent()` — повторный `connect()` при уже установленной связи
   возвращает `true` мгновенно.
4. `testConnectWrongCreds()` — подключение с заведомо неверными данными
   (короткий таймаут ~3 c) → ожидается `false`.

Итог печатается в Serial: `[TEST] RESULT: passed=X failed=Y total=Z`.

## Точка входа (tests/tests.cpp)

`setup()`: инициализация Serial (115200), вызов `runWifiTests()`.
`loop()`: idle (тест выполняется один раз, результат читается монитором).

## Флоу проверки на железе

```mermaid
flowchart LR
    A[pio run -e m5stack-cores3] --> B[прошивка в CoreS3]
    B --> C[Serial Monitor 115200]
    C --> D[запуск runWifiTests]
    D --> E[отчёт PASS/FAIL]
```

## Открытые вопросы / допущения

- Креды сети по умолчанию — из реального проекта; при необходимости
  переопределяются флагами сборки без правки кода.
- Если хочется canonical PlatformIO test-framework (`pio test`, Unity) — это
  отдельная доработка; здесь выбран автономный test-harness с выводом в Serial.