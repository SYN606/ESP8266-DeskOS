#!/usr/bin/env python3
"""
ESP32/ESP8266 DeskOS — PC Telemetry & Media Companion Agent
Collects host system metrics (CPU, RAM, Uptime, Network) and live Windows Media ("Now Playing"),
and streams them in real-time over Serial to your DeskOS desktop display.
"""

import argparse
import asyncio
import json
import sys
import time
from datetime import datetime, timezone
import psutil

try:
    import serial
    import serial.tools.list_ports
    HAS_SERIAL = True
except ImportError:
    HAS_SERIAL = False

try:
    from winsdk.windows.media.control import (
        GlobalSystemMediaTransportControlsSessionManager as MediaManager,
        GlobalSystemMediaTransportControlsSessionPlaybackStatus as PlaybackStatus
    )
    HAS_WINSDK = True
except ImportError:
    HAS_WINSDK = False


def detect_serial_port() -> str | None:
    """Auto-detects the connected ESP32/ESP8266 COM port."""
    if not HAS_SERIAL:
        return None

    ports = list(serial.tools.list_ports.comports())
    if not ports:
        return None

    # Priority 1: Match known USB-to-UART bridge chips (CP210x, CH340, FTDI)
    for p in ports:
        desc = (p.description or "").lower()
        hwid = (p.hwid or "").lower()
        if any(chip in desc or chip in hwid for chip in ["cp210", "ch340", "ftdi", "usb-serial", "uart"]):
            return p.device

    # Priority 2: If only 1 port is available, use it
    if len(ports) == 1:
        return ports[0].device

    return ports[0].device


async def get_current_media_track() -> dict:
    """
    Queries the Windows Media Session Manager for currently playing media (Spotify, YouTube, VLC, etc.)
    """
    if not HAS_WINSDK:
        return {"title": "", "artist": "", "playing": False}

    try:
        manager = await MediaManager.request_async()
        session = manager.get_current_session()

        # If current session is None, inspect all active sessions for one that is PLAYING
        if not session:
            sessions = manager.get_sessions()
            for s in sessions:
                pb = s.get_playback_info()
                if pb and pb.playback_status == PlaybackStatus.PLAYING:
                    session = s
                    break

        if session:
            pb = session.get_playback_info()
            is_playing = (pb.playback_status == PlaybackStatus.PLAYING) if pb else False
            props = await session.try_get_media_properties_async()
            if props:
                return {
                    "title": props.title or "",
                    "artist": props.artist or "",
                    "playing": is_playing
                }
    except Exception:
        pass

    return {"title": "", "artist": "", "playing": False}


def get_cpu_metrics(sample_interval: float = 0.5) -> dict:
    """Collects CPU utilization overall and per-core."""
    cpu_percent = psutil.cpu_percent(interval=sample_interval)
    cpu_cores = psutil.cpu_percent(interval=None, percpu=True)
    cpu_freq = psutil.cpu_freq()

    return {
        "overall_percent": round(cpu_percent, 1),
        "core_count_logical": psutil.cpu_count(logical=True),
        "core_count_physical": psutil.cpu_count(logical=False),
        "frequency_current_mhz": round(cpu_freq.current, 1) if cpu_freq else None,
        "per_core_percent": [round(c, 1) for c in cpu_cores]
    }


def get_memory_metrics() -> dict:
    """Collects system virtual memory utilization."""
    mem = psutil.virtual_memory()
    swap = psutil.swap_memory()

    return {
        "total_bytes": mem.total,
        "used_bytes": mem.used,
        "available_bytes": mem.available,
        "percent_used": round(mem.percent, 1),
        "swap_percent_used": round(swap.percent, 1)
    }


def get_system_uptime() -> dict:
    """Calculates host system uptime from boot time."""
    boot_timestamp = psutil.boot_time()
    boot_time = datetime.fromtimestamp(boot_timestamp, tz=timezone.utc)
    uptime_seconds = int(time.time() - boot_timestamp)

    days, rem = divmod(uptime_seconds, 86400)
    hours, rem = divmod(rem, 3600)
    minutes, seconds = divmod(rem, 60)

    formatted = f"{days}d {hours:02d}h {minutes:02d}m" if days > 0 else f"{hours:02d}h {minutes:02d}m"

    return {
        "boot_time_utc": boot_time.isoformat(),
        "uptime_seconds": uptime_seconds,
        "formatted": formatted
    }


def get_network_throughput(interval: float = 0.5) -> dict:
    """Calculates network throughput over a brief time delta."""
    net1 = psutil.net_io_counters()
    time.sleep(interval)
    net2 = psutil.net_io_counters()

    bytes_sent_sec = (net2.bytes_sent - net1.bytes_sent) / interval
    bytes_recv_sec = (net2.bytes_recv - net1.bytes_recv) / interval

    return {
        "kb_sent_per_sec": round(bytes_sent_sec / 1024.0, 1),
        "kb_recv_per_sec": round(bytes_recv_sec / 1024.0, 1),
        "total_bytes_sent": net2.bytes_sent,
        "total_bytes_recv": net2.bytes_recv
    }


