/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2026
 *
 * 窗口遍历重翻 — 语言切换时枚举本进程所有窗口，对 Button/Static/Edit
 * 等纯文本控件执行 GetWindowText → PhTranslateTextZ → SetWindowText，
 * 对话框框架（#32770）同样按基线机制翻译标题栏（插件 rc 无英文模板，
 * 标题词典键翻译全靠此路径），作为组件专用刷新的兜底覆盖。
 *
 * 不覆盖（需组件专用刷新）：
 * - Toolbar（ToolStatus 已通过 ToolbarLoadSettings 处理）
 * - ComboBox/ListBox（列表项由所有者维护）
 * - TabControl（PhMwpRefreshTabLanguage 已处理）
 * - Header（TreeNew_TranslateColumns 已处理）
 * - StatusBar（动态文本由代码控制）
 *
 * ListView/TreeView：列头走 PhRefreshListViewColumnsLanguage（原始文本数组），
 * 列表项直接遍历反查重翻（英文显示文本反查字典）。
 *
 * 窗口文本/树项走原文基线记录：首次翻译前记录当前文本为基线，此后重翻一律
 * 从基线出发（英文模式正向查表 / 中文模式直接还原基线）。反向反查仅作为
 * 列表项兜底，因为多条中文键共享同一英文值时反查结果不确定（如 Normal 同时
 * 对应 常规/&正常/正常，往返切换会串词），基线机制保证往返精确还原。
 */

#include <ph.h>
#include <guisup.h>
#include <translate.h>
#include <commctrl.h>
#include <treenew.h>

// 前置声明
static VOID PhRetranslateWindowRecursive(
    _In_ HWND hwnd,
    _In_ ULONG Depth
    );

// === ListView 列头原始文本数组结构（与 guisuplistview.cpp 共用约定） ===
// 字段布局与属性名必须两处同步修改。
typedef struct _PHP_LV_COLTEXT_ARRAY
{
    ULONG Count;
    ULONG Capacity;
    PCWSTR Texts[1];
} PHP_LV_COLTEXT_ARRAY, *PPHP_LV_COLTEXT_ARRAY;

#define PHP_LV_COLTEXT_PROP L"SiLvColText"

// === 控件判定与重翻取词 ===

// 白名单：可自动重翻的窗口类名
static BOOLEAN PhpIsRetranslatableClass(
    _In_ PCWSTR ClassName
    )
{
    return
        wcscmp(ClassName, L"Button") == 0 ||   // 按钮/复选框/单选/分组框
        wcscmp(ClassName, L"Static") == 0;      // 静态文本
}

// 判断是否为 Edit/RichEdit 控件（仅未脏时可覆盖）
static BOOLEAN PhpIsEditClass(
    _In_ PCWSTR ClassName
    )
{
    return
        wcscmp(ClassName, L"Edit") == 0 ||
        wcsstr(ClassName, L"RichEdit") != NULL;
}

// 重翻方向：TRUE=正向（zh→en），FALSE=反向（en→zh）。
// 由两个入口在遍历前按当前语言模式设定（遍历同步执行，期间不会变更）。
static BOOLEAN PhpRetranslateForward = TRUE;

// 重翻取词：正向调 zh→en 字典，反向调 en→zh 字典，未命中均返回原文。
static PCWSTR PhpRetranslateLookupText(
    _In_ PCWSTR Text
    )
{
    PCWSTR result;

    if (PhpRetranslateForward)
        return PhTranslateTextZ(Text);

    result = PhTranslateTextReverseZ(Text);

    return result ? result : Text;
}

// === 原文基线记录（窗口文本 / TreeView 项） ===
//
// 往返切换（中→英→中）必须精确还原原文：反向反查在多中文键共享同一英文值时
// 结果不确定。因此首次见到控件/树项时记录其文本为基线（恒为中文原文），此后
// 判断显示文本是否仍等于基线在任一语言下的输出：是则未被外部改动，按基线重翻；
// 否则视为代码重设了新文本，以当前文本重记基线。基线随窗口属性存放，与
// ListView 列头原始文本数组（SiLvColText）同一模式（控件销毁时随属性列表
// 一并丢弃）。

