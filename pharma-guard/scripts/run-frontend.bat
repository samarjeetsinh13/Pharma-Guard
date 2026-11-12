@echo off
rem Starts the React app (Vite dev server) on http://localhost:5173
cd /d "%~dp0..\frontend"
where npm >nul 2>nul
if errorlevel 1 (
  echo [ERROR] npm was not found. Install Node.js LTS from https://nodejs.org and open a NEW terminal.
  exit /b 1
)
if not exist node_modules (
  echo Installing frontend packages - first run only ...
  call npm install
  if errorlevel 1 exit /b 1
)
call npm run dev