def collect_snapshot(sample_interval: float = 0.3) -> dict:
    """Aggregates a complete telemetry snapshot."""
    cpu = get_cpu_metrics(sample_interval=sample_interval)
    memory = get_memory_metrics()
    uptime = get_system_uptime()
    net = get_network_throughput(interval=sample_interval)

    return {
        "schema": "deskos-telemetry-v1",
        "timestamp_utc": datetime.now(timezone.utc).isoformat(),
        "cpu": cpu,
        "memory": memory,
        "uptime": uptime,
        "network": net
    }


def run_serial_stream(port: str, interval: float = 1.5):
    """Streams live telemetry and media packets to the DeskOS OLED."""
    print(f"[BRIDGE] Connecting to {port} at 115200 baud...")
    try:
        ser = serial.Serial(port, 115200, timeout=1)
        time.sleep(1.0)
        print(f"[BRIDGE] Connected successfully to {port}!")
        print(f"[BRIDGE] Streaming PC Stats & Media Telemetry (Ctrl+C to stop)...\n")

        last_media_sig = None

        while True:
            # 1. Collect telemetry
            snap = collect_snapshot(sample_interval=0.3)
            media = asyncio.run(get_current_media_track())

            # 2. Check media state
            media_sig = (media["title"], media["artist"], media["playing"])
            media_changed = (media_sig != last_media_sig)
            last_media_sig = media_sig

            # 3. If media is playing or changed, push media packet
            if media_changed or media["playing"]:
                media_payload = {
                    "cmd": "media",
                    "title": media["title"],
                    "artist": media["artist"],
                    "playing": media["playing"],
                    "auto_show": media["playing"]
                }
                ser.write((json.dumps(media_payload) + "\n").encode("utf-8"))
                ser.flush()

            # 4. Push stats packet (auto_show will display stats if no music is actively playing)
            stats_payload = {
                "cmd": "stats",
                "cpu": round(snap["cpu"]["overall_percent"], 1),
                "ram": round(snap["memory"]["percent_used"], 1),
                "uptime": snap["uptime"]["formatted"],
                "net_in": snap["network"]["kb_recv_per_sec"],
                "net_out": snap["network"]["kb_sent_per_sec"],
                "auto_show": not media["playing"]
            }
            ser.write((json.dumps(stats_payload) + "\n").encode("utf-8"))
            ser.flush()

            # 5. User-friendly terminal log
            music_status = f"Now Playing: \"{media['title']}\" - {media['artist']}" if media["playing"] else "Music: None"
            print(
                f"[STREAM] CPU: {stats_payload['cpu']:>4.1f}% | RAM: {stats_payload['ram']:>4.1f}% | Net: {stats_payload['net_in']:>5.1f} KB/s | {music_status}",
                flush=True
            )

            time.sleep(max(0.5, interval - 0.6))

    except KeyboardInterrupt:
        print("\n[BRIDGE] Stopped by user.")
        sys.exit(0)
    except Exception as err:
        print(f"\n[BRIDGE ERROR] {err}")
        sys.exit(1)


def main():
    parser = argparse.ArgumentParser(description="ESP32/ESP8266 DeskOS PC Companion Agent")
    parser.add_argument("--serial", type=str, default="auto", help="COM port to stream to (default: auto-detect)")
    parser.add_argument("--interval", type=float, default=1.5, help="Update interval in seconds (default: 1.5)")
    parser.add_argument("--qr", type=str, default=None, help="Send a QR code URL or text directly to display")
    parser.add_argument("--msg", type=str, default=None, help="Display a custom message on OLED")
    parser.add_argument("--snapshot", action="store_true", help="Output a single JSON snapshot to stdout and exit")
    parser.add_argument("--pretty", action="store_true", help="Pretty print JSON snapshot")
    args = parser.parse_args()

    # Snapshot Mode
    if args.snapshot:
        snap = collect_snapshot(sample_interval=0.5)
        indent = 2 if args.pretty else None
        print(json.dumps(snap, indent=indent))
        return

    # Auto-detect or validate port
    port = args.serial
    if port == "auto" or port is None:
        port = detect_serial_port()
        if not port:
            print("[ERROR] No serial COM ports detected! Please check your USB cable connection.")
            print("[HINT] Run with --snapshot to test metrics without a microcontroller.")
            sys.exit(1)
        print(f"[PORT] Auto-detected microcontroller on {port}")

    # One-shot QR command
    if args.qr:
        print(f"[BRIDGE] Sending QR Code: '{args.qr}' to {port}...")
        ser = serial.Serial(port, 115200, timeout=1)
        time.sleep(1.0)
        ser.write((json.dumps({"cmd": "qr", "text": args.qr}) + "\n").encode("utf-8"))
        ser.flush()
        ser.close()
        print("[BRIDGE] QR Code rendered successfully!")
        return

    # One-shot Message command
    if args.msg:
        print(f"[BRIDGE] Sending Message: '{args.msg}' to {port}...")
        ser = serial.Serial(port, 115200, timeout=1)
        time.sleep(1.0)
        ser.write((json.dumps({"cmd": "msg", "line1": args.msg, "line2": ""}) + "\n").encode("utf-8"))
        ser.flush()
        ser.close()
        print("[BRIDGE] Message sent!")
        return

    # Default Mode: Stream live PC Stats and Media to OLED
    run_serial_stream(port=port, interval=args.interval)


if __name__ == "__main__":
    main()
