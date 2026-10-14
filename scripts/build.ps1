<#
.SYNOPSIS
    Builds the DeskOS firmware using PlatformIO Core via uv.
.DESCRIPTION
    Compiles C++ source files, resolves libraries, and generates the firmware binary.
.PARAMETER Env
    Target environment (default: nodemcuv2 for ESP8266; esp32dev for ESP32).
.PARAMETER Clean
    Perform a clean build by deleting cached object files first.
.EXAMPLE
    .\scripts\build.ps1
    .\scripts\build.ps1 -Env esp32dev
    .\scripts\build.ps1 -Clean
#>
[CmdletBinding()]
param(
    [Parameter(Position = 0, Mandatory = $false)]
    [ValidateSet('nodemcuv2', 'esp32dev')]
    [string]$Env = 'nodemcuv2',

    [switch]$Clean
)

$ErrorActionPreference = 'Stop'

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  ESP32 DeskOS -- Firmware Build Tool   " -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = Split-Path -Parent $ScriptDir
Set-Location -Path $ProjectRoot

Write-Host "[CONFIG] Target Environment : $Env" -ForegroundColor Yellow
Write-Host "[CONFIG] Working Directory  : $ProjectRoot" -ForegroundColor Gray

if ($Clean) {
    Write-Host "[BUILD] Cleaning previous build artifacts..." -ForegroundColor Yellow
    & uv run pio run -e $Env --target clean
}

Write-Host "[BUILD] Compiling firmware for '$Env'..." -ForegroundColor Yellow
$startTime = Get-Date

& uv run pio run -e $Env

$duration = ((Get-Date) - $startTime).TotalSeconds

if ($LASTEXITCODE -eq 0) {
    Write-Host ""
    Write-Host ("[BUILD] SUCCESS: Firmware compiled in {0:N1}s." -f $duration) -ForegroundColor Green
    Write-Host "Binary ready in: .pio\build\$Env\firmware.bin" -ForegroundColor Gray
    Write-Host "To flash, run: .\scripts\upload.ps1" -ForegroundColor Cyan
} else {
    Write-Host ""
    Write-Host "[BUILD] ERROR: Compilation failed with exit code $LASTEXITCODE." -ForegroundColor Red
    exit $LASTEXITCODE
}
