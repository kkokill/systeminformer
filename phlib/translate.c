/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2026
 *
 * 运行时 UI 文本翻译引擎（zh→en 正向 / en→zh 反向）— 查表翻译硬编码中文字符串。
 *
 * 文件结构：英文模式开关 → 正向查找 → 反向索引与反向查找。
 *
 * 设计要点：
 * - 字典为 PhTranslateTable[]（translate_data.c 自动生成，UTF-16 码元序），
 *   外部 .lang 文件加载后经 PhpActiveTable（translate_lang.c）优先生效
 * - PhTranslateTextZ 二分查找；中文模式零开销直接返回原文，
 *   未命中同样返回原文（调用方以指针相等判断"未翻译"）
 * - 英文模式开关（PhTranslateSetEnglishEnabled）由应用语言设置驱动
 *   （guisup.c PhSetApplicationLanguage）
 * - 反向索引按 En 排序、惰性构建；以（表指针, 条目数）比对活动表，
 *   不一致即自动重建（PhLoadLanguageFile 换表不释放旧表，旧索引无悬挂）
 *
 * 生成器: tools\Localization\gen_translate.ps1（字典）+ _apply_trans.ps1（覆盖）
 */

#include <ph.h>
#include <guisup.h>
#include <translate.h>

// 嵌入表（translate_data.c 生成）
extern const PH_TRANSLATE_ENTRY PhTranslateTable[];
extern const ULONG PhTranslateTableCount;

// === 英文模式开关 ===

// 英文模式开关（FALSE=中文模式，直接返回原文）
static volatile BOOLEAN PhpTranslateEnglishEnabled = FALSE;

VOID NTAPI PhTranslateSetEnglishEnabled(
    _In_ BOOLEAN Enabled
    )
{
    PhpTranslateEnglishEnabled = Enabled;
    MemoryBarrier();
}

BOOLEAN NTAPI PhTranslateIsEnglishEnabled(
    VOID
    )
{
    return PhpTranslateEnglishEnabled;
}

// === 正向查找（zh→en） ===

// 在有序表中二分查找（键按 UTF-16 码元序排序，与 wcscmp 一致）
static PCWSTR PhpTranslateLookup(
    _In_ PCWSTR Text,
    _In_reads_(Count) const PH_TRANSLATE_ENTRY *Table,
    _In_ ULONG Count
    )
{
    ULONG lo;
    ULONG hi;

    lo = 0;
    hi = Count;

    while (lo < hi)
    {
        ULONG mid = (lo + hi) / 2;
        int cmp = wcscmp(Text, Table[mid].Zh);

        if (cmp == 0)
            return Table[mid].En;

        if (cmp < 0)
            hi = mid;
        else
            lo = mid + 1;
    }

    return NULL;
}

PCWSTR NTAPI PhTranslateTextZ(
    _In_opt_ PCWSTR Text
    )
{
    const PH_TRANSLATE_ENTRY *table;
    ULONG count;
    PCWSTR result;

    if (!Text || !Text[0])
        return Text;

    // 中文模式零开销：直接返回原文
    if (!PhpTranslateEnglishEnabled)
        return Text;

    // 外部 .lang 活动表优先，否则嵌入表
    if (PhpActiveTable)
    {
        table = PhpActiveTable;
        count = PhpActiveTableCount;
    }
    else
    {
        table = PhTranslateTable;
        count = PhTranslateTableCount;
    }

    if (!table || count == 0)
        return Text;

    result = PhpTranslateLookup(Text, table, count);

    // 未命中返回原文：字典未收录的串保持原显示
    return result ? result : Text;
}

// === 反向查找（en→zh，惰性索引） ===

