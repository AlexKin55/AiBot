#!/usr/bin/env bash
# Подключение к UART для просмотра лога робота через PlatformIO Serial Monitor.
#
# Порт и скорость берутся из platformio.ini (monitor_port, monitor_speed).
# Переопределить можно переменными: PORT=/dev/ttyUSB0 BAUDRATE=115200
# Настройки DTR/RTS тоже читаются из platformio.ini (monitor_dtr, monitor_rts).
set -euo pipefail

# Ищем PlatformIO: сначала переменная PIO, затем стандартный penv-путь,
# затем бинарник в $PATH (например, установка через pip --user).
PIO_DEFAULT="$HOME/.platformio/penv/bin/pio"
PIO="${PIO:-}"
if [[ -z "$PIO" && -x "$PIO_DEFAULT" ]]; then
    PIO="$PIO_DEFAULT"
fi
if [[ -z "$PIO" ]]; then
    PIO="$(command -v pio 2>/dev/null || true)"
fi

if [[ -z "$PIO" || ! -x "$PIO" ]]; then
    echo "[monitor] PlatformIO не найден (переменная PIO, $PIO_DEFAULT, \$PATH)." >&2
    exit 1
fi

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
ENV_NAME="${ENV_NAME:-m5stack-cores3}"

cd "$ROOT_DIR"

ARGS=(
    "device" "monitor"
    "--environment" "$ENV_NAME"
)

if [[ -n "${PORT:-}" ]]; then
    ARGS+=("--port" "$PORT")
fi
if [[ -n "${BAUDRATE:-}" ]]; then
    ARGS+=("--baud" "$BAUDRATE")
fi

echo "[monitor] Порт: ${PORT:-<из platformio.ini>}, скорость: ${BAUDRATE:-<из platformio.ini>}"
echo "[monitor] Подключаюсь к UART, для выхода нажмите Ctrl+C ..."
exec "$PIO" "${ARGS[@]}"