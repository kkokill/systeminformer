# Align menu-item English values in phlib\translate_data.c with upstream source.
# Restores '&' mnemonics and '\b<accel>' tails dropped during the bilingual conversion.
# Pairs fork menu call sites to upstream by (relative file, ID_xxx) constant.
# Dry-run by default; -Apply writes the dictionary.
#   PS> .\_fix_menu_amp.ps1            # report only -> _menu_amp_report.txt
#   PS> .\_fix_menu_amp.ps1 -Apply     # patch translate_data.c
param([switch]$Apply)
$ErrorActionPreference = 'Stop'

$forkRoots = @('d:\systeminformer\systeminformer\SystemInformer', 'd:\systeminformer\systeminformer\phlib', 'd:\systeminformer\systeminformer\tools\peview')
$forkPluginRoot = 'd:\systeminformer\systeminformer\plugins'
$upRoots = @('C:\Users\Joe\Desktop\systeminformer\SystemInformer', 'C:\Users\Joe\Desktop\systeminformer\phlib', 'C:\Users\Joe\Desktop\systeminformer\tools\peview')
$upPluginRoot = 'C:\Users\Joe\Desktop\systeminformer\plugins'
$dictPath = 'd:\systeminformer\systeminformer\phlib\translate_data.c'
$reportPath = 'd:\systeminformer\systeminformer\tools\peview\_menu_amp_report.txt'
$manualMapPath = 'd:\systeminformer\systeminformer\tools\peview\_menu_amp_manual.tsv'

$cs = @'
using System;
using System.Collections.Generic;
using System.Text;
using System.Text.RegularExpressions;

public class MFSite {
    public string RelKey;   // "<plugin|>|<fileBase>"
    public string Id;
    public string EscapedKey;
    public string DecodedKey;
    public bool Wrapped;    // PhTranslateTextZ(...) present
}

public static class MenuFix {
    public static string Decode(string s) {
        var sb = new StringBuilder();
        for (int i = 0; i < s.Length; i++) {
            char c = s[i];
            if (c == '\\' && i + 1 < s.Length) {
                char n = s[++i];
                switch (n) {
                    case 't': sb.Append('\t'); break;
                    case 'n': sb.Append('\n'); break;
                    case 'r': sb.Append('\r'); break;
                    case '\\': sb.Append('\\'); break;
                    case '"': sb.Append('"'); break;
                    case '\'': sb.Append('\''); break;
                    case 'a': sb.Append('\a'); break;
                    case 'b': sb.Append('\b'); break;
                    case 'f': sb.Append('\f'); break;
                    case 'v': sb.Append('\v'); break;
                    case '0': sb.Append('\0'); break;
                    case 'x': {
                        int j = i + 1; var hx = new StringBuilder();
                        while (j < s.Length && Uri.IsHexDigit(s[j])) { hx.Append(s[j]); j++; }
                        if (hx.Length > 0) { sb.Append((char)Convert.ToInt32(hx.ToString(), 16)); i = j - 1; }
                        break;
                    }
                    case 'u': {
                        if (i + 4 < s.Length) { sb.Append((char)Convert.ToInt32(s.Substring(i + 1, 4), 16)); i += 4; }
                        break;
                    }
                    default: sb.Append(n); break;
                }
            } else sb.Append(c);
        }
        return sb.ToString();
    }

    static readonly Regex RxMenu = new Regex(
        "PhCreateEMenuItem(?:Callback)?\\s*\\(\\s*([^,]*?)\\s*,\\s*([A-Za-z_][A-Za-z0-9_]*|[0-9]+)\\s*,\\s*" +
        "(?:\\(PWSTR\\)\\s*)?(PhTranslateTextZ\\s*\\(\\s*)?L\"((?:[^\"\\\\]|\\\\.)*)\"",
        RegexOptions.Singleline | RegexOptions.Compiled);
    static readonly Regex RxPlug = new Regex(
        "PhPluginCreateEMenuItem\\s*\\(\\s*[A-Za-z_]\\w*\\s*,\\s*([^,]*?)\\s*,\\s*([A-Za-z_][A-Za-z0-9_]*|[0-9]+)\\s*,\\s*" +
        "(?:\\(PWSTR\\)\\s*)?(PhTranslateTextZ\\s*\\(\\s*)?L\"((?:[^\"\\\\]|\\\\.)*)\"",
        RegexOptions.Singleline | RegexOptions.Compiled);
    static readonly Regex RxAnyLiteral = new Regex(
        "L\"((?:[^\"\\\\]|\\\\.)*)\"", RegexOptions.Compiled);

