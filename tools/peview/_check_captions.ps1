# Audit: every CAPTION in plugins\*\*.rc must be a key in phlib\translate_data.c
# (dialog frame captions are now translated via dictionary at runtime).
# ASCII-only script; reads Chinese text from files at runtime.

$root = 'd:\systeminformer\systeminformer'
$dictPath = Join-Path $root 'phlib\translate_data.c'

# --- load dictionary keys (decode C escapes) ---
$keySet = New-Object 'System.Collections.Generic.HashSet[string]'
$count = 0
foreach ($line in [System.IO.File]::ReadLines($dictPath)) {
    if ($line -match '^\s*\{\s*L"((?:[^"\\]|\\.)*)"\s*,') {
        $key = [System.Text.RegularExpressions.Regex]::Unescape($Matches[1])
        [void]$keySet.Add($key)
        $count++
    }
}
Write-Output ("Dictionary keys: {0}" -f $count)

# --- extract CAPTION lines from plugin rc files ---
$missing = New-Object 'System.Collections.Generic.List[string]'
$checked = 0
Get-ChildItem -Path (Join-Path $root 'plugins') -Filter *.rc -Recurse | ForEach-Object {
    $rcFile = $_
    $n = 0
    foreach ($line in [System.IO.File]::ReadLines($rcFile.FullName)) {
        $n++
        if ($line -match '\bCAPTION\s+"((?:[^"]|"")*)"') {
            $caption = $Matches[1] -replace '""', '"'
            $checked++
            if (-not $keySet.Contains($caption)) {
                $missing.Add(("{0}({1}): [{2}]" -f $rcFile.Name, $n, $caption))
            }
        }
    }
}
Write-Output ("Captions checked: {0}" -f $checked)
Write-Output ("Missing from dictionary: {0}" -f $missing.Count)
$missing | ForEach-Object { Write-Output $_ }
