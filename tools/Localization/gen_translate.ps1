# 生成 phlib\translate_data.c：HEAD 硬编码中文 ↔ git 基线英文 自动配对
# 用法: powershell -File gen_translate.ps1 [-Baseline 1487bbf6e]
# 输出: phlib\translate_data.c + translate_report.txt
param(
    [string]$Baseline = "1487bbf6e",
    [string]$Root = "d:\systeminformer\systeminformer"
)

$ErrorActionPreference = 'Stop'
$outC = Join-Path $Root "phlib\translate_data.c"
$reportPath = Join-Path $Root "tools\Localization\translate_report.txt"
$baselineDir = Join-Path $env:TEMP "si_baseline_$Baseline"

# ---------- 1. 导出基线树 ----------
if (Test-Path $baselineDir) { Remove-Item -Recurse -Force $baselineDir }
New-Item -ItemType Directory $baselineDir | Out-Null
$tarFile = Join-Path $env:TEMP "si_baseline.tar"
cmd /c "git -C `"$Root`" archive $Baseline SystemInformer plugins tools > `"$tarFile`""
if ($LASTEXITCODE -ne 0) { throw "git archive failed" }
tar -xf $tarFile -C $baselineDir
if ($LASTEXITCODE -ne 0) { throw "tar extract failed" }

# ---------- 2. 工具函数 ----------
function Decode-Literal([string]$raw) {
    # raw: 含 L 前缀与引号的字面量 → 解码后的运行时值；未知转义返回 $null
    $inner = $raw -replace '^L?"', '' -replace '"$', ''
    $sb = New-Object System.Text.StringBuilder
    $i = 0
    $len = $inner.Length
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
            'a' { [void]$sb.Append([char]7); $i += 2 }
            'b' { [void]$sb.Append([char]8); $i += 2 }
            'f' { [void]$sb.Append([char]12); $i += 2 }
            'v' { [void]$sb.Append([char]11); $i += 2 }
            '0' { [void]$sb.Append([char]0); $i += 2 }
            'x' {
                $j = $i + 2; $hex = ''
                while ($j -lt $len -and $hex.Length -lt 4 -and [char]::IsWhiteSpace($inner[$j]) -eq $false -and '0123456789abcdefABCDEF'.IndexOf($inner[$j]) -ge 0) { $hex += $inner[$j]; $j++ }
                if ($hex.Length -eq 0) { return $null }
                [void]$sb.Append([char][Convert]::ToInt32($hex, 16)); $i = $j
            }
            default {
                if ('1234567'.IndexOf($n) -ge 0) {
                    $j = $i + 1; $oct = ''
                    while ($j -lt $len -and $oct.Length -lt 3 -and '01234567'.IndexOf($inner[$j]) -ge 0) { $oct += $inner[$j]; $j++ }
                    [void]$sb.Append([char][Convert]::ToInt32($oct, 8)); $i = $j
                } else { return $null }
            }
        }
    }
    return $sb.ToString()
}

function Encode-Literal([string]$s) {
    $sb = New-Object System.Text.StringBuilder
    foreach ($ch in $s.ToCharArray()) {
        switch ($ch) {
            '"' { [void]$sb.Append('\"') }
            '\' { [void]$sb.Append('\\') }
            ([char]10) { [void]$sb.Append('\n') }
            ([char]9) { [void]$sb.Append('\t') }
            ([char]13) { [void]$sb.Append('\r') }
            default {
                if ([int]$ch -lt 0x20 -or [int]$ch -eq 0x7F) { [void]$sb.Append('\x{0:x2}' -f [int]$ch) }
                else { [void]$sb.Append($ch) }
            }
        }
    }
    return 'L"' + $sb.ToString() + '"'
}

