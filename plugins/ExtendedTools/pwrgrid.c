/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     OpenAI    2026
 *
 */

#include "exttools.h"
#include <float.h>
#include <roapi.h>
#include <windows.devices.power.h>

#define ET_WM_POWERGRID_UPDATE (WM_APP + 1)

// 峰谷电价时段类型
#define ET_TARIFF_VALLEY        1   // 低谷
#define ET_TARIFF_FLAT          2   // 平段
#define ET_TARIFF_PEAK          3   // 高峰
#define ET_TARIFF_CRITICAL      4   // 尖峰
#define ET_TARIFF_MINUTES_PER_DAY 1440

// 显示模式
#define ET_POWER_GRID_MODE_AUTO         0   // 自动（预测数据不可用时切换峰谷）
#define ET_POWER_GRID_MODE_FORECAST     1   // 电网预测
#define ET_POWER_GRID_MODE_TARIFF       2   // 峰谷电价
#define ET_POWER_GRID_MODE_PENDING      255 // 等待预测数据

DEFINE_GUID(IID_IPowerGridData, 0xc360fb17, 0xfc92, 0x5f6e, 0x99, 0x9d, 0x16, 0xa4, 0xcf, 0x9d, 0x6c, 0x40);
DEFINE_GUID(IID_IPowerGridForecast, 0x077e4de9, 0xed60, 0x58bb, 0xa8, 0x50, 0x00, 0x3c, 0x6a, 0x13, 0x86, 0x85);
DEFINE_GUID(IID_IPowerGridForecastStatics, 0x5b78c806, 0x2e4e, 0x5bcc, 0xbb, 0x34, 0xcb, 0x81, 0xc6, 0x0f, 0x9e, 0x12);

typedef struct _ET_POWER_GRID_SUMMARY
{
    __x_ABI_CWindows_CFoundation_CDateTime ForecastStart;
    __x_ABI_CWindows_CFoundation_CTimeSpan BlockDuration;
    __x_ABI_CWindows_CFoundation_CDateTime BestLowStart;
    __x_ABI_CWindows_CFoundation_CDateTime LowestStart;
    __x_ABI_CWindows_CFoundation_CDateTime HighestStart;
    ULONG TotalBlocks;
    ULONG LowImpactCount;
    DOUBLE AverageSeverity;
    DOUBLE BestLowSeverity;
    DOUBLE LowestSeverity;
    DOUBLE HighestSeverity;
    // 峰谷电价模式专用
    BOOLEAN TariffValid;
    BOOLEAN TariffFallback;
    BOOLEAN TariffAutoSwitched;
    BOOLEAN TariffHasValley;
    BOOLEAN TariffHasPeak;
    UCHAR TariffCurrentType;
    __x_ABI_CWindows_CFoundation_CDateTime TariffCurrentStart;
    __x_ABI_CWindows_CFoundation_CDateTime TariffCurrentEnd;
    __x_ABI_CWindows_CFoundation_CDateTime TariffNextValleyStart;
    __x_ABI_CWindows_CFoundation_CDateTime TariffNextPeakStart;
    UINT64 TariffTableEnd;
} ET_POWER_GRID_SUMMARY, *PET_POWER_GRID_SUMMARY;

typedef struct _POWER_GRID_WINDOW_CONTEXT
{
    HWND WindowHandle;
    HWND ListViewHandle;
    HWND HeaderHandle;
    HWND SummaryHandle;
    HWND ModeButtonHandle;
    HWND ProvinceComboHandle;
    HWND TariffCfgButtonHandle;
    HWND RefreshButtonHandle;
    HWND ParentWindowHandle;
    HFONT WindowFont;
    PH_LAYOUT_MANAGER LayoutManager;
    UCHAR ModeSetting;
    UCHAR EffectiveMode;
    PPH_LIST CurrentEntries;
    PPH_STRING TariffTemplateName;
    UINT64 TariffTableEndTicks;
    ET_POWER_GRID_SUMMARY TariffSummary;
} POWER_GRID_WINDOW_CONTEXT, *PPOWER_GRID_WINDOW_CONTEXT;

typedef struct _ET_POWER_GRID_UPDATE
{
    PPH_LIST Entries;
    ET_POWER_GRID_SUMMARY Summary;
} ET_POWER_GRID_UPDATE, *PET_POWER_GRID_UPDATE;

typedef struct _ET_POWER_GRID_ENTRY
{
    PPH_STRING Severity;
    PPH_STRING LowImpact;
    PPH_STRING StartTime;
    PPH_STRING BlockDuration;
    PPH_STRING TimeUntilStart;
    DOUBLE SeverityValue;
    boolean LowImpactFlag;
    boolean ActiveFlag;
    __x_ABI_CWindows_CFoundation_CDateTime BlockStartTime;
    __x_ABI_CWindows_CFoundation_CDateTime BlockEndTime;
    UCHAR TariffType;
} ET_POWER_GRID_ENTRY, *PET_POWER_GRID_ENTRY;

// 峰谷时段规则：EndMinute <= StartMinute 表示跨午夜
typedef struct _ET_POWER_GRID_TARIFF_RULE
{
    UCHAR Type;
    USHORT StartMinute;
    USHORT EndMinute;
} ET_POWER_GRID_TARIFF_RULE, *PET_POWER_GRID_TARIFF_RULE;

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS NTAPI EtpEnumeratePowerGridForecast(
    _In_ PVOID ThreadParameter
    );

VOID EtpFreePowerGridEntry(
    _In_ PET_POWER_GRID_ENTRY Entry
    )
{
    if (!Entry)
        return;

    PhClearReference(&Entry->Severity);
    PhClearReference(&Entry->LowImpact);
    PhClearReference(&Entry->StartTime);
    PhClearReference(&Entry->BlockDuration);
    PhClearReference(&Entry->TimeUntilStart);
    PhFree(Entry);
}

VOID EtpFreePowerGridList(
    _In_opt_ PPH_LIST List
    )
{
    if (!List)
        return;

    for (ULONG i = 0; i < List->Count; i++)
    {
        EtpFreePowerGridEntry(List->Items[i]);
    }

    PhDereferenceObject(List);
}

VOID EtpClearPowerGridListView(
    _In_ HWND ListViewHandle
    )
{
    ListView_DeleteAllItems(ListViewHandle);
}

__x_ABI_CWindows_CFoundation_CDateTime EtpAddTicks(
    __x_ABI_CWindows_CFoundation_CDateTime DateTime,
    INT64 Ticks
    )
{
    __x_ABI_CWindows_CFoundation_CDateTime Result = DateTime;

    Result.UniversalTime += Ticks;
    return Result;
}

VOID EtpFormatPowerGridDateTime(
    _In_ __x_ABI_CWindows_CFoundation_CDateTime DateTime,
    _Out_writes_(PH_DATETIME_STR_LEN_1) PWSTR Buffer,
    _Out_opt_ PSIZE_T ReturnLength
    )
{
    LARGE_INTEGER largeInteger;
    SYSTEMTIME systemTime;
    SYSTEMTIME localSystemTime;

    if (!DateTime.UniversalTime)
    {
        wcscpy_s(Buffer, PH_DATETIME_STR_LEN_1, L"N/A");
        return;
    }

    largeInteger.QuadPart = DateTime.UniversalTime;
    PhLargeIntegerToSystemTime(&systemTime, &largeInteger);

    if (PhSystemTimeToTzSpecificLocalTime(&systemTime, &localSystemTime) &&
        PhFormatDateTimeToBuffer(&localSystemTime, Buffer, PH_DATETIME_STR_LEN_1 * sizeof(WCHAR), ReturnLength))
    {
        return;
    }

    if (PhFormatDateTimeToBuffer(&systemTime, Buffer, PH_DATETIME_STR_LEN_1 * sizeof(WCHAR), ReturnLength))
        return;

    wcscpy_s(Buffer, PH_DATETIME_STR_LEN_1, L"N/A");
}

// 仅格式化本地时间部分（不显示日期），用于峰谷列表时间列
VOID EtpFormatPowerGridTimeOnly(
    _In_ __x_ABI_CWindows_CFoundation_CDateTime DateTime,
    _Out_writes_(64) PWSTR Buffer
    )
{
    LARGE_INTEGER largeInteger;
    SYSTEMTIME systemTime;
    SYSTEMTIME localSystemTime;

    if (!DateTime.UniversalTime)
    {
        wcscpy_s(Buffer, 64, L"N/A");
        return;
    }

    largeInteger.QuadPart = DateTime.UniversalTime;
    PhLargeIntegerToSystemTime(&systemTime, &largeInteger);

    if (PhSystemTimeToTzSpecificLocalTime(&systemTime, &localSystemTime) &&
        GetTimeFormat(LOCALE_USER_DEFAULT, 0, &localSystemTime, NULL, Buffer, 64))
    {
        return;
    }

    wcscpy_s(Buffer, 64, L"N/A");
}

// 仅格式化 时:分:秒（时段均不足一天，不显示天部分）
VOID EtpFormatPowerGridDuration(
    _In_ __x_ABI_CWindows_CFoundation_CTimeSpan Duration,
    _Out_writes_(PH_TIMESPAN_STR_LEN_1) PWSTR Buffer
    )
{
    ULONG totalSeconds;
    ULONG hours;
    ULONG minutes;
    ULONG seconds;

    if (Duration.Duration <= 0)
    {
        wcscpy_s(Buffer, PH_TIMESPAN_STR_LEN_1, L"N/A");
        return;
    }

    totalSeconds = (ULONG)(Duration.Duration / PH_TICKS_PER_SEC);
    hours = totalSeconds / 3600;
    minutes = (totalSeconds % 3600) / 60;
    seconds = totalSeconds % 60;

    swprintf_s(Buffer, PH_TIMESPAN_STR_LEN_1, L"%lu:%02lu:%02lu", hours, minutes, seconds);
}

