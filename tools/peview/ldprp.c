/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2011
 *     dmex    2017-2026
 *
 */

#include <peview.h>

typedef struct _PV_PE_LOADCONFIG_CONTEXT
{
    HWND WindowHandle;
    HWND ListViewHandle;
    PH_LAYOUT_MANAGER LayoutManager;
    PPV_PROPPAGECONTEXT PropSheetContext;
} PV_PE_LOADCONFIG_CONTEXT, *PPV_PE_LOADCONFIG_CONTEXT;

#define ADD_VALUE(Name, Value) \
{ \
    INT lvItemIndex; \
    lvItemIndex = PhAddListViewItem(lvHandle, MAXINT, Name, NULL); \
    PhSetListViewSubItem(lvHandle, lvItemIndex, 1, Value); \
}

PPH_STRING PvpGetPeGuardFlagsText(
    _In_ ULONG GuardFlags
    )
{
    PH_STRING_BUILDER stringBuilder;
    WCHAR pointer[PH_PTR_STR_LEN_1];

    if (GuardFlags == 0)
        return PhCreateString(L"0x0");

    PhInitializeStringBuilder(&stringBuilder, 10);

    if (GuardFlags & IMAGE_GUARD_CF_INSTRUMENTED)
        PhAppendStringBuilder2(&stringBuilder, PhTranslateTextZ(L"已插桩, "));
    if (GuardFlags & IMAGE_GUARD_CFW_INSTRUMENTED)
        PhAppendStringBuilder2(&stringBuilder, PhTranslateTextZ(L"已插桩（写入）, "));
    if (GuardFlags & IMAGE_GUARD_CF_FUNCTION_TABLE_PRESENT)
        PhAppendStringBuilder2(&stringBuilder, PhTranslateTextZ(L"函数表, "));
    if (GuardFlags & IMAGE_GUARD_SECURITY_COOKIE_UNUSED)
        PhAppendStringBuilder2(&stringBuilder, PhTranslateTextZ(L"未使用的安全 Cookie, "));
    if (GuardFlags & IMAGE_GUARD_PROTECT_DELAYLOAD_IAT)
        PhAppendStringBuilder2(&stringBuilder, PhTranslateTextZ(L"延迟加载 IAT 受保护, "));
    if (GuardFlags & IMAGE_GUARD_DELAYLOAD_IAT_IN_ITS_OWN_SECTION)
        PhAppendStringBuilder2(&stringBuilder, PhTranslateTextZ(L"延迟加载专用节区, "));
    if (GuardFlags & IMAGE_GUARD_CF_ENABLE_EXPORT_SUPPRESSION)
        PhAppendStringBuilder2(&stringBuilder, PhTranslateTextZ(L"导出抑制, "));
    if (GuardFlags & IMAGE_GUARD_CF_EXPORT_SUPPRESSION_INFO_PRESENT)
        PhAppendStringBuilder2(&stringBuilder, PhTranslateTextZ(L"导出信息抑制, "));
    if (GuardFlags & IMAGE_GUARD_CF_LONGJUMP_TABLE_PRESENT)
        PhAppendStringBuilder2(&stringBuilder, PhTranslateTextZ(L"长跳转表, "));
    if (GuardFlags & IMAGE_GUARD_RETPOLINE_PRESENT)
        PhAppendStringBuilder2(&stringBuilder, PhTranslateTextZ(L"存在 Retpoline, "));
    if (GuardFlags & IMAGE_GUARD_EH_CONTINUATION_TABLE_PRESENT_V1 || GuardFlags & IMAGE_GUARD_EH_CONTINUATION_TABLE_PRESENT_V2)
        PhAppendStringBuilder2(&stringBuilder, PhTranslateTextZ(L"EH 续延表, "));
    if (GuardFlags & IMAGE_GUARD_XFG_ENABLED)
        PhAppendStringBuilder2(&stringBuilder, L"XFG, ");
    if (GuardFlags & IMAGE_GUARD_CASTGUARD_PRESENT)
        PhAppendStringBuilder2(&stringBuilder, PhTranslateTextZ(L"Cast 防护, "));
    if (GuardFlags & IMAGE_GUARD_MEMCPY_PRESENT)
        PhAppendStringBuilder2(&stringBuilder, PhTranslateTextZ(L"受防护的 memcpy, "));

    if (PhEndsWithString2(stringBuilder.String, L", ", FALSE))
        PhRemoveEndStringBuilder(&stringBuilder, 2);

    PhPrintPointer(pointer, UlongToPtr(GuardFlags));
    PhAppendFormatStringBuilder(&stringBuilder, L" (%s)", pointer);

    return PhFinalStringBuilderString(&stringBuilder);
}

