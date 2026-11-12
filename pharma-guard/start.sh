#!/usr/bin/env bash
# ONE-COMMAND START for Linux / macOS / WSL: builds the backend, starts it, then starts the frontend.
cd "$(dirname "$0")"
scripts/build-backend.sh || exit 1
backend/build/pharma_guard &
BACKEND_PID=$!
trap 'kill $BACKEND_PID 2>/dev/null' EXIT INT TERM
sleep 1
echo "Open http://localhost:5173 in your browser (Ctrl+C stops everything)."
scripts/run-frontend.sh
