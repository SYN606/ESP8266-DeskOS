@echo off
title ESP32 DeskOS Manager
cd /d "%~dp0"

:menu
cls
echo ==========================================
echo           ESP32 DeskOS Manager
echo ==========================================
echo   [1] Start in Background (Silent)
echo   [2] Start in Foreground (Clean Banner)
echo   [3] Check Status ^& Health
echo   [4] Stop Background Agent
echo   [5] View Live Logs
echo   [6] Exit
echo ==========================================
set /p choice="Select an option (1-6) [default: 1]: "

if "%choice%"=="" set choice=1
if "%choice%"=="1" goto bg
if "%choice%"=="2" goto fg
if "%choice%"=="3" goto status
if "%choice%"=="4" goto stop
if "%choice%"=="5" goto logs
if "%choice%"=="6" exit /b 0
goto menu

:bg
powershell -NoProfile -ExecutionPolicy Bypass -File "scripts\agent.ps1" -Background
pause
exit /b 0

:fg
powershell -NoProfile -ExecutionPolicy Bypass -File "scripts\agent.ps1"
pause
goto menu

:status
powershell -NoProfile -ExecutionPolicy Bypass -File "scripts\agent.ps1" -Status
pause
goto menu

:stop
powershell -NoProfile -ExecutionPolicy Bypass -File "scripts\agent.ps1" -Stop
pause
goto menu

:logs
powershell -NoProfile -ExecutionPolicy Bypass -File "scripts\agent.ps1" -Logs
goto menu
