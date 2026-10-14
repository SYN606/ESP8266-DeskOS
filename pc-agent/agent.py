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
    serial = None  # type: ignore[assignment]
    HAS_SERIAL = False

try:
    from winsdk.windows.media.control import (
        GlobalSystemMediaTransportControlsSessionManager as MediaManager,
        GlobalSystemMediaTransportControlsSessionPlaybackStatus as PlaybackStatus
    )
    HAS_WINSDK = True
except ImportError:
    MediaManager = None  # type: ignore[assignment]
    PlaybackStatus = None  # type: ignore[assignment]
    HAS_WINSDK = False


# State for non-blocking instantaneous differential metrics
_last_net_time = 0.0
_last_net_sent = 0
_last_net_recv = 0
_cpu_init = False


def detect_serial_port() -> str | None:
    """Auto-detects the connected ESP32/ESP8266 COM port."""
    if not HAS_SERIAL or serial is None:
        return None

    ports = list(serial.tools.list_ports.comports())
    if not ports:
        return None

    for p in ports:
        desc = (p.description or "").lower()
        hwid = (p.hwid or "").lower()
        if any(chip in desc or chip in hwid for chip in ["cp210", "ch340", "ftdi", "usb-serial", "uart"]):
            return p.device

    if len(ports) == 1:
        return ports[0].device

    return ports[0].device


async def get_current_media_track() -> dict:
    """Queries Windows Media Session Manager or Spotify window title for currently playing track."""
    if HAS_WINSDK and MediaManager is not None and PlaybackStatus is not None:
        try:
            manager = await MediaManager.request_async()
            if manager:
                session = manager.get_current_session()

                # If no current session or current session is not playing, scan all available sessions
                sessions = manager.get_sessions()
                if sessions is not None:
                    # Priority 1: Find any session actively PLAYING
                    if not session or (session.get_playback_info() and session.get_playback_info().playback_status != PlaybackStatus.PLAYING):
                        for s in sessions:
                            pb = s.get_playback_info()
                            if pb and pb.playback_status == PlaybackStatus.PLAYING:
                                session = s
                                break

                    # Priority 2: If none is playing, find any session that has metadata (e.g. paused)
                    if not session:
                        for s in sessions:
                            props = await s.try_get_media_properties_async()
                            if props and (props.title or props.artist):
                                session = s
                                break

                if session:
                    pb = session.get_playback_info()
                    is_playing = (pb.playback_status == PlaybackStatus.PLAYING) if pb else False
                    props = await session.try_get_media_properties_async()
                    if props and (props.title or props.artist):
                        return {
                            "title": props.title or "",
                            "artist": props.artist or "",
                            "playing": is_playing
                        }
        except Exception:
            pass

    # Windows fallback: Spotify desktop window title
    try:
        import ctypes
        user32 = ctypes.windll.user32
        spotify_info = None

        def enum_win(hwnd: int, _: int) -> bool:
            nonlocal spotify_info
            if not user32.IsWindowVisible(hwnd):
                return True
            length = user32.GetWindowTextLengthW(hwnd)
            if length > 0:
                buff = ctypes.create_unicode_buffer(length + 1)
                user32.GetWindowTextW(hwnd, buff, length + 1)
                title = buff.value
                if " - " in title:
                    pid = ctypes.c_ulong()
                    user32.GetWindowThreadProcessId(hwnd, ctypes.byref(pid))
                    try:
                        p = psutil.Process(pid.value)
                        if "spotify" in p.name().lower():
                            parts = title.split(" - ", 1)
                            spotify_info = {
                                "artist": parts[0].strip(),
                                "title": parts[1].strip(),
                                "playing": True
                            }
                            return False
                    except Exception:
                        pass
            return True

        cb_proto = ctypes.WINFUNCTYPE(ctypes.c_bool, ctypes.c_void_p, ctypes.c_void_p)
        user32.EnumWindows(cb_proto(enum_win), 0)
        if spotify_info:
            return spotify_info
    except Exception:
        pass

    return {"title": "", "artist": "", "playing": False}


def get_cpu_metrics() -> dict:
    """Collects CPU metrics non-blockingly using cumulative counters."""
    global _cpu_init
    if not _cpu_init:
        psutil.cpu_percent(interval=None)
        _cpu_init = True
        time.sleep(0.05)

    cpu_percent = psutil.cpu_percent(interval=None)
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


def get_network_throughput() -> dict:
    """Calculates network throughput non-blockingly using time deltas."""
    global _last_net_time, _last_net_sent, _last_net_recv

    now = time.time()
    net = psutil.net_io_counters()

    if _last_net_time == 0.0:
        _last_net_time = now
        _last_net_sent = net.bytes_sent
        _last_net_recv = net.bytes_recv
        return {
            "kb_sent_per_sec": 0.0,
            "kb_recv_per_sec": 0.0,
            "total_bytes_sent": net.bytes_sent,
            "total_bytes_recv": net.bytes_recv
        }

    dt = max(0.001, now - _last_net_time)
    sent_sec = (net.bytes_sent - _last_net_sent) / dt
    recv_sec = (net.bytes_recv - _last_net_recv) / dt

    _last_net_time = now
    _last_net_sent = net.bytes_sent
    _last_net_recv = net.bytes_recv

    return {
        "kb_sent_per_sec": round(sent_sec / 1024.0, 1),
        "kb_recv_per_sec": round(recv_sec / 1024.0, 1),
        "total_bytes_sent": net.bytes_sent,
        "total_bytes_recv": net.bytes_recv
    }


