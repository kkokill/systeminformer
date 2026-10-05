# Updater 插件词典核对：候选键在 translate_data.c 中的存在性/值一致性 + 插入前驱键定位
$ErrorActionPreference = 'Stop'
$src = 'd:\systeminformer\systeminformer\phlib\translate_data.c'
$text = [System.IO.File]::ReadAllText($src)

$cs = @'
using System;
using System.Collections.Generic;
using System.Text;
using System.Text.RegularExpressions;
public static class DictParse {
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
    // 每条: [0]=keySrc [1]=key(decoded) [2]=valSrc [3]=val(decoded)
    public static List<string[]> Parse(string text) {
        var rx = new Regex("\\{\\s*L\"((?:[^\"\\\\]|\\\\.)*)\"\\s*,\\s*L\"((?:[^\"\\\\]|\\\\.)*)\"\\s*\\}");
        var list = new List<string[]>();
        foreach (Match m in rx.Matches(text)) {
            list.Add(new string[] { m.Groups[1].Value, Decode(m.Groups[1].Value), m.Groups[2].Value, Decode(m.Groups[2].Value) });
        }
        return list;
    }
    public static string Dec(string s) { return Decode(s); }
}
'@
Add-Type -TypeDefinition $cs -Language CSharp

$entries = [DictParse]::Parse($text)
Write-Host ("dict keys: " + $entries.Count)
$dict = @{}
for ($i = 0; $i -lt $entries.Count; $i++) {
    $k = $entries[$i][1]
    if (-not $dict.ContainsKey($k)) { $dict[$k] = $i }
}

