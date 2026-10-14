# ESP32 DeskOS — Windows Development Guide

This guide describes how to develop, build, flash, and debug DeskOS on Windows using Antigravity, `uv`, and PlatformIO Core.

---

## 1. Prerequisites

- **OS:** Windows 10 or 11 (PowerShell 5.1+ or PowerShell 7+).
- **uv:** Modern Python and project manager. Verify with `uv --version`.
- **Git:** Distributed version control. Verify with `git --version`.
- **USB-to-UART Driver:** CP210x or CH340 driver (depending on your ESP32 board).

---

## 2. Environment Setup

All Python dependencies and PlatformIO Core are installed inside a local project virtual environment managed by `uv`.

```powershell
# Check uv environment and dependencies
uv sync

# Verify PlatformIO Core is available
uv run pio --version
```

---

## 3. Common Development Tasks

### A. Building the Firmware
Compile the C++ source files and verify no compilation errors:

```powershell
# Using the helper script:
.\scripts\build.ps1

# Or directly:
uv run pio run
```

### B. Checking Connected Hardware
Find the COM port of your connected ESP32:

```powershell
# In PowerShell:
[System.IO.Ports.SerialPort]::GetPortNames()

# Or run upload without parameters to view port list:
.\scripts\upload.ps1
```

### C. Uploading to ESP32
Flashing requires an explicit COM port for safety:

```powershell
.\scripts\upload.ps1 -ComPort COM3
```

### D. Opening the Serial Monitor
View live debug output at 115200 baud:

```powershell
.\scripts\monitor.ps1 -ComPort COM3
```

To exit the monitor, press `Ctrl+C` or `Ctrl+]`.

### E. Running the PC Telemetry Agent

```powershell
# Single snapshot:
uv run python pc-agent/agent.py

# Continuous streaming mode:
uv run python pc-agent/agent.py --loop --interval 2.0
```

---

## 4. Project Layout

```text
ESP32-DeskOS/
├── platformio.ini      # Board, framework, and library dependencies
├── pyproject.toml      # uv Python configuration (PlatformIO, psutil)
├── uv.lock             # Deterministic dependency lockfile
├── .gitignore          # Git exclusion rules
│
├── src/
│   ├── main.cpp        # Firmware entrypoint & FreeRTOS loop
│   ├── display.cpp     # U8g2 rendering & I2C bus scanner
│   └── display.h       # Display class interface
│
├── include/
│   ├── config.h        # Display driver selection & settings
│   └── pins.h          # Central hardware pin definitions
│
├── data/               # Web dashboard assets (SPIFFS/LittleFS)
│   ├── index.html
│   ├── style.css
│   └── app.js
│
├── pc-agent/           # Workstation telemetry collector
│   ├── agent.py
│   └── README.md
│
├── docs/               # Architecture, hardware, and dev guides
└── scripts/            # PowerShell build, upload, and monitor scripts
```
