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

#include <phapp.h>
#include <phplug.h>
#include <emenu.h>
#include <settings.h>
#include <phsettings.h>
#include <actions.h>
#include <phsvccl.h>
#include <kphuser.h>
#include <ksisup.h>

#define MSG_UPDATE (WM_APP + 1)

INT_PTR CALLBACK PhpMemoryListsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

HWND PhMemoryListsWindowHandle = NULL;
static VOID (NTAPI *UnregisterDialogFunction)(HWND);
static PH_CALLBACK_REGISTRATION ProcessesUpdatedRegistration;

VOID PhShowMemoryListsDialog(
    _In_ HWND ParentWindowHandle,
    _In_opt_ VOID (NTAPI *RegisterDialog)(HWND),
    _In_opt_ VOID (NTAPI *UnregisterDialog)(HWND)
    )
{
    if (!PhMemoryListsWindowHandle)
    {
        PhMemoryListsWindowHandle = PhCreateDialog(
            PhInstanceHandle,
            MAKEINTRESOURCE(IDD_MEMLISTS),
            ParentWindowHandle,
            PhpMemoryListsDlgProc,
            NULL
            );

        if (RegisterDialog)
            RegisterDialog(PhMemoryListsWindowHandle);
        UnregisterDialogFunction = UnregisterDialog;
    }

    if (!IsWindowVisible(PhMemoryListsWindowHandle))
        ShowWindow(PhMemoryListsWindowHandle, SW_SHOW);
    else if (IsMinimized(PhMemoryListsWindowHandle))
        ShowWindow(PhMemoryListsWindowHandle, SW_RESTORE);
    else
        SetForegroundWindow(PhMemoryListsWindowHandle);
}

_Function_class_(PH_CALLBACK_FUNCTION)
static VOID NTAPI ProcessesUpdatedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    if (PhMemoryListsWindowHandle)
    {
        PostMessage(PhMemoryListsWindowHandle, MSG_UPDATE, 0, 0);
    }
}

