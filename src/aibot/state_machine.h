#ifndef AIBOT_STATE_MACHINE_H_
#define AIBOT_STATE_MACHINE_H_

// Состояния прошивки робота:
//   kPowerUp        -> инициализация периферии (в setup), переход к Wi-Fi
//   kWifiConnecting -> подключение к точке доступа (с ретраями)
//   kWifiReady      -> инициализация WebSocket-клиента
//   kConnecting     -> ожидание CONNECTED от сервера
//   kReady          -> обмен данными: приём команд, аудио
//   kWifiLost       -> потеря связи, переподключение Wi-Fi
enum class AppState
{
    kPowerUp,
    kWifiConnecting,
    kWifiReady,
    kConnecting,
    kReady,
    kWifiLost,
};

// Текущее состояние (определено в state_machine.cpp).
extern AppState gState;

// Переход в новое состояние (с логированием).
void transition(AppState next);

// Гарантирует наличие Wi-Fi-соединения (подключает при необходимости).
bool ensureWifiConnected();

// Шаг state machine. Вызывать из loop().
void tickStateMachine();

#endif  // AIBOT_STATE_MACHINE_H_