PPH_STRING PvpGetPeDependentLoadFlagsText(
    _In_ ULONG DependentLoadFlags
    )
{
    PH_STRING_BUILDER stringBuilder;
    WCHAR pointer[PH_PTR_STR_LEN_1];

    if (DependentLoadFlags == 0)
        return PhCreateString(L"0x0");

    PhInitializeStringBuilder(&stringBuilder, 10);

    if (DependentLoadFlags & DONT_RESOLVE_DLL_REFERENCES)
        PhAppendStringBuilder2(&stringBuilder, PhTranslateTextZ(L"忽略 DLL 引用, "));
    if (DependentLoadFlags & LOAD_LIBRARY_AS_DATAFILE)
        PhAppendStringBuilder2(&stringBuilder, PhTranslateTextZ(L"数据文件, "));
    if (DependentLoadFlags & 0x00000004) // LOAD_PACKAGED_LIBRARY
        PhAppendStringBuilder2(&stringBuilder, PhTranslateTextZ(L"打包库, "));
    if (DependentLoadFlags & LOAD_WITH_ALTERED_SEARCH_PATH)
        PhAppendStringBuilder2(&stringBuilder, PhTranslateTextZ(L"更改的搜索路径, "));
    if (DependentLoadFlags & LOAD_IGNORE_CODE_AUTHZ_LEVEL)
        PhAppendStringBuilder2(&stringBuilder, PhTranslateTextZ(L"忽略 Authz 级别, "));
    if (DependentLoadFlags & LOAD_LIBRARY_AS_IMAGE_RESOURCE)
        PhAppendStringBuilder2(&stringBuilder, PhTranslateTextZ(L"映像资源, "));
    if (DependentLoadFlags & LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE)
        PhAppendStringBuilder2(&stringBuilder, PhTranslateTextZ(L"数据文件（独占）, "));
    if (DependentLoadFlags & LOAD_LIBRARY_REQUIRE_SIGNED_TARGET)
        PhAppendStringBuilder2(&stringBuilder, PhTranslateTextZ(L"要求已签名目标, "));
    if (DependentLoadFlags & LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR)
        PhAppendStringBuilder2(&stringBuilder, PhTranslateTextZ(L"搜索 DLL 加载目录, "));
    if (DependentLoadFlags & LOAD_LIBRARY_SEARCH_APPLICATION_DIR)
        PhAppendStringBuilder2(&stringBuilder, PhTranslateTextZ(L"搜索应用程序目录, "));
    if (DependentLoadFlags & LOAD_LIBRARY_SEARCH_USER_DIRS)
        PhAppendStringBuilder2(&stringBuilder, PhTranslateTextZ(L"搜索用户目录, "));
    if (DependentLoadFlags & LOAD_LIBRARY_SEARCH_SYSTEM32)
        PhAppendStringBuilder2(&stringBuilder, PhTranslateTextZ(L"搜索 system32, "));
    if (DependentLoadFlags & LOAD_LIBRARY_SEARCH_DEFAULT_DIRS)
        PhAppendStringBuilder2(&stringBuilder, PhTranslateTextZ(L"搜索默认目录, "));
    if (DependentLoadFlags & LOAD_LIBRARY_SAFE_CURRENT_DIRS)
        PhAppendStringBuilder2(&stringBuilder, PhTranslateTextZ(L"搜索安全的当前目录, "));
    if (DependentLoadFlags & LOAD_LIBRARY_SEARCH_SYSTEM32_NO_FORWARDER)
        PhAppendStringBuilder2(&stringBuilder, PhTranslateTextZ(L"搜索 system32（无转发）, "));
    if (DependentLoadFlags & LOAD_LIBRARY_OS_INTEGRITY_CONTINUITY)
        PhAppendStringBuilder2(&stringBuilder, PhTranslateTextZ(L"OS 完整性连续性, "));

    if (PhEndsWithString2(stringBuilder.String, L", ", FALSE))
        PhRemoveEndStringBuilder(&stringBuilder, 2);

    PhPrintPointer(pointer, UlongToPtr(DependentLoadFlags));
    PhAppendFormatStringBuilder(&stringBuilder, L" (%s)", pointer);

    return PhFinalStringBuilderString(&stringBuilder);
}

