<#
.SYNOPSIS
    Starts the PC Telemetry Agent and streams live CPU/RAM stats to the DeskOS OLED.
.DESCRIPTION
    Auto-detects the connected COM port and streams PC telemetry metrics.
.PARAMETER ComPort
    Target serial port (e.g. COM3). Auto-detected if omitted.
.PARAMETER Interval
    Update interval in seconds (default: 2.0).
.EXAMPLE
    .\scripts\agent.ps1
    .\scripts\agent.ps1 -Interval 1.0
#>
[CmdletBinding()]
param(
    [Parameter(Position = 0, Mandatory = $false)]
    [string]$ComPort,

    [float]$Interval = 2.0
)

$ErrorActionPreference = 'Stop'

Write-Host "==========================================" -ForegroundColor Cyan
Write-Host "  ESP32 DeskOS -- PC Telemetry Bridge     " -ForegroundColor Cyan
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

Write-Host "[BRIDGE] Launching agent on $ComPort (interval: ${Interval}s)..." -ForegroundColor Yellow
Write-Host "[INFO] Press Ctrl+C to stop streaming." -ForegroundColor Gray
Write-Host ""

& uv run python pc-agent/agent.py --serial $ComPort --interval $Interval
