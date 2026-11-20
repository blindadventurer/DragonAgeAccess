# Test helper: presses keys in the foreground game window and prints the new mod
# log lines. Steps: "KEY[xN] seconds" separated by ";", key = a System.Windows.Forms
# Keys name (Return, Escape, Down, Left, Right, F10, F11...).
# Example: powershell -File tools\keys.ps1 -Steps "Return 8; Escape 6; Down x3 1"
param([string]$Steps, [switch]$All)
Add-Type -TypeDefinition @'
using System; using System.Runtime.InteropServices;
public static class DA2Keys {
  [DllImport("user32.dll")] static extern void keybd_event(byte vk, byte scan, uint flags, UIntPtr extra);
  [DllImport("user32.dll")] static extern uint MapVirtualKey(uint code, uint type);
  public static void Tap(byte vk) {
    uint ext = (vk >= 0x21 && vk <= 0x2E) ? 1u : 0u;   // arrows, Home/End, PgUp/PgDn, Ins/Del
    byte sc = (byte)MapVirtualKey(vk, 0);
    keybd_event(vk, sc, ext, UIntPtr.Zero); System.Threading.Thread.Sleep(80);
    keybd_event(vk, sc, 2u | ext, UIntPtr.Zero);
  }
}
'@
Add-Type -AssemblyName System.Windows.Forms
$log = 'F:\SteamLibrary\steamapps\common\Dragon Age II\bin_ship\scripts\DA2Access.log'
$n = (Get-Content $log -Encoding UTF8).Count
foreach ($s in ($Steps -split ';')) {
    $p = $s.Trim() -split '\s+'
    if (-not $p[0]) { continue }
    $count = 1; $wait = 1.0
    foreach ($x in $p[1..($p.Count - 1)]) { if ($x -like 'x*') { $count = [int]$x.Substring(1) } elseif ($x) { $wait = [double]$x } }
    $vk = [byte][System.Windows.Forms.Keys]::($p[0])
    for ($i = 0; $i -lt $count; $i++) { [DA2Keys]::Tap($vk); Start-Sleep -Milliseconds ([int]($wait * 1000)) }
}
$new = Get-Content $log -Encoding UTF8 | Select-Object -Skip $n
if ($All) { $new } else { $new | Where-Object { $_ -notmatch 'perf:|memory:|holder clip|loadMovie|identified after|agent: ready' } }
