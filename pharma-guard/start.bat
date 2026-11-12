@echo off
rem ONE-CLICK START for Windows: builds the backend, opens two windows (backend + frontend) and the browser.
cd /d "%~dp0"
title Pharma Guard launcher

where g++ >nul 2>nul
if errorlevel 1 (
  echo [ERROR] g++ not found. Install MSYS2 and add C:\msys64\ucrt64\bin to PATH - see README.md, section "Windows".
  pause
  exit /b 1
)
where npm >nul 2>nul
if errorlevel 1 (
  echo [ERROR] npm not found. Install Node.js LTS from https://nodejs.org, then open a NEW terminal.
  pause
  exit /b 1
)

call scripts\build-backend.bat
if errorlevel 1 (
  pause
  exit /b 1
)

if not exist frontend\node_modules (
  echo Installing frontend packages - first run only, needs internet ...
  pushd frontend
  call npm install
  popd
)

start "Pharma Guard - C++ backend (port 8080)" cmd /k "cd /d %~dp0 && backend\build\pharma_guard.exe"
start "Pharma Guard - React frontend (port 5173)" cmd /k "cd /d %~dp0frontend && npm run dev"

echo Waiting for the servers to start ...
timeout /t 6 /nobreak >nul
start "" http://localhost:5173
echo.
echo Pharma Guard is running. Close the two server windows to stop it.
