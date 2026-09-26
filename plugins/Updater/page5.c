/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2016-2026
 *
 */

#include "updater.h"

/**
 * \brief Callback procedure for the final state task dialog pages (Install, Latest, Error).
 * \param WindowHandle Handle to the dialog window.
 * \param WindowMessage The window message.
 * \param wParam Additional message-specific information.
 * \param lParam Additional message-specific information.
 * \param dwRefData The updater context.
 * \return HRESULT Successful or errant status.
 */
HRESULT CALLBACK FinalTaskDialogCallbackProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ LONG_PTR dwRefData
    )
{
    PPH_UPDATER_CONTEXT context = (PPH_UPDATER_CONTEXT)dwRefData;

    switch (WindowMessage)
    {
    case TDN_NAVIGATED:
        {
#ifndef FORCE_NO_STATUS_TIMER
            if (context->ProgressTimer)
            {
                PhKillTimer(WindowHandle, 9000);
                context->ProgressTimer = FALSE;
            }
#endif
            context->ElevationRequired = !!UpdateCheckDirectoryElevationRequired();

#ifdef FORCE_ELEVATION_CHECK
            context->ElevationRequired = TRUE;
#endif

            if (context->ElevationRequired)
            {
                SendMessage(WindowHandle, TDM_SET_BUTTON_ELEVATION_REQUIRED_STATE, IDYES, TRUE);
            }
        }
        break;
    case TDN_BUTTON_CLICKED:
        {
            INT buttonId = (INT)wParam;

            if (buttonId == IDRETRY)
            {
                if (context->CryptoBackend == UpdaterCryptoBackendSymCrypt)
                    context->CryptoBackend = UpdaterCryptoBackendBCrypt;
                ShowCheckForUpdatesDialog(context);
                return S_FALSE;
            }
            else if (buttonId == IDYES)
            {
#if defined(PH_BUILD_MSIX)
                // MSIX: the platform already downloaded and installed the update.
                // Nothing to ShellExecute; let the dialog close.
#else
                if (!NT_SUCCESS(UpdateShellExecute(context, WindowHandle)))
                {
                    return S_FALSE;
                }
#endif
            }
        }
        break;
    case TDN_HYPERLINK_CLICKED:
        {
            TaskDialogLinkClicked(context);
            return S_FALSE;
        }
        break;
    }

    return S_OK;
}

/**
 * \brief Shows the Ready to Install dialog page.
 * \param Context The updater context.
 */
VOID ShowUpdateInstallDialog(
    _In_ PPH_UPDATER_CONTEXT Context
    )
{
    TASKDIALOG_BUTTON TaskDialogButtonArray[] =
    {
        { IDYES, L"安装" }
    };
    TASKDIALOGCONFIG config;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED;
    config.dwCommonButtons = TDCBF_CLOSE_BUTTON;
    config.hMainIcon = PhGetApplicationIcon(FALSE, Context->WindowDpi);
    config.cxWidth = 200;
    config.pfCallback = FinalTaskDialogCallbackProc;
    config.lpCallbackData = (LONG_PTR)Context;
    config.pButtons = TaskDialogButtonArray;
    config.cButtons = RTL_NUMBER_OF(TaskDialogButtonArray);

    config.pszWindowTitle = L"System Informer - 更新器";
    if (Context->SwitchingChannel)
    {
        switch (Context->Channel)
        {
        case PhReleaseChannel:
            config.pszMainInstruction = L"准备切换到 Release 渠道？";
            break;
        //case PhPreviewChannel:
        //    config.pszMainInstruction = L"准备切换到 Preview 渠道？";
        //    break;
        case PhCanaryChannel:
            config.pszMainInstruction = L"准备切换到 Canary 渠道？";
            break;
        //case PhDeveloperChannel:
        //    config.pszMainInstruction = L"准备切换到 Developer 渠道？";
        //    break;
        default:
            config.pszMainInstruction = L"准备切换渠道？";
            break;
        }

        config.pszContent = L"该渠道已成功下载并验证。\r\n\r\n点击“安装”继续。";
    }
    else
    {
#if defined(PH_BUILD_MSIX)
        config.pszMainInstruction = L"更新已安装。";
        config.pszContent = L"更新已下载并安装。\r\n\r\n请重启 System Informer 以应用更新。";
#else
        config.pszMainInstruction = L"准备安装更新？";
        config.pszContent = L"该更新已成功下载并验证。\r\n\r\n点击“安装”继续。";
#endif
    }

    PhTaskDialogNavigatePage(Context->DialogHandle, &config);
}

/**
 * \brief Generates the text describing the current latest version.
 * \param Context The updater context.
 * \return A string containing the formatted version text.
 */
PPH_STRING UpdaterGetLatestVersionText(
    _In_ PPH_UPDATER_CONTEXT Context
    )
{
    PPH_STRING version;
    PPH_STRING commit;
    ULONG majorVersion;
    ULONG minorVersion;
    ULONG buildVersion;
    ULONG revisionVersion;

    PhGetBuildVersionNumbers(&majorVersion, &minorVersion, &buildVersion, &revisionVersion);
    commit = PhGetBuildCommit();

    if (commit && commit->Length > 4)
    {
        version = PhFormatString(
            L"%lu.%lu.%lu.%lu (%s)",
            majorVersion,
            minorVersion,
            buildVersion,
            revisionVersion,
            PhGetString(commit)
            );
        PhMoveReference(&version, PhFormatString(
            L"%s\r\n\r\n<A HREF=\"changelog.txt\">查看变更日志</A>",
            PhGetStringOrEmpty(version)
            ));
    }
    else
    {
        version = PhFormatString(
            L"System Informer %lu.%lu.%lu.%lu",
            majorVersion,
            minorVersion,
            buildVersion,
            revisionVersion
            );
        PhMoveReference(&version, PhFormatString(
            L"%s\r\n\r\n<A HREF=\"changelog.txt\">查看变更日志</A>",
            PhGetStringOrEmpty(version)
            ));
    }

    if (commit)
    {
        PhDereferenceObject(commit);
    }

    return version;
}

