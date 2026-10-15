@echo off
title DeskOS Agent Live Logs
cd /d "%~dp0"
powershell -NoProfile -ExecutionPolicy Bypass -File "scripts\agent.ps1" -Logs
pause
