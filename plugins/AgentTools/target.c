/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2026
 *
 */

#include "agenttools.h"

NTSTATUS AtResolveProcessTarget(
    _In_opt_ PVOID Arguments,
    _In_ BOOLEAN RequireSequenceNumber,
    _In_ ACCESS_MASK ProcessAccess,
    _Out_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    ULONG64 processId;
    ULONG64 sequenceNumber;
    BOOLEAN haveSequenceNumber;
    PPH_PROCESS_ITEM processItem;
    HANDLE processHandle = NULL;

    // Zero the whole target so AtDeleteTarget never frees an uninitialized field when a direct
    // caller (a read tool) did not memset its stack target first.
    memset(Target, 0, sizeof(AT_TARGET));

    if (!AtGetArgumentUInt64(Arguments, "pid", &processId) || processId > MAXULONG)
    {
        AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER, L"需要提供 pid，且必须为整数。");
        return STATUS_INVALID_PARAMETER;
    }

    haveSequenceNumber = AtGetArgumentUInt64(Arguments, "process_sequence_number", &sequenceNumber);

    if (RequireSequenceNumber && !haveSequenceNumber)
    {
        AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER, L"需要提供 process_sequence_number；请从 list_processes 或 get_process 获取。");
        return STATUS_INVALID_PARAMETER;
    }

    if (!(processItem = PhReferenceProcessItem(UlongToHandle((ULONG)processId))))
    {
        AtSetToolError(Result, "not_found", STATUS_NOT_FOUND, L"提供程序缓存中没有 pid 为 %llu 的进程。", processId);
        return STATUS_NOT_FOUND;
    }

    if (haveSequenceNumber && processItem->ProcessSequenceNumber != sequenceNumber)
    {
        AtSetToolError(
            Result,
            "identity_mismatch",
            STATUS_PROCESS_IS_TERMINATING,
            L"pid %llu 现在的 process_sequence_number 是 %llu 而非 %llu；之前显示的进程已退出且 pid 被复用。请重新列出后重试。",
            processId,
            processItem->ProcessSequenceNumber,
            sequenceNumber
            );
        PhDereferenceObject(processItem);
        return STATUS_PROCESS_IS_TERMINATING;
    }

    if (ProcessAccess)
    {
        NTSTATUS status;
        ULONGLONG liveSequenceNumber;

        if (!PH_IS_REAL_PROCESS_ID(processItem->ProcessId))
        {
            AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_CID, L"该 pid 不是真实进程。");
            PhDereferenceObject(processItem);
            return STATUS_INVALID_CID;
        }

        status = PhOpenProcess(
            &processHandle,
            ProcessAccess | PROCESS_QUERY_LIMITED_INFORMATION,
            processItem->ProcessId
            );

        if (!NT_SUCCESS(status))
        {
            AtSetToolStatusError(Result, status, L"打开进程");
            PhDereferenceObject(processItem);
            return status;
        }

        status = PhGetProcessSequenceNumber(processHandle, &liveSequenceNumber);

        if (!NT_SUCCESS(status))
        {
            AtSetToolStatusError(Result, status, L"验证进程标识");
            NtClose(processHandle);
            PhDereferenceObject(processItem);
            return status;
        }

        if (liveSequenceNumber != processItem->ProcessSequenceNumber)
        {
            AtSetToolError(
                Result,
                "identity_mismatch",
                STATUS_PROCESS_IS_TERMINATING,
                L"pid %llu 现在的 process_sequence_number 是 %llu 而非 %llu；之前显示的进程已退出且 pid 被复用。请重新列出后重试。",
                processId,
                liveSequenceNumber,
                processItem->ProcessSequenceNumber
                );
            NtClose(processHandle);
            PhDereferenceObject(processItem);
            return STATUS_PROCESS_IS_TERMINATING;
        }
    }

    Target->Kind = AtTargetProcess;
    Target->ProcessItem = processItem;
    Target->ProcessHandle = processHandle;
    Target->Identity[0] = HandleToUlong(processItem->ProcessId);
    Target->Identity[1] = processItem->ProcessSequenceNumber;

    return STATUS_SUCCESS;
}

PPH_STRING AtFormatThreadCreateTime(
    _In_ HANDLE ThreadHandle
    )
{
    KERNEL_USER_TIMES times;
    SYSTEMTIME systemTime;

    if (!NT_SUCCESS(PhGetThreadTimes(ThreadHandle, &times)) || times.CreateTime.QuadPart == 0)
        return NULL;

    PhLargeIntegerToSystemTime(&systemTime, &times.CreateTime);

    return PhFormatSystemTimeISO(&systemTime);
}

