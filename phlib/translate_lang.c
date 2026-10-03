/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2026
 *
 * 语言资源管理器 — 从外部 .lang 文件加载翻译表，通过状态机协调热切换。
 *
 * 设计要点：
 * - 文件缺失/损坏时静默回退嵌入表（translate_data.c），不报错不中断
 * - 换表时旧文件数据保留不释放（语言切换低频，换取查表侧无悬挂指针；
 *   translate.c 反向索引以表指针身份比对感知换表并重建）
 * - 原子切换顺序：先表指针，后计数（MemoryBarrier）
 * - 状态机仅记录当前阶段供查询（PhGetLanguageState），流程驱动在
 *   SystemInformer\langmgr.c（PhSwitchApplicationLanguage）
 */

#include <ph.h>
#include <guisup.h>
#include <translate.h>
#include <phutil.h>
#include <phnative.h>

// 嵌入表（translate_data.c，回退用）
extern const PH_TRANSLATE_ENTRY PhTranslateTable[];
extern const ULONG PhTranslateTableCount;

// === 状态机变量 ===
static PH_LANGUAGE_STATE PhpLanguageState = LanguageStateIdle;

// === 翻译表管理 ===
PPH_TRANSLATE_ENTRY PhpActiveTable = NULL;   // NULL = 使用嵌入表
ULONG PhpActiveTableCount = 0;
static PVOID PhpFileData = NULL;             // 外部文件整块内存
static BOOLEAN PhpFileTableActive = FALSE;

// === 二进制文件格式常量 ===

#define PH_LANGFILE_MAGIC     0x52544850   // "PHTR" 小端
#define PH_LANGFILE_VERSION   1

#pragma pack(push, 4)
typedef struct _PH_LANGFILE_HEADER
{
    ULONG Magic;
    ULONG Version;
    ULONG EntryCount;
    ULONG Flags;
    ULONG EntriesOffset;
    ULONG StringBlobOffset;
    ULONG StringBlobSize;
    ULONG Reserved;
} PH_LANGFILE_HEADER, *PPH_LANGFILE_HEADER;

typedef struct _PH_LANGFILE_ENTRY
{
    ULONG ZhOffset;  // 相对 StringBlobOffset 的字节偏移
    ULONG EnOffset;
} PH_LANGFILE_ENTRY, *PPH_LANGFILE_ENTRY;
#pragma pack(pop)

// === 状态机 API ===

PH_LANGUAGE_STATE PhGetLanguageState(
    VOID
    )
{
    return PhpLanguageState;
}

BOOLEAN PhIsLanguageFileLoaded(
    VOID
    )
{
    return PhpFileTableActive;
}

VOID PhSetLanguageState(
    _In_ PH_LANGUAGE_STATE State
    )
{
    PhpLanguageState = State;
}

// === 外部表加载 ===

