<#
.SYNOPSIS
    Removes the DeskOS PC Companion Agent from Windows Startup.
#>
[CmdletBinding()]
param()

$StartupFolder = [System.Environment]::GetFolderPath('Startup')
$ShortcutPath  = Join-Path $StartupFolder "DeskOS-Agent.lnk"

if (Test-Path $ShortcutPath) {
    Remove-Item $ShortcutPath -Force
    Write-Host "[SUCCESS] Removed DeskOS Agent from Windows Startup." -ForegroundColor Green
} else {
    Write-Host "[INFO] DeskOS Agent was not configured in Windows Startup." -ForegroundColor Gray
}
