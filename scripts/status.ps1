<#
.SYNOPSIS
    Displays the current status, resource usage, and recent logs of the DeskOS Agent.
#>
[CmdletBinding()]
param()

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$AgentPs1  = Join-Path $ScriptDir "agent.ps1"

& $AgentPs1 -Status