#define PHP_WNDTEXT_PROP L"SiWndText" // Button/Static/Edit：值为堆上 PWSTR 基线
#define PHP_TVTEXT_PROP L"SiTvText"   // TreeView：值为 PHP_TV_TEXT_ARRAY 基线数组
#define PHP_LVGROUP_TEXT_PROP L"SiLvGroupText" // ListView 组头：值为 PHP_LVGROUP_TEXT_ARRAY 基线数组
#define PHP_NORETRANSLATE_PROP L"SiNoRetranslate" // 文本由代码全权管理的控件（语言切换按钮等）

typedef struct _PHP_TV_TEXT_ENTRY
{
    HTREEITEM Item;
    PWSTR Text; // 基线原文（中文）
    BOOLEAN Seen;
} PHP_TV_TEXT_ENTRY, *PPHP_TV_TEXT_ENTRY;

typedef struct _PHP_TV_TEXT_ARRAY
{
    ULONG Count;
    ULONG Capacity;
    PHP_TV_TEXT_ENTRY Items[1];
} PHP_TV_TEXT_ARRAY, *PPHP_TV_TEXT_ARRAY;

static PWSTR PhpDuplicateText(
    _In_ PCWSTR Text
    )
{
    SIZE_T length;
    PWSTR copy;

    length = (wcslen(Text) + 1) * sizeof(WCHAR);
    copy = PhAllocate(length);
    memcpy(copy, Text, length);

    return copy;
}

// 判断文本是否仍是我们上次设置的样子（基线原文或其英文译文）：
// 是则按基线重翻；否则说明代码重设了新文本，需要重记基线。
static BOOLEAN PhpTextUntouched(
    _In_ PCWSTR Text,
    _In_ PCWSTR Baseline
    )
{
    PCWSTR english;

    if (wcscmp(Text, Baseline) == 0)
        return TRUE;

    english = PhTranslateTextRawZ(Baseline); // 纯查表（不受语言模式门控）：基线的英文显示值

    return wcscmp(Text, english) == 0;
}

// 首次记录基线时使用：英文模式下当前文本可能是模板/创建期的英文显示值，
// 反查回中文原文作为基线（未命中即原文，如动态值/路径/纯值文本）。
static PCWSTR PhpBaselineFromText(
    _In_ PCWSTR Text
    )
{
    PCWSTR reverse;

    if (!PhTranslateIsEnglishEnabled())
        return Text;

    reverse = PhTranslateTextReverseZ(Text);

    return reverse ? reverse : Text;
}

// 翻译纯文本控件（Button/Static/Edit），带原文基线记录
static VOID PhpRetranslateWindowText(
    _In_ HWND hwnd,
    _In_ BOOLEAN IsEdit
    )
{
    WCHAR text[512];
    PWSTR baseline;
    PCWSTR source;
    PCWSTR translated;

    if (GetPropW(hwnd, PHP_NORETRANSLATE_PROP))
        return; // 文本由代码全权管理（如语言切换按钮）

    if (IsEdit && SendMessageW(hwnd, EM_GETMODIFY, 0, 0) != 0)
        return; // 用户已编辑：不覆盖也不改基线

    if (GetWindowTextW(hwnd, text, RTL_NUMBER_OF(text)) == 0)
        return;

    baseline = (PWSTR)GetPropW(hwnd, PHP_WNDTEXT_PROP);

    if (baseline && PhpTextUntouched(text, baseline))
    {
        source = baseline; // 未被外部改动：以基线为准
    }
    else
    {
        // 首次记录或代码重设了新文本：以当前文本为新基线
        source = text;

        if (baseline)
            PhFree(baseline);

        baseline = PhpDuplicateText(PhpBaselineFromText(text));
        SetPropW(hwnd, PHP_WNDTEXT_PROP, (HANDLE)baseline);
    }

    // 英文模式翻译基线；中文模式显示基线（未被改动时即为还原）
    translated = PhpRetranslateForward ? PhTranslateTextZ(source) : source;

    if (wcscmp(text, translated) != 0)
        SetWindowTextW(hwnd, (PWSTR)translated);
}