NTSTATUS AtpResolveThreadTarget(
    _In_opt_ PVOID Arguments,
    _In_ BOOLEAN RequireSequenceNumber,
    _In_ ACCESS_MASK ThreadAccess,
    _In_ BOOLEAN SuspendIsOptional,
    _Out_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    NTSTATUS status;
    ULONG64 threadId;
    HANDLE threadHandle;
    THREAD_BASIC_INFORMATION basicInfo;
    PPH_STRING createTime;

    memset(Target, 0, sizeof(AT_TARGET));

    if (!AtGetArgumentUInt64(Arguments, "tid", &threadId) || threadId > MAXULONG || threadId == 0)
    {
        AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER, L"需要提供 tid，且必须为整数。");
        return STATUS_INVALID_PARAMETER;
    }

    status = AtResolveProcessTarget(Arguments, RequireSequenceNumber, PROCESS_QUERY_LIMITED_INFORMATION, Target, Result);

    if (!NT_SUCCESS(status))
        return status;

    status = PhOpenThread(
        &threadHandle,
        ThreadAccess | THREAD_QUERY_LIMITED_INFORMATION,
        UlongToHandle((ULONG)threadId)
        );

    // THREAD_SUSPEND_RESUME is outside KPH_THREAD_READ_ACCESS, so asking for it loses the driver
    // route and with it every thread only the driver can open. The walk suspends if it may.
    if (!NT_SUCCESS(status) && SuspendIsOptional && FlagOn(ThreadAccess, THREAD_SUSPEND_RESUME))
    {
        status = PhOpenThread(
            &threadHandle,
            (ThreadAccess & ~THREAD_SUSPEND_RESUME) | THREAD_QUERY_LIMITED_INFORMATION,
            UlongToHandle((ULONG)threadId)
            );
    }

    if (!NT_SUCCESS(status))
    {
        AtSetToolStatusError(Result, status, L"打开线程");
        AtDeleteTarget(Target);
        return status;
    }

    status = PhGetThreadBasicInformation(threadHandle, &basicInfo);

    if (!NT_SUCCESS(status))
    {
        AtSetToolStatusError(Result, status, L"验证线程标识");
        NtClose(threadHandle);
        AtDeleteTarget(Target);
        return status;
    }

    if (basicInfo.ClientId.UniqueProcess != Target->ProcessItem->ProcessId)
    {
        AtSetToolError(
            Result,
            "identity_mismatch",
            STATUS_INVALID_CID,
            L"线程 %llu 属于 pid %lu 而非 pid %lu。请重新列出线程后重试。",
            threadId,
            HandleToUlong(basicInfo.ClientId.UniqueProcess),
            HandleToUlong(Target->ProcessItem->ProcessId)
            );
        NtClose(threadHandle);
        AtDeleteTarget(Target);
        return STATUS_INVALID_CID;
    }

    // Optional, like process_sequence_number on a read: when the caller passes the create_time it
    // saw, a recycled tid is refused instead of acted on.
    if (createTime = AtGetArgumentString(Arguments, "create_time"))
    {
        PPH_STRING liveCreateTime = AtFormatThreadCreateTime(threadHandle);

        if (!liveCreateTime)
        {
            AtSetToolError(
                Result,
                "identity_mismatch",
                STATUS_INVALID_CID,
                L"无法读取线程 %llu 的创建时间，因此无法校验其标识。",
                threadId
                );
            PhDereferenceObject(createTime);
            NtClose(threadHandle);
            AtDeleteTarget(Target);
            return STATUS_INVALID_CID;
        }

        if (!PhEqualString(liveCreateTime, createTime, TRUE))
        {
            AtSetToolError(
                Result,
                "identity_mismatch",
                STATUS_INVALID_CID,
                L"线程 %llu 的创建时间是 %s 而非 %s；该 tid 已被复用。请重新列出线程后重试。",
                threadId,
                PhGetString(liveCreateTime),
                PhGetString(createTime)
                );
            PhDereferenceObject(liveCreateTime);
            PhDereferenceObject(createTime);
            NtClose(threadHandle);
            AtDeleteTarget(Target);
            return STATUS_INVALID_CID;
        }

        PhDereferenceObject(liveCreateTime);
        PhDereferenceObject(createTime);
    }

    Target->Kind = AtTargetThread;
    Target->ThreadId = UlongToHandle((ULONG)threadId);
    Target->ThreadHandle = threadHandle;
    Target->Identity[2] = threadId;

    return STATUS_SUCCESS;
}

