<#
.SYNOPSIS
    Manages the DeskOS PC Telemetry Companion Agent (Foreground or Background).
.DESCRIPTION
    Can launch the agent in foreground, start as an invisible background process,
    stop a running background process, check live status, and tail logs.
.PARAMETER Background
    Launch agent invisibly in the background using pythonw.exe.
.PARAMETER Stop
    Stop any running background agent.
.PARAMETER Restart
    Restart the background agent.
.PARAMETER Status
    Check whether the background agent is running and display recent logs.
.PARAMETER Logs
    Tail the background log file in real-time.
.PARAMETER ComPort
    Target serial port (e.g. COM3). Auto-detected if omitted.
.PARAMETER Interval
    Update interval in seconds (default: 1.0).
.PARAMETER TestMedia
    Stream mock media track for visual testing.
.EXAMPLE
    .\scripts\agent.ps1 -Background
    .\scripts\agent.ps1 -Status
    .\scripts\agent.ps1 -Stop
    .\scripts\agent.ps1 -Logs
#>
[CmdletBinding()]
param(
    [Parameter(Position = 0, Mandatory = $false)]
    [string]$ComPort,

    [switch]$Background,
    [switch]$Stop,
    [switch]$Restart,
    [switch]$Status,
    [switch]$Logs,
    [switch]$TestMedia,
    [switch]$ShowStream,

    [float]$Interval = 1.0
)

$ErrorActionPreference = 'Stop'

$ScriptDir   = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = Split-Path -Parent $ScriptDir
$PidFile     = Join-Path $ProjectRoot ".agent.pid"
$LogsDir     = Join-Path $ProjectRoot "logs"
$LogFile     = Join-Path $LogsDir "agent.log"
$ErrLogFile  = Join-Path $LogsDir "agent.err.log"
$PythonW     = Join-Path $ProjectRoot ".venv\Scripts\pythonw.exe"
$Python      = Join-Path $ProjectRoot ".venv\Scripts\python.exe"
$AgentScript = Join-Path $ProjectRoot "pc-agent\agent.py"

if (-not (Test-Path $LogsDir)) {
    New-Item -ItemType Directory -Path $LogsDir -Force | Out-Null
}

function Get-RunningAgentProcess {
    if (Test-Path $PidFile) {
        try {
            $savedPid = Get-Content $PidFile -ErrorAction SilentlyContinue
            if ($savedPid) {
                $proc = Get-Process -Id ([int]$savedPid.Trim()) -ErrorAction SilentlyContinue
                if ($proc -and ($proc.ProcessName -like "*python*") -and ($proc.HasExited -eq $false)) {
                    return $proc
                } else {
                    Remove-Item $PidFile -Force -ErrorAction SilentlyContinue
                }
            }
        } catch {
            Remove-Item $PidFile -Force -ErrorAction SilentlyContinue
        }
    }
    # Fallback search by python process command line
    try {
        $wmiProcs = @(Get-CimInstance Win32_Process | Where-Object { 
            ($_.Name -like "*python*") -and ($_.CommandLine -like "*agent.py*") -and ($_.ProcessId -ne $PID)
        })
        $candidates = @()
        foreach ($wp in $wmiProcs) {
            $p = Get-Process -Id $wp.ProcessId -ErrorAction SilentlyContinue
            if ($p -and ($p.HasExited -eq $false)) {
                $candidates += $p
            }
        }
        if ($candidates.Count -gt 0) {
            $best = $candidates | Sort-Object WorkingSet64 -Descending | Select-Object -First 1
            if ($best) {
                $best.Id | Out-File -FilePath $PidFile -Encoding ascii -Force
                return $best
            }
        }
    } catch {}
    return $null
}

# --- Action: Logs ---
if ($Logs) {
    if (-not (Test-Path $LogFile)) {
        Write-Host "[LOGS] No log file found at: $LogFile" -ForegroundColor Yellow
        exit 0
    }
    Write-Host "==========================================" -ForegroundColor Cyan
    Write-Host "  DeskOS Agent Live Log (Ctrl+C to exit)  " -ForegroundColor Cyan
    Write-Host "==========================================" -ForegroundColor Cyan
    Get-Content $LogFile -Wait -Tail 30
    exit 0
}

