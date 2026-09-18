// State machine прошивки робота: Wi-Fi подключение, WebSocket,
// восстановление связи.

#include "state_machine.h"

#include <Arduino.h>
#include <WiFi.h>

#include "app.h"
#include "config/config.h"
#include "protocol/protocol.h"

AppState gState = AppState::kPowerUp;
unsigned long gLastAttempt = 0;    // время последней попытки (wifi/ws)

namespace {

const char* stateName(AppState s)
{
    switch (s)
    {
        case AppState::kPowerUp: return "POWER_UP";
        case AppState::kWifiConnecting: return "WIFI_CONNECTING";
        case AppState::kWifiReady: return "WIFI_READY";
        case AppState::kConnecting: return "CONNECTING";
        case AppState::kReady: return "READY";
        case AppState::kWifiLost: return "WIFI_LOST";
    }
    return "UNKNOWN";
}

void logConnection()
{
    Serial.printf("[wifi] connected: SSID=%s IP=%s RSSI=%d dBm\n",
                  WiFi.SSID().c_str(),
                  WiFi.localIP().toString().c_str(),
                  WiFi.RSSI());
}

}  // namespace

void transition(AppState next)
{
    if (next == gState)
        return;
    Serial.printf("[state] %s -> %s\n", stateName(gState), stateName(next));
    gState = next;
    gLastAttempt = millis();
}

bool ensureWifiConnected()
{
    if (gWifi.isConnected())
        return true;
    Serial.printf("[wifi] connecting to \"%s\" (timeout %u ms) ...\n",
                  WIFI_SSID, static_cast<unsigned>(WIFI_CONNECT_TIMEOUT_MS));
    return gWifi.connect(WIFI_SSID, WIFI_PASS, WIFI_CONNECT_TIMEOUT_MS);
}

void tickStateMachine()
{
    switch (gState)
    {
        case AppState::kPowerUp:
            // Периферия инициализирована в setup(); идём подключать Wi-Fi.
            transition(AppState::kWifiConnecting);
            break;

        case AppState::kWifiConnecting:
            if (ensureWifiConnected())
            {
                logConnection();
                transition(AppState::kWifiReady);
            }
            else if (millis() - gLastAttempt >= WIFI_RECONNECT_DELAY_MS)
            {
                Serial.println("[wifi] connection failed, retrying ...");
                gLastAttempt = millis();
            }
            break;

        case AppState::kWifiReady:
            Serial.printf("[ws] connecting to ws://%s:%u%s ...\n",
                          WS_HOST, static_cast<unsigned>(WS_PORT), WS_PATH);
            gWs.connect(WS_HOST, WS_PORT, WS_PATH);
            transition(AppState::kConnecting);
            break;

        case AppState::kConnecting:
            gWs.loop();
            if (!gWifi.isConnected())
            {
                gWs.disconnect();
                transition(AppState::kWifiLost);
            }
            break;

        case AppState::kReady:
            gWs.loop();
            if (!gWifi.isConnected())
            {
                Serial.println("[state] wifi lost while in READY");
                gWs.disconnect();
                transition(AppState::kWifiLost);
                break;
            }
            break;

        case AppState::kWifiLost:
            if (ensureWifiConnected())
            {
                logConnection();
                transition(AppState::kConnecting);
            }
            else if (millis() - gLastAttempt >= WIFI_RECONNECT_DELAY_MS)
            {
                Serial.println("[wifi] retry after link lost ...");
                gLastAttempt = millis();
            }
            break;
    }
}