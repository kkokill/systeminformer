/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2015
 *     dmex    2020-2026
 *
 */

/*
 * The affinity dialog was originally created to support the modification
 * of process affinity masks, but now supports modifying thread affinity
 * and generic masks.
 */

#include <phapp.h>
#include <procprv.h>
#include <thrdprv.h>

#define PHP_AFFINITY_SELECT_PERFCORES 0
#define PHP_AFFINITY_SELECT_ECORES 1
#define PHP_AFFINITY_SELECT_LPECORES 2
#define PHP_AFFINITY_SELECT_HTCORES 3

typedef struct _PH_AFFINITY_CPU_INFO
{
    BOOLEAN Present;
    BOOLEAN SmT;
    UCHAR EfficiencyClass;
    ULONG CoreNumber;
    USHORT Group;
} PH_AFFINITY_CPU_INFO, *PPH_AFFINITY_CPU_INFO;

typedef struct _PH_AFFINITY_DIALOG_CONTEXT
{
    HWND WindowHandle;
    HWND GroupComboHandle;

    PPH_PROCESS_ITEM ProcessItem;
    PPH_THREAD_ITEM ThreadItem;
    KAFFINITY NewAffinityMask;

    PPH_LIST CpuControlList;
    USHORT AffinityGroup;
    KAFFINITY AffinityMask;
    KAFFINITY SystemAffinityMask;

    // Processor core topology
    PPH_AFFINITY_CPU_INFO* CoreInfoGroups;
    USHORT NumberOfCoreInfoGroups;
    UCHAR MinimumEfficiencyClass;
    UCHAR MaximumEfficiencyClass;
    BOOLEAN HasMidEfficiencyClass;
    BOOLEAN HasSmtCore;
    HWND TooltipHandle;
    PPH_LIST TooltipStringList;

    // Multiple selected items (dmex)
    PPH_THREAD_ITEM* Threads;
    ULONG NumberOfThreads;
    PHANDLE ThreadHandles;
} PH_AFFINITY_DIALOG_CONTEXT, *PPH_AFFINITY_DIALOG_CONTEXT;

INT_PTR CALLBACK PhpProcessAffinityDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

VOID PhShowProcessAffinityDialog(
    _In_ HWND ParentWindowHandle,
    _In_opt_ PPH_PROCESS_ITEM ProcessItem,
    _In_opt_ PPH_THREAD_ITEM ThreadItem
    )
{
    PH_AFFINITY_DIALOG_CONTEXT context;

    assert(!!ProcessItem != !!ThreadItem); // make sure we have one and not the other (wj32)

    memset(&context, 0, sizeof(PH_AFFINITY_DIALOG_CONTEXT));
    context.ProcessItem = ProcessItem;
    context.ThreadItem = ThreadItem;

    PhDialogBox(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_AFFINITY),
        ParentWindowHandle,
        PhpProcessAffinityDlgProc,
        &context
        );
}

_Success_(return)
BOOLEAN PhShowProcessAffinityDialog2(
    _In_ HWND ParentWindowHandle,
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _Out_ PKAFFINITY NewAffinityMask
    )
{
    PH_AFFINITY_DIALOG_CONTEXT context;

    memset(&context, 0, sizeof(PH_AFFINITY_DIALOG_CONTEXT));
    context.ProcessItem = ProcessItem;
    context.ThreadItem = NULL;

    if (PhDialogBox(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_AFFINITY),
        ParentWindowHandle,
        PhpProcessAffinityDlgProc,
        &context
        ) == IDOK)
    {
        *NewAffinityMask = context.NewAffinityMask;

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

VOID PhShowThreadAffinityDialog(
    _In_ HWND ParentWindowHandle,
    _In_ PPH_THREAD_ITEM* Threads,
    _In_ ULONG NumberOfThreads
    )
{
    PH_AFFINITY_DIALOG_CONTEXT context;

    memset(&context, 0, sizeof(PH_AFFINITY_DIALOG_CONTEXT));
    context.Threads = Threads;
    context.NumberOfThreads = NumberOfThreads;
    context.ThreadHandles = PhAllocateZero(NumberOfThreads * sizeof(HANDLE));

    // Cache handles to each thread since the ThreadId gets
    // reassigned to a different process after the thread exits. (dmex)
    for (ULONG i = 0; i < NumberOfThreads; i++)
    {
        if (!NT_SUCCESS(PhOpenThread(
            &context.ThreadHandles[i],
            THREAD_QUERY_LIMITED_INFORMATION | THREAD_SET_LIMITED_INFORMATION | THREAD_SET_INFORMATION,
            Threads[i]->ThreadId
            )))
        {
            if (!NT_SUCCESS(PhOpenThread(
                &context.ThreadHandles[i],
                THREAD_QUERY_LIMITED_INFORMATION | THREAD_SET_LIMITED_INFORMATION,
                Threads[i]->ThreadId
                )))
            {
                context.ThreadHandles[i] = INVALID_HANDLE_VALUE;
            }
        }
    }

    PhDialogBox(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_AFFINITY),
        ParentWindowHandle,
        PhpProcessAffinityDlgProc,
        &context
        );
}

