#!/usr/bin/env bash
# Compiles the C++ backend into backend/build/pharma_guard
set -e
cd "$(dirname "$0")/../backend"
command -v g++ >/dev/null || command -v clang++ >/dev/null || { echo "[ERROR] No C++ compiler found. Ubuntu: sudo apt install build-essential"; exit 1; }
command -v make >/dev/null || { echo "[ERROR] make not found. Ubuntu: sudo apt install build-essential"; exit 1; }
make
echo "Build OK: backend/build/pharma_guard"
