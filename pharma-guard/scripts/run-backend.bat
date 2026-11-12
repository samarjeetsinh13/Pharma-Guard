@echo off
rem Starts the C++ backend on http://127.0.0.1:8080  (builds it first if needed).
rem Extra options are passed on, e.g.:  scripts\run-backend.bat --port 9000
cd /d "%~dp0.."
if not exist backend\build\pharma_guard.exe (
  call scripts\build-backend.bat
  if errorlevel 1 exit /b 1
)
backend\build\pharma_guard.exe %*
