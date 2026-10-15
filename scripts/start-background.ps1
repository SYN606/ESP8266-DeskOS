<#
.SYNOPSIS
    Starts the DeskOS PC Telemetry Agent invisibly in the background.
#>
[CmdletBinding()]
param(
    [string]$ComPort,
    [float]$Interval = 1.0,
    [switch]$TestMedia
)

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$AgentPs1  = Join-Path $ScriptDir "agent.ps1"

$params = @{
    Background = $true
    Interval   = $Interval
}
if ($ComPort)   { $params["ComPort"] = $ComPort }
if ($TestMedia) { $params["TestMedia"] = $true }

& $AgentPs1 @params
