# Apply translation overrides + \b accelerator-key variants, rebuild translate_data.c + zh-en.lang
# TSV format: Zh<TAB>En[\tforce] (C escapes: \r \n \t \b ...)
# Pipeline: _apply_trans.ps1 -> (sort+lang done inside) -> rebuild phlib/SystemInformer
$ErrorActionPreference = 'Stop'
Add-Type -ReferencedAssemblies System.IO @'
using System;
using System.IO;
using System.Text;
using System.Text.RegularExpressions;
using System.Collections.Generic;

public static class TransApply {
    static string Decode(string raw) {
        var sb = new StringBuilder(); int i = 0;
        while (i < raw.Length) {
            char c = raw[i];
            if (c != '\\') { sb.Append(c); i++; continue; }
            if (i + 1 >= raw.Length) break;
            char n = raw[i + 1];
            switch (n) {
                case 'n': sb.Append('\n'); i += 2; break;
                case 't': sb.Append('\t'); i += 2; break;
                case 'r': sb.Append('\r'); i += 2; break;
                case 'b': sb.Append('\b'); i += 2; break;
                case 'a': sb.Append('\a'); i += 2; break;
                case 'f': sb.Append('\f'); i += 2; break;
                case 'v': sb.Append('\v'); i += 2; break;
                case '0': sb.Append('\0'); i += 2; break;
                case '\\': sb.Append('\\'); i += 2; break;
                case '"': sb.Append('"'); i += 2; break;
                case '\'': sb.Append('\''); i += 2; break;
                case '?': sb.Append('?'); i += 2; break;
                case 'x': {
                    int j = i + 2; string h = "";
                    while (j < raw.Length && Uri.IsHexDigit(raw[j])) { h += raw[j]; j++; }
                    if (h.Length > 0) { sb.Append((char)Convert.ToInt32(h, 16)); i = j; }
                    else { sb.Append(n); i += 2; }
                    break;
                }
                default: sb.Append(n); i += 2; break;
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
                case '\f': sb.Append("\\f"); break;
                case '\v': sb.Append("\\v"); break;
                case '\a': sb.Append("\\a"); break;
                case '\0': sb.Append("\\0"); break;
                default: sb.Append(c); break;
            }
        }
        return sb.ToString();
    }
    static bool HasCjk(string s) {
        foreach (char c in s)
            if ((c >= 0x3000 && c <= 0x303f) || (c >= 0x3400 && c <= 0x4dbf) || (c >= 0x4e00 && c <= 0x9fff) || (c >= 0xff00 && c <= 0xffef))
                return true;
        return false;
    }
    // Strip accelerator/hotkey markers from EN side only: \b suffix, U+008C suffix, '&'.
    // Keys stay verbatim (must match source literals). Placeholder entries (en==zh) untouched.
    static int enCleaned = 0;
    static string EnClean(string en, string zh) {
        if (en == zh) return en;
        int idx = en.IndexOf('\b');
        if (idx >= 0) en = en.Substring(0, idx);
        idx = en.IndexOf('');
        if (idx >= 0) en = en.Substring(0, idx);
        string clean = en.Replace("&", "");
        if (clean != en) enCleaned++;
        return clean;
    }
    // Whitespace repair for concatenated fragments: TSV editors lose trailing spaces
    static string SpaceFix(string zh, string en) {
        if (zh == " 在 ") en = " in ";
        else if (zh == " 到 ") en = " to ";
        else if (zh == "是否要") { if (!en.EndsWith(" ")) en = en + " "; }
        if (zh.EndsWith(" ") && !en.EndsWith(" ")) en += " ";
        if (zh.StartsWith(" ") && !en.StartsWith(" ") && en.Length > 0 && (char.IsLetterOrDigit(en[0]) || en[0] == '&')) en = " " + en;
        return en;
    }
    static void GetLiterals(string text, List<string> outList) {
        var rx = new Regex("(?<nl>\r?\n)|//|/\\*|\\*/|L?\"(?:[^\"\\\\]|\\\\.)*\"");
        bool inLine = false, inBlock = false;
        int prevEnd = -1; string pending = null;
        foreach (Match m in rx.Matches(text)) {
            string v = m.Value;
            if (m.Groups["nl"].Success) { inLine = false; continue; }
            if (v == "//") { inLine = true; continue; }
            if (v == "/*") { inBlock = true; continue; }
            if (v == "*/") { inBlock = false; continue; }
            if (inLine || inBlock) { prevEnd = m.Index + m.Length; continue; }
            if (v.StartsWith("\"") || v.StartsWith("L\"")) {
                string dec = Decode(v.StartsWith("L") ? v.Substring(2, v.Length - 3) : v.Substring(1, v.Length - 2));
                if (dec == null) { pending = null; prevEnd = m.Index + m.Length; continue; }
                if (pending != null && prevEnd >= 0 && (text.Substring(prevEnd, m.Index - prevEnd).Trim().Length == 0)) {
                    pending = pending + dec;
                } else {
                    if (pending != null) outList.Add(pending);
                    pending = dec;
                }
                prevEnd = m.Index + m.Length;
            }
        }
        if (pending != null) outList.Add(pending);
    }
    static List<string> SourceFiles(string root) {
        var files = new List<string>();
        foreach (var f in Directory.GetFiles(Path.Combine(root, "SystemInformer"), "*.c")) files.Add(f);
        foreach (var d in Directory.GetDirectories(Path.Combine(root, "plugins")))
            foreach (var f in Directory.GetFiles(d, "*.c", SearchOption.AllDirectories)) files.Add(f);
        foreach (var f in Directory.GetFiles(Path.Combine(root, "phlib"), "*.c"))
            if (!f.EndsWith("translate_data.c")) files.Add(f);
        foreach (var f in Directory.GetFiles(Path.Combine(root, "phlib"), "*.cpp")) files.Add(f);
        // tools 工具目录（peview/CustomSetupTool/PortableLauncher 等也链 phlib，UI 文本同样走翻译表）
        string toolsDir = Path.Combine(root, "tools");
        if (Directory.Exists(toolsDir))
            foreach (var f in Directory.GetFiles(toolsDir, "*.c", SearchOption.AllDirectories)) files.Add(f);
        return files;
    }
    public static string Apply(string srcPath, string[] tsvLines, string root, string langPath) {
        // 1. parse TSV
        var ovr = new List<KeyValuePair<string, string>>();
        var force = new HashSet<string>();
        int tsvCount = 0;
        foreach (var rawLine in tsvLines) {
            var line = rawLine.TrimEnd('\r');
            if (line.Length == 0 || line.StartsWith("#")) continue;
            var parts = line.Split('\t');
            if (parts.Length < 2) continue;
            var zh = Decode(parts[0]); var en = Decode(parts[1]);
            if (zh == null || en == null) continue;
            if (parts.Length >= 3 && parts[2].Trim() == "force") force.Add(zh);
            ovr.Add(new KeyValuePair<string, string>(zh, en));
            tsvCount++;
        }
        // 2. parse existing table
        var text = File.ReadAllText(srcPath, new UTF8Encoding(false));
        text = Regex.Replace(text, "(?<!C)trl\\+", "\\bCtrl+"); // idempotent legacy bad-key repair
        var dict = new Dictionary<string, string>();
        var order = new List<string>();
        var rx = new Regex("\\{\\s*L\"((?:[^\"\\\\]|\\\\.)*)\"\\s*,\\s*L\"((?:[^\"\\\\]|\\\\.)*)\"\\s*\\}");
        foreach (Match m in rx.Matches(text)) {
            var zh = Decode(m.Groups[1].Value);
            if (!dict.ContainsKey(zh)) { dict[zh] = Decode(m.Groups[2].Value); order.Add(zh); }
        }
        int dictBase = dict.Count;
        // 3. pass1: fill placeholders only (force overrides); keys missing from dict are added
        int a1 = 0, added = 0;
        foreach (var o in ovr) {
            string cur;
            if (!dict.TryGetValue(o.Key, out cur)) { dict[o.Key] = SpaceFix(o.Key, o.Value); order.Add(o.Key); added++; continue; }
            if (force.Contains(o.Key) || cur == o.Key) { dict[o.Key] = SpaceFix(o.Key, o.Value); a1++; }
        }
        // 4. scan sources for \b accelerator-key literals, add variants (base translation + suffix)
        int newB = 0;
        var bPending = new List<string>();
        foreach (var f in SourceFiles(root)) {
            string t;
            try { t = File.ReadAllText(f); } catch { continue; }
            var lits = new List<string>();
            GetLiterals(t, lits);
            foreach (var lit in lits) {
                if (!HasCjk(lit)) continue;
                int idx = lit.IndexOf('\b');
                if (idx < 0) continue;
                if (dict.ContainsKey(lit)) continue;
                var baseStr = lit.Substring(0, idx);
                var suffix = lit.Substring(idx + 1);
                string baseEn;
                bool hasBase = dict.TryGetValue(baseStr, out baseEn);
                if (!hasBase) baseEn = baseStr;
                dict[lit] = baseEn + "\b" + suffix;
                order.Add(lit); newB++;
                if (!hasBase || baseEn == baseStr) bPending.Add(lit);
            }
        }
        // 5. pass2: overrides against newly added \b variants
        int a2 = 0;
        foreach (var o in ovr) {
            string cur;
            if (!dict.TryGetValue(o.Key, out cur)) continue;
            if (!force.Contains(o.Key) && cur != o.Key) continue;
            var fx = SpaceFix(o.Key, o.Value);
            if (fx != cur) { dict[o.Key] = fx; a2++; }
        }
        // 6. stats
        int placeholders = 0;
        var phSample = new List<string>();
        foreach (var kv in dict)
            if (kv.Value == kv.Key && HasCjk(kv.Key)) {
                placeholders++;
                if (phSample.Count < 25) phSample.Add(kv.Key);
            }
        // 7. sort (Ordinal) + rebuild .c + generate .lang
        var keys = new string[order.Count];
        var items = new KeyValuePair<string, string>[order.Count];
        for (int i = 0; i < order.Count; i++) { keys[i] = order[i]; items[i] = new KeyValuePair<string, string>(order[i], EnClean(dict[order[i]], order[i])); }
        Array.Sort(keys, items, StringComparer.Ordinal);
        for (int i = 1; i < keys.Length; i++)
            if (StringComparer.Ordinal.Compare(keys[i - 1], keys[i]) >= 0)
                throw new Exception("sort failed at " + i);
        var sb = new StringBuilder();
        sb.Append("/*\r\n");
        sb.Append(" * 运行时 UI 文本翻译字典（zh→en）— 自动生成，勿手工编辑。\r\n");
        sb.Append(" * 生成器: _apply_trans.ps1（overrides 补翻 + \\b 变体补表）+ _fix_order.ps1\r\n");
        sb.Append(" * 基线:   1487bbf6e\r\n");
        sb.Append(" * 规则:   键 = 当前源码硬编码中文；值 = git 基线同位置英文。\r\n");
        sb.Append(" *         en=zh 的条目为占位（暂无英文），待补翻。\r\n");
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
            bw.Write((uint)32); bw.Write((uint)(32 + 8 * n)); bw.Write((uint)blob.Position); bw.Write((uint)0);
            for (int i = 0; i < n; i++) { bw.Write(zhOff[i]); bw.Write(enOff[i]); }
            bw.Write(blob.ToArray());
        }
        // 8. report
        var rep = new StringBuilder();
        rep.AppendLine("base=" + dictBase + " tsv=" + tsvCount + " pass1=" + a1 + " added=" + added + " bNew=" + newB + " pass2=" + a2);
        rep.AppendLine("final=" + n + " placeholdersLeft=" + placeholders + " enCleaned=" + enCleaned);
        foreach (var b in bPending) rep.AppendLine("B-PENDING: " + b.Replace("\b", "<b>"));
        foreach (var s in phSample) rep.AppendLine("PH: " + s.Replace("\b", "<b>"));
        return rep.ToString();
    }
}
'@

$tsv = [System.IO.File]::ReadAllLines(
    "d:\systeminformer\systeminformer\tools\Localization\trans_overrides.tsv",
    (New-Object System.Text.UTF8Encoding($true)))
$report = [TransApply]::Apply(
    "d:\systeminformer\systeminformer\phlib\translate_data.c",
    $tsv,
    "d:\systeminformer\systeminformer",
    "d:\systeminformer\systeminformer\bin\Release64\lang\zh-en.lang")
Write-Host $report
