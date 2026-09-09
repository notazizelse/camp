<#
    OBSOLETE - do not run this.

    This script moved the original flat download into a proposed layout
    (hardware\, firmware\, libraries\, production\v2-revB\). That layout was
    never adopted: the repository was reorganised by hand into the structure
    documented in README.md, where campv2\ holds the board, its firmware and
    its fab package together.

    Running it now would scatter the current folders and break the library
    paths. It is kept only so nobody wonders where it went.

    For the cleanup it also used to do - deleting .history sidecars, KiCad
    backups and lock files - use tools\cleanup.ps1 instead.
#>

Write-Host "reorganise.ps1 is obsolete - the repository is already organised." -ForegroundColor Yellow
Write-Host "Use tools\cleanup.ps1 to remove editor history and KiCad backups." -ForegroundColor Yellow
exit 1