$script:HasCjk = [regex]'[\u3000-\u303f\u3400-\u4dbf\u4e00-\u9fff\uff00-\uffef]'
$script:RxToken = [regex]'(?<nl>\r?\n)|//|/\*|\*/|L?"(?:[^"\\]|\\.)*"'
$script:RxPlaceholders = [regex]'%[-+ 0#]*(\d+|\*)?(\.(\d+|\*))?(hh|h|ll|l|I64|I32|I|w)?[diouxXeEfgGaAcscnp]'

function Get-Placeholders([string]$s) {
    $list = @()
    foreach ($m in $script:RxPlaceholders.Matches($s)) { $list += $m.Value.ToLower() }
    return ($list | Sort-Object)
}

function Get-Literals([string]$text) {
    # 状态机扫描：跳过注释，收集字符串字面量，合并相邻（仅空白间隔）字面量
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

function Get-LcsAnchors([string[]]$a, [string[]]$b) {
    # 返回对齐锚点数组，每项 @($i,$j)，按序递增
    $n = $a.Count; $m = $b.Count
    $anchors = New-Object System.Collections.ArrayList
    if ($n -eq 0 -or $m -eq 0) { return ,$anchors }
    $dp = New-Object 'int[,]' ($n + 1), ($m + 1)
    for ($i = $n - 1; $i -ge 0; $i--) {
        $ip = $i + 1
        for ($j = $m - 1; $j -ge 0; $j--) {
            $jp = $j + 1
            if ($a[$i] -eq $b[$j]) { $dp[$i, $j] = $dp[$ip, $jp] + 1 }
            elseif ($dp[$ip, $j] -ge $dp[$i, $jp]) { $dp[$i, $j] = $dp[$ip, $j] }
            else { $dp[$i, $j] = $dp[$i, $jp] }
        }
    }
    $i = 0; $j = 0
    while ($i -lt $n -and $j -lt $m) {
        $ip = $i + 1; $jp = $j + 1
        if ($a[$i] -eq $b[$j]) { [void]$anchors.Add(@($i, $j)); $i++; $j++ }
        elseif ($dp[$ip, $j] -ge $dp[$i, $jp]) { $i++ }
        else { $j++ }
    }
    return ,$anchors
}

# ---------- 3. 逐文件配对 ----------
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
# .rc 资源文件（对话框模板/菜单/字符串表中的中文 UI 串，供 PhTranslateWindowTree 窗口树翻译命中）
$files += @(Get-ChildItem (Join-Path $Root "SystemInformer") -Filter *.rc | ForEach-Object { $_.FullName })
Get-ChildItem (Join-Path $Root "plugins") -Directory | ForEach-Object {
    $files += @(Get-ChildItem $_.FullName -Filter *.rc -Recurse | ForEach-Object { $_.FullName })
}
if (Test-Path (Join-Path $Root "tools")) {
    Get-ChildItem (Join-Path $Root "tools") -Directory | ForEach-Object {
        $files += @(Get-ChildItem $_.FullName -Filter *.rc -Recurse | ForEach-Object { $_.FullName })
    }
}

$dict = @{}          # zh -> en（首个胜出）
$conflicts = New-Object System.Collections.ArrayList
$skips = New-Object System.Collections.ArrayList
$stats = @{ files = 0; missingBase = 0; lits = 0; pairs = 0 }

$utf8 = New-Object System.Text.UTF8Encoding($false)
foreach ($f in $files) {
    $rel = $f.Substring($Root.Length + 1).Replace('\', '/')
    $baseFile = Join-Path $baselineDir ($rel -replace '/', '\')
    if (-not (Test-Path $baseFile)) { $stats.missingBase++; $skips.Add("NEWFILE`t$rel") | Out-Null; continue }
    $stats.files++
    $headLits = Get-Literals ([System.IO.File]::ReadAllText($f, $utf8))
    $baseLits = Get-Literals ([System.IO.File]::ReadAllText($baseFile, $utf8))
    $stats.lits += $headLits.Count
    $anchors = Get-LcsAnchors $headLits.ToArray() $baseLits.ToArray()

    # 构造 gap 段列表：首前导 + 锚点间 + 尾后导
    $segments = New-Object System.Collections.ArrayList
    $prevA = -1; $prevB = -1
    foreach ($an in $anchors) {
        [void]$segments.Add(@(($prevA + 1), ($an[0] - 1), ($prevB + 1), ($an[1] - 1)))
        $prevA = $an[0]; $prevB = $an[1]
    }
    [void]$segments.Add(@(($prevA + 1), ($headLits.Count - 1), ($prevB + 1), ($baseLits.Count - 1)))

    # Pass 1: 数量匹配段（positional pairing，原逻辑，可靠配对优先入字典）
    foreach ($seg in $segments) {
        $h0 = $seg[0]; $h1 = $seg[1]; $b0 = $seg[2]; $b1 = $seg[3]
        $hn = $h1 - $h0 + 1; $bn = $b1 - $b0 + 1
        if ($hn -le 0 -or $hn -ne $bn) { continue }
        for ($k = 0; $k -lt $hn; $k++) {
            $zh = $headLits[$h0 + $k]; $en = $baseLits[$b0 + $k]
            if (-not $script:HasCjk.IsMatch($zh)) { continue }
            if ($script:HasCjk.IsMatch($en)) { $skips.Add("ENCJK`t$rel`t$zh") | Out-Null; continue }
            if (((Get-Placeholders $zh) -join ',') -ne ((Get-Placeholders $en) -join ',')) { $skips.Add("PH`t$rel`t$zh => $en") | Out-Null; continue }
            if ($dict.ContainsKey($zh)) {
                if ($dict[$zh] -ne $en) { $conflicts.Add("[$zh] => '$($dict[$zh])' vs '$en' ($rel)") | Out-Null }
            } else {
                $dict[$zh] = $en; $stats.pairs++
            }
        }
    }
    # Pass 2: COUNT 段（中英字面量数量不匹配，段内就近+占位符签名匹配，仅填补 pass 1 未覆盖的 zh）
    foreach ($seg in $segments) {
        $h0 = $seg[0]; $h1 = $seg[1]; $b0 = $seg[2]; $b1 = $seg[3]
        $hn = $h1 - $h0 + 1; $bn = $b1 - $b0 + 1
        if ($hn -le 0 -and $bn -le 0) { continue }
        if ($hn -eq $bn -and $hn -gt 0) { continue }  # 已在 pass 1 处理
        $usedEn = New-Object 'System.Collections.Generic.HashSet[int]'
        # 预计算 base 段内非 CJK en 的占位符签名（CJK en 不参与匹配）
        $enSigCache = @{}
        for ($e = $b0; $e -le $b1; $e++) {
            $enLit = $baseLits[$e]
            if ($script:HasCjk.IsMatch($enLit)) { continue }
            $enSigCache[$e] = (@(Get-Placeholders $enLit) -join ',')
        }
        for ($k = 0; $k -lt $hn; $k++) {
            $zh = $headLits[$h0 + $k]
            if (-not $script:HasCjk.IsMatch($zh)) { continue }
            $zhPh = @(Get-Placeholders $zh)
            $zhPhSig = $zhPh -join ','
            $maxLenDiff = if ($zhPh.Count -ge 1) { 60 } else { 30 }
            $matchedIdx = -1
            foreach ($e in $enSigCache.Keys) {
                if ($usedEn.Contains($e)) { continue }
                if ($enSigCache[$e] -ne $zhPhSig) { continue }
                $enLit = $baseLits[$e]
                if ([Math]::Abs($enLit.Length - $zh.Length) -gt $maxLenDiff) { continue }
                $matchedIdx = $e; break
            }
            if ($matchedIdx -ge 0) {
                [void]$usedEn.Add($matchedIdx)
                $en = $baseLits[$matchedIdx]
                if ($dict.ContainsKey($zh)) {
                    # pass 1 已覆盖：保留可靠配对，COUNT 模糊匹配仅记冲突供诊断
                    if ($dict[$zh] -ne $en) { $conflicts.Add("[$zh] => '$($dict[$zh])' vs '$en' ($rel)") | Out-Null }
                } else {
                    $dict[$zh] = $en; $stats.pairs++
                }
            } else {
                $skips.Add("COUNT`t$rel`t" + $zh.Replace("`t", " ")) | Out-Null
            }
        }
    }
}

# ---------- 4. 排序 + 输出 ----------
# 按 UTF-16 码元序（与 C 运行时 wcscmp 一致）排序键，供二分查找
$keys = [string[]]$dict.Keys
[Array]::Sort($keys, [StringComparer]::Ordinal)
$sortedList = $keys

$sb = New-Object System.Text.StringBuilder
[void]$sb.AppendLine('/*')
[void]$sb.AppendLine(' * 运行时 UI 文本翻译字典（zh→en）— 自动生成，勿手工编辑。')
[void]$sb.AppendLine(' * 生成器: tools\Localization\gen_translate.ps1')
[void]$sb.AppendLine(' * 基线:   ' + $Baseline)
[void]$sb.AppendLine(' * 规则:   键 = 当前源码硬编码中文；值 = git 基线同位置英文。')
[void]$sb.AppendLine(' * 表按 UTF-16 码元排序，供 translate.c 二分查找。')
[void]$sb.AppendLine(' */')
[void]$sb.AppendLine('')
[void]$sb.AppendLine('#include <ph.h>')
[void]$sb.AppendLine('')
[void]$sb.AppendLine('typedef struct _PH_TRANSLATE_ENTRY')
[void]$sb.AppendLine('{')
[void]$sb.AppendLine('    PCWSTR Zh;')
[void]$sb.AppendLine('    PCWSTR En;')
[void]$sb.AppendLine('} PH_TRANSLATE_ENTRY, *PPH_TRANSLATE_ENTRY;')
[void]$sb.AppendLine('')
[void]$sb.AppendLine('const PH_TRANSLATE_ENTRY PhTranslateTable[] =')
[void]$sb.AppendLine('{')
foreach ($k in $sortedList) {
    [void]$sb.AppendLine('    { ' + (Encode-Literal $k) + ', ' + (Encode-Literal $dict[$k]) + ' },')
}
[void]$sb.AppendLine('};')
[void]$sb.AppendLine('')
[void]$sb.AppendLine('const ULONG PhTranslateTableCount = RTL_NUMBER_OF(PhTranslateTable);')
[System.IO.File]::WriteAllText($outC, $sb.ToString(), (New-Object System.Text.UTF8Encoding($true)))

# ---------- 5. 报告 ----------
$rep = New-Object System.Text.StringBuilder
[void]$rep.AppendLine("基线: $Baseline")
[void]$rep.AppendLine("扫描文件: $($stats.files)（无基线跳过: $($stats.missingBase)）")
[void]$rep.AppendLine("HEAD 字面量总数: $($stats.lits)")
[void]$rep.AppendLine("字典条目: $($stats.pairs)")
[void]$rep.AppendLine("冲突（同键不同译，首个胜出）: $($conflicts.Count)")
$conflicts | ForEach-Object { [void]$rep.AppendLine("  CONFLICT $_") }
[void]$rep.AppendLine("跳过: $($skips.Count)")
$skips | Select-Object -First 200 | ForEach-Object { [void]$rep.AppendLine("  SKIP $_") }
[System.IO.File]::WriteAllText($reportPath, $rep.ToString(), (New-Object System.Text.UTF8Encoding($true)))

Write-Host "entries=$($stats.pairs) conflicts=$($conflicts.Count) skips=$($skips.Count)"
Write-Host "output: $outC"

# ---------- 5. 生成二进制语言文件 ----------
$genLangFile = Join-Path $PSScriptRoot "gen_lang_file.ps1"
if (Test-Path $genLangFile) {
    & $genLangFile -Root $Root
    Write-Host "Generated: lang\zh-en.lang"
}
