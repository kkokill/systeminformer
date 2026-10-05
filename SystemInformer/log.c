/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     dmex    2016-2026
 *
 */

#include <phapp.h>
#include <phplug.h>
#include <settings.h>
#include <phsettings.h>
#include <procprv.h>

PH_CIRCULAR_BUFFER_PVOID PhLogBuffer;

VOID PhLogInitialization(
    VOID
    )
{
    ULONG entries;

    entries = PhGetIntegerSetting(SETTING_LOG_ENTRIES);
    if (entries > 0x1000) entries = 0x1000;
    PhInitializeCircularBuffer_PVOID(&PhLogBuffer, entries);
    memset(PhLogBuffer.Data, 0, sizeof(PVOID) * PhLogBuffer.Size);
}

PPH_LOG_ENTRY PhpCreateLogEntry(
    _In_ UCHAR Type,
    _In_reads_bytes_opt_(BufferLength) PVOID Buffer,
    _In_ ULONG BufferLength
    )
{
    PPH_LOG_ENTRY entry;
    SIZE_T entrySize;

    entrySize = FIELD_OFFSET(PH_LOG_ENTRY, Buffer) + BufferLength;
    entry = PhAllocate(entrySize);
    memset(entry, 0, entrySize);

    entry->Type = Type;
    PhQuerySystemTime(&entry->Time);
    entry->BufferLength = BufferLength;

    if (Buffer && BufferLength)
        memcpy(entry->Buffer, Buffer, BufferLength);

    return entry;
}

VOID PhpFreeLogEntry(
    _In_ _Post_invalid_ PPH_LOG_ENTRY Entry
    )
{
    if (Entry->Type >= PH_LOG_ENTRY_PROCESS_FIRST && Entry->Type <= PH_LOG_ENTRY_PROCESS_LAST)
    {
        PhDereferenceObject(Entry->Process.Name);
        if (Entry->Process.ParentName) PhDereferenceObject(Entry->Process.ParentName);
        if (Entry->Process.Record) PhDereferenceProcessRecord(Entry->Process.Record);
    }
    else if (Entry->Type >= PH_LOG_ENTRY_SERVICE_FIRST && Entry->Type <= PH_LOG_ENTRY_SERVICE_LAST)
    {
        PhDereferenceObject(Entry->Service.Name);
        PhDereferenceObject(Entry->Service.DisplayName);
    }
    else if (Entry->Type == PH_LOG_ENTRY_MESSAGE)
    {
        PhDereferenceObject(Entry->Message);
    }

    PhFree(Entry);
}

PPH_LOG_ENTRY PhpCreateProcessLogEntry(
    _In_ UCHAR Type,
    _In_ HANDLE ProcessId,
    _In_ PPH_STRING Name,
    _In_opt_ HANDLE ParentProcessId,
    _In_opt_ PPH_STRING ParentName,
    _In_opt_ ULONG Status,
    _In_opt_ PPH_PROCESS_RECORD Record
    )
{
    PPH_LOG_ENTRY entry;

    entry = PhpCreateLogEntry(Type, NULL, 0);
    entry->Process.ProcessId = ProcessId;
    PhReferenceObject(Name);
    entry->Process.Name = Name;

    entry->Process.ParentProcessId = ParentProcessId;

    if (!PhIsNullOrEmptyString(ParentName))
    {
        PhReferenceObject(ParentName);
        entry->Process.ParentName = ParentName;
    }

    entry->Process.ExitStatus = Status;

    if (Record)
    {
        PhReferenceProcessRecord(Record);
        entry->Process.Record = Record;
    }

    return entry;
}

PPH_LOG_ENTRY PhpCreateServiceLogEntry(
    _In_ UCHAR Type,
    _In_ PPH_STRING Name,
    _In_ PPH_STRING DisplayName
    )
{
    PPH_LOG_ENTRY entry;

    entry = PhpCreateLogEntry(Type, NULL, 0);
    PhReferenceObject(Name);
    entry->Service.Name = Name;
    PhReferenceObject(DisplayName);
    entry->Service.DisplayName = DisplayName;

    return entry;
}