NTSTATUS AtpResolveServiceTarget(
    _In_opt_ PVOID Arguments,
    _In_ ACCESS_MASK ServiceAccess,
    _Out_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    PPH_STRING name;
    PPH_SERVICE_ITEM serviceItem;
    SC_HANDLE serviceHandle = NULL;

    memset(Target, 0, sizeof(AT_TARGET));

    if (!(name = AtGetArgumentString(Arguments, "name")) || name->Length == 0)
    {
        AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER, L"需要提供 name，且必须是服务名(而非显示名称)。");
        PhClearReference(&name);
        return STATUS_INVALID_PARAMETER;
    }

    if (!(serviceItem = PhReferenceServiceItem(&name->sr)))
    {
        AtSetToolError(Result, "not_found", STATUS_NOT_FOUND, L"提供程序缓存中没有名为 %s 的服务。请使用 list_services 查找服务名。", PhGetString(name));
        PhDereferenceObject(name);
        return STATUS_NOT_FOUND;
    }

    if (ServiceAccess)
    {
        NTSTATUS status;

        status = PhOpenService(&serviceHandle, ServiceAccess, PhGetString(serviceItem->Name));

        if (!NT_SUCCESS(status))
        {
            AtSetToolStatusError(Result, status, L"打开服务");
            PhDereferenceObject(serviceItem);
            PhDereferenceObject(name);
            return status;
        }
    }

    Target->Kind = AtTargetService;
    Target->ServiceItem = serviceItem;
    Target->ServiceHandle = serviceHandle;
    Target->Identity[0] = PhHashStringRefEx(&serviceItem->Name->sr, TRUE, PH_STRING_HASH_X65599);

    PhDereferenceObject(name);

    return STATUS_SUCCESS;
}

NTSTATUS AtResolveHandleTarget(
    _In_opt_ PVOID Arguments,
    _In_ BOOLEAN RequireSequenceNumber,
    _In_ ACCESS_MASK ProcessAccess,
    _Out_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    NTSTATUS status;
    ULONG64 handleValue;
    PPH_STRING expectedType = NULL;
    PSYSTEM_HANDLE_INFORMATION_EX handles;
    PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX entry = NULL;
    ULONG_PTR i;
    PPH_STRING typeName = NULL;
    PPH_STRING bestName = NULL;

    memset(Target, 0, sizeof(AT_TARGET));

    if (!AtGetArgumentPointer(Arguments, "handle", &handleValue) || handleValue == 0)
    {
        AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER, L"需要提供 handle；请传入从 get_process_handles 获取的句柄值。");
        return STATUS_INVALID_PARAMETER;
    }

    status = AtResolveProcessTarget(Arguments, RequireSequenceNumber, ProcessAccess, Target, Result);

    if (!NT_SUCCESS(status))
        return status;

    status = PhEnumHandlesEx(&handles);

    if (!NT_SUCCESS(status))
    {
        AtSetToolStatusError(Result, status, L"枚举句柄");
        AtDeleteTarget(Target);
        return status;
    }

    for (i = 0; i < handles->NumberOfHandles; i++)
    {
        if (handles->Handles[i].UniqueProcessId == Target->ProcessItem->ProcessId &&
            (ULONG64)(ULONG_PTR)handles->Handles[i].HandleValue == handleValue)
        {
            entry = &handles->Handles[i];
            break;
        }
    }

    if (!entry)
    {
        AtSetToolError(Result, "not_found", STATUS_NOT_FOUND, L"pid %lu 没有句柄 0x%llx。", HandleToUlong(Target->ProcessItem->ProcessId), handleValue);
        PhFree(handles);
        AtDeleteTarget(Target);
        return STATUS_NOT_FOUND;
    }

    Target->HandleValue = entry->HandleValue;
    Target->HandleTypeIndex = entry->ObjectTypeIndex;
    Target->HandleAttributes = entry->HandleAttributes;
    Target->HandleObject = entry->Object;
    Target->Identity[2] = handleValue;
    Target->Identity[3] = (ULONG64)(ULONG_PTR)entry->Object;
    PhFree(handles);

    // Names come from the object itself, so the user is shown what the handle refers to.
    PhGetHandleInformation(
        Target->ProcessHandle,
        Target->HandleValue,
        Target->HandleTypeIndex,
        NULL,
        &typeName,
        NULL,
        &bestName
        );

    // Naming the object needs the object, which needs PROCESS_DUP_HANDLE or the driver. The type
    // does not: it is in the handle table entry, and a caller that only asked to read still gets it.
    if (!typeName)
        typeName = PhGetObjectTypeIndexName(Target->HandleTypeIndex);

    Target->HandleTypeName = typeName;
    Target->HandleObjectName = bestName;

    if (expectedType = AtGetArgumentString(Arguments, "type_name"))
    {
        if (!typeName || !PhEqualString(typeName, expectedType, TRUE))
        {
            AtSetToolError(
                Result,
                "identity_mismatch",
                STATUS_OBJECT_TYPE_MISMATCH,
                L"句柄 0x%llx 是 %s 句柄，而非 %s。请重新列出句柄后重试。",
                handleValue,
                PhGetStringOrDefault(typeName, L"(未知类型)"),
                PhGetString(expectedType)
                );
            PhDereferenceObject(expectedType);
            AtDeleteTarget(Target);
            return STATUS_OBJECT_TYPE_MISMATCH;
        }

        PhDereferenceObject(expectedType);
    }

    Target->Kind = AtTargetHandle;

    return STATUS_SUCCESS;
}