static BOOLEAN PhpShowProcessErrorAffinity(
    _In_ HWND hWnd,
    _In_ PPH_PROCESS_ITEM Process,
    _In_ NTSTATUS Status,
    _In_opt_ ULONG Win32Result
    )
{
    return PhShowContinueStatus(
        hWnd,
        PhaFormatString(
        L"无法更改进程 %lu 的亲和性",
        HandleToUlong(Process->ProcessId)
        )->Buffer,
        Status,
        Win32Result
        );
}

static BOOLEAN PhpShowThreadErrorAffinity(
    _In_ HWND hWnd,
    _In_ PPH_THREAD_ITEM Thread,
    _In_ NTSTATUS Status,
    _In_opt_ ULONG Win32Result
    )
{
    return PhShowContinueStatus(
        hWnd,
        PhaFormatString(
        L"无法更改线程 %lu 的亲和性",
        HandleToUlong(Thread->ThreadId)
        )->Buffer,
        Status,
        Win32Result
        );
}

VOID PhpShowThreadErrorAffinityList(
    _In_ PPH_AFFINITY_DIALOG_CONTEXT Context,
    _In_ PPH_LIST AffinityErrorsList
    )
{
    PH_STRING_BUILDER stringBuilder;

    PhInitializeStringBuilder(&stringBuilder, 100);

    for (ULONG i = 0; i < AffinityErrorsList->Count; i++)
    {
        PhAppendFormatStringBuilder(
            &stringBuilder,
            L"%s\n",
            PhGetStringOrDefault(AffinityErrorsList->Items[i], L"发生未知错误。")
            );
    }

    if (PhEndsWithStringRef2(&stringBuilder.String->sr, L"\n", FALSE))
        PhRemoveEndStringBuilder(&stringBuilder, 2);

    PhShowInformation2(
        Context->WindowHandle,
        L"无法更新线程亲和性",
        L"无法更新线程亲和性：\r\n%s",
        PhGetString(PhFinalStringBuilderString(&stringBuilder))
        );

    PhDeleteStringBuilder(&stringBuilder);
    PhDereferenceObjects(AffinityErrorsList->Items, AffinityErrorsList->Count);
    PhDereferenceObject(AffinityErrorsList);
}

BOOLEAN PhpCheckThreadsHaveSameAffinity(
    _In_ PPH_AFFINITY_DIALOG_CONTEXT Context
    )
{
    BOOLEAN result = TRUE;
    GROUP_AFFINITY groupAffinity;
    KAFFINITY lastAffinityMask = 0;
    KAFFINITY affinityMask = 0;

    if (Context->ThreadHandles[0] != INVALID_HANDLE_VALUE)
    {
        if (NT_SUCCESS(PhGetThreadGroupAffinity(Context->ThreadHandles[0], &groupAffinity)))
        {
            lastAffinityMask = groupAffinity.Mask;
        }
    }

    for (ULONG i = 0; i < Context->NumberOfThreads; i++)
    {
        if (Context->ThreadHandles[i] == INVALID_HANDLE_VALUE)
            continue;

        if (NT_SUCCESS(PhGetThreadGroupAffinity(Context->ThreadHandles[i], &groupAffinity)))
        {
            affinityMask = groupAffinity.Mask;
        }

        if (lastAffinityMask != affinityMask)
        {
            result = FALSE;
            break;
        }
    }

    return result;
}