// 在树项基线数组中查找指定树项的记录
static PPHP_TV_TEXT_ENTRY PhpLookupTvText(
    _In_ PPHP_TV_TEXT_ARRAY Array,
    _In_ HTREEITEM Item
    )
{
    ULONG i;

    for (i = 0; i < Array->Count; i++)
    {
        if (Array->Items[i].Item == Item)
            return &Array->Items[i];
    }

    return NULL;
}

// 取指定树项的基线记录，不存在则追加
static PPHP_TV_TEXT_ENTRY PhpEnsureTvText(
    _In_ HWND hwnd,
    _In_ HTREEITEM Item
    )
{
    PPHP_TV_TEXT_ARRAY array;
    PPHP_TV_TEXT_ENTRY entry;

    array = (PPHP_TV_TEXT_ARRAY)GetPropW(hwnd, PHP_TVTEXT_PROP);

    if (!array)
    {
        array = PhAllocate(sizeof(PHP_TV_TEXT_ARRAY));
        array->Count = 0;
        array->Capacity = 1;
        SetPropW(hwnd, PHP_TVTEXT_PROP, (HANDLE)array);
    }

    entry = PhpLookupTvText(array, Item);

    if (!entry)
    {
        if (array->Count == array->Capacity)
        {
            array->Capacity *= 2;
            array = PhReAllocate(
                array,
                sizeof(PHP_TV_TEXT_ARRAY) + (array->Capacity - 1) * sizeof(PHP_TV_TEXT_ENTRY)
                );
            SetPropW(hwnd, PHP_TVTEXT_PROP, (HANDLE)array);
        }

        entry = &array->Items[array->Count++];
        entry->Item = Item;
        entry->Text = NULL;
        entry->Seen = FALSE;
    }

    return entry;
}

// 清理未被本次遍历命中的树项基线（树项已删除），并复位 Seen 标记
static VOID PhpPurgeTvText(
    _In_ HWND hwnd
    )
{
    PPHP_TV_TEXT_ARRAY array;
    ULONG i;

    array = (PPHP_TV_TEXT_ARRAY)GetPropW(hwnd, PHP_TVTEXT_PROP);

    if (!array)
        return;

    for (i = 0; i < array->Count; )
    {
        if (array->Items[i].Seen)
        {
            array->Items[i].Seen = FALSE; // 复位，供下次遍历
            i++;
        }
        else
        {
            if (array->Items[i].Text)
                PhFree(array->Items[i].Text);

            array->Items[i] = array->Items[array->Count - 1];
            array->Count--;
        }
    }
}

// === 窗口遍历重翻 ===

// 翻译单个 ListView 控件的全部列表项（方向由 PhpRetranslateForward 决定）
static VOID PhpRetranslateListViewItems(
    _In_ HWND hwnd
    )
{
    WCHAR text[512];
    LONG count;
    LONG i;

    count = ListView_GetItemCount(hwnd);

    for (i = 0; i < count; i++)
    {
        ListView_GetItemText(hwnd, i, 0, text, RTL_NUMBER_OF(text));

        if (text[0] == 0)
            continue;

        PCWSTR translated = PhpRetranslateLookupText(text);

        if (translated != text)
        {
            ListView_SetItemText(hwnd, i, 0, (PWSTR)translated);
        }
    }
}

// === ListView 组头基线（组头不属于列表项，需单独记录/重翻） ===

typedef struct _PHP_LVGROUP_TEXT_ENTRY
{
    LONG GroupId;
    PWSTR Text; // 基线原文（中文）
    BOOLEAN Seen;
} PHP_LVGROUP_TEXT_ENTRY, *PPHP_LVGROUP_TEXT_ENTRY;

typedef struct _PHP_LVGROUP_TEXT_ARRAY
{
    ULONG Count;
    ULONG Capacity;
    PHP_LVGROUP_TEXT_ENTRY Items[1];
} PHP_LVGROUP_TEXT_ARRAY, *PPHP_LVGROUP_TEXT_ARRAY;

