<#
.SYNOPSIS
    Configures the DeskOS PC Companion Agent to start automatically on Windows login.
#>
[CmdletBinding()]
param()

$ScriptDir     = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot   = Split-Path -Parent $ScriptDir
$StartupFolder = [System.Environment]::GetFolderPath('Startup')
$ShortcutPath  = Join-Path $StartupFolder "DeskOS-Agent.lnk"
$PythonW       = Join-Path $ProjectRoot ".venv\Scripts\pythonw.exe"
$AgentPy       = Join-Path $ProjectRoot "pc-agent\agent.py"

if (-not (Test-Path $PythonW)) {
    Write-Host "[ERROR] pythonw.exe not found in .venv\Scripts. Please run: uv sync" -ForegroundColor Red
    exit 1
}

Write-Host "==========================================" -ForegroundColor Cyan
Write-Host "  DeskOS -- Auto-Start Configuration      " -ForegroundColor Cyan
Write-Host "==========================================" -ForegroundColor Cyan

$WshShell = New-Object -ComObject WScript.Shell
$Shortcut = $WshShell.CreateShortcut($ShortcutPath)
$Shortcut.TargetPath       = $PythonW
$Shortcut.Arguments        = "-u `"$AgentPy`""
$Shortcut.WorkingDirectory = $ProjectRoot
$Shortcut.Description      = "DeskOS PC Telemetry Background Agent"
$Shortcut.Save()

Write-Host "[SUCCESS] Auto-start shortcut created successfully!" -ForegroundColor Green
Write-Host "[INFO] DeskOS Agent will now automatically launch silently in the background when you log in." -ForegroundColor Cyan
Write-Host "[LOCATION] $ShortcutPath" -ForegroundColor Gray
Write-Host ""
Write-Host "To remove auto-start later, run: .\scripts\uninstall-startup.ps1" -ForegroundColor Yellow
