/*
 * Copyright (c) 2026 portable package.
 *
 * Portable launcher for System Informer (App\Data layout).
 *
 * Starts App\SystemInformer\SystemInformer.exe with an explicit -settings
 * path under Data\ so settings and plugin data stay inside the package.
 */

#include <windows.h>
#include <shlwapi.h>
#include <wchar.h>

#pragma comment(lib, "shlwapi.lib")

#define APP_FILE_RELATIVE       L"App\\SystemInformer\\SystemInformer.exe"
#define DATA_DIR_RELATIVE       L"Data"
#define SETTINGS_FILE_RELATIVE  L"Data\\SystemInformer.exe.settings.json"
#define LAUNCHER_TITLE          L"System Informer 便携版"

int APIENTRY wWinMain(
    _In_ HINSTANCE Instance,
    _In_opt_ HINSTANCE PrevInstance,
    _In_ LPWSTR CommandLine,
    _In_ int ShowCmd
    )
{
    WCHAR directory[MAX_PATH];
    WCHAR appFileName[MAX_PATH];
    WCHAR dataDirectory[MAX_PATH];
    WCHAR settingsFileName[MAX_PATH];
    WCHAR processCommandLine[MAX_PATH * 2];
    STARTUPINFOW startupInfo;
    PROCESS_INFORMATION processInfo;
    DWORD lastError;

    UNREFERENCED_PARAMETER(PrevInstance);
    UNREFERENCED_PARAMETER(CommandLine);
    UNREFERENCED_PARAMETER(ShowCmd);

    if (!GetModuleFileNameW(NULL, directory, MAX_PATH))
        return 1;

    PathRemoveFileSpecW(directory);

    PathCombineW(appFileName, directory, APP_FILE_RELATIVE);
    PathCombineW(dataDirectory, directory, DATA_DIR_RELATIVE);
    PathCombineW(settingsFileName, directory, SETTINGS_FILE_RELATIVE);

    if (GetFileAttributesW(appFileName) == INVALID_FILE_ATTRIBUTES)
    {
        MessageBoxW(
            NULL,
            L"无法找到 App\\SystemInformer\\SystemInformer.exe，便携包可能不完整或已损坏。",
            LAUNCHER_TITLE,
            MB_ICONERROR
            );
        return 1;
    }

    // Make sure the data directory exists before starting the application.
    CreateDirectoryW(dataDirectory, NULL);

    swprintf_s(
        processCommandLine,
        RTL_NUMBER_OF(processCommandLine),
        L"\"%s\" -newinstance -settings \"%s\"",
        appFileName,
        settingsFileName
        );

    ZeroMemory(&startupInfo, sizeof(startupInfo));
    startupInfo.cb = sizeof(startupInfo);

    if (!CreateProcessW(
        appFileName,
        processCommandLine,
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        dataDirectory,
        &startupInfo,
        &processInfo
        ))
    {
        lastError = GetLastError();

        MessageBoxW(
            NULL,
            L"无法启动 System Informer。",
            LAUNCHER_TITLE,
            MB_ICONERROR
            );
        return 1;
    }

    CloseHandle(processInfo.hThread);
    CloseHandle(processInfo.hProcess);

    return 0;
}
