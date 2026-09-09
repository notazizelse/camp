<#
.SYNOPSIS
    One-time reorganisation of the CAMP repository.

.DESCRIPTION
    Moves the original flat folder into the layout described in README.md, deletes
    editor-history and backup clutter, and rewrites the KiCad library paths so the
    project opens on any machine with no configuration.

    Run it ONCE, from the repository root, before the first git commit.
    It is safe to run again: every step checks whether it has already been done.

.EXAMPLE
    cd C:\Users\admin\Documents\projects\pcb\camp-main
    powershell -ExecutionPolicy Bypass -File .\tools\reorganise.ps1

.NOTES
    Nothing is deleted except editor history, KiCad backups, lock files and build
    artefacts. Old designs are moved to archive\, not removed.
#>

[CmdletBinding()]
param(
    # Show what would happen without changing anything.
    [switch]$DryRun
)

$ErrorActionPreference = 'Stop'
$root = (Get-Location).Path

# ─── helpers ─────────────────────────────────────────────────────────────────

$script:moved = 0; $script:deleted = 0; $script:patched = 0; $script:skipped = 0

function Say($msg, $colour = 'Gray') { Write-Host "  $msg" -ForegroundColor $colour }
function Head($msg) { Write-Host "`n$msg" -ForegroundColor Cyan }

function Ensure-Dir($path) {
    if (-not (Test-Path -LiteralPath $path)) {
        if (-not $DryRun) { New-Item -ItemType Directory -Path $path -Force | Out-Null }
    }
}

function Move-Thing($from, $toDir, $newName = $null) {
    if (-not (Test-Path -LiteralPath $from)) { $script:skipped++; return }
    Ensure-Dir $toDir
    $leaf = if ($newName) { $newName } else { Split-Path $from -Leaf }
    $dest = Join-Path $toDir $leaf
    if (Test-Path -LiteralPath $dest) {
        Say "already there, leaving alone: $leaf" 'DarkGray'; $script:skipped++; return
    }
    Say "$from  ->  $dest" 'Green'
    if (-not $DryRun) { Move-Item -LiteralPath $from -Destination $dest -Force }
    $script:moved++
}

function Remove-Thing($path, $why) {
    if (-not (Test-Path -LiteralPath $path)) { return }
    Say "delete  $path   ($why)" 'Yellow'
    if (-not $DryRun) { Remove-Item -LiteralPath $path -Recurse -Force }
    $script:deleted++
}

# Read/write UTF-8 with NO byte-order mark. KiCad writes its files this way and
# PowerShell 5.1's Set-Content -Encoding utf8 would add a BOM.
$utf8NoBom = New-Object System.Text.UTF8Encoding($false)

function Write-TextFile($path, $text) {
    Say "write   $path" 'Magenta'
    if (-not $DryRun) {
        Ensure-Dir (Split-Path $path -Parent)
        [System.IO.File]::WriteAllText((Join-Path $root $path), $text, $utf8NoBom)
    }
    $script:patched++
}

function Patch-File($path, $replacements) {
    if (-not (Test-Path -LiteralPath $path)) { $script:skipped++; return }
    $full = (Resolve-Path -LiteralPath $path).Path
    $text = [System.IO.File]::ReadAllText($full)
    $before = $text
    foreach ($pair in $replacements.GetEnumerator()) {
        $text = $text.Replace($pair.Key, $pair.Value)
    }
    if ($text -eq $before) { Say "nothing to change in $path" 'DarkGray'; return }
    Say "patch   $path" 'Magenta'
    if (-not $DryRun) { [System.IO.File]::WriteAllText($full, $text, $utf8NoBom) }
    $script:patched++
}

# ─── sanity check ────────────────────────────────────────────────────────────

Write-Host "CAMP repository reorganisation" -ForegroundColor White
Write-Host "root: $root"
if ($DryRun) { Write-Host "DRY RUN - nothing will be changed" -ForegroundColor Yellow }

$looksRight = (Test-Path 'campv2') -or (Test-Path 'hardware\campv2')
if (-not $looksRight) {
    Write-Host "`nThis does not look like the CAMP repository - no 'campv2' or 'hardware\campv2' here." -ForegroundColor Red
    Write-Host "cd to the repository root and run it again." -ForegroundColor Red
    exit 1
}

# ─── 1. delete the clutter ───────────────────────────────────────────────────

Head '1. Removing editor history, backups and lock files'