static BOOLEAN PhpQueryProcessorCoreTopology(
    _In_ PPH_AFFINITY_DIALOG_CONTEXT Context
    )
{
    ULONG returnLength = 0;
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX buffer;
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX entry;
    USHORT numberOfGroups;
    ULONG coreNumber = 0;
    UCHAR minimumEfficiencyClass = 0xff;
    UCHAR maximumEfficiencyClass = 0;

    numberOfGroups = PhSystemProcessorInformation.NumberOfProcessorGroups;

    if (numberOfGroups == 0)
        numberOfGroups = 1;

    Context->CoreInfoGroups = PhAllocateZero(numberOfGroups * sizeof(PPH_AFFINITY_CPU_INFO));

    for (USHORT group = 0; group < numberOfGroups; group++)
    {
        Context->CoreInfoGroups[group] = PhAllocateZero(MAXIMUM_PROC_PER_GROUP * sizeof(PH_AFFINITY_CPU_INFO));
    }

    Context->NumberOfCoreInfoGroups = numberOfGroups;

    GetLogicalProcessorInformationEx(RelationProcessorCore, NULL, &returnLength);

    if (returnLength == 0)
        return FALSE;

    buffer = PhAllocateZero(returnLength);

    if (!GetLogicalProcessorInformationEx(RelationProcessorCore, buffer, &returnLength))
    {
        PhFree(buffer);
        return FALSE;
    }

    entry = buffer;

    while ((PBYTE)entry < (PBYTE)buffer + returnLength)
    {
        if (entry->Relationship == RelationProcessorCore && entry->Processor.GroupCount > 0)
        {
            BOOLEAN isSmt = (entry->Processor.Flags & LTP_PC_SMT) != 0;
            UCHAR efficiencyClass = entry->Processor.EfficiencyClass;

            for (ULONG groupIndex = 0; groupIndex < entry->Processor.GroupCount; groupIndex++)
            {
                PGROUP_AFFINITY groupAffinity = &entry->Processor.GroupMask[groupIndex];

                if (groupAffinity->Group >= numberOfGroups)
                    continue;

                for (ULONG bit = 0; bit < MAXIMUM_PROC_PER_GROUP; bit++)
                {
                    if ((groupAffinity->Mask >> bit) & 0x1)
                    {
                        PPH_AFFINITY_CPU_INFO info = &Context->CoreInfoGroups[groupAffinity->Group][bit];

                        info->Present = TRUE;
                        info->SmT = isSmt;
                        info->EfficiencyClass = efficiencyClass;
                        info->CoreNumber = coreNumber;
                        info->Group = (USHORT)groupAffinity->Group;
                    }
                }
            }

            if (efficiencyClass < minimumEfficiencyClass)
                minimumEfficiencyClass = efficiencyClass;
            if (efficiencyClass > maximumEfficiencyClass)
                maximumEfficiencyClass = efficiencyClass;

            if (isSmt)
                Context->HasSmtCore = TRUE;

            coreNumber++;
        }

        entry = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)((PBYTE)entry + entry->Size);
    }

    PhFree(buffer);

    if (coreNumber == 0)
        return FALSE;

    Context->MinimumEfficiencyClass = minimumEfficiencyClass;
    Context->MaximumEfficiencyClass = maximumEfficiencyClass;

    // A middle efficiency class indicates a platform with three or more
    // core tiers (e.g. P/E/LPE cores).
    for (USHORT group = 0; group < numberOfGroups && !Context->HasMidEfficiencyClass; group++)
    {
        for (ULONG bit = 0; bit < MAXIMUM_PROC_PER_GROUP; bit++)
        {
            PPH_AFFINITY_CPU_INFO info = &Context->CoreInfoGroups[group][bit];

            if (info->Present &&
                info->EfficiencyClass > Context->MinimumEfficiencyClass &&
                info->EfficiencyClass < Context->MaximumEfficiencyClass)
            {
                Context->HasMidEfficiencyClass = TRUE;
                break;
            }
        }
    }

    return TRUE;
}

static USHORT PhpGetCurrentAffinityGroup(
    _In_ PPH_AFFINITY_DIALOG_CONTEXT Context
    )
{
    if (Context->GroupComboHandle)
    {
        LONG index = ComboBox_GetCurSel(Context->GroupComboHandle);

        if (index != CB_ERR)
            return (USHORT)index;
    }

    return Context->AffinityGroup;
}

static VOID PhpSelectAffinityCoreClass(
    _In_ PPH_AFFINITY_DIALOG_CONTEXT Context,
    _In_ ULONG SelectionType
    )
{
    USHORT group;

    if (!Context->CoreInfoGroups)
        return;

    group = PhpGetCurrentAffinityGroup(Context);

    if (group >= Context->NumberOfCoreInfoGroups)
        return;

    for (ULONG i = 0; i < MAXIMUM_PROC_PER_GROUP; i++)
    {
        HWND checkBox = Context->CpuControlList->Items[i];
        PPH_AFFINITY_CPU_INFO info = &Context->CoreInfoGroups[group][i];
        BOOLEAN select = FALSE;

        if (!info->Present)
            continue;

        switch (SelectionType)
        {
        case PHP_AFFINITY_SELECT_PERFCORES:
            select = info->EfficiencyClass == Context->MaximumEfficiencyClass;
            break;
        case PHP_AFFINITY_SELECT_ECORES:
            // On platforms with two core tiers (e.g. P/E) the efficient cores
            // are the lowest class; with three or more tiers (e.g. P/E/LPE)
            // they are the middle class. (dmex)
            if (Context->HasMidEfficiencyClass)
                select = info->EfficiencyClass > Context->MinimumEfficiencyClass &&
                    info->EfficiencyClass < Context->MaximumEfficiencyClass;
            else
                select = info->EfficiencyClass == Context->MinimumEfficiencyClass;
            break;
        case PHP_AFFINITY_SELECT_LPECORES:
            select = Context->HasMidEfficiencyClass &&
                info->EfficiencyClass == Context->MinimumEfficiencyClass;
            break;
        case PHP_AFFINITY_SELECT_HTCORES:
            select = info->SmT;
            break;
        }

        if (IsWindowEnabled(checkBox))
        {
            Button_SetCheck(checkBox, select ? BST_CHECKED : BST_UNCHECKED);
        }
    }
}

