#include "protocol/protocol.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <vector>

namespace protocol {

namespace {

std::string toUpper(std::string s)
{
    for (char& c : s)
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return s;
}

std::string toLower(std::string s)
{
    for (char& c : s)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

std::vector<std::string> split(const std::string& s, char sep)
{
    std::vector<std::string> out;
    size_t start = 0;
    while (true)
    {
        const size_t pos = s.find(sep, start);
        if (pos == std::string::npos)
        {
            out.push_back(s.substr(start));
            break;
        }
        out.push_back(s.substr(start, pos - start));
        start = pos + 1;
    }
    return out;
}

bool isEmotionName(const std::string& name)
{
    return name == "neutral" || name == "happy" || name == "angry" ||
           name == "sad" || name == "doubt" || name == "sleepy";
}

bool isMoveAxis(const std::string& axis)
{
    return axis == "left" || axis == "right" || axis == "up" ||
           axis == "down" || axis == "center";
}

}  // namespace

Command parse(const char* text, size_t length)
{
    Command cmd;
    std::string s(text, length);

    // Обрезаем хвостовые \r\n.
    while (!s.empty() && (s.back() == '\r' || s.back() == '\n'))
        s.pop_back();
    if (s.empty())
        return cmd;

    const std::vector<std::string> parts = split(s, ':');
    const std::string key = toUpper(parts[0]);

    if (key == "PING")
    {
        cmd.type = CommandType::Ping;
        cmd.valid = true;
        return cmd;
    }
    if (key == "EMOTION")
    {
        cmd.type = CommandType::Emotion;
        if (parts.size() >= 2 && isEmotionName(toLower(parts[1])))
        {
            cmd.emotion = toLower(parts[1]);
            cmd.valid = true;
        }
        return cmd;
    }
    if (key == "MOVE")
    {
        cmd.type = CommandType::Move;
        if (parts.size() >= 2)
        {
            cmd.moveAxis = toLower(parts[1]);
            if (cmd.moveAxis == "center")
            {
                cmd.valid = true;
            }
            else if (isMoveAxis(cmd.moveAxis) && parts.size() >= 3)
            {
                cmd.moveDegrees = std::atoi(parts[2].c_str());
                cmd.valid = true;
            }
        }
        return cmd;
    }
    if (key == "LED")
    {
        cmd.type = CommandType::Led;
        if (parts.size() >= 2)
        {
            int r = -1, g = -1, b = -1;
            if (std::sscanf(parts[1].c_str(), "%d,%d,%d", &r, &g, &b) == 3 &&
                r >= 0 && r <= 255 && g >= 0 && g <= 255 && b >= 0 && b <= 255)
            {
                cmd.ledR = static_cast<uint8_t>(r);
                cmd.ledG = static_cast<uint8_t>(g);
                cmd.ledB = static_cast<uint8_t>(b);
                cmd.valid = true;
            }
        }
        return cmd;
    }
    if (key == "AUDIO")
    {
        cmd.type = CommandType::Audio;
        if (parts.size() >= 2)
        {
            const std::string mode = toLower(parts[1]);
            if (mode == "start")
            {
                cmd.audioStart = true;
                cmd.valid = true;
            }
            else if (mode == "stop")
            {
                cmd.audioStart = false;
                cmd.valid = true;
            }
        }
        return cmd;
    }

    // Неизвестная команда.
    return cmd;
}

const char* typeName(CommandType type)
{
    switch (type)
    {
        case CommandType::Ping: return "PING";
        case CommandType::Emotion: return "EMOTION";
        case CommandType::Move: return "MOVE";
        case CommandType::Led: return "LED";
        case CommandType::Audio: return "AUDIO";
        case CommandType::Unknown:
        default: return "COMMAND";
    }
}

std::string response(const Command& cmd, bool ok, const char* reason)
{
    if (!ok)
    {
        const char* r = reason ? reason : "invalid";
        return std::string("ERR:") + typeName(cmd.type) + ":" + r;
    }

    switch (cmd.type)
    {
        case CommandType::Emotion:
            return "ACK:EMOTION:" + cmd.emotion;
        case CommandType::Move:
            if (cmd.moveAxis == "center")
                return "ACK:MOVE:center";
            return "ACK:MOVE:" + cmd.moveAxis + ":" +
                   std::to_string(cmd.moveDegrees);
        case CommandType::Led:
            return "ACK:LED:" + std::to_string(cmd.ledR) + "," +
                   std::to_string(cmd.ledG) + "," + std::to_string(cmd.ledB);
        case CommandType::Audio:
            return cmd.audioStart ? "ACK:AUDIO:start" : "ACK:AUDIO:stop";
        case CommandType::Ping:
        default:
            return "ACK:" + std::string(typeName(cmd.type));
    }
}

std::string pong(uint32_t uptimeMs)
{
    return "PONG:" + std::to_string(uptimeMs);
}

std::string heartbeat()
{
    return "HB";
}

}  // namespace protocol