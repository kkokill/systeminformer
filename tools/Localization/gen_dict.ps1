# 生成中英翻译字典：从 git 基线(英文)到 HEAD(中文) 的 diff 中自动配对字符串
# 用法: powershell -File gen_dict.ps1 [-Baseline 1487bbf6e] [-OutDir tools\Localization]
# 输出:
#   dict_pairs.tsv   zh<TAB>en<TAB>file 全量配对（含重复，供审查）
#   dict_stats.txt   统计与配对失败报告

param(
    [string]$Baseline = "1487bbf6e",
    [string]$OutDir = "$PSScriptRoot"
)

$ErrorActionPreference = 'Stop'
$repo = "d:\systeminformer\systeminformer"
$diffFile = Join-Path $env:TEMP "si_dict_diff.txt"

Write-Host "Generating diff $Baseline -> HEAD ..."
git -C $repo diff $Baseline HEAD -U0 --no-color --no-prefix > $diffFile
if ($LASTEXITCODE -ne 0) { throw "git diff failed" }

$lines = [System.IO.File]::ReadAllLines($diffFile)
Write-Host "Diff lines: $($lines.Count)"

$cjkRegex = [regex]'[\u4E00-\u9FFF\u3000-\u303F\uFF00-\uFFEF]'
# C 宽/窄字符串字面量（含转义）；.rc 同样适用
$litRegex = [regex]'L?"((\\.|[^"\\])*)"'

function Get-Literals([string]$s) {
    # 返回该行中所有字符串字面量的内部内容（源码转义形式原样保留）
    $result = @()
    foreach ($m in $litRegex.Matches($s)) {
        $result += ,@($m.Groups[0].Value.StartsWith('L'), $m.Groups[1].Value)
    }
    return ,$result
}

$pairs = New-Object System.Collections.Generic.List[object]   # @{Zh;En;File}
$unmatchedZh = New-Object System.Collections.Generic.List[object] # 新增代码中的中文（无英文对照）
$curFile = ''
$oldLines = @()
$newLines = @()
$inHunk = $false
$pairFailHunks = 0
$pairOkHunks = 0
$newFileMode = $false