NTSTATUS AtpResolveDeviceTarget(
    _In_opt_ PVOID Arguments,
    _Out_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    PPH_STRING instanceId;
    PPH_DEVICE_TREE tree;
    PPH_DEVICE_ITEM item;
    PPH_DEVICE_PROPERTY property;

    memset(Target, 0, sizeof(AT_TARGET));

    if (!(instanceId = AtGetArgumentString(Arguments, "instance_id")))
    {
        AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER,
            L"需要提供 instance_id；list_devices 会报告每个设备的实例 ID。");
        return STATUS_INVALID_PARAMETER;
    }

    if (!(tree = PhReferenceDeviceTree()))
    {
        AtSetToolError(Result, "failed", STATUS_UNSUCCESSFUL, L"设备树不可用。");
        PhDereferenceObject(instanceId);
        return STATUS_UNSUCCESSFUL;
    }

    if (!(item = PhLookupDeviceItem(tree, &instanceId->sr)))
    {
        AtSetToolError(Result, "not_found", STATUS_NOT_FOUND,
            L"没有设备具有该实例 ID；list_devices 会报告现存的设备。");
        PhDereferenceObject(tree);
        PhDereferenceObject(instanceId);
        return STATUS_NOT_FOUND;
    }

    Target->Kind = AtTargetDevice;
    Target->DeviceInstanceId = instanceId;

    // The name is what the user is shown, so a device with no name still has to read as something:
    // the instance id stands in rather than the prompt naming nothing.
    if ((property = PhGetDeviceProperty(item, PhDevicePropertyName)) && property->Valid && property->AsString)
        Target->DeviceName = PhReferenceObject(property->AsString);
    else
        Target->DeviceName = PhReferenceObject(instanceId);

    if ((property = PhGetDeviceProperty(item, PhDevicePropertyClass)) && property->Valid && property->AsString)
        Target->DeviceClass = PhReferenceObject(property->AsString);

    // The instance id identifies the device; it is what the write is applied to.
    Target->Identity[0] = PhHashStringRefEx(&instanceId->sr, TRUE, PH_STRING_HASH_X65599);

    PhDereferenceObject(tree);

    return STATUS_SUCCESS;
}

NTSTATUS AtpResolveConnectionTarget(
    _In_opt_ PVOID Arguments,
    _Out_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    NTSTATUS status;
    PPH_NETWORK_ITEM networkItem;
    PPH_STRING local;
    PPH_STRING remote;

    status = AtResolveProcessTarget(Arguments, TRUE, 0, Target, Result);

    if (!NT_SUCCESS(status))
        return status;

    status = AtFindNetworkConnection(Arguments, Target->ProcessItem, &networkItem, Result);

    if (!NT_SUCCESS(status))
    {
        AtDeleteTarget(Target);
        return status;
    }

    local = AtFormatNetworkEndpoint(&networkItem->LocalEndpoint, networkItem->ProtocolType, networkItem->LocalScopeId, TRUE);
    remote = AtFormatNetworkEndpoint(&networkItem->RemoteEndpoint, networkItem->ProtocolType, networkItem->RemoteScopeId, TRUE);

    Target->Kind = AtTargetConnection;
    Target->NetworkItem = networkItem;
    Target->ConnectionText = PhFormatString(
        L"%s %s -> %s",
        AtProtocolTypeString(networkItem->ProtocolType),
        PhGetString(local),
        PhGetString(remote)
        );
    Target->Identity[2] = ((ULONG64)networkItem->ProtocolType << 32) | PhHashStringRefEx(&local->sr, TRUE, PH_STRING_HASH_X65599);
    Target->Identity[3] = PhHashStringRefEx(&remote->sr, TRUE, PH_STRING_HASH_X65599);

    PhDereferenceObject(local);
    PhDereferenceObject(remote);

    return STATUS_SUCCESS;
}