def collect_snapshot() -> dict:
    """Aggregates a complete telemetry snapshot without blocking delays."""
    cpu = get_cpu_metrics()
    memory = get_memory_metrics()
    uptime = get_system_uptime()
    net = get_network_throughput()

    return {
        "schema": "deskos-telemetry-v1",
        "timestamp_utc": datetime.now(timezone.utc).isoformat(),
        "cpu": cpu,
        "memory": memory,
        "uptime": uptime,
        "network": net
    }


def run_serial_stream(port: str, interval: float = 1.0, mock_media: dict | None = None):
    """Streams live telemetry and media packets to the DeskOS OLED."""
    if not HAS_SERIAL or serial is None:
        print("[ERROR] pyserial is not installed in the environment.")
        sys.exit(1)

    print(f"[BRIDGE] Connecting to {port} at 115200 baud...")
    try:
        ser = serial.Serial(port, 115200, timeout=1)
        time.sleep(1.0)
        print(f"[BRIDGE] Connected successfully to {port}!")
        print(f"[BRIDGE] Streaming PC Stats & Media Telemetry (Ctrl+C to stop)...\n")

        # Prime counters
        collect_snapshot()
        last_media_sig = None
        last_media_sent_time = 0.0

        while True:
            t_start = time.time()

            snap = collect_snapshot()
            if mock_media:
                media = mock_media
            else:
                media = asyncio.run(get_current_media_track())

            media_sig = (media["title"], media["artist"], media["playing"])
            media_changed = (media_sig != last_media_sig)
            time_since_last_media = t_start - last_media_sent_time

            # Send media update if changed, or periodically every 4 seconds if we have a track
            if media_changed or (media["title"] and time_since_last_media > 4.0):
                was_playing = last_media_sig[2] if last_media_sig else False
                just_started_playing = media["playing"] and not was_playing

                media_payload = {
                    "cmd": "media",
                    "title": media["title"],
                    "artist": media["artist"],
                    "playing": media["playing"],
                    "auto_show": just_started_playing
                }
                ser.write((json.dumps(media_payload) + "\n").encode("utf-8"))
                ser.flush()
                last_media_sent_time = t_start

            last_media_sig = media_sig

            now_dt = datetime.now()
            stats_payload = {
                "cmd": "stats",
                "cpu": round(snap["cpu"]["overall_percent"], 1),
                "ram": round(snap["memory"]["percent_used"], 1),
                "uptime": snap["uptime"]["formatted"],
                "net_in": snap["network"]["kb_recv_per_sec"],
                "net_out": snap["network"]["kb_sent_per_sec"],
                "time": now_dt.strftime("%H:%M:%S"),
                "date": now_dt.strftime("%a, %b %d %Y"),
                "auto_show": False
            }
            ser.write((json.dumps(stats_payload) + "\n").encode("utf-8"))
            ser.flush()

            music_status = f"Now Playing: \"{media['title']}\" - {media['artist']}" if media["playing"] else (
                f"Music Paused: \"{media['title']}\"" if media["title"] else "Music: None"
            )
            print(
                f"[STREAM] {stats_payload['time']} | CPU: {stats_payload['cpu']:>4.1f}% | RAM: {stats_payload['ram']:>4.1f}% | Net: {stats_payload['net_in']:>5.1f} KB/s | {music_status}",
                flush=True
            )

            elapsed = time.time() - t_start
            time.sleep(max(0.1, interval - elapsed))

    except KeyboardInterrupt:
        print("\n[BRIDGE] Stopped by user.")
        sys.exit(0)
    except Exception as err:
        print(f"\n[BRIDGE ERROR] {err}")
        sys.exit(1)


def main():
    parser = argparse.ArgumentParser(description="ESP32/ESP8266 DeskOS PC Companion Agent")
    parser.add_argument("--serial", type=str, default="auto", help="COM port to stream to (default: auto-detect)")
    parser.add_argument("--interval", type=float, default=1.0, help="Update interval in seconds (default: 1.0)")
    parser.add_argument("--qr", type=str, default=None, help="Send a QR code URL or text directly to display")
    parser.add_argument("--test-media", action="store_true", help="Stream mock playing music track for visual testing")
    parser.add_argument("--track", type=str, default=None, help="Custom track title for testing")
    parser.add_argument("--artist", type=str, default="Rick Astley", help="Custom artist for testing")
    parser.add_argument("--snapshot", action="store_true", help="Output a single JSON snapshot to stdout and exit")
    parser.add_argument("--pretty", action="store_true", help="Pretty print JSON snapshot")
    args = parser.parse_args()

    # Snapshot Mode
    if args.snapshot:
        snap = collect_snapshot()
        indent = 2 if args.pretty else None
        print(json.dumps(snap, indent=indent))
        return

    # Check serial support
    if not HAS_SERIAL or serial is None:
        print("[ERROR] pyserial is not installed in the environment.")
        sys.exit(1)

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

    # Mock media handling
    mock_media = None
    if args.test_media or args.track:
        mock_media = {
            "title": args.track or "Never Gonna Give You Up",
            "artist": args.artist or "Rick Astley",
            "playing": True
        }

    # Default Mode: Stream live PC Stats and Media to OLED
    run_serial_stream(port=port, interval=args.interval, mock_media=mock_media)


if __name__ == "__main__":
    main()