PPH_LOG_ENTRY PhpCreateDeviceLogEntry(
    _In_ UCHAR Type,
    _In_ PPH_STRING Classification,
    _In_ PPH_STRING Name
    )
{
    PPH_LOG_ENTRY entry;

    entry = PhpCreateLogEntry(Type, NULL, 0);
    PhReferenceObject(Classification);
    entry->Device.Classification = Classification;
    PhReferenceObject(Name);
    entry->Device.Name = Name;

    return entry;
}

PPH_LOG_ENTRY PhpCreateMessageLogEntry(
    _In_ UCHAR Type,
    _In_ PPH_STRING Message
    )
{
    PPH_LOG_ENTRY entry;

    entry = PhpCreateLogEntry(Type, NULL, 0);
    PhReferenceObject(Message);
    entry->Message = Message;

    return entry;
}

VOID PhpLogEntry(
    _In_ PPH_LOG_ENTRY Entry
    )
{
    PPH_LOG_ENTRY oldEntry;

    oldEntry = PhAddItemCircularBuffer2_PVOID(&PhLogBuffer, Entry);

    if (oldEntry)
        PhpFreeLogEntry(oldEntry);

    PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackLoggedEvent), Entry);
}

VOID PhClearLogEntries(
    VOID
    )
{
    ULONG i;

    for (i = 0; i < PhLogBuffer.Size; i++)
    {
        if (PhLogBuffer.Data[i])
            PhpFreeLogEntry(PhLogBuffer.Data[i]);
    }

    PhClearCircularBuffer_PVOID(&PhLogBuffer);
    memset(PhLogBuffer.Data, 0, sizeof(PVOID) * PhLogBuffer.Size);
}

VOID PhLogProcessEntry(
    _In_ UCHAR Type,
    _In_ HANDLE ProcessId,
    _In_ PPH_STRING Name,
    _In_opt_ HANDLE ParentProcessId,
    _In_opt_ PPH_STRING ParentName,
    _In_opt_ ULONG Status,
    _In_opt_ PPH_PROCESS_RECORD Record
    )
{
    PhpLogEntry(PhpCreateProcessLogEntry(Type, ProcessId, Name, ParentProcessId, ParentName, Status, Record));
}

VOID PhLogServiceEntry(
    _In_ UCHAR Type,
    _In_ PPH_STRING Name,
    _In_ PPH_STRING DisplayName
    )
{
    PhpLogEntry(PhpCreateServiceLogEntry(Type, Name, DisplayName));
}

VOID PhLogDeviceEntry(
    _In_ UCHAR Type,
    _In_ PPH_STRING Classification,
    _In_ PPH_STRING Name
    )
{
    PhpLogEntry(PhpCreateDeviceLogEntry(Type, Classification, Name));
}

VOID PhLogMessageEntry(
    _In_ UCHAR Type,
    _In_ PPH_STRING Message
    )
{
    PhLogMessageEntryEx(Type, Message, NULL, 0);
}

VOID PhLogMessageEntryEx(
    _In_ UCHAR Type,
    _In_ PPH_STRING Message,
    _In_reads_bytes_opt_(BufferLength) PVOID Buffer,
    _In_ ULONG BufferLength
    )
{
    PPH_LOG_ENTRY entry;

    entry = PhpCreateLogEntry(Type, Buffer, BufferLength);
    PhReferenceObject(Message);
    entry->Message = Message;

    PhpLogEntry(entry);
}

PPH_STRING PhpFormatLogEntryToBuffer(
    _In_ PPH_FORMAT Format,
    _In_ ULONG Count
    )
{
    SIZE_T formatLength;
    WCHAR formatBuffer[0x80];

    if (PhFormatToBuffer(
        Format,
        Count,
        formatBuffer,
        sizeof(formatBuffer),
        &formatLength
        ))
    {
        PH_STRINGREF text;

        text.Length = formatLength - sizeof(UNICODE_NULL);
        text.Buffer = formatBuffer;

        return PhCreateString2(&text);
    }

    return PhFormat(Format, Count, 0x80);
}

static PPH_STRING PhpFormatLogEntryExtra(
    _In_ PPH_LOG_ENTRY Entry
    )
{
    if (Entry->BufferLength == 0)
        return PhReferenceEmptyString();

    return PhCreateStringEx((PVOID)Entry->Buffer, Entry->BufferLength);
}

