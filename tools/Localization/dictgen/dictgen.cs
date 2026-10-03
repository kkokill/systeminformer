// 中英字典生成器：从 git diff 基线(英文)到 HEAD(中文) 自动配对字符串
// 用法: dictgen.exe <repoPath> <baseline>
// 输出: dict_pairs.tsv / dict_stats.txt / run_summary.txt
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Text;
using System.Text.RegularExpressions;

class Pair { public string Zh, En, File; }

class Program
{
    static readonly Regex CjkRegex = new Regex(@"[\u4E00-\u9FFF\u3000-\u303F\uFF00-\uFFEF]");
    static readonly Regex LitRegex = new Regex("L?\"((\\\\.|[^\"\\\\])*)\"", RegexOptions.Compiled);
    static readonly List<Pair> Pairs = new List<Pair>();
    static readonly List<KeyValuePair<string, string>> UnmatchedZh = new List<KeyValuePair<string, string>>();
    static readonly List<string> Conflicts = new List<string>();
    static readonly List<string> Mismatches = new List<string>();
    static string CurFile = "";
    static readonly List<string> OldLines = new List<string>();
    static readonly List<string> NewLines = new List<string>();
    static bool InHunk, NewFileMode;
    static int PairOkHunks, PairFailHunks;

    static void Main(string[] args)
    {
        var repo = args[0];
        var baseline = args[1];
        var outDir = Path.GetDirectoryName(Process.GetCurrentProcess().MainModule.FileName);

        string diff;
        var psi = new ProcessStartInfo("git", string.Format("-c core.quotepath=false diff {0} HEAD -U0 --no-color --no-prefix", baseline))
        {
            WorkingDirectory = repo,
            RedirectStandardOutput = true,
            RedirectStandardError = true,
            UseShellExecute = false,
            StandardOutputEncoding = Encoding.UTF8,
        };
        using (var p = Process.Start(psi))
        {
            diff = p.StandardOutput.ReadToEnd();
            var err = p.StandardError.ReadToEnd();
            p.WaitForExit();
            if (p.ExitCode != 0) { File.WriteAllText(Path.Combine(outDir, "run_summary.txt"), "git failed: " + err); return; }
        }

        foreach (var line in diff.Split('\n'))
            ProcessLine(line.TrimEnd('\r'));

        FlushHunk();

        var map = new Dictionary<string, string>();
        foreach (var p in Pairs)
        {
            if (map.ContainsKey(p.Zh)) { if (map[p.Zh] != p.En) Conflicts.Add(p.Zh + " => [" + map[p.Zh] + "] vs [" + p.En + "] (" + p.File + ")"); }
            else map[p.Zh] = p.En;
        }

        WriteTsv(Path.Combine(outDir, "dict_pairs.tsv"), Pairs);
        WriteStats(Path.Combine(outDir, "dict_stats.txt"), map);
        WriteDataFile(map, Path.Combine(outDir, "translate_data.c"));
        File.WriteAllText(Path.Combine(outDir, "run_summary.txt"),
            "diffLines=done\nhunksPaired=" + PairOkHunks + "\nhunksFailed=" + PairFailHunks +
            "\nrawPairs=" + Pairs.Count + "\nuniqueZh=" + map.Count + "\nconflicts=" + Conflicts.Count +
            "\nunmatchedZh=" + UnmatchedZh.Count + "\n", Encoding.UTF8);
    }

    static void WriteDataFile(Dictionary<string, string> map, string path)
    {
        using (var sw = new StreamWriter(path, false, new UTF8Encoding(true)))
        {
            sw.WriteLine("// 由 tools\\Localization\\dictgen.exe 从 git 历史自动生成，勿手改。");
            sw.WriteLine("// 中英翻译表：key = 源码中的中文串（源码转义形式），value = 基线英文串。");
            sw.WriteLine("// 本文件被 phlib\\guisup.c 末尾 #include，类型定义见 guisup.c。");
            sw.WriteLine();
            sw.WriteLine("const PH_TRANSLATION_ENTRY PhTranslationTable[] =");
            sw.WriteLine("{");
            foreach (var kv in map)
            {
                sw.WriteLine("    { L\"" + kv.Key + "\", L\"" + kv.Value + "\" },");
            }
            sw.WriteLine("};");
            sw.WriteLine();
            sw.WriteLine("const ULONG PhTranslationTableCount = RTL_NUMBER_OF(PhTranslationTable);");
        }
    }

