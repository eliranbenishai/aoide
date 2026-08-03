# Launch the built Tramp app, wait for its window, capture it to a PNG, then close it.
param(
  [string]$Exe = "build\windows\x64\runner\Debug\tramp.exe",
  [string]$Out = ".scratch\graphite-chrome\probes\real-app.png",
  [int]$WaitSeconds = 8
)

Add-Type -AssemblyName System.Drawing

Add-Type @"
using System;
using System.Runtime.InteropServices;
public class Win {
  [StructLayout(LayoutKind.Sequential)]
  public struct RECT { public int Left, Top, Right, Bottom; }
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
}
"@

$proc = Start-Process -FilePath $Exe -PassThru
Write-Host "launched pid $($proc.Id), waiting ${WaitSeconds}s for the window to settle"
Start-Sleep -Seconds $WaitSeconds

$proc.Refresh()
$h = $proc.MainWindowHandle
if ($h -eq [IntPtr]::Zero) {
  # Frameless windows sometimes report the handle late; look again across the process tree.
  $h = (Get-Process -Name tramp -ErrorAction SilentlyContinue |
        Where-Object { $_.MainWindowHandle -ne 0 } |
        Select-Object -First 1).MainWindowHandle
}

if (-not $h -or $h -eq [IntPtr]::Zero) {
  Write-Host "NO WINDOW HANDLE - the app may have failed to open a window"
  $proc | Stop-Process -Force -ErrorAction SilentlyContinue
  exit 2
}

[void][Win]::SetForegroundWindow($h)
Start-Sleep -Milliseconds 900

$r = New-Object Win+RECT
[void][Win]::GetWindowRect($h, [ref]$r)
$w = $r.Right - $r.Left
$hh = $r.Bottom - $r.Top
Write-Host "window rect ${w}x${hh} at $($r.Left),$($r.Top)"

if ($w -le 0 -or $hh -le 0) {
  Write-Host "DEGENERATE WINDOW RECT"
  $proc | Stop-Process -Force -ErrorAction SilentlyContinue
  exit 3
}

$bmp = New-Object System.Drawing.Bitmap $w, $hh
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.CopyFromScreen($r.Left, $r.Top, 0, 0, $bmp.Size)
$bmp.Save((Resolve-Path -LiteralPath (Split-Path $Out -Parent)).Path + "\" + (Split-Path $Out -Leaf),
          [System.Drawing.Imaging.ImageFormat]::Png)
$g.Dispose(); $bmp.Dispose()
Write-Host "wrote $Out (${w}x${hh})"

$proc | Stop-Process -Force -ErrorAction SilentlyContinue
Write-Host "closed the app"
