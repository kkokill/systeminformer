/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2026
 *
 * 翻译子系统公共头 — 声明翻译表条目类型、语言状态机与运行时热切换
 * API；外部 .lang 文件由 translate_lang.c 加载（见设计要点）。
 *
 * 设计要点：
 * - PhTranslateTextZ 通过 PhpActiveTable 间接访问翻译表，若为 NULL 则回退嵌入表
 * - PhTranslateTextReverseZ 反向查找（en→zh），索引随活动表变更自动重建
 * - 外部文件缺失时自动回退 translate_data.c 中的嵌入表，不报错
 * - 状态机可查询（PhGetLanguageState），驱动函数在 SystemInformer.exe 中
 * - 窗口遍历重翻（PhRetranslateAllWindows）作为兜底覆盖
 */

#ifndef PH_TRANSLATE_H
#define PH_TRANSLATE_H

#include <ph.h>

EXTERN_C_START

// === 翻译表条目类型（translate.c / translate_data.c / translate_lang.c 共用） ===

typedef struct _PH_TRANSLATE_ENTRY
{
    PCWSTR Zh;
    PCWSTR En;
} PH_TRANSLATE_ENTRY, *PPH_TRANSLATE_ENTRY;

// === 语言状态机 ===

typedef enum _PH_LANGUAGE_STATE
{
    LanguageStateIdle = 0,               // 默认运行态
    LanguageStateLoadingFile = 1,        // 正在加载外部翻译文件
    LanguageStateRefreshingHost = 2,    // 正在刷新主程序本体界面
    LanguageStateRefreshingPlugins = 3,  // 正在广播给插件
} PH_LANGUAGE_STATE, *PPH_LANGUAGE_STATE;

// === 语言变更通知参数（广播给插件用） ===

typedef struct _PH_LANGUAGE_CHANGE_PARAM
{
    BOOLEAN English;       // 新语言：TRUE=英文，FALSE=中文
    ULONG OldLanguage;     // 旧语言 ID（0=中文, 1=英文）
    ULONG NewLanguage;     // 新语言 ID
} PH_LANGUAGE_CHANGE_PARAM, *PPH_LANGUAGE_CHANGE_PARAM;

// === 公共 API ===

// 查询状态机当前状态（无锁读，调试/查询用）
PHLIBAPI
PH_LANGUAGE_STATE
NTAPI
PhGetLanguageState(
    VOID
    );

// 外部 .lang 文件是否已加载
PHLIBAPI
BOOLEAN
NTAPI
PhIsLanguageFileLoaded(
    VOID
    );

// 从 <AppDir>\lang\zh-en.lang 加载翻译表；失败回退嵌入表
// 启动时调用一次（main.c）
PHLIBAPI
NTSTATUS
NTAPI
PhLoadLanguageFile(
    VOID
    );

// 由 SystemInformer.exe 的状态机驱动器调用
PHLIBAPI
VOID
NTAPI
PhSetLanguageState(
    _In_ PH_LANGUAGE_STATE State
    );

// 窗口遍历重翻：枚举本进程所有窗口，对 Button/Static 等控件
// 执行 GetWindowText → PhTranslateTextZ → SetWindowText
PHLIBAPI
VOID
NTAPI
PhRetranslateAllWindows(
    VOID
    );

// 翻译单个窗口及其全部子孙（对话框创建后调用，覆盖 .rc 模板内的
// Button/Static/Edit/ListView 项/TreeView 项中文文本；中文模式零开销）
PHLIBAPI
VOID
NTAPI
PhTranslateWindowTree(
    _In_ HWND WindowHandle
    );

// 刷新指定 ListView 控件的所有列头文本
// 取 ListView 上附加的原始中文 Text 数组（由 PhAddListViewColumnDpi 记录），
// 重新调 PhTranslateTextZ 翻译，并 ListView_SetColumn 重设 pszText
PHLIBAPI
VOID
NTAPI
PhRefreshListViewColumnsLanguage(
    _In_ HWND ListViewHandle
    );

// 枚举本进程所有顶层窗口及其子孙，对 SysListView32 子窗口刷新列头
// 语言切换时调用一次，覆盖所有 ListView（含插件窗口）
PHLIBAPI
VOID
NTAPI
PhRefreshAllListViewColumnsLanguage(
    VOID
    );

// === phlib 内部（translate.c 直接引用） ===

// 活动翻译表指针（NULL=使用嵌入表 PhTranslateTable）
// translate_lang.c 定义，translate.c 通过 extern 引用
extern PPH_TRANSLATE_ENTRY PhpActiveTable;
extern ULONG PhpActiveTableCount;

EXTERN_C_END
#endif
