// Тест WebSocket на реальном железе: connect / send / recv / disconnect.
//
// Сначала гарантированно подключаемся к сети через EspWifiManager, затем
// устанавливаем WebSocket-соединение с ws://WS_HOST:WS_PORT/WS_PATH,
// отправляем текстовое сообщение, ждём ответ сервера (recv) и разрываем связь.
// Результаты выводятся в Serial и учитываются в общем отчёте (tests/test_util.h).

#include <Arduino.h>

#include <string>

#include "app_config.h"
#include "test_util.h"
#include "websocket/websocket.h"
#include "wifi/wifi.h"

namespace {

// Ожидание события с обслуживанием WebSocket и лимитом по времени.
void pumpUntil(unsigned long timeoutMs, EspWebsocketClient& ws,
               const bool* finished)
{
    const unsigned long start = millis();
    while (!*finished && (millis() - start) < timeoutMs)
    {
        ws.loop();
        delay(10);
    }
}

}  // namespace

void testWebsocketConnectSendRecvDisconnect()
{
    Serial.println("[TEST] === WebSocket test on real hardware ===");

    // 1. Подключаемся к сети через EspWifiManager (обязательное условие).
    EspWifiManager wifi;
    if (!wifi.isConnected())
    {
        const bool w = wifi.connect(WIFI_SSID, WIFI_PASS, WIFI_CONNECT_TIMEOUT_MS);
        report(w, "websocket: wifi connected (EspWifiManager)");
        if (!w)
            return;
    }

    // 2. WebSocket-клиент и обработчики событий.
    EspWebsocketClient ws;
    bool gotConnected = false;
    bool gotDisconnected = false;
    bool gotMessage = false;
    std::string received;

    ws.setConnectedCallback([&]() {
        gotConnected = true;
        Serial.println("[ws] CONNECTED");
    });
    ws.setDisconnectedCallback([&](uint16_t code, const std::string& reason) {
        gotDisconnected = true;
        Serial.printf("[ws] DISCONNECTED code=%u reason=\"%s\"\n", code, reason.c_str());
    });
    ws.setMessageCallback([&](const uint8_t* data, size_t size, bool binary) {
        gotMessage = true;
        if (binary)
        {
            Serial.printf("[ws] recv BINARY %u bytes\n", static_cast<unsigned>(size));
        }
        else
        {
            received.assign(reinterpret_cast<const char*>(data), size);
            Serial.printf("[ws] recv TEXT: %s\n", received.c_str());
        }
    });

    // 3. connect (лишь инициирует подключение; факт связи подтверждается
    // событием CONNECTED ниже).
    report(ws.connect(WS_HOST, WS_PORT, WS_PATH), "websocket connect() initiated");

    // 4. Ждём события CONNECTED.
    pumpUntil(WS_TEST_CONNECT_TIMEOUT_MS, ws, &gotConnected);
    report(gotConnected, "websocket CONNECTED event");
    report(ws.isConnected(), "websocket isConnected()");
    if (!gotConnected)
    {
        Serial.printf("[ws] hint: не получен CONNECTED за %u мс — проверьте,\n"
                      "      что WS-эхо-сервер запущен на ws://%s:%u%s\n",
                      static_cast<unsigned>(WS_TEST_CONNECT_TIMEOUT_MS),
                      WS_HOST, static_cast<unsigned>(WS_PORT), WS_PATH);
    }

    if (gotConnected)
    {
        // 5. send.
        report(ws.sendText("TEST:hello"), "websocket sendText()");

        // 6. recv — ждём ответа сервера.
        pumpUntil(WS_TEST_RECV_TIMEOUT_MS, ws, &gotMessage);
        report(gotMessage, "websocket recv message");

        // Проверяем, что полученный текст совпадает с ожидаемым echo.
        if (gotMessage && !received.empty())
        {
            report(received == "TEST:hello", "websocket recv echo matches sent text");
        }
    }

    // 7. disconnect.
    ws.disconnect();
    pumpUntil(3000u, ws, &gotDisconnected);
    report(gotDisconnected, "websocket DISCONNECTED event");
    report(!ws.isConnected(), "websocket isConnected()==false after disconnect");
}

void runWebsocketTests()
{
    testWebsocketConnectSendRecvDisconnect();
}