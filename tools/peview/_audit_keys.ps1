# Audit: collect all PhTranslateTextZ/RawZ literal keys used by main program + phlib,
# compare against translate_data.c keys. ASCII-only script (CJK matched via unicode class).
$ErrorActionPreference = 'Stop'

$root = 'd:\systeminformer\systeminformer'
$dictPath = Join-Path $root 'phlib\translate_data.c'

function Decode-CEscapes([string]$s) {
    # decode C string escapes to runtime chars (same semantics as C compiler)
    $sb = New-Object System.Text.StringBuilder
    for ($i = 0; $i -lt $s.Length; $i++) {
        $c = $s[$i]
        if ($c -eq '\' -and $i + 1 -lt $s.Length) {
            $i++
            $n = $s[$i]
            switch ($n) {
                'n' { [void]$sb.Append("`n") }
                'r' { [void]$sb.Append("`r") }
                't' { [void]$sb.Append("`t") }
                'b' { [void]$sb.Append([char]8) }
                'f' { [void]$sb.Append([char]12) }
                'v' { [void]$sb.Append([char]11) }
                'a' { [void]$sb.Append([char]7) }
                '\' { [void]$sb.Append('\') }
                '"' { [void]$sb.Append('"') }
                "'" { [void]$sb.Append("'") }
                '?' { [void]$sb.Append('?') }
                'x' {
                    $hex = ''
                    while ($i + 1 -lt $s.Length -and $s[$i+1] -match '[0-9a-fA-F]') { $i++; $hex += $s[$i] }
                    if ($hex) { [void]$sb.Append([char][Convert]::ToInt32($hex, 16)) }
                }
                default {
                    if ($n -match '[0-7]') {
                        $oct = ''
                        while ($i + 1 -lt $s.Length -and $s[$i+1] -match '[0-7]' -and $oct.Length -lt 2) { $i++; $oct += $s[$i] }
                        [void]$sb.Append([char][Convert]::ToInt32($oct, 8))
                    } else { [void]$sb.Append($n) }
                }
            }
        } else { [void]$sb.Append($c) }
    }
    return $sb.ToString()
}

# --- parse dictionary ---
$dictLines = [System.IO.File]::ReadAllLines($dictPath)
$dict = @{}   # decodedZh -> psobj(rawZh, rawEn, decodedEn, lineNo)
$entryRe = [regex]'^\s*\{\s*L"((?:[^"\\]|\\.)*)"\s*,\s*L"((?:[^"\\]|\\.)*)"\s*\}'
for ($i = 0; $i -lt $dictLines.Length; $i++) {
    $m = $entryRe.Match($dictLines[$i])
    if ($m.Success) {
        $rawZh = $m.Groups[1].Value
        $dz = Decode-CEscapes $rawZh
        if (-not $dict.ContainsKey($dz)) {
            $dict[$dz] = [pscustomobject]@{ RawZh=$rawZh; RawEn=$m.Groups[2].Value; DecodedEn=(Decode-CEscapes $m.Groups[2].Value); Line=($i+1) }
        } else {
            Write-Output ("DUPDICT line {0}: {1}" -f ($i+1), $rawZh)
        }
    }
}
Write-Output ("DICT entries: {0}" -f $dict.Count)

# --- scan source files for PhTranslateTextZ / PhTranslateTextRawZ literal args ---
$lit = 'L"(?:[^"\\\n]|\\.)*"'   # single-line literal (no raw newline inside)
$callRe = [regex]('PhTranslateText(?:Raw)?Z\(\s*((?:' + $lit + '\s*)+)\)')
$files = @(Get-ChildItem (Join-Path $root 'SystemInformer') -Recurse -Include *.c,*.h -File | Where-Object { $_.FullName -notmatch '\\(obj|bin)\\' })
$files += @(Get-ChildItem (Join-Path $root 'phlib') -File -Include *.c,*.h | Where-Object { $_.Name -ne 'translate_data.c' })
$used = @{}   # decodedKey -> list of "file:line"
foreach ($f in $files) {
    $lines = [System.IO.File]::ReadAllLines($f.FullName)
    for ($i = 0; $i -lt $lines.Length; $i++) {
        $line = $lines[$i]
        if (-not ($line -match 'PhTranslateText')) { continue }
        foreach ($m in $callRe.Matches($line)) {
            $litStr = $m.Groups[1].Value
            $lits = [regex]::Matches($litStr, $lit)
            $key = ''
            foreach ($lm in $lits) { $key += Decode-CEscapes ($lm.Value.Substring(2, $lm.Value.Length - 3).Replace('""','"')) }
            if (-not $used.ContainsKey($key)) { $used[$key] = New-Object System.Collections.Generic.List[string] }
            $used[$key].Add(('{0}:{1}' -f $f.Name, ($i+1)))
        }
    }
}
Write-Output ("SOURCE keys used: {0}" -f $used.Count)

# --- report ---
$missing = New-Object System.Collections.Generic.List[string]
foreach ($k in $used.Keys) {
    if (-not $dict.ContainsKey($k)) { $missing.Add($k) }
}
$sorted = $missing | Sort-Object
Write-Output ("MISSING from dict: {0}" -f $missing.Count)
foreach ($k in $sorted) {
    $vis = $k -replace "`r", '\r' -replace "`n", '\n' -replace "`t", '\t'
    $loc = ($used[$k] | Select-Object -First 3) -join ', '
    Write-Output ("MISS|{0}|{1}" -f $vis, $loc)
}
# also report keys used in source but dict value suspiciously equal (identity entries)
$ident = 0
foreach ($k in $used.Keys) {
    if ($dict.ContainsKey($k) -and $dict[$k].DecodedEn -ceq $k) { $ident++; Write-Output ("IDENT|{0}|dictline {1}" -f ($k -replace "`n",'\n'), $dict[$k].Line) }
}
Write-Output ("IDENT entries: {0}" -f $ident)
