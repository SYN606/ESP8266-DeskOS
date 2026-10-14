# DeskOS PC Companion Agent

The **DeskOS PC Companion Agent** is a lightweight Python background bridge that collects real-time host telemetry (CPU %, RAM %, Uptime, Network throughput) and polls Windows Media (`winsdk`) to detect whatever song is currently playing on your computer.

It automatically streams structured JSON packets over USB Serial to your DeskOS OLED display.

---

## 🚀 Features

- **Auto-Port Detection**: Automatically detects your connected ESP8266 / ESP32 board (CP210x, CH340, FTDI) without requiring manual COM port arguments.
- **Windows Media ("Now Playing")**: Uses the native Windows Runtime Media Control API (`winsdk`) to capture live track metadata (Title, Artist, Playback Status) from **Spotify**, **YouTube**, **Apple Music**, **VLC**, etc.
- **Hardware Telemetry**: Accurate, non-intrusive sampling of CPU utilization, RAM usage, host uptime, and network throughput via `psutil`.
- **Dynamic QR Code Push**: Push any URL or text directly to your OLED screen using the `--qr` flag.
- **Custom OLED Messages**: Push custom text banners to your screen using the `--msg` flag.

---

## 📦 Requirements

- **Python**: 3.11+
- **Environment**: Managed exclusively through `uv` in the project root.
- **Dependencies**: `psutil`, `pyserial`, `winsdk` (installed automatically via `uv sync`).

---

## 💻 Usage

### 1. Start the Live Streaming Bridge

From the workspace root directory, run:

```powershell
.\scripts\agent.ps1
```
or directly via `uv`:
```powershell
uv run python pc-agent/agent.py
```

Expected output:
```text
[PORT] Auto-detected microcontroller on COM3
[BRIDGE] Connecting to COM3 at 115200 baud...
[BRIDGE] Connected successfully to COM3!
[BRIDGE] Streaming PC Stats & Media Telemetry (Ctrl+C to stop)...

[STREAM] CPU: 12.4% | RAM: 58.2% | Net: 45.2 KB/s | Now Playing: "Song Title" - Artist Name
```

### 2. Display a QR Code on OLED

To instantly generate and display a QR code on your desk screen:

```powershell
uv run python pc-agent/agent.py --qr "https://github.com/SYN606/ESP8266-DeskOS"
```

### 3. Display a Custom Message Banner

```powershell
uv run python pc-agent/agent.py --msg "Meeting in 5 mins!"
```

### 4. Console Snapshot (No Hardware Required)

To print a single formatted JSON telemetry snapshot without connecting to a board:

```powershell
uv run python pc-agent/agent.py --snapshot --pretty
```

---

## ⚙️ Command-Line Options

| Flag | Default | Description |
|---|---|---|
| `--serial <PORT>` | `auto` | Serial COM port (e.g. `COM3`). Auto-detects if omitted. |
| `--interval <SEC>` | `1.5` | Update interval in seconds. |
| `--qr <TEXT>` | `None` | Render a QR code on the display and exit. |
| `--msg <TEXT>` | `None` | Display a custom message banner on OLED and exit. |
| `--snapshot` | `False` | Output a single JSON metrics snapshot to stdout. |
| `--pretty` | `False` | Pretty-print JSON snapshot with indentation. |

---

## 🔒 Security & Performance

- **Zero Inbound Listeners:** Does not open any listening TCP/UDP ports on your computer.
- **Low CPU Footprint:** Uses non-blocking asynchronous sampling (<0.1% CPU consumption).
- **Graceful Fallbacks:** If media is paused or unavailable, the agent seamlessly falls back to PC stats without errors.
