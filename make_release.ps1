# Builds both mods and packs the release files into release\:
#   <Mod>-<version>.zip  for DAOAccess (Origins) and DA2Access (Dragon Age II):
#     bin_ship  - extract into the game's bin_ship folder (loader, mod, speech library)
#     override  - the Flash agent for Documents\BioWare\<game>\packages\core\override
#   DragonAgeAccess-Setup.exe  the one installer; it downloads the newest <Mod>-*.zip
#     from the GitHub releases itself, so it only changes with setup.cpp.
# Upload all three to the GitHub release vX (the installer finds the zips by name).
# Usage: .\make_release.ps1 -Version X [-Game dao|da2|all]
param([Parameter(Mandatory = $true)][string]$Version, [string]$Game = 'all')
$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
. "$root\tools\games.ps1"

& "$root\build.ps1" -Game $Game

foreach ($g in Select-Games $Game) {
    $info = $Games[$g]
    $mod = $info.Mod
    $stage = "$root\release\stage-$g"
    if (Test-Path $stage) { Remove-Item -Recurse -Force $stage }
    $bin = "$stage\bin_ship"
    $data = "$bin\scripts\$mod"
    $files = @(
        @{ From = "$root\vendor\ual-9.7.4\dinput8.dll";     To = "$bin\dinput8.dll" },
        @{ From = "$root\build\$g\$mod.asi";                To = "$bin\scripts\$mod.asi" },
        @{ From = "$root\build\$g\$($info.Agent)";          To = "$data\$($info.Agent)" },
        @{ From = "$root\build\$g\$($info.Agent)";          To = "$stage\override\$($info.Agent)" },
        @{ From = "$root\lib\prism-x86\prism.dll";          To = "$data\prism.dll" },
        @{ From = "$root\lib\prism\third-party\prism\mpl-2.0.txt"; To = "$data\third-party\prism\mpl-2.0.txt" },
        @{ From = "$root\lib\prism\third-party\prism\NOTICE";      To = "$data\third-party\prism\NOTICE" },
        @{ From = "$root\vendor\ual-9.7.4\LICENSE.txt";     To = "$data\third-party\ultimate-asi-loader\LICENSE.txt" },
        @{ From = "$root\LICENSE";                          To = "$data\LICENSE.txt" },
        @{ From = "$root\README.md";                        To = "$stage\README.md" }
    )
    foreach ($f in $files) {
        New-Item -ItemType Directory -Force (Split-Path $f.To) | Out-Null
        Copy-Item $f.From $f.To -Force
    }
    $zip = "$root\release\$mod-$Version.zip"
    if (Test-Path $zip) { Remove-Item -Force $zip }
    # tar.exe (Windows 10+) writes zip entries with forward slashes; Compress-Archive of
    # PowerShell 5.1 writes backslashes, which some extractors do not take as folders.
    & "$env:SystemRoot\System32\tar.exe" -a -c -f $zip -C $stage bin_ship override README.md
    if ($LASTEXITCODE -ne 0) { throw "packing failed ($LASTEXITCODE)" }
    Write-Host "Release $zip"
    Write-Host "SHA-256 $((Get-FileHash $zip -Algorithm SHA256).Hash)"
}

# The one installer for both games (no mod files inside).
$setup = "$root\release\DragonAgeAccess-Setup.exe"
& "$root\core\installer\build_setup.ps1" -OutExe $setup
Write-Host "Installer $setup"
Write-Host "SHA-256 $((Get-FileHash $setup -Algorithm SHA256).Hash)"
