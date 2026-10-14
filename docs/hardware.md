# ESP32 DeskOS — Hardware Specification & Verification Guide

This document details the hardware configuration, pin mapping, display controller specifics, and verification steps for ESP32 DeskOS.

---

## 1. Primary Hardware Components

| Component | Specification | Notes |
|---|---|---|
| **Microcontroller** | ESP32 NodeMCU (ESP-WROOM-32) | 240MHz dual-core Xtensa LX6, 520KB SRAM, 4MB Flash |
| **Display Module** | 1.3-inch Monochrome OLED | 128×64 pixels, I²C interface |
| **Display Driver IC** | SH1106 (common) or SSD1306 | Selected in `include/config.h` |
| **Host PC Connection** | Micro-USB or USB-C (CP2102/CH340) | UART programming & serial telemetry |

---

## 2. Wiring & Pinout

The OLED communicates with the ESP32 over the default hardware I²C bus (`Wire`).

| OLED Pin | ESP32 Pin | Function | Notes |
|---|---|---|---|
| **VCC** | **3V3** | Power (+3.3V) | Do not connect to 5V unless module has 5V regulator |
| **GND** | **GND** | Ground | Common system ground |
| **SDA** | **GPIO21** | I²C Serial Data | Defined in `include/pins.h` |
| **SCL** | **GPIO22** | I²C Serial Clock | Defined in `include/pins.h` |

### Pull-Up Resistors
Most 1.3" OLED breakout boards include onboard 4.7kΩ or 10kΩ pull-up resistors on SDA and SCL. If using a bare panel without integrated pull-ups, install 4.7kΩ resistors between 3V3 and SDA/SCL.

---

## 3. OLED Controller Identification (SH1106 vs. SSD1306)

1.3-inch and 0.96-inch OLED modules look very similar, but have crucial internal differences:

- **1.3-inch displays** predominantly use the **SH1106** controller.
  - The SH1106 internal RAM buffer is 132×64 columns (with 2 offset pixels on either side).
  - If driven by an SSD1306 driver, a 1.3" display will typically show a 2-pixel shifted image or noise artifacts along the screen edge.
- **0.96-inch displays** predominantly use the **SSD1306** controller.
  - The internal RAM buffer is exactly 128×64 columns.

### Switching the Driver in Code
In [`include/config.h`](file:///d:/projects/ESP32-DeskOS/include/config.h):

```cpp
// Set to 1 for 1.3" SH1106, or 0 for 0.96" SSD1306
#define OLED_CONTROLLER_SH1106 1
```

> [!WARNING]
> Both SH1106 and SSD1306 respond to I²C addresses `0x3C` (or `0x3D` if address jumper is bridged). An I²C bus scan **cannot** distinguish between SH1106 and SSD1306. Visual inspection of the physical screen output is required.

---

## 4. Hardware Verification Procedure

Follow this checklist when testing a physical device:

1. **Wiring Inspection:**
   - Confirm VCC is on 3V3 and GND is connected.
   - Confirm SDA is on GPIO21 and SCL is on GPIO22.
2. **Connect to Host PC:**
   - Plug the ESP32 into a USB port on your Windows PC.
   - Run `[System.IO.Ports.SerialPort]::GetPortNames()` or check Device Manager under "Ports (COM & LPT)" to identify the COM port.
3. **Flash Firmware:**
   ```powershell
   .\scripts\upload.ps1 -ComPort COMx
   ```
4. **Monitor Boot Logs:**
   ```powershell
   .\scripts\monitor.ps1 -ComPort COMx
   ```
   Expected serial output:
   - ESP32 chip info and free heap.
   - `[I2C] Found device at address 0x3C (Likely OLED display - SH1106 or SSD1306)`
   - `[DISPLAY] Startup screen sent to OLED.`
   - Periodic heartbeat messages every 5 seconds.
5. **Physical Display Inspection:**
   - Verify that the outer border and text ("ESP32 DeskOS", "SYSTEM ONLINE", "Firmware v0.1") appear clearly centered.
   - If the image is shifted by 2 pixels or corrupt, change `OLED_CONTROLLER_SH1106` to `0` in `include/config.h` and rebuild.
