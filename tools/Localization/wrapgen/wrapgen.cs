// 源码包装工具：把 .c/.cpp 中的 L"中文" 包裹为 PhT(L"中文")
// 用法: wrapgen.exe <repoPath> [--dry]
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Text;
using System.Text.RegularExpressions;

class Program
{
    static readonly Regex CjkRegex = new Regex(@"[\u4E00-\u9FFF\u3000-\u303F\uFF00-\uFFEF]");
    // 仅匹配含 CJK 的宽字符串字面量，且 L 前不是标识符字符
    static readonly Regex WideCjkLiteral = new Regex(
        "(?<![A-Za-z0-9_])L\"((\\\\.|[^\"\\\\])*)\"",
        RegexOptions.Compiled);

    static readonly string[] SkipLinePatterns =
    {
        "PhT(",                        // 已包装（幂等）
        "CreateListSection(",          // mini 区段名：显示+匹配双身份，保持中文
        "SectionName =",               // 同上（notifico/iconext）
        "PH_STRINGREF_INIT(",          // 静态初始化，不能调用函数
        "SIP(",                        // smbios 静态表
        "PhSearchControlMatch",        // 搜索匹配候选词
    };

    static int Main(string[] args)
    {
        if (args.Length < 1) { Console.Error.WriteLine("usage: wrapgen <repo> [--dry]"); return 1; }
        var repo = args[0];
        bool dry = args.Length > 1 && args[1] == "--dry";
        var rootDirs = new[] { "SystemInformer", "phlib", "plugins", "tools" };

        var files = new List<string>();
        foreach (var d in rootDirs)
        {
            var dir = Path.Combine(repo, d);
            if (!Directory.Exists(dir)) continue;
            foreach (var pattern in new[] { "*.c", "*.cpp" })
                foreach (var f in Directory.EnumerateFiles(dir, pattern, SearchOption.AllDirectories))
                {
                    var norm = f.Replace('\\', '/');
                    if (norm.Contains("/thirdparty/") || norm.Contains("/obj/") || norm.Contains("/bin/") || norm.Contains("/Localization/")) continue;
                    files.Add(f);
                }
        }

        int totalWrapped = 0, filesChanged = 0;
        var risks = new List<string>();
        var noHeader = new List<string>();

        foreach (var file in files)
        {
            var bytes = File.ReadAllBytes(file);
            bool hasBom = bytes.Length >= 3 && bytes[0] == 0xEF && bytes[1] == 0xBB && bytes[2] == 0xBF;
            string text;
            try
            {
                text = new UTF8Encoding(false, true).GetString(hasBom ? bytes.Substring(3) : bytes);
            }
            catch (DecoderFallbackException)
            {
                risks.Add("SKIP-NONUTF8 " + file);
                continue;
            }

            var lines = text.Split('\n');
            int wrapped = 0;
            bool changed = false;

            for (int i = 0; i < lines.Length; i++)
            {
                var line = lines[i];
                if (!CjkRegex.IsMatch(line)) continue;

                bool skip = false;
                foreach (var p in SkipLinePatterns)
                    if (line.Contains(p)) { skip = true; break; }
                var trimmed = line.TrimStart();
                if (trimmed.StartsWith("#")) skip = true;
                if (skip) continue;

                var newLine = WideCjkLiteral.Replace(line, m => "PhT(" + m.Value + ")");
                if (newLine != line)
                {
                    wrapped += CountMatches(line);
                    changed = true;
                    lines[i] = newLine;

                    // 相邻字面量风险检测：PhT(...) 与其他字面量相邻会导致 C 编译错误
                    if (Regex.IsMatch(newLine, @"PhT\((L""(\\.|[^""\\])*"")\)\s*L""") ||
                        Regex.IsMatch(newLine, @"L""(\\.|[^""\\])*""\s*PhT\("))
                        risks.Add("ADJACENT " + file + ":" + (i + 1));
                }
            }

            if (!changed) continue;

            if (dry)
            {
                Console.WriteLine(file.Replace(repo, "").TrimStart('\\') + " " + wrapped);
            }
            else
            {
                var enc = new UTF8Encoding(hasBom);
                File.WriteAllText(file, string.Join("\n", lines), enc);
            }

            totalWrapped += wrapped;
            filesChanged++;
            if (!text.Contains("guisup.h") && !text.Contains("phapp.h") && !text.Contains("phgui.h"))
                noHeader.Add(file.Replace(repo, "").TrimStart('\\'));
        }

        var report = new StringBuilder();
        report.AppendLine(dry ? "DRY RUN" : "APPLIED");
        report.AppendLine("filesChanged=" + filesChanged);
        report.AppendLine("totalWrapped=" + totalWrapped);
        report.AppendLine("-- risks --");
        foreach (var r in risks) report.AppendLine(r);
        report.AppendLine("-- no header (may need #include) --");
        foreach (var h in noHeader) report.AppendLine(h);
        File.WriteAllText(Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "wrap_report.txt"), report.ToString(), new UTF8Encoding(true));
        Console.WriteLine("filesChanged=" + filesChanged + " totalWrapped=" + totalWrapped +
            " risks=" + risks.Count + " noHeader=" + noHeader.Count);
        return 0;
    }

    static int CountMatches(string line)
    {
        int n = 0;
        foreach (Match m in WideCjkLiteral.Matches(line)) n++;
        return n;
    }
}

static class ByteArrayExt
{
    public static byte[] Substring(this byte[] b, int start)
    {
        var r = new byte[b.Length - start];
        Array.Copy(b, start, r, 0, r.Length);
        return r;
    }
}