NTSTATUS AtpResolveTargetParameter(
    _In_ PCAT_TOOL Tool,
    _In_opt_ PVOID Arguments,
    _Out_ PPH_STRING* Parameter,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    PPH_STRING value;
    PPH_STRING text = NULL;

    *Parameter = NULL;

    switch (Tool->Action)
    {
    case AtActionSetProcessPriority:
        {
            ULONG priorityClass;

            value = AtGetArgumentString(Arguments, "priority_class");

            if (!AtParsePriorityClass(value, &priorityClass))
            {
                AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER, L"priority_class 必须是 idle、below_normal、normal、above_normal、high、realtime 之一。");
                PhClearReference(&value);
                return STATUS_INVALID_PARAMETER;
            }

            text = PhCreateString(AtPriorityClassString(priorityClass));
            PhClearReference(&value);
        }
        break;
    case AtActionSetProcessIoPriority:
        {
            IO_PRIORITY_HINT ioPriority;

            value = AtGetArgumentString(Arguments, "io_priority");

            if (!AtParseIoPriority(value, &ioPriority))
            {
                AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER, L"io_priority 必须是 very_low、low、normal、high 之一。");
                PhClearReference(&value);
                return STATUS_INVALID_PARAMETER;
            }

            text = PhCreateString(AtIoPriorityString(ioPriority));
            PhClearReference(&value);
        }
        break;
    case AtActionSetProcessAffinity:
        {
            SYSTEM_BASIC_INFORMATION basicInfo;
            ULONG64 mask = 0;
            ULONG64 group = 0;
            BOOLEAN hasGroup;

            if (!AtGetArgumentUInt64(Arguments, "affinity_mask", &mask) || mask == 0)
            {
                AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER,
                    L"需要提供 affinity_mask，且必须指定至少一个处理器；掩码为零不是“无限制”，而是无法在任何处理器上运行的进程。");
                return STATUS_INVALID_PARAMETER;
            }

            hasGroup = AtGetArgumentUInt64(Arguments, "group", &group);

            // Only the processors of one group can be in a mask, so the system's own set is the
            // right fence only for a call that did not name a group.
            memset(&basicInfo, 0, sizeof(basicInfo));

            if (!hasGroup &&
                NT_SUCCESS(NtQuerySystemInformation(SystemBasicInformation, &basicInfo, sizeof(basicInfo), NULL)) &&
                (mask & ~(ULONG64)basicInfo.ActiveProcessorsAffinityMask) != 0)
            {
                AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER,
                    L"affinity_mask 指定了该计算机不存在的处理器；当前活动处理器为 0x%I64x。",
                    (ULONG64)basicInfo.ActiveProcessorsAffinityMask);
                return STATUS_INVALID_PARAMETER;
            }

            if (hasGroup && group > MAXUSHORT)
            {
                AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER, L"group 超出范围。");
                return STATUS_INVALID_PARAMETER;
            }

            if (hasGroup)
                text = PhFormatString(L"组 %I64u 掩码 0x%I64x", group, mask);
            else
                text = PhFormatString(L"掩码 0x%I64x", mask);
        }
        break;
    case AtActionSetDeviceEnabled:
        {
            value = AtGetArgumentString(Arguments, "instance_id");

            if (PhIsNullOrEmptyString(value))
            {
                AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER,
                    L"需要提供 instance_id；请从 list_devices 获取。");
                PhClearReference(&value);
                return STATUS_INVALID_PARAMETER;
            }

            if (!AtJsonGetObjectMember(Arguments, "enabled", PH_JSON_OBJECT_TYPE_BOOLEAN))
            {
                AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER,
                    L"需要提供 enabled：true 启用设备，false 禁用设备。");
                PhClearReference(&value);
                return STATUS_INVALID_PARAMETER;
            }

            // Just the direction: the headline names the device and the description carries its
            // instance id, so repeating the id here would push the one word that matters off the end.
            text = PhCreateString(AtJsonGetObjectBoolean(Arguments, "enabled") ? L"启用" : L"禁用");
            PhClearReference(&value);
        }
        break;
    case AtActionCloseWindow:
    case AtActionSetWindowState:
        {
            ULONG64 handleValue = 0;
            ULONG showCommand;
            BOOLEAN foreground;

            if (!AtGetArgumentPointer(Arguments, "handle", &handleValue) || handleValue == 0)
            {
                AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER,
                    L"需要提供 handle；请从 list_windows 或 get_process_windows 获取。");
                return STATUS_INVALID_PARAMETER;
            }

            if (Tool->Action == AtActionSetWindowState)
            {
                value = AtGetArgumentString(Arguments, "state");

                if (!AtParseWindowState(value, &showCommand, &foreground))
                {
                    AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER,
                        L"state 必须是 show、hide、minimize、maximize、restore、foreground 之一。");
                    PhClearReference(&value);
                    return STATUS_INVALID_PARAMETER;
                }

                // The window is named in the approval as well as the change: the process alone
                // does not say which of its windows this is about.
                text = PhFormatString(L"窗口 0x%I64x %s", handleValue, value->Buffer);
                PhClearReference(&value);
            }
            else
            {
                text = PhFormatString(L"窗口 0x%I64x", handleValue);
            }
        }
        break;
    case AtActionSetProcessPagePriority:
        {
            ULONG64 pagePriority = 0;

            if (!AtGetArgumentUInt64(Arguments, "page_priority", &pagePriority) ||
                pagePriority > MEMORY_PRIORITY_NORMAL)
            {
                AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER,
                    L"需要提供 page_priority，且必须为 0 到 5：0 最低、1 极低、2 低、3 中、4 低于正常、5 正常。");
                return STATUS_INVALID_PARAMETER;
            }

            text = PhFormatString(L"页优先级 %I64u (%s)", pagePriority,
                AtPagePriorityString((ULONG)pagePriority));
        }
        break;
    case AtActionSetServiceConfig:
        {
            text = AtFormatServiceConfigParameter(Arguments, Result);

            if (!text)
                return STATUS_INVALID_PARAMETER;
        }
        break;
    case AtActionCreateProcessMinidump:
        {
            value = AtGetArgumentString(Arguments, "path");

            if (!value || PhDetermineDosPathNameType(&value->sr) != RtlPathTypeDriveAbsolute)
            {
                AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER, L"需要提供 path，且必须是绝对驱动器路径，例如 C:\\dumps\\process.dmp。");
                PhClearReference(&value);
                return STATUS_INVALID_PARAMETER;
            }

            text = value;
        }
        break;
    default:
        return STATUS_SUCCESS;
    }

    AtSanitizeDisplayString(text);

    *Parameter = text;

    return STATUS_SUCCESS;
}

