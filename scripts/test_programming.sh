#!/usr/bin/env bash
# Прошивка ТЕСТОВ в робота: сборка тестовой прошивки + загрузка в устройство.
#
# Порт задаётся в platformio.ini (upload_port). Можно переопределить порт
# переменной PORT, например: PORT=/dev/ttyUSB0 ./scripts/test_programming.sh
# Окружение — переменной ENV_NAME (по умолчанию m5stack-cores3).
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
    echo "[test-programming] PlatformIO не найден (переменная PIO, $PIO_DEFAULT, \$PATH)." >&2
    exit 1
fi

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
# Тестовое окружение по умолчанию.
ENV_NAME="${ENV_NAME:-m5stack-cores3-tests}"

cd "$ROOT_DIR"

ARGS=(
    "run"
    "--environment" "$ENV_NAME"
    "--target" "upload"
)

if [[ -n "${PORT:-}" ]]; then
    ARGS+=("--upload-port" "$PORT")
fi

echo "[test-programming] Порт для загрузки: ${PORT:-<из platformio.ini>}"
echo "[test-programming] Запускаю сборку и прошивку ТЕСТОВ ($ENV_NAME) ..."
exec "$PIO" "${ARGS[@]}"