static VOID PhpAddAffinityTooltip(
    _In_ PPH_AFFINITY_DIALOG_CONTEXT Context,
    _In_ HWND ControlHandle,
    _In_ PWSTR Text
    )
{
    TOOLINFO toolInfo;

    if (!Context->TooltipHandle)
    {
        Context->TooltipHandle = PhCreateWindowEx(
            TOOLTIPS_CLASS,
            NULL,
            WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX | TTS_NOANIMATE | TTS_NOFADE,
            WS_EX_TOPMOST,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            Context->WindowHandle,
            NULL,
            NULL,
            NULL
            );

        SetWindowPos(
            Context->TooltipHandle,
            HWND_TOPMOST,
            0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE
            );
    }

    memset(&toolInfo, 0, sizeof(TOOLINFO));
    toolInfo.cbSize = sizeof(TOOLINFO);
    toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
    toolInfo.hwnd = Context->WindowHandle;
    toolInfo.uId = (UINT_PTR)ControlHandle;
    toolInfo.lpszText = Text;

    SendMessage(Context->TooltipHandle, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);
}

static PPH_STRING PhpCreateCpuTooltipText(
    _In_ PPH_AFFINITY_DIALOG_CONTEXT Context,
    _In_ USHORT Group,
    _In_ ULONG ProcessorIndex
    )
{
    PH_STRING_BUILDER stringBuilder;
    PPH_AFFINITY_CPU_INFO info;
    BOOLEAN hybrid;

    info = &Context->CoreInfoGroups[Group][ProcessorIndex];
    hybrid = Context->MinimumEfficiencyClass != Context->MaximumEfficiencyClass;

    PhInitializeStringBuilder(&stringBuilder, 60);
    PhAppendFormatStringBuilder(&stringBuilder, L"CPU %lu：", ProcessorIndex);

    if (hybrid)
    {
        if (info->EfficiencyClass == Context->MaximumEfficiencyClass)
            PhAppendStringBuilder2(&stringBuilder, L"性能核（P-core）");
        else if (
            info->EfficiencyClass > Context->MinimumEfficiencyClass &&
            info->EfficiencyClass < Context->MaximumEfficiencyClass
            )
            PhAppendStringBuilder2(&stringBuilder, L"能效核（E-core）");
        else if (Context->HasMidEfficiencyClass)
            PhAppendStringBuilder2(&stringBuilder, L"低功耗能效核（LP E-core）");
        else
            PhAppendStringBuilder2(&stringBuilder, L"能效核（E-core）");

        if (info->SmT)
            PhAppendStringBuilder2(&stringBuilder, L"；超线程核心");
        else
            PhAppendStringBuilder2(&stringBuilder, L"；独立物理核心");
    }
    else
    {
        if (info->SmT)
            PhAppendStringBuilder2(&stringBuilder, L"超线程核心");
        else
            PhAppendStringBuilder2(&stringBuilder, L"独立物理核心");
    }

    if (info->SmT)
    {
        BOOLEAN firstSibling = TRUE;

        PhAppendStringBuilder2(&stringBuilder, L"（与 ");

        for (ULONG i = 0; i < MAXIMUM_PROC_PER_GROUP; i++)
        {
            PPH_AFFINITY_CPU_INFO sibling = &Context->CoreInfoGroups[Group][i];

            if (i == ProcessorIndex || !sibling->Present || sibling->CoreNumber != info->CoreNumber)
                continue;

            if (!firstSibling)
                PhAppendStringBuilder2(&stringBuilder, L"、");

            PhAppendFormatStringBuilder(&stringBuilder, L"CPU %lu", i);
            firstSibling = FALSE;
        }

        PhAppendStringBuilder2(&stringBuilder, L" 共享同一物理核心）");
    }

    return PhFinalStringBuilderString(&stringBuilder);
}

