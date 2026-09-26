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

#include "onlnchk.h"

HRESULT CALLBACK TaskDialogResultFoundProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ LONG_PTR dwRefData
    )
{
    PUPLOAD_CONTEXT context = (PUPLOAD_CONTEXT)dwRefData;

    switch (WindowMessage)
    {
    case TDN_NAVIGATED:
        {
            //if (context->TaskbarListClass)
            //{
            //    PhTaskbarListSetProgressState(context->TaskbarListClass, context->DialogHandle, PH_TBLF_NOPROGRESS);
            //}
        }
        break;
    case TDN_BUTTON_CLICKED:
        {
            INT buttonID = (INT)wParam;

            if (buttonID == IDOK)
            {
                ShowFileUploadProgressDialog(context);
                return S_FALSE;
            }
            else if (buttonID == IDRETRY)
            {
//#ifdef PH_BUILD_API
                ShowVirusTotalReScanProgressDialog(context);
                return S_FALSE;
//#else
//                if (!PhIsNullOrEmptyString(context->ReAnalyseUrl))
//                {
//                    PhShellExecute(WindowHandle, PhGetString(context->ReAnalyseUrl), NULL);
//                }
//#endif
            }
            else if (buttonID == IDYES)
            {
//#ifdef PH_BUILD_API
                ShowVirusTotalViewReportProgressDialog(context);
                return S_FALSE;
//#else
//                if (!PhIsNullOrEmptyString(context->LaunchCommand))
//                {
//                    PhShellExecute(WindowHandle, PhGetString(context->LaunchCommand), NULL);
//                }
//#endif
            }
        }
        break;
    case TDN_VERIFICATION_CLICKED:
        {
            BOOL verification = (BOOL)wParam;
        }
        break;
    }

    return S_OK;
}

VOID ShowFileFoundDialog(
    _In_ PUPLOAD_CONTEXT Context
    )
{
    static TASKDIALOG_BUTTON TaskDialogButtonArray[] =
    {
        { IDYES, L"查看上次分析\n查看上次或过期的分析页面" },
        //{ IDRETRY, L"Reanalyze file\nRescan the existing sample on VirusTotal" },
        { IDOK, L"上传文件\n上传新样本以进行更新分析" },
    };

    TASKDIALOGCONFIG config;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED | TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS;
    config.dwCommonButtons = TDCBF_CLOSE_BUTTON;
    config.hMainIcon = PhGetApplicationIcon(FALSE, PhGetWindowDpi(Context->DialogHandle));
    config.pszMainInstruction = PhaFormatString(
        L"%s 上次分析于 %s",
        PhGetStringOrEmpty(Context->BaseFileName),
        PhGetStringOrEmpty(Context->LastAnalysisDate)
        )->Buffer;

    if (Context->Service == MENUITEM_VIRUSTOTAL_UPLOAD || Context->Service == MENUITEM_VIRUSTOTAL_UPLOAD_SERVICE)
    {
        // was last analyzed by VirusTotal on 2016-12-28 05:26:50 UTC (1 hour ago) it was first analyzed by VirusTotal on 2016-12-12 17:08:19 UTC.
        config.pszContent = PhaFormatString(
            L"%s %s\r\n%s %s\r\n%s %s\r\n%s %s\r\n\r\n%s",
            L"检测数：",
            PhGetStringOrEmpty(Context->Detected),
            L"首次分析：",
            PhGetStringOrEmpty(Context->FirstAnalysisDate),
            L"上次分析：",
            PhGetStringOrEmpty(Context->LastAnalysisDate),
            L"上传大小：",
            PhGetStringOrEmpty(Context->FileSize),
            L"您可以查看上次分析，或立即重新上传。"
            )->Buffer;
    }
    else
    {
        config.pszContent = PhaFormatString(
            L"%s %s\r\n%s %s\r\n\r\n%s",
            L"检测数：",
            PhGetStringOrEmpty(Context->Detected),
            //L"Last analyzed:",
            //PhGetStringOrEmpty(Context->LastAnalysisDate),
            L"上传大小：",
            PhGetStringOrEmpty(Context->FileSize),
            L"您可以查看上次分析，或立即重新上传。"
            )->Buffer;
    }

    //config.pszVerificationText = L"Remember this selection...";
    config.pButtons = TaskDialogButtonArray;
    config.cButtons = ARRAYSIZE(TaskDialogButtonArray);
    config.lpCallbackData = (LONG_PTR)Context;
    config.pfCallback = TaskDialogResultFoundProc;
    config.cxWidth = 250;

    PhTaskDialogNavigatePage(Context->DialogHandle, &config);
}
