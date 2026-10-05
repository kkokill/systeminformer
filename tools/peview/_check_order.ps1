# 字典 wcscmp（UTF-16 码元）排序终验：解码 C 转义后按 Ordinal 严格递增校验
$ErrorActionPreference = 'Stop'
$src = 'd:\systeminformer\systeminformer\phlib\translate_data.c'
$text = [System.IO.File]::ReadAllText($src)

$cs = @'
using System;
using System.Collections.Generic;
using System.Text;
using System.Text.RegularExpressions;
public static class DictCheck {
    static string Decode(string s) {
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
                    case 'U': {
                        if (i + 8 < s.Length) { sb.Append(char.ConvertFromUtf32(Convert.ToInt32(s.Substring(i + 1, 8), 16))); i += 8; }
                        break;
                    }
                    default: sb.Append(n); break;
                }
            } else sb.Append(c);
        }
        return sb.ToString();
    }
    public static void Run(string text) {
        var rx = new Regex("\\{\\s*L\"((?:[^\"\\\\]|\\\\.)*)\"\\s*,\\s*L\"((?:[^\"\\\\]|\\\\.)*)\"\\s*\\}", RegexOptions.Multiline);
        var keys = new List<string>(); var vals = new List<string>();
        foreach (Match m in rx.Matches(text)) { keys.Add(Decode(m.Groups[1].Value)); vals.Add(Decode(m.Groups[2].Value)); }
        Console.WriteLine("keys: " + keys.Count);
        int violations = 0, dupdict = 0; var dupseen = new HashSet<string>();
        for (int i = 1; i < keys.Count; i++) {
            int cmp = string.CompareOrdinal(keys[i - 1], keys[i]);
            if (cmp > 0) { violations++; Console.WriteLine("VIOLATION[" + i + "]: '" + keys[i - 1] + "' > '" + keys[i] + "'"); }
            if (cmp == 0) { dupdict++; if (dupseen.Add(keys[i])) Console.WriteLine("DUPDICT: '" + keys[i] + "'"); }
        }
        Console.WriteLine("violations: " + violations);
        Console.WriteLine("dupdict: " + dupdict);
    }
}
'@
Add-Type -TypeDefinition $cs -Language CSharp
[DictCheck]::Run($text)