VOID EtAddSummaryWindowText(
    _In_ HWND SummaryHandle,
    _In_ const ET_POWER_GRID_SUMMARY* Summary
    )
{
    PH_STRING_BUILDER sb;
    WCHAR startBuffer[PH_DATETIME_STR_LEN_1];
    WCHAR endBuffer[PH_DATETIME_STR_LEN_1];
    __x_ABI_CWindows_CFoundation_CDateTime forecastEnd;
    __x_ABI_CWindows_CFoundation_CDateTime bestLowEnd;
    DOUBLE blockMinutes;
    DOUBLE totalMinutes;
    DOUBLE lowImpactRatio;

    if (!Summary || Summary->TotalBlocks == 0)
    {
        SetWindowText(SummaryHandle, L"无可用的预测数据。");
        return;
    }

    forecastEnd = EtpAddTicks(Summary->ForecastStart, (INT64)Summary->TotalBlocks * Summary->BlockDuration.Duration);
    blockMinutes = (DOUBLE)Summary->BlockDuration.Duration / (DOUBLE)PH_TICKS_PER_MIN;
    totalMinutes = blockMinutes * Summary->TotalBlocks;
    lowImpactRatio = (DOUBLE)Summary->LowImpactCount * 100.0 / (DOUBLE)Summary->TotalBlocks;

    EtpFormatPowerGridDateTime(Summary->ForecastStart, startBuffer, NULL);
    EtpFormatPowerGridDateTime(forecastEnd, endBuffer, NULL);

    PhInitializeStringBuilder(&sb, 512);
    PhAppendFormatStringBuilder(&sb, L"预测时段：%s 至 %s\r\n", startBuffer, endBuffer);
    PhAppendFormatStringBuilder(&sb, L"总时段数：%u（每段约 %.0f 分钟，共约 %.0f 分钟）\r\n", Summary->TotalBlocks, blockMinutes, totalMinutes);
    PhAppendFormatStringBuilder(&sb, L"平均严重程度：%.4f\r\n", Summary->AverageSeverity);
    PhAppendFormatStringBuilder(&sb, L"低影响时段：%u / %u（%.1f%%）\r\n", Summary->LowImpactCount, Summary->TotalBlocks, lowImpactRatio);

    if (Summary->BestLowSeverity < DBL_MAX)
    {
        WCHAR rangeFrom[PH_DATETIME_STR_LEN_1];
        WCHAR rangeTo[PH_DATETIME_STR_LEN_1];

        bestLowEnd = EtpAddTicks(Summary->BestLowStart, Summary->BlockDuration.Duration);
        EtpFormatPowerGridDateTime(Summary->BestLowStart, rangeFrom, NULL);
        EtpFormatPowerGridDateTime(bestLowEnd, rangeTo, NULL);
        PhAppendFormatStringBuilder(&sb, L"预测低影响时间：严重程度 %.4f，%s 至 %s\r\n", Summary->BestLowSeverity, rangeFrom, rangeTo);
    }
    else
    {
        PhAppendStringBuilder2(&sb, L"预测低影响时间：N/A\r\n");
    }

    PhSetWindowText(SummaryHandle, PhFinalStringBuilderString(&sb)->Buffer);
    PhDeleteStringBuilder(&sb);
}

PPH_STRING EtpFormatRelativeTimeString(
    _In_ __x_ABI_CWindows_CFoundation_CDateTime Target,
    _In_ __x_ABI_CWindows_CFoundation_CDateTime Reference
    )
{
    if (!Target.UniversalTime)
        return PhCreateString(L"N/A");

    INT64 delta = Target.UniversalTime - Reference.UniversalTime;
    BOOLEAN future = delta >= 0;

    if (!future)
        delta = -delta;

    // 分钟精度（秒四舍五入进位），避免秒级倒计时频繁重绘
    ULONG totalMinutes = (ULONG)((delta + (INT64)PH_TICKS_PER_MIN / 2) / (INT64)PH_TICKS_PER_MIN);
    ULONG hours = totalMinutes / 60;
    ULONG minutes = totalMinutes % 60;

    if (totalMinutes == 0)
    {
        return PhCreateString(future ? L"即将开始" : L"刚刚");
    }

    if (hours > 0)
    {
        if (minutes > 0)
            return PhFormatString(future ? L"%u 小时 %u 分后" : L"%u 小时 %u 分前", hours, minutes);

        return PhFormatString(future ? L"%u 小时后" : L"%u 小时前", hours);
    }

    return PhFormatString(future ? L"%u 分后" : L"%u 分前", minutes);
}

BOOLEAN EtpIsPowerGridEntryActive(
    _In_ __x_ABI_CWindows_CFoundation_CDateTime Start,
    _In_ __x_ABI_CWindows_CFoundation_CDateTime End,
    _In_ __x_ABI_CWindows_CFoundation_CDateTime Reference
    )
{
    if (!Start.UniversalTime || !End.UniversalTime || !Reference.UniversalTime)
        return FALSE;

    return (Reference.UniversalTime >= Start.UniversalTime) && (Reference.UniversalTime < End.UniversalTime);
}

static PCWSTR EtpGetTariffTypeName(
    _In_ UCHAR Type
    )
{
    switch (Type)
    {
    case ET_TARIFF_VALLEY:
        return L"低谷";
    case ET_TARIFF_PEAK:
        return L"高峰";
    case ET_TARIFF_CRITICAL:
        return L"尖峰";
    default:
        return L"平段";
    }
}

#define ET_TARIFF_IS_DIGIT(c) ((c) >= L'0' && (c) <= L'9')

// 解析 H:M[:S]，允许 24:00 表示午夜结束；秒被忽略
static BOOLEAN EtpParseTariffTime(
    _Inout_ PCWSTR* String,
    _Out_ USHORT* Minute
    )
{
    PCWSTR p = *String;
    ULONG hour = 0;
    ULONG minute = 0;

    if (!ET_TARIFF_IS_DIGIT(*p))
        return FALSE;

    while (ET_TARIFF_IS_DIGIT(*p))
        hour = hour * 10 + (*p++ - L'0');

    if (hour > 24)
        return FALSE;

    if (*p++ != L':')
        return FALSE;

    if (!ET_TARIFF_IS_DIGIT(*p))
        return FALSE;

    while (ET_TARIFF_IS_DIGIT(*p))
        minute = minute * 10 + (*p++ - L'0');

    if (minute > 59)
        return FALSE;

    if (hour == 24 && minute != 0)
        return FALSE;

    // 可选秒，忽略
    if (*p == L':')
    {
        p++;

        if (!ET_TARIFF_IS_DIGIT(*p))
            return FALSE;

        while (ET_TARIFF_IS_DIGIT(*p))
            p++;
    }

    *Minute = (USHORT)(hour * 60 + minute);
    *String = p;
    return TRUE;
}

// 解析 "类型:起-止;..."（1=低谷 2=平段 3=高峰 4=尖峰），32 条上限，任一条非法即整串拒绝
static BOOLEAN EtpParseTariffConfig(
    _In_ PCWSTR Config,
    _In_ PPH_LIST Rules
    )
{
    PCWSTR p = Config;

    while (*p)
    {
        ULONG type;
        USHORT startMinute;
        USHORT endMinute;
        PET_POWER_GRID_TARIFF_RULE rule;

        if (!ET_TARIFF_IS_DIGIT(*p))
            return FALSE;

        type = *p++ - L'0';

        if (type < ET_TARIFF_VALLEY || type > ET_TARIFF_CRITICAL)
            return FALSE;

        if (*p++ != L':')
            return FALSE;

        if (!EtpParseTariffTime(&p, &startMinute))
            return FALSE;

        if (*p++ != L'-')
            return FALSE;

        if (!EtpParseTariffTime(&p, &endMinute))
            return FALSE;

        if (startMinute == endMinute)
            return FALSE;

        if (Rules->Count >= 32)
            return FALSE;

        rule = PhAllocate(sizeof(ET_POWER_GRID_TARIFF_RULE));
        rule->Type = (UCHAR)type;
        rule->StartMinute = startMinute;
        rule->EndMinute = endMinute;
        PhAddItemList(Rules, rule);

        if (*p == L';')
        {
            p++;
            continue;
        }

        if (*p == 0)
            break;

        return FALSE;
    }

    return Rules->Count > 0;
}

static VOID EtpClearTariffRules(
    _In_ PPH_LIST Rules
    )
{
    for (ULONG i = 0; i < Rules->Count; i++)
    {
        PhFree(Rules->Items[i]);
    }

    Rules->Count = 0;
}

static VOID EtpFreeTariffRules(
    _In_ PPH_LIST Rules
    )
{
    EtpClearTariffRules(Rules);
    PhDereferenceObject(Rules);
}

// 将规则写入分钟轴；跨午夜拆分为 [start, 1440) + [0, end)
static VOID EtpApplyTariffRule(
    _Out_writes_(ET_TARIFF_MINUTES_PER_DAY) PBYTE Axis,
    _In_ const ET_POWER_GRID_TARIFF_RULE* Rule
    )
{
    USHORT start = Rule->StartMinute;
    USHORT end = Rule->EndMinute;

    if (start < end)
    {
        for (USHORT m = start; m < end; m++)
            Axis[m] = Rule->Type;
    }
    else
    {
        for (USHORT m = start; m < ET_TARIFF_MINUTES_PER_DAY; m++)
            Axis[m] = Rule->Type;

        for (USHORT m = 0; m < end; m++)
            Axis[m] = Rule->Type;
    }
}

