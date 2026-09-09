<#
.SYNOPSIS
    Deletes editor history, KiCad backups and build artefacts from the repository.

.DESCRIPTION
    The folder layout is already set. This script only removes clutter that should
    never be committed: the .history sidecars written by the VS Code Local History
    extension and the KiCad history plugin (around 550 files, one of which is a
    nested git repository), KiCad restore backups and lock files, and stray .o files.

    Everything it deletes is also listed in .gitignore, so if you skip this script
    nothing broken gets committed - the files just sit on your disk.

.EXAMPLE
    cd C:\Users\admin\Documents\projects\pcb\camp-main
    powershell -ExecutionPolicy Bypass -File .\tools\cleanup.ps1 -DryRun
    powershell -ExecutionPolicy Bypass -File .\tools\cleanup.ps1
#>

[CmdletBinding()]
param([switch]$DryRun)

$ErrorActionPreference = 'Stop'
$removed = 0; $bytes = 0

if (-not (Test-Path 'campv2\campv2.kicad_pro')) {
    Write-Host "Run this from the repository root (the folder containing campv2\)." -ForegroundColor Red
    exit 1
}
if ($DryRun) { Write-Host "DRY RUN - nothing will be deleted" -ForegroundColor Yellow }

function Drop($path, $why) {
    if (-not (Test-Path -LiteralPath $path)) { return }
    $size = (Get-ChildItem -LiteralPath $path -Recurse -Force -File -ErrorAction SilentlyContinue |
             Measure-Object -Property Length -Sum).Sum
    if (-not $size) { $size = 0 }
    Write-Host ("  {0,-46} {1,8:N0} KB   {2}" -f $path, ($size/1KB), $why) -ForegroundColor Yellow
    if (-not $DryRun) { Remove-Item -LiteralPath $path -Recurse -Force }
    $script:removed++; $script:bytes += $size
}

Write-Host "`nEditor history" -ForegroundColor Cyan
Get-ChildItem -Path . -Directory -Recurse -Force -Filter '.history' -ErrorAction SilentlyContinue |
    ForEach-Object { Drop $_.FullName 'editor history sidecar' }

Write-Host "`nKiCad backups, lock files and autosaves" -ForegroundColor Cyan
Get-ChildItem -Path . -Directory -Recurse -Force -Filter '_restore_backup_*' -ErrorAction SilentlyContinue |
    ForEach-Object { Drop $_.FullName 'KiCad restore backup' }
Get-ChildItem -Path . -Directory -Recurse -Force -Filter '*-backups' -ErrorAction SilentlyContinue |
    ForEach-Object { Drop $_.FullName 'KiCad automatic backups' }
Get-ChildItem -Path . -File -Recurse -Force -Include '*.lck', '~*.lck', '_autosave-*' -ErrorAction SilentlyContinue |
    ForEach-Object { Drop $_.FullName 'KiCad lock / autosave' }

Write-Host "`nBuild artefacts" -ForegroundColor Cyan
Get-ChildItem -Path . -File -Recurse -Force -Filter '*.o' -ErrorAction SilentlyContinue |
    ForEach-Object { Drop $_.FullName 'compiler output' }
Drop '.localized' 'empty macOS marker file'

Write-Host ("`nRemoved {0} items, {1:N1} MB`n" -f $removed, ($bytes/1MB)) -ForegroundColor White
if ($DryRun) { Write-Host "That was a dry run. Run it again without -DryRun to apply." -ForegroundColor Yellow }