# --- Action: Stop ---
if ($Stop -or $Restart) {
    $stoppedAny = $false
    if (Test-Path $PidFile) {
        try {
            $savedPid = Get-Content $PidFile -ErrorAction SilentlyContinue
            if ($savedPid) {
                $p = Get-Process -Id ([int]$savedPid.Trim()) -ErrorAction SilentlyContinue
                if ($p) {
                    Write-Host "[STOP] Stopping DeskOS Agent (PID: $($p.Id))..." -ForegroundColor Yellow
                    Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue
                    $stoppedAny = $true
                }
            }
        } catch {}
        Remove-Item $PidFile -Force -ErrorAction SilentlyContinue
    }

    # Terminate any remaining python/uv processes executing agent.py
    $wmiProcs = Get-CimInstance Win32_Process | Where-Object { 
        $_.CommandLine -like "*agent.py*" -and $_.ProcessId -ne $PID 
    }
    foreach ($wp in $wmiProcs) {
        try {
            Write-Host "[STOP] Stopping agent process (PID: $($wp.ProcessId))..." -ForegroundColor Yellow
            Stop-Process -Id $wp.ProcessId -Force -ErrorAction SilentlyContinue
            $stoppedAny = $true
        } catch {}
    }

    if ($stoppedAny) {
        Start-Sleep -Milliseconds 500
        Write-Host "[STOP] All agent processes stopped successfully." -ForegroundColor Green
    } else {
        Write-Host "[INFO] No background agent was running." -ForegroundColor Gray
    }

    if (-not $Restart) { exit 0 }
}

# --- Action: Status ---
if ($Status) {
    Write-Host "==========================================" -ForegroundColor Cyan
    Write-Host "  DeskOS Background Agent Status          " -ForegroundColor Cyan
    Write-Host "==========================================" -ForegroundColor Cyan
    $proc = Get-RunningAgentProcess
    if ($proc) {
        $memMb = [math]::Round($proc.WorkingSet64 / 1MB, 1)
        Write-Host "[STATUS] State       : RUNNING (Background)" -ForegroundColor Green
        Write-Host "[STATUS] Process ID  : $($proc.Id)" -ForegroundColor Cyan
        Write-Host "[STATUS] Memory Usage: ${memMb} MB" -ForegroundColor Cyan
        Write-Host "[STATUS] Start Time  : $($proc.StartTime.ToString('yyyy-MM-dd HH:mm:ss'))" -ForegroundColor Cyan
        Write-Host "[STATUS] Log File    : $LogFile" -ForegroundColor Gray
        Write-Host ""
        if (Test-Path $LogFile) {
            Write-Host "--- Recent Log Output (Last 6 Lines) ---" -ForegroundColor Yellow
            Get-Content $LogFile -Tail 6
        }
    } else {
        Write-Host "[STATUS] State       : STOPPED (Not Running)" -ForegroundColor Red
        if (Test-Path $LogFile) {
            Write-Host ""
            Write-Host "--- Last Recorded Log Lines ---" -ForegroundColor Gray
            Get-Content $LogFile -Tail 4
        }
    }
    exit 0
}

# Check if already running when starting
$existing = Get-RunningAgentProcess
if ($existing) {
    Write-Host "[WARNING] An agent is already running (PID: $($existing.Id))." -ForegroundColor Yellow
    Write-Host "To stop it, run: .\scripts\agent.ps1 -Stop" -ForegroundColor Cyan
    Write-Host "To restart, run: .\scripts\agent.ps1 -Restart" -ForegroundColor Cyan
    exit 0
}

