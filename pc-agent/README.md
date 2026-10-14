# ESP32 DeskOS — PC Monitoring Agent

The PC Monitoring Agent is a lightweight telemetry collector built with Python and `psutil`. It reads CPU utilization, RAM usage, host uptime, network throughput, and optional CPU temperature sensors, outputting structured JSON metrics for DeskOS.

## Requirements

- Python 3.11+
- Managed via `uv` in the project root
- Dependencies: `psutil`

## Execution

From the workspace root directory, run:

```powershell
uv run python pc-agent/agent.py
```

### Options

| Flag | Description |
|---|---|
| *(default)* | Outputs a single formatted JSON snapshot |
| `--compact` | Outputs a single-line compact JSON object |
| `--loop` | Continuously outputs telemetry snapshots |
| `--interval <seconds>` | Sampling interval for continuous mode (default: 2.0s) |

### Example Continuous Run

```powershell
uv run python pc-agent/agent.py --loop --interval 3.0
```

## Security & Architecture

- **No Privileged Operations:** Runs entirely in unprivileged user space.
- **No Inbound Network Listeners:** Does not expose open HTTP or TCP ports on your host machine.
- **Future Firmware Bridge:** Will deliver JSON telemetry to the ESP32 DeskOS over USB Serial UART or local Wi-Fi HTTP POST/WebSocket when configured.
