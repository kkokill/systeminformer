# 字典覆盖率调查脚本 — 复用 gen_translate.ps1 的 Get-Literals 状态机与 LCS 配对逻辑
# 用法: powershell -File _survey.ps1 [-Root ...] [-Baseline 1487bbf6e]
param(
    [string]$Root = "d:\systeminformer\systeminformer",
    [string]$Baseline = "1487bbf6e"
)
$ErrorActionPreference = 'Stop'

# ---------- 工具函数（复制自 gen_translate.ps1）----------
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

$script:HasCjk = [regex]'[\u3000-\u303f\u3400-\u4dbf\u4e00-\u9fff\uff00-\uffef]'
$script:RxToken = [regex]'(?<nl>\r?\n)|//|/\*|\*/|L?"(?:[^"\\]|\\.)*"'
$script:RxPlaceholders = [regex]'%[-+ 0#]*(\d+|\*)?(\.(\d+|\*))?(hh|h|ll|l|I64|I32|I|w)?[diouxXeEfgGaAcscnp]'

# 编译型 LCS（PowerShell 解释型 DP 过慢）—— 仅此函数下沉到 C#
if (-not ('LcsHelper' -as [type])) {
    Add-Type -Language CSharp -TypeDefinition @'
using System;
using System.Collections.Generic;
public static class LcsHelper {
    public static int[] Anchors(string[] a, string[] b) {
        int n = a.Length, m = b.Length;
        if (n == 0 || m == 0) return new int[0];
        int[,] dp = new int[n + 1, m + 1];
        for (int i = n - 1; i >= 0; i--) {
            int ip = i + 1;
            for (int j = m - 1; j >= 0; j--) {
                int jp = j + 1;
                if (a[i] == b[j]) dp[i, j] = dp[ip, jp] + 1;
                else if (dp[ip, j] >= dp[i, jp]) dp[i, j] = dp[ip, j];
                else dp[i, j] = dp[i, jp];
            }
        }
        List<int> res = new List<int>();
        int ii = 0, jj = 0;
        while (ii < n && jj < m) {
            int iip = ii + 1, jp2 = jj + 1;
            if (a[ii] == b[jj]) { res.Add(ii); res.Add(jj); ii++; jj++; }
            else if (dp[iip, jj] >= dp[ii, jp2]) ii++;
            else jj++;
        }
        return res.ToArray();
    }
}
'@
}

function Get-Placeholders([string]$s) {
    $list = @()
    foreach ($m in $script:RxPlaceholders.Matches($s)) { $list += $m.Value.ToLower() }
    return ($list | Sort-Object)
}

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

function Get-LcsAnchors([string[]]$a, [string[]]$b) {
    $flat = [LcsHelper]::Anchors([string[]]$a, [string[]]$b)
    $anchors = New-Object System.Collections.ArrayList
    for ($k = 0; $k + 1 -lt $flat.Length; $k += 2) {
        [void]$anchors.Add(@($flat[$k], $flat[$k + 1]))
    }
    return ,$anchors
}

# ---------- 收集 .c 文件 ----------
$files = @(Get-ChildItem (Join-Path $Root "SystemInformer") -Filter *.c | ForEach-Object { $_.FullName })
Get-ChildItem (Join-Path $Root "plugins") -Directory | ForEach-Object {
    $files += @(Get-ChildItem $_.FullName -Filter *.c -Recurse | ForEach-Object { $_.FullName })
}
Write-Host "扫描 .c 文件数: $($files.Count)"

# ---------- 解析 translate_data.c 的 Zh 键 ----------
$dictPath = Join-Path $Root "phlib\translate_data.c"
$dictText = [System.IO.File]::ReadAllText($dictPath, (New-Object System.Text.UTF8Encoding($false)))
$rxEntry = [regex]'\{\s*L"((?:[^"\\]|\\.)*)"\s*,\s*L"((?:[^"\\]|\\.)*)"\s*\}'
$dictKeys = New-Object 'System.Collections.Generic.HashSet[string]'
foreach ($m in $rxEntry.Matches($dictText)) {
    $zh = Decode-Literal ('L"' + $m.Groups[1].Value + '"')
    if ($null -ne $zh) { [void]$dictKeys.Add($zh) }
}
Write-Host "字典 Zh 键数: $($dictKeys.Count)"

# ---------- Part A: 覆盖率统计 ----------
$utf8 = New-Object System.Text.UTF8Encoding($false)
$allCjk = New-Object 'System.Collections.Generic.HashSet[string]'
$perFileMissing = @{}                          # rel -> HashSet
$missingSamples = New-Object System.Collections.ArrayList
$missingSampleSeen = New-Object 'System.Collections.Generic.HashSet[string]'

