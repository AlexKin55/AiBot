#ifndef PROTOCOL_H_
#define PROTOCOL_H_

#include <cstddef>
#include <cstdint>
#include <string>

// Протокол взаимодействия с роботом по WebSocket (текстовые фреймы).
//
// Формат команд (сервер -> робот):
//   PING
//   STATUS
//   EMOTION:<name>              name = neutral|happy|angry|sad|doubt|sleepy
//   MOVE:left|right|up|down:<deg>
//   MOVE:center
//   LED:<r>,<g>,<b>             r,g,b = 0..255
//   AUDIO:start | AUDIO:stop    вкл/выкл трансляцию звука с микрофона
//
// Ответы (робот -> сервер):
//   ACK:<COMMAND>[:args]        команда выполнена
//   ERR:<COMMAND>:<reason>      ошибка
//   PONG:<uptime_ms>            ответ на PING
//   STATUS:online               heartbeat/статус
namespace protocol {

enum class CommandType
{
    Unknown,
    Ping,
    Status,
    Emotion,
    Move,
    Led,
    Audio,
};

// Разобранная команда.
struct Command
{
    CommandType type = CommandType::Unknown;
    bool valid = false;

    // EMOTION
    std::string emotion;

    // MOVE
    std::string moveAxis;  // left|right|up|down|center
    int moveDegrees = 0;

    // LED
    uint8_t ledR = 0;
    uint8_t ledG = 0;
    uint8_t ledB = 0;

    // AUDIO
    bool audioStart = false;  // true = AUDIO:start, false = AUDIO:stop
};

// Разбирает входящий текстовый фрейм. Ключевые слова регистронезависимы.
Command parse(const char* text, size_t length);

// Текстовое имя типа команды (для ACK/ERR).
const char* typeName(CommandType type);

// Формирует подтверждение или ошибку для команды:
//   response(cmd, true)            -> "ACK:..."
//   response(cmd, false, reason)   -> "ERR:<TYPE>:<reason>"
std::string response(const Command& cmd, bool ok, const char* reason = nullptr);

// Ответ на PING: "PONG:<uptime_ms>".
std::string pong(uint32_t uptimeMs);

// Heartbeat/статус: "STATUS:online".
std::string statusOnline();

}  // namespace protocol

#endif  // PROTOCOL_H_