# 与 phlib\translate_data.c 现有键 diff，输出 ExtendedTools 缺失词条
$ErrorActionPreference = 'Stop'
$plugin = "d:\systeminformer\systeminformer\plugins\ExtendedTools"
$dictPath = "d:\systeminformer\systeminformer\phlib\translate_data.c"

# 1. 读字典所有键（Zh）：形如 {L"键", L"值"},
$dictText = [System.IO.File]::ReadAllText($dictPath, [System.Text.Encoding]::UTF8)
$dictKeys = New-Object System.Collections.Generic.HashSet[string]
$ms = [regex]::Matches($dictText, '\{\s*L"((?:[^"\\]|\\.)*)"\s*,')
foreach ($m in $ms) { [void]$dictKeys.Add($m.Groups[1].Value) }
Write-Host "字典键数: $($dictKeys.Count)"

# 2. 读 _pairs.txt，Zh 侧不在字典的输出
$pairs = [System.IO.File]::ReadAllLines((Join-Path $plugin "_pairs.txt"))
$missing = New-Object System.Collections.Generic.List[string]
$present = 0
foreach ($line in $pairs) {
    $idx = $line.IndexOf("`t")
    if ($idx -lt 0) { continue }
    $en = $line.Substring(0, $idx)
    $zh = $line.Substring($idx + 1)
    if ($zh -and $en) {
        if ($dictKeys.Contains($zh)) { $present++ }
        else { $missing.Add("$en`t$zh") }
    }
}
Write-Host "已覆盖: $present  缺失: $($missing.Count)"

# 3. pwrgrid.c 中文行（fork 自研，需自行翻译）
$pgLines = [System.IO.File]::ReadAllLines((Join-Path $plugin "pwrgrid.c"), [System.Text.Encoding]::UTF8)
$pgOut = New-Object System.Collections.Generic.List[string]
for ($i = 0; $i -lt $pgLines.Count; $i++) {
    if ($pgLines[$i] -match '[\u4e00-\u9fff]' -and $pgLines[$i] -notmatch '^\s*//') {
        $pgOut.Add(("pwrgrid.c:{0}: {1}" -f ($i+1), $pgLines[$i].Trim()))
    }
}

$sorted = $missing.ToArray()
[System.Array]::Sort($sorted, [System.StringComparer]::Ordinal)
[System.IO.File]::WriteAllLines((Join-Path $plugin "_missing.txt"), $sorted, (New-Object System.Text.UTF8Encoding($true)))
[System.IO.File]::WriteAllLines((Join-Path $plugin "_pwrgrid_zh.txt"), $pgOut.ToArray(), (New-Object System.Text.UTF8Encoding($true)))
Write-Host "pwrgrid 中文行: $($pgOut.Count)"
