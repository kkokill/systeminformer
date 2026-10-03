# Alt+V opens View menu, then LEFT arrow switches to System menu (standard menu keyboard nav)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
Add-Type -AssemblyName System.Windows.Forms
Add-Type -ReferencedAssemblies System.Drawing @'
using System;
using System.Text;
using System.Collections.Generic;
using System.Runtime.InteropServices;
public static class W32d {
    public delegate bool EnumProc(IntPtr h, IntPtr l);
    [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc cb, IntPtr l);
    [DllImport("user32.dll")] public static extern int GetClassName(IntPtr h, StringBuilder s, int n);
    [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr hdc, uint flags);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
    public struct RECT { public int L, T, R, B; }
    public static List<IntPtr> FindByClass(string cls) {
        var list = new List<IntPtr>();
        EnumWindows((h, l) => {
            var sb = new StringBuilder(256);
            GetClassName(h, sb, 256);
            if (sb.ToString() == cls && IsWindowVisible(h)) list.Add(h);
            return true;
        }, IntPtr.Zero);
        return list;
    }
    public static void Shot(IntPtr hwnd, string path) {
        RECT r; GetWindowRect(hwnd, out r);
        int w = r.R - r.L, hgt = r.B - r.T;
        if (w <= 0 || hgt <= 0) return;
        var bmp = new System.Drawing.Bitmap(w, hgt);
        var g = System.Drawing.Graphics.FromImage(bmp);
        IntPtr hdc = g.GetHdc();
        PrintWindow(hwnd, hdc, 2);
        g.ReleaseHdc(hdc); g.Dispose();
        bmp.Save(path, System.Drawing.Imaging.ImageFormat.Png); bmp.Dispose();
    }
}
'@

$exe = "d:\systeminformer\systeminformer\bin\Release64\SystemInformer.exe"
$out = "d:\systeminformer\systeminformer\tools\Localization"

Get-Process SystemInformer -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Milliseconds 500
$proc = Start-Process $exe -PassThru
Start-Sleep -Seconds 6

$main = (Get-Process -Id $proc.Id).MainWindowHandle
if ($main -eq [IntPtr]::Zero) { Write-Host "no main window"; exit 1 }
[W32d]::SetForegroundWindow($main) | Out-Null
Start-Sleep -Milliseconds 600

[System.Windows.Forms.SendKeys]::SendWait("%v")
Start-Sleep -Milliseconds 900
[System.Windows.Forms.SendKeys]::SendWait("{LEFT}")
Start-Sleep -Milliseconds 1000

$menus = [W32d]::FindByClass("#32768")
Write-Host ("menus visible: " + $menus.Count)
if ($menus.Count -gt 0) {
    [W32d]::Shot($menus[$menus.Count - 1], (Join-Path $out "_v_system.png"))
    Write-Host "system menu shot saved"
}
[System.Windows.Forms.SendKeys]::SendWait("{ESC}")
Start-Sleep -Milliseconds 300
Stop-Process -Id $proc.Id -Force
Write-Host "done"
