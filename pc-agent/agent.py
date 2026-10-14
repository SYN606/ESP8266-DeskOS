#!/usr/bin/env python3
"""
ESP32 DeskOS — PC Telemetry Monitoring Agent
Collects local host metrics (CPU, RAM, Uptime, Network, and Temperature when available)
and outputs structured JSON for the DeskOS desktop companion.
"""

import argparse
import json
import sys
import time
from datetime import datetime, timezone
import psutil


def get_cpu_metrics(sample_interval: float = 1.0) -> dict:
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
    
    formatted = f"{days}d {hours:02d}h {minutes:02d}m {seconds:02d}s" if days > 0 else f"{hours:02d}h {minutes:02d}m {seconds:02d}s"
    
    return {
        "boot_time_utc": boot_time.isoformat(),
        "uptime_seconds": uptime_seconds,
        "formatted": formatted
    }


def get_cpu_temperature() -> dict:
    """
    Attempts to read CPU temperature sensors.
    Gracefully handles platforms (like standard Windows) where temperature sensors
    are not exposed via psutil standard APIs.
    """
    result = {
        "supported": False,
        "current_celsius": None,
        "sensors": {}
    }
    
    if not hasattr(psutil, "sensors_temperatures"):
        result["note"] = "sensors_temperatures API not available on this platform"
        return result
        
    try:
        temps = psutil.sensors_temperatures()
        if not temps:
            result["note"] = "No hardware temperature sensors detected"
            return result
            
        result["supported"] = True
        sensor_data = {}
        first_temp = None
        
        for name, entries in temps.items():
            sensor_data[name] = []
            for entry in entries:
                sensor_data[name].append({
                    "label": entry.label or name,
                    "current": entry.current,
                    "high": entry.high,
                    "critical": entry.critical
                })
                if first_temp is None and entry.current is not None:
                    first_temp = entry.current
                    
        result["current_celsius"] = first_temp
        result["sensors"] = sensor_data
        
    except Exception as exc:
        result["note"] = f"Temperature query error: {str(exc)}"
        
    return result


def get_network_throughput(interval: float = 1.0) -> dict:
    """Calculates network throughput over a brief time delta."""
    net1 = psutil.net_io_counters()
    time.sleep(interval)
    net2 = psutil.net_io_counters()
    
    bytes_sent_sec = (net2.bytes_sent - net1.bytes_sent) / interval
    bytes_recv_sec = (net2.bytes_recv - net1.bytes_recv) / interval
    
    return {
        "bytes_sent_per_sec": round(bytes_sent_sec, 1),
        "bytes_recv_per_sec": round(bytes_recv_sec, 1),
        "kb_sent_per_sec": round(bytes_sent_sec / 1024.0, 1),
        "kb_recv_per_sec": round(bytes_recv_sec / 1024.0, 1),
        "total_bytes_sent": net2.bytes_sent,
        "total_bytes_recv": net2.bytes_recv
    }


def collect_snapshot(sample_interval: float = 0.5) -> dict:
    """Aggregates a complete telemetry snapshot."""
    # We sample CPU first with interval, then network
    cpu = get_cpu_metrics(sample_interval=sample_interval)
    memory = get_memory_metrics()
    uptime = get_system_uptime()
    temp = get_cpu_temperature()
    net = get_network_throughput(interval=sample_interval)
    
    return {
        "schema": "esp32-deskos-telemetry-v1",
        "timestamp_utc": datetime.now(timezone.utc).isoformat(),
        "cpu": cpu,
        "memory": memory,
        "uptime": uptime,
        "temperature": temp,
        "network": net
    }


def main():
    parser = argparse.ArgumentParser(description="ESP32 DeskOS PC Telemetry Agent")
    parser.add_argument("--pretty", action="store_true", help="Format JSON with indentation", default=True)
    parser.add_argument("--compact", action="store_true", help="Output compact single-line JSON")
    parser.add_argument("--loop", action="store_true", help="Continuously output telemetry stream")
    parser.add_argument("--interval", type=float, default=2.0, help="Interval between loops in seconds")
    parser.add_argument("--serial", type=str, default=None, help="COM port to stream live telemetry to (e.g. COM3)")
    parser.add_argument("--url", type=str, default=None, help="HTTP URL to POST live telemetry to (e.g. http://192.168.1.50/api/stats)")
    args = parser.parse_args()

    indent = None if args.compact else 2

    # Mode 1: Stream to Serial Port
    if args.serial:
        import serial
        print(f"[BRIDGE] Connecting to {args.serial} at 115200 baud...")
        try:
            ser = serial.Serial(args.serial, 115200, timeout=1)
            time.sleep(1.0)
            print(f"[BRIDGE] Connected! Streaming live telemetry to DeskOS (Ctrl+C to stop)...")
            while True:
                snap = collect_snapshot(sample_interval=0.5)
                payload = {
                    "cmd": "stats",
                    "cpu": round(snap["cpu"]["overall_percent"], 1),
                    "ram": round(snap["memory"]["percent_used"], 1),
                    "uptime": snap["uptime"]["formatted"],
                    "net_in": snap["network"]["kb_recv_per_sec"],
                    "net_out": snap["network"]["kb_sent_per_sec"],
                    "auto_show": True
                }
                msg = json.dumps(payload) + "\n"
                ser.write(msg.encode("utf-8"))
                ser.flush()
                print(f"[STREAM] CPU: {payload['cpu']}% | RAM: {payload['ram']}% | Net: {payload['net_in']} KB/s", flush=True)
                time.sleep(max(0.5, args.interval - 0.5))
        except KeyboardInterrupt:
            print("\n[BRIDGE] Stopped by user.")
            sys.exit(0)
        except Exception as err:
            print(f"[BRIDGE ERROR] {err}")
            sys.exit(1)

    # Mode 2: Stream over HTTP
    if args.url:
        import urllib.request
        print(f"[HTTP BRIDGE] Streaming to {args.url} (Ctrl+C to stop)...")
        try:
            while True:
                snap = collect_snapshot(sample_interval=0.5)
                payload = {
                    "cpu": round(snap["cpu"]["overall_percent"], 1),
                    "ram": round(snap["memory"]["percent_used"], 1),
                    "uptime": snap["uptime"]["formatted"],
                    "net_in": snap["network"]["kb_recv_per_sec"],
                    "net_out": snap["network"]["kb_sent_per_sec"]
                }
                req = urllib.request.Request(
                    args.url,
                    data=json.dumps(payload).encode("utf-8"),
                    headers={"Content-Type": "application/json"}
                )
                try:
                    with urllib.request.urlopen(req, timeout=3) as resp:
                        pass
                except Exception as ex:
                    print(f"[HTTP] Post warning: {ex}")
                print(f"[STREAM] CPU: {payload['cpu']}% | RAM: {payload['ram']}%", flush=True)
                time.sleep(max(0.5, args.interval - 0.5))
        except KeyboardInterrupt:
            print("\n[HTTP BRIDGE] Stopped by user.")
            sys.exit(0)

    # Default Mode: Output JSON snapshot or loop
    try:
        if args.loop:
            while True:
                snapshot = collect_snapshot(sample_interval=0.5)
                print(json.dumps(snapshot, indent=indent), flush=True)
                time.sleep(max(0.1, args.interval - 1.0))
        else:
            snapshot = collect_snapshot(sample_interval=0.5)
            print(json.dumps(snapshot, indent=indent))
    except KeyboardInterrupt:
        sys.exit(0)


if __name__ == "__main__":
    main()
