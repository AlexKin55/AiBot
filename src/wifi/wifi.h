#ifndef WIFI_H_
#define WIFI_H_

#include <cstdint>

// Управление Wi-Fi для ESP32 на базе Arduino <WiFi.h>.
class EspWifiManager
{
    public:
    // Подключение к точке доступа. Блокируется до установления связи либо
    // до истечения timeoutMs. Возвращает true при успешном подключении.
    bool connect(const char* ssid, const char* pass, uint32_t timeoutMs);

    // Текущее состояние соединения.
    bool isConnected() const;
};

#endif  // WIFI_H_