// 内置省份峰谷模板（典型参考值，以当地公告为准），"自定义"为哨兵。
// 各省按季节执行不同时段：RulesText=春秋（默认），SummerRulesText=夏季（7、8 月），WinterRulesText=冬季（1、12 月）
typedef struct _ET_POWER_GRID_TARIFF_TEMPLATE
{
    PCWSTR Name;
    PCWSTR RulesText;
    PCWSTR SummerRulesText;
    PCWSTR WinterRulesText;
} ET_POWER_GRID_TARIFF_TEMPLATE;

static const ET_POWER_GRID_TARIFF_TEMPLATE EtpTariffTemplates[] =
{
    // 依据各省发改委分时电价文件整理；尖峰多执行于夏/冬主力窗口，平段=其余时间
    { L"通用两峰", L"1:23:00-7:00;3:7:00-10:00;3:18:00-21:00", NULL, NULL },
    { L"北京", L"1:23:00-7:00;3:10:00-13:00;3:17:00-22:00",
        L"1:23:00-7:00;3:10:00-13:00;3:17:00-22:00;4:11:00-13:00;4:16:00-17:00",
        L"1:23:00-7:00;3:10:00-13:00;3:17:00-22:00;4:18:00-21:00" },
    { L"上海", L"1:22:00-6:00;3:8:00-11:00;3:18:00-21:00",
        L"1:22:00-6:00;3:8:00-15:00;3:18:00-21:00;4:12:00-14:00",
        L"1:22:00-6:00;3:8:00-11:00;3:18:00-21:00;4:19:00-21:00" },
    { L"广东", L"1:0:00-8:00;3:10:00-12:00;3:14:00-19:00",
        L"1:0:00-8:00;3:10:00-12:00;3:14:00-19:00;4:11:00-12:00;4:15:00-17:00", NULL },
    { L"江苏", L"1:2:00-6:00;1:10:00-14:00;3:15:00-22:00",
        L"1:0:00-6:00;1:11:00-13:00;3:14:00-22:00;4:14:00-15:00;4:19:30-21:30",
        L"1:0:00-6:00;1:11:00-13:00;3:14:00-22:00;4:18:00-20:00" },
    { L"浙江", L"1:0:00-7:00;1:11:00-14:00;3:16:00-23:00",
        L"1:0:00-7:00;1:11:00-14:00;3:16:00-18:00;3:22:00-23:00;4:18:00-22:00",
        L"1:0:00-7:00;1:11:00-14:00;3:16:00-18:00;3:22:00-23:00;4:18:00-22:00" },
    { L"山东", L"1:9:00-15:00;3:16:00-22:00;4:17:00-20:00",
        L"1:1:00-6:00;3:16:00-23:00;4:17:00-22:00",
        L"1:2:00-6:00;1:10:00-15:00;3:7:00-9:00;3:16:00-21:00;4:16:00-19:00" },
    { L"河南", L"1:0:00-6:00;1:11:00-14:00;3:16:00-24:00",
        L"1:0:00-7:00;3:16:00-24:00;4:20:00-23:00",
        L"1:0:00-7:00;3:16:00-24:00;4:17:00-19:00" },
    { L"湖北", L"1:0:00-6:00;1:12:00-14:00;3:16:00-18:00;3:20:00-24:00;4:18:00-20:00",
        L"1:0:00-6:00;1:12:00-14:00;3:16:00-20:00;3:22:00-24:00;4:20:00-22:00", NULL },
    { L"湖南", L"1:0:00-6:00;1:12:00-14:00;3:16:00-24:00",
        L"1:0:00-6:00;1:12:00-14:00;3:16:00-24:00;4:20:00-24:00",
        L"1:0:00-6:00;1:12:00-14:00;3:16:00-24:00;4:18:00-22:00" },
    { L"四川", L"1:22:00-8:00;3:10:00-12:00;3:17:00-22:00",
        L"1:1:00-7:00;3:11:00-18:00;3:20:00-23:00;4:13:00-14:00;4:21:00-23:00",
        L"1:0:00-8:00;3:10:00-12:00;3:16:00-22:00" },
    { L"陕西", L"1:0:00-6:00;1:11:00-14:00;3:16:00-23:00",
        L"1:0:00-6:00;1:11:00-14:00;3:16:00-23:00;4:19:00-21:00",
        L"1:0:00-6:00;1:11:00-14:00;3:16:00-23:00;4:18:00-20:00" },
    { L"辽宁", L"1:22:00-5:00;1:11:30-12:30;3:7:30-10:30;3:16:00-21:00",
        L"1:22:00-5:00;1:11:30-12:30;3:7:30-10:30;3:16:00-21:00;4:17:00-19:00",
        L"1:22:00-5:00;1:11:30-12:30;3:7:30-10:30;3:16:00-21:00;4:17:00-19:00" },
    { L"新疆", L"1:4:00-8:00;1:13:00-17:00;3:8:00-11:00;3:19:00-24:00",
        L"1:4:00-8:00;1:13:00-17:00;3:8:00-11:00;3:19:00-24:00;4:21:00-23:00",
        L"1:4:00-8:00;1:13:00-17:00;3:8:00-11:00;3:19:00-24:00;4:19:00-21:00" },
    { L"自定义", NULL, NULL, NULL },
};

#define ET_TARIFF_TEMPLATE_CUSTOM_INDEX (RTL_NUMBER_OF(EtpTariffTemplates) - 1)

// 当前季节：0=春秋（默认）1=夏季（7、8 月）2=冬季（1、12 月）
static ULONG EtpGetTariffSeason(
    VOID
    )
{
    LARGE_INTEGER now;
    SYSTEMTIME localNow;

    PhQuerySystemTime(&now);
    PhLargeIntegerToLocalSystemTime(&localNow, &now);

    if (localNow.wMonth == 7 || localNow.wMonth == 8)
        return 1;

    if (localNow.wMonth == 1 || localNow.wMonth == 12)
        return 2;

    return 0;
}

static PCWSTR EtpGetTariffSeasonName(
    _In_ ULONG Season
    )
{
    switch (Season)
    {
    case 1: return L"夏季（7-8 月）";
    case 2: return L"冬季（1、12 月）";
    default: return L"春秋季";
    }
}

// 由模板/自定义配置构建 1440 分钟轴；后写覆盖先写，天然处理排序/重叠
static BOOLEAN EtpBuildTariffAxis(
    _In_ PPOWER_GRID_WINDOW_CONTEXT Context,
    _Out_writes_(ET_TARIFF_MINUTES_PER_DAY) PBYTE Axis,
    _Out_opt_ PBOOLEAN Fallback
    )
{
    PCWSTR rulesText = NULL;
    BOOLEAN fallback = FALSE;
    BOOLEAN parsed = FALSE;
    PPH_LIST rules;

    memset(Axis, ET_TARIFF_FLAT, ET_TARIFF_MINUTES_PER_DAY);

    if (!Context->TariffTemplateName)
    {
        rulesText = EtpTariffTemplates[0].RulesText;
    }
    else
    {
        for (ULONG i = 0; i < RTL_NUMBER_OF(EtpTariffTemplates); i++)
        {
            if (PhEqualStringZ(PhGetStringOrEmpty(Context->TariffTemplateName), EtpTariffTemplates[i].Name, FALSE))
            {
                const ET_POWER_GRID_TARIFF_TEMPLATE* tariffTemplate = &EtpTariffTemplates[i];
                ULONG season = EtpGetTariffSeason();

                // 按当前月份选择该省对应季节的时段版本
                if (season == 1 && tariffTemplate->SummerRulesText)
                    rulesText = tariffTemplate->SummerRulesText;
                else if (season == 2 && tariffTemplate->WinterRulesText)
                    rulesText = tariffTemplate->WinterRulesText;
                else
                    rulesText = tariffTemplate->RulesText;

                break;
            }
        }
    }

    rules = PhCreateList(8);

    if (rulesText)
    {
        parsed = EtpParseTariffConfig(rulesText, rules);
    }
    else
    {
        // 自定义模板
        PPH_STRING custom = PhGetStringSetting(SETTING_NAME_POWER_GRID_TARIFF_CUSTOM);

        if (custom && custom->Length != 0)
            parsed = EtpParseTariffConfig(PhGetStringOrEmpty(custom), rules);

        PhClearReference(&custom);
    }

    if (!parsed)
    {
        // 配置无效，回退通用两峰
        EtpClearTariffRules(rules);
        parsed = EtpParseTariffConfig(EtpTariffTemplates[0].RulesText, rules);
        fallback = TRUE;
    }

    for (ULONG i = 0; i < rules->Count; i++)
    {
        EtpApplyTariffRule(Axis, rules->Items[i]);
    }

    EtpFreeTariffRules(rules);

    if (Fallback)
        *Fallback = fallback;

    return TRUE;
}