/**
 * \brief Shows the Latest Version dialog page.
 * \param Context The updater context.
 */
VOID ShowLatestVersionDialog(
    _In_ PPH_UPDATER_CONTEXT Context
    )
{
    TASKDIALOGCONFIG config;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED | TDF_ENABLE_HYPERLINKS;
    config.dwCommonButtons = TDCBF_CLOSE_BUTTON;
    config.hMainIcon = PhGetApplicationIcon(FALSE, Context->WindowDpi);
    config.cxWidth = 200;
    config.pfCallback = FinalTaskDialogCallbackProc;
    config.lpCallbackData = (LONG_PTR)Context;

    config.pszWindowTitle = L"System Informer - 更新器";
    config.pszMainInstruction = L"您正在运行最新版本。";
    config.pszContent = PH_AUTO_T(PH_STRING, UpdaterGetLatestVersionText(Context))->Buffer;

    PhTaskDialogNavigatePage(Context->DialogHandle, &config);
}

/**
 * \brief Shows the Newer Version dialog page (e.g., when running a pre-release).
 * \param Context The updater context.
 */
VOID ShowNewerVersionDialog(
    _In_ PPH_UPDATER_CONTEXT Context
    )
{
    TASKDIALOGCONFIG config;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED | TDF_ENABLE_HYPERLINKS;
    config.dwCommonButtons = TDCBF_CLOSE_BUTTON;
    config.hMainIcon = PhGetApplicationIcon(FALSE, Context->WindowDpi);
    config.cxWidth = 200;
    config.pfCallback = FinalTaskDialogCallbackProc;
    config.lpCallbackData = (LONG_PTR)Context;

    config.pszWindowTitle = L"System Informer - 更新器";
    config.pszMainInstruction = L"您正在运行预发布版本。";
    config.pszContent = PH_AUTO_T(PH_STRING, UpdaterGetLatestVersionText(Context))->Buffer;

    PhTaskDialogNavigatePage(Context->DialogHandle, &config);
}

/**
 * \brief Shows the Update Failed dialog page.
 * \param Context The updater context.
 * \param HashFailed TRUE if the hash verification failed.
 * \param SignatureFailed TRUE if the signature verification failed.
 */
VOID ShowUpdateFailedDialog(
    _In_ PPH_UPDATER_CONTEXT Context,
    _In_ BOOLEAN HashFailed,
    _In_ BOOLEAN SignatureFailed
    )
{
    TASKDIALOGCONFIG config;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    //config.pszMainIcon = MAKEINTRESOURCE(65529);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED;
    config.dwCommonButtons = TDCBF_CLOSE_BUTTON | TDCBF_RETRY_BUTTON;
    config.hMainIcon = PhGetApplicationIcon(FALSE, Context->WindowDpi);

    config.pszWindowTitle = L"System Informer - 更新器";
    if (Context->SwitchingChannel)
        config.pszMainInstruction = L"下载渠道时出错。";
    else
        config.pszMainInstruction = L"下载更新时出错。";

    if (SignatureFailed)
    {
        if (Context->SwitchingChannel)
            config.pszContent = L"签名检查失败。点击“重试”以重新下载该渠道。";
        else
            config.pszContent = L"签名检查失败。点击“重试”以重新下载该更新。";
    }
    else if (HashFailed)
    {
        if (Context->SwitchingChannel)
            config.pszContent = L"哈希检查失败。点击“重试”以重新下载该渠道。";
        else
            config.pszContent = L"哈希检查失败。点击“重试”以重新下载该更新。";
    }
    else
    {
        if (Context->UpdateStatus)
        {
            PPH_STRING errorMessage;

            if (errorMessage = PhHttpGetErrorMessage(Context->UpdateStatus))
            {
                config.pszContent = PhaFormatString(L"[%lu] %s", Context->UpdateStatus, errorMessage->Buffer)->Buffer;
                PhDereferenceObject(errorMessage);
            }
            else if (errorMessage = PhGetStatusMessage(Context->UpdateStatus, 0))
            {
                config.pszContent = PhaFormatString(L"[%lu] %s", Context->UpdateStatus, errorMessage->Buffer)->Buffer;
                PhDereferenceObject(errorMessage);
            }
            else
            {
                if (Context->SwitchingChannel)
                    config.pszContent = L"点击“重试”以重新下载该渠道。";
                else
                    config.pszContent = L"点击“重试”以重新下载该更新。";
            }
        }
        else
        {
            if (Context->SwitchingChannel)
                config.pszContent = L"点击“重试”以重新下载该渠道。";
            else
                config.pszContent = L"点击“重试”以重新下载该更新。";
        }
    }

    config.cxWidth = 200;
    config.pfCallback = FinalTaskDialogCallbackProc;
    config.lpCallbackData = (LONG_PTR)Context;

    PhTaskDialogNavigatePage(Context->DialogHandle, &config);
}
