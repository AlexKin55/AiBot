#include "websocket/websocket.h"

EspWebsocketClient::EspWebsocketClient()
{
    // Все события библиотеки перенаправляем в handleEvent(), откуда вызываем
    // пользовательские колбэки.
    m_ws.onEvent([this](WStype_t type, uint8_t* payload, size_t length) {
        handleEvent(type, payload, length);
    });
}

EspWebsocketClient::~EspWebsocketClient() = default;

bool EspWebsocketClient::connect(const char* host, uint16_t port, const char* path)
{
    m_ws.begin(host, port, path);
    return true;
}

bool EspWebsocketClient::isConnected()
{
    std::lock_guard<std::recursive_mutex> lock(m_wsMutex);
    return m_ws.isConnected();
}

void EspWebsocketClient::disconnect()
{
    std::lock_guard<std::recursive_mutex> lock(m_wsMutex);
    m_ws.disconnect();
}

bool EspWebsocketClient::sendText(const std::string& text)
{
    // Сериализуем все операции WebSocketsClient из разных задач
    // (I2S-микрофон шлёт PCM, loopTask шлёт текст/ACK и принимает данные):
    // библиотека не потокобезопасна.
    std::lock_guard<std::recursive_mutex> lock(m_wsMutex);
    return m_ws.sendTXT(text.c_str());
}

bool EspWebsocketClient::sendBinary(const uint8_t* data, size_t size)
{
    std::lock_guard<std::recursive_mutex> lock(m_wsMutex);
    return m_ws.sendBIN(data, size);
}

void EspWebsocketClient::loop()
{
    // Приём данных тоже под мьютексом: пока I2S-задача отправляет PCM-чанк,
    // loopTask ждёт завершения отправки, и наоборот. Рекурсивный мьютекс
    // позволяет колбэкам (вызываются внутри m_ws.loop()) слать ACK/ответы
    // без дедлока.
    std::lock_guard<std::recursive_mutex> lock(m_wsMutex);
    m_ws.loop();
}

void EspWebsocketClient::setConnectedCallback(ConnectedCallback cb)
{
    m_onConnected = std::move(cb);
}

void EspWebsocketClient::setDisconnectedCallback(DisconnectedCallback cb)
{
    m_onDisconnected = std::move(cb);
}

void EspWebsocketClient::setMessageCallback(MessageCallback cb)
{
    m_onMessage = std::move(cb);
}

void EspWebsocketClient::handleEvent(WStype_t type, uint8_t* payload, size_t length)
{
    switch (type)
    {
        case WStype_CONNECTED:
            if (m_onConnected) m_onConnected();
            break;

        case WStype_DISCONNECTED:
            if (m_onDisconnected)
            {
                const std::string reason = payload ? reinterpret_cast<const char*>(payload) : "";
                m_onDisconnected(0, reason);
            }
            break;

        case WStype_TEXT:
            if (m_onMessage)
            {
                size_t n = length;
                // Некоторые версии библиотеки включают завершающий NUL в length.
                if (n > 0 && payload[n - 1] == 0) --n;
                m_onMessage(payload, n, false);
            }
            break;

        case WStype_BIN:
            if (m_onMessage) m_onMessage(payload, length, true);
            break;

        default:
            break;
    }
}