// 由分钟轴生成自当前段起点起 24 小时的峰谷条目
static VOID EtpBuildTariffEntries(
    _In_ PPOWER_GRID_WINDOW_CONTEXT Context,
    _In_ PPH_LIST List,
    _Out_ ET_POWER_GRID_SUMMARY* Summary
    )
{
    BYTE axis[ET_TARIFF_MINUTES_PER_DAY];
    BOOLEAN fallback = FALSE;
    LARGE_INTEGER nowSystemTime;
    __x_ABI_CWindows_CFoundation_CDateTime nowDateTime = { 0 };
    SYSTEMTIME localNow;
    SYSTEMTIME utcMidnight;
    UINT64 nowTicks;
    UINT64 dayStartTicks;
    UINT64 tableEndTicks;
    ULONG nowMinute;
    ULONG segStart;
    ULONG segEnd;
    ULONG x;
    ULONG startX;
    LARGE_INTEGER dayStartLarge;

    memset(Summary, 0, sizeof(ET_POWER_GRID_SUMMARY));
    Summary->TariffValid = TRUE;

    PhQuerySystemTime(&nowSystemTime);
    nowTicks = (UINT64)nowSystemTime.QuadPart;
    nowDateTime.UniversalTime = nowSystemTime.QuadPart;

    PhLargeIntegerToLocalSystemTime(&localNow, &nowSystemTime);

    nowMinute = localNow.wHour * 60 + localNow.wMinute;

    // 本地今天 0 点对应的 UTC ticks（中国无夏令时，安全）
    localNow.wHour = 0;
    localNow.wMinute = 0;
    localNow.wSecond = 0;
    localNow.wMilliseconds = 0;

    // 本地今天 0 点对应的 UTC ticks（SystemTimeToFileTime 把输入当作 UTC、不做时区转换，
    // 会导致基准偏移时区小时数；必须先经 TzSpecificLocalTimeToSystemTime 转为 UTC）
    if (!TzSpecificLocalTimeToSystemTime(NULL, &localNow, &utcMidnight) ||
        !PhSystemTimeToLargeInteger(&dayStartLarge, &utcMidnight))
    {
        Summary->TariffValid = FALSE;
        return;
    }

    dayStartTicks = (UINT64)dayStartLarge.QuadPart;
    tableEndTicks = dayStartTicks + (UINT64)ET_TARIFF_MINUTES_PER_DAY * PH_TICKS_PER_MIN;
    Summary->TariffTableEnd = tableEndTicks;

    EtpBuildTariffAxis(Context, axis, &fallback);
    Summary->TariffFallback = fallback;

    segStart = nowMinute;
    while (segStart > 0 && axis[segStart - 1] == axis[nowMinute])
        segStart--;

    segEnd = nowMinute;
    while (segEnd < ET_TARIFF_MINUTES_PER_DAY && axis[segEnd] == axis[nowMinute])
        segEnd++;

    Summary->TariffCurrentType = axis[nowMinute];
    Summary->TariffCurrentStart.UniversalTime = (INT64)(dayStartTicks + (UINT64)segStart * PH_TICKS_PER_MIN);
    Summary->TariffCurrentEnd.UniversalTime = (INT64)(dayStartTicks + (UINT64)segEnd * PH_TICKS_PER_MIN);

    // 从当前时刻起环形搜索下一低谷/下一高峰的段起点
    for (ULONG i = 1; i <= ET_TARIFF_MINUTES_PER_DAY; i++)
    {
        ULONG m = (nowMinute + i) % ET_TARIFF_MINUTES_PER_DAY;
        UCHAR type = axis[m];
        UCHAR previous = axis[(m + ET_TARIFF_MINUTES_PER_DAY - 1) % ET_TARIFF_MINUTES_PER_DAY];
        UINT64 startTicks;

        if (type == previous)
            continue;

        if (m > nowMinute)
            startTicks = dayStartTicks + (UINT64)m * PH_TICKS_PER_MIN;
        else
            startTicks = tableEndTicks + (UINT64)m * PH_TICKS_PER_MIN;

        if (!Summary->TariffHasValley && type == ET_TARIFF_VALLEY)
        {
            Summary->TariffHasValley = TRUE;
            Summary->TariffNextValleyStart.UniversalTime = (INT64)startTicks;
        }

        if (!Summary->TariffHasPeak && type == ET_TARIFF_PEAK)
        {
            Summary->TariffHasPeak = TRUE;
            Summary->TariffNextPeakStart.UniversalTime = (INT64)startTicks;
        }

        if (Summary->TariffHasValley && Summary->TariffHasPeak)
            break;
    }

    // 从包含当前时刻的连续段起点起生成 24 小时条目
    startX = segStart;
    x = startX;

    while (x < startX + ET_TARIFF_MINUTES_PER_DAY)
    {
        UCHAR type = axis[x % ET_TARIFF_MINUTES_PER_DAY];
        ULONG segEndX = x;
        UINT64 startTicks;
        UINT64 endTicks;
        PET_POWER_GRID_ENTRY entry;
        __x_ABI_CWindows_CFoundation_CTimeSpan durationSpan;
        WCHAR timeBuffer[64];
        WCHAR durationBuffer[PH_TIMESPAN_STR_LEN_1];

        while (segEndX + 1 < startX + ET_TARIFF_MINUTES_PER_DAY &&
            axis[(segEndX + 1) % ET_TARIFF_MINUTES_PER_DAY] == type)
        {
            segEndX++;
        }

        startTicks = dayStartTicks + (UINT64)x * PH_TICKS_PER_MIN;
        endTicks = dayStartTicks + (UINT64)(segEndX + 1) * PH_TICKS_PER_MIN;

        if (startTicks >= endTicks)
            break;

        entry = PhAllocateZero(sizeof(ET_POWER_GRID_ENTRY));
        entry->TariffType = type;
        entry->Severity = PhCreateString(EtpGetTariffTypeName(type));
        entry->BlockStartTime.UniversalTime = (INT64)startTicks;
        entry->BlockEndTime.UniversalTime = (INT64)endTicks;
        entry->ActiveFlag = startTicks <= nowTicks && nowTicks < endTicks;

        // 跨过今天 24:00 的条目标注"次日"，避免与今天时段混淆
        EtpFormatPowerGridTimeOnly(entry->BlockStartTime, timeBuffer); // 列1：开始时间（仅时间）
        entry->LowImpact = startTicks >= tableEndTicks
            ? PhConcatStrings2(L"次日 ", timeBuffer)
            : PhCreateString(timeBuffer);

        EtpFormatPowerGridTimeOnly(entry->BlockEndTime, timeBuffer); // 列2：结束时间（仅时间）
        entry->TimeUntilStart = endTicks >= tableEndTicks
            ? PhConcatStrings2(L"次日 ", timeBuffer)
            : PhCreateString(timeBuffer);

        durationSpan.Duration = (INT64)(endTicks - startTicks);
        EtpFormatPowerGridDuration(durationSpan, durationBuffer);
        entry->StartTime = PhCreateString(durationBuffer); // 列3：时段长度（时:分:秒）

        entry->BlockDuration = EtpFormatRelativeTimeString(entry->BlockStartTime, nowDateTime); // 列4：距开始

        PhAddItemList(List, entry);

        x = segEndX + 1;
    }
}

VOID EtAddTariffSummaryText(
    _In_ HWND SummaryHandle,
    _In_ const ET_POWER_GRID_SUMMARY* Summary,
    _In_opt_ PPH_STRING TemplateName
    )
{
    PH_STRING_BUILDER sb;
    LARGE_INTEGER nowSystemTime;
    __x_ABI_CWindows_CFoundation_CDateTime nowDateTime = { 0 };
    WCHAR startBuffer[64];
    WCHAR endBuffer[64];

    PhQuerySystemTime(&nowSystemTime);
    nowDateTime.UniversalTime = nowSystemTime.QuadPart;

    PhInitializeStringBuilder(&sb, 512);

    if (Summary->TariffAutoSwitched)
        PhAppendStringBuilder2(&sb, L"微软电网预测数据不可用，已自动切换至峰谷电价模式。\r\n");

    if (Summary->TariffFallback)
        PhAppendStringBuilder2(&sb, L"自定义时段配置无效，已回退通用模板。\r\n");

    // 内置模板显示季节版本（自定义配置无季节概念）
    if (PhEqualStringZ(PhGetStringOrEmpty(TemplateName), L"自定义", FALSE))
    {
        PhAppendFormatStringBuilder(&sb, L"时段模板：%s\r\n", PhGetStringOrEmpty(TemplateName));
    }
    else
    {
        PhAppendFormatStringBuilder(&sb, L"时段模板：%s（%s）\r\n",
            PhGetStringOrEmpty(TemplateName), EtpGetTariffSeasonName(EtpGetTariffSeason()));
    }

    if (Summary->TariffValid && Summary->TariffCurrentType)
    {
        EtpFormatPowerGridTimeOnly(Summary->TariffCurrentStart, startBuffer);
        EtpFormatPowerGridTimeOnly(Summary->TariffCurrentEnd, endBuffer);
        PhAppendFormatStringBuilder(&sb, L"当前时段：%s（%s 至 %s）\r\n",
            EtpGetTariffTypeName(Summary->TariffCurrentType), startBuffer, endBuffer);
    }

    if (Summary->TariffHasValley)
    {
        PPH_STRING relative = EtpFormatRelativeTimeString(Summary->TariffNextValleyStart, nowDateTime);
        EtpFormatPowerGridDateTime(Summary->TariffNextValleyStart, startBuffer, NULL);
        PhAppendFormatStringBuilder(&sb, L"下一低谷：%s（%s）\r\n", startBuffer, PhGetStringOrEmpty(relative));
        PhDereferenceObject(relative);
    }
    else
    {
        PhAppendStringBuilder2(&sb, L"下一低谷：无\r\n");
    }

    if (Summary->TariffHasPeak)
    {
        PPH_STRING relative = EtpFormatRelativeTimeString(Summary->TariffNextPeakStart, nowDateTime);
        EtpFormatPowerGridDateTime(Summary->TariffNextPeakStart, startBuffer, NULL);
        PhAppendFormatStringBuilder(&sb, L"下一高峰：%s（%s）\r\n", startBuffer, PhGetStringOrEmpty(relative));
        PhDereferenceObject(relative);
    }
    else
    {
        PhAppendStringBuilder2(&sb, L"下一高峰：无\r\n");
    }

    PhAppendStringBuilder2(&sb, L"时段设置为典型参考值，请以当地公告为准。");

    PhSetWindowText(SummaryHandle, PhFinalStringBuilderString(&sb)->Buffer);
    PhDeleteStringBuilder(&sb);
}

