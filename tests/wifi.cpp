// Тесты Wi-Fi на реальном железе (M5Stack CoreS3 / ESP32).
//
// Сценарии выполняются при запуске прошивки, результат выводится в Serial.
// Подсчёт результатов — общий через tests/test_util.h. Точка входа (setup/loop)
// и итоговый отчёт — в tests/tests.cpp.

#include <Arduino.h>
#include <WiFi.h>

#include "config/test_config.h"
#include "test_util.h"
#include "wifi/wifi.h"

namespace {

void printLinkInfo()
{
    Serial.printf("[wifi] SSID=%s IP=%s RSSI=%d dBm\n",
                  WiFi.SSID().c_str(),
                  WiFi.localIP().toString().c_str(),
                  WiFi.RSSI());
}

}  // namespace

// Подключение с корректными учётными данными должно завершиться успешно.
void testConnectCorrect()
{
    EspWifiManager wifi;
    const bool ok = wifi.connect(WIFI_SSID, WIFI_PASS, WIFI_CONNECT_TIMEOUT_MS);
    report(ok, "connect with correct credentials");
    if (ok)
        printLinkInfo();
}

// После успешного connect() статус isConnected() должен быть true.
void testIsConnectedAfterConnect()
{
    EspWifiManager wifi;
    report(wifi.isConnected(), "isConnected after successful connect");
}

// Повторный connect() при уже установленной связи возвращает true мгновенно
// (не должен рвать соединение и висеть до таймаута).
void testConnectIdempotent()
{
    EspWifiManager wifi;
    // Короткий таймаут: если логика некорректна и попытается переподключаться,
    // тест упадёт по времени — это ожидаемо для FAIL.
    const bool ok = wifi.connect(WIFI_SSID, WIFI_PASS, 500u);
    report(ok, "connect while already connected (idempotent)");
    report(wifi.isConnected(), "still connected after redundant connect");
}

// Подключение с заведомо неверными данными должно вернуть false.
void testConnectWrongCreds()
{
    // Отключаемся от текущей сети: пока мы подключены, connect() возвращает
    // true по идемпотентному раннему выходу, и негативный сценарий не проверяется.
    WiFi.disconnect();
    delay(300);

    EspWifiManager wifi;
    const bool ok = wifi.connect("WRONG_SSID_NONEXIST", "wrong-pass",
                                 WIFI_TEST_WRONG_TIMEOUT_MS);
    report(!ok, "connect with wrong credentials returns false");
}

void runWifiTests()
{
    Serial.println("[TEST] === WiFi tests on real hardware ===");

    // Не даём Wi-Fi-стеку уходить в deep sleep во время теста.
    WiFi.setSleep(false);

    testConnectCorrect();
    testIsConnectedAfterConnect();
    testConnectIdempotent();
    testConnectWrongCreds();
}