NTSTATUS AtResolveTarget(
    _In_ PCAT_TOOL Tool,
    _In_opt_ PVOID Arguments,
    _Out_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    NTSTATUS status;
    PCAT_ACTION_INFO action = &AtActionInfo[Tool->Action];
    BOOLEAN requireSequenceNumber = Tool->Tier == AtTierWrite;
    PPH_STRING parameter = NULL;

    memset(Target, 0, sizeof(AT_TARGET));

    // The arguments are judged before the object is opened: a write naming a change that makes no
    // sense is wrong whether or not the caller could have made it, and access_denied would send
    // them looking for a permission.
    status = AtpResolveTargetParameter(Tool, Arguments, &parameter, Result);

    if (!NT_SUCCESS(status))
        return status;

    switch (action->TargetKind)
    {
    case AtTargetNone:
        status = STATUS_SUCCESS;
        break;
    case AtTargetProcess:
        status = AtResolveProcessTarget(Arguments, requireSequenceNumber, action->TargetAccess, Target, Result);
        break;
    case AtTargetThread:
        status = AtpResolveThreadTarget(Arguments, requireSequenceNumber, action->TargetAccess, Tool->Tier != AtTierWrite, Target, Result);
        break;
    case AtTargetService:
        status = AtpResolveServiceTarget(Arguments, action->TargetAccess, Target, Result);
        break;
    case AtTargetHandle:
        status = AtResolveHandleTarget(Arguments, requireSequenceNumber, action->TargetAccess, Target, Result);
        break;
    case AtTargetConnection:
        status = AtpResolveConnectionTarget(Arguments, Target, Result);
        break;
    case AtTargetDevice:
        status = AtpResolveDeviceTarget(Arguments, Target, Result);
        break;
    default:
        status = STATUS_NOT_IMPLEMENTED;
        break;
    }

    if (!NT_SUCCESS(status))
    {
        PhClearReference(&parameter);
        AtDeleteTarget(Target);
        return status;
    }

    // The parameter is part of what the user approves, so it is folded into the identity here -
    // after the target resolution, which clears the target and sets the rest of it.
    if (parameter)
    {
        Target->Identity[3] ^= (ULONG64)PhHashStringRefEx(&parameter->sr, TRUE, PH_STRING_HASH_X65599) << 32;
        PhMoveReference(&Target->Parameter, parameter);
    }

    return STATUS_SUCCESS;
}