// 强制列最小宽度：旧配置保存的窄列宽会被纠正
static VOID EtpEnsureColumnMinWidth(
    _In_ HWND ListViewHandle,
    _In_ INT Index,
    _In_ INT MinWidth
    )
{
    if (ListView_GetColumnWidth(ListViewHandle, Index) < MinWidth)
        ListView_SetColumnWidth(ListViewHandle, Index, MinWidth);
}

// 按显示模式重建列表列：先删光列再创建，两模式使用各自列配置键
static VOID EtpSetupListViewColumns(
    _In_ HWND ListViewHandle,
    _In_ UCHAR EffectiveMode
    )
{
    while (ListView_DeleteColumn(ListViewHandle, 0))
        ;

    if (EffectiveMode == ET_POWER_GRID_MODE_TARIFF)
    {
        PhAddListViewColumn(ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 160, L"时段类型");
        PhAddListViewColumn(ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 240, L"开始时间");
        PhAddListViewColumn(ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 240, L"结束时间");
        PhAddListViewColumn(ListViewHandle, 3, 3, 3, LVCFMT_LEFT, 110, L"时段长度");
        PhAddListViewColumn(ListViewHandle, 4, 4, 4, LVCFMT_LEFT, 140, L"距开始");

        PhLoadListViewColumnsFromSetting(SETTING_NAME_POWER_GRID_TARIFF_LISTVIEW_COLUMNS, ListViewHandle);

        // 旧配置保存的列宽偏窄，容纳"次日 X:00:00"需保证最小宽度
        EtpEnsureColumnMinWidth(ListViewHandle, 1, 240);
        EtpEnsureColumnMinWidth(ListViewHandle, 2, 240);
    }
    else
    {
        PhAddListViewColumn(ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 240, L"严重程度");
        PhAddListViewColumn(ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 180, L"低影响");
        PhAddListViewColumn(ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 240, L"开始时间");
        PhAddListViewColumn(ListViewHandle, 3, 3, 3, LVCFMT_LEFT, 150, L"时段长度");
        PhAddListViewColumn(ListViewHandle, 4, 4, 4, LVCFMT_LEFT, 160, L"距开始");

        PhLoadListViewColumnsFromSetting(SETTING_NAME_POWER_GRID_LISTVIEW_COLUMNS, ListViewHandle);

        EtpEnsureColumnMinWidth(ListViewHandle, 0, 240);
        EtpEnsureColumnMinWidth(ListViewHandle, 1, 180);
        EtpEnsureColumnMinWidth(ListViewHandle, 2, 240);
        EtpEnsureColumnMinWidth(ListViewHandle, 3, 150);
        EtpEnsureColumnMinWidth(ListViewHandle, 4, 160);
    }
}

static BOOLEAN EtpHasTariffCustomConfig(
    VOID
    )
{
    PPH_STRING custom = PhGetStringSetting(SETTING_NAME_POWER_GRID_TARIFF_CUSTOM);
    BOOLEAN has = custom && custom->Length != 0;
    PhClearReference(&custom);
    return has;
}

static VOID EtpInitTariffCombo(
    _In_ PPOWER_GRID_WINDOW_CONTEXT Context
    )
{
    PPH_STRING saved;
    LONG index;

    ComboBox_ResetContent(Context->ProvinceComboHandle);

    for (ULONG i = 0; i < RTL_NUMBER_OF(EtpTariffTemplates); i++)
    {
        ComboBox_AddString(Context->ProvinceComboHandle, EtpTariffTemplates[i].Name);
    }

    saved = PhGetStringSetting(SETTING_NAME_POWER_GRID_TARIFF_TEMPLATE);
    index = ComboBox_FindStringExact(Context->ProvinceComboHandle, -1, PhGetStringOrEmpty(saved));
    PhDereferenceObject(saved);

    if (index == CB_ERR)
        index = 0;

    ComboBox_SetCurSel(Context->ProvinceComboHandle, index);
    PhMoveReference(&Context->TariffTemplateName, PhGetComboBoxString(Context->ProvinceComboHandle, (ULONG)index));
}

static VOID EtpRebuildTariffEntries(
    _In_ PPOWER_GRID_WINDOW_CONTEXT Context,
    _In_ BOOLEAN ShowAutoSwitched
    )
{
    ET_POWER_GRID_SUMMARY summary;
    PPH_LIST list;

    if (Context->CurrentEntries)
    {
        EtpFreePowerGridList(Context->CurrentEntries);
        Context->CurrentEntries = NULL;
    }

    list = PhCreateList(24);
    EtpBuildTariffEntries(Context, list, &summary);

    Context->CurrentEntries = list;
    Context->TariffSummary = summary;
    Context->TariffTableEndTicks = summary.TariffTableEnd;

    ExtendedListView_SetRedraw(Context->ListViewHandle, FALSE);
    EtpClearPowerGridListView(Context->ListViewHandle);

    for (ULONG i = 0; i < list->Count; i++)
    {
        PET_POWER_GRID_ENTRY entry = list->Items[i];
        LONG lvItemIndex;

        lvItemIndex = PhAddListViewItem(
            Context->ListViewHandle,
            MAXINT,
            PhGetString(entry->Severity),
            entry
            );

        PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 1, PhGetString(entry->LowImpact));
        PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 2, PhGetString(entry->TimeUntilStart));
        PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 3, PhGetString(entry->StartTime));
        PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 4, PhGetString(entry->BlockDuration));
    }

    ExtendedListView_SetRedraw(Context->ListViewHandle, TRUE);

    summary.TariffAutoSwitched = ShowAutoSwitched;
    Context->TariffSummary = summary;

    EtAddTariffSummaryText(Context->SummaryHandle, &summary, Context->TariffTemplateName);
}

// 峰谷专属控件与预测专用刷新按钮的统一显隐（TariffVisible = 是否处于峰谷渲染）
static VOID EtpShowModeControls(
    _In_ PPOWER_GRID_WINDOW_CONTEXT Context,
    _In_ BOOLEAN TariffVisible
    )
{
    ShowWindow(Context->ProvinceComboHandle, TariffVisible ? SW_SHOW : SW_HIDE);
    ShowWindow(Context->TariffCfgButtonHandle, TariffVisible ? SW_SHOW : SW_HIDE);
    // 刷新仅对微软电网预测有意义（峰谷为本地计算，定时器已自动刷新）
    ShowWindow(Context->RefreshButtonHandle, TariffVisible ? SW_HIDE : SW_SHOW);
}

VOID EtpApplyModeToUi(
    _In_ PPOWER_GRID_WINDOW_CONTEXT Context
    )
{
    static PCWSTR modeCaptions[] =
    {
        L"模式：自动",
        L"模式：电网预测",
        L"模式：峰谷电价",
    };

    if (Context->ModeSetting > ET_POWER_GRID_MODE_TARIFF)
        Context->ModeSetting = ET_POWER_GRID_MODE_AUTO;

    SetWindowText(Context->ModeButtonHandle, modeCaptions[Context->ModeSetting]);

    EtpShowModeControls(Context, Context->ModeSetting == ET_POWER_GRID_MODE_TARIFF);

    if (Context->ModeSetting == ET_POWER_GRID_MODE_TARIFF)
    {
        Context->EffectiveMode = ET_POWER_GRID_MODE_TARIFF;
        EtpSetupListViewColumns(Context->ListViewHandle, ET_POWER_GRID_MODE_TARIFF);
        EtpRebuildTariffEntries(Context, FALSE);
    }
    else
    {
        // 显式选择电网预测时不回落峰谷；仅自动模式等待数据判定
        Context->EffectiveMode = Context->ModeSetting == ET_POWER_GRID_MODE_FORECAST
            ? ET_POWER_GRID_MODE_FORECAST
            : ET_POWER_GRID_MODE_PENDING;
        EtpSetupListViewColumns(Context->ListViewHandle, ET_POWER_GRID_MODE_FORECAST);
        EtpClearPowerGridListView(Context->ListViewHandle);
        SetWindowText(Context->SummaryHandle, L"正在获取电网预测数据...");
        PhCreateThread2(EtpEnumeratePowerGridForecast, Context->WindowHandle);
    }
}

