// Простейший уровневый логгер прошивки (вывод в Serial).
//
// Уровень задаётся в config/config.h (LOG_LEVEL) и может быть переопределён
// флагом сборки -DLOG_LEVEL=N (например, в platformio.ini build_flags):
//   0 = OFF      — логов нет;
//   1 = ERROR    — только ошибки;
//   2 = WARN     — ошибки и предупреждения;
//   3 = INFO     — обычная работа (по умолчанию);
//   4 = DEBUG    — детальные/частые сообщения (чанки, кадры, фреймы WS).
//
// Формат вызова — как Serial.printf: LOG_I("[audio] готово: %d\n", n);
// (перевод строки включается в саму строку, как в существующем коде).

#pragma once

#include <Arduino.h>

#include "config/config.h"

#define LOG_LEVEL_OFF 0
#define LOG_LEVEL_ERROR 1
#define LOG_LEVEL_WARN 2
#define LOG_LEVEL_INFO 3
#define LOG_LEVEL_DEBUG 4

#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_INFO
#endif

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
#define LOG_D(fmt, ...) Serial.printf((fmt), ##__VA_ARGS__)
#else
#define LOG_D(fmt, ...) ((void)0)
#endif

#if LOG_LEVEL >= LOG_LEVEL_INFO
#define LOG_I(fmt, ...) Serial.printf((fmt), ##__VA_ARGS__)
#else
#define LOG_I(fmt, ...) ((void)0)
#endif

#if LOG_LEVEL >= LOG_LEVEL_WARN
#define LOG_W(fmt, ...) Serial.printf((fmt), ##__VA_ARGS__)
#else
#define LOG_W(fmt, ...) ((void)0)
#endif

#if LOG_LEVEL >= LOG_LEVEL_ERROR
#define LOG_E(fmt, ...) Serial.printf((fmt), ##__VA_ARGS__)
#else
#define LOG_E(fmt, ...) ((void)0)
#endif