# --- Auto-detect Port if not given ---
if (-not $ComPort) {
    $availablePorts = [System.IO.Ports.SerialPort]::GetPortNames()
    if ($availablePorts.Count -eq 0) {
        Write-Host "[ERROR] No serial COM ports detected! Connect your board via USB." -ForegroundColor Red
        exit 1
    } elseif ($availablePorts.Count -eq 1) {
        $ComPort = $availablePorts[0]
    } else {
        # Prefer Silicon Labs / USB Serial if multiple
        $ComPort = $availablePorts[0]
    }
}

# --- Action: Background Start ---
if ($Background -or $Restart) {
    if (-not $Restart) {
        $existing = Get-RunningAgentProcess
        if ($existing) {
            Write-Host "==========================================" -ForegroundColor Cyan
            Write-Host "  DeskOS Agent Status                     " -ForegroundColor Cyan
            Write-Host "==========================================" -ForegroundColor Cyan
            Write-Host "[INFO] DeskOS Agent is ALREADY running in background (PID: $($existing.Id))." -ForegroundColor Yellow
            Write-Host ""
            Write-Host "Commands:" -ForegroundColor Gray
            Write-Host "  View Status : .\scripts\agent.ps1 -Status   (or DeskOS.bat option 3)"
            Write-Host "  Stream Logs : .\scripts\agent.ps1 -Logs     (or DeskOS.bat option 5)"
            Write-Host "  Restart     : .\scripts\agent.ps1 -Restart"
            Write-Host "  Stop Agent  : .\scripts\agent.ps1 -Stop     (or DeskOS.bat option 4)"
            Write-Host "==========================================" -ForegroundColor Cyan
            exit 0
        }
    }

    Write-Host "==========================================" -ForegroundColor Cyan
    Write-Host "  Starting DeskOS Agent in Background     " -ForegroundColor Cyan
    Write-Host "==========================================" -ForegroundColor Cyan
    Write-Host "[PORT] Port: $ComPort | Interval: ${Interval}s" -ForegroundColor Gray
    Write-Host "[LOGS] Output redirected to: $LogFile" -ForegroundColor Gray

    if (Test-Path $PidFile) {
        Remove-Item $PidFile -Force -ErrorAction SilentlyContinue
    }

    $agentArgs = @("run", "python", "-u", $AgentScript, "--serial", $ComPort, "--interval", [string]$Interval, "--log-file", $LogFile)
    if ($TestMedia) { $agentArgs += "--test-media" }

    $proc = Start-Process -FilePath "uv" `
                          -ArgumentList $agentArgs `
                          -WorkingDirectory $ProjectRoot `
                          -WindowStyle Hidden `
                          -PassThru

    Start-Sleep -Milliseconds 1500
    $actualProc = Get-RunningAgentProcess
    if ($actualProc) {
        $actualProc.Id | Out-File -FilePath $PidFile -Encoding ascii -Force
        Write-Host "[SUCCESS] Agent running in background (PID: $($actualProc.Id))!" -ForegroundColor Green
        Write-Host ""
        Write-Host "Commands:" -ForegroundColor Yellow
        Write-Host "  View Status : .\scripts\agent.ps1 -Status   (or DeskOS.bat option 3)"
        Write-Host "  Stream Logs : .\scripts\agent.ps1 -Logs     (or DeskOS.bat option 5)"
        Write-Host "  Stop Agent  : .\scripts\agent.ps1 -Stop     (or DeskOS.bat option 4)"
        Write-Host "==========================================" -ForegroundColor Cyan
    } else {
        Write-Host "[ERROR] Agent process failed to start." -ForegroundColor Red
        if (Test-Path $LogFile)    { Get-Content $LogFile -Tail 10 }
        if (Test-Path $ErrLogFile) { Get-Content $ErrLogFile -Tail 10 }
        exit 1
    }
    exit 0
}

# --- Foreground Execution (Default if no switch passed) ---
$fgArgs = @($AgentScript, "--serial", $ComPort, "--interval", [string]$Interval)
if ($TestMedia) { $fgArgs += "--test-media" }
if ($ShowStream -or $PSBoundParameters['Verbose']) { $fgArgs += "--verbose" }

& uv run python @fgArgs
