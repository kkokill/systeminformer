# 生成二进制语言文件 lang\zh-en.lang
# 从 translate_data.c 解析 (Zh, En) 对，输出二进制格式供 translate_lang.c 运行时加载
#
# 用法: powershell -File gen_lang_file.ps1 [-Root "d:\systeminformer\systeminformer"]
# 输出: <Root>\bin\Release64\lang\zh-en.lang

param(
    [string]$Root = "d:\systeminformer\systeminformer"
)

$ErrorActionPreference = 'Stop'
$srcPath = Join-Path $Root "phlib\translate_data.c"
$outDir = Join-Path $Root "bin\Release64\lang"
$outPath = Join-Path $outDir "zh-en.lang"

if (-not (Test-Path $srcPath)) { throw "translate_data.c not found: $srcPath" }

# ---------- 1. 解析 translate_data.c ----------
$utf8 = New-Object System.Text.UTF8Encoding($false)
$content = [System.IO.File]::ReadAllText($srcPath, $utf8)

# 匹配 { L"...", L"..." } 形式的条目
$rx = [regex]'\{\s*L"((?:[^"\\]|\\.)*)"\s*,\s*L"((?:[^"\\]|\\.)*)"\s*\}'
$rxMatches = $rx.Matches($content)

Write-Host "Parsed $($rxMatches.Count) entries from translate_data.c"

# 解码 C 字符串字面量
function Decode-Literal([string]$raw) {
    $sb = New-Object System.Text.StringBuilder
    $i = 0; $len = $raw.Length
    while ($i -lt $len) {
        $c = $raw[$i]
        if ($c -ne '\') { [void]$sb.Append($c); $i++; continue }
        if ($i + 1 -ge $len) { return $sb.ToString() }
        $n = $raw[$i + 1]
        switch ($n) {
            'n' { [void]$sb.Append([char]10); $i += 2 }
            't' { [void]$sb.Append([char]9); $i += 2 }
            'r' { [void]$sb.Append([char]13); $i += 2 }
            '\' { [void]$sb.Append('\'); $i += 2 }
            '"' { [void]$sb.Append('"'); $i += 2 }
            "'" { [void]$sb.Append("'"); $i += 2 }
            default {
                if ('01234567'.IndexOf($n) -ge 0) {
                    $j = $i + 1; $oct = ''
                    while ($j -lt $len -and $oct.Length -lt 3 -and '01234567'.IndexOf($raw[$j]) -ge 0) { $oct += $raw[$j]; $j++ }
                    [void]$sb.Append([char][Convert]::ToInt32($oct, 8)); $i = $j
                } elseif ($n -eq 'x') {
                    $j = $i + 2; $hex = ''
                    while ($j -lt $len -and $hex.Length -lt 4 -and '0123456789abcdefABCDEF'.IndexOf($raw[$j]) -ge 0) { $hex += $raw[$j]; $j++ }
                    if ($hex.Length -gt 0) { [void]$sb.Append([char][Convert]::ToInt32($hex, 16)) }
                    $i = $j
                } else {
                    [void]$sb.Append($n); $i += 2
                }
            }
        }
    }
    return $sb.ToString()
}

# 构建条目列表
$entries = New-Object System.Collections.ArrayList
foreach ($m in $rxMatches) {
    $zh = Decode-Literal $m.Groups[1].Value
    $en = Decode-Literal $m.Groups[2].Value
    [void]$entries.Add(@{ Zh = $zh; En = $en })
}

# 按 UTF-16 码元序排序（与 C wcscmp 一致）— 必须用 Ordinal 比较器
$zhKeys = [string[]]@($entries | ForEach-Object { $_.Zh })
$entryObjs = [object[]]@($entries)
[Array]::Sort($zhKeys, $entryObjs, [StringComparer]::Ordinal)
$entries = @($entryObjs)

# ---------- 2. 构建二进制文件 ----------
# 确保输出目录存在
if (-not (Test-Path $outDir)) { New-Item -ItemType Directory -Path $outDir -Force | Out-Null }

# 文件格式：
# [32B Header] [8B*Count Entries] [StringBlob]
# Header: Magic(4) Version(4) EntryCount(4) Flags(4) EntriesOffset(4) StringBlobOffset(4) StringBlobSize(4) Reserved(4)
# Entry:  ZhOffset(4) EnOffset(4)
# StringBlob: 所有 Zh+En 字符串 UTF-16LE null 结尾

$entryCount = $entries.Count
$headerSize = 32
$entriesSize = $entryCount * 8

# 构建字符串 blob 并记录偏移
$stringBlob = New-Object System.IO.MemoryStream
$zhOffsets = New-Object uint32[] $entryCount
$enOffsets = New-Object uint32[] $entryCount

for ($i = 0; $i -lt $entryCount; $i++) {
    $zh = $entries[$i].Zh
    $en = $entries[$i].En

    # Zh 字符串
    $zhOffsets[$i] = [uint32]$stringBlob.Position
    $zhBytes = [System.Text.Encoding]::Unicode.GetBytes($zh)
    $stringBlob.Write($zhBytes, 0, $zhBytes.Length)
    $nullBytes = [byte[]](0, 0)
    $stringBlob.Write($nullBytes, 0, 2)

    # En 字符串
    $enOffsets[$i] = [uint32]$stringBlob.Position
    $enBytes = [System.Text.Encoding]::Unicode.GetBytes($en)
    $stringBlob.Write($enBytes, 0, $enBytes.Length)
    $stringBlob.Write($nullBytes, 0, 2)
}

$stringBlobSize = [uint32]$stringBlob.Position
$stringBlobOffset = [uint32]($headerSize + $entriesSize)

# 写入文件
$fs = [System.IO.File]::Create($outPath)
$bw = New-Object System.IO.BinaryWriter($fs)

# Header
$bw.Write([uint32]0x52544850)       # Magic "PHTR"
$bw.Write([uint32]1)                 # Version
$bw.Write([uint32]$entryCount)       # EntryCount
$bw.Write([uint32]0)                 # Flags
$bw.Write([uint32]$headerSize)       # EntriesOffset
$bw.Write([uint32]$stringBlobOffset) # StringBlobOffset
$bw.Write([uint32]$stringBlobSize)   # StringBlobSize
$bw.Write([uint32]0)                 # Reserved

# Entries
for ($i = 0; $i -lt $entryCount; $i++) {
    $bw.Write([uint32]$zhOffsets[$i])
    $bw.Write([uint32]$enOffsets[$i])
}

# StringBlob
$blobData = $stringBlob.ToArray()
$bw.Write($blobData, 0, $blobData.Length)

$bw.Close()
$fs.Close()

$fileSize = (Get-Item $outPath).Length
Write-Host "Generated: $outPath ($fileSize bytes, $entryCount entries)"