static VOID PhpUpdateMemoryListInfo(
    _In_ HWND hwndDlg
    )
{
    SYSTEM_MEMORY_LIST_INFORMATION memoryListInfo;

    if (NT_SUCCESS(NtQuerySystemInformation(
        SystemMemoryListInformation,
        &memoryListInfo,
        sizeof(SYSTEM_MEMORY_LIST_INFORMATION),
        NULL
        )))
    {
        ULONG_PTR standbyPageCount;
        ULONG_PTR repurposedPageCount;
        ULONG i;

        standbyPageCount = 0;
        repurposedPageCount = 0;

        for (i = 0; i < 8; i++)
        {
            standbyPageCount += memoryListInfo.PageCountByPriority[i];
            repurposedPageCount += memoryListInfo.RepurposedPagesByPriority[i];
        }

        PhSetDialogItemText(hwndDlg, IDC_ZLISTZEROED_V, PhaFormatSize((ULONG64)memoryListInfo.ZeroPageCount * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(hwndDlg, IDC_ZLISTFREE_V, PhaFormatSize((ULONG64)memoryListInfo.FreePageCount * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(hwndDlg, IDC_ZLISTMODIFIED_V, PhaFormatSize((ULONG64)memoryListInfo.ModifiedPageCount * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(hwndDlg, IDC_ZLISTMODIFIEDNOWRITE_V, PhaFormatSize((ULONG64)memoryListInfo.ModifiedNoWritePageCount * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(hwndDlg, IDC_ZLISTBAD_V, PhaFormatSize((ULONG64)memoryListInfo.BadPageCount * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(hwndDlg, IDC_ZLISTSTANDBY_V, PhaFormatSize((ULONG64)standbyPageCount * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(hwndDlg, IDC_ZLISTSTANDBY0_V, PhaFormatSize((ULONG64)memoryListInfo.PageCountByPriority[0] * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(hwndDlg, IDC_ZLISTSTANDBY1_V, PhaFormatSize((ULONG64)memoryListInfo.PageCountByPriority[1] * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(hwndDlg, IDC_ZLISTSTANDBY2_V, PhaFormatSize((ULONG64)memoryListInfo.PageCountByPriority[2] * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(hwndDlg, IDC_ZLISTSTANDBY3_V, PhaFormatSize((ULONG64)memoryListInfo.PageCountByPriority[3] * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(hwndDlg, IDC_ZLISTSTANDBY4_V, PhaFormatSize((ULONG64)memoryListInfo.PageCountByPriority[4] * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(hwndDlg, IDC_ZLISTSTANDBY5_V, PhaFormatSize((ULONG64)memoryListInfo.PageCountByPriority[5] * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(hwndDlg, IDC_ZLISTSTANDBY6_V, PhaFormatSize((ULONG64)memoryListInfo.PageCountByPriority[6] * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(hwndDlg, IDC_ZLISTSTANDBY7_V, PhaFormatSize((ULONG64)memoryListInfo.PageCountByPriority[7] * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(hwndDlg, IDC_ZLISTREPURPOSED_V, PhaFormatSize((ULONG64)repurposedPageCount * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(hwndDlg, IDC_ZLISTREPURPOSED0_V, PhaFormatSize((ULONG64)memoryListInfo.RepurposedPagesByPriority[0] * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(hwndDlg, IDC_ZLISTREPURPOSED1_V, PhaFormatSize((ULONG64)memoryListInfo.RepurposedPagesByPriority[1] * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(hwndDlg, IDC_ZLISTREPURPOSED2_V, PhaFormatSize((ULONG64)memoryListInfo.RepurposedPagesByPriority[2] * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(hwndDlg, IDC_ZLISTREPURPOSED3_V, PhaFormatSize((ULONG64)memoryListInfo.RepurposedPagesByPriority[3] * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(hwndDlg, IDC_ZLISTREPURPOSED4_V, PhaFormatSize((ULONG64)memoryListInfo.RepurposedPagesByPriority[4] * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(hwndDlg, IDC_ZLISTREPURPOSED5_V, PhaFormatSize((ULONG64)memoryListInfo.RepurposedPagesByPriority[5] * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(hwndDlg, IDC_ZLISTREPURPOSED6_V, PhaFormatSize((ULONG64)memoryListInfo.RepurposedPagesByPriority[6] * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(hwndDlg, IDC_ZLISTREPURPOSED7_V, PhaFormatSize((ULONG64)memoryListInfo.RepurposedPagesByPriority[7] * PAGE_SIZE, ULONG_MAX)->Buffer);

        if (WindowsVersion >= WINDOWS_8)
            PhSetDialogItemText(hwndDlg, IDC_ZLISTMODIFIEDPAGEFILE_V, PhaFormatSize((ULONG64)memoryListInfo.ModifiedPageCountPageFile * PAGE_SIZE, ULONG_MAX)->Buffer);
        else
            PhSetDialogItemText(hwndDlg, IDC_ZLISTMODIFIEDPAGEFILE_V, L"N/A");
    }
    else
    {
        PhSetDialogItemText(hwndDlg, IDC_ZLISTZEROED_V, L"未知");
        PhSetDialogItemText(hwndDlg, IDC_ZLISTFREE_V, L"未知");
        PhSetDialogItemText(hwndDlg, IDC_ZLISTMODIFIED_V, L"未知");
        PhSetDialogItemText(hwndDlg, IDC_ZLISTMODIFIEDNOWRITE_V, L"未知");
        PhSetDialogItemText(hwndDlg, IDC_ZLISTMODIFIEDPAGEFILE_V, L"未知");
        PhSetDialogItemText(hwndDlg, IDC_ZLISTBAD_V, L"未知");
        PhSetDialogItemText(hwndDlg, IDC_ZLISTSTANDBY_V, L"未知");
        PhSetDialogItemText(hwndDlg, IDC_ZLISTSTANDBY0_V, L"未知");
        PhSetDialogItemText(hwndDlg, IDC_ZLISTSTANDBY1_V, L"未知");
        PhSetDialogItemText(hwndDlg, IDC_ZLISTSTANDBY2_V, L"未知");
        PhSetDialogItemText(hwndDlg, IDC_ZLISTSTANDBY3_V, L"未知");
        PhSetDialogItemText(hwndDlg, IDC_ZLISTSTANDBY4_V, L"未知");
        PhSetDialogItemText(hwndDlg, IDC_ZLISTSTANDBY5_V, L"未知");
        PhSetDialogItemText(hwndDlg, IDC_ZLISTSTANDBY6_V, L"未知");
        PhSetDialogItemText(hwndDlg, IDC_ZLISTSTANDBY7_V, L"未知");
        PhSetDialogItemText(hwndDlg, IDC_ZLISTREPURPOSED_V, L"未知");
        PhSetDialogItemText(hwndDlg, IDC_ZLISTREPURPOSED0_V, L"未知");
        PhSetDialogItemText(hwndDlg, IDC_ZLISTREPURPOSED1_V, L"未知");
        PhSetDialogItemText(hwndDlg, IDC_ZLISTREPURPOSED2_V, L"未知");
        PhSetDialogItemText(hwndDlg, IDC_ZLISTREPURPOSED3_V, L"未知");
        PhSetDialogItemText(hwndDlg, IDC_ZLISTREPURPOSED4_V, L"未知");
        PhSetDialogItemText(hwndDlg, IDC_ZLISTREPURPOSED5_V, L"未知");
        PhSetDialogItemText(hwndDlg, IDC_ZLISTREPURPOSED6_V, L"未知");
        PhSetDialogItemText(hwndDlg, IDC_ZLISTREPURPOSED7_V, L"未知");
    }
}

typedef
_Function_class_(PH_MEMORY_LIST_COMMAND_CALLBACK)
VOID
NTAPI
PH_MEMORY_LIST_COMMAND_CALLBACK(
    _In_ HWND ParentWindow,
    _Out_ PPH_STRING* Message
    );
typedef PH_MEMORY_LIST_COMMAND_CALLBACK *PPH_MEMORY_LIST_COMMAND_CALLBACK;

PPH_STRING PhpCreateCommandStatusString(
    _In_ PCWSTR Message,
    _In_ NTSTATUS Status
    )
{
    PPH_STRING string;
    PPH_STRING statusString;

    if (statusString = PhGetStatusMessage(Status, 0))
    {
        PH_FORMAT format[3];

        PhInitFormatS(&format[0], Message);
        PhInitFormatC(&format[1], L' ');
        PhInitFormatSR(&format[2], statusString->sr);

        string = PhFormat(format, RTL_NUMBER_OF(format), 10);

        PhDereferenceObject(statusString);
    }
    else
    {
        string = PhCreateString(Message);
    }

    return string;
}

NTSTATUS PhpMemoryListCommandCommon(
    _In_ HWND ParentWindow,
    _In_ SYSTEM_MEMORY_LIST_COMMAND Command
    )
{
    NTSTATUS status;

    PhSetCursor(PhLoadCursor(NULL, IDC_WAIT));
    status = NtSetSystemInformation(
        SystemMemoryListInformation,
        &Command,
        sizeof(SYSTEM_MEMORY_LIST_COMMAND)
        );
    PhSetCursor(PhLoadCursor(NULL, IDC_ARROW));

    if (status == STATUS_PRIVILEGE_NOT_HELD)
    {
        if (!PhGetOwnTokenAttributes().Elevated)
        {
            if (PhUiConnectToPhSvc(ParentWindow, FALSE))
            {
                PhSetCursor(PhLoadCursor(NULL, IDC_WAIT));
                status = PhSvcCallIssueMemoryListCommand(Command);
                PhSetCursor(PhLoadCursor(NULL, IDC_ARROW));
                PhUiDisconnectFromPhSvc();
            }
            else
            {
                // User cancelled elevation.
                status = STATUS_SUCCESS;
            }
        }
    }

    return status;
}

_Function_class_(PH_MEMORY_LIST_COMMAND_CALLBACK)
VOID NTAPI PhpEmptyWorkingSetsCommand(
    _In_ HWND ParentWindow,
    _Out_ PPH_STRING* Message
    )
{
    NTSTATUS status;

    status = PhpMemoryListCommandCommon(ParentWindow, MemoryEmptyWorkingSets);

    if (NT_SUCCESS(status))
        *Message = PhCreateString(L"已清空工作集。");
    else
        *Message = PhpCreateCommandStatusString(L"无法清空工作集。", status);
}

_Function_class_(PH_MEMORY_LIST_COMMAND_CALLBACK)
VOID NTAPI PhpFlushModifiedListCommand(
    _In_ HWND ParentWindow,
    _Out_ PPH_STRING* Message
    )
{
    NTSTATUS status;

    status = PhpMemoryListCommandCommon(ParentWindow, MemoryFlushModifiedList);

    if (NT_SUCCESS(status))
        *Message = PhCreateString(L"已清空已修改页列表。");
    else
        *Message = PhpCreateCommandStatusString(L"无法清空已修改页列表。", status);
}

_Function_class_(PH_MEMORY_LIST_COMMAND_CALLBACK)
VOID NTAPI PhpPurgeStandbyListCommand(
    _In_ HWND ParentWindow,
    _Out_ PPH_STRING* Message
    )
{
    NTSTATUS status;

    status = PhpMemoryListCommandCommon(ParentWindow, MemoryPurgeStandbyList);

    if (NT_SUCCESS(status))
        *Message = PhCreateString(L"已清空备用列表。");
    else
        *Message = PhpCreateCommandStatusString(L"无法清空备用列表。", status);
}

_Function_class_(PH_MEMORY_LIST_COMMAND_CALLBACK)
VOID NTAPI PhpPurgeLowPriorityStandbyListCommand(
    _In_ HWND ParentWindow,
    _Out_ PPH_STRING* Message
    )
{
    NTSTATUS status;

    status = PhpMemoryListCommandCommon(ParentWindow, MemoryPurgeLowPriorityStandbyList);

    if (NT_SUCCESS(status))
        *Message = PhCreateString(L"已清空优先级 0 备用列表。");
    else
        *Message = PhpCreateCommandStatusString(L"无法清空优先级 0 备用列表。", status);
}

_Function_class_(PH_MEMORY_LIST_COMMAND_CALLBACK)
VOID NTAPI PhpCombineMemoryListsCommand(
    _In_ HWND ParentWindow,
    _Out_ PPH_STRING* Message
    )
{
    NTSTATUS status;
    MEMORY_COMBINE_INFORMATION_EX combineInfo = { 0 };

    PhSetCursor(PhLoadCursor(NULL, IDC_WAIT));
    status = NtSetSystemInformation(
        SystemCombinePhysicalMemoryInformation,
        &combineInfo,
        sizeof(MEMORY_COMBINE_INFORMATION_EX)
        );
    PhSetCursor(PhLoadCursor(NULL, IDC_ARROW));

    if (NT_SUCCESS(status))
    {
        PH_FORMAT format[5];

        // Memory pages combined: %s (%llu pages)
        PhInitFormatS(&format[0], L"已合并内存页：");
        PhInitFormatSize(&format[1], combineInfo.PagesCombined * PAGE_SIZE);
        PhInitFormatS(&format[2], L"（");
        PhInitFormatI64U(&format[3], combineInfo.PagesCombined);
        PhInitFormatS(&format[4], L" 页）");

        *Message = PhFormat(format, RTL_NUMBER_OF(format), 0);
    }
    else
    {
        *Message = PhpCreateCommandStatusString(L"无法合并内存页。", status);
    }
}

_Function_class_(PH_MEMORY_LIST_COMMAND_CALLBACK)
VOID NTAPI PhpEmptyCompressionStoreCommand(
    _In_ HWND ParentWindow,
    _Out_ PPH_STRING* Message
    )
{
    NTSTATUS status;

    if (KsiLevel() < KphLevelMax)
    {
        PhShowKsiNotConnected(
            ParentWindow,
            L"清空压缩存储需要连接到内核驱动程序。"
            );

        *Message = NULL;
        return;
    }

    PhSetCursor(PhLoadCursor(NULL, IDC_WAIT));
    status = KphSystemControl(
        KphSystemControlEmptyCompressionStore,
        NULL,
        0
        );
    PhSetCursor(PhLoadCursor(NULL, IDC_ARROW));

    if (NT_SUCCESS(status))
        *Message = PhCreateString(L"已清空压缩存储。");
    else
        *Message = PhpCreateCommandStatusString(L"无法清空压缩存储。", status);
}

_Function_class_(PH_MEMORY_LIST_COMMAND_CALLBACK)
VOID NTAPI PhpEmptyRegistryCacheCommand(
    _In_ HWND ParentWindow,
    _Out_ PPH_STRING* Message
    )
{
    NTSTATUS status;

    PhSetCursor(PhLoadCursor(NULL, IDC_WAIT));
    status = NtSetSystemInformation(SystemRegistryReconciliationInformation, NULL, 0);
    PhSetCursor(PhLoadCursor(NULL, IDC_ARROW));

    if (NT_SUCCESS(status))
        *Message = PhCreateString(L"已清空注册表缓存。");
    else
        *Message = PhpCreateCommandStatusString(L"无法清空注册表缓存。", status);
}

_Function_class_(PH_MEMORY_LIST_COMMAND_CALLBACK)
VOID NTAPI PhpEmptySystemFileCacheCommand(
    _In_ HWND ParentWindow,
    _Out_ PPH_STRING* Message
    )
{
    NTSTATUS status;
    SYSTEM_FILECACHE_INFORMATION cacheInfo = { 0 };

    PhAdjustPrivilege(NULL, SE_INCREASE_QUOTA_PRIVILEGE, TRUE);

    PhGetSystemFileCacheSize(&cacheInfo);

    PhSetCursor(PhLoadCursor(NULL, IDC_WAIT));
    status = PhSetSystemFileCacheSize(
        MAXSIZE_T,
        MAXSIZE_T,
        0
        );
    PhSetCursor(PhLoadCursor(NULL, IDC_ARROW));

    if (NT_SUCCESS(status))
    {
        PH_FORMAT format[5];

        // System file cache emptied: %s (%llu pages)
        PhInitFormatS(&format[0], L"已清空系统文件缓存：");
        PhInitFormatSize(&format[1], cacheInfo.CurrentSize);
        PhInitFormatS(&format[2], L"（");
        PhInitFormatI64U(&format[3], cacheInfo.CurrentSize / PAGE_SIZE);
        PhInitFormatS(&format[4], L" 页）");

        *Message = PhFormat(format, RTL_NUMBER_OF(format), 0);
    }
    else
    {
        *Message = PhpCreateCommandStatusString(L"无法清空系统文件缓存。", status);
    }
}

_Function_class_(PH_MEMORY_LIST_COMMAND_CALLBACK)
VOID NTAPI PhpFlushModifiedFileCacheCommand(
    _In_ HWND ParentWindow,
    _Out_ PPH_STRING* Message
    )
{
    NTSTATUS status;

    PhSetCursor(PhLoadCursor(NULL, IDC_WAIT));
    status = PhFlushVolumeCache();
    PhSetCursor(PhLoadCursor(NULL, IDC_ARROW));

    if (NT_SUCCESS(status))
        *Message = PhCreateString(L"已将卷文件缓存刷新到磁盘。");
    else
        *Message = PhpCreateCommandStatusString(L"无法刷新卷文件缓存。", status);
}

typedef struct _PH_MEMORY_LIST_COMMAND_ENTRY
{
    PPH_MEMORY_LIST_COMMAND_CALLBACK Callback;
    PH_STRINGREF WorkingMessage;
} PH_MEMORY_LIST_COMMAND_ENTRY, *PPH_MEMORY_LIST_COMMAND_ENTRY;

static const PH_MEMORY_LIST_COMMAND_ENTRY PhMemoryListCommands[] =
{
    { PhpCombineMemoryListsCommand, PH_STRINGREF_INIT(L"正在合并内存页...") },
    { PhpEmptyCompressionStoreCommand, PH_STRINGREF_INIT(L"正在清空压缩存储...") },
    { PhpEmptySystemFileCacheCommand, PH_STRINGREF_INIT(L"正在清空系统文件缓存 ...") },
    { PhpEmptyRegistryCacheCommand, PH_STRINGREF_INIT(L"正在清空注册表缓存...") },
    { PhpEmptyWorkingSetsCommand, PH_STRINGREF_INIT(L"正在清空工作集...") },
    { PhpFlushModifiedListCommand, PH_STRINGREF_INIT(L"正在清空已修改页列表...") },
    { PhpFlushModifiedFileCacheCommand, PH_STRINGREF_INIT(L"正在清空已修改文件缓存...") },
    { PhpPurgeStandbyListCommand, PH_STRINGREF_INIT(L"正在清空备用列表...") },
    { PhpPurgeLowPriorityStandbyListCommand, PH_STRINGREF_INIT(L"正在清空优先级 0 备用列表...") },
};

typedef struct _PH_MEMORY_LIST_COMMAND_CONTEXT
{
    HWND WindowHandle;
    HANDLE ThreadHandle;
    BOOLEAN Cancel;
    BOOLEAN Completed;
    PH_QUEUED_LOCK MessageLock;
    PPH_STRING Message;
    ULONG CommandsComplete;
    ULONG CommandsCount;
    PH_MEMORY_LIST_COMMAND_ENTRY Commands[RTL_NUMBER_OF(PhMemoryListCommands)];
} PH_MEMORY_LIST_COMMAND_CONTEXT, *PPH_MEMORY_LIST_COMMAND_CONTEXT;

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS NTAPI PhMemoryListCommandThread(
    _In_ PVOID ThreadParameter
    )
{
    static const PH_STRINGREF lineBreak = PH_STRINGREF_INIT(L"\r\n");
    PPH_MEMORY_LIST_COMMAND_CONTEXT context = ThreadParameter;

    for (ULONG i = 0; i < context->CommandsCount; i++)
    {
        PPH_MEMORY_LIST_COMMAND_ENTRY entry;
        PPH_STRING message = NULL;

        entry = &context->Commands[i];

        if (context->Cancel)
            break;

        PhAcquireQueuedLockExclusive(&context->MessageLock);
        if (context->Message)
            PhMoveReference(&context->Message, PhConcatStringRef3(&context->Message->sr, &lineBreak, &entry->WorkingMessage));
        else
            context->Message = PhCreateString2(&entry->WorkingMessage);
        PhReleaseQueuedLockExclusive(&context->MessageLock);

        entry->Callback(context->WindowHandle, &message);
        context->CommandsComplete++;

        if (message)
        {
            PhAcquireQueuedLockExclusive(&context->MessageLock);
            PhMoveReference(&context->Message, PhConcatStringRef3(&context->Message->sr, &lineBreak, &message->sr));
            PhReleaseQueuedLockExclusive(&context->MessageLock);
            PhDereferenceObject(message);
        }
    }

    return STATUS_SUCCESS;
}

HRESULT CALLBACK PhMemoryListCommandDialogCallbackProc(
    _In_ HWND WindowHandle,
    _In_ UINT Notification,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ LONG_PTR Context
    )
{
    PPH_MEMORY_LIST_COMMAND_CONTEXT context = (PPH_MEMORY_LIST_COMMAND_CONTEXT)Context;

    switch (Notification)
    {
    case TDN_CREATED:
        {
            NTSTATUS status;
            HANDLE threadHandle;

            context->WindowHandle = WindowHandle;
            PhInitializeQueuedLock(&context->MessageLock);

            SendMessage(WindowHandle, TDM_ENABLE_BUTTON, IDCLOSE, FALSE);

            if (context->CommandsCount == 1)
            {
                SendMessage(WindowHandle, TDM_SET_MARQUEE_PROGRESS_BAR, TRUE, 0);
                SendMessage(WindowHandle, TDM_SET_PROGRESS_BAR_MARQUEE, TRUE, 1);
            }

            if (NT_SUCCESS(status = PhCreateThreadEx(
                &threadHandle,
                PhMemoryListCommandThread,
                context
                )))
            {
                context->ThreadHandle = threadHandle;
            }
            else
            {
                PhShowStatus(WindowHandle, L"无法创建窗口。", status, 0);
            }
        }
        break;
    case TDN_BUTTON_CLICKED:
        {
            if ((INT)wParam == IDCANCEL)
            {
                context->Cancel = TRUE;
                SendMessage(WindowHandle, TDM_ENABLE_BUTTON, IDCANCEL, FALSE);
            }
        }
        break;
    case TDN_TIMER:
        {
            PPH_STRING string;

            if (context->Completed)
                break;

            PhAcquireQueuedLockExclusive(&context->MessageLock);
            string = context->Message ? PhReferenceObject(context->Message) : NULL;
            PhReleaseQueuedLockExclusive(&context->MessageLock);

            if (string)
            {
                SendMessage(WindowHandle, TDM_SET_ELEMENT_TEXT, TDE_CONTENT, (LPARAM)string->Buffer);
                PhDereferenceObject(string);
            }

            if (context->CommandsComplete == context->CommandsCount)
            {
                if (context->CommandsCount == 1)
                {
                    SendMessage(WindowHandle, TDM_SET_MARQUEE_PROGRESS_BAR, FALSE, 0);
                    SendMessage(WindowHandle, TDM_SET_PROGRESS_BAR_MARQUEE, FALSE, 0);
                }

                SendMessage(WindowHandle, TDM_SET_PROGRESS_BAR_POS, (WPARAM)100, 0);

                SendMessage(WindowHandle, TDM_ENABLE_BUTTON, IDCLOSE, TRUE);
                SendMessage(WindowHandle, TDM_ENABLE_BUTTON, IDCANCEL, FALSE);

                context->Completed = TRUE;
            }
            else if (context->CommandsCount > 1)
            {
                FLOAT progress = (FLOAT)(context->CommandsComplete + 1) / context->CommandsCount;
                SendMessage(WindowHandle, TDM_SET_PROGRESS_BAR_POS, (WPARAM)(progress * 100), 0);
            }
        }
        break;
    }

    return S_OK;
}

VOID PhMemoryListCommandDialog(
    _In_ HWND PrentWindow,
    _In_ PPH_MEMORY_LIST_COMMAND_CONTEXT Context
    )
{
    TASKDIALOGCONFIG config;
    PPH_MEMORY_LIST_COMMAND_CONTEXT context;

    context = PhAllocateCopy(Context, sizeof(PH_MEMORY_LIST_COMMAND_CONTEXT));

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SHOW_PROGRESS_BAR | TDF_CAN_BE_MINIMIZED | TDF_CALLBACK_TIMER;
    config.dwCommonButtons = TDCBF_CLOSE_BUTTON | TDCBF_CANCEL_BUTTON;
    config.hMainIcon = PhGetApplicationIcon(FALSE, PhGetWindowDpi(PrentWindow));
    config.pfCallback = PhMemoryListCommandDialogCallbackProc;
    config.hwndParent = PrentWindow;
    config.lpCallbackData = (LONG_PTR)context;
    config.pszWindowTitle = PhApplicationName;
    if (context->CommandsCount > 1)
        config.pszMainInstruction = L"正在执行内存命令...";
    else
        config.pszMainInstruction = L"正在执行内存命令...";
    config.pszContent = L" ";
    config.cxWidth = 200;

    TaskDialogIndirect(&config, NULL, NULL, NULL);

    if (context->ThreadHandle)
        NtWaitForSingleObject(context->ThreadHandle, FALSE, NULL);

    PhClearReference(&context->Message);

    PhFree(context);
}

VOID PhShowMemoryListCommand(
    _In_ HWND ParentWindow,
    _In_ HWND ButtonWindow,
    _In_ BOOLEAN ShowTopAlign
    )
{
    PPH_EMENU menu;
    RECT buttonRect = { 0 };
    PPH_EMENU_ITEM selectedItem;

    menu = PhCreateEMenu();
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 0, L"合并内存页(&C)", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 1, L"清空压缩缓存(&C)", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 2, L"清空系统文件缓存(&F)", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 3, L"清空注册表缓存(&R)", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 4, L"清空工作集(&W)", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 5, L"清空已修改页列表(&M)", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 6, L"清空已修改文件缓存(&M)", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 7, L"清空备用列表(&S)", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 8, L"清空优先级 0 备用列表(&P)", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, RTL_NUMBER_OF(PhMemoryListCommands), L"全部清空(&A)", NULL, NULL), ULONG_MAX);

    if (ShowTopAlign)
    {
        PhGetWindowRect(ButtonWindow, &buttonRect);
        selectedItem = PhShowEMenu(
            menu,
            ParentWindow,
            PH_EMENU_SHOW_LEFTRIGHT,
            PH_ALIGN_LEFT | PH_ALIGN_BOTTOM,
            buttonRect.left,
            buttonRect.top
            );
    }
    else
    {
        POINT point;

        PhGetClientRect(ButtonWindow, &buttonRect);
        point.x = 0;
        point.y = buttonRect.bottom;
        ClientToScreen(ButtonWindow, &point);

        selectedItem = PhShowEMenu(
            menu,
            ParentWindow,
            PH_EMENU_SHOW_LEFTRIGHT,
            PH_ALIGN_LEFT | PH_ALIGN_TOP,
            point.x,
            point.y
            );
    }

    if (selectedItem)
    {
        PH_MEMORY_LIST_COMMAND_CONTEXT context;

        memset(&context, 0, sizeof(PH_MEMORY_LIST_COMMAND_CONTEXT));

        if (selectedItem->Id < RTL_NUMBER_OF(PhMemoryListCommands))
        {
            context.CommandsCount = 1;
            context.Commands[0] = PhMemoryListCommands[selectedItem->Id];
            PhMemoryListCommandDialog(ParentWindow, &context);
        }
        else if (!PhGetOwnTokenAttributes().Elevated)
        {
            PhShowStatus(ParentWindow, L"无法清空内存列表。", 0, ERROR_ELEVATION_REQUIRED);
        }
        else
        {
            context.CommandsCount = RTL_NUMBER_OF(PhMemoryListCommands);
            memcpy(
                context.Commands,
                PhMemoryListCommands,
                sizeof(PH_MEMORY_LIST_COMMAND_ENTRY) * context.CommandsCount
                );
            PhMemoryListCommandDialog(ParentWindow, &context);
        }
    }

    PhDestroyEMenu(menu);
}

INT_PTR CALLBACK PhpMemoryListsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PhAdjustPrivilege(NULL, SE_PROF_SINGLE_PROCESS_PRIVILEGE, TRUE);

            PhRegisterCallback(PhGetGeneralCallback(GeneralCallbackProcessProviderUpdatedEvent), ProcessesUpdatedCallback, NULL, &ProcessesUpdatedRegistration);
            PhpUpdateMemoryListInfo(hwndDlg);

            if (PhValidWindowPlacementFromSetting(SETTING_MEMORY_LISTS_WINDOW_POSITION))
                PhLoadWindowPlacementFromSetting(SETTING_MEMORY_LISTS_WINDOW_POSITION, NULL, hwndDlg);
            else
                PhCenterWindow(hwndDlg, GetParent(hwndDlg));
            PhRegisterDialog(hwndDlg);

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhUnregisterDialog(hwndDlg);
            PhSaveWindowPlacementToSetting(SETTING_MEMORY_LISTS_WINDOW_POSITION, NULL, hwndDlg);

            PhUnregisterCallback(PhGetGeneralCallback(GeneralCallbackProcessProviderUpdatedEvent), &ProcessesUpdatedRegistration);

            if (UnregisterDialogFunction)
                UnregisterDialogFunction(hwndDlg);
            PhMemoryListsWindowHandle = NULL;
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
            case IDOK:
                DestroyWindow(hwndDlg);
                break;
            case IDC_EMPTY:
                {
                    PhShowMemoryListCommand(hwndDlg, GET_WM_COMMAND_HWND(wParam, lParam), FALSE);
                }
                break;
            }
        }
        break;
    case MSG_UPDATE:
        {
            PhpUpdateMemoryListInfo(hwndDlg);
        }
        break;
    case WM_CTLCOLORBTN:
        return HANDLE_WM_CTLCOLORBTN(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        return HANDLE_WM_CTLCOLORSTATIC(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    }

    return FALSE;
}
