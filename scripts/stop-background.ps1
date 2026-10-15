<#
.SYNOPSIS
    Stops the background DeskOS PC Telemetry Agent.
#>
[CmdletBinding()]
param()

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$AgentPs1  = Join-Path $ScriptDir "agent.ps1"

& $AgentPs1 -Stop
