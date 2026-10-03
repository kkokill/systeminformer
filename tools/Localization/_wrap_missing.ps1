# 批量包装遗漏的创建时翻译：菜单项/ListView项/SetWindowText 等调用中的 CJK 字面量
# 排除：已包装、内联三元（? L"）、注释行。UTF-8(BOM) + CRLF 原样保留。
$ErrorActionPreference = 'Stop'
$Root = "d:\systeminformer\systeminformer"
$utf8B = [System.Text.UTF8Encoding]::new($true)   # 读时 BOM 自动识别，写时保留
$cjk = [regex]'[\u4e00-\u9fff]'
$rxLit = [regex]'L"[^"]*[\u4e00-\u9fff][^"]*"'

$files = @()
$files += Get-ChildItem "$Root\SystemInformer" -Filter *.c | ForEach-Object { $_.FullName }
Get-ChildItem "$Root\plugins" -Directory | ForEach-Object { $files += Get-ChildItem $_.FullName -Filter *.c -Recurse | ForEach-Object { $_.FullName } }
$files += Get-ChildItem "$Root\phlib" -Filter *.c | ForEach-Object { $_.FullName }

$total = 0
$hits = 0
$report = @()
foreach ($f in $files) {
    if ($f -like '*translate_data.c') { continue }
    $text = [System.IO.File]::ReadAllText($f, $utf8B)
    $nl = if ($text.Contains("`r`n")) { "`r`n" } else { "`n" }
    $lines = $text -split "`r?`n", -1
    $changed = 0
    for ($i = 0; $i -lt $lines.Count; $i++) {
        $L = $lines[$i]
        if ($L -match '^\s*//' -or $L -match '/\*' -or $L -match 'PhTranslateTextZ' -or $L -match '\? L"') { continue }
        $isTarget = $false
        if ($L -match '(PhCreateEMenuItem|PhInsertEMenuItem|PhAddEMenuItem|PhSetWindowText|PhSetDlgItemText)\s*\(') { $isTarget = $true }
        if ($L -match 'PhAddListViewItem\s*\(' -and $f -like '*options.c') { $isTarget = $true }
        if (-not $isTarget) { continue }
        $hits++
        if ($f -like '*options.c' -and $L -match 'IDC_DEFSTATE') { Write-Host "DBG: [$L]"; Write-Host "DBG cjk=$($cjk.IsMatch($L)) new=$([regex]::Replace($L, 'L\"([^\"]*[\u4e00-\u9fff][^\"]*)\"', 'PhTranslateTextZ(L\"`$1\")'))" }
        if (-not $cjk.IsMatch($L)) { continue }
        $new = [regex]::Replace($L, 'L"([^"]*[\u4e00-\u9fff][^"]*)"', 'PhTranslateTextZ(L"$1")')
        if ($new -ne $L) { $lines[$i] = $new; $changed++ }
    }
    if ($changed -gt 0) {
        [System.IO.File]::WriteAllText($f, ($lines -join $nl), $utf8B)
        $rel = $f.Substring($Root.Length + 1)
        $report += "  $changed  $rel"
        $total += $changed
    }
}
Write-Host "target-hits: $hits"; Write-Host "包装总数: $total"
$report | ForEach-Object { Write-Host $_ }
