# Plugin localization punch list (ASCII-only to avoid LF/no-BOM PowerShell parser bug)
$Root = 'd:\systeminformer\systeminformer'
$plugins = Get-ChildItem (Join-Path $Root "plugins") -Directory | Where-Object { $_.Name -ne 'include' }

$apis = @(
    'ToolbarGetText',
    'SB_SETTEXT',
    'LVM_SETCOLUMN',
    'LVM_INSERTCOLUMN',
    'PhAddListViewColumn',
    'TreeNew_TranslateColumns',
    'SetWindowText',
    'PhCreateEMenuItem',
    'CreateWindowEx',
    'PhCreateDialogBox'
)

$hasCjk = [regex]::new('[\u3000-\u303f\u3400-\u4dbf\u4e00-\u9fff\uff00-\uffef]')
$utf8 = New-Object System.Text.UTF8Encoding($false)

$registered = @('OnlineChecks','ToolStatus','ExtendedTools','HardwareDevices')

Write-Host ("{0,-22} {1,-6} {2,-8} {3,-10} {4}" -f "Plugin", "Reg?", "CJKFile", "CJKTotal", "Dynamic-text APIs (count)")
Write-Host ("=" * 110)

foreach ($p in $plugins) {
    $name = $p.Name
    $isReg = if ($registered -contains $name) { 'Y' } else { 'N' }

    $cFiles = Get-ChildItem $p.FullName -Filter *.c -Recurse
    $cjkFileCount = 0
    $cjkTotal = 0
    $apiHits = @{}

    foreach ($f in $cFiles) {
        $text = [System.IO.File]::ReadAllText($f.FullName, $utf8)
        $cjkMatches = $hasCjk.Matches($text)
        if ($cjkMatches.Count -gt 0) {
            $cjkFileCount++
            $cjkTotal += $cjkMatches.Count
        }
        foreach ($apiName in $apis) {
            $count = ([regex]::new([regex]::Escape($apiName))).Matches($text).Count
            if ($count -gt 0) { $apiHits[$apiName] = ($apiHits[$apiName] + $count) }
        }
    }

    $apiStr = if ($apiHits.Count -gt 0) {
        ($apiHits.GetEnumerator() | Sort-Object { $_.Value } -Descending | ForEach-Object { "$($_.Key)x$($_.Value)" }) -join ' '
    } else { '-' }

    Write-Host ("{0,-22} {1,-6} {2,-8} {3,-10} {4}" -f $name, $isReg, $cjkFileCount, $cjkTotal, $apiStr)
}