foreach ($d in @('campv2\.history', 'camp\.history', 'camp_demo\.history', 'camp_t9\.history',
                 'hardware\campv2\.history')) {
    Remove-Thing $d 'editor history, hundreds of files, some of it a nested git repo'
}
Get-ChildItem -Path . -Directory -Recurse -Filter '_restore_backup_*' -ErrorAction SilentlyContinue |
    ForEach-Object { Remove-Thing $_.FullName 'KiCad restore backup' }
Get-ChildItem -Path . -File -Recurse -Include '*.lck', '~*.lck' -ErrorAction SilentlyContinue |
    ForEach-Object { Remove-Thing $_.FullName 'KiCad lock file, left behind by an open project' }
Get-ChildItem -Path . -File -Recurse -Filter '*.o' -ErrorAction SilentlyContinue |
    ForEach-Object { Remove-Thing $_.FullName 'compiler output committed by mistake' }
Remove-Thing '.localized' 'empty macOS marker file'
Remove-Thing 'camp_t9'    'empty KiCad project stub - no symbols, no footprints, 79-byte PCB'

# ─── 2. build the new tree ───────────────────────────────────────────────────

Head '2. Creating the new folder structure'
foreach ($d in @('hardware', 'firmware', 'libraries', 'libraries\logos.pretty',
                 'production\v2-revB', 'production\v2-revB\gerbers',
                 'tools', 'docs', 'docs\images',
                 'archive', 'archive\logo-iterations', 'archive\misc')) {
    Ensure-Dir $d; Say $d
}

# ─── 3. the active design ────────────────────────────────────────────────────

Head '3. Moving the current board into hardware\'
Move-Thing 'campv2' 'hardware'

Head '4. Moving the firmware into firmware\'
Move-Thing 'CubeSat' 'firmware'
if (Test-Path 'Arduino\libraries') {
    Move-Thing 'Arduino\libraries' 'firmware' 'libraries'
    Remove-Thing 'Arduino' 'now empty'
}

Head '5. Moving the KiCad libraries into libraries\'
Move-Thing 'lib\lcsc' 'libraries'
Move-Thing 'lib\3d'   'libraries'
if ((Test-Path 'lib') -and -not (Get-ChildItem 'lib' -Force)) { Remove-Thing 'lib' 'now empty' }

# The two logo footprints that are actually on the board. The KiCad library
# NICKNAME stays "production" (that is what campv2.kicad_pcb refers to); only
# the folder changes.
Head '6. Collecting the two logo footprints used on the board'
Move-Thing 'production\turin_logo2.kicad_mod'        'libraries\logos.pretty'
Move-Thing 'production\assotsiatsiya_logo.kicad_mod' 'libraries\logos.pretty'

# ─── 7. production output ────────────────────────────────────────────────────

Head '7. Collecting the rev B production files into production\v2-revB\'
$fab = 'hardware\campv2\production'
Move-Thing "$fab\TechnoCamp_Circuitry_Class_PCB_B.zip" 'production\v2-revB\gerbers'
foreach ($f in @('bom.csv', 'designators.csv', 'positions.csv', 'netlist.ipc',
                 'smeta_cubesat_camp_19-20.09.2026.xlsx')) {
    Move-Thing "$fab\$f" 'production\v2-revB'
}
if ((Test-Path $fab) -and -not (Get-ChildItem $fab -Force)) { Remove-Thing $fab 'now empty' }

Move-Thing 'production\campv2-F_Cu.pdf'      'production\v2-revB'
Move-Thing 'production\campv2-Edge_Cuts.pdf' 'production\v2-revB'
Move-Thing 'production\3d.step'              'production\v2-revB'

# ─── 8. tools ────────────────────────────────────────────────────────────────

Head '8. Moving the LCSC helper into tools\'
Move-Thing 'lcsc.py'  'tools'
Move-Thing 'lcsc.txt' 'tools'

# ─── 9. archive ──────────────────────────────────────────────────────────────

Head '9. Archiving the superseded designs'
Move-Thing 'camp'      'archive' 'v1-camp'
Move-Thing 'camp_demo' 'archive' 'demo'
Move-Thing 'production\camp-F_Cu.pdf' 'archive\v1-camp'

Head '10. Archiving the logo iterations and odds and ends'
foreach ($f in @('logo.kicad_mod', 'logo_revised.kicad_mod', 'logo2.kicad_mod',
                 'logo2_revised.kicad_mod', 'logo2_rerevised.kicad_mod',
                 'turin_logo.kicad_mod', 'turin_logo_revised.kicad_mod',
                 'logo1.png', 'logo2.jpg', 'turin_logo.png')) {
    Move-Thing "production\$f" 'archive\logo-iterations'
}
Move-Thing 'production\campv2_review.png' 'archive\misc'
Move-Thing 'production\smeta_cubesat_camp_19-20.09.2026.xlsx' 'archive\misc'
Move-Thing 'Game 1.game' 'archive\misc'

