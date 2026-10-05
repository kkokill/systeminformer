/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2013
 *     dmex    2019-2023
 *
 */

#include <phapp.h>
#include <lsasup.h>
#include <winsta.h>

static CONST PH_KEY_VALUE_PAIR PhpMessageBoxIconPairs[] =
{
    SIP(L"无 ", MB_OK), // 尾空格标记键：与主程序 {无,N/A} 区分，英文值 None 对位上游组合框
    SIP(L"信息", MB_ICONINFORMATION),
    SIP(L"警告", MB_ICONWARNING),
    SIP(L"错误", MB_ICONERROR),
    SIP(L"问题", MB_ICONQUESTION)
};

INT_PTR CALLBACK PhpSessionSendMessageDlgProc(
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
            HWND iconComboBox;
            PPH_STRING currentUserName;

            PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, UlongToPtr((ULONG)lParam));

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            iconComboBox = GetDlgItem(hwndDlg, IDC_TYPE);
            ComboBox_AddString(iconComboBox, PhTranslateTextZ(L"无 ")); // 语言切换：与查找表同键
            ComboBox_AddString(iconComboBox, PhTranslateTextZ(L"信息"));
            ComboBox_AddString(iconComboBox, PhTranslateTextZ(L"警告"));
            ComboBox_AddString(iconComboBox, PhTranslateTextZ(L"错误"));
            ComboBox_AddString(iconComboBox, PhTranslateTextZ(L"问题"));
            PhSelectComboBoxString(iconComboBox, PhTranslateTextZ(L"无 "), FALSE);

            if (currentUserName = PhGetTokenUserString(PhGetOwnTokenAttributes().TokenHandle, TRUE))
            {
                PhSetDialogItemText(
                    hwndDlg,
                    IDC_TITLE,
                    PhaFormatString(PhTranslateTextZ(L"来自 %s 的消息"), currentUserName->Buffer)->Buffer
                    );
                PhDereferenceObject(currentUserName);
            }

            PhSetDialogFocus(hwndDlg, GetDlgItem(hwndDlg, IDC_TEXT));

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport); // HACK (dmex)
        }
        break;
    case WM_DESTROY:
        {
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
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
                    ULONG sessionId = PtrToUlong(PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT));
                    PPH_STRING title;
                    PPH_STRING text;
                    ULONG icon = 0;
                    ULONG64 timeout = 0;
                    ULONG response;

                    title = PhaGetDlgItemText(hwndDlg, IDC_TITLE);
                    text = PhaGetDlgItemText(hwndDlg, IDC_TEXT);

                    if (!PhFindIntegerSiKeyValuePairs(
                        PhpMessageBoxIconPairs,
                        sizeof(PhpMessageBoxIconPairs),
                        PhaGetDlgItemText(hwndDlg, IDC_TYPE)->Buffer,
                        &icon
                        ))
                    {
                        // 英文模式下组合框为英文显示文本（如 None/Information），反查回中文键再匹配
                        PPH_STRING reversedText;

                        reversedText = PhCreateString(PhTranslateTextReverseZ(PhaGetDlgItemText(hwndDlg, IDC_TYPE)->Buffer));
                        PhFindIntegerSiKeyValuePairs(
                            PhpMessageBoxIconPairs,
                            sizeof(PhpMessageBoxIconPairs),
                            reversedText->Buffer,
                            &icon
                            );
                        PhDereferenceObject(reversedText);
                    }
                    PhStringToInteger64(
                        &PhaGetDlgItemText(hwndDlg, IDC_TIMEOUT)->sr,
                        10,
                        &timeout
                        );

                    if (WinStationSendMessageW(
                        NULL,
                        sessionId,
                        title->Buffer,
                        (ULONG)title->Length,
                        text->Buffer,
                        (ULONG)text->Length,
                        icon,
                        (ULONG)timeout,
                        &response,
                        TRUE
                        ))
                    {
                        EndDialog(hwndDlg, IDOK);
                    }
                    else
                    {
                        PhShowStatus(hwndDlg, L"无法发送消息。", 0, GetLastError());
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

VOID PhShowSessionSendMessageDialog(
    _In_ HWND ParentWindowHandle,
    _In_ ULONG SessionId
    )
{
    PhDialogBox(
        NtCurrentImageBase(),
        MAKEINTRESOURCE(IDD_EDITMESSAGE),
        ParentWindowHandle,
        PhpSessionSendMessageDlgProc,
        UlongToPtr(SessionId)
        );
}