PPH_STRING PhFormatLogEntry(
    _In_ PPH_LOG_ENTRY Entry
    )
{
    switch (Entry->Type)
    {
    case PH_LOG_ENTRY_PROCESS_CREATE:
        {
            PH_FORMAT format[8];

            // 进程已创建：%s (%lu) 由 %s (%lu) 启动
            //PhInitFormatS(&format[0], L"进程已创建：");
            PhInitFormatSR(&format[0], Entry->Process.Name->sr);
            PhInitFormatS(&format[1], PhTranslateTextZ(L"（"));
            PhInitFormatU(&format[2], HandleToUlong(Entry->Process.ProcessId));
            PhInitFormatS(&format[3], PhTranslateTextZ(L"）由 "));
            if (Entry->Process.ParentName)
                PhInitFormatSR(&format[4], Entry->Process.ParentName->sr);
            else
                PhInitFormatS(&format[4], PhTranslateTextZ(L"未知进程"));
            PhInitFormatS(&format[5], PhTranslateTextZ(L"（"));
            PhInitFormatU(&format[6], HandleToUlong(Entry->Process.ParentProcessId));
            PhInitFormatS(&format[7], PhTranslateTextZ(L"）"));

            //return PhFormatString(
            //    L"进程已创建：%s (%lu) 由 %s (%lu) 启动",
            //    Entry->Process.Name->Buffer,
            //    HandleToUlong(Entry->Process.ProcessId),
            //    PhGetStringOrDefault(Entry->Process.ParentName, L"未知进程"),
            //    HandleToUlong(Entry->Process.ParentProcessId)
            //    );
            return PhpFormatLogEntryToBuffer(format, RTL_NUMBER_OF(format));
        }
    case PH_LOG_ENTRY_PROCESS_DELETE:
        {
            PH_FORMAT format[5];

            // 进程已终止：%s (%lu)；退出状态 0x%x
            //PhInitFormatS(&format[0], L"进程已终止：");
            PhInitFormatSR(&format[0], Entry->Process.Name->sr);
            PhInitFormatS(&format[1], PhTranslateTextZ(L"（"));
            PhInitFormatU(&format[2], HandleToUlong(Entry->Process.ProcessId));
            PhInitFormatS(&format[3], PhTranslateTextZ(L"）；退出状态 "));
            PhInitFormatX(&format[4], Entry->Process.ExitStatus);

            //return PhFormatString(
            //    L"进程已终止：%s (%lu)；退出状态 0x%x",
            //    Entry->Process.Name->Buffer,
            //    HandleToUlong(Entry->Process.ProcessId),
            //    Entry->Process.ExitStatus
            //    );
            return PhpFormatLogEntryToBuffer(format, RTL_NUMBER_OF(format));
        }
    case PH_LOG_ENTRY_SERVICE_CREATE:
        {
            PH_FORMAT format[4];

            // 服务已创建：%s (%s)
            //PhInitFormatS(&format[0], L"服务已创建：");
            PhInitFormatSR(&format[0], Entry->Service.Name->sr);
            PhInitFormatS(&format[1], PhTranslateTextZ(L"（"));
            PhInitFormatSR(&format[2], Entry->Service.DisplayName->sr);
            PhInitFormatS(&format[3], PhTranslateTextZ(L"）"));

            //return PhFormatString(
            //    L"服务已创建：%s (%s)",
            //    Entry->Service.Name->Buffer,
            //    Entry->Service.DisplayName->Buffer
            //    );
            return PhpFormatLogEntryToBuffer(format, RTL_NUMBER_OF(format));
        }
    case PH_LOG_ENTRY_SERVICE_DELETE:
        {
            PH_FORMAT format[4];

            // 服务已删除：%s (%s)
            //PhInitFormatS(&format[0], L"服务已删除：");
            PhInitFormatSR(&format[0], Entry->Service.Name->sr);
            PhInitFormatS(&format[1], PhTranslateTextZ(L"（"));
            PhInitFormatSR(&format[2], Entry->Service.DisplayName->sr);
            PhInitFormatS(&format[3], PhTranslateTextZ(L"）"));

            //return PhFormatString(
            //    L"服务已删除：%s (%s)",
            //    Entry->Service.Name->Buffer,
            //    Entry->Service.DisplayName->Buffer
            //    );
            return PhpFormatLogEntryToBuffer(format, RTL_NUMBER_OF(format));
        }
    case PH_LOG_ENTRY_SERVICE_START:
        {
            PH_FORMAT format[4];

            // 服务已启动：%s (%s)
            //PhInitFormatS(&format[0], L"服务已启动：");
            PhInitFormatSR(&format[0], Entry->Service.Name->sr);
            PhInitFormatS(&format[1], PhTranslateTextZ(L"（"));
            PhInitFormatSR(&format[2], Entry->Service.DisplayName->sr);
            PhInitFormatS(&format[3], PhTranslateTextZ(L"）"));

            //return PhFormatString(
            //    L"服务已启动：%s (%s)",
            //    Entry->Service.Name->Buffer,
            //    Entry->Service.DisplayName->Buffer
            //    );
            return PhpFormatLogEntryToBuffer(format, RTL_NUMBER_OF(format));
        }
    case PH_LOG_ENTRY_SERVICE_STOP:
        {
            PH_FORMAT format[4];

            // 服务已停止：%s (%s)
            //PhInitFormatS(&format[0], L"服务已停止：");
            PhInitFormatSR(&format[0], Entry->Service.Name->sr);
            PhInitFormatS(&format[1], PhTranslateTextZ(L"（"));
            PhInitFormatSR(&format[2], Entry->Service.DisplayName->sr);
            PhInitFormatS(&format[3], PhTranslateTextZ(L"）"));

            //return PhFormatString(
            //    L"服务已停止：%s (%s)",
            //    Entry->Service.Name->Buffer,
            //    Entry->Service.DisplayName->Buffer
            //    );
            return PhpFormatLogEntryToBuffer(format, RTL_NUMBER_OF(format));
        }
    case PH_LOG_ENTRY_SERVICE_CONTINUE:
        {
            PH_FORMAT format[4];

            // 服务已继续：%s (%s)
            //PhInitFormatS(&format[0], L"服务已继续：");
            PhInitFormatSR(&format[0], Entry->Service.Name->sr);
            PhInitFormatS(&format[1], PhTranslateTextZ(L"（"));
            PhInitFormatSR(&format[2], Entry->Service.DisplayName->sr);
            PhInitFormatS(&format[3], PhTranslateTextZ(L"）"));

            //return PhFormatString(
            //    L"服务已继续：%s (%s)",
            //    Entry->Service.Name->Buffer,
            //    Entry->Service.DisplayName->Buffer
            //    );
            return PhpFormatLogEntryToBuffer(format, RTL_NUMBER_OF(format));
        }
    case PH_LOG_ENTRY_SERVICE_PAUSE:
        {
            PH_FORMAT format[4];

            // 服务已暂停：%s (%s)
            //PhInitFormatS(&format[0], L"服务已暂停：");
            PhInitFormatSR(&format[0], Entry->Service.Name->sr);
            PhInitFormatS(&format[1], PhTranslateTextZ(L"（"));
            PhInitFormatSR(&format[2], Entry->Service.DisplayName->sr);
            PhInitFormatS(&format[3], PhTranslateTextZ(L"）"));

            //return PhFormatString(
            //    L"服务已暂停：%s (%s)",
            //    Entry->Service.Name->Buffer,
            //    Entry->Service.DisplayName->Buffer
            //    );
            return PhpFormatLogEntryToBuffer(format, RTL_NUMBER_OF(format));
        }
    case PH_LOG_ENTRY_SERVICE_MODIFIED:
        {
            PH_FORMAT format[4];

            // 服务已修改：%s (%s)
            //PhInitFormatS(&format[0], L"服务已修改：");
            PhInitFormatSR(&format[0], Entry->Service.Name->sr);
            PhInitFormatS(&format[1], PhTranslateTextZ(L"（"));
            PhInitFormatSR(&format[2], Entry->Service.DisplayName->sr);
            PhInitFormatS(&format[3], PhTranslateTextZ(L"）"));

            //return PhFormatString(
            //    L"服务已修改：%s (%s)",
            //    Entry->Service.Name->Buffer,
            //    Entry->Service.DisplayName->Buffer
            //    );
            return PhpFormatLogEntryToBuffer(format, RTL_NUMBER_OF(format));
        }
    case PH_LOG_ENTRY_DEVICE_REMOVED:
        {
            PH_FORMAT format[4];

            //PhInitFormatS(&format[0], L"设备已移除：");
            PhInitFormatSR(&format[0], Entry->Device.Classification->sr);
            PhInitFormatS(&format[1], PhTranslateTextZ(L"（"));
            PhInitFormatSR(&format[2], Entry->Device.Name->sr);
            PhInitFormatS(&format[3], PhTranslateTextZ(L"）"));

            return PhpFormatLogEntryToBuffer(format, RTL_NUMBER_OF(format));
        }
    case PH_LOG_ENTRY_DEVICE_ARRIVED:
        {
            PH_FORMAT format[4];

            //PhInitFormatS(&format[0], L"设备已到达：");
            PhInitFormatSR(&format[0], Entry->Device.Classification->sr);
            PhInitFormatS(&format[1], PhTranslateTextZ(L"（"));
            PhInitFormatSR(&format[2], Entry->Device.Name->sr);
            PhInitFormatS(&format[3], PhTranslateTextZ(L"）"));

            return PhpFormatLogEntryToBuffer(format, RTL_NUMBER_OF(format));
        }
    case PH_LOG_ENTRY_MESSAGE:
        {
            PPH_STRING extraString;

            PhReferenceObject(Entry->Message);

            if (Entry->BufferLength == 0)
                return Entry->Message;

            extraString = PH_AUTO_T(PH_STRING, PhpFormatLogEntryExtra(Entry));
            return PhaFormatString(PhTranslateTextZ(L"%s [附加信息：%s]"), Entry->Message->Buffer, extraString->Buffer);
        }
    default:
        {
            PPH_STRING extraString;

            if (Entry->BufferLength == 0)
                return PhReferenceEmptyString();

            extraString = PH_AUTO_T(PH_STRING, PhpFormatLogEntryExtra(Entry));
            return PhaFormatString(PhTranslateTextZ(L"[附加信息：%s]"), extraString->Buffer);
        }
    }
}