NTSTATUS PhLoadLanguageFile(
    VOID
    )
{
    NTSTATUS status;
    PPH_STRING langFilePath;
    HANDLE fileHandle = NULL;
    LARGE_INTEGER fileSize;
    PVOID fileData = NULL;
    ULONG bytesRead;
    PPH_LANGFILE_HEADER header;
    PPH_LANGFILE_ENTRY fileEntries;
    PCHAR stringBlobBase;
    PPH_TRANSLATE_ENTRY fixedEntries;
    ULONG entryCount;
    ULONG i;

    // 路径：<AppDir>\lang\zh-en.lang
    langFilePath = PhGetApplicationDirectoryFileNameZ(L"lang\\zh-en.lang", FALSE);

    if (PhIsNullOrEmptyString(langFilePath))
        return STATUS_OBJECT_NAME_NOT_FOUND;

    // 打开文件
    status = PhCreateFileWin32(
        &fileHandle,
        PhGetString(langFilePath),
        FILE_READ_DATA | SYNCHRONIZE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ,
        FILE_OPEN,
        FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE
        );

    if (!NT_SUCCESS(status))
    {
        // 文件不存在，静默回退嵌入表
        PhDereferenceObject(langFilePath);
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    PhDereferenceObject(langFilePath);

    // 获取文件大小
    status = PhGetFileSize(fileHandle, &fileSize);

    if (!NT_SUCCESS(status) || fileSize.QuadPart == 0 || fileSize.QuadPart > 16 * 1024 * 1024)
    {
        // 过大或异常
        NtClose(fileHandle);
        return STATUS_FILE_INVALID;
    }

    // 一次性分配内存读入整个文件（大小已校验 ≤16MB）
    fileData = PhAllocate((SIZE_T)fileSize.QuadPart);

    if (!fileData)
    {
        NtClose(fileHandle);
        return STATUS_NO_MEMORY;
    }

    status = PhReadFile(fileHandle, fileData, (ULONG)fileSize.QuadPart, NULL, &bytesRead);

    NtClose(fileHandle);

    if (!NT_SUCCESS(status) || bytesRead != (ULONG)fileSize.QuadPart)
    {
        PhFree(fileData);
        return STATUS_FILE_CORRUPT_ERROR;
    }

    // 校验头部
    header = (PPH_LANGFILE_HEADER)fileData;

    if (header->Magic != PH_LANGFILE_MAGIC)
    {
        PhFree(fileData);
        return STATUS_FILE_CORRUPT_ERROR;
    }

    if (header->Version != PH_LANGFILE_VERSION)
    {
        PhFree(fileData);
        return STATUS_FILE_CORRUPT_ERROR;
    }

    entryCount = header->EntryCount;

    if (entryCount == 0 || entryCount > 100000)
    {
        PhFree(fileData);
        return STATUS_FILE_CORRUPT_ERROR;
    }

    // 校验偏移范围
    if (header->EntriesOffset < sizeof(PH_LANGFILE_HEADER) ||
        header->EntriesOffset + (ULONGLONG)entryCount * sizeof(PH_LANGFILE_ENTRY) > (ULONGLONG)fileSize.QuadPart ||
        header->StringBlobOffset < header->EntriesOffset + (ULONGLONG)entryCount * sizeof(PH_LANGFILE_ENTRY) ||
        header->StringBlobOffset + header->StringBlobSize > (ULONGLONG)fileSize.QuadPart)
    {
        PhFree(fileData);
        return STATUS_FILE_CORRUPT_ERROR;
    }

    fileEntries = (PPH_LANGFILE_ENTRY)((PCHAR)fileData + header->EntriesOffset);
    stringBlobBase = (PCHAR)fileData + header->StringBlobOffset;

    // 分配 fixup 后的条目数组
    fixedEntries = PhAllocate(entryCount * sizeof(PH_TRANSLATE_ENTRY));

    if (!fixedEntries)
    {
        PhFree(fileData);
        return STATUS_NO_MEMORY;
    }

    // 校验每条偏移并 fixup 指针
    for (i = 0; i < entryCount; i++)
    {
        ULONG zhOff = fileEntries[i].ZhOffset;
        ULONG enOff = fileEntries[i].EnOffset;

        if (zhOff >= header->StringBlobSize || enOff >= header->StringBlobSize)
        {
            // 偏移越界
            PhFree(fixedEntries);
            PhFree(fileData);
            return STATUS_FILE_CORRUPT_ERROR;
        }

        fixedEntries[i].Zh = (PCWSTR)(stringBlobBase + zhOff);
        fixedEntries[i].En = (PCWSTR)(stringBlobBase + enOff);
    }

    // 原子切换：先设置表指针，再设置计数
    // 旧文件数据（如有）保留不释放（语言切换是低频操作，可接受微量内存占用）
    PhpActiveTable = fixedEntries;
    MemoryBarrier();
    PhpActiveTableCount = entryCount;

    // 记录文件数据指针（用于卸载/调试）
    PhpFileData = fileData;
    PhpFileTableActive = TRUE;

    return STATUS_SUCCESS;
}