static PPHP_LVGROUP_TEXT_ENTRY PhpLookupLvGroupText(
    _In_ PPHP_LVGROUP_TEXT_ARRAY Array,
    _In_ LONG GroupId
    )
{
    ULONG i;

    for (i = 0; i < Array->Count; i++)
    {
        if (Array->Items[i].GroupId == GroupId)
            return &Array->Items[i];
    }

    return NULL;
}

static PPHP_LVGROUP_TEXT_ENTRY PhpEnsureLvGroupText(
    _In_ HWND hwnd,
    _In_ LONG GroupId
    )
{
    PPHP_LVGROUP_TEXT_ARRAY array;
    PPHP_LVGROUP_TEXT_ENTRY entry;

    array = (PPHP_LVGROUP_TEXT_ARRAY)GetPropW(hwnd, PHP_LVGROUP_TEXT_PROP);

    if (!array)
    {
        array = PhAllocate(sizeof(PHP_LVGROUP_TEXT_ARRAY));
        array->Count = 0;
        array->Capacity = 1;
        SetPropW(hwnd, PHP_LVGROUP_TEXT_PROP, (HANDLE)array);
    }

    entry = PhpLookupLvGroupText(array, GroupId);

    if (!entry)
    {
        if (array->Count == array->Capacity)
        {
            array->Capacity *= 2;
            array = PhReAllocate(
                array,
                sizeof(PHP_LVGROUP_TEXT_ARRAY) + (array->Capacity - 1) * sizeof(PHP_LVGROUP_TEXT_ENTRY)
                );
            SetPropW(hwnd, PHP_LVGROUP_TEXT_PROP, (HANDLE)array);
        }

        entry = &array->Items[array->Count++];
        entry->GroupId = GroupId;
        entry->Text = NULL;
        entry->Seen = FALSE;
    }

    return entry;
}

// 清理未被本次遍历命中的组头基线（组已删除），并复位 Seen 标记
static VOID PhpPurgeLvGroupText(
    _In_ HWND hwnd
    )
{
    PPHP_LVGROUP_TEXT_ARRAY array;
    ULONG i;

    array = (PPHP_LVGROUP_TEXT_ARRAY)GetPropW(hwnd, PHP_LVGROUP_TEXT_PROP);

    if (!array)
        return;

    for (i = 0; i < array->Count; )
    {
        if (array->Items[i].Seen)
        {
            array->Items[i].Seen = FALSE; // 复位，供下次遍历
            i++;
        }
        else
        {
            if (array->Items[i].Text)
                PhFree(array->Items[i].Text);

            array->Items[i] = array->Items[array->Count - 1];
            array->Count--;
        }
    }
}

// 翻译 ListView 组头（与树项同一基线模型：往返精确还原）
static VOID PhpRetranslateListViewGroups(
    _In_ HWND hwnd
    )
{
    WCHAR text[512];
    LONG count;
    LONG i;

    count = (LONG)ListView_GetGroupCount(hwnd);

    for (i = 0; i < count; i++)
    {
        LVGROUP group;
        PPHP_LVGROUP_TEXT_ENTRY entry;
        PCWSTR source;
        PCWSTR translated;

        memset(&group, 0, sizeof(LVGROUP));
        group.cbSize = sizeof(LVGROUP);
        group.mask = LVGF_HEADER | LVGF_GROUPID;
        group.pszHeader = text;
        group.cchHeader = RTL_NUMBER_OF(text);

        if (ListView_GetGroupInfoByIndex(hwnd, i, &group) == -1)
            continue;

        if (group.iGroupId == -1 || text[0] == 0)
            continue;

        entry = PhpEnsureLvGroupText(hwnd, group.iGroupId);
        entry->Seen = TRUE;

        if (entry->Text && PhpTextUntouched(text, entry->Text))
        {
            source = entry->Text; // 未被外部改动：以基线为准
        }
        else
        {
            // 首次记录或代码重设了新文本：以当前文本为新基线
            source = text;

            if (entry->Text)
                PhFree(entry->Text);

            entry->Text = PhpDuplicateText(PhpBaselineFromText(text));
        }

        // 英文模式翻译基线；中文模式显示基线（未被改动时即为还原）
        translated = PhpRetranslateForward ? PhTranslateTextZ(source) : source;

        if (wcscmp(text, translated) != 0)
        {
            memset(&group, 0, sizeof(LVGROUP));
            group.cbSize = sizeof(LVGROUP);
            group.mask = LVGF_HEADER;
            group.pszHeader = (PWSTR)translated;
            ListView_SetGroupInfo(hwnd, group.iGroupId, &group);
        }
    }

    PhpPurgeLvGroupText(hwnd);
}

