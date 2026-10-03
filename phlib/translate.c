/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2026
 *
 * 运行时 UI 文本翻译引擎（zh→en）— 查表翻译硬编码中文字符串。
 *
 * 设计要点：
 * - 字典为 PhTranslateTable[]（translate_data.c 自动生成，UTF-16 码元序），
 *   外部 .lang 文件加载后经 PhpActiveTable（translate_lang.c）优先生效
 * - PhTranslateTextZ 二分查找；中文模式零开销直接返回原文，
 *   未命中同样返回原文（调用方以指针相等判断"未翻译"）
 * - 英文模式开关（PhTranslateSetEnglishEnabled）由应用语言设置驱动
 *   （guisup.c PhSetApplicationLanguage）
 *
 * 生成器: tools\Localization\gen_translate.ps1（字典）+ _apply_trans.ps1（覆盖）
 */

#include <ph.h>
#include <guisup.h>
#include <translate.h>

// 嵌入表（translate_data.c 生成）
extern const PH_TRANSLATE_ENTRY PhTranslateTable[];
extern const ULONG PhTranslateTableCount;

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

    // 未命中返回原文（zh→en 单向：字典未收录的串保持原显示）
    return result ? result : Text;
}