VOID AtDeleteTarget(
    _Inout_ PAT_TARGET Target
    )
{
    if (Target->ThreadHandle)
        NtClose(Target->ThreadHandle);
    if (Target->ProcessHandle)
        NtClose(Target->ProcessHandle);
    if (Target->ServiceHandle)
        PhCloseServiceHandle(Target->ServiceHandle);

    PhClearReference(&Target->ProcessItem);
    PhClearReference(&Target->ServiceItem);
    PhClearReference(&Target->HandleTypeName);
    PhClearReference(&Target->HandleObjectName);
    PhClearReference(&Target->ConnectionText);
    PhClearReference(&Target->DeviceInstanceId);
    PhClearReference(&Target->DeviceName);
    PhClearReference(&Target->DeviceClass);
    PhClearReference(&Target->Parameter);

    if (Target->NetworkItem)
        PhFree(Target->NetworkItem);

    memset(Target, 0, sizeof(AT_TARGET));
}

VOID AtSetTargetParameter(
    _Inout_ PAT_TARGET Target,
    _In_ PCWSTR Parameter
    )
{
    PhMoveReference(&Target->Parameter, PhCreateString(Parameter));
}

PPH_STRING AtpFormatProcessHeadline(
    _In_ PPH_PROCESS_ITEM ProcessItem
    )
{
    return PhFormatString(
        L"%s (PID %lu)",
        PhGetStringOrDefault(ProcessItem->ProcessName, L"(未命名)"),
        HandleToUlong(ProcessItem->ProcessId)
        );
}

PPH_STRING AtFormatTargetHeadline(
    _In_ PAT_TARGET Target
    )
{
    PPH_STRING process = NULL;
    PPH_STRING result;

    if (Target->ProcessItem)
        process = AtpFormatProcessHeadline(Target->ProcessItem);

    switch (Target->Kind)
    {
    case AtTargetProcess:
        result = PhReferenceObject(process);
        break;
    case AtTargetThread:
        result = PhFormatString(L"%s 的线程 %lu", PhGetString(process), HandleToUlong(Target->ThreadId));
        break;
    case AtTargetService:
        if (Target->ServiceItem->DisplayName && !PhEqualString(Target->ServiceItem->DisplayName, Target->ServiceItem->Name, TRUE))
            result = PhFormatString(L"服务 %s (%s)", PhGetString(Target->ServiceItem->Name), PhGetString(Target->ServiceItem->DisplayName));
        else
            result = PhFormatString(L"服务 %s", PhGetString(Target->ServiceItem->Name));
        break;
    case AtTargetHandle:
        result = PhFormatString(
            L"句柄 0x%Ix (%s)，位于 %s",
            (ULONG_PTR)Target->HandleValue,
            PhGetStringOrDefault(Target->HandleTypeName, L"未知类型"),
            PhGetString(process)
            );
        break;
    case AtTargetConnection:
        result = PhFormatString(L"%s 的连接 %s", PhGetString(process), PhGetString(Target->ConnectionText));
        break;
    case AtTargetDevice:
        result = PhFormatString(L"设备 %s", PhGetString(Target->DeviceName));
        break;
    default:
        result = PhCreateString(L"(无目标)");
        break;
    }

    PhClearReference(&process);

    AtSanitizeDisplayString(result);

    return result;
}

VOID AtpAppendDisplayValue(
    _Inout_ PPH_STRING_BUILDER Builder,
    _In_ PCWSTR Prefix,
    _In_ PCWSTR Value
    )
{
    PPH_STRING text;

    PhAppendStringBuilder2(Builder, Prefix);

    text = PhCreateString(Value);
    AtSanitizeDisplayString(text);
    PhAppendStringBuilder(Builder, &text->sr);
    PhDereferenceObject(text);
}

