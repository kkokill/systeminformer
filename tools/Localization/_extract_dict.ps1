# Extract the embedded PH_TRANSLATE_ENTRY table from a built SystemInformer.exe
# and emit it as force-override TSV lines (recovers lost translate_data.c state).
# Usage: powershell -File _extract_dict.ps1 -ExePath <exe> -OutTsv <out.tsv>
# ASCII-only script (no Chinese literals) so it parses correctly without BOM.
param(
    [Parameter(Mandatory = $true)][string]$ExePath,
    [Parameter(Mandatory = $true)][string]$OutTsv
)

$ErrorActionPreference = 'Stop'
$b = [System.IO.File]::ReadAllBytes($ExePath)

function I32([byte[]]$buf, [int]$off) { [BitConverter]::ToInt32($buf, $off) }
function I64([byte[]]$buf, [int]$off) { [BitConverter]::ToInt64($buf, $off) }

$peOff = I32 $b 0x3C
if ((I32 $b $peOff) -ne 0x00004550) { throw "not a PE file" }
$optOff = $peOff + 0x18
$magic = [BitConverter]::ToUInt16($b, $optOff)
if ($magic -ne 0x20B) { throw "not PE32+ (x64)" }
$imageBase = I64 $b ($optOff + 0x18)
$numSec = [BitConverter]::ToUInt16($b, $peOff + 6)
$optSize = [BitConverter]::ToUInt16($b, $peOff + 0x14)
$sec0 = $optOff + $optSize

$sections = @()
for ($i = 0; $i -lt $numSec; $i++) {
    $s = $sec0 + 40 * $i
    $name = [System.Text.Encoding]::ASCII.GetString($b, $s, 8).TrimEnd([char]0)
    $vsize = I32 $b ($s + 8); $vaddr = I32 $b ($s + 12)
    $rawsize = I32 $b ($s + 16); $rawptr = I32 $b ($s + 20)
    if ($rawsize -gt 0) { $sections += [PSCustomObject]@{ Name = $name; VA = $vaddr; VS = [Math]::Max($vsize, $rawsize); RP = $rawptr; RS = $rawsize } }
}

function ConvertTo-Off([long]$va) {
    $rva = $va - $imageBase
    foreach ($s in $sections) {
        if ($rva -ge $s.VA -and $rva -lt ($s.VA + $s.VS)) {
            $o = $s.RP + ($rva - $s.VA)
            if ($o -ge 0 -and $o -lt ($s.RP + $s.RS)) { return $o }
            return -1
        }
    }
    return -1
}

# harvest UTF-16 NUL-terminated strings: va -> string (fast path only)
$strByVa = @{}
foreach ($s in $sections) {
    $end = $s.RP + $s.RS
    $o = $s.RP
    while ($o -lt ($end - 2)) {
        $start = $o
        $len = 0
        while ($o -lt $end) {
            $c = [BitConverter]::ToUInt16($b, $o)
            if ($c -eq 0) { break }
            $o += 2; $len++
            if ($len -gt 8192) { break }
        }
        if ($len -ge 1 -and $len -le 8192) {
            $str = [System.Text.Encoding]::Unicode.GetString($b, $start, $len * 2)
            $va = $imageBase + $s.VA + ($start - $s.RP)
            if (-not $strByVa.ContainsKey($va)) { $strByVa[$va] = $str }
        }
        $o += 2
    }
}
Write-Output ("strings harvested: " + $strByVa.Count)

function Has-Cjk([string]$t) {
    foreach ($ch in $t.ToCharArray()) {
        $c = [int]$ch
        if (($c -ge 0x3000 -and $c -le 0x303f) -or ($c -ge 0x3400 -and $c -le 0x4dbf) -or ($c -ge 0x4e00 -and $c -le 0x9fff) -or ($c -ge 0xff00 -and $c -le 0xffef)) { return $true }
    }
    return $false
}

# read a NUL-terminated UTF-16 string at VA directly from the file.
# Handles /GF suffix pooling (pointer into the middle of a longer literal),
# which the run-start harvest map cannot index.
function Get-Str([long]$va) {
    if ($strByVa.ContainsKey($va)) { return $strByVa[$va] }
    $o = ConvertTo-Off $va
    if ($o -lt 2 -or ($o -band 1) -ne 0) { return $null }
    $len = 0
    while ($o + 2 * $len + 1 -lt $b.Length) {
        $c = [BitConverter]::ToUInt16($b, $o + 2 * $len)
        if ($c -eq 0) { break }
        $len++
        if ($len -gt 8192) { return $null }
    }
    if ($len -ge 1) { return [System.Text.Encoding]::Unicode.GetString($b, $o, $len * 2) }
    return $null
}

