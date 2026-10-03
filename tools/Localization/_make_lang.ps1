# 遍历全代码已汉化的中文字符串，补齐翻译表占位条目，重建 translate_data.c + zh-en.lang
# 缺失条目以 en=zh 占位（切换英文时保持原文，不劣于现状），后续可 LLM 补翻。
# 用法: powershell -File _make_lang.ps1 [-Root "d:\systeminformer\systeminformer"]
# 注意: 本脚本的 PowerShell 排序在 hashtable 场景会静默失效（表乱序→二分查找 miss），
#       跑完必须再执行 _fix_order.ps1 用 C# 重排并重新生成 zh-en.lang。
param(
    [string]$Root = "d:\systeminformer\systeminformer"
)
$ErrorActionPreference = 'Stop'

# ---------- 工具函数（摘自 _survey.ps1）----------
function Decode-Literal([string]$raw) {
    $inner = $raw -replace '^L?"', '' -replace '"$', ''
    $sb = New-Object System.Text.StringBuilder
    $i = 0; $len = $inner.Length
    while ($i -lt $len) {
        $c = $inner[$i]
        if ($c -ne '\') { [void]$sb.Append($c); $i++; continue }
        if ($i + 1 -ge $len) { return $null }
        $n = $inner[$i + 1]
        switch ($n) {
            'n' { [void]$sb.Append([char]10); $i += 2 }
            't' { [void]$sb.Append([char]9); $i += 2 }
            'r' { [void]$sb.Append([char]13); $i += 2 }
            '\' { [void]$sb.Append('\'); $i += 2 }
            '"' { [void]$sb.Append('"'); $i += 2 }
            "'" { [void]$sb.Append("'"); $i += 2 }
            '?' { [void]$sb.Append('?'); $i += 2 }
            '0' { [void]$sb.Append([char]0); $i += 2 }
            'b' { [void]$sb.Append([char]8); $i += 2 }
            'a' { [void]$sb.Append([char]7); $i += 2 }
            'f' { [void]$sb.Append([char]12); $i += 2 }
            'v' { [void]$sb.Append([char]11); $i += 2 }
            'x' {
                $j = $i + 2; $hex = ''
                while ($j -lt $len -and [char]::IsWhiteSpace($inner[$j]) -eq $false -and '0123456789abcdefABCDEF'.IndexOf($inner[$j]) -ge 0) { $hex += $inner[$j]; $j++ }
                if ($hex.Length -eq 0) { return $null }
                [void]$sb.Append([char][Convert]::ToInt32($hex, 16)); $i = $j
            }
            default { return $null }
        }
    }
    return $sb.ToString()
}

function Encode-Literal([string]$s) {
    $t = $s -replace '\\', '\\\\' -replace '"', '\"'
    # 控制字符必须转义，否则以裸字节写入 .c 源文件（不可读且破坏往返）
    $t = $t -replace "`b", '\b' -replace "`f", '\f' -replace "`v", '\v' -replace "`a", '\a'
    $t = $t -replace "`r", '\r' -replace "`n", '\n' -replace "`t", '\t'
    return $t
}

$script:HasCjk = [regex]'[\u3000-\u303f\u3400-\u4dbf\u4e00-\u9fff\uff00-\uffef]'
$script:RxToken = [regex]'(?<nl>\r?\n)|//|/\*|\*/|L?"(?:[^"\\]|\\.)*"'

function Get-Literals([string]$text) {
    $lits = New-Object System.Collections.ArrayList
    $inLine = $false; $inBlock = $false
    $prevEnd = -1; $pending = $null
    foreach ($m in $script:RxToken.Matches($text)) {
        $v = $m.Value
        if ($m.Groups['nl'].Success) { $inLine = $false; continue }
        if ($v -eq '//') { $inLine = $true; continue }
        if ($v -eq '/*') { $inBlock = $true; continue }
        if ($v -eq '*/') { $inBlock = $false; continue }
        if ($inLine -or $inBlock) { $prevEnd = $m.Index + $m.Length; continue }
        if ($v.StartsWith('"') -or $v.StartsWith('L"')) {
            $decoded = Decode-Literal $v
            if ($null -eq $decoded) { $pending = $null; $prevEnd = $m.Index + $m.Length; continue }
            if ($null -ne $pending -and $prevEnd -ge 0 -and ($text.Substring($prevEnd, $m.Index - $prevEnd) -match '^\s*$')) {
                $pending = $pending + $decoded
            } else {
                if ($null -ne $pending) { [void]$lits.Add($pending) }
                $pending = $decoded
            }
            $prevEnd = $m.Index + $m.Length
        }
    }
    if ($null -ne $pending) { [void]$lits.Add($pending) }
    return ,$lits
}

# ---------- 收集源文件：SystemInformer/plugins 的 .c + phlib 的 .c/.cpp（排除 translate_data.c） ----------
$utf8 = New-Object System.Text.UTF8Encoding($true)   # BOM（与生成器一致）
$files = @(Get-ChildItem (Join-Path $Root "SystemInformer") -Filter *.c | ForEach-Object { $_.FullName })
Get-ChildItem (Join-Path $Root "plugins") -Directory | ForEach-Object {
    $files += @(Get-ChildItem $_.FullName -Filter *.c -Recurse | ForEach-Object { $_.FullName })
}
# tools 工具目录（peview/CustomSetupTool/PortableLauncher 等也链 phlib，UI 文本同样走翻译表）
if (Test-Path (Join-Path $Root "tools")) {
    Get-ChildItem (Join-Path $Root "tools") -Directory | ForEach-Object {
        $files += @(Get-ChildItem $_.FullName -Filter *.c -Recurse | ForEach-Object { $_.FullName })
    }
}
$files += @(Get-ChildItem (Join-Path $Root "phlib") -Filter *.c | Where-Object { $_.Name -ne 'translate_data.c' } | ForEach-Object { $_.FullName })
$files += @(Get-ChildItem (Join-Path $Root "phlib") -Filter *.cpp | ForEach-Object { $_.FullName })
Write-Host "扫描文件数: $($files.Count)"

# ---------- 解析现有 translate_data.c ----------
$dictPath = Join-Path $Root "phlib\translate_data.c"
$dictText = [System.IO.File]::ReadAllText($dictPath, (New-Object System.Text.UTF8Encoding($false)))
$rxEntry = [regex]'\{\s*L"((?:[^"\\]|\\.)*)"\s*,\s*L"((?:[^"\\]|\\.)*)"\s*\}'
$dict = @{}   # zh -> en
foreach ($m in $rxEntry.Matches($dictText)) {
    $zh = Decode-Literal ('L"' + $m.Groups[1].Value + '"')
    $en = Decode-Literal ('L"' + $m.Groups[2].Value + '"')
    if ($null -ne $zh -and -not $dict.ContainsKey($zh)) { $dict[$zh] = $en }
}
Write-Host "现有词典条目: $($dict.Count)"

# ---------- 扫描代码 CJK 字面量，找出未收录的 ----------
$missing = New-Object 'System.Collections.Generic.HashSet[string]'
foreach ($f in $files) {
    $text = [System.IO.File]::ReadAllText($f, (New-Object System.Text.UTF8Encoding($false)))
    foreach ($l in (Get-Literals $text)) {
        if (-not $script:HasCjk.IsMatch($l)) { continue }
        if (-not $dict.ContainsKey($l)) { [void]$missing.Add($l) }
    }
}
Write-Host "代码中未收录的中文条目: $($missing.Count)"

# ---------- 合并占位条目（en=zh）并按 Ordinal 排序重建 ----------
foreach ($zh in $missing) { $dict[$zh] = $zh }

$zhAll = [string[]]@($dict.Keys)
$pairList = New-Object System.Collections.ArrayList
foreach ($zh in $zhAll) { [void]$pairList.Add(@{ Zh = $zh; En = $dict[$zh] }) }
$arr = $pairList.ToArray()
$keys = [string[]]@($arr | ForEach-Object { $_.Zh })
[Array]::Sort($keys, $arr, [StringComparer]::Ordinal)

# ---------- 重建 translate_data.c ----------
$sb = New-Object System.Text.StringBuilder
[void]$sb.Append("/*`r`n")
[void]$sb.Append(" * 运行时 UI 文本翻译字典（zh→en）— 自动生成，勿手工编辑。`r`n")
[void]$sb.Append(" * 生成器: tools\Localization\gen_translate.ps1 + _make_lang.ps1（占位补齐）`r`n")
[void]$sb.Append(" * 基线:   1487bbf6e`r`n")
[void]$sb.Append(" * 规则:   键 = 当前源码硬编码中文；值 = git 基线同位置英文。`r`n")
[void]$sb.Append(" *         en=zh 的条目为占位（源码新增中文，暂无基线英文），待 LLM 补翻。`r`n")
[void]$sb.Append(" * 表按 UTF-16 码元排序，供 translate.c 二分查找。`r`n")
[void]$sb.Append(" */`r`n`r`n")
[void]$sb.Append("#include <ph.h>`r`n`r`n")
[void]$sb.Append("typedef struct _PH_TRANSLATE_ENTRY`r`n{`r`n")
[void]$sb.Append("    PCWSTR Zh;`r`n    PCWSTR En;`r`n")
[void]$sb.Append("} PH_TRANSLATE_ENTRY, *PPH_TRANSLATE_ENTRY;`r`n`r`n")
[void]$sb.Append("const PH_TRANSLATE_ENTRY PhTranslateTable[] =`r`n{`r`n")
foreach ($e in $arr) {
    [void]$sb.Append("    { L""" + (Encode-Literal $e.Zh) + """, L""" + (Encode-Literal $e.En) + """ },`r`n")
}
[void]$sb.Append("};`r`n`r`n")
[void]$sb.Append("const ULONG PhTranslateTableCount = RTL_NUMBER_OF(PhTranslateTable);`r`n")
[System.IO.File]::WriteAllText($dictPath, $sb.ToString(), $utf8)
Write-Host "重建 translate_data.c: $($arr.Count) 条（新增占位 $($missing.Count) 条）"

# ---------- 生成 zh-en.lang ----------
& (Join-Path $PSScriptRoot "gen_lang_file.ps1") -Root $Root