// 递归翻译 TreeView 项（深度限制防自绘/回调控件异常）；
// 项文本走原文基线记录，往返切换精确还原
static VOID PhpRetranslateTreeItems(
    _In_ HWND hwnd,
    _In_opt_ HTREEITEM item,
    _In_ ULONG Depth
    )
{
    WCHAR text[512];
    TVITEMW tvItem;
    HTREEITEM child;

    if (Depth >= 16)
        return;

    while (item)
    {
        memset(&tvItem, 0, sizeof(TVITEMW));
        tvItem.mask = TVIF_TEXT | TVIF_CHILDREN;
        tvItem.hItem = item;
        tvItem.pszText = text;
        tvItem.cchTextMax = RTL_NUMBER_OF(text);

        if (TreeView_GetItem(hwnd, &tvItem) && text[0] != 0)
        {
            BOOLEAN hasChildren = tvItem.cChildren > 0;
            PPHP_TV_TEXT_ENTRY entry;
            PCWSTR source;
            PCWSTR translated;

            entry = PhpEnsureTvText(hwnd, item);
            entry->Seen = TRUE;

            if (entry->Text && PhpTextUntouched(text, entry->Text))
            {
                source = entry->Text; // 未被外部改动：以基线为准
            }
            else
            {
                // 首次记录或代码重设了新文本：以当前文本为新基线
                source = text;

                if (entry->Text)
                    PhFree(entry->Text);

                entry->Text = PhpDuplicateText(PhpBaselineFromText(text));
            }

            // 英文模式翻译基线；中文模式显示基线（未被改动时即为还原）
            translated = PhpRetranslateForward ? PhTranslateTextZ(source) : source;

            if (wcscmp(text, translated) != 0)
            {
                memset(&tvItem, 0, sizeof(TVITEMW));
                tvItem.mask = TVIF_TEXT;
                tvItem.hItem = item;
                tvItem.pszText = (PWSTR)translated;
                tvItem.cchTextMax = 0;
                TreeView_SetItem(hwnd, &tvItem);
            }

            if (hasChildren)
            {
                child = TreeView_GetChild(hwnd, item);

                if (child)
                    PhpRetranslateTreeItems(hwnd, child, Depth + 1);
            }
        }

        item = TreeView_GetNextSibling(hwnd, item);
    }
}

// 翻译组件型子窗口（ListView 项 / TreeView 项）
static VOID PhpRetranslateComponentItems(
    _In_ HWND hwnd
    )
{
    WCHAR className[64];

    if (GetClassNameW(hwnd, className, RTL_NUMBER_OF(className)) == 0)
        return;

    if (wcscmp(className, L"SysListView32") == 0)
    {
        PhpRetranslateListViewGroups(hwnd); // 组头（不属于列表项）
        PhpRetranslateListViewItems(hwnd);
    }
    else if (wcscmp(className, L"SysTreeView32") == 0)
    {
        HTREEITEM root = TreeView_GetRoot(hwnd);

        if (root)
            PhpRetranslateTreeItems(hwnd, root, 0);

        PhpPurgeTvText(hwnd); // 清理已删除树项的基线并复位 Seen 标记
    }
}

// 递归遍历子窗口
static BOOL CALLBACK PhpRetranslateChildProc(
    _In_ HWND hwnd,
    _In_ LPARAM lParam
    )
{
    PhRetranslateWindowRecursive(hwnd, (ULONG)lParam + 1);
    return TRUE;
}

