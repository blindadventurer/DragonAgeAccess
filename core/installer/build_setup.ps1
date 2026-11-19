# Builds the installer <ModName>-Setup-<Version>.exe with every file of a release stage
# folder inside it (resources): stage\bin_ship\* go to the game's bin_ship,
# stage\override\* to the game's override folder in Documents.
# Called by make_release.ps1; games\<Game>\game.h gives the game's names.
param(
    [Parameter(Mandatory = $true)][string]$Game,
    [Parameter(Mandatory = $true)][string]$Stage,
    [Parameter(Mandatory = $true)][string]$Version,
    [Parameter(Mandatory = $true)][string]$ModName,
    [Parameter(Mandatory = $true)][string]$OutExe,
    # Test builds only: no administrator request, so tests run without the UAC window.
    [switch]$AsInvoker
)
$ErrorActionPreference = 'Stop'
$here = $PSScriptRoot
$root = Split-Path (Split-Path $here)
$gameDir = "$root\games\$Game"
if (-not (Test-Path "$gameDir\game.h")) { throw "No $gameDir\game.h" }
$work = "$root\build\$Game\setup"
New-Item -ItemType Directory -Force $work | Out-Null

# Manifest and resource script.
$lines = @()
$rc = @('#include <winver.h>', '')
$id = 101
foreach ($part in @(@{ Dir = 'bin_ship'; Where = 'bin' }, @{ Dir = 'override'; Where = 'ovr' })) {
    $base = Join-Path $Stage $part.Dir
    foreach ($f in Get-ChildItem $base -Recurse -File) {
        $rel = $f.FullName.Substring($base.Length + 1)
        $lines += "$id`t$($part.Where)`t$rel"
        $rc += "$id RCDATA `"$($f.FullName.Replace('\', '/'))`""
        $id++
    }
}
[IO.File]::WriteAllText("$work\files.txt", ($lines -join "`n"), (New-Object Text.UTF8Encoding $false))
$rc += "FILES RCDATA `"$($work.Replace('\', '/'))/files.txt`""

$nums = ($Version -split '[^0-9]+' | Where-Object { $_ -ne '' } | Select-Object -First 3)
while ($nums.Count -lt 3) { $nums += '0' }
$v = ($nums -join ',') + ',0'
$rc += @(
    '', "VS_VERSION_INFO VERSIONINFO FILEVERSION $v PRODUCTVERSION $v",
    'BEGIN', ' BLOCK "StringFileInfo"', ' BEGIN', '  BLOCK "040904B0"', '  BEGIN',
    "   VALUE `"FileDescription`", `"$ModName setup`"",
    "   VALUE `"ProductName`", `"$ModName`"",
    "   VALUE `"FileVersion`", `"$Version`"",
    "   VALUE `"ProductVersion`", `"$Version`"",
    "   VALUE `"LegalCopyright`", `"MIT licence, $ModName contributors`"",
    "   VALUE `"OriginalFilename`", `"$(Split-Path $OutExe -Leaf)`"",
    '  END', ' END', ' BLOCK "VarFileInfo"', ' BEGIN', '  VALUE "Translation", 0x409, 1200', ' END', 'END')
[IO.File]::WriteAllText("$work\setup.rc", ($rc -join "`r`n"), (New-Object Text.ASCIIEncoding))

$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vs) { throw 'Visual Studio C++ build tools not found' }
$vcvars = Join-Path $vs 'VC\Auxiliary\Build\vcvars32.bat'
$env:PATH = "$(Split-Path $vswhere);$env:PATH"
$level = if ($AsInvoker) { 'asInvoker' } else { 'requireAdministrator' }
$cmd = "`"$vcvars`" >nul && rc /nologo /fo `"$work\setup.res`" `"$work\setup.rc`" && " +
       "cl /nologo /std:c++17 /O2 /MT /EHsc /utf-8 /W3 /I`"$gameDir`" `"$here\setup.cpp`" `"$work\setup.res`" " +
       "/Fo`"$work\\`" /Fe`"$OutExe`" /link /SUBSYSTEM:WINDOWS " +
       "user32.lib shell32.lib ole32.lib advapi32.lib imagehlp.lib " +
       "/MANIFEST:EMBED /MANIFESTUAC:`"level='$level' uiAccess='false'`""
cmd /c $cmd
if ($LASTEXITCODE -ne 0) { throw "setup build failed ($LASTEXITCODE)" }
Write-Host "Built $OutExe ($($id - 101) files inside)"
