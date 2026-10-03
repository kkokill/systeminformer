# 诊断v2：PrintWindow 抓图（免遮挡）+ 枚举子窗口类名 + 发 TNM_TRANSLATECOLUMNS
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
Add-Type -ReferencedAssemblies System.Drawing @"
using System;
using System.Runtime.InteropServices;
using System.Text;
using System.Collections.Generic;
public class W2 {
    [DllImport("user32.dll")] public static extern bool EnumChildWindows(IntPtr p, EnumProc cb, IntPtr l);
    [DllImport("user32.dll")] public static extern IntPtr SendMessage(IntPtr h, uint m, IntPtr w, IntPtr l);
    [DllImport("user32.dll")] public static extern bool GetClassName(IntPtr h, StringBuilder s, int n);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr dc, uint flags);
    [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int cmd);
    public delegate bool EnumProc(IntPtr h, IntPtr l);
    public struct RECT { public int L, T, R, B; }
    public static List<string> Classes = new List<string>();
    public static IntPtr Found;
    public static void ListChildren(IntPtr parent) {
        Classes.Clear();
        EnumChildWindows(parent, (h, l) => {
            var sb = new StringBuilder(64); GetClassName(h, sb, 64);
            Classes.Add(sb.ToString());
            return true;
        }, IntPtr.Zero);
    }
    public static void FindByClass(IntPtr parent, string cls) {
        Found = IntPtr.Zero;
        EnumChildWindows(parent, (h, l) => {
            var sb = new StringBuilder(64); GetClassName(h, sb, 64);
            if (sb.ToString() == cls) { Found = h; return false; }
            return true;
        }, IntPtr.Zero);
    }
    public static void Snap(IntPtr hwnd, string path) {
        RECT r; GetWindowRect(hwnd, out r);
        int w = r.R - r.L, h2 = r.B - r.T;
        if (w <= 0 || h2 <= 0) { System.IO.File.WriteAllText(path, "BADRECT"); return; }
        var bmp = new System.Drawing.Bitmap(w, h2);
        var g = System.Drawing.Graphics.FromImage(bmp);
        IntPtr dc = g.GetHdc();
        PrintWindow(hwnd, dc, 2); // PW_RENDERFULLCONTENT
        g.ReleaseHdc(dc);
        bmp.Save(path, System.Drawing.Imaging.ImageFormat.Png);
        g.Dispose(); bmp.Dispose();
    }
}
"@

$exe = "d:\systeminformer\systeminformer\bin\Release64\SystemInformer.exe"
$p = Start-Process -FilePath $exe -PassThru
Start-Sleep -Seconds 5
$p.Refresh()
$main = $p.MainWindowHandle
if ($main -eq [IntPtr]::Zero) { Write-Host "NO MAIN WINDOW"; Stop-Process -Id $p.Id -Force; exit 1 }
[W2]::ShowWindow($main, 5) | Out-Null  # SW_SHOW
Start-Sleep -Seconds 1

[W2]::ListChildren($main)
Write-Host "=== 子窗口类名 ==="
[W2]::Classes | Group-Object | Sort-Object Count -Descending | ForEach-Object { Write-Host ("  {0} x {1}" -f $_.Count, $_.Name) }

[W2]::Snap($main, "d:\systeminformer\systeminformer\tools\Localization\_diag_boot.png")
Write-Host "boot snapshot saved"

[W2]::FindByClass($main, "PhTreeNew")
$tree = [W2]::Found
Write-Host "PhTreeNew: $tree"
if ($tree -ne [IntPtr]::Zero) {
    [W2]::SendMessage($tree, 1050, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
    Start-Sleep -Milliseconds 800
    [W2]::Snap($main, "d:\systeminformer\systeminformer\tools\Localization\_diag_after.png")
    Write-Host "after snapshot saved"
}
Stop-Process -Id $p.Id -Force
Write-Host "Done"
