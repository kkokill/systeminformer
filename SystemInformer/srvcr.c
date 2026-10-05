/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2013
 *     dmex    2019-2026
 *
 */

#include <phapp.h>
#include <phsettings.h>
#include <svcsup.h>

#include <actions.h>
#include <phsvccl.h>

/**
 * Dialog procedure for the create service dialog.
 *
 * \param hwndDlg The handle to the dialog.
 * \param uMsg The message being processed.
 * \param wParam Message-specific parameter.
 * \param lParam Message-specific parameter.
 * \return TRUE if the message was handled, otherwise FALSE.
 */
INT_PTR CALLBACK PhpCreateServiceDlgProc(
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
            PhSetApplicationWindowIcon(hwndDlg);

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            for (ULONG i = 0; i < RTL_NUMBER_OF(PhServiceTypeStrings); i++) // 语言切换：显示译文
                ComboBox_AddString(GetDlgItem(hwndDlg, IDC_TYPE), PhTranslateTextZ(PhServiceTypeStrings[i]->Buffer));
            for (ULONG i = 0; i < RTL_NUMBER_OF(PhServiceStartTypeStrings); i++)
                ComboBox_AddString(GetDlgItem(hwndDlg, IDC_STARTTYPE), PhTranslateTextZ(PhServiceStartTypeStrings[i]->Buffer));
            for (ULONG i = 0; i < RTL_NUMBER_OF(PhServiceErrorControlStrings); i++)
                ComboBox_AddString(GetDlgItem(hwndDlg, IDC_ERRORCONTROL), PhTranslateTextZ(PhServiceErrorControlStrings[i]->Buffer));

            PhSelectComboBoxString(GetDlgItem(hwndDlg, IDC_TYPE), PhTranslateTextZ(L"独立进程"), FALSE);
            PhSelectComboBoxString(GetDlgItem(hwndDlg, IDC_STARTTYPE), PhTranslateTextZ(L"手动启动"), FALSE);
            PhSelectComboBoxString(GetDlgItem(hwndDlg, IDC_ERRORCONTROL), PhTranslateTextZ(L"忽略"), FALSE);

            if (!PhGetOwnTokenAttributes().Elevated)
            {
                Button_SetElevationRequiredState(GetDlgItem(hwndDlg, IDOK), TRUE);
            }

            PhSetDialogFocus(hwndDlg, GetDlgItem(hwndDlg, IDC_NAME));
            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
                {
                    EndDialog(hwndDlg, IDCANCEL);
                }
                break;
            case IDOK:
                {
                    NTSTATUS status;
                    SC_HANDLE serviceHandle;
                    PPH_STRING serviceName;
                    PPH_STRING serviceDisplayName;
                    PPH_STRING serviceTypeString;
                    PPH_STRING serviceStartTypeString;
                    PPH_STRING serviceErrorControlString;
                    ULONG serviceType;
                    ULONG serviceStartType;
                    ULONG serviceErrorControl;
                    PPH_STRING serviceBinaryPath;
                    PPH_STRING serviceBinaryEscaped;

                    serviceName = PH_AUTO(PhGetWindowText(GetDlgItem(hwndDlg, IDC_NAME)));
                    serviceDisplayName = PH_AUTO(PhGetWindowText(GetDlgItem(hwndDlg, IDC_DISPLAYNAME)));
                    serviceTypeString = PH_AUTO(PhGetWindowText(GetDlgItem(hwndDlg, IDC_TYPE)));
                    serviceStartTypeString = PH_AUTO(PhGetWindowText(GetDlgItem(hwndDlg, IDC_STARTTYPE)));
                    serviceErrorControlString = PH_AUTO(PhGetWindowText(GetDlgItem(hwndDlg, IDC_ERRORCONTROL)));
                    serviceBinaryPath = PH_AUTO(PhGetWindowText(GetDlgItem(hwndDlg, IDC_BINARYPATH)));

                    serviceType = PhGetServiceTypeInteger(&serviceTypeString->sr);
                    serviceStartType = PhGetServiceStartTypeInteger(&serviceStartTypeString->sr);
                    serviceErrorControl = PhGetServiceErrorControlInteger(&serviceErrorControlString->sr);

                    if (PhIsNullOrEmptyString(serviceBinaryPath))
                    {
                        PhShowError2(hwndDlg, L"无法创建服务。", L"%s", L"二进制路径为空。");
                        break;
                    }

                    if (!FlagOn(serviceType, SERVICE_DRIVER))
                    {
                        serviceBinaryEscaped = PH_AUTO(PhCommandLineQuoteSpaces(&serviceBinaryPath->sr));

                        if (!PhIsNullOrEmptyString(serviceBinaryEscaped))
                        {
                            serviceBinaryPath = serviceBinaryEscaped;
                        }
                    }

                    if (PhGetOwnTokenAttributes().Elevated)
                    {
                        status = PhCreateService(
                            &serviceHandle,
                            PhGetString(serviceName),
                            PhGetString(serviceDisplayName),
                            SERVICE_QUERY_CONFIG,
                            serviceType,
                            serviceStartType,
                            serviceErrorControl,
                            PhGetString(serviceBinaryPath),
                            NULL,
                            L""
                            );

                        if (NT_SUCCESS(status))
                        {
                            EndDialog(hwndDlg, IDOK);
                            PhCloseServiceHandle(serviceHandle);
                        }
                    }
                    else
                    {
                        if (PhUiConnectToPhSvc(hwndDlg, FALSE))
                        {
                            status = PhSvcCallCreateService(
                                PhGetString(serviceName),
                                PhGetString(serviceDisplayName),
                                serviceType,
                                serviceStartType,
                                serviceErrorControl,
                                PhGetString(serviceBinaryPath),
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                L""
                                );
                            PhUiDisconnectFromPhSvc();

                            if (NT_SUCCESS(status))
                            {
                                EndDialog(hwndDlg, IDOK);
                            }
                        }
                        else
                        {
                            // User cancelled elevation.
                            status = STATUS_SUCCESS;
                        }
                    }

                    if (!NT_SUCCESS(status))
                        PhShowStatus(hwndDlg, L"无法创建服务。", status, 0);
                }
                break;
            case IDC_BROWSE:
                {
                    PH_FILETYPE_FILTER filters[] =
                    {
                        { (PWSTR)PhTranslateTextZ(L"可执行文件 (*.exe;*.sys)"), L"*.exe;*.sys" },
                        { (PWSTR)PhTranslateTextZ(L"所有文件 (*.*)"), L"*.*" }
                    };
                    PVOID fileDialog;
                    PPH_STRING fileName;

                    fileDialog = PhCreateOpenFileDialog();
                    PhSetFileDialogFilter(fileDialog, filters, sizeof(filters) / sizeof(PH_FILETYPE_FILTER));

                    fileName = PhGetFileName(PhaGetDlgItemText(hwndDlg, IDC_BINARYPATH));
                    PhSetFileDialogFileName(fileDialog, fileName->Buffer);
                    PhDereferenceObject(fileName);

                    if (PhShowFileDialog(hwndDlg, fileDialog))
                    {
                        fileName = PhGetFileDialogFileName(fileDialog);
                        PhSetDialogItemText(hwndDlg, IDC_BINARYPATH, fileName->Buffer);
                        PhDereferenceObject(fileName);
                    }

                    PhFreeFileDialog(fileDialog);
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

/**
 * Shows the create service dialog.
 *
 * \param ParentWindowHandle The parent window handle.
 */
VOID PhShowCreateServiceDialog(
    _In_ HWND ParentWindowHandle
    )
{
    PhDialogBox(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_CREATESERVICE),
        PhCsForceNoParent ? NULL : ParentWindowHandle,
        PhpCreateServiceDlgProc,
        NULL
        );
}