INT_PTR CALLBACK EtpTariffConfigDlgProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    switch (WindowMessage)
    {
    case WM_INITDIALOG:
        {
            PPH_STRING custom;

            custom = PhGetStringSetting(SETTING_NAME_POWER_GRID_TARIFF_CUSTOM);
            PhSetDialogItemText(WindowHandle, IDC_POWER_GRID_TARIFF_EDIT, PhGetStringOrEmpty(custom));
            PhDereferenceObject(custom);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDOK:
                {
                    PPH_STRING text;
                    PPH_LIST rules;

                    text = PhaGetDlgItemText(WindowHandle, IDC_POWER_GRID_TARIFF_EDIT);
                    rules = PhCreateList(2);

                    if (EtpParseTariffConfig(PhGetStringOrEmpty(text), rules))
                    {
                        EtpFreeTariffRules(rules);
                        PhSetStringSetting(SETTING_NAME_POWER_GRID_TARIFF_CUSTOM, PhGetStringOrEmpty(text));
                        EndDialog(WindowHandle, IDOK);
                    }
                    else
                    {
                        EtpFreeTariffRules(rules);
                        PhShowMessage(WindowHandle, MB_OK | MB_ICONWARNING,
                            L"时段配置格式无效。\n"
                            L"格式：类型:起-止;...（1=低谷 2=平段 3=高峰 4=尖峰）\n"
                            L"例如：1:23:00-7:00;3:7:00-10:00;4:10:00-12:00");
                    }
                }
                return TRUE;
            case IDCANCEL:
                EndDialog(WindowHandle, IDCANCEL);
                return TRUE;
            }
        }
        break;
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORSTATIC:
        return (INT_PTR)PhWindowThemeControlColor(WindowHandle, (HDC)wParam, (HWND)lParam,
            WindowMessage == WM_CTLCOLORBTN ? CTLCOLOR_BTN : CTLCOLOR_STATIC);
    }

    return FALSE;
}

VOID EtpPublishPowerGridList(
    _In_ HWND ListViewHandle,
    _In_ HWND SummaryHandle,
    _In_opt_ PPH_LIST List,
    _In_opt_ const ET_POWER_GRID_SUMMARY* Summary
    )
{
    ExtendedListView_SetRedraw(ListViewHandle, FALSE);
    EtpClearPowerGridListView(ListViewHandle);

    if (List)
    {
        for (ULONG i = 0; i < List->Count; i++)
        {
            PET_POWER_GRID_ENTRY entry = List->Items[i];
            LONG lvItemIndex;

            lvItemIndex = PhAddListViewItem(
                ListViewHandle,
                MAXINT,
                PhGetString(entry->Severity),
                entry
                );

            PhSetListViewSubItem(ListViewHandle, lvItemIndex, 1, PhGetString(entry->LowImpact));
            PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, PhGetString(entry->StartTime));
            PhSetListViewSubItem(ListViewHandle, lvItemIndex, 3, PhGetString(entry->BlockDuration));
            PhSetListViewSubItem(ListViewHandle, lvItemIndex, 4, PhGetString(entry->TimeUntilStart));
        }
    }

    EtAddSummaryWindowText(SummaryHandle, Summary);

    ExtendedListView_SetRedraw(ListViewHandle, TRUE);
}

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS NTAPI EtpEnumeratePowerGridForecast(
    _In_ PVOID ThreadParameter
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static typeof(&RoInitialize) RoInitialize_I = NULL;
    static typeof(&RoUninitialize) RoUninitialize_I = NULL;
    static typeof(&CoUninitialize) CoUninitialize_I = NULL;
    HWND parentWindow = (HWND)ThreadParameter;
    HRESULT status;
    __FIVectorView_1_Windows__CDevices__CPower__CPowerGridData* powerGridDataVector = NULL;
    __x_ABI_CWindows_CDevices_CPower_CIPowerGridForecastStatics* powerGridForecastStatics = NULL;
    __x_ABI_CWindows_CDevices_CPower_CIPowerGridForecast* powerGridForecast = NULL;
    __x_ABI_CWindows_CFoundation_CDateTime startTime = { 0 };
    __x_ABI_CWindows_CFoundation_CTimeSpan blockDuration = { 0 };
    PPH_LIST powerGridList = NULL;
    UINT32 count = 0;
    WCHAR blockDurationBuffer[PH_TIMESPAN_STR_LEN_1];
    ET_POWER_GRID_SUMMARY summary = { 0 };
    HSTRING runtimeClassString = NULL;
    __x_ABI_CWindows_CFoundation_CDateTime nowTime = { 0 };
    LARGE_INTEGER nowSystemTime = { 0 };
    LARGE_INTEGER nowLarge = { 0 };

    if (PhBeginInitOnce(&initOnce))
    {
        PVOID baseAddress;

        if (baseAddress = PhLoadLibrary(L"combase.dll"))
        {
            RoInitialize_I = PhGetProcedureAddress(baseAddress, "RoInitialize", 0);
            RoUninitialize_I = PhGetProcedureAddress(baseAddress, "RoUninitialize", 0);
            CoUninitialize_I = PhGetProcedureAddress(baseAddress, "CoUninitialize", 0);
        }

        PhEndInitOnce(&initOnce);
    }

    if (!(RoInitialize_I && RoUninitialize_I && CoUninitialize_I))
        return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);

    {
        CoUninitialize_I();

        status = RoInitialize_I(RO_INIT_MULTITHREADED);

        if (HR_FAILED(status))
            return status;
    }

    status = PhGetActivationFactory(
        L"Windows.Energy.dll",
        RuntimeClass_Windows_Devices_Power_PowerGridForecast,
        &IID_IPowerGridForecastStatics,
        &powerGridForecastStatics
        );

    if (HR_FAILED(status))
        goto CleanupExit;

    status = __x_ABI_CWindows_CDevices_CPower_CIPowerGridForecastStatics_GetForecast(
        powerGridForecastStatics,
        &powerGridForecast
        );

    if (HR_FAILED(status))
        goto CleanupExit;

    status = __x_ABI_CWindows_CDevices_CPower_CIPowerGridForecast_get_StartTime(powerGridForecast, &startTime);

    if (HR_FAILED(status))
        goto CleanupExit;

    status = __x_ABI_CWindows_CDevices_CPower_CIPowerGridForecast_get_BlockDuration(powerGridForecast, &blockDuration);

    if (HR_FAILED(status))
        goto CleanupExit;

    status = __x_ABI_CWindows_CDevices_CPower_CIPowerGridForecast_get_Forecast(
        powerGridForecast,
        &powerGridDataVector
        );

    if (HR_FAILED(status))
        goto CleanupExit;

    status = __FIVectorView_1_Windows__CDevices__CPower__CPowerGridData_get_Size(
        powerGridDataVector,
        &count
        );

    if (HR_FAILED(status))
        goto CleanupExit;

    powerGridList = PhCreateList(count);
    EtpFormatPowerGridDuration(blockDuration, blockDurationBuffer);

    summary.ForecastStart = startTime;
    summary.BlockDuration = blockDuration;
    summary.TotalBlocks = count;
    summary.BestLowSeverity = DBL_MAX;
    summary.LowestSeverity = DBL_MAX;
    summary.HighestSeverity = -DBL_MAX;

    PhQuerySystemTime(&nowSystemTime);
    nowTime.UniversalTime = nowSystemTime.QuadPart;

    for (UINT32 i = 0; i < count; i++)
    {
        __x_ABI_CWindows_CDevices_CPower_CIPowerGridData* powerGridData = NULL;
        __x_ABI_CWindows_CFoundation_CDateTime blockStart = startTime;
        PET_POWER_GRID_ENTRY entry;
        DOUBLE severity = 0;
        boolean isLowUserExperienceImpact = FALSE;
        WCHAR blockStartBuffer[PH_DATETIME_STR_LEN_1];

        status = __FIVectorView_1_Windows__CDevices__CPower__CPowerGridData_GetAt(
            powerGridDataVector,
            i,
            &powerGridData
            );

        if (HR_FAILED(status))
            continue;

        if (HR_FAILED(__x_ABI_CWindows_CDevices_CPower_CIPowerGridData_get_Severity(powerGridData, &severity)))
        {
            severity = 0;
        }

        if (HR_FAILED(__x_ABI_CWindows_CDevices_CPower_CIPowerGridData_get_IsLowUserExperienceImpact(powerGridData, &isLowUserExperienceImpact)))
        {
            isLowUserExperienceImpact = FALSE;
        }

        blockStart.UniversalTime += (INT64)i * blockDuration.Duration;

        entry = PhAllocateZero(sizeof(ET_POWER_GRID_ENTRY));
        entry->Severity = PhFormatString(L"%.2f", severity);
        entry->LowImpact = PhCreateString(isLowUserExperienceImpact ? L"是" : L"否");
        EtpFormatPowerGridDateTime(blockStart, blockStartBuffer, NULL);
        entry->StartTime = PhCreateString(blockStartBuffer);
        entry->BlockDuration = PhCreateString(blockDurationBuffer);
        entry->TimeUntilStart = EtpFormatRelativeTimeString(blockStart, nowTime);
        entry->SeverityValue = severity;
        entry->LowImpactFlag = isLowUserExperienceImpact;
        entry->BlockStartTime = blockStart;
        entry->BlockEndTime = EtpAddTicks(blockStart, blockDuration.Duration);
        entry->ActiveFlag = EtpIsPowerGridEntryActive(entry->BlockStartTime, entry->BlockEndTime, nowTime);
        PhAddItemList(powerGridList, entry);

        if (isLowUserExperienceImpact)
        {
            summary.LowImpactCount++;
        }

        if (severity < summary.BestLowSeverity && isLowUserExperienceImpact)
        {
            summary.BestLowSeverity = severity;
            summary.BestLowStart = blockStart;
        }

        if (severity < summary.LowestSeverity)
        {
            summary.LowestSeverity = severity;
            summary.LowestStart = blockStart;
        }

        if (severity > summary.HighestSeverity)
        {
            summary.HighestSeverity = severity;
            summary.HighestStart = blockStart;
        }

        summary.AverageSeverity += severity;

        __x_ABI_CWindows_CDevices_CPower_CIPowerGridData_Release(powerGridData);
    }

