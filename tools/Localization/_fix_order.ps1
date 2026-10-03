# 修复翻译表排序：C# 全程做 解析→Ordinal排序→重写 translate_data.c→生成 zh-en.lang
# 根因：PowerShell [Array]::Sort 联动排序静默失效，表乱序导致二分查找全部 miss
$ErrorActionPreference = 'Stop'
Add-Type -ReferencedAssemblies System.IO @'
using System;
using System.IO;
using System.Text;
using System.Text.RegularExpressions;
using System.Collections.Generic;

public static class TableFix {
    static string Decode(string raw) {
        var sb = new StringBuilder();
        int i = 0;
        while (i < raw.Length) {
            char c = raw[i];
            if (c != '\\') { sb.Append(c); i++; continue; }
            char n = raw[i+1];
            switch (n) {
                case 'n': sb.Append('\n'); i+=2; break;
                case 't': sb.Append('\t'); i+=2; break;
                case 'r': sb.Append('\r'); i+=2; break;
                case '\\': sb.Append('\\'); i+=2; break;
                case '"': sb.Append('"'); i+=2; break;
                case '?': sb.Append('?'); i+=2; break;
                case 'b': sb.Append('\b'); i+=2; break;
                case 'a': sb.Append('\a'); i+=2; break;
                case 'f': sb.Append('\f'); i+=2; break;
                case 'v': sb.Append('\v'); i+=2; break;
                default: sb.Append(n); i+=2; break;
            }
        }
        return sb.ToString();
    }
    static string Encode(string s) {
        var sb = new StringBuilder();
        foreach (char c in s) {
            switch (c) {
                case '\\': sb.Append("\\\\"); break;
                case '"': sb.Append("\\\""); break;
                case '\n': sb.Append("\\n"); break;
                case '\r': sb.Append("\\r"); break;
                case '\t': sb.Append("\\t"); break;
                case '\b': sb.Append("\\b"); break;
                default: sb.Append(c); break;
            }
        }
        return sb.ToString();
    }
    public static int Fix(string srcPath, string langPath) {
        var text = File.ReadAllText(srcPath, new UTF8Encoding(false));
        // 修复 gen_translate.ps1 解码 \b 时吞掉 "\bC" 的历史坏 key：XXXtrl+F5 → XXX\bCtrl+F5
        text = Regex.Replace(text, "(?<!C)trl\\+", "\\bCtrl+");
        var rx = new Regex("\\{\\s*L\"((?:[^\"\\\\]|\\\\.)*)\"\\s*,\\s*L\"((?:[^\"\\\\]|\\\\.)*)\"\\s*\\}");
        var entries = new List<KeyValuePair<string,string>>();
        foreach (Match m in rx.Matches(text))
            entries.Add(new KeyValuePair<string,string>(Decode(m.Groups[1].Value), Decode(m.Groups[2].Value)));

        var seen = new HashSet<string>();
        var uniq = new List<KeyValuePair<string,string>>();
        foreach (var e in entries) if (seen.Add(e.Key)) uniq.Add(e);

        var keys = new string[uniq.Count];
        var items = uniq.ToArray();
        for (int i = 0; i < uniq.Count; i++) keys[i] = uniq[i].Key;
        Array.Sort(keys, items, StringComparer.Ordinal);

        for (int i = 1; i < keys.Length; i++)
            if (StringComparer.Ordinal.Compare(keys[i-1], keys[i]) >= 0)
                throw new Exception("sort failed at " + i);

        var sb = new StringBuilder();
        sb.Append("/*\r\n");
        sb.Append(" * 运行时 UI 文本翻译字典（zh→en）— 自动生成，勿手工编辑。\r\n");
        sb.Append(" * 生成器: gen_translate.ps1 + _make_lang.ps1（占位补齐）+ _fix_order.ps1（排序修复）\r\n");
        sb.Append(" * 基线:   1487bbf6e\r\n");
        sb.Append(" * 规则:   键 = 当前源码硬编码中文；值 = git 基线同位置英文。\r\n");
        sb.Append(" *         en=zh 的条目为占位（暂无基线英文），待 LLM 补翻。\r\n");
        sb.Append(" * 表按 UTF-16 码元序（wcscmp/Ordinal）排序，供二分查找。\r\n");
        sb.Append(" */\r\n\r\n");
        sb.Append("#include <ph.h>\r\n\r\n");
        sb.Append("typedef struct _PH_TRANSLATE_ENTRY\r\n{\r\n");
        sb.Append("    PCWSTR Zh;\r\n    PCWSTR En;\r\n");
        sb.Append("} PH_TRANSLATE_ENTRY, *PPH_TRANSLATE_ENTRY;\r\n\r\n");
        sb.Append("const PH_TRANSLATE_ENTRY PhTranslateTable[] =\r\n{\r\n");
        foreach (var e in items)
            sb.Append("    { L\"").Append(Encode(e.Key)).Append("\", L\"").Append(Encode(e.Value)).Append("\" },\r\n");
        sb.Append("};\r\n\r\n");
        sb.Append("const ULONG PhTranslateTableCount = RTL_NUMBER_OF(PhTranslateTable);\r\n");
        File.WriteAllText(srcPath, sb.ToString(), new UTF8Encoding(true));

        var dir = Path.GetDirectoryName(langPath);
        if (!Directory.Exists(dir)) Directory.CreateDirectory(dir);
        int n = items.Length;
        using (var fs = File.Create(langPath))
        using (var bw = new BinaryWriter(fs)) {
            var blob = new MemoryStream();
            var zhOff = new uint[n]; var enOff = new uint[n];
            for (int i = 0; i < n; i++) {
                zhOff[i] = (uint)blob.Position;
                var b = Encoding.Unicode.GetBytes(items[i].Key); blob.Write(b, 0, b.Length); blob.Write(new byte[2], 0, 2);
                enOff[i] = (uint)blob.Position;
                b = Encoding.Unicode.GetBytes(items[i].Value); blob.Write(b, 0, b.Length); blob.Write(new byte[2], 0, 2);
            }
            bw.Write((uint)0x52544850); bw.Write((uint)1); bw.Write((uint)n); bw.Write((uint)0);
            bw.Write((uint)32); bw.Write((uint)(32 + 8*n)); bw.Write((uint)blob.Position); bw.Write((uint)0);
            for (int i = 0; i < n; i++) { bw.Write(zhOff[i]); bw.Write(enOff[i]); }
            bw.Write(blob.ToArray());
        }
        return n;
    }
}
'@

$n = [TableFix]::Fix(
    "d:\systeminformer\systeminformer\phlib\translate_data.c",
    "d:\systeminformer\systeminformer\bin\Release64\lang\zh-en.lang")
Write-Host "OK: $n entries, sorted + lang regenerated"
