#!/usr/bin/env bash
# Starts the React app (Vite dev server) on http://localhost:5173
cd "$(dirname "$0")/../frontend"
command -v npm >/dev/null || { echo "[ERROR] npm not found. Install Node.js LTS from https://nodejs.org"; exit 1; }
[ -d node_modules ] || npm install
exec npm run dev
