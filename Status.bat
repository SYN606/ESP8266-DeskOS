@echo off
title DeskOS Agent Status
cd /d "%~dp0"
powershell -NoProfile -ExecutionPolicy Bypass -File "scripts\agent.ps1" -Status
pause