// Entries below PH_LOG_ENTRY_MESSAGE must stay dense and ordered by type: PhIndexStringRefSiKeyValuePairs
// indexes the array directly when the type is less than the element count, and only searches beyond it.
static CONST PH_KEY_VALUE_PAIR PhpLogEntryTypePairs[] =
{
    SIP(SREF(L"未知"), 0),
    SIP(SREF(L"进程已创建"), PH_LOG_ENTRY_PROCESS_CREATE),
    SIP(SREF(L"进程已终止"), PH_LOG_ENTRY_PROCESS_DELETE),
    SIP(SREF(L"服务已创建"), PH_LOG_ENTRY_SERVICE_CREATE),
    SIP(SREF(L"服务已删除"), PH_LOG_ENTRY_SERVICE_DELETE),
    SIP(SREF(L"服务已启动"), PH_LOG_ENTRY_SERVICE_START),
    SIP(SREF(L"服务已停止"), PH_LOG_ENTRY_SERVICE_STOP),
    SIP(SREF(L"服务已继续"), PH_LOG_ENTRY_SERVICE_CONTINUE),
    SIP(SREF(L"服务已暂停"), PH_LOG_ENTRY_SERVICE_PAUSE),
    SIP(SREF(L"服务已修改"), PH_LOG_ENTRY_SERVICE_MODIFIED),
    SIP(SREF(L"未知"), PH_LOG_ENTRY_SERVICE_LAST), // padding, no entry uses this type
    SIP(SREF(L"设备已移除"), PH_LOG_ENTRY_DEVICE_REMOVED),
    SIP(SREF(L"设备已到达"), PH_LOG_ENTRY_DEVICE_ARRIVED),
    SIP(SREF(L"消息"), PH_LOG_ENTRY_MESSAGE)
};

PCPH_STRINGREF PhFormatLogType(
    _In_ PPH_LOG_ENTRY Entry
    )
{
    PCPH_STRINGREF string;

    if (PhIndexStringRefSiKeyValuePairs(
        PhpLogEntryTypePairs,
        sizeof(PhpLogEntryTypePairs),
        Entry->Type,
        &string
        ))
    {
        return string;
    }

    // Callers format the result without checking, so never return NULL.
    return (PCPH_STRINGREF)PhpLogEntryTypePairs[0].Key;
}