INT_PTR CALLBACK PhpProcessAffinityDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_AFFINITY_DIALOG_CONTEXT context = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PPH_AFFINITY_DIALOG_CONTEXT)lParam;
        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);
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
            NTSTATUS status = STATUS_UNSUCCESSFUL;
            BOOLEAN differentAffinity = FALSE;
            ULONG i;

            context->WindowHandle = hwndDlg;

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));
            PhSetApplicationWindowIcon(hwndDlg);

            {
                context->CpuControlList = PhCreateList(MAXIMUM_PROC_PER_GROUP);

                for (i = 0; i < MAXIMUM_PROC_PER_GROUP; i++)
                {
                    PhAddItemList(context->CpuControlList, GetDlgItem(hwndDlg, IDC_CPU0 + i));
                }
            }

            if (!PhSystemProcessorInformation.SingleProcessorGroup)
            {
                context->GroupComboHandle = GetDlgItem(hwndDlg, IDC_GROUPCPU);

                for (USHORT processorGroup = 0; processorGroup < PhSystemProcessorInformation.NumberOfProcessorGroups; processorGroup++)
                {
                    ComboBox_AddString(context->GroupComboHandle, PhaFormatString(L"组 %hu", processorGroup)->Buffer);
                }

                ShowWindow(context->GroupComboHandle, SW_SHOW);
            }

            if (context->ProcessItem)
            {
                HANDLE processHandle;
                GROUP_AFFINITY processGroupAffinity = { 0 };

                if (NT_SUCCESS(status = PhOpenProcess(
                    &processHandle,
                    PROCESS_QUERY_LIMITED_INFORMATION,
                    context->ProcessItem->ProcessId
                    )))
                {
                    status = PhGetProcessGroupAffinity(processHandle, &processGroupAffinity);

                    if (NT_SUCCESS(status))
                    {
                        context->AffinityMask = processGroupAffinity.Mask;
                        context->AffinityGroup = processGroupAffinity.Group;

                        if (context->GroupComboHandle)
                        {
                            ComboBox_SetCurSel(context->GroupComboHandle, processGroupAffinity.Group);
                        }
                    }

                    NtClose(processHandle);
                }
            }
            else if (context->ThreadItem)
            {
                HANDLE threadHandle;
                THREAD_BASIC_INFORMATION basicInfo;
                GROUP_AFFINITY groupAffinity = { 0 };
                HANDLE processHandle;

                if (NT_SUCCESS(status = PhOpenThread(
                    &threadHandle,
                    THREAD_QUERY_LIMITED_INFORMATION,
                    context->ThreadItem->ThreadId
                    )))
                {
                    status = PhGetThreadGroupAffinity(threadHandle, &groupAffinity);

                    if (NT_SUCCESS(status))
                    {
                        context->AffinityMask = groupAffinity.Mask;
                        context->AffinityGroup = groupAffinity.Group;

                        if (context->GroupComboHandle)
                        {
                            ComboBox_SetCurSel(context->GroupComboHandle, groupAffinity.Group);
                        }

                        if (NT_SUCCESS(PhGetThreadBasicInformation(
                            threadHandle,
                            &basicInfo
                            )))
                        {
                            // A thread's affinity mask is restricted by the process affinity mask,
                            // so use that as the system affinity mask. (wj32)

                            if (NT_SUCCESS(PhOpenProcess(
                                &processHandle,
                                PROCESS_QUERY_LIMITED_INFORMATION,
                                basicInfo.ClientId.UniqueProcess
                                )))
                            {
                                GROUP_AFFINITY processGroupAffinity = { 0 };

                                if (NT_SUCCESS(PhGetProcessGroupAffinity(processHandle, &processGroupAffinity)))
                                {
                                    context->SystemAffinityMask = processGroupAffinity.Mask;
                                }

                                NtClose(processHandle);
                            }
                        }
                    }

                    NtClose(threadHandle);
                }
            }
            else if (context->Threads)
            {
                HANDLE processHandle;
                THREAD_BASIC_INFORMATION basicInfo;
                GROUP_AFFINITY groupAffinity = { 0 };
                PPH_STRING windowText;

                windowText = PH_AUTO(PhGetWindowText(hwndDlg));
                PhSetWindowText(hwndDlg, PhaFormatString(
                    L"%s（%lu 个线程）",
                    windowText->Buffer,
                    context->NumberOfThreads
                    )->Buffer);

                differentAffinity = !PhpCheckThreadsHaveSameAffinity(context);

                if (context->ThreadHandles[0] != INVALID_HANDLE_VALUE)
                {
                    // Use affinity from the first thread when all threads are identical (dmex)
                    status = PhGetThreadGroupAffinity(
                        context->ThreadHandles[0],
                        &groupAffinity
                        );
                }
                else
                {
                    status = STATUS_UNSUCCESSFUL;
                }

                if (NT_SUCCESS(status))
                {
                    context->AffinityMask = groupAffinity.Mask;
                    context->AffinityGroup = groupAffinity.Group;

                    if (context->GroupComboHandle)
                    {
                        ComboBox_SetCurSel(context->GroupComboHandle, groupAffinity.Group);
                    }

                    if (NT_SUCCESS(PhGetThreadBasicInformation(
                        context->ThreadHandles[0],
                        &basicInfo
                        )))
                    {
                        if (NT_SUCCESS(PhOpenProcess(
                            &processHandle,
                            PROCESS_QUERY_LIMITED_INFORMATION,
                            basicInfo.ClientId.UniqueProcess
                            )))
                        {
                            GROUP_AFFINITY processGroupAffinity = { 0 };

                            if (NT_SUCCESS(PhGetProcessGroupAffinity(processHandle, &processGroupAffinity)))
                            {
                                context->SystemAffinityMask = processGroupAffinity.Mask;
                            }

                            NtClose(processHandle);
                        }
                    }
                }
            }

            if (NT_SUCCESS(status) && context->SystemAffinityMask == 0)
            {
                KAFFINITY systemAffinityMask;

                if (PhSystemProcessorInformation.SingleProcessorGroup)
                {
                    status = PhGetProcessorSystemAffinityMask(&systemAffinityMask);
                }
                else
                {
                    status = PhGetProcessorGroupActiveAffinityMask(context->AffinityGroup, &systemAffinityMask);
                }

                if (NT_SUCCESS(status))
                {
                    context->SystemAffinityMask = systemAffinityMask;
                }
            }

            if (!NT_SUCCESS(status))
            {
                PhShowStatus(hwndDlg, L"无法查询当前的亲和性。", status, 0);
                EndDialog(hwndDlg, IDCANCEL);
                break;
            }

            // Disable the CPU checkboxes which aren't part of the system affinity mask,
            // and check the CPU checkboxes which are part of the affinity mask. (wj32)

            for (i = 0; i < MAXIMUM_PROC_PER_GROUP; i++)
            {
                if ((context->SystemAffinityMask >> i) & 0x1)
                {
                    if (differentAffinity) // Skip for multiple selection (dmex)
                        continue;

                    if ((context->AffinityMask >> i) & 0x1)
                    {
                        Button_SetCheck(context->CpuControlList->Items[i], BST_CHECKED);
                    }
                }
                else
                {
                    EnableWindow(context->CpuControlList->Items[i], FALSE);
                }
            }

            // Query the processor core topology for P/E/LPE core
            // selection and hyper-threading identification. (dmex)
            if (PhpQueryProcessorCoreTopology(context))
            {
                BOOLEAN hybrid = context->MinimumEfficiencyClass != context->MaximumEfficiencyClass;
                USHORT currentGroup;
                PPH_STRING tooltipText;

                currentGroup = PhpGetCurrentAffinityGroup(context);

                EnableWindow(GetDlgItem(hwndDlg, IDC_PERFCORES), hybrid);
                EnableWindow(GetDlgItem(hwndDlg, IDC_ECORES), hybrid);
                EnableWindow(GetDlgItem(hwndDlg, IDC_LPECORES), context->HasMidEfficiencyClass);
                EnableWindow(GetDlgItem(hwndDlg, IDC_HTCORES), context->HasSmtCore);

                context->TooltipStringList = PhCreateList(16);

                // Add tooltips which identify the core type of each logical processor.
                if (currentGroup < context->NumberOfCoreInfoGroups)
                {
                    for (ULONG i = 0; i < MAXIMUM_PROC_PER_GROUP; i++)
                    {
                        PPH_AFFINITY_CPU_INFO info = &context->CoreInfoGroups[currentGroup][i];

                        if (!info->Present)
                            continue;

                        tooltipText = PhpCreateCpuTooltipText(context, currentGroup, i);
                        PhAddItemList(context->TooltipStringList, tooltipText);
                        PhpAddAffinityTooltip(context, context->CpuControlList->Items[i], PhGetString(tooltipText));
                    }
                }

                PhpAddAffinityTooltip(context, GetDlgItem(hwndDlg, IDC_PERFCORES), L"选中全部性能核（P-core）逻辑处理器");
                PhpAddAffinityTooltip(context, GetDlgItem(hwndDlg, IDC_ECORES), L"选中全部能效核（E-core）逻辑处理器");
                PhpAddAffinityTooltip(context, GetDlgItem(hwndDlg, IDC_LPECORES), L"选中全部低功耗能效核（LP E-core）逻辑处理器");
                PhpAddAffinityTooltip(context, GetDlgItem(hwndDlg, IDC_HTCORES), L"选中全部属于超线程核心（SMT）的逻辑处理器");
            }
            else
            {
                EnableWindow(GetDlgItem(hwndDlg, IDC_PERFCORES), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_ECORES), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_LPECORES), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_HTCORES), FALSE);
            }

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);

            if (context->CoreInfoGroups)
            {
                for (USHORT group = 0; group < context->NumberOfCoreInfoGroups; group++)
                {
                    PhFree(context->CoreInfoGroups[group]);
                }

                PhFree(context->CoreInfoGroups);
                context->CoreInfoGroups = NULL;
            }

            if (context->TooltipStringList)
            {
                PhDereferenceObjects(context->TooltipStringList->Items, context->TooltipStringList->Count);
                PhDereferenceObject(context->TooltipStringList);
                context->TooltipStringList = NULL;
            }

            if (context->ThreadHandles)
            {
                for (ULONG i = 0; i < context->NumberOfThreads; i++)
                {
                    if (context->ThreadHandles[i] != INVALID_HANDLE_VALUE)
                    {
                        NtClose(context->ThreadHandles[i]);
                        context->ThreadHandles[i] = NULL;
                    }
                }

                PhFree(context->ThreadHandles);
            }
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
                EndDialog(hwndDlg, IDCANCEL);
                break;
            case IDOK:
                {
                    NTSTATUS status = STATUS_SUCCESS;
                    KAFFINITY affinityMask = 0;
                    USHORT affinityGroup = 0;

                    // Work out the affinity mask.

                    for (ULONG i = 0; i < MAXIMUM_PROC_PER_GROUP; i++)
                    {
                        if (Button_GetCheck(context->CpuControlList->Items[i]) == BST_CHECKED)
                            affinityMask |= AFFINITY_MASK(i);
                    }

                    if (context->GroupComboHandle)
                    {
                         LONG affinityGroupSelection = ComboBox_GetCurSel(context->GroupComboHandle);

                         if (affinityGroupSelection == CB_ERR)
                             affinityGroup = context->AffinityGroup;
                         else
                             affinityGroup = (USHORT)affinityGroupSelection;
                    }

                    if (affinityMask == 0)
                    {
                        PhShowError2(hwndDlg, L"无法更改亲和性设置。", L"%s", L"必须至少选择一个 CPU。");
                        break;
                    }

                    if (context->ProcessItem)
                    {
                        HANDLE processHandle;

                        if (NT_SUCCESS(status = PhOpenProcess(
                            &processHandle,
                            PROCESS_SET_INFORMATION,
                            context->ProcessItem->ProcessId
                            )))
                        {
                            if (PhSystemProcessorInformation.SingleProcessorGroup)
                            {
                                status = PhSetProcessAffinityMask(processHandle, affinityMask);
                            }
                            else
                            {
                                GROUP_AFFINITY groupAffinity;

                                memset(&groupAffinity, 0, sizeof(GROUP_AFFINITY));
                                groupAffinity.Group = affinityGroup;
                                groupAffinity.Mask = affinityMask;

                                status = PhSetProcessGroupAffinity(processHandle, groupAffinity);
                            }

                            NtClose(processHandle);
                        }

                        if (NT_SUCCESS(status))
                        {
                            context->NewAffinityMask = affinityMask;
                        }
                        else
                        {
                            PhpShowProcessErrorAffinity(hwndDlg, context->ProcessItem, status, 0);
                        }
                    }
                    else if (context->ThreadItem)
                    {
                        if (PhSystemProcessorInformation.SingleProcessorGroup)
                        {
                            HANDLE threadHandle;

                            status = PhOpenThread(
                                &threadHandle,
                                THREAD_SET_LIMITED_INFORMATION,
                                context->ThreadItem->ThreadId
                                );

                            if (NT_SUCCESS(status))
                            {
                                status = PhSetThreadAffinityMask(threadHandle, affinityMask);
                                NtClose(threadHandle);
                            }

                            if (!NT_SUCCESS(status))
                            {
                                PhpShowThreadErrorAffinity(hwndDlg, context->ThreadItem, status, 0);
                            }
                        }
                        else
                        {
                            HANDLE threadHandle;

                            status = PhOpenThread(
                                &threadHandle,
                                THREAD_SET_INFORMATION,
                                context->ThreadItem->ThreadId
                                );

                            if (NT_SUCCESS(status))
                            {
                                GROUP_AFFINITY groupAffinity;

                                memset(&groupAffinity, 0, sizeof(GROUP_AFFINITY));
                                groupAffinity.Group = affinityGroup;
                                groupAffinity.Mask = affinityMask;

                                status = PhSetThreadGroupAffinity(threadHandle, groupAffinity);
                                NtClose(threadHandle);
                            }

                            if (!NT_SUCCESS(status))
                            {
                                PhpShowThreadErrorAffinity(hwndDlg, context->ThreadItem, status, 0);
                            }
                        }
                    }
                    else if (context->Threads)
                    {
                        PPH_LIST threadAffinityErrors = PhCreateList(1);

                        for (ULONG i = 0; i < context->NumberOfThreads; i++)
                        {
                            if (context->ThreadHandles[i] == INVALID_HANDLE_VALUE)
                                continue;

                            if (PhSystemProcessorInformation.SingleProcessorGroup)
                            {
                                status = PhSetThreadAffinityMask(context->ThreadHandles[i], affinityMask);
                            }
                            else
                            {
                                GROUP_AFFINITY groupAffinity;

                                memset(&groupAffinity, 0, sizeof(GROUP_AFFINITY));
                                groupAffinity.Group = affinityGroup;
                                groupAffinity.Mask = affinityMask;

                                status = PhSetThreadGroupAffinity(context->ThreadHandles[i], groupAffinity);
                            }

                            if (!NT_SUCCESS(status))
                            {
                                PPH_STRING errorMessage;

                                if (errorMessage = PhGetNtMessage(status))
                                {
                                    PhAddItemList(threadAffinityErrors, errorMessage);
                                }
                            }
                        }

                        if (threadAffinityErrors->Count > 0)
                            PhpShowThreadErrorAffinityList(context, threadAffinityErrors);
                        else
                            PhDereferenceObject(threadAffinityErrors);
                    }

                    if (NT_SUCCESS(status))
                    {
                        EndDialog(hwndDlg, IDOK);
                    }
                }
                break;
            case IDC_SELECTALL:
            case IDC_DESELECTALL:
                {
                    for (ULONG i = 0; i < MAXIMUM_PROC_PER_GROUP; i++)
                    {
                        HWND checkBox = context->CpuControlList->Items[i];

                        if (IsWindowEnabled(checkBox))
                            Button_SetCheck(checkBox, GET_WM_COMMAND_ID(wParam, lParam) == IDC_SELECTALL ? BST_CHECKED : BST_UNCHECKED);
                    }
                }
                break;
            case IDC_PERFCORES:
                PhpSelectAffinityCoreClass(context, PHP_AFFINITY_SELECT_PERFCORES);
                break;
            case IDC_ECORES:
                PhpSelectAffinityCoreClass(context, PHP_AFFINITY_SELECT_ECORES);
                break;
            case IDC_LPECORES:
                PhpSelectAffinityCoreClass(context, PHP_AFFINITY_SELECT_LPECORES);
                break;
            case IDC_HTCORES:
                PhpSelectAffinityCoreClass(context, PHP_AFFINITY_SELECT_HTCORES);
                break;
            case IDC_GROUPCPU:
                {
                    LONG index;

                    if (!context->GroupComboHandle)
                        break;

                    index = ComboBox_GetCurSel(context->GroupComboHandle);

                    if (index != CB_ERR)
                    {
                        if (index != context->AffinityGroup)
                        {
                            for (ULONG i = 0; i < MAXIMUM_PROC_PER_GROUP; i++)
                            {
                                Button_SetCheck(context->CpuControlList->Items[i], BST_UNCHECKED);
                            }
                        }
                        else
                        {
                            BOOLEAN differentAffinity = FALSE;

                            if (context->Threads)
                            {
                                differentAffinity = !PhpCheckThreadsHaveSameAffinity(context);
                            }

                            for (ULONG i = 0; i < MAXIMUM_PROC_PER_GROUP; i++)
                            {
                                if ((context->SystemAffinityMask >> i) & 0x1)
                                {
                                    if (differentAffinity) // Skip for multiple selection (dmex)
                                        continue;

                                    if ((context->AffinityMask >> i) & 0x1)
                                    {
                                        Button_SetCheck(context->CpuControlList->Items[i], BST_CHECKED);
                                    }
                                }
                            }
                        }
                    }
                }
                break;
            }
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

