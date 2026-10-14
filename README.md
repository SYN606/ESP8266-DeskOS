# ESP32 DeskOS

**ESP32 DeskOS** is a modular desktop companion gadget powered by an ESP32 microcontroller and a 1.3-inch monochrome OLED display (SH1106 / SSD1306).

It features animated robot eyes, real-time PC telemetry monitoring, Wi-Fi status, custom OLED notifications, a local web dashboard, and QR code generation.

---

## 🛠️ Technology Stack

| Layer | Technology |
|---|---|
| **Python Environment & Package Manager** | [uv](https://github.com/astral-sh/uv) |
| **Embedded Build System** | [PlatformIO Core](https://platformio.org/) (managed inside `uv`) |
| **Framework** | Arduino (ESP32) |
| **Firmware Language** | C++17 |
| **Display Library** | [olikraus/U8g2](https://github.com/olikraus/u8g2) |
| **Web Dashboard** | Vanilla HTML5 / CSS3 / ES6 (LittleFS ready) |
| **PC Telemetry Agent** | Python 3.11+ & `psutil` |

---

## 📂 Project Structure

```text
esp32-deskos/
│
├── .venv/                      # Local virtual environment (managed by uv)
├── .pio/                       # PlatformIO build artifacts & libraries
├── .gitignore                  # Git exclusions
├── .python-version             # Pinned Python version (3.12)
├── pyproject.toml              # uv project definitions & dependencies
├── uv.lock                     # Dependency lockfile
├── platformio.ini              # PlatformIO board & library configuration
├── README.md                   # Project overview & quickstart
├── AGENTS.md                   # AI agent & developer guidelines
│
├── src/
│   ├── main.cpp                # System bootloader, I2C scan & heartbeat loop
│   ├── display.cpp             # U8g2 driver integration & startup screen
│   └── display.h               # Display subsystem interface
│
├── include/
│   ├── config.h                # Display controller (SH1106 / SSD1306) & parameters
│   └── pins.h                  # Central hardware GPIO pin definitions
│
├── data/
│   ├── index.html              # Futuristic dark theme web dashboard
│   ├── style.css               # Responsive layout with red accent aesthetics
│   └── app.js                  # Client logic & interactive preview
│
├── pc-agent/
│   ├── agent.py                # Standalone psutil telemetry reporter (JSON)
│   └── README.md               # PC Agent usage instructions
│
├── docs/
│   ├── architecture.md         # Full system architecture & data flow
│   ├── hardware.md             # OLED wiring, pinout & controller details
│   └── development.md          # Step-by-step Windows development workflow
│
└── scripts/
    ├── build.ps1               # Compile firmware without flashing
    ├── upload.ps1              # Safe upload script requiring explicit COM port
    └── monitor.ps1             # 115200 baud serial monitor
```

---

## ⚡ Quickstart

### 1. Requirements

Ensure `uv` and `git` are installed on your Windows machine:

```powershell
uv --version
git --version
```

### 2. Install Project Dependencies

Dependencies are automatically resolved and installed via `uv`:

```powershell
uv sync
```

### 3. Compile Firmware

```powershell
# Using the helper script:
.\scripts\build.ps1

# Or with PlatformIO Core directly:
uv run pio run
```

### 4. Upload Firmware to ESP32

Connect your ESP32 board via USB, then pass your COM port to the upload script:

```powershell
.\scripts\upload.ps1 -ComPort COM3
```

> [!IMPORTANT]
> DeskOS will never flash automatically or guess a port. You must explicitly pass `-ComPort COMx`.

### 5. Monitor Serial Output

```powershell
.\scripts\monitor.ps1 -ComPort COM3
```

### 6. Run PC Telemetry Monitoring Agent

```powershell
# Single snapshot:
uv run python pc-agent/agent.py

# Continuous live stream:
uv run python pc-agent/agent.py --loop --interval 2.0
```

---

## 🔌 Hardware Wiring

| OLED Pin | ESP32 Pin | Function |
|---|---|---|
| **VCC** | **3V3** | Power (+3.3V) |
| **GND** | **GND** | Ground |
| **SDA** | **GPIO21** | I²C Data |
| **SCL** | **GPIO22** | I²C Clock |

*For full details on OLED controller identification and physical verification, see [`docs/hardware.md`](file:///d:/projects/ESP32-DeskOS/docs/hardware.md).*
