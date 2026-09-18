// WebSocket-события и обработка команд протокола:
// движение, эмоции, светодиоды, аудиотрансляция с микрофона.

#include "commands.h"

#include <Arduino.h>
#include <M5Unified.h>

#include <algorithm>
#include <atomic>
#include <cstring>
#include <deque>
#include <freertos/queue.h>
#include <string>

#include "app.h"
#include "config/config.h"
#include "leds/leds.h"
#include "log/log.h"
#include "microfon/microfon.h"
#include "move/move.h"
#include "protocol/protocol.h"
#include "screen/screen.h"
#include "state_machine.h"

namespace {

Emotion parseEmotion(const std::string& name)
{
    if (name == "happy") return Emotion::Happy;
    if (name == "angry") return Emotion::Angry;
    if (name == "sad") return Emotion::Sad;
    if (name == "doubt") return Emotion::Doubt;
    if (name == "sleepy") return Emotion::Sleepy;
    return Emotion::Neutral;
}

void sendResponse(const protocol::Command& cmd, bool ok, const char* reason = nullptr)
{
    if (gWs.isConnected())
    {
        gWs.sendText(protocol::response(cmd, ok, reason));
    }
}

// Отправляет накопленный чанк как бинарный фрейм:
// [0]=kAudioFrameType, [1]=kAudioCodecPcm, [2..]=сырые PCM-байты (int16 LE).
void sendAudioChunk()
{
    // Без живого соединения не отправляем: микрофонная задача может наполнить
    // чанк, пока Wi-Fi/WebSocket уже разорвался, а send по битому сокету
    // падает с LoadProhibited. Чанк просто сбрасывается.
    if (gAudioChunk.empty() || !gWs.isConnected())
    {
        gAudioChunk.clear();
        gAudioChunkPackets = 0;
        return;
    }
    const size_t payload = 2 + gAudioChunk.size();
    if (gAudioFrame.size() < payload)
    {
        gAudioFrame.resize(payload);
    }
    uint8_t* out = gAudioFrame.data();
    out[0] = kAudioFrameType;
    out[1] = kAudioCodecPcm;
    std::memcpy(out + 2, gAudioChunk.data(), gAudioChunk.size());
    gWs.sendBinary(out, payload);
}

// Callback захвата микрофона: копит сырые PCM-байты (int16 LE, 16 кГц/моно)
// в чанк-буфер. Чанк отправляется по достижении MIC_AUDIO_CHUNK_SECONDS:
// непрерывный стриминг по Wi-Fi во время записи даёт помехи I2S на CoreS3.
// Сжатия нет — канал целиком PCM (codec 1), сервер передаёт его в Yandex
// STT как есть (raw LINEAR16_PCM).
//
// Входящее аудио для воспроизведения: большой вызов playSample блокирует
// loopTask, поэтому чанки ставятся в очередь и воспроизводятся отдельной
// задачей с собственным стеком.
struct PlaybackMsg
{
    std::vector<uint8_t> data;  // тело фрейма: сырые PCM-байты (int16 LE)
};

QueueHandle_t gPlayQ = nullptr;
// Идёт сессия воспроизведения: началась с первого чанка, завершается
// нулевым чанком (маркер конца) или таймаутом PLAYBACK_IDLE_TIMEOUT_MS.
// Пока активна — микрофон (VAD) не перезапускается между чанками.
volatile bool gPlaybackActive = false;
// Момент блокировки VAD ожиданием озвучки (ставится в stopSegment).
// Если сервер вообще не пришлёт аудио (пустой STT/ошибка), по таймауту
// PLAYBACK_IDLE_TIMEOUT_MS блокировка снимается и микрофон возвращается.
uint32_t gPlaybackBlockedAt = 0;

void resetPlaybackState();

// Ждёт, пока динамик доиграет накопленный DMA-буфер (хвост озвучки),
// но не дольше ~2 с. Вызывается перед возвратом микрофона, чтобы он не
// услышал остаток ответа (иначе VAD открывает ложный сегмент — «эхо»).
void waitSpeakerIdle()
{
    for (int i = 0; i < 200 && M5.Speaker.isPlaying(); ++i)
    {
        vTaskDelay(10);
    }
}

void playbackTask(void* /*arg*/)
{
    // Весь чанк одним вызовом playSample: он ждёт окончания текущего звука,
    // поэтому дробление на мелкие куски давало бы серию щелчков.
    // Чанк 2 с = 64 КБ PCM.
    //
    // ВАЖНО: M5.Speaker.playRaw НЕ копирует данные — задача динамика читает
    // сэмплы напрямую из буфера чанка, пока тот играется. Удалять буфер сразу
    // после playSample нельзя (use-after-free: чтение освобождённой кучи даёт
    // треск/заикание). Освобождение делаем ПО ФАКТУ ПРОЧТЕНИЯ:
    // M5.Speaker.isPlaying(0) возвращает число чанков, чьи данные динамик
    // ещё держит (published/playing, максимум 2). Все опубликованные ранее —
    // гарантированно доиграны, их буферы можно удалять сразу.
    std::deque<PlaybackMsg*> inflight;  // опубликованные в M5 чанки (по порядку)

    while (true)
    {
        PlaybackMsg* msg = nullptr;
        // Ожидание следующего чанка. Возврат микрофона — только по концу
        // потока: нулевой чанк (маркер от сервера) либо таймаут без данных.
        if (xQueueReceive(gPlayQ, &msg,
                          pdMS_TO_TICKS(PLAYBACK_IDLE_TIMEOUT_MS)) != pdTRUE)
        {
            resetPlaybackState();
            // Дожидаемся, пока DMA-буфер динамика доиграет хвост: если
            // включить микрофон раньше, он услышит остаток ответа и VAD
            // запустит ложный сегмент («эхо» — голос повторяется).
            waitSpeakerIdle();
            for (auto* m : inflight) { delete m; }
            inflight.clear();
            Serial.println("[audio] playback idle timeout, mic back to VAD");
            continue;
        }
        if (msg == nullptr)
        {
            continue;
        }
        const uint8_t* data = msg->data.data();
        const size_t size = msg->data.size();
        if (size >= sizeof(int16_t))
        {
            // Перед воспроизведением убеждаемся, что динамик инициализирован
            // (микрофон выключает его при переключении общего I2S на CoreS3).
            if (!gSound.ensureReady())
            {
                LOG_W("[audio] speaker not ready, chunk skipped\n");
                delete msg;
            }
            else
            {
                const int16_t* samples =
                    reinterpret_cast<const int16_t*>(data);
                const size_t count = size / sizeof(int16_t);
                gSound.playSample(samples, count);
                LOG_D("[audio] played pcm chunk: %u samples (%.2f s)\n",
                      static_cast<unsigned>(count),
                      static_cast<double>(count) / MIC_SAMPLE_RATE);

                inflight.push_back(msg);
                // Сколько чанков динамик сейчас реально читает/держит.
                const size_t flying = M5.Speaker.isPlaying(0);
                // Освобождаем все, кроме самой свежей тройки (минимум один —
                // только что опубликованный): остальные прочитаны полностью.
                const size_t keep = std::max<size_t>(flying, 1u);
                while (inflight.size() > keep)
                {
                    delete inflight.front();
                    inflight.pop_front();
                }
            }
        }
        else
        {
            // Нулевой чанк — маркер конца озвучки от сервера. Ждём, пока
            // динамик доиграет хвост, и только потом возвращаем микрофон
            // (иначе микрофон услышит остаток ответа -> ложный сегмент/эхо).
            resetPlaybackState();
            waitSpeakerIdle();
            for (auto* m : inflight) { delete m; }
            inflight.clear();
            delete msg;
            Serial.println("[audio] playback done (eof), mic back to VAD");
        }
    }
}

// Ставит входящий PCM-чанк в очередь воспроизведения (если очередь полна —
// чанк отбрасывается).
void enqueuePlayback(const uint8_t* data, size_t size)
{
    if (gPlayQ == nullptr)
    {
        gPlayQ = xQueueCreate(4, sizeof(PlaybackMsg*));
        if (gPlayQ != nullptr)
        {
            // Стек 64 КБ: playRaw + DMA-вывод I2S при длинных чанках
            // (до 0.25 с = 8 КБ PCM). Приоритет 2 — как советует M5Unified,
            // чтобы не было шумов в выводе динамика.
            xTaskCreatePinnedToCore(playbackTask, "playpcm", 65536, nullptr,
                                    2, nullptr, 0);
        }
    }
    if (gPlayQ == nullptr)
    {
        return;
    }
    auto* msg = new PlaybackMsg();
    msg->data.assign(data, data + size);
    if (size > 0)
    {
        gPlaybackActive = true;  // сессия воспроизведения идёт
    }
    if (xQueueSend(gPlayQ, &msg, 0) != pdTRUE)
    {
        delete msg;  // очередь занята — пропускаем (не накапливаем)
    }
    if (size > 0)
    {
        // Чанки пошли — автоснятие блокировки по таймауту больше не нужно:
        // сессию закроет playbackTask (нулевой чанк / таймаут очереди).
        gPlaybackBlockedAt = 0;
    }
}

void onAudioSamples(const int16_t* data, size_t samples)
{
    if (!gAudioCapturing)
    {
        return;
    }
    // Накопление и отправка PCM — всё в I2S-задаче микрофона: буфер
    // gAudioChunk однопотоковый. Гонка за WebSocket-клиент исключена
    // мьютексом на отправках (см. websocket.h): sendBinary из этой задачи
    // и sendText из loopTask сериализуются.
    const size_t bytes = samples * sizeof(int16_t);
    if (gAudioChunk.size() + bytes > gAudioChunkMaxBytes)
    {
        sendAudioChunk();
        gAudioChunk.clear();
        gAudioChunkPackets = 0;
    }
    const uint8_t* raw = reinterpret_cast<const uint8_t*>(data);
    gAudioChunk.insert(gAudioChunk.end(), raw, raw + bytes);
    ++gAudioChunkPackets;
}

// ---------------------------------------------------------------------------
// VAD: робот сам начинает запись по резкому росту шума (RMS > порога) и
// завершает её по тишине (VAD_SILENCE_MS) или по таймауту
// (VAD_MAX_SEGMENT_MS), отправляя серверу RECORD:start / RECORD:stop.
// Микрофон слушает постоянно в состоянии kReady (кроме времени воспроизведения).
// ---------------------------------------------------------------------------

float gLatestRms = 0.0f;        // RMS последнего кадра (из noiseCb_)
float gNoiseFloor = 0.0f;       // оценка фонового шума
float gVadThr = VAD_MIN_THRESHOLD;  // текущий порог детекции речи
bool gVadActive = false;        // идёт VAD-сегмент записи
uint32_t gVadSegStartMs = 0;    // время начала сегмента
uint32_t gVadSpeechMs = 0;      // время последнего «речевого» кадра
unsigned gVadStartCount = 0;    // кадры подряд выше порога

// Флаг «голова двигается»: ставит motionTask (ядро 0), снимает по завершении
// серии движений. Читает только loopTask (tickAudioInternal) — микрофон/VAD
// во время движения останавливаются именно там, чтобы все изменения общего
// состояния микрофона жили в одной задаче без гонок.
std::atomic<bool> gMoveInProgress{false};

// Снимает блокировку VAD после воспроизведения/ожидания ответа.
//
// Принцип «не трогать VAD при озвучке»: во время воспроизведения отключаем
// ТОЛЬКО микрофон (см. onWsMessage -> gMic.stop()), а состояние VAD
// (gVadActive, gAudioCapturing, таймеры gVadSegStartMs/gVadSpeechMs)
// сохраняем. Если на момент старта озвучки шёл активный сегмент, то после
// неё tickVad() корректно закроет его по таймауту тишины и отправит
// RECORD:stop.
//
// Раньше здесь стоял полный сброс VAD «молча» (без RECORD:stop): сервер
// навсегда зависал с открытым сегментом и игнорировал повторные
// RECORD:start (симптом «робот долго молчит после вопроса»). Поэтому
// gVadActive/gAudioCapturing/таймеры здесь НЕ трогаем.
//
// Адаптированный фон (gNoiseFloor/gVadThr) тоже не сбрасываем: если обнулить
// его, порог падает до VAD_MIN_THRESHOLD (100) и обычный фоновый шум
// микрофона (RMS ~1000-1500) принимается за речь.
//
// Счётчик накопления старта gVadStartCount сбрасываем — это защита от
// ложного сегмента из застывшего RMS микрофона, а не состояние записи.
void resetPlaybackState()
{
    gPlaybackActive = false;
    gPlaybackBlockedAt = 0;
    gVadStartCount = 0;
}

void startSegment()
{
    if (!gWs.isConnected())
    {
        gVadStartCount = 0;  // нет связи — речь не пишем
        return;
    }
    gAudioChunk.clear();
    gAudioChunkPackets = 0;
    gAudioCapturing = true;
    gVadActive = true;
    gVadSegStartMs = millis();
    gVadSpeechMs = millis();
    gWs.sendText("RECORD:start");
    Serial.println("[vad] speech started -> RECORD:start");
}

void stopSegment()
{
    if (!gVadActive)
    {
        return;
    }
    gVadActive = false;
    gAudioCapturing = false;
    if (gWs.isConnected())
    {
        gWs.sendText("RECORD:stop");
    }
    // Последний неполный чанк (sendAudioChunk сам проверит соединение).
    sendAudioChunk();
    gAudioChunk.clear();
    gAudioChunkPackets = 0;
    Serial.println("[vad] speech ended -> RECORD:stop");

    // Блокируем VAD до прихода озвучки ответа: иначе застывший RMS ещё
    // работающего микрофона открывает ложные сегменты, пока сервер
    // распознаёт/синтезирует (повторные RECORD:start/stop без аудио).
    gPlaybackActive = true;
    gPlaybackBlockedAt = millis();
}

void tickVad()
{
    // Только в READY, микрофон реально слушает и не идёт ожидание/
    // воспроизведение ответа (иначе застывший RMS открывает ложные
    // сегменты после RECORD:stop и во время озвучки).
    if (gState != AppState::kReady || !gMic.isRunning() ||
        M5.Speaker.isPlaying() || gPlaybackActive)
    {
        return;
    }
    const float rms = gLatestRms;
    if (!gVadActive)
    {
        if (rms >= gVadThr)
        {
            ++gVadStartCount;
            if (gVadStartCount >= VAD_START_HANGOVER_FRAMES)
            {
                startSegment();
            }
        }
        else
        {
            gVadStartCount = 0;
            // Адаптация фона в тишине.
            gNoiseFloor += (rms - gNoiseFloor) * VAD_NOISE_ADAPT;
            gVadThr = std::max(VAD_MIN_THRESHOLD, gNoiseFloor * 1.6f);
        }
    }
    else
    {
        if (rms >= gVadThr)
        {
            gVadSpeechMs = millis();
        }
        const uint32_t now = millis();
        if ((now - gVadSpeechMs >= VAD_SILENCE_MS) ||
            (now - gVadSegStartMs >= VAD_MAX_SEGMENT_MS))
        {
            stopSegment();
        }
    }
}

// Включает микрофон для прослушивания, если он выключен и нет воспроизведения.
void ensureMicListening()
{
    if (gState != AppState::kReady || gMic.isRunning() ||
        M5.Speaker.isPlaying() || gPlaybackActive)
    {
        return;
    }
    EspMicrophone::Config cfg;
    cfg.sampleRate = MIC_SAMPLE_RATE;
    cfg.channels = MIC_CHANNELS;
    cfg.frameSamples = MIC_FRAME_SAMPLES;
    cfg.noiseGateEnabled = false;  // полный поток; VAD решает сам
    cfg.noiseGateThreshold = MIC_NOISE_THRESHOLD;
    cfg.hangoverFrames = MIC_HANGOVER_FRAMES;

    gAudioChunkMaxBytes = MIC_SAMPLE_RATE * 2u * MIC_AUDIO_CHUNK_SECONDS;

    if (gMic.begin(cfg))
    {
        gMic.setNoiseLevelCallback([](float rms) { gLatestRms = rms; });
        gMic.start(onAudioSamples);
        Serial.println("[mic] listening (VAD)");
    }
}

// Периодический heartbeat (робот -> сервер): маленький текстовый фрейм "HB"
// каждые WS_HEARTBEAT_INTERVAL_MS держит канал живым — роутер/NAT сбрасывает
// TCP-сессию, простаивающую ~2 минуты, и сервер потом не может достучаться.
// Сервер HB только логирует и НЕ обрывает соединение при его отсутствии.
unsigned long gLastHbAt = 0;

void tickHeartbeatInternal()
{
    if (gState != AppState::kReady || !gWs.isConnected())
    {
        gLastHbAt = 0;  // сброс: в следующем READY отправим сразу
        return;
    }
    const unsigned long now = millis();
    if (gLastHbAt == 0 || now - gLastHbAt >= WS_HEARTBEAT_INTERVAL_MS)
    {
        gLastHbAt = now;
        gWs.sendText(protocol::heartbeat());
    }
}

void tickAudioInternal()
{
    if (gState != AppState::kReady)
    {
        return;
    }
    // Сервер не прислал озвучку в отведённое время (пустой STT/ошибка):
    // снимаем блокировку VAD и возвращаем микрофон к прослушиванию.
    if (gPlaybackActive && gPlaybackBlockedAt != 0 &&
        millis() - gPlaybackBlockedAt >= PLAYBACK_IDLE_TIMEOUT_MS)
    {
        resetPlaybackState();
        Serial.println("[audio] answer wait timeout, mic back to VAD");
    }
    // Во время движения сервоприводы гудят — микрофон слышит их как речь:
    // останавливаем запись и держим микрофон выключенным, пока голова
    // двигается; слушать начнём в следующем тике после снятия флага.
    if (gMoveInProgress.load(std::memory_order_relaxed))
    {
        if (gVadActive)
        {
            stopSegment();  // корректно закрыть активную запись
        }
        if (gMic.isRunning())
        {
            gMic.stop();
        }
        return;
    }
    ensureMicListening();
    tickVad();
}

void onWsConnected()
{
    Serial.println("[ws] connected to server");
    if (gState == AppState::kConnecting)
    {
        transition(AppState::kReady);
    }
    // Короткий сигнал успешного подключения.
    gSound.playTone(880, 120);
}

void onWsDisconnected(uint16_t code, const std::string& reason)
{
    Serial.printf("[ws] disconnected code=%u reason=\"%s\"\n", code, reason.c_str());
    // Обрыв соединения: немедленно завершаем VAD-сегмент и останавливаем
    // микрофон. Иначе задача захвата продолжит копить чанки и отправлять их
    // в разорванный WebSocket -> краш (LoadProhibited).
    gVadActive = false;
    gAudioCapturing = false;
    gAudioChunk.clear();
    gAudioChunkPackets = 0;
    if (gMic.isRunning())
    {
        gMic.stop();
    }
    if (gState == AppState::kReady || gState == AppState::kConnecting)
    {
        transition(AppState::kWifiLost);
    }
}

// ---------------------------------------------------------------------------
// Движения головы выполняются в отдельной задаче. EspMovement::apply()
// ждёт завершения поворота синхронно (waitMotion, до 1.5 с): если звать его
// прямо из обработчика WebSocket, на время движения замирает приём PCM-чанков
// (m_ws.loop() не вызывается) и звук заикается — например, танец во время
// озвучки.
//
// Мьютексы/синхронизация: motionTask трогает ТОЛЬКО свои ресурсы — очередь
// gMoveQ и серво (gMove). WebSocket отправка (ACK) уже под мьютексом внутри
// gWs.sendText. Микрофон/VAD задача НЕ трогает: она лишь ставит флаг
// gMoveInProgress, а остановку/возврат микрофона делает loopTask
// (tickAudioInternal) — единственный владелец состояния микрофона, поэтому
// гонок между задачами нет.
// ---------------------------------------------------------------------------
struct MoveCmd
{
    protocol::Command cmd;
};

QueueHandle_t gMoveQ = nullptr;

void motionTask(void* /*arg*/)
{
    while (true)
    {
        MoveCmd* m = nullptr;
        if (xQueueReceive(gMoveQ, &m, portMAX_DELAY) != pdTRUE)
        {
            continue;
        }
        const protocol::Command& cmd = m->cmd;
        // Голова двигается: loopTask сам остановит запись/микрофон на время
        // движения (см. tickAudioInternal) и вернёт его после снятия флага.
        gMoveInProgress.store(true, std::memory_order_relaxed);
        if (cmd.moveAxis == "center")
        {
            gMove.center();
        }
        else if (cmd.moveAxis == "left")
        {
            gMove.turnLeft(cmd.moveDegrees);
        }
        else if (cmd.moveAxis == "right")
        {
            gMove.turnRight(cmd.moveDegrees);
        }
        else if (cmd.moveAxis == "up")
        {
            gMove.turnUp(cmd.moveDegrees);
        }
        else if (cmd.moveAxis == "down")
        {
            gMove.turnDown(cmd.moveDegrees);
        }
        // Флаг держим до конца всей серии движений (пока в очереди что-то
        // есть): микрофон не дёргается между шагами танца.
        if (uxQueueMessagesWaiting(gMoveQ) == 0)
        {
            gMoveInProgress.store(false, std::memory_order_relaxed);
        }
        sendResponse(cmd, true);
        delete m;
    }
}

// Ставит команду движения в очередь motionTask (если очередь полна — отбрасывает).
void enqueueMove(const protocol::Command& cmd)
{
    if (gMoveQ == nullptr)
    {
        gMoveQ = xQueueCreate(8, sizeof(MoveCmd*));
        if (gMoveQ != nullptr)
        {
            // Приоритет 1 на ядре 0: ниже playbackTask (2), чтобы движение
            // никогда не отбирало CPU у воспроизведения звука.
            xTaskCreatePinnedToCore(motionTask, "motion", 8192, nullptr,
                                    1, nullptr, 0);
        }
    }
    if (gMoveQ == nullptr)
    {
        return;
    }
    auto* m = new MoveCmd{cmd};
    if (xQueueSend(gMoveQ, &m, 0) != pdTRUE)
    {
        delete m;  // очередь занята — пропускаем (не накапливаем)
    }
}

// Выполняет команду протокола и отправляет подтверждение/ошибку.
void handleCommand(const protocol::Command& cmd)
{
    switch (cmd.type)
    {
        case protocol::CommandType::Ping:
        {
            gWs.sendText(protocol::pong(static_cast<uint32_t>(millis())));
            break;
        }
        case protocol::CommandType::Emotion:
        {
            if (cmd.valid)
            {
                gScreen.setEmotion(parseEmotion(cmd.emotion));
                sendResponse(cmd, true);
            }
            else
            {
                sendResponse(cmd, false, "unknown emotion");
            }
            break;
        }
        case protocol::CommandType::Move:
        {
            if (!cmd.valid)
            {
                sendResponse(cmd, false, "invalid move");
                break;
            }
            // Движение выполняется асинхронно (motionTask): синхронный
            // поворот блокировал бы приём PCM-чанков, и звук заикался бы
            // (особенно заметно при танце во время озвучки).
            enqueueMove(cmd);
            break;
        }
        case protocol::CommandType::Led:
        {
        /*
            if (cmd.valid)
            {
                gLeds.setColor(cmd.ledR, cmd.ledG, cmd.ledB);
                sendResponse(cmd, true);
            }
            else
            {
                sendResponse(cmd, false, "invalid color");
            }
            break;
        */
            Serial.println("[led] not implemented");
        }
        case protocol::CommandType::Unknown:
        default:
        {
            sendResponse(cmd, false, "unknown command");
            break;
        }
    }
}

void onWsMessage(const uint8_t* data, size_t size, bool binary)
{
    if (binary)
    {
        // Входящий аудиопоток для воспроизведения — всегда PCM:
        // [0]=kAudioFrameType, [1]=kAudioCodecPcm,
        // [2..]=сырые сэмплы int16 LE (16 кГц/моно).
        if (size >= 2 && data[0] == kAudioFrameType)
        {
            // Динамик и микрофон делят I2S на CoreS3: перед воспроизведением
            // останавливаем прослушивание микрофона, если оно идёт.
            if (gMic.isRunning())
            {
                gMic.stop();
                Serial.println("[audio] mic stopped (playback)");
            }
            // Динамик включается позже, в playbackTask, через gSound.ensureReady()
            // (после M5.Mic.end() порту I2S нужно время на освобождение).

            if (data[1] == kAudioCodecPcm)
            {
                // Воспроизведение в отдельной задаче с собственным стеком.
                enqueuePlayback(data + 2, size - 2);
            }
            else
            {
                LOG_W("[audio] unknown codec %u\n", data[1]);
            }
            return;
        }
        LOG_D("[ws] binary frame %u bytes\n", static_cast<unsigned>(size));
        return;
    }

    // Робот слушает WebSocket и выполняет команды протокола.
    const protocol::Command cmd =
        protocol::parse(reinterpret_cast<const char*>(data), size);
    LOG_D("[ws] text << %.*s\n", static_cast<int>(size),
          reinterpret_cast<const char*>(data));
    handleCommand(cmd);
}

}  // namespace

// Глобальная точка входа (вызывается из loop()): делегирует в namespace.
void tickAudio()
{
    tickAudioInternal();
    tickHeartbeatInternal();
}

void setupCommands()
{
    gWs.setConnectedCallback(onWsConnected);
    gWs.setDisconnectedCallback(onWsDisconnected);
    gWs.setMessageCallback(onWsMessage);
}