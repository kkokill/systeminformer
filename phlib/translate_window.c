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
 * 作为组件专用刷新的兜底覆盖。
 *
 * 不覆盖（需组件专用刷新）：
 * - Toolbar（ToolStatus 已通过 ToolbarLoadSettings 处理）
 * - ComboBox/ListBox（列表项由所有者维护）
 * - TabControl（PhMwpRefreshTabLanguage 已处理）
 * - Header（TreeNew_TranslateColumns 已处理）
 * - StatusBar（动态文本由代码控制）
 *
 * ListView/TreeView：列头走 PhRefreshListViewColumnsLanguage（原始文本数组），
 * 列表项/树项在此处直接遍历翻译。重翻方向跟随当前语言模式：切英文正向查找
 * （zh→en），切回中文反向查找（en→zh，PhTranslateTextReverseZ 反向索引），
 * 双向均可实时还原。
 */

#include <ph.h>
#include <guisup.h>
#include <translate.h>
#include <commctrl.h>

// 前置声明
static VOID PhRetranslateWindowRecursive(
    _In_ HWND hwnd,
    _In_ ULONG Depth
    );

// === ListView 列头原始文本数组结构（与 guisuplistview.cpp 共用约定） ===
typedef struct _PHP_LV_COLTEXT_ARRAY
{
    ULONG Count;
    ULONG Capacity;
    PCWSTR Texts[1];
} PHP_LV_COLTEXT_ARRAY, *PPHP_LV_COLTEXT_ARRAY;

#define PHP_LV_COLTEXT_PROP L"SiLvColText"

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

// 递归翻译 TreeView 项（深度限制防自绘/回调控件异常）
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
        BOOLEAN hasChildren;
        memset(&tvItem, 0, sizeof(TVITEMW));
        tvItem.mask = TVIF_TEXT | TVIF_CHILDREN;
        tvItem.hItem = item;
        tvItem.pszText = text;
        tvItem.cchTextMax = RTL_NUMBER_OF(text);

        if (TreeView_GetItem(hwnd, &tvItem) && tvItem.mask & TVIF_TEXT && text[0] != 0)
        {
            PCWSTR translated = PhpRetranslateLookupText(text);

            hasChildren = tvItem.cChildren > 0;

            if (translated != text)
            {
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
        PhpRetranslateListViewItems(hwnd);
    }
    else if (wcscmp(className, L"SysTreeView32") == 0)
    {
        HTREEITEM root = TreeView_GetRoot(hwnd);

        if (root)
            PhpRetranslateTreeItems(hwnd, root, 0);
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
    WCHAR text[512];

    if (GetClassNameW(hwnd, className, RTL_NUMBER_OF(className)) == 0)
        return;

    // 白名单控件：直接翻译
    if (PhpIsRetranslatableClass(className))
    {
        if (GetWindowTextW(hwnd, text, RTL_NUMBER_OF(text)) > 0)
        {
            PCWSTR translated = PhpRetranslateLookupText(text);

            if (translated != text)
            {
                SetWindowTextW(hwnd, (PWSTR)translated);
            }
        }
    }
    // Edit/RichEdit：仅未脏时覆盖（防止覆盖用户输入）
    else if (PhpIsEditClass(className))
    {
        if (SendMessageW(hwnd, EM_GETMODIFY, 0, 0) == 0)
        {
            if (GetWindowTextW(hwnd, text, RTL_NUMBER_OF(text)) > 0)
            {
                PCWSTR translated = PhpRetranslateLookupText(text);

                if (translated != text)
                {
                    SetWindowTextW(hwnd, (PWSTR)translated);
                }
            }
        }
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