function Flush-Hunk {
    if (-not $script:inHunk) { return }
    $script:inHunk = $false
    if ($script:newFileMode) {
        # 纯新增文件：所有中文串记为 unmatched
        foreach ($l in $script:newLines) {
            foreach ($lit in (Get-Literals $l)) {
                if ($lit[1] -match $cjkRegex) {
                    $script:unmatchedZh.Add(@{ Zh = $lit[1]; File = $script:curFile })
                }
            }
        }
        return
    }
    if ($script:oldLines.Count -eq 0 -and $script:newLines.Count -eq 0) { return }
    $oldLits = @(); $newLits = @()
    foreach ($l in $script:oldLines) { $oldLits += (Get-Literals $l) }
    foreach ($l in $script:newLines) { $newLits += (Get-Literals $l) }
    if ($oldLits.Count -gt 0 -and $oldLits.Count -eq $newLits.Count) {
        $script:pairOkHunks++
        for ($i = 0; $i -lt $oldLits.Count; $i++) {
            $en = $oldLits[$i][1]; $zh = $newLits[$i][1]; $wide = $newLits[$i][0]
            if ($script:pairOkHunks -le 3 -and $i -lt 3) {
                Write-Host ("DBG hunk#{0} i={1} en=[{2}] zh=[{3}] eq={4} zhCJK={5}" -f $script:pairOkHunks, $i, $en, $zh, ($zh -ceq $en), ($zh -match $cjkRegex))
            }
            if ($zh -ceq $en) { continue }                    # 未变化的串
            if (-not ($zh -match $cjkRegex)) { continue }     # 新串不含中文（非翻译改动）
            if ($en -match $cjkRegex) { continue }            # 异常：旧串也含中文
            $script:pairs.Add(@{ Zh = $zh; En = $en; File = $script:curFile })
            if ($script:pairs.Count -le 1) { Write-Host "DBG2 type=$($script:pairs.GetType().FullName) topType=$($pairs.GetType().FullName) sameRef=$([object]::ReferenceEquals($script:pairs, $pairs))" }
        }
    }
    else {
        # 配对失败：收集新增侧孤立的中文串
        $script:pairFailHunks++
        $hadZh = $false
        foreach ($lit in $newLits) {
            if ($lit[1] -match $cjkRegex) {
                $hadZh = $true
                $script:unmatchedZh.Add(@{ Zh = $lit[1]; File = $script:curFile })
            }
        }
        if ($hadZh) {
            Write-Host ("MISMATCH hunk in {0}: oldLits={1} newLits={2} (oldLines={3} newLines={4})" -f `
                $script:curFile, $oldLits.Count, $newLits.Count, $script:oldLines.Count, $script:newLines.Count)
        }
    }
}

foreach ($line in $lines) {
    if ($line.StartsWith('diff --git ')) {
        Flush-Hunk
        $parts = $line -split ' '
        $script:curFile = $parts[-1]
        $script:newFileMode = $false
    }
    elseif ($line.StartsWith('new file mode')) { $script:newFileMode = $true }
    elseif ($line.StartsWith('deleted file mode')) { $script:newFileMode = $false; $script:curFile = '' }
    elseif ($line.StartsWith('@@')) {
        Flush-Hunk
        $script:inHunk = $true
        $script:oldLines = @(); $script:newLines = @()
    }
    elseif ($script:inHunk) {
        if ($line.StartsWith('-')) { $script:oldLines += $line.Substring(1) }
        elseif ($line.StartsWith('+')) { $script:newLines += $line.Substring(1) }
        # 其余（hunk 头 \ No newline 等）忽略
    }
}
Flush-Hunk

Write-Host ""
Write-Host "Hunks paired: $pairOkHunks, failed: $pairFailHunks"
Write-Host "Raw pairs: $($pairs.Count) id=$($pairs.GetHashCode())"
Write-Host "Unmatched zh strings (new code / failed hunks): $($unmatchedZh.Count)"

# 唯一化与冲突检测
$map = @{}   # zh -> en
$conflicts = New-Object System.Collections.Generic.List[string]
foreach ($p in $pairs) {
    if ($map.ContainsKey($p.Zh)) {
        if ($map[$p.Zh] -ne $p.En) {
            $conflicts.Add("$($p.Zh) => [$($map[$p.Zh])] vs [$($p.En)] ($($p.File))")
        }
    } else {
        $map[$p.Zh] = $p.En
    }
}
Write-Host "Unique zh keys: $($map.Count), conflicts: $($conflicts.Count)"

# 输出 TSV
$tsv = Join-Path $OutDir 'dict_pairs.tsv'
$sw = New-Object System.IO.StreamWriter($tsv, $false, (New-Object System.Text.UTF8Encoding($true)))
$sw.WriteLine("zh`ten`tpairfile")
foreach ($p in $pairs) { $sw.WriteLine("$($p.Zh)`t$($p.En)`t$($p.File)") }
$sw.Close()

# 输出统计
$stats = Join-Path $OutDir 'dict_stats.txt'
$sw = New-Object System.IO.StreamWriter($stats, $false, (New-Object System.Text.UTF8Encoding($true)))
$sw.WriteLine("== 配对冲突 (同 zh 不同 en)，取先出现的 ==")
foreach ($c in $conflicts) { $sw.WriteLine($c) }
$sw.WriteLine("")
$sw.WriteLine("== 未匹配中文串（新增代码/配对失败 hunk），英文模式将回退中文 ==")
$seen = @{}
foreach ($u in $unmatchedZh) {
    $k = "$($u.Zh)|$($u.File)"
    if ($seen.ContainsKey($k)) { continue }
    $seen[$k] = 1
    $sw.WriteLine("$($u.Zh)`t$($u.File)")
}
$sw.Close()

Write-Host "Written: $tsv"
Write-Host "Written: $stats"