    public static List<MFSite> ParseFile(string content, string relKey) {
        var list = new List<MFSite>();
        foreach (var rx in new[] { RxMenu, RxPlug }) {
            foreach (Match m in rx.Matches(content)) {
                var s = new MFSite();
                s.RelKey = relKey;
                s.Id = m.Groups[2].Value;
                s.EscapedKey = m.Groups[4].Value;
                s.Wrapped = m.Groups[3].Success;
                s.DecodedKey = Decode(s.EscapedKey);
                list.Add(s);
            }
        }
        return list;
    }

    public static bool HasCJK(string s) {
        foreach (char c in s) if (c >= 0x2E80 && c <= 0xFFEF) return true;
        return false;
    }

    // Histogram of decoded literals per file, for the multi-use audit.
    public static Dictionary<string, int> LiteralHistogram(string content) {
        var h = new Dictionary<string, int>();
        foreach (Match m in RxAnyLiteral.Matches(content)) {
            string d = Decode(m.Groups[1].Value);
            if (!h.ContainsKey(d)) h[d] = 0;
            h[d]++;
        }
        return h;
    }

    public struct DictEntry { public int Index; public string EscapedKey; public string EscapedVal; public string DecodedKey; public string DecodedVal; public int ValStart; public int ValLen; }

    public static List<DictEntry> ParseDict(string text) {
        var list = new List<DictEntry>();
        var rx = new Regex("\\{\\s*L\"((?:[^\"\\\\]|\\\\.)*)\"\\s*,\\s*L\"((?:[^\"\\\\]|\\\\.)*)\"\\s*\\}", RegexOptions.Multiline);
        foreach (Match m in rx.Matches(text)) {
            var e = new DictEntry();
            e.Index = list.Count;
            e.EscapedKey = m.Groups[1].Value;
            e.EscapedVal = m.Groups[2].Value;
            e.DecodedKey = Decode(e.EscapedKey);
            e.DecodedVal = Decode(e.EscapedVal);
            e.ValStart = m.Groups[2].Index;
            e.ValLen = m.Groups[2].Length;
            list.Add(e);
        }
        return list;
    }
}
'@
Add-Type -TypeDefinition $cs -Language CSharp

function Get-RelKey([string]$root, [string]$file) {
    $base = [System.IO.Path]::GetFileNameWithoutExtension($file)
    return $base
}

# ---- collect files ----
$forkFiles = New-Object System.Collections.Generic.List[object]
foreach ($r in $forkRoots) {
    Get-ChildItem -LiteralPath $r -Filter *.c | Where-Object { $_.BaseName -cne 'translate_data' } | ForEach-Object { $forkFiles.Add(@{ Path = $_.FullName; Rel = Get-RelKey $r $_.FullName }) }
}
Get-ChildItem -LiteralPath $forkPluginRoot -Directory | ForEach-Object {
    $plug = $_.Name
    Get-ChildItem -LiteralPath $_.FullName -Filter *.c -ErrorAction SilentlyContinue | ForEach-Object { $forkFiles.Add(@{ Path = $_.FullName; Rel = $plug + '|' + $_.BaseName }) }
}
$upFiles = New-Object System.Collections.Generic.List[object]
foreach ($r in $upRoots) {
    Get-ChildItem -LiteralPath $r -Filter *.c | ForEach-Object { $upFiles.Add(@{ Path = $_.FullName; Rel = Get-RelKey $r $_.FullName }) }
}
if (Test-Path -LiteralPath $upPluginRoot) {
    Get-ChildItem -LiteralPath $upPluginRoot -Directory | ForEach-Object {
        $plug = $_.Name
        Get-ChildItem -LiteralPath $_.FullName -Filter *.c -ErrorAction SilentlyContinue | ForEach-Object { $upFiles.Add(@{ Path = $_.FullName; Rel = $plug + '|' + $_.BaseName }) }
    }
}

# ---- upstream map: rel|id -> distinct decoded texts ----
$upMap = @{}
foreach ($f in $upFiles) {
    $content = [System.IO.File]::ReadAllText($f.Path)
    $sites = [MenuFix]::ParseFile($content, $f.Rel)
    foreach ($s in $sites) {
        $k = $f.Rel + '|' + $s.Id
        if (-not $upMap.ContainsKey($k)) { $upMap[$k] = New-Object System.Collections.Generic.HashSet[string] }
        [void]$upMap[$k].Add($s.DecodedKey)
    }
}

