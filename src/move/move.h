#ifndef MOVE_H_
#define MOVE_H_

#include <cstdint>

// Управление поворотом головы робота (пан-тилт сервоприводы) для ESP32.
// Горизонталь (yaw): влево/вправо. Вертикаль (pitch): вверх/вниз.
// Углы — целые (градусы); отслеживаются во внутреннем состоянии (panDeg()/
// tiltDeg()) и отправляются на приводы через M5StackChan. Целые значения
// выбраны, т.к. ESP32-S3 не имеет аппаратного FPU, а точность float тут
// избыточна — движение квантуется коэффициентом kDegToMotion.
class EspMovement
{
    public:
    struct Config
    {
        // Пределы углов, градусы.
        int minAngle = -180;
        int maxAngle = 180;
    };

    bool begin();
    // Возвращает голову в центральное положение (pan=0, tilt=0).
    void center();

    // Относительные повороты относительно текущего положения.
    void turnLeft(int degrees);   // pan += degrees
    void turnRight(int degrees);  // pan -= degrees
    void turnUp(int degrees);     // tilt += degrees
    void turnDown(int degrees);   // tilt -= degrees

    // Абсолютное позиционирование.
    void setPan(int degrees);
    void setTilt(int degrees);

    // Текущая позиция (внутреннее состояние), градусы.
    int panDeg() const;
    int tiltDeg() const;

    // Останавливает движение / возвращает в центр.
    void stop();

    private:
    void apply();  // отправляет текущие углы на приводы

    int panDeg_ = 0;
    int tiltDeg_ = 0;
    Config config_;
    bool initialized_ = false;
};

#endif  // MOVE_H_