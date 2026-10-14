# AGENTS.md — Developer & AI Agent Guidelines for ESP32 DeskOS

Welcome to **ESP32 DeskOS**. This document defines the architectural rules, coding standards, and operational guidelines for AI coding agents and human developers working in this repository.

---

## 1. Project Philosophy & Stack

- **Target Device:** ESP32 NodeMCU (ESP-WROOM-32), 1.3" Monochrome OLED (128×64, I²C).
- **Core Frameworks:** C++ / Arduino via PlatformIO Core, U8g2 for graphics.
- **Python Management:** Exclusively managed via `uv`. Never use global `pip` or create untracked `.venv`s.
- **Host Agent:** Python 3.11+ using `psutil`.

---

## 2. Hard Architectural Rules

1. **Explicit Upload Ports:**
   - **Never flash firmware automatically** without explicit user confirmation and an explicitly validated COM port.
   - Never guess a COM port when multiple devices exist.
2. **Central Hardware Definitions:**
   - All GPIO assignments must reside in [`include/pins.h`](file:///d:/projects/ESP32-DeskOS/include/pins.h). Never hardcode pin numbers in `.cpp` files.
   - Display driver configurations (SH1106 vs SSD1306) belong in [`include/config.h`](file:///d:/projects/ESP32-DeskOS/include/config.h).
3. **Non-Blocking Firmware Loops:**
   - Avoid long blocking `delay()` calls in the main FreeRTOS loop (`loop()`).
   - Use `millis()`-based timestamps for recurring tasks (heartbeats, screen refresh, sensor reads).
4. **Display Architecture:**
   - Keep OLED drawing routines inside `DeskDisplay` (`src/display.cpp`).
   - Use `U8G2_..._F_HW_I2C` full-framebuffer mode. Memory is abundant on ESP32 (320KB+ internal SRAM).
   - Both SH1106 and SSD1306 share address `0x3C`/`0x3D`. Do not assume the controller is identified based solely on an I²C scan.
5. **No Embedded Credentials:**
   - Do not commit hardcoded Wi-Fi SSIDs, passwords, or authentication tokens into source code.
   - Use configuration templates (`.env.example` or runtime NVS / captive portal configuration).
6. **Frontend Constraints:**
   - Dashboard assets in `data/` must remain lightweight, standalone (vanilla HTML/CSS/JS), and suitable for LittleFS flash storage.
   - Do not introduce npm/Node.js build pipelines for `data/` unless explicitly agreed upon.

---

## 3. Tool Execution Conventions

Always invoke development commands via `uv`:

```powershell
# Build firmware
uv run pio run

# Inspect project config
uv run pio project config

# Run PC agent
uv run python pc-agent/agent.py

# Launch serial monitor
uv run pio device monitor --baud 115200 --port COMx
```

Or execute the provided helper scripts in `scripts/`:
- `.\scripts\build.ps1`
- `.\scripts\upload.ps1 -ComPort COMx`
- `.\scripts\monitor.ps1 -ComPort COMx`
