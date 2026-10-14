<#
.SYNOPSIS
    Opens the interactive PlatformIO Serial Monitor.
.DESCRIPTION
    Auto-detects the connected COM port (or prompts if multiple exist)
    and connects at 115200 baud to view live DeskOS logs.
.PARAMETER ComPort
    Target serial port (e.g. COM3). Auto-detected if omitted and only one device exists.
.PARAMETER Baud
    Baud rate (default: 115200).
.EXAMPLE
    .\scripts\monitor.ps1
    .\scripts\monitor.ps1 -ComPort COM3
#>
[CmdletBinding()]
param(
    [Parameter(Position = 0, Mandatory = $false)]
    [string]$ComPort,

    [int]$Baud = 115200
)

$ErrorActionPreference = 'Stop'

Write-Host "==========================================" -ForegroundColor Cyan
Write-Host "  ESP32 DeskOS -- Serial Monitor ($Baud)  " -ForegroundColor Cyan
Write-Host "==========================================" -ForegroundColor Cyan

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = Split-Path -Parent $ScriptDir
Set-Location -Path $ProjectRoot

# Detect connected ports
$availablePorts = [System.IO.Ports.SerialPort]::GetPortNames()

if (-not $ComPort) {
    if ($availablePorts.Count -eq 0) {
        Write-Host ""
        Write-Host "[ERROR] No serial COM ports detected." -ForegroundColor Red
        exit 1
    } elseif ($availablePorts.Count -eq 1) {
        $ComPort = $availablePorts[0]
        Write-Host "[PORT] Auto-detected single connected board on $ComPort" -ForegroundColor Green
    } else {
        Write-Host ""
        Write-Host "[PORT] Multiple serial ports detected:" -ForegroundColor Yellow
        for ($i = 0; $i -lt $availablePorts.Count; $i++) {
            Write-Host "  [$($i+1)] $($availablePorts[$i])" -ForegroundColor Cyan
        }
        $choice = Read-Host "Select port number (1-$($availablePorts.Count))"
        $idx = [int]$choice - 1
        if ($idx -ge 0 -and $idx -lt $availablePorts.Count) {
            $ComPort = $availablePorts[$idx]
        } else {
            Write-Host "[ERROR] Invalid selection." -ForegroundColor Red
            exit 1
        }
    }
}

Write-Host "[MONITOR] Connecting to $ComPort at $Baud baud..." -ForegroundColor Yellow
Write-Host "[SHORTCUTS] In monitor: 'e'=eyes, 's'=stats, 'c'=clock, 'm'=msg, 'b'=blink" -ForegroundColor Gray
Write-Host "[EXIT] Press Ctrl+C or Ctrl+] to disconnect." -ForegroundColor Cyan
Write-Host ""

& uv run pio device monitor --baud $Baud --port $ComPort