# 候选表：K/V 均为 C 源码转义形式
$cands = @(
    @{K='检查(&C)'; V='&Check'}
    @{K='稳定版\n - 推荐'; V='Stable\n - Recommended'}
    @{K='金丝雀版\n - 预览'; V='Canary\n - Preview'}
    @{K='System Informer - 更新器'; V='System Informer - Updater'}
    @{K='检查 System Informer 更新版本？'; V='Check for an updated System Informer release?'}
    @{K='点击“检查”以继续。'; V='Click Check to continue.'}
    @{K='正在检查 release 频道...'; V='Checking the release channel...'}
    @{K='正在检查 canary 频道...'; V='Checking the canary channel...'}
    @{K='正在检查频道...'; V='Checking the channel...'}
    @{K='正在检查更新的版本...'; V='Checking for an updated release...'}
    @{K='下载(&D)'; V='&Download'}
    @{K='是否下载 Release 版本？'; V='Would you like to download the Release build?'}
    @{K='是否下载 Canary 版本？'; V='Would you like to download the Canary build?'}
    @{K='是否下载更新？'; V='Would you like to download the update?'}
    @{K='有新版 System Informer 可供下载。'; V='A newer build of System Informer is available.'}
    @{K='版本: %s\r\n下载大小: %s\r\n\r\n<A HREF=\"changelog.txt\">查看变更日志</A>'; V='Version: %s\r\nDownload size: %s\r\n\r\n<A HREF=\"changelog.txt\">View the changelog</A>'}
    @{K='正在下载 %s 渠道 %s...'; V='Downloading%s channel %s...'}
    @{K='正在下载更新 %s...'; V='Downloading update %s...'}
    @{K='已下载: ~ / ~ (0%)\r\n速度: ~ KB/s'; V='Downloaded: ~ of ~ (0%)\r\nSpeed: ~ KB/s'}
    @{K='安装'; V='Install'}
    @{K='准备切换到 Release 渠道？'; V='Ready to switch to the release channel?'}
    @{K='准备切换到 Canary 渠道？'; V='Ready to switch to the canary channel?'}
    @{K='准备切换渠道？'; V='Ready to switch the channel?'}
    @{K='该渠道已成功下载并验证。\r\n\r\n点击“安装”继续。'; V='The channel has been successfully downloaded and verified.\r\n\r\nClick Install to continue.'}
    @{K='更新已安装。'; V='Update installed.'}
    @{K='更新已下载并安装。\r\n\r\n请重启 System Informer 以应用更新。'; V='The update has been downloaded and installed.\r\n\r\nRestart System Informer to apply the update.'}
    @{K='准备安装更新？'; V='Ready to install update?'}
    @{K='该更新已成功下载并验证。\r\n\r\n点击“安装”继续。'; V='The update has been successfully downloaded and verified.\r\n\r\nClick Install to continue.'}
    @{K='您正在运行最新版本。'; V='You''re running the latest version.'}
    @{K='您正在运行预发布版本。'; V='You''re running a pre-release build.'}
    @{K='下载渠道时出错。'; V='Error downloading the channel.'}
    @{K='下载更新时出错。'; V='Error downloading the update.'}
    @{K='签名检查失败。点击“重试”以重新下载该渠道。'; V='Signature check failed. Click Retry to download the channel again.'}
    @{K='签名检查失败。点击“重试”以重新下载该更新。'; V='Signature check failed. Click Retry to download the update again.'}
    @{K='哈希检查失败。点击“重试”以重新下载该渠道。'; V='Hash check failed. Click Retry to download the channel again.'}
    @{K='哈希检查失败。点击“重试”以重新下载该更新。'; V='Hash check failed. Click Retry to download the update again.'}
    @{K='点击“重试”以重新下载该渠道。'; V='Click Retry to download the channel again.'}
    @{K='点击“重试”以重新下载该更新。'; V='Click Retry to download the update again.'}
    @{K='%s\r\n\r\n<A HREF=\"changelog.txt\">查看变更日志</A>'; V='%s\r\n\r\n<A HREF=\"changelog.txt\">View changelog</A>'}
    @{K='有新版本的 System Informer 可供使用'; V='New version of System Informer available'}
    @{K='帮助菜单 > 检查更新'; V='Help menu > Check for updates'}
    @{K='正在初始化下载请求...'; V='Initializing download request...'}
    @{K='正在连接...'; V='Connecting...'}
    @{K='正在发送下载请求...'; V='Sending download request...'}
    @{K='正在等待响应...'; V='Waiting for response...'}
    @{K='正在下载发行版 %s...'; V='Downloading release %s...'}
    @{K='已下载：约 ~ / ~ (0%)\r\n速度：~ KB/s'; V='Downloaded: ~ of ~ (0%)\r\nSpeed: ~ KB/s'}
    @{K='已下载：'; V='Downloaded: '}
    @{K='%)\r\n速度：'; V='%)\r\nSpeed: '}
    @{K='正在初始化...'; V='Initializing...'}
    @{K='无法创建窗口。'; V='Unable to create the window.'}
    @{K='正在下载 release %s...'; V='Downloading release %s...'}
    @{K='1 天'; V='1 day'}
    @{K='1 周'; V='1 week'}
    @{K='1 月'; V='1 month'}
    @{K='上次更新检查：%s（%s 前）'; V='Last update check: %s (%s ago)'}
    @{K='下次更新检查：%s（%s）'; V='Next update check: %s (%s)'}
    @{K='下次更新检查：%s'; V='Next update check: %s'}
    @{K='日期'; V='Date'}
    @{K='作者'; V='Author'}
    @{K='提交信息'; V='Comments'}
    @{K='提交'; V='Commit'}
    @{K='正在查询变更日志...'; V='Querying changelog...'}
    @{K='在 Github 上查看'; V='View on Github'}
    @{K='复制(&C)'; V='&Copy'}
    @{K='检查更新(&U)'; V='Check for &updates'}
    @{K='更新器'; V='Updater'}
    @{K='更新检查器'; V='Update Checker'}
    @{K='通过帮助菜单检查新版 System Informer 发布的插件。'; V='Plugin for checking new System Informer releases via the Help menu.'}
    @{K='无法执行安装程序。'; V='Unable to execute the setup.'}
    @{K='速度: %s/s'; V='Speed: %s/s'}
    @{K='已下载: ~ / ~'; V='Downloaded: ~ of ~'}
    @{K='开始下载...'; V='Starting download...'}
    @{K='签名检查失败。'; V='Signature check failed.'}
    @{K='哈希检查失败。'; V='Hash check failed.'}
    @{K='点击“检查更新”以重试。'; V='Click Check for updates to try again.'}
    @{K='点击检查更新以重试。'; V='Click Check for updates to try again.'}
    @{K=('<toast launch=\"\" duration=\"long\"><visual><binding template=\"ToastGeneric\"><text>System Informer - 有可用更新</text><text>版本 %s（下载 %s）</text></binding></visual><actions><action content=\"下载\" arguments=\"download\" activationType=\"foreground\"/></actions></toast>'); V=('<toast launch=\"\" duration=\"long\"><visual><binding template=\"ToastGeneric\"><text>System Informer - Update Available</text><text>Version %s (download %s)</text></binding></visual><actions><action content=\"Download\" arguments=\"download\" activationType=\"foreground\"/></actions></toast>')}
    @{K=('<toast launch=\"\" duration=\"long\"><visual><binding template=\"ToastGeneric\"><text>正在下载 System Informer %s</text><progress title=\"\" status=\"{progressStatus}\" value=\"{progressValue}\" valueStringOverride=\"{progressValueString}\"/></binding></visual><actions><action content=\"关闭\" arguments=\"dismiss\" activationType=\"system\"/></actions></toast>'); V=('<toast launch=\"\" duration=\"long\"><visual><binding template=\"ToastGeneric\"><text>Downloading System Informer %s</text><progress title=\"\" status=\"{progressStatus}\" value=\"{progressValue}\" valueStringOverride=\"{progressValueString}\"/></binding></visual><actions><action content=\"Close\" arguments=\"dismiss\" activationType=\"system\"/></actions></toast>')}
    @{K=('<toast launch=\"\" scenario=\"reminder\"><visual><binding template=\"ToastGeneric\"><text>System Informer %s</text><text>更新已成功下载并验证。</text></binding></visual><actions><action content=\"安装\" arguments=\"install\" activationType=\"foreground\"/><action content=\"取消\" arguments=\"dismiss\" activationType=\"system\"/></actions></toast>'); V=('<toast launch=\"\" scenario=\"reminder\"><visual><binding template=\"ToastGeneric\"><text>System Informer %s</text><text>Update successfully downloaded and verified.</text></binding></visual><actions><action content=\"Install\" arguments=\"install\" activationType=\"foreground\"/><action content=\"Cancel\" arguments=\"dismiss\" activationType=\"system\"/></actions></toast>')}
    @{K=('<toast launch=\"\" duration=\"long\"><visual><binding template=\"ToastGeneric\"><text>System Informer 更新失败</text><text>%s</text></binding></visual></toast>'); V=('<toast launch=\"\" duration=\"long\"><visual><binding template=\"ToastGeneric\"><text>System Informer update failed</text><text>%s</text></binding></visual></toast>')}
    @{K='正在完成 BITS 下载...'; V='Finalizing BITS download...'}
    @{K='正在排队 BITS 下载...'; V='Queued BITS download...'}
    @{K='正在通过 BITS 连接...'; V='Connecting with BITS...'}
    @{K='正在通过 BITS 下载...'; V='Downloading with BITS...'}
    @{K='BITS 下载重试中...'; V='BITS download retry pending...'}
    @{K='正在初始化 BITS 下载...'; V='Initializing BITS download...'}
    @{K='正在完成 Delivery Optimization 下载...'; V='Finalizing Delivery Optimization download...'}
    @{K='正在初始化 Delivery Optimization...'; V='Initializing Delivery Optimization...'}
    @{K='正在开始 Delivery Optimization 下载...'; V='Starting Delivery Optimization download...'}
    @{K='更新器选项'; V='Updater Options'}
    @{K='自动检查更新'; V='Check for updates automatically'}
    @{K='上次更新检查：N/A'; V='Last update check: N/A'}
    @{K='下次更新检查：N/A'; V='Next update check: N/A'}
    @{K='在主窗口前显示更新提示'; V='Show update prompt before main window'}
    @{K='跳过检查更新并自动搜索'; V='Skip check for updates and search automatically'}
    @{K='显示更新通知而非更新提示'; V='Show update notifications instead of update prompts'}
    @{K='变更日志'; V='Changelog'}
    @{K='确定'; V='OK'}
)

$match = 0; $diff = 0; $missing = 0
foreach ($c in $cands) {
    $k = [DictParse]::Dec($c.K)
    $v = [DictParse]::Dec($c.V)
    if ($dict.ContainsKey($k)) {
        $e = $entries[$dict[$k]]
        if ($e[3] -ceq $v) { $match++ }
        else { $diff++; Write-Host ("DIFF  [" + $c.K + "] dict=" + $e[2] + " want=" + $c.V) }
    }
    else {
        $missing++
        $pred = $null
        for ($i = $entries.Count - 1; $i -ge 0; $i--) {
            if ([string]::CompareOrdinal($entries[$i][1], $k) -lt 0) { $pred = $entries[$i][0]; break }
        }
        Write-Host ("MISS  [" + $c.K + "]")
        Write-Host ("      after: " + $(if ($pred) { $pred } else { '(insert at head)' }))
    }
}
Write-Host ("match=" + $match + " diff=" + $diff + " missing=" + $missing)