# ---- fork sites + literal histograms ----
$forkSiteCount = 0; $forkSiteList = New-Object System.Collections.Generic.List[object]
$histos = @{}
foreach ($f in $forkFiles) {
    $content = [System.IO.File]::ReadAllText($f.Path)
    $histos[$f.Path] = [MenuFix]::LiteralHistogram($content)
    $sites = [MenuFix]::ParseFile($content, $f.Rel)
    $forkSiteCount += $sites.Count
    foreach ($s in $sites) { $forkSiteList.Add(@{ Site = $s; File = $f.Path; Rel = $f.Rel }) }
}

# ---- build mapping decodedKey -> upstream escaped/decoded ----
$mapping = @{}   # decodedKey -> hashtable { UpEsc, UpDec, Status, Rel, Id }
$statuses = @{}
foreach ($e in $forkSiteList) {
    $s = $e.Site
    if (-not [MenuFix]::HasCJK($s.DecodedKey)) { $statuses['SKIPASCII']++; continue }
    $k = $e.Rel + '|' + $s.Id
    if (-not $upMap.ContainsKey($k)) { $statuses['NOUP']++; if (-not $mapping.ContainsKey($s.DecodedKey)) { $mapping[$s.DecodedKey] = @{ UpEsc = $null; Status = 'NOUP'; Rel = $e.Rel; Id = $s.Id } }; continue }
    $set = $upMap[$k]
    if ($set.Count -gt 1) { $statuses['AMBIG']++; if (-not $mapping.ContainsKey($s.DecodedKey)) { $mapping[$s.DecodedKey] = @{ UpEsc = $null; Status = 'AMBIG'; Rel = $e.Rel; Id = $s.Id } }; continue }
    $upDec = $null; foreach ($v in $set) { $upDec = $v }
    if (-not $mapping.ContainsKey($s.DecodedKey)) {
        # recover upstream ESCAPED form: search upstream file for its literal
        $upEsc = $null
        $mapping[$s.DecodedKey] = @{ UpEsc = '__NEED_SEARCH__'; UpDec = $upDec; Status = 'OK'; Rel = $e.Rel; Id = $s.Id }
    }
    $statuses['CANDIDATE']++
}

# recover escaped upstream forms by scanning upstream files once
$upEscIndex = @{}
foreach ($f in $upFiles) {
    $content = [System.IO.File]::ReadAllText($f.Path)
    $rx = [System.Text.RegularExpressions.Regex]::new('L"((?:[^"\\]|\\.)*)"', [System.Text.RegularExpressions.RegexOptions]::Compiled)
    foreach ($m in $rx.Matches($content)) {
        $esc = $m.Groups[1].Value
        $dec = [MenuFix]::LiteralHistogram($m.Value) # reuse decode via histogram of 1
        $decKey = $null; foreach ($kv in $dec.GetEnumerator()) { $decKey = $kv.Key }
        if (-not $upEscIndex.ContainsKey($decKey)) { $upEscIndex[$decKey] = $esc }
    }
}

# ---- multi-use audit: key literal appears outside menu sites? ----
# site counts per decoded key (all fork files)
$siteKeyCount = @{}
foreach ($e in $forkSiteList) {
    $k = $e.Site.DecodedKey
    if (-not $siteKeyCount.ContainsKey($k)) { $siteKeyCount[$k] = 0 }
    $siteKeyCount[$k]++
}
$multiUse = @{}   # decodedKey -> total literal count
$manualMap = @{}
if (Test-Path -LiteralPath $manualMapPath) {
    foreach ($line in [System.IO.File]::ReadAllLines($manualMapPath, (New-Object System.Text.UTF8Encoding($false)))) {
        if ($line.StartsWith('#') -or -not $line.Trim()) { continue }
        $parts = $line -split "`t"
        if ($parts.Count -lt 2) { continue }
        $manualMap[[MenuFix]::Decode($parts[0])] = $parts[1]
    }
}
foreach ($key in @($mapping.Keys)) {
    $m = $mapping[$key]
    if ($m.Status -ne 'OK') { continue }
    $total = 0
    foreach ($path in $histos.Keys) { if ($histos[$path].ContainsKey($key)) { $total += $histos[$path][$key] } }
    if ($total -gt $siteKeyCount[$key]) { $multiUse[$key] = $total }
}

# ---- parse dict, compute edits ----
$dictText = [System.IO.File]::ReadAllText($dictPath)
$entries = [MenuFix]::ParseDict($dictText)
$dictByKey = @{}
foreach ($e in $entries) {
    if (-not $dictByKey.ContainsKey($e.DecodedKey)) { $dictByKey[$e.DecodedKey] = New-Object System.Collections.Generic.List[object] }
    [void]$dictByKey[$e.DecodedKey].Add($e)
}