VOID AtpAppendProcessDescription(
    _Inout_ PPH_STRING_BUILDER Builder,
    _In_ PPH_PROCESS_ITEM ProcessItem
    )
{
    PhAppendFormatStringBuilder(Builder, L"\n序列：%I64u", ProcessItem->ProcessSequenceNumber);
    AtpAppendDisplayValue(Builder, L"\n映像：", PhGetStringOrDefault(ProcessItem->FileName, L"(未知)"));

    if (ProcessItem->VerifyResult == VrTrusted)
    {
        AtpAppendDisplayValue(Builder, L"\n签名者：受信任 (", PhGetStringOrDefault(ProcessItem->VerifySignerName, L"未知"));
        PhAppendStringBuilder2(Builder, L")");
    }
    else if (ProcessItem->VerifyResult == VrUnknown)
    {
        PhAppendStringBuilder2(Builder, L"\n签名者：未验证");
    }
    else
    {
        PhAppendStringBuilder2(Builder, L"\n签名者：不受信任");
    }

    if (ProcessItem->UserName)
        AtpAppendDisplayValue(Builder, L"\n用户：", PhGetString(ProcessItem->UserName));
}

PPH_STRING AtFormatTargetDescription(
    _In_ PAT_TARGET Target
    )
{
    PH_STRING_BUILDER builder;
    PPH_STRING headline;

    PhInitializeStringBuilder(&builder, 256);

    headline = AtFormatTargetHeadline(Target);
    PhAppendStringBuilder(&builder, &headline->sr);
    PhDereferenceObject(headline);

    switch (Target->Kind)
    {
    case AtTargetService:
        {
            PPH_SERVICE_ITEM serviceItem = Target->ServiceItem;

            PhAppendFormatStringBuilder(&builder, L"\n状态：%s", PhGetServiceStateString(serviceItem->State)->Buffer);
            PhAppendFormatStringBuilder(&builder, L"\n启动类型：%s", PhGetServiceStartTypeString(serviceItem->StartType)->Buffer);
            AtpAppendDisplayValue(&builder, L"\n映像：", PhGetStringOrDefault(serviceItem->FileName, L"(未知)"));

            if (serviceItem->VerifyResult == VrTrusted)
            {
                AtpAppendDisplayValue(&builder, L"\n签名者：受信任 (", PhGetStringOrDefault(serviceItem->VerifySignerName, L"未知"));
                PhAppendStringBuilder2(&builder, L")");
            }
            else if (serviceItem->VerifyResult == VrUnknown)
            {
                PhAppendStringBuilder2(&builder, L"\n签名者：未验证");
            }
            else
            {
                PhAppendStringBuilder2(&builder, L"\n签名者：不受信任");
            }

            if (serviceItem->ProcessId)
                PhAppendFormatStringBuilder(&builder, L"\nPID：%lu", HandleToUlong(serviceItem->ProcessId));
        }
        break;
    case AtTargetHandle:
        {
            AtpAppendDisplayValue(&builder, L"\n对象：", PhGetStringOrDefault(Target->HandleObjectName, L"(未命名)"));
            AtpAppendProcessDescription(&builder, Target->ProcessItem);
        }
        break;
    case AtTargetConnection:
        {
            PhAppendFormatStringBuilder(&builder, L"\n状态：%s", PhGetTcpStateName(Target->NetworkItem->State)->Buffer);
            AtpAppendProcessDescription(&builder, Target->ProcessItem);
        }
        break;
    case AtTargetDevice:
        {
            if (Target->DeviceClass)
                AtpAppendDisplayValue(&builder, L"\n类：", PhGetString(Target->DeviceClass));

            AtpAppendDisplayValue(&builder, L"\n实例：", PhGetString(Target->DeviceInstanceId));
        }
        break;
    default:
        {
            if (Target->ProcessItem)
                AtpAppendProcessDescription(&builder, Target->ProcessItem);
        }
        break;
    }

    return PhFinalStringBuilderString(&builder);
}

PPH_STRING AtFormatTargetAudit(
    _In_ PAT_TARGET Target
    )
{
    PPH_STRING headline;
    PPH_STRING result;

    headline = AtFormatTargetHeadline(Target);

    if (Target->ProcessItem)
    {
        result = PhFormatString(
            L"%s [序列 %I64u, %s]",
            PhGetString(headline),
            Target->ProcessItem->ProcessSequenceNumber,
            PhGetStringOrDefault(Target->ProcessItem->FileName, L"未知映像")
            );
    }
    else if (Target->Kind == AtTargetService)
    {
        result = PhFormatString(
            L"%s [%s]",
            PhGetString(headline),
            PhGetStringOrDefault(Target->ServiceItem->FileName, L"未知映像")
            );
    }
    else
    {
        result = PhReferenceObject(headline);
    }

    PhDereferenceObject(headline);
    AtSanitizeDisplayString(result);

    return result;
}
