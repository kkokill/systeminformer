/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2016
 *     dmex    2017-2023
 *
 */

#include <phapp.h>
#include <procmtgn.h>

NTSTATUS PhpCopyProcessMitigationPolicy(
    _Inout_ PNTSTATUS Status,
    _In_ HANDLE ProcessHandle,
    _In_ PROCESS_MITIGATION_POLICY Policy,
    _In_ SIZE_T Offset,
    _In_ SIZE_T Size,
    _Out_writes_bytes_(Size) PVOID Destination
    )
{
    NTSTATUS status;
    PROCESS_MITIGATION_POLICY_INFORMATION policyInfo;

    policyInfo.Policy = Policy;
    status = NtQueryInformationProcess(
        ProcessHandle,
        ProcessMitigationPolicy,
        &policyInfo,
        sizeof(PROCESS_MITIGATION_POLICY_INFORMATION),
        NULL
        );

    if (!NT_SUCCESS(status))
    {
        if (*Status == STATUS_NONE_MAPPED)
            *Status = status;
        return status;
    }

    memcpy(Destination, PTR_ADD_OFFSET(&policyInfo, Offset), Size);
    *Status = STATUS_SUCCESS;

    return status;
}

NTSTATUS PhGetProcessMitigationPolicyAllInformation(
    _In_ HANDLE ProcessHandle,
    _Out_ PPH_PROCESS_MITIGATION_POLICY_ALL_INFORMATION Information
    )
{
    NTSTATUS status = STATUS_NONE_MAPPED;
    NTSTATUS subStatus;
#ifdef _WIN64
    BOOLEAN isWow64;
#endif
    ULONG depStatus;

    memset(Information, 0, sizeof(PH_PROCESS_MITIGATION_POLICY_ALL_INFORMATION));

#ifdef _WIN64
    if (NT_SUCCESS(subStatus = PhGetProcessIsWow64(ProcessHandle, &isWow64)) && !isWow64)
    {
        depStatus = PH_PROCESS_DEP_ENABLED | PH_PROCESS_DEP_PERMANENT;
    }
    else
    {
#endif
        subStatus = PhGetProcessDepStatus(ProcessHandle, &depStatus);
#ifdef _WIN64
    }
#endif

    if (NT_SUCCESS(subStatus))
    {
        status = STATUS_SUCCESS;
        Information->DEPPolicy.Enable = !!(depStatus & PH_PROCESS_DEP_ENABLED);
        Information->DEPPolicy.DisableAtlThunkEmulation = !!(depStatus & PH_PROCESS_DEP_ATL_THUNK_EMULATION_DISABLED);
        Information->DEPPolicy.Permanent = !!(depStatus & PH_PROCESS_DEP_PERMANENT);
        Information->Pointers[ProcessDEPPolicy] = &Information->DEPPolicy;
    }
    else if (status == STATUS_NONE_MAPPED)
    {
        status = subStatus;
    }

#define COPY_PROCESS_MITIGATION_POLICY(PolicyName, StructName) \
    if (NT_SUCCESS(PhpCopyProcessMitigationPolicy(&status, ProcessHandle, Process##PolicyName##Policy, \
        UFIELD_OFFSET(PROCESS_MITIGATION_POLICY_INFORMATION, PolicyName##Policy), \
        sizeof(StructName), \
        &Information->PolicyName##Policy))) \
    { \
        Information->Pointers[Process##PolicyName##Policy] = &Information->PolicyName##Policy; \
    }

    COPY_PROCESS_MITIGATION_POLICY(ASLR, PROCESS_MITIGATION_ASLR_POLICY);
    COPY_PROCESS_MITIGATION_POLICY(DynamicCode, PROCESS_MITIGATION_DYNAMIC_CODE_POLICY);
    COPY_PROCESS_MITIGATION_POLICY(StrictHandleCheck, PROCESS_MITIGATION_STRICT_HANDLE_CHECK_POLICY);
    COPY_PROCESS_MITIGATION_POLICY(SystemCallDisable, PROCESS_MITIGATION_SYSTEM_CALL_DISABLE_POLICY);
    COPY_PROCESS_MITIGATION_POLICY(ExtensionPointDisable, PROCESS_MITIGATION_EXTENSION_POINT_DISABLE_POLICY);
    COPY_PROCESS_MITIGATION_POLICY(ControlFlowGuard, PROCESS_MITIGATION_CONTROL_FLOW_GUARD_POLICY);
    COPY_PROCESS_MITIGATION_POLICY(Signature, PROCESS_MITIGATION_BINARY_SIGNATURE_POLICY);
    COPY_PROCESS_MITIGATION_POLICY(FontDisable, PROCESS_MITIGATION_FONT_DISABLE_POLICY);
    COPY_PROCESS_MITIGATION_POLICY(ImageLoad, PROCESS_MITIGATION_IMAGE_LOAD_POLICY);
    COPY_PROCESS_MITIGATION_POLICY(SystemCallFilter, PROCESS_MITIGATION_SYSTEM_CALL_FILTER_POLICY); // REDSTONE3
    COPY_PROCESS_MITIGATION_POLICY(PayloadRestriction, PROCESS_MITIGATION_PAYLOAD_RESTRICTION_POLICY);
    COPY_PROCESS_MITIGATION_POLICY(ChildProcess, PROCESS_MITIGATION_CHILD_PROCESS_POLICY);
    COPY_PROCESS_MITIGATION_POLICY(SideChannelIsolation, PROCESS_MITIGATION_SIDE_CHANNEL_ISOLATION_POLICY); // 19H1
    COPY_PROCESS_MITIGATION_POLICY(UserShadowStack, PROCESS_MITIGATION_USER_SHADOW_STACK_POLICY); // 20H1
    COPY_PROCESS_MITIGATION_POLICY(RedirectionTrust, PROCESS_MITIGATION_REDIRECTION_TRUST_POLICY); // 22H1
    COPY_PROCESS_MITIGATION_POLICY(UserPointerAuth, PROCESS_MITIGATION_USER_POINTER_AUTH_POLICY);
    COPY_PROCESS_MITIGATION_POLICY(SEHOP, PROCESS_MITIGATION_SEHOP_POLICY);
    COPY_PROCESS_MITIGATION_POLICY(ActivationContextTrust, PROCESS_MITIGATION_ACTIVATION_CONTEXT_TRUST_POLICY2);

    return status;
}

_Success_(return)
BOOLEAN PhDescribeProcessMitigationPolicy(
    _In_ PROCESS_MITIGATION_POLICY Policy,
    _In_ PVOID Data,
    _Out_opt_ PPH_STRING *ShortDescription,
    _Out_opt_ PPH_STRING *LongDescription
    )
{
    BOOLEAN result = FALSE;
    PH_STRING_BUILDER sb;

    switch (Policy)
    {
    case ProcessDEPPolicy:
        {
            PPROCESS_MITIGATION_DEP_POLICY data = Data;

            if (data->Enable)
            {
                if (ShortDescription)
                {
                    PhInitializeStringBuilder(&sb, 20);
                    PhAppendStringBuilder2(&sb, L"DEP");
                    if (data->Permanent) PhAppendStringBuilder2(&sb, L" (永久)");
                    *ShortDescription = PhFinalStringBuilderString(&sb);
                }

                if (LongDescription)
                {
                    PhInitializeStringBuilder(&sb, 50);
                    PhAppendFormatStringBuilder(&sb, PhTranslateTextZ(L"此进程已%s启用数据执行保护 (DEP)。\r\n"), data->Permanent ? PhTranslateTextZ(L"永久") : L"");
                    if (data->DisableAtlThunkEmulation) PhAppendStringBuilder2(&sb, PhTranslateTextZ(L"ATL thunk 模拟已禁用。\r\n"));
                    *LongDescription = PhFinalStringBuilderString(&sb);
                }

                result = TRUE;
            }
        }
        break;
    case ProcessASLRPolicy:
        {
            PPROCESS_MITIGATION_ASLR_POLICY data = Data;

            if (data->EnableBottomUpRandomization || data->EnableForceRelocateImages || data->EnableHighEntropy)
            {
                if (ShortDescription)
                {
                    PhInitializeStringBuilder(&sb, 20);
                    PhAppendStringBuilder2(&sb, L"ASLR");

                    if (data->EnableHighEntropy || data->EnableForceRelocateImages)
                    {
                        PhAppendStringBuilder2(&sb, L" (");
                        if (data->EnableHighEntropy) PhAppendStringBuilder2(&sb, L"高熵, ");
                        if (data->EnableForceRelocateImages) PhAppendStringBuilder2(&sb, L"强制重定位, ");
                        if (data->DisallowStrippedImages) PhAppendStringBuilder2(&sb, L"禁止剥离, ");
                        if (PhEndsWithStringRef2(&sb.String->sr, L", ", FALSE)) PhRemoveEndStringBuilder(&sb, 2);
                        PhAppendCharStringBuilder(&sb, L')');
                    }

                    *ShortDescription = PhFinalStringBuilderString(&sb);
                }

                if (LongDescription)
                {
                    PhInitializeStringBuilder(&sb, 100);
                    PhAppendStringBuilder2(&sb, L"此进程已启用地址空间布局随机化。\r\n");
                    if (data->EnableHighEntropy) PhAppendStringBuilder2(&sb, L"高熵随机化已启用。\r\n");
                    if (data->EnableForceRelocateImages) PhAppendStringBuilder2(&sb, L"所有映像被强制重定位（无论其是否支持 ASLR）。\r\n");
                    if (data->DisallowStrippedImages) PhAppendStringBuilder2(&sb, L"不允许加载剥离了重定位数据的映像。\r\n");
                    *LongDescription = PhFinalStringBuilderString(&sb);
                }

                result = TRUE;
            }
        }
        break;
    case ProcessDynamicCodePolicy:
        {
            PPROCESS_MITIGATION_DYNAMIC_CODE_POLICY data = Data;

            if (data->ProhibitDynamicCode)
            {
                if (ShortDescription)
                    *ShortDescription = PhCreateString(L"动态代码已禁止");

                if (LongDescription)
                    *LongDescription = PhCreateString(L"不允许执行动态加载的代码。\r\n");

                result = TRUE;
            }

            if (data->AllowThreadOptOut)
            {
                if (ShortDescription)
                    *ShortDescription = PhCreateString(L"动态代码已禁止（按线程）");

                if (LongDescription)
                    *LongDescription = PhCreateString(L"允许个别线程不受动态代码生成限制的约束。\r\n");

                result = TRUE;
            }

            if (data->AllowRemoteDowngrade)
            {
                if (ShortDescription)
                    *ShortDescription = PhCreateString(L"动态代码可降级");

                if (LongDescription)
                    *LongDescription = PhCreateString(L"允许非 AppContainer 进程修改调用进程的全部动态代码设置，包括在设置后放宽动态代码限制。\r\n");

                result = TRUE;
            }
        }
        break;
    case ProcessStrictHandleCheckPolicy:
        {
            PPROCESS_MITIGATION_STRICT_HANDLE_CHECK_POLICY data = Data;

            if (data->RaiseExceptionOnInvalidHandleReference)
            {
                if (ShortDescription)
                    *ShortDescription = PhCreateString(L"严格句柄检查");

                if (LongDescription)
                    *LongDescription = PhCreateString(L"进程使用无效句柄时会引发异常。\r\n");

                result = TRUE;
            }
        }
        break;
    case ProcessSystemCallDisablePolicy:
        {
            PPROCESS_MITIGATION_SYSTEM_CALL_DISABLE_POLICY data = Data;

            if (data->DisallowWin32kSystemCalls)
            {
                if (ShortDescription)
                    *ShortDescription = PhCreateString(L"Win32k 系统调用已禁用");

                if (LongDescription)
                    *LongDescription = PhCreateString(L"不允许 Win32k (GDI/USER) 系统调用。\r\n");

                result = TRUE;
            }

            if (data->AuditDisallowWin32kSystemCalls)
            {
                if (ShortDescription)
                    *ShortDescription = PhCreateString(L"Win32k 系统调用（审核）");

                if (LongDescription)
                    *LongDescription = PhCreateString(L"Win32k (GDI/USER) 系统调用将触发 ETW 事件。\r\n");

                result = TRUE;
            }
        }
        break;
    case ProcessExtensionPointDisablePolicy:
        {
            PPROCESS_MITIGATION_EXTENSION_POINT_DISABLE_POLICY data = Data;

            if (data->DisableExtensionPoints)
            {
                if (ShortDescription)
                    *ShortDescription = PhCreateString(L"扩展点已禁用");

                if (LongDescription)
                    *LongDescription = PhCreateString(L"无法将旧版扩展点 DLL 加载到进程中。注意：uiAccess=true 的进程会自动绕过此策略并注入旧版扩展点 DLL。\r\n");

                result = TRUE;
            }
        }
        break;
    case ProcessControlFlowGuardPolicy:
        {
            PPROCESS_MITIGATION_CONTROL_FLOW_GUARD_POLICY data = Data;

            if (data->EnableControlFlowGuard)
            {
                if (ShortDescription)
                {
                    PhInitializeStringBuilder(&sb, 50);
                    if (data->StrictMode) PhAppendStringBuilder2(&sb, L"严格 ");

#if !defined(NTDDI_WIN10_CO) || (NTDDI_VERSION < NTDDI_WIN10_CO)
                    if (_bittest((const PLONG)&data->Flags, 4))
#else
                    if (data->EnableXfgAuditMode)
#endif
                        PhAppendStringBuilder2(&sb, L"审核 ");

                #if !defined(NTDDI_WIN10_CO) || (NTDDI_VERSION < NTDDI_WIN10_CO)
                    PhAppendStringBuilder2(&sb, _bittest((const PLONG)&data->Flags, 3) ? L"XF Guard" : L"CF Guard");
                #else
                    PhAppendStringBuilder2(&sb, data->EnableXfg ? L"XF Guard" : L"CF Guard");
                #endif

                    *ShortDescription = PhFinalStringBuilderString(&sb);
                }

                if (LongDescription)
                {
                    PhInitializeStringBuilder(&sb, 100);

                #if !defined(NTDDI_WIN10_CO) || (NTDDI_VERSION < NTDDI_WIN10_CO)
                    if (_bittest((const PLONG)&data->Flags, 3))
                #else
                    if (data->EnableXfg)
                #endif
                    {
                        PhAppendStringBuilder2(&sb, L"此进程已启用扩展控制流防护 (XFG)。\r\n");

                        if (data->EnableXfgAuditMode) PhAppendStringBuilder2(&sb, L"审核 XFG：XFG 正在审核模式下运行。\r\n");
                        if (data->StrictMode) PhAppendStringBuilder2(&sb, L"严格 XFG：仅可加载 XFG 模块。\r\n");
                        if (data->EnableExportSuppression) PhAppendStringBuilder2(&sb, L"DLL 导出可被标记为 XFG 无效目标。\r\n");
                    }
                    else
                    {
                        PhAppendStringBuilder2(&sb, L"此进程已启用控制流防护 (CFG)。\r\n");

                        if (data->StrictMode) PhAppendStringBuilder2(&sb, L"严格 CFG：仅可加载 CFG 模块。\r\n");
                        if (data->EnableExportSuppression) PhAppendStringBuilder2(&sb, L"DLL 导出可被标记为 CFG 无效目标。\r\n");
                    }

                    *LongDescription = PhFinalStringBuilderString(&sb);
                }

                result = TRUE;
            }
        }
        break;
    case ProcessSignaturePolicy:
        {
            PPROCESS_MITIGATION_BINARY_SIGNATURE_POLICY data = Data;

            if (data->MicrosoftSignedOnly || data->StoreSignedOnly)
            {
                if (ShortDescription)
                {
                    PhInitializeStringBuilder(&sb, 50);
                    PhAppendStringBuilder2(&sb, L"签名受限 (");
                    if (data->MicrosoftSignedOnly) PhAppendStringBuilder2(&sb, L"仅 Microsoft, ");
                    if (data->StoreSignedOnly) PhAppendStringBuilder2(&sb, L"仅应用商店, ");
                    if (PhEndsWithStringRef2(&sb.String->sr, L", ", FALSE)) PhRemoveEndStringBuilder(&sb, 2);
                    PhAppendCharStringBuilder(&sb, L')');

                    *ShortDescription = PhFinalStringBuilderString(&sb);
                }

                if (LongDescription)
                {
                    PhInitializeStringBuilder(&sb, 100);
                    PhAppendStringBuilder2(&sb, L"此进程已启用映像签名限制。\r\n");
                    if (data->MicrosoftSignedOnly) PhAppendStringBuilder2(&sb, L"仅允许 Microsoft 签名。\r\n");
                    if (data->StoreSignedOnly) PhAppendStringBuilder2(&sb, L"仅允许 Windows Store 签名。\r\n");
                    if (data->MitigationOptIn) PhAppendStringBuilder2(&sb, L"这是一项选择性加入的限制。\r\n");
                    *LongDescription = PhFinalStringBuilderString(&sb);
                }

                result = TRUE;
            }
        }
        break;
    case ProcessFontDisablePolicy:
        {
            PPROCESS_MITIGATION_FONT_DISABLE_POLICY data = Data;

            if (data->DisableNonSystemFonts)
            {
                if (ShortDescription)
                    *ShortDescription = PhCreateString(L"非系统字体已禁用");

                if (LongDescription)
                {
                    PhInitializeStringBuilder(&sb, 100);
                    PhAppendStringBuilder2(&sb, L"此进程无法使用非系统字体。\r\n");
                    if (data->AuditNonSystemFontLoading) PhAppendStringBuilder2(&sb, L"在此进程中加载非系统字体将触发 ETW 事件。\r\n");
                    *LongDescription = PhFinalStringBuilderString(&sb);
                }

                result = TRUE;
            }
        }
        break;
    case ProcessImageLoadPolicy:
        {
            PPROCESS_MITIGATION_IMAGE_LOAD_POLICY data = Data;

            if (data->NoRemoteImages || data->NoLowMandatoryLabelImages)
            {
                if (ShortDescription)
                {
                    PhInitializeStringBuilder(&sb, 50);
                    PhAppendStringBuilder2(&sb, L"映像受限 (");
                    if (data->NoRemoteImages) PhAppendStringBuilder2(&sb, L"远程映像, ");
                    if (data->NoLowMandatoryLabelImages) PhAppendStringBuilder2(&sb, L"低强制性标签映像, ");
                    if (PhEndsWithStringRef2(&sb.String->sr, L", ", FALSE)) PhRemoveEndStringBuilder(&sb, 2);
                    PhAppendCharStringBuilder(&sb, L')');

                    *ShortDescription = PhFinalStringBuilderString(&sb);
                }

                if (LongDescription)
                {
                    PhInitializeStringBuilder(&sb, 50);
                    if (data->NoRemoteImages) PhAppendStringBuilder2(&sb, L"无法将远程位置的映像加载到进程中。\r\n");
                    if (data->NoLowMandatoryLabelImages) PhAppendStringBuilder2(&sb, L"无法将具有低强制性标签的映像加载到进程中。\r\n");

                    *LongDescription = PhFinalStringBuilderString(&sb);
                }

                result = TRUE;
            }

            if (data->PreferSystem32Images)
            {
                if (ShortDescription)
                    *ShortDescription = PhCreateString(L"优先 System32 映像");

                if (LongDescription)
                    *LongDescription = PhCreateString(L"强制映像先从安装 Windows 的 System32 文件夹加载，然后在标准 DLL 搜索顺序之前从应用程序目录加载。\r\n");

                result = TRUE;
            }
        }
        break;
    case ProcessSystemCallFilterPolicy:
        {
            PPROCESS_MITIGATION_SYSTEM_CALL_FILTER_POLICY data = Data;

            if (data->FilterId)
            {
                if (ShortDescription)
                    *ShortDescription = PhCreateString(L"系统调用筛选");

                if (LongDescription)
                    *LongDescription = PhCreateString(L"系统调用筛选处于活动状态。\r\n");

                result = TRUE;
            }
        }
        break;
    case ProcessPayloadRestrictionPolicy:
        {
            PPROCESS_MITIGATION_PAYLOAD_RESTRICTION_POLICY data = Data;

            if (data->EnableExportAddressFilter || data->EnableExportAddressFilterPlus ||
                data->EnableImportAddressFilter || data->EnableRopStackPivot ||
                data->EnableRopCallerCheck || data->EnableRopSimExec)
            {
                if (ShortDescription)
                    *ShortDescription = PhCreateString(L"有效载荷限制");

                if (LongDescription)
                {
                    PhInitializeStringBuilder(&sb, 100);
                    PhAppendStringBuilder2(&sb, L"此进程已启用有效载荷限制。\r\n");
                    if (data->EnableExportAddressFilter) PhAppendStringBuilder2(&sb, L"导出地址筛选已启用。\r\n");
                    if (data->EnableExportAddressFilterPlus) PhAppendStringBuilder2(&sb, L"导出地址筛选 (Plus) 已启用。\r\n");
                    if (data->EnableImportAddressFilter) PhAppendStringBuilder2(&sb, L"导入地址筛选已启用。\r\n");
                    if (data->EnableRopStackPivot) PhAppendStringBuilder2(&sb, L"StackPivot 已启用。\r\n");
                    if (data->EnableRopCallerCheck) PhAppendStringBuilder2(&sb, L"CallerCheck 已启用。\r\n");
                    if (data->EnableRopSimExec) PhAppendStringBuilder2(&sb, L"SimExec 已启用。\r\n");
                    *LongDescription = PhFinalStringBuilderString(&sb);
                }

                result = TRUE;
            }
        }
        break;
    case ProcessChildProcessPolicy:
        {
            PPROCESS_MITIGATION_CHILD_PROCESS_POLICY data = Data;

            if (data->NoChildProcessCreation)
            {
                if (ShortDescription)
                    *ShortDescription = PhCreateString(L"子进程创建已禁用");

                if (LongDescription)
                    *LongDescription = PhCreateString(L"此进程无法创建子进程。\r\n");

                result = TRUE;
            }
        }
        break;
    case ProcessSideChannelIsolationPolicy:
        {
            PPROCESS_MITIGATION_SIDE_CHANNEL_ISOLATION_POLICY data = Data;

            if (data->SmtBranchTargetIsolation)
            {
                if (ShortDescription)
                    *ShortDescription = PhCreateString(L"SMT 线程分支目标隔离");

                if (LongDescription)
                    *LongDescription = PhCreateString(L"用户模式下的跨 SMT 线程分支目标污染已启用。\r\n");

                result = TRUE;
            }

            if (data->IsolateSecurityDomain)
            {
                if (ShortDescription)
                    *ShortDescription = PhCreateString(L"独立安全域");

                if (LongDescription)
                    *LongDescription = PhCreateString(L"隔离安全域已启用。\r\n");

                result = TRUE;
            }

            if (data->DisablePageCombine)
            {
                if (ShortDescription)
                    *ShortDescription = PhCreateString(L"受限页面合并");

                if (LongDescription)
                    *LongDescription = PhCreateString(L"对此进程禁用所有页面合并。\r\n");

                result = TRUE;
            }

            if (data->SpeculativeStoreBypassDisable)
            {
                if (ShortDescription)
                    *ShortDescription = PhCreateString(L"内存消歧 (SSBD)");

                if (LongDescription)
                    *LongDescription = PhCreateString(L"此进程已启用内存消歧。\r\n");

                result = TRUE;
            }
        }
        break;
    case ProcessUserShadowStackPolicy:
        {
            PPROCESS_MITIGATION_USER_SHADOW_STACK_POLICY data = Data;

            if (data->EnableUserShadowStack || data->AuditUserShadowStack)
            {
                if (ShortDescription)
                {
                    PhInitializeStringBuilder(&sb, 50);

                    if (data->AuditUserShadowStack)
                        PhAppendStringBuilder2(&sb, L"审核 ");

                    if (data->EnableUserShadowStackStrictMode)
                        PhAppendStringBuilder2(&sb, L"严格 ");

                    PhAppendStringBuilder2(&sb, L"堆栈保护");

                    *ShortDescription = PhFinalStringBuilderString(&sb);
                }

                if (LongDescription)
                {
                    PhInitializeStringBuilder(&sb, 100);

                    PhAppendStringBuilder2(&sb, L"CPU 通过硬件强制实施的影子堆栈在运行时验证函数返回地址。\r\n");

                    if (data->AuditUserShadowStack)
                        PhAppendStringBuilder2(&sb, L"审核堆栈保护：将 ROP 失败记录到事件日志。\r\n");

                    if (data->EnableUserShadowStackStrictMode)
                        PhAppendStringBuilder2(&sb, L"严格堆栈保护：检测到任何 ROP 都将导致进程终止。\r\n");

                    if (data->AuditSetContextIpValidation)
                        PhAppendStringBuilder2(&sb, L"审核 Set Context IP 验证：将上下文 IP 的修改记录到事件日志。\r\n");

                    if (data->SetContextIpValidation)
                        PhAppendStringBuilder2(&sb, L"Set Context IP 验证：检测到上下文 IP 被修改将导致进程终止。\r\n");

                    if (data->AuditBlockNonCetBinaries)
                        PhAppendStringBuilder2(&sb, L"审核阻止非 CET 二进制文件：将加载不支持 CET 的二进制文件的尝试记录到事件日志。\r\n");

                    if (data->BlockNonCetBinaries)
                        PhAppendStringBuilder2(&sb, L"阻止不支持 CET 的二进制文件\r\n");

                    if (data->BlockNonCetBinariesNonEhcont)
                        PhAppendStringBuilder2(&sb, L"阻止不支持 CET 或缺少 EH 连续性元数据的二进制文件。\r\n");

                    *LongDescription = PhFinalStringBuilderString(&sb);
                }

                result = TRUE;
            }
        }
        break;
case ProcessRedirectionTrustPolicy:
{
    PPROCESS_MITIGATION_REDIRECTION_TRUST_POLICY data = Data;

    // Ensure data is not NULL
    if (data)
    {
        // Handle Enforce Redirection Trust Policy
        if (data->EnforceRedirectionTrust && data->AuditRedirectionTrust)
        {
            if (ShortDescription)
                *ShortDescription = PhCreateString(L"联接点重定向保护 / 审核");

            if (LongDescription)
                *LongDescription = PhCreateString(L"阻止进程跟随非管理员用户创建的文件系统联接点并记录该尝试。\r\n记录进程跟随非管理员用户创建的文件系统联接点的尝试。\r\n");

            result = TRUE;
        }
        else if (data->EnforceRedirectionTrust)
        {
            if (ShortDescription)
                *ShortDescription = PhCreateString(L"联接点重定向保护");

            if (LongDescription)
                *LongDescription = PhCreateString(L"阻止进程跟随非管理员用户创建的文件系统联接点并记录该尝试。\r\n");

            result = TRUE;
        }
        else if (data->AuditRedirectionTrust)
        {
            if (ShortDescription)
                *ShortDescription = PhCreateString(L"联接点重定向保护（审核）");

            if (LongDescription)
                *LongDescription = PhCreateString(L"记录进程跟随非管理员用户创建的文件系统联接点的尝试。\r\n");

            result = TRUE;
        }
    }
}
break;
    case ProcessUserPointerAuthPolicy:
        {
            PPROCESS_MITIGATION_USER_POINTER_AUTH_POLICY data = Data;

            if (data->EnablePointerAuthUserIp)
            {
                if (ShortDescription)
                    *ShortDescription = PhCreateString(L"ARM 指针认证");

                if (LongDescription)
                    *LongDescription = PhCreateString(L"指针认证 (PAC) 可防止指针被意外更改。\r\n");

                result = TRUE;
            }
        }
        break;
    case ProcessSEHOPPolicy:
        {
            PPROCESS_MITIGATION_SEHOP_POLICY data = Data;

            if (data->EnableSehop)
            {
                if (ShortDescription)
                    *ShortDescription = PhCreateString(L"结构化异常处理覆写保护 (SEHOP)");

                if (LongDescription)
                    *LongDescription = PhCreateString(L"SEHOP 可防止结构化异常处理程序 (SEH) 被覆写。\r\n");

                result = TRUE;
            }
        }
        break;
    default:
        result = FALSE;
    }

    return result;
}