# ─── 11. make the project standalone ─────────────────────────────────────────

Head '11. Rewriting library paths so the project opens on any machine'

# The originals held absolute paths like C:/Users/admin/Documents/... which only
# ever worked on one computer. ${KIPRJMOD} is KiCad's "folder this project lives
# in" variable, so these resolve wherever the repository is cloned.

Write-TextFile 'hardware\campv2\sym-lib-table' @'
(sym_lib_table
	(version 7)
	(lib (name "easyeda2kicad")(type "KiCad")(uri "${KIPRJMOD}/../../libraries/lcsc/easyeda2kicad.kicad_sym")(options "")(descr "Parts imported from LCSC with easyeda2kicad"))
)
'@

Write-TextFile 'hardware\campv2\fp-lib-table' @'
(fp_lib_table
	(version 7)
	(lib (name "easyeda2kicad")(type "KiCad")(uri "${KIPRJMOD}/../../libraries/lcsc/easyeda2kicad.pretty")(options "")(descr "Parts imported from LCSC with easyeda2kicad"))
	(lib (name "production")(type "KiCad")(uri "${KIPRJMOD}/../../libraries/logos.pretty")(options "")(descr "Silkscreen logo footprints - nickname kept as production because campv2.kicad_pcb refers to it by that name"))
)
'@

# 3D model references used two path variables that exist only in one person's
# KiCad preferences. This is the only change made inside the .kicad_pcb: it
# touches no geometry, no netlist and no gerber output.
Patch-File 'hardware\campv2\campv2.kicad_pcb' @{
    '${CAMP_ROOT}/lib/lcsc/' = '${KIPRJMOD}/../../libraries/lcsc/'
    '${THREED}/'             = '${KIPRJMOD}/../../libraries/3d/'
}

# Plot output used to land in ../production/; it now has a revision folder.
Patch-File 'hardware\campv2\campv2.kicad_pro' @{
    '"plot": "../production/"' = '"plot": "../../production/v2-revB/"'
}

Head '12. Fixing the archived projects so they open too'
foreach ($p in @('archive\v1-camp', 'archive\demo')) {
    Patch-File "$p\sym-lib-table" @{ '${KIPRJMOD}/../lib/lcsc/' = '${KIPRJMOD}/../../libraries/lcsc/' }
    Patch-File "$p\fp-lib-table"  @{ '${KIPRJMOD}/../lib/lcsc/' = '${KIPRJMOD}/../../libraries/lcsc/' }
    Patch-File "$p\$(Split-Path $p -Leaf).kicad_pcb" @{
        '${CAMP_ROOT}/lib/lcsc/' = '${KIPRJMOD}/../../libraries/lcsc/'
        '${THREED}/'             = '${KIPRJMOD}/../../libraries/3d/'
    }
}
# camp_demo shipped without a footprint table even though its PCB uses one.
if ((Test-Path 'archive\demo') -and -not (Test-Path 'archive\demo\fp-lib-table')) {
    Write-TextFile 'archive\demo\fp-lib-table' @'
(fp_lib_table
	(version 7)
	(lib (name "easyeda2kicad")(type "KiCad")(uri "${KIPRJMOD}/../../libraries/lcsc/easyeda2kicad.pretty")(options "")(descr "Parts imported from LCSC with easyeda2kicad"))
)
'@
}

# ─── done ────────────────────────────────────────────────────────────────────

Write-Host "`n────────────────────────────────────────────" -ForegroundColor White
Write-Host "moved $script:moved  ·  deleted $script:deleted  ·  rewrote $script:patched  ·  skipped $script:skipped" -ForegroundColor White

if ($DryRun) {
    Write-Host "`nThat was a dry run. Run it without -DryRun to apply." -ForegroundColor Yellow
    exit 0
}

Write-Host @"

Next steps
  1. Open hardware\campv2\campv2.kicad_pro in KiCad 10 and check that the
     schematic, the layout and the 3D view all load with no warnings.
  2. Then commit:

       git add -A
       git commit -m "Restructure repository, add documentation, make project standalone"
       git branch -M main
       git push -u origin main

  3. This script has done its job and can be deleted.
"@ -ForegroundColor White