// Note: Workaround for UserNotes plugin dialog overrides (dmex)
NTSTATUS PhSetProcessItemAffinityMask(
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_ KAFFINITY AffinityMask
    )
{
    NTSTATUS status;
    HANDLE processHandle;

    status = PhOpenProcess(
        &processHandle,
        PROCESS_SET_INFORMATION,
        ProcessItem->ProcessId
        );

    if (NT_SUCCESS(status))
    {
        status = PhSetProcessAffinityMask(processHandle, AffinityMask);
        NtClose(processHandle);
    }

    return status;
}

// Note: Workaround for UserNotes plugin dialog overrides (dmex)
NTSTATUS PhSetProcessItemPagePriority(
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_ ULONG PagePriority
    )
{
    NTSTATUS status;
    HANDLE processHandle;

    status = PhOpenProcess(
        &processHandle,
        PROCESS_SET_INFORMATION,
        ProcessItem->ProcessId
        );

    if (NT_SUCCESS(status))
    {
        status = PhSetProcessPagePriority(processHandle, PagePriority);
        NtClose(processHandle);
    }

    return status;
}

// Note: Workaround for UserNotes plugin dialog overrides (dmex)
NTSTATUS PhSetProcessItemIoPriority(
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_ IO_PRIORITY_HINT IoPriority
    )
{
    NTSTATUS status;
    HANDLE processHandle;

    status = PhOpenProcess(
        &processHandle,
        PROCESS_SET_INFORMATION,
        ProcessItem->ProcessId
        );

    if (NT_SUCCESS(status))
    {
        status = PhSetProcessIoPriority(processHandle, IoPriority);
        NtClose(processHandle);
    }

    return status;
}

