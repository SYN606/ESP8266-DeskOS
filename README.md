# ESP8266 / ESP32 DeskOS

**DeskOS** is an open-source, ultra-responsive desktop companion gadget powered by an ESP8266 (NodeMCU) or ESP32 microcontroller and a 1.3-inch monochrome OLED display (SH1106 / SSD1306).

It features lifelike animated robot eyes, real-time PC telemetry gauges, a Windows Media "Now Playing" music player with animated equalizer, dynamic QR code generation, and a local Wi-Fi web dashboard.

---

## ✨ Features

- 👁️ **Lifelike Robot Eye & Character Animations**:
  - **Asymmetric Eyelid Dynamics**: Upper eyelids drop ~82% while lower lids rise ~18% for true biological blinking.
  - **Double-Blinks**: Spontaneous organic double-blinks (Pixar/Anki inspired).
  - **Saccadic Gaze & Micro-Tremors**: Critically damped spring physics for ballistic glance shifts combined with sub-pixel physiological ocular drift.
  - **Active Dark Pupils & Specular Glint**: Black pupils that actively track the gaze in all directions with specular reflections.
  - **Articulated 3-Point Curved Eyebrows**: Inner, mid-arch, and outer tail control points with dynamic inertial secondary motion and blink bounce physics.
  - **Unique Character Faces & Moods**:
    - **Neutral**: Iconic balanced robot companion.
    - **Happy**: Smiling cheek cutouts and joyous upward brows.
    - **Skeptical**: High questioning arched brow and squinted eye.
    - **Angry**: Fierce downward V-chevron brow furrow.
    - **Sleepy**: Heavy drooping eyelids.
    - **Love**: Pulsing heart eyes.
    - **Wink**: Cheeky winking eye with cheerful smile arc.
    - **Cat Face**: Pointed 3D ears, feline vertical slit pupils, cute nose & whiskers!
    - **Owl Face**: Large feathery facial discs, nocturnal plumicorn ear tufts & hooked center beak!
- 📊 **Real-Time PC Telemetry Monitor**:
  - Live CPU % utilization, RAM % memory gauge, System Uptime, and Network I/O throughput (KB/s).
  - Smooth linear interpolation (lerp) for fluid analog-style bar gauges.
- 🎵 **Live PC Music Player ("Now Playing")**:
  - Automatically hooks into Windows Media (`winsdk`) and Spotify desktop (`ctypes`) to detect music playing from **Spotify**, **YouTube**, **Apple Music**, **VLC**, etc.
  - Renders an animated 5-bar audio equalizer, spinning vinyl disc graphic, scrolling marquee track title, and artist name.
  - Retains song title and artist when paused with dedicated `"MUSIC PAUSED"` screen.
- 📱 **Dynamic QR Code Generator**:
  - Generates sharp 2x scaled QR codes (Version 3) directly on the OLED.
  - Instant one-shot push via CLI (`--qr "https://..."`) or the Web Dashboard.
- 🕒 **PC-Synced Live Clock & Calendar**:
  - Automatically synchronizes with your computer's local clock over USB (`SYNC: PC AGENT`).
- 🌐 **Embedded Wi-Fi Web Dashboard**:
  - Futuristic dark-themed web interface hosted directly on the ESP.
  - Switch display modes, trigger blinks, adjust screen contrast/brightness, and change character moods.
- ⚡ **High-Performance Non-Blocking Architecture**:
  - Overclocked to 160MHz with high-speed 800kHz I²C bus.
  - 60 FPS animation tick rate with zero blocking `delay()` calls.

---

## 🛠️ Technology Stack

