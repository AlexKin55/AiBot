#!/usr/bin/env bash
# Прошивка робота: сборка + загрузка прошивки в устройство.
#
# Порт задаётся в platformio.ini (upload_port). Можно переопределить порт
# переменной PORT, например: PORT=/dev/ttyUSB0 ./scripts/programming.sh
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
    echo "[programming] PlatformIO не найден (переменная PIO, $PIO_DEFAULT, \$PATH)." >&2
    exit 1
fi

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
ENV_NAME="${ENV_NAME:-m5stack-cores3}"

cd "$ROOT_DIR"

ARGS=(
    "run"
    "--environment" "$ENV_NAME"
    "--target" "upload"
)

# Позволяет явно указать порт при необходимости.
if [[ -n "${PORT:-}" ]]; then
    ARGS+=("--upload-port" "$PORT")
fi

echo "[programming] Порт для загрузки: ${PORT:-<из platformio.ini>}"
echo "[programming] Запускаю сборку и прошивку ($ENV_NAME) ..."
exec "$PIO" "${ARGS[@]}"