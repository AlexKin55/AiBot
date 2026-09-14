#!/usr/bin/env bash
# Сборка ТОЛЬКО тестовой прошивки (без прошивки в устройство).
#
# Собирает окружение тестового проекта (по умолчанию m5stack-cores3).
# Переопределить окружение можно переменной ENV_NAME.
# Быстрая проверка компиляции тестов: ./scripts/test_build.sh
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
    echo "[test-build] PlatformIO не найден (переменная PIO, $PIO_DEFAULT, \$PATH)." >&2
    exit 1
fi

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
# Тестовое окружение по умолчанию.
ENV_NAME="${ENV_NAME:-m5stack-cores3-tests}"

cd "$ROOT_DIR"
echo "[test-build] Собираю тестовую прошивку ($ENV_NAME) без загрузки ..."
exec "$PIO" run --environment "$ENV_NAME"