static VOID PhRetranslateWindowRecursive(
    _In_ HWND hwnd,
    _In_ ULONG Depth
    )
{
    WCHAR className[64];

    if (GetClassNameW(hwnd, className, RTL_NUMBER_OF(className)) == 0)
        return;

    // 白名单控件：直接翻译（带原文基线记录）
    if (PhpIsRetranslatableClass(className))
    {
        PhpRetranslateWindowText(hwnd, FALSE);
    }
    // Edit/RichEdit：仅未脏时覆盖（防止覆盖用户输入）
    else if (PhpIsEditClass(className))
    {
        PhpRetranslateWindowText(hwnd, TRUE);
    }
    // 对话框框架：翻译标题栏。主程序/peview 的 rc 有中英双模板（英文模式标题
    // 已由资源给出，反查命中同键译文一致，无视觉变化）；插件 rc 仅有中文模板，
    // 英文模式的标题翻译全靠此路径（标题中文文本即词典键）。
    else if (wcscmp(className, L"#32770") == 0)
    {
        PhpRetranslateWindowText(hwnd, FALSE);
    }

    // 组件型子窗口：ListView/TreeView 的列表项、树项
    PhpRetranslateComponentItems(hwnd);

    // 递归子窗口（深度限制防无限递归）
    if (Depth < 16)
    {
        EnumChildWindows(hwnd, PhpRetranslateChildProc, (LPARAM)Depth);
    }
}

// 顶层窗口枚举回调：只处理本进程的窗口
static BOOL CALLBACK PhpEnumTopWndProc(
    _In_ HWND hwnd,
    _In_ LPARAM lParam
    )
{
    DWORD pid;
    GetWindowThreadProcessId(hwnd, &pid);

    if (pid == GetCurrentProcessId())
    {
        PhRetranslateWindowRecursive(hwnd, 0);
    }

    return TRUE;
}

// === 公共入口 ===

VOID PhRetranslateAllWindows(
    VOID
    )
{
    // 方向=当前语言模式：切英文时屏幕仍为中文（正向），
    // 切回中文时屏幕仍为英文（反向）。
    PhpRetranslateForward = PhTranslateIsEnglishEnabled();

    EnumWindows(PhpEnumTopWndProc, 0);
}

// 翻译单个窗口及其全部子孙（对话框创建后调用，覆盖 .rc 模板内的
// Button/Static/Edit/ListView 项/TreeView 项中文文本；中文模式零开销）
VOID PhTranslateWindowTree(
    _In_ HWND WindowHandle
    )
{
    if (!WindowHandle)
        return;

    // 模板文本恒为中文：英文模式正向翻译；中文模式反向未命中即原文（零变更）
    PhpRetranslateForward = PhTranslateIsEnglishEnabled();

    PhRetranslateWindowRecursive(WindowHandle, 0);
}

// === ListView 列头刷新 ===

// 刷新指定 ListView 控件的所有列头文本
// 取 ListView 上附加的原始中文 Text 数组（由 PhAddListViewColumnDpi 记录），
// 重新调 PhTranslateTextZ 翻译，并 ListView_SetColumn 重设 pszText。
// 原始文本恒为中文：英文模式得到英文列头；中文模式得到原文（还原中文列头）。
VOID PhRefreshListViewColumnsLanguage(
    _In_ HWND ListViewHandle
    )
{
    PPHP_LV_COLTEXT_ARRAY arr;
    LVCOLUMN column;
    LONG count;
    LONG i;

    if (!ListViewHandle)
        return;

    arr = (PPHP_LV_COLTEXT_ARRAY)GetPropW(ListViewHandle, PHP_LV_COLTEXT_PROP);

    if (!arr || arr->Count == 0)
        return;

    // 列数取自 header（实际 ListView 列数）
    count = Header_GetItemCount(ListView_GetHeader(ListViewHandle));

    if (count <= 0)
        return;

    // 按 arr->Count 与 ListView 实际列数中较小者遍历
    if ((ULONG)count > arr->Count)
        count = (LONG)arr->Count;

    for (i = 0; i < count; i++)
    {
        PCWSTR originalText = arr->Texts[i];

        if (!originalText)
            continue;

        PCWSTR translated = PhTranslateTextZ(originalText);

        // 中文模式 translated==原文：重设即还原中文列头，不能跳过
        memset(&column, 0, sizeof(LVCOLUMN));
        column.mask = LVCF_TEXT;
        column.pszText = (PWSTR)translated;
        ListView_SetColumn(ListViewHandle, i, &column);
    }
}