// Note: Workaround for UserNotes plugin dialog overrides (dmex)
NTSTATUS PhSetProcessItemPriority(
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_ UCHAR PriorityClass
    )
{
    NTSTATUS status;
    HANDLE processHandle;

    status = PhOpenProcess(
        &processHandle,
        PROCESS_SET_INFORMATION,
        ProcessItem->ProcessId
        );

    if (NT_SUCCESS(status))
    {
        status = PhSetProcessPriorityClass(processHandle, PriorityClass);
        NtClose(processHandle);
    }

    return status;
}

// Note: Workaround for UserNotes plugin dialog overrides (dmex)
NTSTATUS PhSetProcessItemPriorityBoost(
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_ BOOLEAN PriorityBoost
    )
{
    NTSTATUS status;
    HANDLE processHandle;

    status = PhOpenProcess(
        &processHandle,
        PROCESS_SET_INFORMATION,
        ProcessItem->ProcessId
        );

    if (NT_SUCCESS(status))
    {
        status = PhSetProcessPriorityBoost(processHandle, PriorityBoost);
        NtClose(processHandle);
    }

    return status;
}

// Note: Workaround for UserNotes plugin dialog overrides (dmex)
NTSTATUS PhSetProcessItemThrottlingState(
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_ BOOLEAN ClearThrottlingState
    )
{
    NTSTATUS status;
    HANDLE processHandle;

    status = PhOpenProcess(
        &processHandle,
        PROCESS_SET_INFORMATION,
        ProcessItem->ProcessId
        );

    if (NT_SUCCESS(status))
    {
        if (ClearThrottlingState)
        {
            PhSetProcessPriorityClass(processHandle, PROCESS_PRIORITY_CLASS_NORMAL);

            status = PhSetProcessPowerThrottlingState(processHandle, 0, 0);
        }
        else
        {
            // Taskmgr sets the process priority to idle before enabling 'Eco mode'. (dmex)
            PhSetProcessPriorityClass(processHandle, PROCESS_PRIORITY_CLASS_IDLE);

            status = PhSetProcessPowerThrottlingState(
                processHandle,
                POWER_THROTTLING_PROCESS_EXECUTION_SPEED,
                POWER_THROTTLING_PROCESS_EXECUTION_SPEED
                );
        }

        NtClose(processHandle);
    }

    return status;
}
