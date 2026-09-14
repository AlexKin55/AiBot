// Тест движения (пан-тилт сервоприводы головы) на реальном железе.
//
// Сценарии: поворот влево/вправо, абсолютные положения +180/-180,
// наклон вверх/вниз. Корректность проверяется по внутреннему состоянию
// модуля (panDeg()/tiltDeg()), а сами команды выполняются реальными
// приводами через M5StackChan.

#include <Arduino.h>

#include "app_config.h"
#include "move/move.h"
#include "test_util.h"

namespace {

EspMovement gMove;

void reportAngle(bool ok, const char* name, int value)
{
    Serial.printf("[move] %s: %d deg\n", name, value);
    report(ok, name);
}

}  // namespace

void testMoveInit()
{
    EspMovement::Config cfg;
    cfg.minAngle = MOVEMENT_MIN_ANGLE;
    cfg.maxAngle = MOVEMENT_MAX_ANGLE;

    report(gMove.begin(), "movement begin()");
    gMove.center();
}

// Поворот влево увеличивает горизонтальный угол (pan > 0).
void testTurnLeft()
{
    gMove.setPan(0);
    gMove.turnLeft(45);
    reportAngle(gMove.panDeg() > 0, "turn left (pan increases)", gMove.panDeg());
}

// Поворот вправо уменьшает горизонтальный угол (pan < 0).
void testTurnRight()
{
    gMove.setPan(0);
    gMove.turnRight(45);
    reportAngle(gMove.panDeg() < 0, "turn right (pan decreases)", gMove.panDeg());
}

// Абсолютный поворот на +180.
void testTurnPositive180()
{
    gMove.setPan(0);
    gMove.setPan(180);
    reportAngle(gMove.panDeg() == 180, "set pan +180", gMove.panDeg());
}

// Абсолютный поворот на -180.
void testTurnNegative180()
{
    gMove.setPan(0);
    gMove.setPan(-180);
    reportAngle(gMove.panDeg() == -180, "set pan -180", gMove.panDeg());
}

// Наклон вверх увеличивает вертикальный угол (tilt > 0).
void testTurnUp()
{
    gMove.setTilt(0);
    gMove.turnUp(30);
    reportAngle(gMove.tiltDeg() > 0, "turn up (tilt increases)", gMove.tiltDeg());
}

// Наклон вниз уменьшает вертикальный угол (tilt < 0).
void testTurnDown()
{
    gMove.setTilt(0);
    gMove.turnDown(30);
    reportAngle(gMove.tiltDeg() < 0, "turn down (tilt decreases)", gMove.tiltDeg());
}

void runMoveTests()
{
    Serial.println("[TEST] === Movement tests on real hardware ===");

    testMoveInit();
    testTurnLeft();
    testTurnRight();
    testTurnPositive180();
    testTurnNegative180();
    testTurnUp();
    testTurnDown();

    // Возвращаем голову в центр и снимаем нагрузку с приводов.
    gMove.center();
    gMove.stop();
}