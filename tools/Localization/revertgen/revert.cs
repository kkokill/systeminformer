// 还原静态初始化表中的 PhT() 包装：PhT(L"X") → L"X"
// 用法: revert <file...>
using System;
using System.IO;
using System.Text;
using System.Text.RegularExpressions;

class Revert
{
    static void Main(string[] args)
    {
        var rx = new Regex("PhT\\((L\"(?:\\\\.|[^\"\\\\])*\")\\)");
        foreach (var path in args)
        {
            var text = File.ReadAllText(path, Encoding.UTF8);
            int count = 0;
            var replaced = rx.Replace(text, m => { count++; return m.Groups[1].Value; });
            if (count > 0)
            {
                File.WriteAllText(path, replaced, new UTF8Encoding(true));
            }
            Console.WriteLine("{0}: reverted {1}", Path.GetFileName(path), count);
        }
    }
}
