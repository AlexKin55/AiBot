#!/usr/bin/env bash
# Сборка прошивки PlatformIO.
# Путь к PlatformIO указан явно, т.к. penv может не быть в $PATH.
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
    echo "[build] PlatformIO не найден (переменная PIO, $PIO_DEFAULT, \$PATH)." >&2
    exit 1
fi

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
# Рабочее окружение по умолчанию — прошивка робота.
ENV_NAME="${ENV_NAME:-m5stack-cores3}"

cd "$ROOT_DIR"
echo "[build] Собираю прошивку робота ($ENV_NAME) ..."
exec "$PIO" run --environment "$ENV_NAME" "$@"