# Audit all CJK string literals in main-program files against the dictionary.
# Pure ASCII script (safe for PS5.1, no BOM dependency).
$ErrorActionPreference = 'Stop'
. d:\systeminformer\systeminformer\tools\peview\_decode_lib.ps1
# alias for compatibility
function Decode-Lit([string]$s) { Decode-CEscapes $s }

$root = 'd:\systeminformer\systeminformer'
$dictPath = "$root\phlib\translate_data.c"
$files = @('options.c','mainwnd.c','appsup.c','usrlist.c','runas.c','sessprp.c','thrdstk.c','srvprp.c','srvctl.c','runaspkg.c','prpgwmi.c','prpgvdm.c','prpgmod.c','prpggen.c','procrec.c','procprp.c','miniinfo.c','hndlmenu.c','actions.c')

# 1. Load dictionary keys (decoded)
$dictSrc = [IO.File]::ReadAllText($dictPath, (New-Object Text.UTF8Encoding($true)))
$dictKeys = New-Object 'System.Collections.Generic.HashSet[string]'
$rxEntry = [regex]'(?m)\{\s*L"((?:[^"\\]|\\.)*)"\s*,\s*L"((?:[^"\\]|\\.)*)"\s*\}'
foreach ($m in $rxEntry.Matches($dictSrc)) { [void]$dictKeys.Add((Decode-Lit $m.Groups[1].Value)) }

# 2. Extract CJK string literals from source files
$rxLit = [regex]'"((?:[^"\\\r\n]|\\.)+)"'
$cjk = [regex]'[\p{IsCJKUnifiedIdeographs}]'
$missing = @{}   # literal -> list of "file:line"
$found = 0; $missingCount = 0

foreach ($f in $files) {
    $p = "$root\SystemInformer\$f"
    $lines = [IO.File]::ReadAllLines($p, (New-Object Text.UTF8Encoding($true)))
    for ($i = 0; $i -lt $lines.Count; $i++) {
        $line = $lines[$i]
        if (-not $cjk.IsMatch($line)) { continue }
        foreach ($m in $rxLit.Matches($line)) {
            $raw = $m.Groups[1].Value
            if (-not $cjk.IsMatch($raw)) { continue }
            $lit = Decode-Lit $raw
            $found++
            if (-not $dictKeys.Contains($lit)) {
                $missingCount++
                $loc = "${f}:$($i+1)"
                if ($missing.ContainsKey($lit)) { $missing[$lit] += ",$loc" } else { $missing[$lit] = $loc }
            }
        }
    }
}

Write-Host "total CJK literals: $found  missing keys: $missingCount  distinct: $($missing.Count)"
$sb = New-Object Text.StringBuilder
[void]$sb.AppendLine("distinct missing: $($missing.Count)")
foreach ($k in $missing.Keys) { [void]$sb.AppendLine("$($missing[$k])`t$k") }
[IO.File]::WriteAllText("$root\tools\peview\_missing_keys.txt", $sb.ToString(), (New-Object Text.UTF8Encoding($true)))
