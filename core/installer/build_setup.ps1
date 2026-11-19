# Builds the one installer for both games, DragonAgeAccess-Setup.exe. It holds no mod
# files (it downloads the newest release from GitHub), so it is rebuilt only when
# setup.cpp or a games\<game>\game.h changes, not for every release.
# Usage: core\installer\build_setup.ps1 -OutExe <path> [-AsInvoker]
param(
    [Parameter(Mandatory = $true)][string]$OutExe,
    # Test builds only: no administrator request, so tests run without the UAC window.
    [switch]$AsInvoker
)
$ErrorActionPreference = 'Stop'
$here = $PSScriptRoot
$root = Split-Path (Split-Path $here)
$work = "$root\build\setup"
New-Item -ItemType Directory -Force $work | Out-Null

# Version resource from the installer's own version (kSetupVersion in setup.cpp).
$m = Select-String -Path "$here\setup.cpp" -Pattern 'kSetupVersion\[\] = L"([0-9.]+)"'
if (-not $m) { throw 'kSetupVersion not found in setup.cpp' }
$Version = $m.Matches[0].Groups[1].Value
$nums = @($Version -split '\.' | Where-Object { $_ -ne '' })
while ($nums.Count -lt 4) { $nums += '0' }
$v = ($nums[0..3] -join ',')
$rc = @(
    '#include <winver.h>', '',
    "VS_VERSION_INFO VERSIONINFO FILEVERSION $v PRODUCTVERSION $v",
    'BEGIN', ' BLOCK "StringFileInfo"', ' BEGIN', '  BLOCK "040904B0"', '  BEGIN',
    '   VALUE "FileDescription", "Dragon Age Access setup"',
    '   VALUE "ProductName", "Dragon Age Access"',
    "   VALUE `"FileVersion`", `"$Version`"",
    "   VALUE `"ProductVersion`", `"$Version`"",
    '   VALUE "LegalCopyright", "MIT licence, DragonAgeAccess contributors"',
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
       "cl /nologo /std:c++17 /O2 /MT /EHsc /utf-8 /W3 `"$here\setup.cpp`" `"$work\setup.res`" " +
       "/Fo`"$work\\`" /Fe`"$OutExe`" /link /SUBSYSTEM:WINDOWS " +
       "user32.lib shell32.lib ole32.lib advapi32.lib imagehlp.lib comctl32.lib winhttp.lib bcrypt.lib " +
       "/MANIFEST:EMBED /MANIFESTUAC:`"level='$level' uiAccess='false'`""
cmd /c $cmd
if ($LASTEXITCODE -ne 0) { throw "setup build failed ($LASTEXITCODE)" }
Write-Host "Built $OutExe (installer $Version)"
