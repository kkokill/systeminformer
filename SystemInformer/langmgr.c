/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2026
 *
 * 语言状态机驱动器 — 协调运行时语言热切换的全流程：
 * 1. 加载阶段（确认翻译表就绪）
 * 2. 刷新本体阶段（重建菜单 + 各 TreeNew 列头 + Tab 标签 + System Information + 窗口遍历）
 * 3. 刷新插件阶段（广播 GeneralCallbackLanguageChanged + 兼容 GeneralCallbackSettingsUpdated）
 * 4. 回到 Idle
 *
 * 全程同步执行在调用方线程（主线程）。性能约束：
 * - 仅在用户主动切换语言时执行，正常运行零开销。
 * - 刷新操作只重设文本指针，不重建节点列表。
 */

#include <phapp.h>
#include <translate.h>
#include <guisup.h>
#include <emenu.h>
#include <mainwnd.h>

#include <mainwndp.h>
#include <proctree.h>
#include <srvlist.h>
#include <netlist.h>
#include <sysinfo.h>
#include <phplug.h>
#include <phsettings.h>

/**
 * 驱动语言热切换的状态机入口。
 *
 * \param English TRUE 切换到英文，FALSE 切换到中文。
 *
 * \remarks 由 options.c 的 IDC_LANGUAGE CBN_SELCHANGE 调用。
 *          PhSetIntegerSetting(SETTING_LANGUAGE, ...) 由调用方负责。
 */
VOID PhSwitchApplicationLanguage(
    _In_ BOOLEAN English
    )
{
    PH_LANGUAGE_CHANGE_PARAM param = { 0 };

    param.English = English;
    param.OldLanguage = PhGetApplicationLanguage() ? 1 : 0;
    param.NewLanguage = English ? 1 : 0;

    // === 1. 加载阶段 ===
    PhSetLanguageState(LanguageStateLoadingFile);
    // PhLoadLanguageFile 在 main.c 启动时已调；此处仅在首次切换时确保
    if (!PhIsLanguageFileLoaded())
    {
        PhLoadLanguageFile(); // 失败则回退嵌入表，不报错
    }

    // === 2. 刷新本体阶段 ===
    PhSetLanguageState(LanguageStateRefreshingHost);

    // 切换语言标志 + 线程 UI 语言
    PhSetApplicationLanguage(English); // guisup.c: SetThreadUILanguage + PhTranslateSetEnglishEnabled

    // 重建主菜单
    if (PhMainWndHandle)
    {
        HMENU oldMenu = GetMenu(PhMainWndHandle);
        PhMwpInitializeMainMenu(PhMainWndHandle);
        if (oldMenu) DestroyMenu(oldMenu);
        InvalidateRect(PhMainWndHandle, NULL, TRUE);
    }

    // 本体刷新：各 TreeNew 列头 + Tab 标签 + System Information
    PhRefreshProcessTreeLanguage();
    PhRefreshServiceTreeLanguage();
    PhRefreshNetworkTreeLanguage();
    PhMwpRefreshTabLanguage();
    PhSipRecreateForLanguageChange();

    // 枚举本进程所有 SysListView32 子窗口刷新列头（含插件窗口）
    // 覆盖 PhAddListViewColumnDpi 创建的 ListView 列头（含插件 80+ 列）
    PhRefreshAllListViewColumnsLanguage();

    // 枚举本进程所有 PhTreeNew 窗口重翻列头并重绘行内容
    // （覆盖 Users List、插件 TreeNew 等未显式订阅语言切换的列表）
    PhRefreshAllTreeNewColumnsLanguage();

    // 窗口遍历兜底重翻（Button/Static/Edit 等纯文本控件，方向跟随当前语言模式）
    PhRetranslateAllWindows();

    // === 3. 刷新插件阶段 ===
    PhSetLanguageState(LanguageStateRefreshingPlugins);

    // 广播语言变更通知（新回调，插件可选择性订阅）
    PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackLanguageChanged), &param);

    // 兼容广播：现有插件通过 SettingsUpdated 订阅了语言刷新
    {
        BOOLEAN restartRequired = FALSE;
        PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackSettingsUpdated), &restartRequired);
    }

    // === 4. 完成 ===
    PhSetLanguageState(LanguageStateIdle);
}
