#!/usr/bin/env bash
set -euo pipefail
PORT="${1:-8080}"
mkdir -p build
make >/dev/null
# echo "[blocking] listening on :$PORT"
exec ./build/server_blocking "$PORT"
