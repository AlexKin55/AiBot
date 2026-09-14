#ifndef SCREEN_H_
#define SCREEN_H_

#include <cstddef>
#include <cstdint>

#include <Avatar.h>

// Эмоции (мимика) лица робота. Порядок значений совпадает с Expression
// библиотеки M5Stack-Avatar.
enum class Emotion
{
    Neutral,  // спокойное выражение
    Happy,    // радость
    Angry,    // злость
    Sad,      // грусть
    Doubt,    // сомнение
    Sleepy,   // сонливость
};

// Вывод эмоций на дисплей робота через анимированный аватар M5Stack-Avatar.
class EspScreen
{
    public:
    bool begin();
    // Устанавливает текущую эмоцию на лице аватара.
    void setEmotion(Emotion emotion);
    // Возвращает текущую эмоцию.
    Emotion getEmotion() const;
    // Показывает речевой пузырь с текстом поверх аватара.
    void setSpeechText(const char* text);
    // Убирает текст речевого пузыря.
    void clearSpeechText();

    // Выводит кадр RGB565 (например, превью камеры) на весь экран.
    void showFrame(uint16_t width, uint16_t height, const void* rgb565);

    private:
    m5avatar::Avatar avatar_;
    Emotion emotion_ = Emotion::Neutral;
    bool initialized_ = false;
};

#endif  // SCREEN_H_