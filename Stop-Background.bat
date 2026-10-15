@echo off
title Stop DeskOS Background Agent
cd /d "%~dp0"
powershell -NoProfile -ExecutionPolicy Bypass -File "scripts\agent.ps1" -Stop
pause
