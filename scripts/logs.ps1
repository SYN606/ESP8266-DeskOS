<#
.SYNOPSIS
    Tails the DeskOS PC Telemetry Agent background log file in real time.
#>
[CmdletBinding()]
param()

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$AgentPs1  = Join-Path $ScriptDir "agent.ps1"

& $AgentPs1 -Logs
