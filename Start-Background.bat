@echo off
title DeskOS Background Agent Launcher
cd /d "%~dp0"
powershell -NoProfile -ExecutionPolicy Bypass -File "scripts\agent.ps1" -Background
pause