PPH_STRING PvpGetPeEnclaveImportsText(
    _In_ PPH_MAPPED_IMAGE_ENCLAVE_CONFIG EnclaveConfig
    )
{
    PH_STRING_BUILDER stringBuilder;
    ULONG i;

    PhInitializeStringBuilder(&stringBuilder, 10);

    for (i = 0; i < EnclaveConfig->NumberOfImports; i++)
    {
        PIMAGE_ENCLAVE_IMPORT enclaveImport = &EnclaveConfig->Imports[i];
        PCSTR importName;

        if (enclaveImport->ImportName == USHRT_MAX)
            break;

        if (NT_SUCCESS(PhMappedImageRvaToVa(&PvMappedImage, enclaveImport->ImportName, &importName)))
        {
            PhAppendFormatStringBuilder(&stringBuilder, L"%hs, ", importName);
        }
    }

    if (PhEndsWithString2(stringBuilder.String, L", ", FALSE))
        PhRemoveEndStringBuilder(&stringBuilder, 2);

    return PhFinalStringBuilderString(&stringBuilder);
}

VOID PvpAddPeEnclaveConfig(
    _In_ HWND lvHandle
    )
{
    PH_MAPPED_IMAGE_ENCLAVE_CONFIG enclaveConfigInfo;

    if (!NT_SUCCESS(PhGetMappedImageEnclaveConfig(&PvMappedImage, &enclaveConfigInfo)))
        return;

    if (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
    {
        PIMAGE_ENCLAVE_CONFIG32 enclaveConfig = enclaveConfigInfo.EnclaveConfig;

        ADD_VALUE(L"Enclave 策略标志", PhaFormatUInt64(enclaveConfig->PolicyFlags, TRUE)->Buffer);
        ADD_VALUE(L"Enclave 家族 ID", PH_AUTO_T(PH_STRING, PhFormatGuid((PGUID)enclaveConfig->FamilyID))->Buffer);
        ADD_VALUE(L"Enclave 映像 ID", PH_AUTO_T(PH_STRING, PhFormatGuid((PGUID)enclaveConfig->ImageID))->Buffer);
        ADD_VALUE(L"Enclave 映像版本", PhaFormatUInt64(enclaveConfig->ImageVersion, TRUE)->Buffer);
        ADD_VALUE(L"Enclave 安全版本", PhaFormatUInt64(enclaveConfig->SecurityVersion, TRUE)->Buffer);
        ADD_VALUE(L"Enclave 大小 ", PhaFormatUInt64(enclaveConfig->EnclaveSize, TRUE)->Buffer);
        ADD_VALUE(L"Enclave 线程数", PhaFormatUInt64(enclaveConfig->NumberOfThreads, TRUE)->Buffer);
        ADD_VALUE(L"Enclave 标志", PhaFormatUInt64(enclaveConfig->EnclaveFlags, TRUE)->Buffer);
    }
    else
    {
        PIMAGE_ENCLAVE_CONFIG64 enclaveConfig = enclaveConfigInfo.EnclaveConfig;

        ADD_VALUE(L"Enclave 策略标志", PhaFormatUInt64(enclaveConfig->PolicyFlags, TRUE)->Buffer);
        ADD_VALUE(L"Enclave 家族 ID", PH_AUTO_T(PH_STRING, PhFormatGuid((PGUID)enclaveConfig->FamilyID))->Buffer);
        ADD_VALUE(L"Enclave 映像 ID", PH_AUTO_T(PH_STRING, PhFormatGuid((PGUID)enclaveConfig->ImageID))->Buffer);
        ADD_VALUE(L"Enclave 映像版本", PhaFormatUInt64(enclaveConfig->ImageVersion, TRUE)->Buffer);
        ADD_VALUE(L"Enclave 安全版本", PhaFormatUInt64(enclaveConfig->SecurityVersion, TRUE)->Buffer);
        ADD_VALUE(L"Enclave 大小 ", PhaFormatUInt64(enclaveConfig->EnclaveSize, TRUE)->Buffer);
        ADD_VALUE(L"Enclave 线程数", PhaFormatUInt64(enclaveConfig->NumberOfThreads, TRUE)->Buffer);
        ADD_VALUE(L"Enclave 标志", PhaFormatUInt64(enclaveConfig->EnclaveFlags, TRUE)->Buffer);
    }

    ADD_VALUE(L"Enclave 导入数", PhaFormatUInt64(enclaveConfigInfo.NumberOfImports, TRUE)->Buffer);
    ADD_VALUE(L"Enclave 导入", PH_AUTO_T(PH_STRING, PvpGetPeEnclaveImportsText(&enclaveConfigInfo))->Buffer);
}

VOID PvpAddPeLockPrefixTable(
    _In_ HWND lvHandle
    )
{
    PH_MAPPED_IMAGE_LOCK_PREFIX lockPrefix;
    ULONG i;

    if (!NT_SUCCESS(PhGetMappedImageLockPrefixTable(&PvMappedImage, &lockPrefix)))
        return;

    for (i = 0; i < lockPrefix.NumberOfEntries; i++)
    {
        ADD_VALUE(
            PhaFormatString(PhTranslateTextZ(L"Lock 前缀 %lu"), i)->Buffer,
            PhaFormatString(L"0x%I64x", lockPrefix.Entries[i])->Buffer
            );
    }

    if (lockPrefix.Entries)
        PhFree(lockPrefix.Entries);
}

INT_PTR CALLBACK PvPeLoadConfigDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPV_PE_LOADCONFIG_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(PV_PE_LOADCONFIG_CONTEXT));
        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);

        if (lParam)
        {
            LPPROPSHEETPAGE propSheetPage = (LPPROPSHEETPAGE)lParam;
            context->PropSheetContext = (PPV_PROPPAGECONTEXT)propSheetPage->lParam;
        }
    }
    else
    {
        context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PIMAGE_LOAD_CONFIG_DIRECTORY32 config32;
            PIMAGE_LOAD_CONFIG_DIRECTORY64 config64;
            HWND lvHandle;

            context->WindowHandle = hwndDlg;
            context->ListViewHandle = lvHandle = GetDlgItem(hwndDlg, IDC_LIST);

            PhSetListViewStyle(context->ListViewHandle, TRUE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PvConfigListViewFont(hwndDlg, context->ListViewHandle);
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 220, L"名称");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 170, L"值");
            PhSetExtendedListView(context->ListViewHandle);
            PhLoadListViewColumnsFromSetting(L"ImageLoadCfgListViewColumns", context->ListViewHandle);
            PvConfigTreeBorders(context->ListViewHandle);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);

            #define ADD_VALUES(Type, Config) \
            { \
                LARGE_INTEGER time; \
                SYSTEMTIME systemTime; \
                \
                PhSecondsSince1970ToTime((Config)->TimeDateStamp, &time); \
                PhLargeIntegerToLocalSystemTime(&systemTime, &time); \
                \
                ADD_VALUE(L"时间戳", (Config)->TimeDateStamp ? PhaFormatDateTime(&systemTime)->Buffer : L"0"); \
                ADD_VALUE(L"版本", PhaFormatString(L"%u.%u", (Config)->MajorVersion, (Config)->MinorVersion)->Buffer); \
                ADD_VALUE(L"要清除的全局标志", PhaFormatString(L"0x%x", (Config)->GlobalFlagsClear)->Buffer); \
                ADD_VALUE(L"要设置的全局标志", PhaFormatString(L"0x%x", (Config)->GlobalFlagsSet)->Buffer); \
                ADD_VALUE(L"临界区默认超时", PhaFormatUInt64((Config)->CriticalSectionDefaultTimeout, TRUE)->Buffer); \
                ADD_VALUE(L"解除提交空闲块阈值", PhaFormatUInt64((Config)->DeCommitFreeBlockThreshold, TRUE)->Buffer); \
                ADD_VALUE(L"解除提交总空闲阈值", PhaFormatUInt64((Config)->DeCommitTotalFreeThreshold, TRUE)->Buffer); \
                ADD_VALUE(L"Lock 前缀表", PhaFormatString(L"0x%x", (Config)->LockPrefixTable)->Buffer); \
                ADD_VALUE(L"最大分配大小", PhaFormatString(L"0x%Ix", (Config)->MaximumAllocationSize)->Buffer); \
                ADD_VALUE(L"虚拟内存阈值", PhaFormatString(L"0x%Ix", (Config)->VirtualMemoryThreshold)->Buffer); \
                ADD_VALUE(L"进程堆标志", PhaFormatString(L"0x%Ix", (Config)->ProcessHeapFlags)->Buffer); \
                ADD_VALUE(L"进程关联掩码", PhaFormatString(L"0x%Ix", (Config)->ProcessAffinityMask)->Buffer); \
                ADD_VALUE(L"CSD 版本", PhaFormatString(L"%u", (Config)->CSDVersion)->Buffer); \
                ADD_VALUE(L"依赖加载标志", PH_AUTO_T(PH_STRING, PvpGetPeDependentLoadFlagsText((Config)->DependentLoadFlags))->Buffer); \
                ADD_VALUE(L"编辑列表", PhaFormatString(L"0x%Ix", (Config)->EditList)->Buffer); \
                ADD_VALUE(L"安全 Cookie", PhaFormatString(L"0x%Ix", (Config)->SecurityCookie)->Buffer); \
                ADD_VALUE(L"SEH 处理程序表", PhaFormatString(L"0x%Ix", (Config)->SEHandlerTable)->Buffer); \
                ADD_VALUE(L"SEH 处理程序数", PhaFormatUInt64((Config)->SEHandlerCount, TRUE)->Buffer); \
                \
                if (RTL_CONTAINS_FIELD((Config), (Config)->Size, GuardCFCheckFunctionPointer)) \
                { \
                    ADD_VALUE(L"CFG 防护标志", PH_AUTO_T(PH_STRING, PvpGetPeGuardFlagsText((Config)->GuardFlags))->Buffer); \
                    ADD_VALUE(L"CFG 检查函数指针", PhaFormatString(L"0x%Ix", (Config)->GuardCFCheckFunctionPointer)->Buffer); \
                    ADD_VALUE(L"CFG 调度函数指针", PhaFormatString(L"0x%Ix", (Config)->GuardCFDispatchFunctionPointer)->Buffer); \
                    ADD_VALUE(L"CFG 函数表", PhaFormatString(L"0x%Ix", (Config)->GuardCFFunctionTable)->Buffer); \
                    ADD_VALUE(L"CFG 函数表计数", PhaFormatUInt64((Config)->GuardCFFunctionCount, TRUE)->Buffer); \
                    if (RTL_CONTAINS_FIELD((Config), (Config)->Size, GuardAddressTakenIatEntryTable)) \
                    { \
                        ADD_VALUE(L"CFG IatEntry 表", PhaFormatString(L"0x%Ix", (Config)->GuardAddressTakenIatEntryTable)->Buffer); \
                        ADD_VALUE(L"CFG IatEntry 表项数", PhaFormatUInt64((Config)->GuardAddressTakenIatEntryCount, TRUE)->Buffer); \
                    } \
                    if (RTL_CONTAINS_FIELD((Config), (Config)->Size, GuardLongJumpTargetTable)) \
                    { \
                        ADD_VALUE(L"CFG LongJump 表", PhaFormatString(L"0x%Ix", (Config)->GuardLongJumpTargetTable)->Buffer); \
                        ADD_VALUE(L"CFG LongJump 表项数", PhaFormatUInt64((Config)->GuardLongJumpTargetCount, TRUE)->Buffer); \
                    } \
                } \
                \
                if (RTL_CONTAINS_FIELD((Config), (Config)->Size, CodeIntegrity)) \
                { \
                    ADD_VALUE(L"CI 标志", PhaFormatString(L"0x%x", (Config)->CodeIntegrity.Flags)->Buffer); \
                    ADD_VALUE(L"CI 目录", PhaFormatString(L"0x%Ix", (Config)->CodeIntegrity.Catalog)->Buffer); \
                    ADD_VALUE(L"CI 目录偏移", PhaFormatString(L"0x%Ix", (Config)->CodeIntegrity.CatalogOffset)->Buffer); \
                } \
                \
                if (RTL_CONTAINS_FIELD((Config), (Config)->Size, DynamicValueRelocTable)) \
                { \
                    ADD_VALUE(L"DynamicValue 重定位表", PhaFormatString(L"0x%Ix", (Config)->DynamicValueRelocTable)->Buffer); \
                    ADD_VALUE(L"混合元数据指针", PhaFormatString(L"0x%Ix", (Config)->CHPEMetadataPointer)->Buffer); \
                    ADD_VALUE(L"GuardRF 失败函数例程", PhaFormatString(L"0x%Ix", (Config)->GuardRFFailureRoutine)->Buffer); \
                    ADD_VALUE(L"GuardRF 失败函数指针", PhaFormatString(L"0x%Ix", (Config)->GuardRFFailureRoutineFunctionPointer)->Buffer); \
                } \
                \
                if (RTL_CONTAINS_FIELD((Config), (Config)->Size, DynamicValueRelocTableOffset)) \
                { \
                    ADD_VALUE(L"DynamicValue 重定位表偏移", PhaFormatString(L"0x%Ix", (Config)->DynamicValueRelocTableOffset)->Buffer); \
                    ADD_VALUE(L"DynamicValue 重定位节区", PhaFormatString(L"%u", (Config)->DynamicValueRelocTableSection)->Buffer); \
                    ADD_VALUE(L"GuardRF 验证栈指针", PhaFormatString(L"0x%Ix", (Config)->GuardRFVerifyStackPointerFunctionPointer)->Buffer); \
                    ADD_VALUE(L"热修补表偏移", PhaFormatString(L"0x%Ix", (Config)->HotPatchTableOffset)->Buffer); \
                    ADD_VALUE(L"Enclave 配置指针", PhaFormatString(L"0x%Ix", (Config)->EnclaveConfigurationPointer)->Buffer); \
                } \
                \
                if (RTL_CONTAINS_FIELD((Config), (Config)->Size, VolatileMetadataPointer)) \
                { \
                    ADD_VALUE(L"可变元数据指针", PhaFormatString(L"0x%Ix", (Config)->VolatileMetadataPointer)->Buffer); \
                } \
                if (RTL_CONTAINS_FIELD((Config), (Config)->Size, GuardEHContinuationTable)) \
                { \
                    ADD_VALUE(L"Guard EH 续延表", PhaFormatString(L"0x%Ix", (Config)->GuardEHContinuationTable)->Buffer); \
                    ADD_VALUE(L"Guard EH 续延表项数", PhaFormatUInt64((Config)->GuardEHContinuationCount, TRUE)->Buffer); \
                } \
            }

            #define ADD_VALUES2(Type, Config) \
            { \
                if (RTL_CONTAINS_FIELD((Config), (Config)->Size, GuardXFGCheckFunctionPointer)) \
                { \
                    ADD_VALUE(L"XFG 检查函数指针", PhaFormatString(L"0x%Ix", (Config)->GuardXFGCheckFunctionPointer)->Buffer); \
                    ADD_VALUE(L"XFG 调度函数指针", PhaFormatString(L"0x%Ix", (Config)->GuardXFGDispatchFunctionPointer)->Buffer); \
                    ADD_VALUE(L"XFG 表调度函数指针", PhaFormatString(L"0x%Ix", (Config)->GuardXFGTableDispatchFunctionPointer)->Buffer); \
                } \
                if (RTL_CONTAINS_FIELD((Config), (Config)->Size, CastGuardOsDeterminedFailureMode)) \
                { \
                    ADD_VALUE(L"Cast 防护失败模式", PhaFormatString(L"0x%Ix", (Config)->CastGuardOsDeterminedFailureMode)->Buffer); \
                } \
            }

            #define ADD_VALUES3(Type, Config) \
            { \
                if (RTL_CONTAINS_FIELD((Config), (Config)->Size, GuardMemcpyFunctionPointer)) \
                { \
                    ADD_VALUE(L"Guard memcpy 函数指针", PhaFormatString(L"0x%Ix", (Config)->GuardMemcpyFunctionPointer)->Buffer); \
                } \
            }

            if (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
            {
                if (NT_SUCCESS(PhGetMappedImageLoadConfig32(&PvMappedImage, &config32)))
                {
                    ADD_VALUES(IMAGE_LOAD_CONFIG_DIRECTORY32, config32);
                #if defined(NTDDI_WIN10_CO) && (NTDDI_VERSION >= NTDDI_WIN10_CO)
                    ADD_VALUES2(IMAGE_LOAD_CONFIG_DIRECTORY32, config32);
                #endif
                #if defined(NTDDI_WIN10_NI) && (NTDDI_VERSION >= NTDDI_WIN10_NI)
                    ADD_VALUES3(IMAGE_LOAD_CONFIG_DIRECTORY32, config32);
                #endif
                    PvpAddPeEnclaveConfig(context->ListViewHandle);
                    PvpAddPeLockPrefixTable(context->ListViewHandle);
                }
            }
            else
            {
                if (NT_SUCCESS(PhGetMappedImageLoadConfig64(&PvMappedImage, &config64)))
                {
                    ADD_VALUES(IMAGE_LOAD_CONFIG_DIRECTORY64, config64);
                #if defined(NTDDI_WIN10_CO) && (NTDDI_VERSION >= NTDDI_WIN10_CO)
                    ADD_VALUES2(IMAGE_LOAD_CONFIG_DIRECTORY64, config64);
                #endif
                #if defined(NTDDI_WIN10_NI) && (NTDDI_VERSION >= NTDDI_WIN10_NI)
                    ADD_VALUES3(IMAGE_LOAD_CONFIG_DIRECTORY64, config64);
                #endif
                    PvpAddPeEnclaveConfig(context->ListViewHandle);
                    PvpAddPeLockPrefixTable(context->ListViewHandle);
                }
            }

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(L"ImageLoadCfgListViewColumns", context->ListViewHandle);
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
            PhFree(context);
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (context->PropSheetContext && !context->PropSheetContext->LayoutInitialized)
            {
                PvAddPropPageLayoutItem(hwndDlg, hwndDlg, PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);
                PvDoPropPageLayout(hwndDlg);

                context->PropSheetContext->LayoutInitialized = TRUE;
            }
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_NOTIFY:
        {
            PvHandleListViewNotifyForCopy(lParam, context->ListViewHandle);
        }
        break;
    case WM_CONTEXTMENU:
        {
            PvHandleListViewCommandCopy(hwndDlg, lParam, wParam, context->ListViewHandle);
        }
        break;
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORLISTBOX:
        {
            SetBkMode((HDC)wParam, TRANSPARENT);
            SetTextColor((HDC)wParam, RGB(0, 0, 0));
            SetDCBrushColor((HDC)wParam, RGB(255, 255, 255));
            return (INT_PTR)PhGetStockBrush(DC_BRUSH);
        }
        break;
    }

    return FALSE;
}