    static void ProcessLine(string line)
    {
        if (line.StartsWith("diff --git "))
        {
            FlushHunk();
            var parts = line.Split(' ');
            CurFile = parts[parts.Length - 1];
            NewFileMode = false;
        }
        else if (line.StartsWith("new file mode")) NewFileMode = true;
        else if (line.StartsWith("deleted file mode")) { NewFileMode = false; CurFile = ""; }
        else if (line.StartsWith("@@")) { FlushHunk(); InHunk = true; OldLines.Clear(); NewLines.Clear(); }
        else if (InHunk)
        {
            if (line.StartsWith("-")) OldLines.Add(line.Substring(1));
            else if (line.StartsWith("+")) NewLines.Add(line.Substring(1));
        }
    }

    static void FlushHunk()
    {
        if (!InHunk) return;
        InHunk = false;
        if (NewFileMode)
        {
            foreach (var l in NewLines)
                foreach (var lit in GetLiterals(l))
                    if (CjkRegex.IsMatch(lit.Item2))
                        UnmatchedZh.Add(new KeyValuePair<string, string>(lit.Item2, CurFile));
            return;
        }
        if (OldLines.Count == 0 && NewLines.Count == 0) return;
        var oldLits = new List<Tuple<bool, string>>();
        var newLits = new List<Tuple<bool, string>>();
        foreach (var l in OldLines) oldLits.AddRange(GetLiterals(l));
        foreach (var l in NewLines) newLits.AddRange(GetLiterals(l));

        if (oldLits.Count > 0 && oldLits.Count == newLits.Count)
        {
            PairOkHunks++;
            for (int i = 0; i < oldLits.Count; i++)
            {
                var en = oldLits[i].Item2; var zh = newLits[i].Item2;
                if (string.CompareOrdinal(zh, en) == 0) continue;   // 未变化的串
                if (!CjkRegex.IsMatch(zh)) continue;                 // 新串不含中文
                if (CjkRegex.IsMatch(en)) continue;                  // 旧串异常含中文
                Pairs.Add(new Pair { Zh = zh, En = en, File = CurFile });
            }
        }
        else
        {
            PairFailHunks++;
            var hadZh = false;
            foreach (var lit in newLits)
                if (CjkRegex.IsMatch(lit.Item2))
                {
                    hadZh = true;
                    UnmatchedZh.Add(new KeyValuePair<string, string>(lit.Item2, CurFile));
                }
            if (hadZh)
                Mismatches.Add(string.Format("{0}: oldLits={1} newLits={2} oldLines={3} newLines={4}",
                    CurFile, oldLits.Count, newLits.Count, OldLines.Count, NewLines.Count));
        }
    }

    static IEnumerable<Tuple<bool, string>> GetLiterals(string s)
    {
        var result = new List<Tuple<bool, string>>();
        foreach (Match m in LitRegex.Matches(s))
            result.Add(Tuple.Create(m.Value[0] == 'L', m.Groups[1].Value));
        return result;
    }

    static void WriteTsv(string path, List<Pair> pairs)
    {
        using (var sw = new StreamWriter(path, false, new UTF8Encoding(true)))
        {
            sw.WriteLine("zh\ten\tpairfile");
            foreach (var p in pairs) sw.WriteLine(p.Zh + "\t" + p.En + "\t" + p.File);
        }
    }

    static void WriteStats(string path, Dictionary<string, string> map)
    {
        using (var sw = new StreamWriter(path, false, new UTF8Encoding(true)))
        {
            sw.WriteLine("== 配对冲突 (同 zh 不同 en)，取先出现的 ==");
            foreach (var c in Conflicts) sw.WriteLine(c);
            sw.WriteLine();
            sw.WriteLine("== hunk 配对失败统计 ==");
            foreach (var m in Mismatches) sw.WriteLine(m);
            sw.WriteLine();
            sw.WriteLine("== 未匹配中文串（新增代码/配对失败 hunk），英文模式将回退中文 ==");
            var seen = new HashSet<string>();
            foreach (var u in UnmatchedZh)
                if (seen.Add(u.Key + "|" + u.Value))
                    sw.WriteLine(u.Key + "\t" + u.Value);
        }
    }
}
