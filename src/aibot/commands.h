#ifndef AIBOT_COMMANDS_H_
#define AIBOT_COMMANDS_H_

// Регистрирует WebSocket-колбэки (подключение/отключение/сообщения).
// Вызывается один раз из setup().
void setupCommands();

// Тик аудио-логики (VAD): прослушивание микрофона в READY, автозапись по шуму
// и отправка RECORD:start/RECORD:stop. Вызывать из loop() каждый цикл.
void tickAudio();

#endif  // AIBOT_COMMANDS_H_