// 指向活动表条目的指针数组，按 En（UTF-16 码元序）排序。
// 以（表指针, 条目数）比对当前活动表，不一致即自动重建；
// PhLoadLanguageFile 换表不释放旧表，旧索引不存在悬挂指针。
static const PH_TRANSLATE_ENTRY **PhpReverseIndex = NULL;
static ULONG PhpReverseIndexCount = 0;
static const PH_TRANSLATE_ENTRY *PhpReverseIndexTable = NULL;
static ULONG PhpReverseIndexTableCount = 0;

static int __cdecl PhpReverseIndexCompare(
    _In_ const void *elem1,
    _In_ const void *elem2
    )
{
    const PH_TRANSLATE_ENTRY *entry1 = *(const PH_TRANSLATE_ENTRY **)elem1;
    const PH_TRANSLATE_ENTRY *entry2 = *(const PH_TRANSLATE_ENTRY **)elem2;

    return wcscmp(entry1->En, entry2->En);
}

// 从活动表构建反向索引（过滤无效与 En==Zh 占位条目）
static VOID PhpReverseIndexBuild(
    _In_ const PH_TRANSLATE_ENTRY *Table,
    _In_ ULONG Count
    )
{
    const PH_TRANSLATE_ENTRY **index;
    ULONG used;
    ULONG i;

    used = 0;

    if (PhpReverseIndex)
    {
        PhFree((PVOID)PhpReverseIndex);
        PhpReverseIndex = NULL;
        PhpReverseIndexCount = 0;
    }

    PhpReverseIndexTable = NULL;
    PhpReverseIndexTableCount = 0;

    index = PhAllocate(Count * sizeof(PH_TRANSLATE_ENTRY *));

    for (i = 0; i < Count; i++)
    {
        if (!Table[i].Zh || !Table[i].En || !Table[i].En[0])
            continue;

        // En==Zh 占位条目：反向命中也返回原文，无意义，跳过
        if (wcscmp(Table[i].Zh, Table[i].En) == 0)
            continue;

        index[used++] = &Table[i];
    }

    qsort(index, used, sizeof(PH_TRANSLATE_ENTRY *), PhpReverseIndexCompare);

    PhpReverseIndex = index;
    PhpReverseIndexCount = used;
    PhpReverseIndexTable = Table;
    PhpReverseIndexTableCount = Count;
}

// 反向查找：英→中。返回对应中文原文，未命中返回 NULL。
// 供窗口重翻路径切回中文时使用；纯查表，不受英文模式开关影响。
PCWSTR NTAPI PhTranslateTextReverseZ(
    _In_opt_ PCWSTR Text
    )
{
    const PH_TRANSLATE_ENTRY *table;
    ULONG count;
    ULONG lo;
    ULONG hi;

    if (!Text || !Text[0])
        return NULL;

    // 活动表与正向路径一致
    if (PhpActiveTable)
    {
        table = PhpActiveTable;
        count = PhpActiveTableCount;
    }
    else
    {
        table = PhTranslateTable;
        count = PhTranslateTableCount;
    }

    if (!table || count == 0)
        return NULL;

    // 表变更（外部 .lang 换表 / 首次使用）时重建
    if (!PhpReverseIndex ||
        PhpReverseIndexTable != table ||
        PhpReverseIndexTableCount != count)
    {
        PhpReverseIndexBuild(table, count);
    }

    if (!PhpReverseIndex || PhpReverseIndexCount == 0)
        return NULL;

    lo = 0;
    hi = PhpReverseIndexCount;

    while (lo < hi)
    {
        ULONG mid = (lo + hi) / 2;
        int cmp = wcscmp(Text, PhpReverseIndex[mid]->En);

        if (cmp == 0)
        {
            ULONG first = mid;

            // 多 Zh 同 En：左移取排序最前条目，结果确定
            while (first > 0 && wcscmp(PhpReverseIndex[first - 1]->En, Text) == 0)
                first--;

            return PhpReverseIndex[first]->Zh;
        }

        if (cmp < 0)
            hi = mid;
        else
            lo = mid + 1;
    }

    return NULL;
}
