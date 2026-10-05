$ErrorActionPreference = 'Stop'
$dir = 'd:\systeminformer\systeminformer\plugins\ExtendedTools'
$dictPath = 'd:\systeminformer\systeminformer\phlib\translate_data.c'

# 1. collect wrapped keys: PhTranslateTextZ(L"...")
$keys = New-Object 'System.Collections.Generic.HashSet[string]'
$files = Get-ChildItem $dir -Filter *.c | Where-Object { $_.Name -notin @('objmgr.c','objprp.c') }
foreach ($f in $files) {
    $text = [IO.File]::ReadAllText($f.FullName, (New-Object Text.UTF8Encoding($true)))
    foreach ($m in [regex]::Matches($text, 'PhTranslateTextZ\(L"((?:[^"\\]|\\.)*)"')) {
        [void]$keys.Add($m.Groups[1].Value)
    }
}

# 2. data table entries shown via wrapped sinks (smbios/acpitable SIP values, fwtab SIP tables, tpm ShortName, treeext column table)
foreach ($f in @('smbios.c','acpitable.c','fwtab.c','tpm.c','treeext.c','wbcl.c')) {
    $p = Join-Path $dir $f
    if (-not (Test-Path $p)) { continue }
    $text = [IO.File]::ReadAllText($p, (New-Object Text.UTF8Encoding($true)))
    foreach ($m in [regex]::Matches($text, 'L"((?:[^"\\]|\\.)*)"')) {
        $v = $m.Groups[1].Value
        if ($v -match '\p{IsCJKUnifiedIdeographs}') { [void]$keys.Add($v) }
    }
}

Write-Host "collected keys: $($keys.Count)"

# 3. existing dict keys
$dictText = [IO.File]::ReadAllText($dictPath, (New-Object Text.UTF8Encoding($true)))
$dictKeys = New-Object 'System.Collections.Generic.HashSet[string]'
foreach ($m in [regex]::Matches($dictText, '\{\s*L"((?:[^"\\]|\\.)*)"\s*,')) {
    [void]$dictKeys.Add($m.Groups[1].Value)
}

# 4. missing keys
$missing = @()
foreach ($k in $keys) { if (-not $dictKeys.Contains($k)) { $missing += $k } }
Write-Host "missing: $($missing.Count)"

# 5. pair map from _pairs.txt (En TAB Zh)
$pairs = @{}
if (Test-Path "$dir\_pairs.txt") {
    foreach ($line in [IO.File]::ReadAllLines("$dir\_pairs.txt")) {
        $ix = $line.IndexOf("`t")
        if ($ix -gt 0) {
            $en = $line.Substring(0, $ix); $zh = $line.Substring($ix + 1)
            if (-not $pairs.ContainsKey($zh)) { $pairs[$zh] = $en }
        }
    }
}

# 6. output missing with candidate En
$rows = foreach ($k in $missing) {
    $en = if ($pairs.ContainsKey($k)) { $pairs[$k] } else { '' }
    '{0}`t{1}' -f $k, $en
}
$rows | Set-Content "$dir\_newkeys.txt" -Encoding UTF8
$noEn = @($missing | Where-Object { -not $pairs.ContainsKey($_) })
Write-Host "without En: $($noEn.Count)"
$noEn | Set-Content "$dir\_newkeys_noen.txt" -Encoding UTF8
