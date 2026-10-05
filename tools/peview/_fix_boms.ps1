$ErrorActionPreference = 'Stop'
$root = 'd:\systeminformer\systeminformer'
$git = 'C:\Program Files\Git\cmd\git.exe'
$files = & $git status --porcelain | ForEach-Object { $_.Substring(3).Trim() } | Where-Object { $_ -match '\.(c|h|rc)$' }
$fixed = 0
foreach ($f in $files) {
    $p = Join-Path $root $f
    if (-not (Test-Path $p)) { continue }
    if ($f -match '_upstream|_merge|_fix|_pv_entries') { continue }
    $b = [System.IO.File]::ReadAllBytes($p)
    $hasBom = ($b.Length -ge 3 -and $b[0] -eq 0xEF -and $b[1] -eq 0xBB -and $b[2] -eq 0xBF)
    $hasDbl = ($b.Length -ge 6 -and $b[3] -eq 0xEF -and $b[4] -eq 0xBB -and $b[5] -eq 0xBF)
    if ($hasDbl) { Write-Host "DOUBLE-BOM: $f"; continue }
    if ($hasBom) { continue }
    $t = [System.Text.Encoding]::UTF8.GetString($b)
    if ($t -match '[\u4e00-\u9fff]') {
        $out = New-Object byte[] ($b.Length + 3)
        $out[0] = 0xEF; $out[1] = 0xBB; $out[2] = 0xBF
        [Array]::Copy($b, 0, $out, 3, $b.Length)
        [System.IO.File]::WriteAllBytes($p, $out)
        $fixed++
        Write-Host "BOM-ADDED: $f"
    }
}
Write-Host "fixed: $fixed"