foreach ($f in $files) {
    $rel = $f.Substring($Root.Length + 1).Replace('\', '/')
    $text = [System.IO.File]::ReadAllText($f, $utf8)
    $lits = Get-Literals $text
    $fileMissing = New-Object 'System.Collections.Generic.HashSet[string]'
    foreach ($l in $lits) {
        if (-not $script:HasCjk.IsMatch($l)) { continue }
        [void]$allCjk.Add($l)
        if (-not $dictKeys.Contains($l)) {
            [void]$fileMissing.Add($l)
            if ($missingSamples.Count -lt 10 -and $missingSampleSeen.Add($l)) {
                [void]$missingSamples.Add($l)
            }
        }
    }
    if ($fileMissing.Count -gt 0) { $perFileMissing[$rel] = $fileMissing }
}

$totalCjk = $allCjk.Count
$matched = 0
foreach ($l in $allCjk) { if ($dictKeys.Contains($l)) { $matched++ } }
$missing = $totalCjk - $matched

Write-Host ""
Write-Host "==== Part A: 字典覆盖率 ===="
Write-Host ("中文字面量总数(去重): {0}" -f $totalCjk)
Write-Host ("字典已收录: {0}" -f $matched)
Write-Host ("漏失数: {0}" -f $missing)
Write-Host ("覆盖率: {0:P2}" -f ($matched / [Math]::Max($totalCjk,1)))

$topFiles = $perFileMissing.GetEnumerator() | Sort-Object { $_.Value.Count } -Descending | Select-Object -First 10
Write-Host ""
Write-Host "漏失 Top 10 文件 (漏失数  文件):"
foreach ($e in $topFiles) { Write-Host ("  {0,-5} {1}" -f $e.Value.Count, $e.Key) }

Write-Host ""
Write-Host "漏失样本 10 条:"
foreach ($s in $missingSamples) {
    $disp = $s -replace "`t", '\t' -replace "`r", '\r' -replace "`n", '\n'
    Write-Host ("  " + $disp)
}

# ---------- Part B: 跳过类别分布（复现 gen_translate 配对逻辑）----------
$baselineDir = Join-Path $env:TEMP "si_survey_$Baseline"
$tarFile = Join-Path $env:TEMP "si_survey.tar"
if (Test-Path $baselineDir) { Remove-Item -Recurse -Force $baselineDir }
New-Item -ItemType Directory $baselineDir | Out-Null
cmd /c "git -C `"$Root`" archive $Baseline SystemInformer plugins > `"$tarFile`""
if ($LASTEXITCODE -ne 0) { throw "git archive failed (baseline $Baseline)" }
tar -xf $tarFile -C $baselineDir
if ($LASTEXITCODE -ne 0) { throw "tar extract failed" }

$skipCounts = @{ NEWFILE = 0; ENCJK = 0; COUNT = 0; PH = 0 }
$skipEncjkSamples = New-Object System.Collections.ArrayList
$skipCountSamples = New-Object System.Collections.ArrayList
$encjkSeen = New-Object 'System.Collections.Generic.HashSet[string]'
$countSeen = New-Object 'System.Collections.Generic.HashSet[string]'

foreach ($f in $files) {
    $rel = $f.Substring($Root.Length + 1).Replace('\', '/')
    $baseFile = Join-Path $baselineDir ($rel -replace '/', '\')
    if (-not (Test-Path $baseFile)) { $skipCounts.NEWFILE++; continue }
    $headLits = Get-Literals ([System.IO.File]::ReadAllText($f, $utf8))
    $baseLits = Get-Literals ([System.IO.File]::ReadAllText($baseFile, $utf8))
    $anchors = Get-LcsAnchors $headLits.ToArray() $baseLits.ToArray()
    $segments = New-Object System.Collections.ArrayList
    $prevA = -1; $prevB = -1
    foreach ($an in $anchors) {
        [void]$segments.Add(@(($prevA + 1), ($an[0] - 1), ($prevB + 1), ($an[1] - 1)))
        $prevA = $an[0]; $prevB = $an[1]
    }
    [void]$segments.Add(@(($prevA + 1), ($headLits.Count - 1), ($prevB + 1), ($baseLits.Count - 1)))
    foreach ($seg in $segments) {
        $h0 = $seg[0]; $h1 = $seg[1]; $b0 = $seg[2]; $b1 = $seg[3]
        $hn = $h1 - $h0 + 1; $bn = $b1 - $b0 + 1
        if ($hn -le 0 -and $bn -le 0) { continue }
        if ($hn -ne $bn -or $hn -le 0) {
            for ($k = 0; $k -lt [Math]::Max($hn, $bn); $k++) {
                $hv = if ($h0 + $k -le $h1) { $headLits[$h0 + $k] } else { $null }
                if ($hv -and $script:HasCjk.IsMatch($hv)) {
                    $skipCounts.COUNT++
                    if ($skipCountSamples.Count -lt 5 -and $countSeen.Add($hv)) {
                        [void]$skipCountSamples.Add("$hv  [$rel]")
                    }
                }
            }
            continue
        }
        for ($k = 0; $k -lt $hn; $k++) {
            $zh = $headLits[$h0 + $k]; $en = $baseLits[$b0 + $k]
            if (-not $script:HasCjk.IsMatch($zh)) { continue }
            if ($script:HasCjk.IsMatch($en)) {
                $skipCounts.ENCJK++
                if ($skipEncjkSamples.Count -lt 5 -and $encjkSeen.Add($zh)) {
                    [void]$skipEncjkSamples.Add("$zh  =>  $en  [$rel]")
                }
                continue
            }
            if (((Get-Placeholders $zh) -join ',') -ne ((Get-Placeholders $en) -join ',')) { $skipCounts.PH++; continue }
        }
    }
}

Write-Host ""
Write-Host "==== Part B: 跳过类别分布 (基线 $Baseline) ===="
Write-Host ("NEWFILE  (无基线文件): {0}" -f $skipCounts.NEWFILE)
Write-Host ("ENCJK    (基线英文也是中文，未翻译): {0}" -f $skipCounts.ENCJK)
Write-Host ("COUNT    (中英字面量数量不匹配，无法配对): {0}" -f $skipCounts.COUNT)
Write-Host ("PH       (占位符不匹配): {0}" -f $skipCounts.PH)
Write-Host ""
Write-Host "ENCJK 样本:"
foreach ($s in $skipEncjkSamples) { Write-Host ("  " + ($s -replace "`t", ' ')) }
Write-Host "COUNT 样本:"
foreach ($s in $skipCountSamples) { Write-Host ("  " + ($s -replace "`t", ' ')) }

# ---------- 清理临时基线 ----------
Remove-Item -Recurse -Force $baselineDir
Remove-Item -Force $tarFile
Write-Host ""
Write-Host "已清理临时基线目录: $baselineDir"
