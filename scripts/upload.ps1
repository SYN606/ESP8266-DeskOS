<#
.SYNOPSIS
    Flashes DeskOS firmware to the connected ESP board.
.DESCRIPTION
    Auto-detects connected COM ports (prompting if multiple exist).
    Validates hardware and uploads firmware using PlatformIO Core.
.PARAMETER ComPort
    Target serial port (e.g. COM3). Auto-detected if omitted and only one device is connected.
.PARAMETER Env
    PlatformIO target environment: 'nodemcuv2' (default) or 'esp32dev'.
.PARAMETER Yes
    Skip confirmation prompts.
.EXAMPLE
    .\scripts\upload.ps1
    .\scripts\upload.ps1 -ComPort COM3
    .\scripts\upload.ps1 -Env esp32dev -ComPort COM4 -Yes
#>
[CmdletBinding()]
param(
    [Parameter(Position = 0, Mandatory = $false)]
    [string]$ComPort,

    [ValidateSet('nodemcuv2', 'esp32dev')]
    [string]$Env = 'nodemcuv2',

    [Alias('Force', 'y')]
    [switch]$Yes
)

$ErrorActionPreference = 'Stop'

Write-Host "=========================================" -ForegroundColor Cyan
Write-Host "  ESP32 DeskOS -- Firmware Upload Tool   " -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = Split-Path -Parent $ScriptDir
Set-Location -Path $ProjectRoot

# Detect connected ports
$availablePorts = [System.IO.Ports.SerialPort]::GetPortNames()

if (-not $ComPort) {
    if ($availablePorts.Count -eq 0) {
        Write-Host ""
        Write-Host "[ERROR] No serial COM ports detected." -ForegroundColor Red
        Write-Host "Please verify:" -ForegroundColor Yellow
        Write-Host "  1. Your ESP board is plugged into USB." -ForegroundColor Gray
        Write-Host "  2. USB cable supports data (not charge-only)." -ForegroundColor Gray
        Write-Host "  3. CP210x or CH340 USB drivers are installed." -ForegroundColor Gray
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

# Validate format
if ($ComPort -notmatch '^COM\d+$') {
    Write-Host "[ERROR] Invalid COM port format: '$ComPort'. Expected COM1, COM3, etc." -ForegroundColor Red
    exit 1
}

Write-Host "[CONFIG] Target Board       : $Env" -ForegroundColor Yellow
Write-Host "[CONFIG] Target Serial Port : $ComPort" -ForegroundColor Yellow

if (-not $Yes) {
    $confirm = Read-Host "`nReady to flash firmware to $ComPort? [Y/n]"
    if ($confirm -eq 'n' -or $confirm -eq 'N') {
        Write-Host "[ABORTED] Flashing cancelled by user." -ForegroundColor Yellow
        exit 0
    }
}

Write-Host ""
Write-Host "[UPLOAD] Flashing firmware to $ComPort (env: $Env)..." -ForegroundColor Yellow
$startTime = Get-Date

& uv run pio run -e $Env --target upload --upload-port $ComPort

$duration = ((Get-Date) - $startTime).TotalSeconds

if ($LASTEXITCODE -eq 0) {
    Write-Host ""
    Write-Host ("[UPLOAD] SUCCESS: Firmware flashed in {0:N1}s!" -f $duration) -ForegroundColor Green
    Write-Host "To view live logs, run: .\scripts\monitor.ps1" -ForegroundColor Cyan
} else {
    Write-Host ""
    Write-Host "[UPLOAD] ERROR: Flashing failed with exit code $LASTEXITCODE." -ForegroundColor Red
    exit $LASTEXITCODE
}
