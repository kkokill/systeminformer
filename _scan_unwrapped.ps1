$ErrorActionPreference = 'Stop'
$root = 'd:\systeminformer\systeminformer\SystemInformer'
$cjkA = [char]0x4E00
$cjkB = [char]0x9FA5
$cjkClass = "[$cjkA-$cjkB]"

$patterns = @(
    "(PhSetDialogItemText\(|PhSetWindowText\(|PhAppendStringBuilder|PhConcatStrings|PhConcatStringRefZ\(|PhInitFormat|ComboBox_AddString\(|SetDlgItemText\()[^)]*$cjkClass",
    "$cjkClass[^)]*(PhSetDialogItemText\(|PhSetWindowText\(|PhAppendStringBuilder|PhConcatStrings|PhConcatStringRefZ\(|PhInitFormat|ComboBox_AddString\(|SetDlgItemText\()",
    "psz[A-Za-z]*\s*=\s*[^;]*$cjkClass",
    "PhGetStringOrDefault\([^;]*$cjkClass",
    "PhCreateStringEx?\([^)]*$cjkClass",
    "=\s*\{\s*L`"[^`"]*$cjkClass",
    "^\s*L`"[^`"]*$cjkClass"
)

$files = Get-ChildItem -Path $root -Filter *.c -File | Sort-Object Name
$total = 0
foreach ($f in $files) {
    $lines = [System.IO.File]::ReadAllLines($f.FullName)
    $hits = @()
    for ($i = 0; $i -lt $lines.Count; $i++) {
        $line = $lines[$i]
        if ($line -notmatch $cjkClass) { continue }
        if ($line -match '^\s*//') { continue }
        if ($line -match 'PhTranslateTextZ\s*\(') { continue }
        $matched = $false
        foreach ($p in $patterns) {
            if ($line -match $p) { $matched = $true; break }
        }
        if ($matched) {
            $hits += ("{0}: {1}" -f ($i + 1), $line.TrimEnd())
        }
    }
    if ($hits.Count -gt 0) {
        Write-Output ("=== {0} ({1}) ===" -f $f.Name, $hits.Count)
        $hits | ForEach-Object { Write-Output $_ }
        $total += $hits.Count
    }
}
Write-Output ("TOTAL: {0}" -f $total)
