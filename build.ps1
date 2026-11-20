# Builds the 32-bit mod DLL and the Flash agent of each game with the MSVC x86 toolset:
#   build\dao\DAOAccess.asi  build\dao\daoaccess.gfx
#   build\da2\DA2Access.asi  build\da2\da2access.gfx
# Sources: core\src (shared) plus games\<game>\src (that game's addresses and GFx
# layer); games\<game>\game.h names the game.
# Usage: .\build.ps1 [-Game dao|da2|all]
param([string]$Game = 'all')
$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
. "$root\tools\games.ps1"

$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vs) { throw 'Visual Studio C++ build tools not found' }
$vcvars = Join-Path $vs 'VC\Auxiliary\Build\vcvars32.bat'
$env:PATH = "$(Split-Path $vswhere);$env:PATH"  # vcvars calls vswhere by bare name

foreach ($g in Select-Games $Game) {
    $info = $Games[$g]
    $out = "$root\build\$g"
    New-Item -ItemType Directory -Force $out | Out-Null
    $files = @(Get-ChildItem "$root\core\src\*.cpp") + @(Get-ChildItem "$root\games\$g\src\*.cpp")
    $sources = ($files | ForEach-Object { "`"$($_.FullName)`"" }) -join ' '
    $inc = "/I`"$root\games\$g`" /I`"$root\games\$g\src`" /I`"$root\core\src`" /I`"$root\lib\prism\include`""
    $cmd = "`"$vcvars`" >nul && cl /nologo /std:c++17 /O2 /MT /EHsc /utf-8 /W3 /LD $inc $sources " +
           "/Fo`"$out\\`" /Fe`"$out\$($info.Mod).asi`" /link /DLL user32.lib version.lib"
    cmd /c $cmd
    if ($LASTEXITCODE -ne 0) { throw "build of $g failed ($LASTEXITCODE)" }
    Write-Host "Built $out\$($info.Mod).asi"
    # Exit watcher (core\watcher) for a game whose game.h lists programs to close
    # after it exits (Dragon Age II: the EA app).
    if (Select-String -Path "$root\games\$g\game.h" -Pattern 'kCloseAfterExit' -Quiet) {
        $cmd = "`"$vcvars`" >nul && cl /nologo /std:c++17 /O2 /MT /EHsc /utf-8 /W3 `"$root\core\watcher\watcher.cpp`" " +
               "/Fo`"$out\\`" /Fe`"$out\$($info.Mod)Watch.exe`" /link /SUBSYSTEM:WINDOWS user32.lib shell32.lib"
        cmd /c $cmd
        if ($LASTEXITCODE -ne 0) { throw "watcher build of $g failed ($LASTEXITCODE)" }
        Write-Host "Built $out\$($info.Mod)Watch.exe"
    }
    # Flash agent loaded into the game's menus (FFDec compiles core\agent\daoaccess.as)
    python "$root\tools\build_agent.py" "$out\$($info.Agent)"
    if ($LASTEXITCODE -ne 0) { throw "agent build of $g failed ($LASTEXITCODE)" }
}
