# C# script run via PowerShell - validate & optionally fix wcscmp order of translate_data.c
$ErrorActionPreference = 'Stop'
$dictPath = 'd:\systeminformer\systeminformer\phlib\translate_data.c'

$src = [IO.File]::ReadAllText($dictPath, (New-Object Text.UTF8Encoding($true)))
$rx = [regex]'(?m)^(\s*)\{\s*L"((?:[^"\\]|\\.)*)"\s*,\s*L"((?:[^"\\]|\\.)*)"\s*\},?\s*$'
$matches = $rx.Matches($src)
Write-Host "entries: $($matches.Count)"

# decode C escapes to runtime string (handles \t \n \\ \" \0; keeps unknown escapes literal)
function Decode([string]$s) {
    $sb = New-Object Text.StringBuilder
    for ($i = 0; $i -lt $s.Length; $i++) {
        $c = $s[$i]
        if ($c -eq '\') {
            $i++
            if ($i -ge $s.Length) { [void]$sb.Append('\'); break }
            switch ($s[$i]) {
                'n' { [void]$sb.Append([char]10) }
                't' { [void]$sb.Append([char]9) }
                'r' { [void]$sb.Append([char]13) }
                'b' { [void]$sb.Append([char]8) }  # \b compiles to 0x08 at runtime; source-text order (0x5C) is wrong for binary search
                '0' { [void]$sb.Append([char]0) }
                '\' { [void]$sb.Append('\') }
                '"' { [void]$sb.Append('"') }
                default { [void]$sb.Append('\').Append($s[$i]) }
            }
        } else { [void]$sb.Append($c) }
    }
    return $sb.ToString()
}

$keys = New-Object 'System.Collections.Generic.List[string]'
foreach ($m in $matches) { $keys.Add((Decode $m.Groups[2].Value)) }

$sorted = [string[]]$keys.ToArray()
[Array]::Sort($sorted, [System.StringComparer]::Ordinal)

$viol = 0
$prevBad = $false
for ($i = 0; $i -lt $keys.Count; $i++) {
    if ($i -gt 0 -and [System.StringComparer]::Ordinal.Compare($keys[$i-1], $keys[$i]) -gt 0) {
        if (-not $prevBad) {
            Write-Host ("VIOLATION at entry {0}: [{1}] should come after [{2}] (sorted pos {3})" -f ($i+1), $keys[$i-1], $keys[$i], ([Array]::IndexOf($sorted, $keys[$i-1])))
        }
        $prevBad = $true; $viol++
    } else { $prevBad = $false }
}
Write-Host "descents: $viol"
