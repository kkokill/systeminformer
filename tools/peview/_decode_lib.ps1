# Shared C-escape decoder (dot-sourced). Full support: \n \r \t \b \f \v \a \\ \" \' \? \xHH \uHHHH \UHHHHHHHH \OOO
function Decode-CEscapes([string]$s) {
    $sb = New-Object System.Text.StringBuilder
    for ($i = 0; $i -lt $s.Length; $i++) {
        $c = $s[$i]
        if ($c -eq '\' -and $i + 1 -lt $s.Length) {
            $i++; $n = $s[$i]
            switch -CaseSensitive ($n) {
                'n' { [void]$sb.Append("`n") } 'r' { [void]$sb.Append("`r") } 't' { [void]$sb.Append("`t") }
                'b' { [void]$sb.Append([char]8) } 'f' { [void]$sb.Append([char]12) } 'v' { [void]$sb.Append([char]11) }
                'a' { [void]$sb.Append([char]7) } '\' { [void]$sb.Append('\') } '"' { [void]$sb.Append('"') }
                "'" { [void]$sb.Append("'") } '?' { [void]$sb.Append('?') }
                'u' {
                    $hex = ''
                    while ($i + 1 -lt $s.Length -and $hex.Length -lt 4 -and $s[$i+1] -match '[0-9a-fA-F]') { $i++; $hex += $s[$i] }
                    if ($hex.Length -eq 4) { [void]$sb.Append([char][Convert]::ToInt32($hex, 16)) }
                    else { [void]$sb.Append($n).Append($hex) }
                }
                'U' {
                    $hex = ''
                    while ($i + 1 -lt $s.Length -and $hex.Length -lt 8 -and $s[$i+1] -match '[0-9a-fA-F]') { $i++; $hex += $s[$i] }
                    if ($hex.Length -eq 8) { [void]$sb.Append([char]::ConvertFromUtf32([Convert]::ToInt32($hex, 16))) }
                    else { [void]$sb.Append($n).Append($hex) }
                }
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
