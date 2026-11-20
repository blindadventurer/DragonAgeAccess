# Builds the mod and copies it into the game on this PC. Refuses while the game runs.
# Game folder changes are listed in games\<game>\NOTEBOOK.md. Testers use the
# installer instead (make_release.ps1).
# Usage: .\deploy.ps1 -Game dao|da2|all
param([string]$Game = 'all')
$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
. "$root\tools\games.ps1"

# The games' bin_ship folders on the player's PC.
$GamePaths = @{
    dao = 'F:\SteamLibrary\steamapps\common\Dragon Age Ultimate Edition\bin_ship'
    da2 = 'F:\SteamLibrary\steamapps\common\Dragon Age II\bin_ship'
}

$selected = Select-Games $Game
foreach ($g in $selected) {
    $running = Get-Process -ErrorAction SilentlyContinue | Where-Object { $_.Name -in $Games[$g].Processes }
    if ($running) { throw "The game is running ($($running.Name -join ', ')). Close it first." }
}

& "$root\build.ps1" -Game $Game

foreach ($g in $selected) {
    $info = $Games[$g]
    $game = $GamePaths[$g]
    $mod = $info.Mod
    # The game's official override folder (resources placed here add to the game's).
    $override = Join-Path ([Environment]::GetFolderPath('MyDocuments')) "$($info.Docs)\packages\core\override"
    $files = @(
        @{ From = "$root\vendor\ual-9.7.4\dinput8.dll"; To = "$game\dinput8.dll" },
        @{ From = "$root\build\$g\$mod.asi";            To = "$game\scripts\$mod.asi" },
        @{ From = "$root\build\$g\$($info.Agent)";      To = "$game\scripts\$mod\$($info.Agent)" },
        @{ From = "$root\build\$g\$($info.Agent)";      To = "$override\$($info.Agent)" },
        @{ From = "$root\lib\prism-x86\prism.dll";      To = "$game\scripts\$mod\prism.dll" },
        @{ From = "$root\lib\prism\third-party\prism\mpl-2.0.txt"; To = "$game\scripts\$mod\third-party\prism\mpl-2.0.txt" },
        @{ From = "$root\lib\prism\third-party\prism\NOTICE";      To = "$game\scripts\$mod\third-party\prism\NOTICE" }
    )
    # Loader settings a game needs (games\<game>\loader\global.ini, Dragon Age II only).
    $ini = "$root\games\$g\loader\global.ini"
    if (Test-Path $ini) { $files += @{ From = $ini; To = "$game\scripts\global.ini" } }
    # Exit watcher (Dragon Age II only, see build.ps1).
    $watch = "$root\build\$g\$($mod)Watch.exe"
    if (Test-Path $watch) { $files += @{ From = $watch; To = "$game\scripts\$mod\$($mod)Watch.exe" } }
    foreach ($f in $files) {
        New-Item -ItemType Directory -Force (Split-Path $f.To) | Out-Null
        if ((Test-Path $f.To) -and ($f.To -like '*\dinput8.dll')) {
            $same = (Get-FileHash $f.From).Hash -eq (Get-FileHash $f.To).Hash
            if (-not $same) { throw "$($f.To) exists and is not our loader. Not overwriting." }
        }
        Copy-Item $f.From $f.To -Force
        Write-Host "Copied $($f.From) -> $($f.To)"
    }
}