// EnumChildWindows 回调：对本进程内的 SysListView32 子窗口刷新列头
// EnumChildWindows 会递归枚举所有子孙窗口
static BOOL CALLBACK PhpRefreshAllListViewProc(
    _In_ HWND hwnd,
    _In_ LPARAM lParam
    )
{
    WCHAR className[16];

    if (GetClassNameW(hwnd, className, RTL_NUMBER_OF(className)) > 0 &&
        wcscmp(className, L"SysListView32") == 0)
    {
        PhRefreshListViewColumnsLanguage(hwnd);
    }

    return TRUE;
}

static BOOL CALLBACK PhpRefreshAllListViewTopProc(
    _In_ HWND hwnd,
    _In_ LPARAM lParam
    )
{
    DWORD pid;

    GetWindowThreadProcessId(hwnd, &pid);

    if (pid == GetCurrentProcessId())
    {
        // 检查顶层窗口本身
        WCHAR className[16];

        if (GetClassNameW(hwnd, className, RTL_NUMBER_OF(className)) > 0 &&
            wcscmp(className, L"SysListView32") == 0)
        {
            PhRefreshListViewColumnsLanguage(hwnd);
        }

        // 递归枚举子孙
        EnumChildWindows(hwnd, PhpRefreshAllListViewProc, 0);
    }

    return TRUE;
}

// 枚举本进程所有顶层窗口及其子孙，对 SysListView32 子窗口刷新列头
VOID PhRefreshAllListViewColumnsLanguage(
    VOID
    )
{
    EnumWindows(PhpRefreshAllListViewTopProc, 0);
}

// === TreeNew 列头全局刷新 ===

// EnumChildWindows 回调：对本进程内的 PhTreeNew 子窗口重翻列头
static BOOL CALLBACK PhpRefreshAllTreeNewChildProc(
    _In_ HWND hwnd,
    _In_ LPARAM lParam
    )
{
    WCHAR className[32];

    if (GetClassNameW(hwnd, className, RTL_NUMBER_OF(className)) > 0 &&
        wcscmp(className, PH_TREENEW_CLASSNAME) == 0)
    {
        SendMessage(hwnd, TNM_TRANSLATECOLUMNS, 0, 0);
    }

    return TRUE;
}

static BOOL CALLBACK PhpRefreshAllTreeNewTopProc(
    _In_ HWND hwnd,
    _In_ LPARAM lParam
    )
{
    DWORD pid;

    GetWindowThreadProcessId(hwnd, &pid);

    if (pid == GetCurrentProcessId())
    {
        // 检查顶层窗口本身
        WCHAR className[32];

        if (GetClassNameW(hwnd, className, RTL_NUMBER_OF(className)) > 0 &&
            wcscmp(className, PH_TREENEW_CLASSNAME) == 0)
        {
            SendMessage(hwnd, TNM_TRANSLATECOLUMNS, 0, 0);
        }

        // 递归枚举子孙
        EnumChildWindows(hwnd, PhpRefreshAllTreeNewChildProc, 0);
    }

    return TRUE;
}

// 语言切换：对所有 PhTreeNew 窗口发送 TNM_TRANSLATECOLUMNS（重翻列头 +
// 失效客户区），覆盖未显式订阅语言切换的列表（Users List、插件 TreeNew 等）
VOID PhRefreshAllTreeNewColumnsLanguage(
    VOID
    )
{
    EnumWindows(PhpRefreshAllTreeNewTopProc, 0);
}

VOID PhSetWindowNoRetranslate(
    _In_ HWND WindowHandle
    )
{
    if (WindowHandle)
        SetPropW(WindowHandle, PHP_NORETRANSLATE_PROP, (HANDLE)TRUE);
}
