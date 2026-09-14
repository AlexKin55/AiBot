#ifndef WEBSOCKET_H_
#define WEBSOCKET_H_

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>

#include <WebSocketsClient.h>

// Клиент WebSocket поверх Wi-Fi для ESP32 (библиотека links2004/WebSockets).
class EspWebsocketClient
{
    public:
    using ConnectedCallback = std::function<void()>;
    using DisconnectedCallback = std::function<void(uint16_t code, const std::string& reason)>;
    using MessageCallback = std::function<void(const uint8_t* data, size_t size, bool binary)>;

    EspWebsocketClient();
    ~EspWebsocketClient();

    // Начинает подключение к ws://host:port/path. Соединение устанавливается
    // асинхронно — событие подключения придёт в loop(), вызывать её надо из
    // основного цикла (например, в тесте) до появления CONNECTED.
    bool connect(const char* host, uint16_t port, const char* path = "/");

    // Активно ли соединение в данный момент.
    bool isConnected();

    // Разрывает соединение.
    void disconnect();

    // Отправка текстового и бинарного сообщения.
    bool sendText(const std::string& text);
    bool sendBinary(const uint8_t* data, size_t size);

    // Обслуживает сеть WebSocket (приём событий/данных). Вызывать регулярно.
    void loop();

    // Регистрация обработчиков событий.
    void setConnectedCallback(ConnectedCallback cb);
    void setDisconnectedCallback(DisconnectedCallback cb);
    void setMessageCallback(MessageCallback cb);

    private:
    void handleEvent(WStype_t type, uint8_t* payload, size_t length);

    WebSocketsClient m_ws;
    ConnectedCallback m_onConnected;
    DisconnectedCallback m_onDisconnected;
    MessageCallback m_onMessage;
};

#endif  // WEBSOCKET_H_