$edits = New-Object System.Collections.Generic.List[object]
$report = New-Object System.Collections.Generic.List[string]
$cnt = @{ TOCHANGE = 0; ALREADY = 0; NOKEY = 0; NOUP = 0; AMBIG = 0; MULTIUSE = 0 }

foreach ($key in @($mapping.Keys)) {
    $m = $mapping[$key]
    if ($manualMap.ContainsKey($key)) {
        $upEsc = $manualMap[$key]
        if (-not $dictByKey.ContainsKey($key)) { $cnt['NOKEY']++; $report.Add("NOKEY`t" + $key); continue }
        foreach ($en in $dictByKey[$key]) {
            if ($en.DecodedVal -ceq ([MenuFix]::Decode($upEsc))) { $cnt['ALREADY']++; continue }
            $cnt['TOCHANGE']++
            $report.Add("CHANGE`t" + $key + "`tOLD<" + $en.DecodedVal + ">`tNEW<" + ([MenuFix]::Decode($upEsc)) + ">")
            if ($Apply) { $edits.Add(@{ Start = $en.ValStart; Len = $en.ValLen; Text = $upEsc }) }
        }
        continue
    }
    if ($m.Status -eq 'NOUP') { $cnt['NOUP']++; $report.Add("NOUP`t" + $m.Rel + "`t" + $m.Id + "`t" + $key); continue }
    if ($m.Status -eq 'AMBIG') {
        $cnt['AMBIG']++
        $k = $m.Rel + '|' + $m.Id
        $cands = ''
        if ($upMap.ContainsKey($k)) { $cands = ($upMap[$k] -join ' ||| ') }
        $report.Add("AMBIG`t" + $m.Rel + "`t" + $m.Id + "`t" + $key + "`tCANDS<" + $cands + ">")
        continue
    }
    if ($multiUse.ContainsKey($key)) { $cnt['MULTIUSE']++; $report.Add("MULTIUSE`ttotal=" + $multiUse[$key] + "`t" + $key); continue }
    $upEsc = $upEscIndex[$m.UpDec]
    if (-not $upEsc) { $report.Add("NOESCAPE`t" + $key); continue }
    if (-not $dictByKey.ContainsKey($key)) { $cnt['NOKEY']++; $report.Add("NOKEY`t" + $key); continue }
    foreach ($en in $dictByKey[$key]) {
        if ($en.DecodedVal -ceq $m.UpDec) { $cnt['ALREADY']++; continue }
        $cnt['TOCHANGE']++
        $report.Add("CHANGE`t" + $key + "`tOLD<" + $en.DecodedVal + ">`tNEW<" + $m.UpDec + ">")
        if ($Apply) { $edits.Add(@{ Start = $en.ValStart; Len = $en.ValLen; Text = $upEsc }) }
    }
}

# ---- output ----
$summary = "forkSites=$forkSiteCount upPairs=$($upMap.Count) dictEntries=$($entries.Count) " +
    "candidate=$($statuses['CANDIDATE']) TOCHANGE=$($cnt['TOCHANGE']) ALREADY=$($cnt['ALREADY']) " +
    "NOKEY=$($cnt['NOKEY']) NOUP=$($cnt['NOUP']) AMBIG=$($cnt['AMBIG']) MULTIUSE=$($cnt['MULTIUSE']) SKIPASCII=$($statuses['SKIPASCII'])"
$report.Insert(0, $summary) | Out-Null
[System.IO.File]::WriteAllLines($reportPath, $report, (New-Object System.Text.UTF8Encoding($true)))
Write-Host $summary
Write-Host "report: $reportPath"

if ($Apply -and $edits.Count -gt 0) {
    $sb = New-Object System.Text.StringBuilder($dictText)
    $sorted = $edits | Sort-Object { $_.Start } -Descending
    foreach ($e in $sorted) { [void]$sb.Remove($e.Start, $e.Len); [void]$sb.Insert($e.Start, $e.Text) }
    $bytes = [System.IO.File]::ReadAllBytes($dictPath)
    $hasBom = ($bytes.Length -ge 3 -and $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF)
    [System.IO.File]::WriteAllText($dictPath, $sb.ToString(), (New-Object System.Text.UTF8Encoding($hasBom)))
    Write-Host ("APPLIED edits=" + $edits.Count + " bom=" + $hasBom)
}
