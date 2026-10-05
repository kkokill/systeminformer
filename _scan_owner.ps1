$ErrorActionPreference = 'Stop'
$root = 'd:\systeminformer\systeminformer\SystemInformer'
$cjkA = [char]0x4E00
$cjkB = [char]0x9FA5
$cjkClass = "[$cjkA-$cjkB]"
$fullPunct = '[' + [char]0xFF00 + '-' + [char]0xFFEF + [char]0x2018 + [char]0x2019 + [char]0x201C + [char]0x201D + ']'

# calls that are already hooked inside phlib -> skip wrapping (dict key check only)
$hookedRx = 'PhShowConfirmMessage|PhShowStatus2?\(|PhShowError2?\(|PhShowMessage\w*\(|PhShowWarning2?\(|PhShowInfoMessage|PhFormatString\w*\(|PhShowChooseFileDialog|PhAddListViewItem\(|PhAddListViewColumn\(|PhSetListViewSubItem\(|PhAddTreeNewColumn\w*\(|PhCreateEMenuItem\(|PhInsertEMenuItem\(|PhAppendFormatStringBuilder_V\(|PhAddTreeNew\w*\('
# any call-start marker to stop upward scan
$callRx = '(PhShow\w+|PhUi\w+|PhFormat\w*|PhCreate\w+|PhSet\w+|PhAdd\w+|PhInsert\w+|PhAppend\w+|PhConcat\w+|PhTranslate\w+|PhGet\w+|PhMap\w+|PhFind\w+|ComboBox_\w+|SendMessage\w*\(|SetDlgItemText\(|Buttons\[|buttons\[|page\.\w+|config\.\w+|propSheetHeader\.\w+)'

$patterns = @(
    "(PhSetDialogItemText\(|PhSetWindowText\(|PhAppendStringBuilder2\(|PhConcatStrings|PhConcatStringRefZ\(|PhInitFormat|ComboBox_AddString\(|SetDlgItemText\()[^)]*$cjkClass",
    "$cjkClass[^)]*(PhSetDialogItemText\(|PhSetWindowText\(|PhAppendStringBuilder2\(|PhConcatStrings|PhConcatStringRefZ\(|PhInitFormat|ComboBox_AddString\(|SetDlgItemText\()",
    "psz[A-Za-z]*\s*=\s*[^;]*$cjkClass",
    "PhGetStringOrDefault\([^;]*$cjkClass",
    "=\s*\{\s*L`"[^`"]*$cjkClass",
    "^\s*L`"[^`"]*$cjkClass"
)

$files = Get-ChildItem -Path $root -Filter *.c -File | Sort-Object Name
foreach ($f in $files) {
    $lines = [System.IO.File]::ReadAllLines($f.FullName)
    $out = @()
    for ($i = 0; $i -lt $lines.Count; $i++) {
        $line = $lines[$i]
        if (($line -notmatch $cjkClass) -and ($line -notmatch $fullPunct)) { continue }
        if ($line -match '^\s*//') { continue }
        if ($line -match 'PhTranslateTextZ\s*\(') { continue }
        $matched = $false
        foreach ($p in $patterns) { if ($line -match $p) { $matched = $true; break } }
        if (-not $matched) { continue }
        # upward scan for owning call
        $owner = '?'
        for ($j = $i - 1; $j -ge [Math]::Max(0, $i - 40); $j--) {
            $l = $lines[$j]
            if ($l -match $hookedRx) { $owner = 'HOOKED:' + ($l.Trim() -replace '\s+', ' '); break }
            if ($l -match $callRx) { $owner = 'NEEDCHK:' + ($l.Trim() -replace '\s+', ' '); break }
        }
        $out += ("{0}:{1} [{2}]" -f $f.Name, ($i + 1), $owner.Substring(0, [Math]::Min(110, $owner.Length)))
    }
    if ($out.Count -gt 0) {
        Write-Output ("=== {0} ({1}) ===" -f $f.Name, $out.Count)
        $out | ForEach-Object { Write-Output $_ }
    }
}
