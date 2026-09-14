# Протокол взаимодействия с роботом по WebSocket

Робот (M5Stack CoreS3) подключается к серверу как **WebSocket-клиент**:
`ws://WS_HOST:WS_PORT/WS_PATH` (значения из `include/app_config.h`,
по умолчанию `ws://192.168.1.8:9001/`).

Транспорт — **текстовые фреймы** для команд и **бинарные фреймы** для аудио
(см. разделы «AUDIO — трансляция звука» и «Воспроизведение аудио»).

Ключевые слова команд регистронезависимы. Пробелы внутри аргументов не
допускаются. Каждая команда завершается либо подтверждением `ACK`, либо
ошибкой `ERR`.

## Команды (сервер → робот)

### PING
Запрос живого соединения.

```
PING
```
Ответ: `PONG:<uptime_ms>`

### STATUS
Запрос статуса робота.

```
STATUS
```
Ответ: `STATUS:online`

Периодически (каждые `WS_HEARTBEAT_INTERVAL_MS`) робот сам шлёт `STATUS:online`.

### EMOTION — вывод эмоций на экран
Задать выражение лица (аватар M5Stack-Avatar).

```
EMOTION:<name>
```
`name` ∈ `neutral | happy | angry | sad | doubt | sleepy`

Примеры:
```
EMOTION:happy
EMOTION:sleepy
```
Ответ: `ACK:EMOTION:<name>` / ошибка: `ERR:EMOTION:unknown emotion`

### MOVE — движение головы (пан-тилт)
Поворот относительно текущего положения или возврат в центр.

```
MOVE:left:<degrees>
MOVE:right:<degrees>
MOVE:up:<degrees>
MOVE:down:<degrees>
MOVE:center
```

Примеры:
```
MOVE:left:45
MOVE:up:30
MOVE:center
```
Ответ: `ACK:MOVE:left:45` (или `ACK:MOVE:center`) /
ошибка: `ERR:MOVE:invalid move`

### LED — подсветка (светодиоды на голове)
```
LED:<r>,<g>,<b>
```
`r,g,b` — 0..255.

Пример:
```
LED:255,0,0
```
Ответ: `ACK:LED:255,0,0` / ошибка: `ERR:LED:invalid color`

### AUDIO — трансляция звука с микрофона
Включить/выключить передачу звука с микрофона подключившемуся клиенту.

```
AUDIO:start
AUDIO:stop
```
Ответ: `ACK:AUDIO:start` / `ACK:AUDIO:stop` /
ошибка: `ERR:AUDIO:mic start failed`

После `AUDIO:start` робот захватывает звук (`MIC_SAMPLE_RATE` Гц, моно)
и шлёт **бинарные фреймы**:

```
byte[0]   = 1              (тип: AUDIO)
byte[1]   = 1              (кодек: PCM int16)
byte[2..] = PCM little-endian, MIC_FRAME_SAMPLES сэмплов на кадр
```

## Воспроизведение аудио (сервер → робот)

Сервер может отправить роботу аудио для воспроизведения в динамике теми же
бинарными фреймами того же формата:

```
byte[0]   = 1              (тип: AUDIO)
byte[1]   = 1              (кодек: PCM int16)
byte[2..] = PCM little-endian, int16 моно, SOUND_SAMPLE_RATE (16 кГц)
```

При получении такого фрейма робот воспроизводит его через `EspSound`.
Если в этот момент идёт трансляция с микрофона (`AUDIO:start` активен),
робот останавливает её (динамик и микрофон делят I2S-ресурсы на CoreS3).

## Ответы (робот → сервер)

| Формат | Назначение |
|--------|-----------|
| `ACK:<COMMAND>[:args]` | команда выполнена успешно |
| `ERR:<COMMAND>:<reason>` | команда отклонена (неверные аргументы / неизвестная команда) |
| `PONG:<uptime_ms>` | ответ на `PING` |
| `STATUS:online` | heartbeat / ответ на `STATUS` |

## Пример сессии

```
сервер → PING
робот  → PONG:120345

сервер → EMOTION:happy
робот  → ACK:EMOTION:happy

сервер → MOVE:left:45
робот  → ACK:MOVE:left:45

сервер → MOVE:center
робот  → ACK:MOVE:center

сервер → LED:0,255,0
робот  → ACK:LED:0,255,0

сервер → AUDIO:start
робот  → ACK:AUDIO:start
(робот шлёт бинарные PCM-фреймы)

сервер → AUDIO:stop
робот  → ACK:AUDIO:stop

сервер → FOOBAR
робот  → ERR:COMMAND:unknown command
```

## Реализация

- Модуль протокола: `src/protocol/protocol.{h,cpp}` (парсер + формирование ответов).
- Приём и исполнение команд: `src/aibot/main.cpp` → `onWsMessage()` →
  `protocol::parse()` → `handleCommand()` → подсистемы
  (`EspScreen`, `EspMovement`, `EspLeds`) с отправкой `ACK`/`ERR`.