| Layer | Technology |
|---|---|
| **Microcontroller** | NodeMCU ESP8266EX (160MHz) / ESP32 NodeMCU |
| **Display** | 1.3" Monochrome OLED 128×64 (SH1106 / SSD1306, I²C address `0x3C`) |
| **Firmware Core** | C++17 via Arduino / [PlatformIO Core](https://platformio.org/) |
| **Graphics Library** | [U8g2](https://github.com/olikraus/u8g2) (Full framebuffer mode) |
| **QR Code Engine** | [ricmoo/QRCode](https://github.com/ricmoo/QRCode) |
| **JSON Serialization** | [ArduinoJson 7](https://arduinojson.org/) |
| **Python Environment** | Managed exclusively with [`uv`](https://github.com/astral-sh/uv) |
| **PC Host Bridge** | Python 3.11+ using `psutil`, `pyserial`, and `winsdk` |

---

## 📂 Project Structure

```text
ESP8266-DeskOS/
│
├── .venv/                      # Virtual environment (managed by uv)
├── .pio/                       # PlatformIO build artifacts & libraries
├── pyproject.toml              # uv dependencies (psutil, pyserial, winsdk, platformio)
├── platformio.ini              # PlatformIO environments (nodemcuv2, esp32dev)
├── README.md                   # Main project documentation
├── AGENTS.md                   # AI agent & developer guidelines
│
├── src/
│   ├── main.cpp                # System bootloader, serial commands & main loop
│   ├── display.h / .cpp        # U8g2 display engine & screen renderers
│   ├── eyes.h / .cpp           # Physics-based robot, cat & owl animations
│   ├── network.h / .cpp        # Wi-Fi AP/STA mode, NTP time & REST dashboard
│   └── telemetry.h             # Shared telemetry & media state structures
│
├── include/
│   ├── config.h                # Controller settings (SH1106 vs SSD1306), I2C speed
│   └── pins.h                  # Hardware pin mapping for ESP8266 and ESP32
│
├── pc-agent/
│   ├── agent.py                # PC telemetry collector & Windows Media poller
│   └── README.md               # PC companion agent documentation
│
├── docs/
│   ├── architecture.md         # Full system architecture & data flow
│   ├── hardware.md             # OLED wiring, pinout & controller details
│   └── development.md          # Step-by-step Windows development workflow
│
├── DeskOS.bat                  # Unified 1-click launcher (Background, Status, Stop, Logs)
└── scripts/
    ├── build.ps1               # Compile firmware without flashing
    ├── upload.ps1              # Safe upload script with auto-port detection
    ├── monitor.ps1             # 115200 baud serial monitor
    └── agent.ps1               # Companion bridge manager (-Background, -Status, -Stop, -Logs)
```

---

## 🔌 Hardware Wiring

### ESP8266 NodeMCU Pinout (Recommended)

| OLED Pin | ESP8266 NodeMCU Pin | GPIO | Function |
|---|---|---|---|
| **VCC** | **3V3** | — | +3.3V Power |
| **GND** | **GND** | — | Ground |
| **SDA** | **D2** | **GPIO4** | Hardware I²C Data |
| **SCL** | **D1** | **GPIO5** | Hardware I²C Clock |

> [!NOTE]
> Do **not** use pin `D0` (GPIO16) for I²C on ESP8266; `D0` is reserved for deep-sleep wake and lacks open-drain output. Pins `D2` and `D1` provide reliable 800kHz I²C performance.

### ESP32 NodeMCU Pinout

| OLED Pin | ESP32 Pin | GPIO | Function |
|---|---|---|---|
| **VCC** | **3V3** | — | +3.3V Power |
| **GND** | **GND** | — | Ground |
| **SDA** | **D21** | **GPIO21** | Hardware I²C Data |
| **SCL** | **D22** | **GPIO22** | Hardware I²C Clock |

---

## ⚡ Quickstart

### 1. Requirements

Ensure [`uv`](https://github.com/astral-sh/uv) and `git` are installed:

```powershell
uv --version
git --version
```

### 2. Install Project Dependencies

Dependencies are automatically resolved and synced in an isolated environment via `uv`:

```powershell
uv sync
```

### 3. Build Firmware

```powershell
# Using the helper script:
.\scripts\build.ps1

# Or with PlatformIO directly:
uv run pio run
```

### 4. Upload Firmware

Connect your board via USB and run:

```powershell
# Auto-detects port or pass explicitly:
.\scripts\upload.ps1 -ComPort COM3 -Yes
```

### 5. Launch the PC Companion Bridge

The PC companion streams live CPU/RAM/Network stats, local time, and active media playback (Spotify, YouTube, VLC) directly to your DeskOS OLED.

#### Option A: 1-Click Interactive Menu (Recommended)
Simply double-click **`DeskOS.bat`** in the repository root for an instant interactive menu:
- `[1] Start in Background (Silent)` — Starts the agent silently without an open window.
- `[2] Start in Foreground (Clean Banner)` — Runs with a concise 5-line startup banner.
- `[3] Check Status & Health` — Shows PID, memory usage, and recent activity.
- `[4] Stop Background Agent` — Cleanly terminates the background process.
- `[5] View Live Logs` — Streams real-time telemetry output.

#### Option B: PowerShell CLI (`scripts/agent.ps1`)
All functions are directly available from PowerShell:
```powershell
# Start silently in the background
.\scripts\agent.ps1 -Background

# Check status & memory usage
.\scripts\agent.ps1 -Status

# Stream real-time logs (Ctrl+C to stop viewing)
.\scripts\agent.ps1 -Logs

# Stop the background process
.\scripts\agent.ps1 -Stop

# Or run in foreground with clean single banner
.\scripts\agent.ps1
```

> [!TIP]
> The foreground agent displays a clean 5-line startup banner without spamming console output every second. To view real-time per-second telemetry logs, pass `-ShowStream` (or `--verbose` / `-v`).

---

## 🎮 Controls & Shortcuts

### Serial Monitor Commands (115200 Baud)

When connected to `.\scripts\monitor.ps1`, press:
- `e` — Switch to **Robot / Character Eyes**
- `s` — Switch to **PC Telemetry Monitor**
- `p` — Switch to **Music Player**
- `q` — Switch to **QR Code**
- `c` — Switch to **Digital Clock & Date**
- `b` — Trigger instant **Blink**
- `h` — Set mood to **Happy**
- `n` — Set mood to **Neutral**
- `k` — Set mood to **Skeptical**
- `a` — Set mood to **Angry**
- `z` — Set mood to **Sleepy**
- `l` — Set mood to **Love** (Heart Eyes)
- `w` — Set mood to **Wink**
- `t` — Set face to **Cat** (Pointed Ears & Slit Pupils)
- `o` — Set face to **Owl** (Round Eyes, Horns & Beak)
- `m` — Cycle to **Next Mood / Face**
- `[Space]` — Cycle to next display mode

### PC Agent CLI Commands

```powershell
# Display a custom QR Code on the OLED:
uv run python pc-agent/agent.py --qr "https://github.com/SYN606/ESP8266-DeskOS"

# Print a single JSON telemetry snapshot to console:
uv run python pc-agent/agent.py --snapshot --pretty
```

---

## 📄 License

MIT License. Feel free to use, modify, and distribute for personal or commercial projects.