function Get-Pair([long]$off) {
    # returns @{ Key; Val; Ok } for a 16-byte entry at file offset $off
    if (($off + 16) -gt $b.Length) { return $null }
    $p1 = I64 $b $off; $p2 = I64 $b ($off + 8)
    if (($p1 -band 1) -ne 0 -or ($p2 -band 1) -ne 0) { return $null }
    $s1 = Get-Str $p1; $s2 = Get-Str $p2
    if ($null -eq $s1 -or $null -eq $s2 -or -not (Has-Cjk $s1)) { return $null }
    return @{ Key = $s1; Val = $s2; P1 = $p1 }
}

# 1. forward scan for the longest strictly-Ordinal-increasing valid run
$best = @{ start = -1; len = 0; sec = "" }
foreach ($s in $sections) {
    if ($s.Name -notin @('.rdata', '.data')) { continue }
    $runStart = -1; $runLen = 0; $lastKey = $null
    $o = $s.RP
    $limit = $s.RP + $s.RS
    while (($o + 16) -le $limit) {
        $e = Get-Pair $o
        $ok = $null -ne $e
        if ($ok -and ($runLen -eq 0 -or [string]::CompareOrdinal($e.Key, $lastKey) -gt 0)) {
            if ($runLen -eq 0) { $runStart = $o }
            $runLen++; $lastKey = $e.Key
            $o += 16
        } else {
            if ($runLen -gt $best.len) { $best.start = $runStart; $best.len = $runLen; $best.sec = $s.Name }
            $runStart = -1; $runLen = 0; $lastKey = $null
            if ($ok) { $runStart = $o; $runLen = 1; $lastKey = $e.Key; $o += 16 } else { $o += 8 }
        }
    }
    if ($runLen -gt $best.len) { $best.start = $runStart; $best.len = $runLen; $best.sec = $s.Name }
}

if ($best.len -lt 5000) { throw ("translate table not found (best run=" + $best.len + " at " + $best.start + ")") }
Write-Output ("table found: section=" + $best.sec + " entries=" + $best.len)

# 2. extend the run backward to the true array head (stride/alignment sync)
$head = $best.start
$first = Get-Pair $head
while ($head -ge 16) {
    $cand = $head - 16
    $e = Get-Pair $cand
    if ($e -and [string]::CompareOrdinal($e.Key, $first.Key) -lt 0) { $head = $cand; $first = $e; continue }
    # parity off by 8: candidate at -8 shows {En_prev, Zh_cur}; its p2 == current p1
    $cand8 = $head - 8
    $e8 = Get-Pair $cand8
    if ($e8 -and $e8.P1 -eq $first.P1) { $head = $head - 16; $first = Get-Pair $head; continue }
    break
}
if ($head -ne $best.start) {
    $best.len = $best.len + ($best.start - $head) / 16
    $best.start = $head
    Write-Output ("head extended: entries=" + $best.len)
}

# 3. emit TSV (force)
$sb = New-Object System.Text.StringBuilder
for ($i = 0; $i -lt $best.len; $i++) {
    $e = Get-Pair ($best.start + 16 * $i)
    if ($null -eq $e) { throw ("broken entry at index " + $i) }
    $k = $e.Key -replace '\\', '\\\\' -replace '"', '\"' -replace "`t", '\t' -replace "`r", '\r' -replace "`n", '\n' -replace ([string][char]8), '\b'
    $v = $e.Val -replace '\\', '\\\\' -replace '"', '\"' -replace "`t", '\t' -replace "`r", '\r' -replace "`n", '\n' -replace ([string][char]8), '\b'
    [void]$sb.Append($k); [void]$sb.Append("`t"); [void]$sb.Append($v); [void]$sb.Append("`tforce`r`n")
}
[System.IO.File]::WriteAllText($OutTsv, $sb.ToString(), (New-Object System.Text.UTF8Encoding($false)))
Write-Output ("written: " + $OutTsv + " (" + $best.len + " entries)")