CleanupExit:

    if (powerGridDataVector)
        __FIVectorView_1_Windows__CDevices__CPower__CPowerGridData_Release(powerGridDataVector);
    if (powerGridForecast)
        __x_ABI_CWindows_CDevices_CPower_CIPowerGridForecast_Release(powerGridForecast);
    if (powerGridForecastStatics)
        __x_ABI_CWindows_CDevices_CPower_CIPowerGridForecastStatics_Release(powerGridForecastStatics);

    summary.AverageSeverity = summary.TotalBlocks ? summary.AverageSeverity / summary.TotalBlocks : 0.0;

    {
        PET_POWER_GRID_UPDATE update;

        update = PhAllocateZero(sizeof(ET_POWER_GRID_UPDATE));
        update->Entries = powerGridList;
        update->Summary = summary;

        if (!PostMessage(parentWindow, ET_WM_POWERGRID_UPDATE, status, (LPARAM)update))
        {
            EtpFreePowerGridList(powerGridList);
            PhFree(update);
        }
    }

    RoUninitialize_I();

    return STATUS_SUCCESS;
}

INT_PTR CALLBACK EtPowerGridDlgProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPOWER_GRID_WINDOW_CONTEXT context;

    if (WindowMessage == WM_INITDIALOG)
    {
        context = (PPOWER_GRID_WINDOW_CONTEXT)lParam;
        PhSetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (context == NULL)
        return FALSE;

    switch (WindowMessage)
    {
    case WM_INITDIALOG:
        {
            context->WindowHandle = WindowHandle;
            context->ListViewHandle = GetDlgItem(WindowHandle, IDC_POWER_GRID_LIST);
            context->SummaryHandle = GetDlgItem(WindowHandle, IDC_POWER_GRID_SUMMARY);
            context->ModeButtonHandle = GetDlgItem(WindowHandle, IDC_POWER_GRID_MODE);
            context->ProvinceComboHandle = GetDlgItem(WindowHandle, IDC_POWER_GRID_PROVINCE);
            context->TariffCfgButtonHandle = GetDlgItem(WindowHandle, IDC_POWER_GRID_TARIFF_CFG);
            context->RefreshButtonHandle = GetDlgItem(WindowHandle, IDC_REFRESH);
            context->HeaderHandle = ListView_GetHeader(context->ListViewHandle);
            context->WindowFont = PhCreateTreeWindowFont(PhGetWindowDpi(WindowHandle));

            PhSetApplicationWindowIcon(WindowHandle);

            PhSetListViewStyle(context->ListViewHandle, TRUE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhSetExtendedListView(context->ListViewHandle);

            PhInitializeLayoutManager(&context->LayoutManager, WindowHandle);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, context->SummaryHandle, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(WindowHandle, IDOK), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, context->RefreshButtonHandle, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, context->ModeButtonHandle, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, context->ProvinceComboHandle, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, context->TariffCfgButtonHandle, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM);

            SetWindowFont(context->ListViewHandle, context->WindowFont, FALSE);
            SetWindowFont(context->SummaryHandle, context->WindowFont, FALSE);

            EtpInitTariffCombo(context);

            context->ModeSetting = (UCHAR)PhGetIntegerSetting(SETTING_NAME_POWER_GRID_MODE);
            EtpApplyModeToUi(context);

            if (PhValidWindowPlacementFromSetting(SETTING_NAME_POWER_GRID_WINDOW_POSITION))
                PhLoadWindowPlacementFromSetting(SETTING_NAME_POWER_GRID_WINDOW_POSITION, SETTING_NAME_POWER_GRID_WINDOW_SIZE, WindowHandle);
            else
                PhCenterWindow(WindowHandle, context->ParentWindowHandle);

            // 分钟级倒计时无需秒刷，30 秒刷新一次即可
            PhSetTimer(WindowHandle, PH_WINDOW_TIMER_DEFAULT, 30000, NULL);
        }
        break;
    case WM_DESTROY:
        {
            PhRemoveWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);

            PhSaveWindowPlacementToSetting(SETTING_NAME_POWER_GRID_WINDOW_POSITION, SETTING_NAME_POWER_GRID_WINDOW_SIZE, WindowHandle);

            if (context->EffectiveMode == ET_POWER_GRID_MODE_TARIFF)
                PhSaveListViewColumnsToSetting(SETTING_NAME_POWER_GRID_TARIFF_LISTVIEW_COLUMNS, context->ListViewHandle);
            else
                PhSaveListViewColumnsToSetting(SETTING_NAME_POWER_GRID_LISTVIEW_COLUMNS, context->ListViewHandle);

            if (context->CurrentEntries)
            {
                EtpFreePowerGridList(context->CurrentEntries);
                context->CurrentEntries = NULL;
            }

            PhClearReference(&context->TariffTemplateName);

            PhDeleteLayoutManager(&context->LayoutManager);

            if (context->WindowFont) DeleteFont(context->WindowFont);

            PhFree(context);

            PostQuitMessage(0);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);

            InvalidateRect(context->ListViewHandle, NULL, FALSE);
        }
        break;
    case WM_DPICHANGED:
        {
            HFONT windowFont;

            if (windowFont = PhCreateTreeWindowFont(LOWORD(wParam)))
            {
                PhSwapReferenceFont(&context->WindowFont, context->ListViewHandle, windowFont, TRUE);
                SetWindowFont(context->SummaryHandle, context->WindowFont, TRUE);
            }

            PhLayoutManagerUpdate(&context->LayoutManager, LOWORD(wParam));
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_TIMER:
        {
            switch (wParam)
            {
            case PH_WINDOW_TIMER_DEFAULT:
                {
                    __x_ABI_CWindows_CFoundation_CDateTime nowTime = { 0 };
                    LARGE_INTEGER nowSystemTime = { 0 };
                    LONG index = INT_ERROR;

                    // 时间基准循环外取一次即可（分钟精度，无需逐条目查询）
                    PhQuerySystemTime(&nowSystemTime);
                    nowTime.UniversalTime = nowSystemTime.QuadPart;

                    while ((index = PhFindListViewItemByFlags(
                        context->ListViewHandle,
                        index,
                        LVNI_ALL
                        )) != INT_ERROR)
                    {
                        PET_POWER_GRID_ENTRY param;

                        if (PhGetListViewItemParam(context->ListViewHandle, index, &param))
                        {
                            param->ActiveFlag = EtpIsPowerGridEntryActive(param->BlockStartTime, param->BlockEndTime, nowTime);

                            if (param->TariffType != 0)
                            {
                                // 峰谷模式：刷新第 5 列"距开始"
                                if (param->ActiveFlag)
                                {
                                    PhSetListViewSubItem(context->ListViewHandle, index, 4, L"活动中");
                                }
                                else
                                {
                                    PPH_STRING relative;

                                    relative = EtpFormatRelativeTimeString(param->BlockStartTime, nowTime);
                                    PhSetListViewSubItem(context->ListViewHandle, index, 4, PhGetStringOrEmpty(relative));
                                    PhDereferenceObject(relative);
                                }
                            }
                            else
                            {
                                PhMoveReference(&param->TimeUntilStart, EtpFormatRelativeTimeString(param->BlockStartTime, nowTime));

                                if (param->ActiveFlag)
                                {
                                    PhSetListViewSubItem(context->ListViewHandle, index, 4, L"活动中");
                                }
                                else
                                {
                                    PhSetListViewSubItem(context->ListViewHandle, index, 4, PhGetString(param->TimeUntilStart));
                                }
                            }
                        }
                    }

                    if (context->EffectiveMode == ET_POWER_GRID_MODE_TARIFF)
                    {
                        if (context->TariffTableEndTicks && (UINT64)nowSystemTime.QuadPart >= context->TariffTableEndTicks)
                        {
                            // 跨日：重建全天条目
                            EtpRebuildTariffEntries(context, FALSE);
                        }
                        else
                        {
                            EtAddTariffSummaryText(context->SummaryHandle, &context->TariffSummary, context->TariffTemplateName);
                        }
                    }
                }
                break;
            }
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
            case IDOK:
                DestroyWindow(WindowHandle);
                break;
            case IDC_REFRESH:
                {
                    EnableWindow(context->RefreshButtonHandle, FALSE);

                    if (context->EffectiveMode == ET_POWER_GRID_MODE_TARIFF)
                    {
                        EtpRebuildTariffEntries(context, FALSE);
                        EnableWindow(context->RefreshButtonHandle, TRUE);
                    }
                    else
                    {
                        PhCreateThread2(EtpEnumeratePowerGridForecast, WindowHandle);
                    }
                }
                break;
            case IDC_POWER_GRID_MODE:
                {
                    context->ModeSetting = (context->ModeSetting + 1) % (ET_POWER_GRID_MODE_TARIFF + 1);
                    PhSetIntegerSetting(SETTING_NAME_POWER_GRID_MODE, context->ModeSetting);
                    EtpApplyModeToUi(context);
                }
                break;
            case IDC_POWER_GRID_PROVINCE:
                {
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        LONG index = ComboBox_GetCurSel(context->ProvinceComboHandle);

                        if (index != CB_ERR)
                        {
                            PPH_STRING name = PhGetComboBoxString(context->ProvinceComboHandle, (ULONG)index);

                            if (name)
                            {
                                PhSetStringSetting(SETTING_NAME_POWER_GRID_TARIFF_TEMPLATE, PhGetStringOrEmpty(name));
                                PhMoveReference(&context->TariffTemplateName, name);
                            }

                            if ((ULONG)index == ET_TARIFF_TEMPLATE_CUSTOM_INDEX && !EtpHasTariffCustomConfig())
                            {
                                DialogBox(
                                    PluginInstance->DllBase,
                                    MAKEINTRESOURCE(IDD_POWER_GRID_TARIFF),
                                    WindowHandle,
                                    EtpTariffConfigDlgProc
                                    );
                            }

                            if (context->EffectiveMode == ET_POWER_GRID_MODE_TARIFF)
                            {
                                EtpRebuildTariffEntries(context, FALSE);
                            }
                        }
                    }
                }
                break;
            case IDC_POWER_GRID_TARIFF_CFG:
                {
                    if (DialogBox(
                        PluginInstance->DllBase,
                        MAKEINTRESOURCE(IDD_POWER_GRID_TARIFF),
                        WindowHandle,
                        EtpTariffConfigDlgProc
                        ) != IDCANCEL &&
                        context->EffectiveMode == ET_POWER_GRID_MODE_TARIFF)
                    {
                        EtpRebuildTariffEntries(context, FALSE);
                    }
                }
                break;
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            if (header->hwndFrom == context->ListViewHandle)
            {
                switch (header->code)
                {
                case NM_CUSTOMDRAW:
                    {
                        LPNMLVCUSTOMDRAW customDraw = (LPNMLVCUSTOMDRAW)lParam;

                        switch (customDraw->nmcd.dwDrawStage)
                        {
                        case CDDS_PREPAINT:
                            {
                                SetWindowLongPtr(WindowHandle, DWLP_MSGRESULT, CDRF_NOTIFYITEMDRAW);
                                return CDRF_NOTIFYITEMDRAW;
                            }
                            break;
                        case CDDS_ITEMPREPAINT:
                            {
                                PET_POWER_GRID_ENTRY item;
                                RECT rowRect;
                                RECT clientRect;
                                RECT progressRect;
                                double severity;
                                double effectiveSeverity;
                                BYTE red;
                                BYTE green;
                                COLORREF fillColor;
                                HBRUSH fillBrush;

                                if (customDraw->iSubItem > 0)
                                    return CDRF_DODEFAULT;

                                item = (PET_POWER_GRID_ENTRY)customDraw->nmcd.lItemlParam;
                                if (!item)
                                    return CDRF_DODEFAULT;

                                // 取第 0 行第 0 列的标签矩形作为整列着色基准（各行列宽一致）
                                ListView_GetSubItemRect(context->ListViewHandle, 0, 0, LVIR_LABEL, &clientRect);

                                rowRect = customDraw->nmcd.rc;
                                rowRect.left = clientRect.left;
                                rowRect.right = clientRect.right;

                                // 峰谷条目按时段类型整行着色；电网条目按严重程度渐变填充
                                if (item->TariffType != 0)
                                {
                                    switch (item->TariffType)
                                    {
                                    case ET_TARIFF_VALLEY:
                                        fillColor = RGB(80, 180, 80);
                                        break;
                                    case ET_TARIFF_PEAK:
                                        fillColor = RGB(235, 150, 50);
                                        break;
                                    case ET_TARIFF_CRITICAL:
                                        fillColor = RGB(215, 65, 55);
                                        break;
                                    default:
                                        fillColor = RGB(220, 190, 70);
                                        break;
                                    }

                                    fillBrush = CreateSolidBrush(fillColor);
                                    FillRect(customDraw->nmcd.hdc, &rowRect, fillBrush);
                                    DeleteBrush(fillBrush);
                                }
                                else
                                {
                                    severity = item->SeverityValue;
                                    if (severity < 0.0) severity = 0.0;
                                    else if (severity > 1.0) severity = 1.0;

                                    effectiveSeverity = severity;
                                    if (item->LowImpactFlag)
                                        effectiveSeverity *= 0.65;

                                    red = (BYTE)((0.15 + effectiveSeverity * 0.6) * 255.0);
                                    green = (BYTE)((0.35 + (1.0 - effectiveSeverity) * 0.65) * 255.0);

                                    if (item->LowImpactFlag)
                                        green = min(255, green + 20);

                                    progressRect = rowRect;
                                    progressRect.right = progressRect.left + (LONG)((progressRect.right - progressRect.left) * severity);

                                    if (progressRect.right > progressRect.left)
                                    {
                                        fillColor = RGB(red, green, 0);
                                        fillBrush = CreateSolidBrush(fillColor);
                                        FillRect(customDraw->nmcd.hdc, &progressRect, fillBrush);
                                        DeleteBrush(fillBrush);
                                    }
                                }

                                customDraw->clrText = GetSysColor(COLOR_WINDOWTEXT);
                                customDraw->clrTextBk = CLR_NONE;

                                SetWindowLongPtr(WindowHandle, DWLP_MSGRESULT, CDRF_NEWFONT);
                                return CDRF_NEWFONT;
                            }
                        }
                    }
                    break;
                case LVN_KEYDOWN:
                    {
                        LPNMLVKEYDOWN keyDown = (LPNMLVKEYDOWN)lParam;

                        switch (keyDown->wVKey)
                        {
                        case 'C':
                            {
                                if (PhGetKeyState(VK_CONTROL))
                                {
                                    PhCopyListView(context->ListViewHandle);
                                }
                            }
                            break;
                        }
                    }
                    break;
                }
            }
            else if (header && header->hwndFrom == context->HeaderHandle)
            {
                // Redraw row progress backgrounds while/after column resize.

                switch (header->code)
                {
                case HDN_TRACKW:
                case HDN_TRACKA:
                case HDN_ENDTRACKW:
                case HDN_ENDTRACKA:
                case HDN_ITEMCHANGEDW:
                case HDN_ITEMCHANGEDA:
                    InvalidateRect(context->ListViewHandle, NULL, FALSE);
                    break;
                }
            }

            REFLECT_MESSAGE_DLG(WindowHandle, context->ListViewHandle, WindowMessage, wParam, lParam);
        }
        break;

    case ET_WM_POWERGRID_UPDATE:
        {
            PET_POWER_GRID_UPDATE update = (PET_POWER_GRID_UPDATE)lParam;
            BOOLEAN useTariff;

            EnableWindow(context->RefreshButtonHandle, TRUE);

            // 自动模式：预测获取失败或无数据时切换峰谷渲染
            if (context->EffectiveMode == ET_POWER_GRID_MODE_TARIFF)
                useTariff = TRUE;
            else if (context->EffectiveMode == ET_POWER_GRID_MODE_FORECAST)
                useTariff = FALSE;
            else
                useTariff = ((HRESULT)wParam < 0) || update->Summary.TotalBlocks == 0;

            if (useTariff)
            {
                if (update->Entries)
                    EtpFreePowerGridList(update->Entries);

                if (context->EffectiveMode != ET_POWER_GRID_MODE_TARIFF)
                {
                    // 切换峰谷渲染前统一显隐（先隐去微软预测专用的刷新按钮）
                    EtpShowModeControls(context, TRUE);
                    context->EffectiveMode = ET_POWER_GRID_MODE_TARIFF;
                    EtpSetupListViewColumns(context->ListViewHandle, ET_POWER_GRID_MODE_TARIFF);
                }

                EtpRebuildTariffEntries(context, context->ModeSetting == ET_POWER_GRID_MODE_AUTO);
            }
            else
            {
                context->EffectiveMode = ET_POWER_GRID_MODE_FORECAST;

                SendMessage(context->ListViewHandle, WM_SETREDRAW, FALSE, 0);

                // ListView item 参数持有指针，旧列表保活至本次发布完成后再释放
                if (context->CurrentEntries)
                {
                    EtpFreePowerGridList(context->CurrentEntries);
                    context->CurrentEntries = NULL;
                }

                EtpClearPowerGridListView(context->ListViewHandle);

                context->CurrentEntries = update->Entries;

                EtpPublishPowerGridList(context->ListViewHandle, context->SummaryHandle, update->Entries, &update->Summary);

                SendMessage(context->ListViewHandle, WM_SETREDRAW, TRUE, 0);
            }

            PhFree(update);
        }
        break;
    }

    return FALSE;
}

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS EtPowerGridDialogThreadStart(
    _In_ PVOID Parameter
    )
{
    PPOWER_GRID_WINDOW_CONTEXT context = Parameter;
    BOOL result;
    MSG message;
    HWND windowHandle;
    PH_AUTO_POOL autoPool;

    PhInitializeAutoPool(&autoPool);

    windowHandle = PhCreateDialog(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_POWER_GRID),
        !!PhGetIntegerSetting(SETTING_FORCE_NO_PARENT) ? NULL : context->ParentWindowHandle,
        EtPowerGridDlgProc,
        context
        );

    ShowWindow(windowHandle, SW_SHOW);
    SetForegroundWindow(windowHandle);

    while (result = GetMessage(&message, NULL, 0, 0))
    {
        if (result == INT_ERROR)
            break;

        if (!IsDialogMessage(windowHandle, &message))
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        PhDrainAutoPool(&autoPool);
    }

    PhDeleteAutoPool(&autoPool);

    return STATUS_SUCCESS;
}

VOID EtShowPowerGridDialog(
    _In_ HWND ParentWindowHandle
    )
{
    PPOWER_GRID_WINDOW_CONTEXT context;

    context = PhAllocateZero(sizeof(POWER_GRID_WINDOW_CONTEXT));
    context->ParentWindowHandle = ParentWindowHandle;

    PhCreateThread2(EtPowerGridDialogThreadStart, context);
}
