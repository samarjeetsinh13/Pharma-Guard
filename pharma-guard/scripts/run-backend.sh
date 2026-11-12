#!/usr/bin/env bash
# Starts the C++ backend on http://127.0.0.1:8080 (builds it first if needed).
# Extra options are passed on, e.g.:  scripts/run-backend.sh --port 9000
cd "$(dirname "$0")/.."
[ -x backend/build/pharma_guard ] || scripts/build-backend.sh || exit 1
exec backend/build/pharma_guard "$@"
