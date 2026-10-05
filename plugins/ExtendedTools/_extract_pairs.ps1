# 提取 ExtendedTools fork(中文) vs 上游(英文) 的 (En, Zh) 文本配对
$ErrorActionPreference = 'Stop'
$plugin = "d:\systeminformer\systeminformer\plugins\ExtendedTools"
$upstream = "C:\Users\Joe\Desktop\systeminformer\plugins\ExtendedTools"
$git = "C:\Program Files\Git\cmd\git.exe"

$codeFiles = @(
    'acpitable.c','cacheprp.c','counters.c','disktab.c','etwdisk.c','etwmini.c','etwmon.c',
    'firmware.c','firmware_editor.c','frameprp.c','fwmon.c','fwtab.c','gpudetails.c','gpumini.c',
    'gpumon.c','gpunodes.c','gpuprprp.c','gpusys.c','iconext.c','main.c','modsrv.c','namedpipes.c',
    'npudetails.c','npumini.c','npumon.c','npunodes.c','npuprprp.c','npusys.c','pooldb.c',
    'pooldialog.c','pooldialogbig.c','pooltree.c','pwrgrid.c','reparse.c','smbios.c','svcext.c',
    'tpm.c','tpm_editor.c','wbcl.c','treeext.c','etwstat.c','etwprprp.c','etwsys.c','options.c',
    'thrdact.c','unldll.c','utils.c','waitchain.c','wswatch.c','exttools.h',
    'ExtendedTools.rc'
)

function Unescape([string]$s) {
    $sb = New-Object System.Text.StringBuilder
    for ($i = 0; $i -lt $s.Length; $i++) {
        $c = $s[$i]
        if ($c -eq [char]92 -and $i + 1 -lt $s.Length) {
            $n = $s[$i+1]
            if ($n -eq 't') { [void]$sb.Append([char]9); $i++ }
            elseif ($n -eq 'n') { [void]$sb.Append([char]10); $i++ }
            elseif ($n -eq 'r') { [void]$sb.Append([char]13); $i++ }
            elseif ($n -eq [char]92) { [void]$sb.Append([char]92); $i++ }
            elseif ($n -eq '"') { [void]$sb.Append('"'); $i++ }
            elseif ($n -eq '0') { [void]$sb.Append([char]0); $i++ }
            else { [void]$sb.Append($n); $i++ }
        } else { [void]$sb.Append($c) }
    }
    $sb.ToString()
}

function ExtractLiteral([string]$line) {
    $parts = [regex]::Matches($line, 'L"((?:[^"\\]|\\.)*)"')
    if ($parts.Count -eq 0) { return $null }
    $joined = ''
    foreach ($m in $parts) { $joined += $m.Groups[1].Value }
    Unescape $joined
}

# 无字面量行也占位 $false，保证 hunk 内 -/+ 逐行镜像对齐；配对要求两侧均为文本
function ExtractSlot([string]$line) {
    $t = ExtractLiteral $line
    if ($null -eq $t) { return $false } else { return $t }
}

$pairs = @{}
$mismatch = New-Object System.Collections.Generic.List[string]

function ProcessHunk([string]$f, $delTexts, $addTexts) {
    $n = [Math]::Min($delTexts.Count, $addTexts.Count)
    $max = [Math]::Max($delTexts.Count, $addTexts.Count)
    for ($k = 0; $k -lt $max; $k++) {
        $e = $false; $z = $false
        if ($k -lt $delTexts.Count) { $e = $delTexts[$k] }
        if ($k -lt $addTexts.Count) { $z = $addTexts[$k] }
        if ($e -is [string] -and $z -is [string] -and $e -ne $z) {
            $key = "$e`0$z"
            if ($script:pairs.ContainsKey($key)) { $script:pairs[$key]++ } else { $script:pairs[$key] = 1 }
        } elseif (($e -is [string] -or $z -is [string]) -and $e -ne $z) {
            $es = ''; $zs = ''
            if ($e -is [string]) { $es = $e }
            if ($z -is [string]) { $zs = $z }
            $script:mismatch.Add("UNEQ|$f|$es|$zs")
        }
    }
}

foreach ($f in $codeFiles) {
    $upPath = Join-Path $upstream $f
    $forkPath = Join-Path $plugin $f
    if (-not (Test-Path $upPath)) { Write-Host "SKIP(no upstream): $f"; continue }
    if (-not (Test-Path $forkPath)) { Write-Host "SKIP(no fork): $f"; continue }

    $diff = & $git diff --no-index -U0 -- $upPath $forkPath 2>$null | Out-String
    $delTexts = New-Object System.Collections.Generic.List[string]
    $addTexts = New-Object System.Collections.Generic.List[string]

    foreach ($line in ($diff -split "`n")) {
        $line = $line.TrimEnd("`r")
        if ($line -match '^diff |^index |^--- |^\+\+\+ ') { continue }
        if ($line.StartsWith('@')) {
            ProcessHunk $f $delTexts $addTexts
            $delTexts.Clear(); $addTexts.Clear()
            continue
        }
        if ($line.StartsWith('-')) {
            $delTexts.Add((ExtractSlot ($line.Substring(1))))
        } elseif ($line.StartsWith('+')) {
            $addTexts.Add((ExtractSlot ($line.Substring(1))))
        }
    }
    # flush tail
    ProcessHunk $f $delTexts $addTexts
}

$outList = New-Object System.Collections.Generic.List[string]
foreach ($kv in $pairs.GetEnumerator()) {
    $parts = $kv.Key -split "`0"
    $outList.Add("$($parts[0])`t$($parts[1])")
}
$sorted = $outList.ToArray()
[System.Array]::Sort($sorted, [System.StringComparer]::Ordinal)
[System.IO.File]::WriteAllLines((Join-Path $plugin "_pairs.txt"), $sorted, (New-Object System.Text.UTF8Encoding($true)))
[System.IO.File]::WriteAllLines((Join-Path $plugin "_pairs_unequal.txt"), $mismatch.ToArray(), (New-Object System.Text.UTF8Encoding($true)))
Write-Host "配对数: $($sorted.Count)  UNEQ: $($mismatch.Count)"
