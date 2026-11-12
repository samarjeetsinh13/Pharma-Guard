@echo off
rem Compiles the C++ backend into backend\build\pharma_guard.exe (needs g++ from MSYS2/MinGW-w64).
setlocal enabledelayedexpansion
cd /d "%~dp0..\backend"

where g++ >nul 2>nul
if errorlevel 1 (
  echo [ERROR] g++ was not found. Install MSYS2 and add C:\msys64\ucrt64\bin to PATH - see README.md, section "Windows".
  exit /b 1
)

if not exist build mkdir build
set FILES=
for %%f in (src\*.cpp) do set FILES=!FILES! %%f

echo Compiling the C++ backend ...
g++ -std=c++17 -O2 -Wall -Wextra -pedantic -pthread -Iinclude !FILES! -o build\pharma_guard.exe -lws2_32 -static
if errorlevel 1 (
  echo [ERROR] Compilation failed. Read the messages above.
  exit /b 1
)
echo Build OK: backend\build\pharma_guard.exe
exit /b 0
