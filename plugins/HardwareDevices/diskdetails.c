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

#include "devices.h"

VOID DiskDeviceAddListViewItems(
    _Inout_ PDV_DISK_PAGE_CONTEXT Context
    )
{
    PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_FS_CREATION_TIME, L"卷创建时间", NULL);
    PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_SERIAL_NUMBER, L"卷序列号", NULL);
    PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_UNIQUEID, L"卷唯一标识符", NULL);
    PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_PARTITIONID, L"卷分区标识符", NULL);
    PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_FILE_SYSTEM, L"卷文件系统", NULL);
    PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_FS_VERSION, L"卷版本", NULL);
    PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_LFS_VERSION, L"LFS 版本", NULL);

    //PhAddListViewItem(Context->ListViewHandle, MAXINT, L"BytesPerPhysicalSector", NULL);
    PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_TOTAL_SIZE, L"总大小", NULL);
    PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_TOTAL_FREE, L"总空闲", NULL);
    PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_TOTAL_USED, L"总已用", NULL);
    PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_TOTAL_SECTORS, L"总扇区数", NULL);
    PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_TOTAL_CLUSTERS, L"总簇数", NULL);
    PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_FREE_CLUSTERS, L"空闲簇数", NULL);
    PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_TOTAL_RESERVED, L"保留簇数", NULL);
    PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_TOTAL_BYTES_PER_SECTOR, L"每扇区字节数", NULL);
    PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_TOTAL_BYTES_PER_CLUSTER, L"每簇字节数", NULL);
    PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_TOTAL_BYTES_PER_RECORD, L"每文件记录段字节数", NULL);
    PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_TOTAL_CLUSTERS_PER_RECORD, L"每文件记录段簇数", NULL);

    PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_MFT_RECORDS, L"MFT 记录数", NULL);
    PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_MFT_SIZE, L"MFT 大小", NULL);
    PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_MFT_START, L"MFT 起始位置", NULL);
    PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_MFT_ZONE, L"MFT 区域簇数", NULL);
    PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_MFT_ZONE_SIZE, L"MFT 区域大小", NULL);
    PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_MFT_MIRROR_START, L"MFT 镜像起始位置", NULL);

    PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_FILE_READS, L"文件读取次数", NULL);
    PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_FILE_WRITES, L"文件写入次数", NULL);
    PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_DISK_READS, L"磁盘读取次数", NULL);
    PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_DISK_WRITES, L"磁盘写入次数", NULL);
    PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_FILE_READ_BYTES, L"文件读取字节数", NULL);
    PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_FILE_WRITE_BYTES, L"文件写入字节数", NULL);

    PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_METADATA_READS, L"元数据读取次数", NULL);
    PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_METADATA_WRITES, L"元数据写入次数", NULL);
    PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_METADATA_DISK_READS, L"元数据磁盘读取次数", NULL);
    PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_METADATA_DISK_WRITES, L"元数据磁盘写入次数", NULL);
    PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_METADATA_READ_BYTES, L"元数据读取字节数", NULL);
    PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_METADATA_WRITE_BYTES, L"元数据写入字节数", NULL);

    PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_MFT_READS, L"MFT 读取次数", NULL);
    PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_MFT_WRITES, L"MFT 写入次数", NULL);
    PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_MFT_READ_BYTES, L"MFT 读取字节数", NULL);
    PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_MFT_WRITE_BYTES, L"MFT 写入字节数", NULL);

    //PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_ROOT_INDEX_READS, L"RootIndex reads", NULL);
    //PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_ROOT_INDEX_WRITES, L"RootIndex writes", NULL);
    //PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_ROOT_INDEX_READ_BYTES, L"RootIndex read bytes", NULL);
    //PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_ROOT_INDEX_WRITE_BYTES, L"RootIndex write bytes", NULL);

    //PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_BITMAP_READS, L"Bitmap reads", NULL);
    //PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_BITMAP_WRITES, L"Bitmap writes", NULL);
    //PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_BITMAP_READ_BYTES, L"Bitmap read bytes", NULL);
    //PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_BITMAP_WRITE_BYTES, L"Bitmap write bytes", NULL);

    //PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_MFT_BITMAP_READS, L"Mft bitmap reads", NULL);
    //PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_MFT_BITMAP_WRITES, L"Mft bitmap writes", NULL);
    //PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_MFT_BITMAP_READ_BYTES, L"Mft bitmap read bytes", NULL);
    //PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_MFT_BITMAP_WRITE_BYTES, L"Mft bitmap write bytes", NULL);

    //PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_USER_INDEX_READS, L"User Index reads", NULL);
    //PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_USER_INDEX_WRITES, L"User Index writes", NULL);
    //PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_USER_INDEX_READ_BYTES, L"User Index read bytes", NULL);
    //PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_USER_INDEX_WRITE_BYTES, L"User Index write bytes", NULL);

    //PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_LOGFILE_READS, L"LogFile reads", NULL);
    //PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_LOGFILE_WRITES, L"LogFile writes", NULL);
    //PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_LOGFILE_READ_BYTES, L"LogFile read bytes", NULL);
    //PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_LOGFILE_WRITE_BYTES, L"LogFile write bytes", NULL);

    PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_OTHER_EXCEPTIONS, L"异常", NULL);

    if (PhWindowsVersion >= WINDOWS_8_1)
    {
        PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_RESOURCES_EXHAUSTED, L"资源耗尽", NULL);
    }

    if (PhWindowsVersion >= WINDOWS_10)
    {
        PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_VOLUME_TRIM_COUNT, L"卷 Trim 次数", NULL);
        PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_VOLUME_TRIM_TIME, L"卷 Trim 时间", NULL);
        PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_VOLUME_TRIM_BYTES, L"卷 Trim 字节数", NULL);
        PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_VOLUME_TRIM_SKIPPED_COUNT, L"卷 Trim 跳过次数", NULL);
        PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_VOLUME_TRIM_SKIPPED_BYTES, L"卷 Trim 跳过字节数", NULL);
        PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_FILE_TRIM_COUNT, L"文件 Trim 次数", NULL);
        PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_FILE_TRIM_TIME, L"文件 Trim 时间", NULL);
        PhAddListViewItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_FILE_TRIM_BYTES, L"文件 Trim 字节数", NULL);
    }
}

VOID AddNvmeSmartEntry(
    _In_ HWND ListViewHandle, 
    _In_ LPCWSTR Name, 
    _In_ PPH_STRING Value
    )
{
    INT lvItemIndex = PhAddListViewItem(
        ListViewHandle,
        MAXINT,
        Name,
        0
        );

    PhSetListViewSubItem(
        ListViewHandle,
        lvItemIndex,
        1,
        PhGetString(Value)
        );
}

VOID DiskDeviceQuerySmart(
    _Inout_ PDV_DISK_PAGE_CONTEXT Context
    )
{
    PPH_LIST attributes;
    HANDLE deviceHandle;

    if (NT_SUCCESS(PhCreateFile(
        &deviceHandle,
        &Context->PageContext->DiskId.DevicePath->sr,
        FILE_READ_ATTRIBUTES | SYNCHRONIZE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        )))
    {
        if (NT_SUCCESS(DiskDriveQueryImminentFailure(deviceHandle, &attributes)))
        {
            for (ULONG i = 0; i < attributes->Count; i++)
            {
                PSMART_ATTRIBUTES attribute = attributes->Items[i];

                INT lvItemIndex = PhAddListViewItem(
                    Context->ListViewHandle,
                    MAXINT,
                    SmartAttributeGetText(attribute->AttributeId),
                    IntToPtr(attribute->AttributeId)
                    );

                PhSetListViewSubItem(
                    Context->ListViewHandle,
                    lvItemIndex,
                    1,
                    PhaFormatUInt64(attribute->CurrentValue, TRUE)->Buffer
                    );
                PhSetListViewSubItem(
                    Context->ListViewHandle,
                    lvItemIndex,
                    2,
                    PhaFormatUInt64(attribute->WorstValue, TRUE)->Buffer
                    );

                if (attribute->RawValue)
                {
                    PhSetListViewSubItem(
                        Context->ListViewHandle,
                        lvItemIndex,
                        3,
                        PhaFormatUInt64(attribute->RawValue, TRUE)->Buffer
                        );

                    PhSetListViewSubItem(
                        Context->ListViewHandle,
                        lvItemIndex,
                        4,
                        PhaFormatString(L"%#014x", attribute->RawValue)->Buffer
                        );
                }

                //PhSetListViewSubItem(
                //    Context->ListViewHandle,
                //    lvItemIndex,
                //    5,
                //    PhaFormatString(L"%lu", attribute->Advisory)->Buffer
                //    );
                //PhSetListViewSubItem(
                //    Context->ListViewHandle,
                //    lvItemIndex,
                //    6,
                //    PhaFormatString(L"%lu", attribute->FailureImminent)->Buffer
                //    );

                PhFree(attribute);
            }

            PhDereferenceObject(attributes);
        }
        else
        {
            NVME_HEALTH_INFO_LOG healthInfo;

            if (NT_SUCCESS(DiskDriveQueryNvmeHealthInfo(deviceHandle, &healthInfo)))
            {
                // https://en.wikipedia.org/wiki/Self-Monitoring,_Analysis_and_Reporting_Technology#Known_NVMe_S.M.A.R.T._attributes

                AddNvmeSmartEntry(Context->ListViewHandle, L"可用备用空间低于阈值", PhaFormatUInt64(healthInfo.CriticalWarning.AvailableSpaceLow, TRUE));
                AddNvmeSmartEntry(Context->ListViewHandle, L"温度超过阈值", PhaFormatUInt64(healthInfo.CriticalWarning.TemperatureThreshold, TRUE));
                AddNvmeSmartEntry(Context->ListViewHandle, L"驱动器可靠性已降级", PhaFormatUInt64(healthInfo.CriticalWarning.ReliabilityDegraded, TRUE));
                AddNvmeSmartEntry(Context->ListViewHandle, L"驱动器处于只读模式", PhaFormatUInt64(healthInfo.CriticalWarning.ReadOnly, TRUE));
                AddNvmeSmartEntry(Context->ListViewHandle, L"易失性内存备份设备故障", PhaFormatUInt64(healthInfo.CriticalWarning.VolatileMemoryBackupDeviceFailed, TRUE));

                AddNvmeSmartEntry(Context->ListViewHandle, L"温度", PhaFormatString(L"%u\u00b0C", *(PUSHORT)(healthInfo.Temperature) - 273));

                AddNvmeSmartEntry(Context->ListViewHandle, L"可用备用空间", PhaFormatString(L"%u%%", healthInfo.AvailableSpare));
                AddNvmeSmartEntry(Context->ListViewHandle, L"可用备用空间阈值", PhaFormatString(L"%u%%", healthInfo.AvailableSpareThreshold));
                AddNvmeSmartEntry(Context->ListViewHandle, L"已用百分比", PhaFormatString(L"%u%%", healthInfo.PercentageUsed));

                AddNvmeSmartEntry(Context->ListViewHandle, L"已读取数据", PhaFormatSize(*(ULONG64*)(healthInfo.DataUnitRead) * 1000 * 512, ULONG_MAX));
                AddNvmeSmartEntry(Context->ListViewHandle, L"已写入数据", PhaFormatSize(*(ULONG64*)(healthInfo.DataUnitWritten) * 1000 * 512, ULONG_MAX));

                AddNvmeSmartEntry(Context->ListViewHandle, L"主机读取命令数", PhaFormatUInt64(*(ULONG64*)(healthInfo.HostReadCommands), TRUE));
                AddNvmeSmartEntry(Context->ListViewHandle, L"主机写入命令数", PhaFormatUInt64(*(ULONG64*)(healthInfo.HostWrittenCommands), TRUE));
                AddNvmeSmartEntry(Context->ListViewHandle, L"控制器忙碌时间", PhaFormatUInt64(*(ULONG64*)(healthInfo.ControllerBusyTime), TRUE));

                AddNvmeSmartEntry(Context->ListViewHandle, L"通电周期数", PhaFormatUInt64(*(ULONG64*)(healthInfo.PowerCycle), TRUE));
                AddNvmeSmartEntry(Context->ListViewHandle, L"通电小时数", PhaFormatUInt64(*(ULONG64*)(healthInfo.PowerOnHours), TRUE));
                AddNvmeSmartEntry(Context->ListViewHandle, L"非安全关机次数", PhaFormatUInt64(*(ULONG64*)(healthInfo.UnsafeShutdowns), TRUE));
                AddNvmeSmartEntry(Context->ListViewHandle, L"介质和数据完整性错误", PhaFormatUInt64(*(ULONG64*)(healthInfo.MediaErrors), TRUE));
                AddNvmeSmartEntry(Context->ListViewHandle, L"错误信息日志条目数", PhaFormatUInt64(*(ULONG64*)(healthInfo.ErrorInfoLogEntryCount), TRUE));

                AddNvmeSmartEntry(Context->ListViewHandle, L"警告复合温度时间", PhaFormatUInt64(healthInfo.WarningCompositeTemperatureTime, TRUE));
                AddNvmeSmartEntry(Context->ListViewHandle, L"严重复合温度时间", PhaFormatUInt64(healthInfo.CriticalCompositeTemperatureTime, TRUE));
            }
        }

        NtClose(deviceHandle);
    }
}

//COLORREF NTAPI PhpColorItemColorFunction(
//    _In_ INT Index,
//    _In_ PVOID Param,
//    _In_opt_ PVOID Context
//    )
//{
//    PSMART_ATTRIBUTES item = Param;
//
//    if (item->FailureImminent)
//        return RGB(255, 119, 0);// RGB(255, 60, 40);
//
//    return RGB(0xFF, 0xFF, 0xFF);
//}

VOID DiskDeviceQueryVolumeInfo(
    _Inout_ PDV_DISK_PAGE_CONTEXT Context,
    _In_ USHORT Type,
    _In_ INT Column,
    _In_ HANDLE DeviceHandle
    )
{
    if (Type == FILESYSTEM_STATISTICS_TYPE_NTFS)
    {
        NTFS_VOLUME_INFO ntfsVolumeInfo;

        PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_FILE_SYSTEM, Column, L"NTFS");

        if (DiskDriveQueryNtfsVolumeInfo(DeviceHandle, &ntfsVolumeInfo))
        {
            ntfsVolumeInfo.VolumeData.VolumeSerialNumber.QuadPart = _byteswap_uint64(ntfsVolumeInfo.VolumeData.VolumeSerialNumber.QuadPart);

            PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_SERIAL_NUMBER, Column,
                PhaFormatString(L"0x%s", PH_AUTO_T(PH_STRING, PhBufferToHexString((PUCHAR)&ntfsVolumeInfo.VolumeData.VolumeSerialNumber.QuadPart, sizeof(ntfsVolumeInfo.VolumeData.VolumeSerialNumber.QuadPart)))->Buffer)->Buffer);
            PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_FS_VERSION, Column,
                PhaFormatString(L"%lu.%lu", ntfsVolumeInfo.VolumeDataEx.MajorVersion, ntfsVolumeInfo.VolumeDataEx.MinorVersion)->Buffer);
            PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_LFS_VERSION, Column,
                PhaFormatString(L"%lu.%lu", ntfsVolumeInfo.VolumeDataEx.LfsMajorVersion, ntfsVolumeInfo.VolumeDataEx.LfsMinorVersion)->Buffer);
            //PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, Column,
            //    PhaFormatSize(ntfsVolumeInfo.ExtendedVolumeData.BytesPerPhysicalSector, ULONG_MAX)->Buffer);
            PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_TOTAL_SIZE, Column,
                PhaFormatSize(ntfsVolumeInfo.VolumeData.NumberSectors.QuadPart * ntfsVolumeInfo.VolumeData.BytesPerSector, ULONG_MAX)->Buffer);
            PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_TOTAL_FREE, Column,
                PhaFormatString(L"%s (%.2f%%)", PhaFormatSize(ntfsVolumeInfo.VolumeData.FreeClusters.QuadPart * ntfsVolumeInfo.VolumeData.BytesPerCluster, ULONG_MAX)->Buffer,
                (FLOAT)(ntfsVolumeInfo.VolumeData.FreeClusters.QuadPart * 100.f) / ntfsVolumeInfo.VolumeData.TotalClusters.QuadPart)->Buffer);
            PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_TOTAL_USED, Column,
                PhaFormatString(L"%s (%.2f%%)", PhaFormatSize(
                (ntfsVolumeInfo.VolumeData.NumberSectors.QuadPart * ntfsVolumeInfo.VolumeData.BytesPerSector) - (ntfsVolumeInfo.VolumeData.FreeClusters.QuadPart * ntfsVolumeInfo.VolumeData.BytesPerCluster), ULONG_MAX)->Buffer,
                100.f - (FLOAT)(ntfsVolumeInfo.VolumeData.FreeClusters.QuadPart * 100) / ntfsVolumeInfo.VolumeData.TotalClusters.QuadPart)->Buffer); // HACK (dmex)
            PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_TOTAL_SECTORS, Column,
                PhaFormatUInt64(ntfsVolumeInfo.VolumeData.NumberSectors.QuadPart, TRUE)->Buffer);
            PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_TOTAL_CLUSTERS, Column,
                PhaFormatUInt64(ntfsVolumeInfo.VolumeData.TotalClusters.QuadPart, TRUE)->Buffer);
            PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_FREE_CLUSTERS, Column,
                PhaFormatUInt64(ntfsVolumeInfo.VolumeData.FreeClusters.QuadPart, TRUE)->Buffer);
            PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_TOTAL_RESERVED, Column,
                PhaFormatUInt64(ntfsVolumeInfo.VolumeData.TotalReserved.QuadPart, TRUE)->Buffer);
            PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_TOTAL_BYTES_PER_SECTOR, Column,
                PhaFormatString(L"%lu", ntfsVolumeInfo.VolumeData.BytesPerSector)->Buffer);
            PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_TOTAL_BYTES_PER_CLUSTER, Column,
                PhaFormatString(L"%lu", ntfsVolumeInfo.VolumeData.BytesPerCluster)->Buffer);
            PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_TOTAL_BYTES_PER_RECORD, Column,
                PhaFormatString(L"%lu", ntfsVolumeInfo.VolumeData.BytesPerFileRecordSegment)->Buffer);
            PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_TOTAL_CLUSTERS_PER_RECORD, Column,
                PhaFormatString(L"%lu", ntfsVolumeInfo.VolumeData.ClustersPerFileRecordSegment)->Buffer);
            PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_MFT_RECORDS, Column,
                PhaFormatUInt64(ntfsVolumeInfo.VolumeData.MftValidDataLength.QuadPart / ntfsVolumeInfo.VolumeData.BytesPerFileRecordSegment, TRUE)->Buffer);
            PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_MFT_SIZE, Column,
                PhaFormatString(L"%s (%.2f%%)", PhaFormatSize(ntfsVolumeInfo.VolumeData.MftValidDataLength.QuadPart, ULONG_MAX)->Buffer, ((FLOAT)ntfsVolumeInfo.VolumeData.MftValidDataLength.QuadPart / ntfsVolumeInfo.VolumeData.BytesPerCluster * 100.f) / ntfsVolumeInfo.VolumeData.TotalClusters.QuadPart)->Buffer);
            PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_MFT_START, Column,
                PhaFormatString(L"%I64u", ntfsVolumeInfo.VolumeData.MftStartLcn.QuadPart)->Buffer);
            PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_MFT_ZONE, Column,
                PhaFormatString(L"%I64u - %I64u", ntfsVolumeInfo.VolumeData.MftZoneStart.QuadPart, ntfsVolumeInfo.VolumeData.MftZoneEnd.QuadPart)->Buffer);
            PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_MFT_ZONE_SIZE, Column,
                PhaFormatString(L"%s (%.2f%%)", PhaFormatSize((ntfsVolumeInfo.VolumeData.MftZoneEnd.QuadPart - ntfsVolumeInfo.VolumeData.MftZoneStart.QuadPart) * ntfsVolumeInfo.VolumeData.BytesPerCluster, ULONG_MAX)->Buffer, (FLOAT)(ntfsVolumeInfo.VolumeData.MftZoneEnd.QuadPart - ntfsVolumeInfo.VolumeData.MftZoneStart.QuadPart) * 100.f / ntfsVolumeInfo.VolumeData.TotalClusters.QuadPart)->Buffer);
            PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_MFT_MIRROR_START, Column,
                PhaFormatString(L"%I64u", ntfsVolumeInfo.VolumeData.Mft2StartLcn.QuadPart)->Buffer);
        }
    }
    else if (Type == FILESYSTEM_STATISTICS_TYPE_FAT)
    {
        PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_FILE_SYSTEM, Column, L"FAT");
    }
    else if (Type == FILESYSTEM_STATISTICS_TYPE_REFS)
    {
        REFS_VOLUME_DATA_BUFFER refsVolumeInfo;

        PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_FILE_SYSTEM, Column, L"ReFS");

        if (DiskDriveQueryRefsVolumeInfo(DeviceHandle, &refsVolumeInfo))
        {
            refsVolumeInfo.VolumeSerialNumber.QuadPart = _byteswap_uint64(refsVolumeInfo.VolumeSerialNumber.QuadPart);

            PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_SERIAL_NUMBER, Column,
                PhaFormatString(L"0x%s", PH_AUTO_T(PH_STRING, PhBufferToHexString((PUCHAR)&refsVolumeInfo.VolumeSerialNumber.QuadPart, sizeof(LONGLONG)))->Buffer)->Buffer);
            PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_FS_VERSION, Column,
                PhaFormatString(L"%lu.%lu", refsVolumeInfo.MajorVersion, refsVolumeInfo.MinorVersion)->Buffer);
            PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_TOTAL_SIZE, Column,
                PhaFormatSize(refsVolumeInfo.NumberSectors.QuadPart * refsVolumeInfo.BytesPerSector, ULONG_MAX)->Buffer);
            PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_TOTAL_FREE, Column,
                PhaFormatString(L"%s (%.2f%%)", PhaFormatSize(refsVolumeInfo.FreeClusters.QuadPart * refsVolumeInfo.BytesPerCluster, ULONG_MAX)->Buffer, (FLOAT)(refsVolumeInfo.FreeClusters.QuadPart * 100) / refsVolumeInfo.TotalClusters.QuadPart)->Buffer);
            PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_TOTAL_SECTORS, Column,
                PhaFormatUInt64(refsVolumeInfo.NumberSectors.QuadPart, TRUE)->Buffer);
            PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_TOTAL_CLUSTERS, Column,
                PhaFormatUInt64(refsVolumeInfo.TotalClusters.QuadPart, TRUE)->Buffer);
            PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_FREE_CLUSTERS, Column,
                PhaFormatUInt64(refsVolumeInfo.FreeClusters.QuadPart, TRUE)->Buffer);
            PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_TOTAL_RESERVED, Column,
                PhaFormatUInt64(refsVolumeInfo.TotalReserved.QuadPart, TRUE)->Buffer);
            PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_TOTAL_BYTES_PER_SECTOR, Column,
                PhaFormatUInt64(refsVolumeInfo.BytesPerSector, TRUE)->Buffer);
            PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_TOTAL_BYTES_PER_CLUSTER, Column,
                PhaFormatUInt64(refsVolumeInfo.BytesPerCluster, TRUE)->Buffer);
        }
    }
}

VOID DiskDeviceQueryFileSystem(
    _Inout_ PDV_DISK_PAGE_CONTEXT Context,
    _In_ BOOLEAN AddListViewColumns
    )
{
    PPH_LIST deviceMountHandles;

    if (!(deviceMountHandles = DiskDriveQueryMountPointHandles(Context->PageContext->DiskIndex)))
        return;

    for (ULONG i = 0; i < deviceMountHandles->Count; i++)
    {
        INT column = i + 1;
        USHORT fsInfoType;
        PVOID fsInfoBuffer;
        PFILE_FS_VOLUME_INFORMATION volumeInfo;
        PDISK_HANDLE_ENTRY entry;
        PPH_STRING uniqueId;
        PPH_STRING partitionId;

        entry = deviceMountHandles->Items[i];

        if (NT_SUCCESS(DiskDriveQueryVolumeInformation(entry->DeviceHandle, &volumeInfo)))
        {
            SYSTEMTIME systemTime;

            PhLargeIntegerToLocalSystemTime(&systemTime, &volumeInfo->VolumeCreationTime);

            if (AddListViewColumns)
            {
                if (volumeInfo->VolumeLabelLength > 0)
                {
                    PhAddListViewColumn(Context->ListViewHandle, column, column, column, LVCFMT_LEFT, 200, PhaFormatString(
                        L"卷 %wc: [%s]", entry->DeviceLetter,
                        PhaCreateStringEx(volumeInfo->VolumeLabel, volumeInfo->VolumeLabelLength)->Buffer
                        )->Buffer);
                }
                else
                {
                    PhAddListViewColumn(Context->ListViewHandle, column, column, column, LVCFMT_LEFT, 200, PhaFormatString(
                        L"卷 %wc:", entry->DeviceLetter)->Buffer);
                }
            }

            PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_FS_CREATION_TIME, column, PhaFormatDateTime(&systemTime)->Buffer);

            PhFree(volumeInfo);
        }
        else
        {
            if (AddListViewColumns)
            {
                PhAddListViewColumn(Context->ListViewHandle, column, column, column, LVCFMT_LEFT, 200, PhaFormatString(
                    L"卷 %wc:", entry->DeviceLetter)->Buffer);
            }
        }

        if (NT_SUCCESS(DiskDriveQueryUniqueId(entry->DeviceHandle, &uniqueId, &partitionId)))
        {
            PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_UNIQUEID, column, PhGetString(uniqueId));
            PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_PARTITIONID, column, PhGetString(partitionId));

            PhClearReference(&uniqueId);
            PhClearReference(&partitionId);
        }

        if (DiskDriveQueryFileSystemInfoEx(entry->DeviceHandle, &fsInfoType, &fsInfoBuffer))
        {
            DiskDeviceQueryVolumeInfo(Context, fsInfoType, column, entry->DeviceHandle);

            switch (fsInfoType)
            {
            case FILESYSTEM_STATISTICS_TYPE_NTFS:
            case FILESYSTEM_STATISTICS_TYPE_FAT:
            case FILESYSTEM_STATISTICS_TYPE_REFS:
                {
                    PNTFS_FILESYSTEM_STATISTICS_EX buffer = fsInfoBuffer;

                    // File System

                    PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_FILE_READS, column,
                        PhaFormatUInt64(buffer->FileSystemStatistics.UserFileReads, TRUE)->Buffer);
                    PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_FILE_WRITES, column,
                        PhaFormatUInt64(buffer->FileSystemStatistics.UserFileWrites, TRUE)->Buffer);
                    PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_DISK_READS, column,
                        PhaFormatUInt64(buffer->FileSystemStatistics.UserDiskReads, TRUE)->Buffer);
                    PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_DISK_WRITES, column,
                        PhaFormatUInt64(buffer->FileSystemStatistics.UserDiskWrites, TRUE)->Buffer);
                    PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_FILE_READ_BYTES, column,
                        PhaFormatSize(buffer->FileSystemStatistics.UserFileReadBytes, ULONG_MAX)->Buffer);
                    PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_FILE_WRITE_BYTES, column,
                        PhaFormatSize(buffer->FileSystemStatistics.UserFileWriteBytes, ULONG_MAX)->Buffer);
                    PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_METADATA_READS, column,
                        PhaFormatUInt64(buffer->FileSystemStatistics.MetaDataReads, TRUE)->Buffer);
                    PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_METADATA_WRITES, column,
                        PhaFormatUInt64(buffer->FileSystemStatistics.MetaDataWrites, TRUE)->Buffer);
                    PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_METADATA_DISK_READS, column,
                        PhaFormatUInt64(buffer->FileSystemStatistics.MetaDataDiskReads, TRUE)->Buffer);
                    PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_METADATA_DISK_WRITES, column,
                        PhaFormatUInt64(buffer->FileSystemStatistics.MetaDataDiskWrites, TRUE)->Buffer);
                    PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_METADATA_READ_BYTES, column,
                        PhaFormatSize(buffer->FileSystemStatistics.MetaDataReadBytes, ULONG_MAX)->Buffer);
                    PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_METADATA_WRITE_BYTES, column,
                        PhaFormatSize(buffer->FileSystemStatistics.MetaDataWriteBytes, ULONG_MAX)->Buffer);

                    // NTFS specific

                    PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_MFT_READS, column,
                        PhaFormatUInt64(buffer->NtfsStatistics.MftReads, TRUE)->Buffer);
                    PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_MFT_WRITES, column,
                        PhaFormatUInt64(buffer->NtfsStatistics.MftWrites, TRUE)->Buffer);
                    PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_MFT_READ_BYTES, column,
                        PhaFormatSize(buffer->NtfsStatistics.MftReadBytes, ULONG_MAX)->Buffer);
                    PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_MFT_WRITE_BYTES, column,
                        PhaFormatSize(buffer->NtfsStatistics.MftWriteBytes, ULONG_MAX)->Buffer);

                    PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_OTHER_EXCEPTIONS, column,
                        PhaFormatUInt64(UInt32Add32To64(buffer->NtfsStatistics.LogFileFullExceptions, buffer->NtfsStatistics.OtherExceptions), TRUE)->Buffer);

                    if (PhWindowsVersion >= WINDOWS_8_1)
                    {
                        PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_RESOURCES_EXHAUSTED, column,
                            PhaFormatUInt64(buffer->NtfsStatistics.DiskResourcesExhausted, TRUE)->Buffer);
                    }

                    if (PhWindowsVersion >= WINDOWS_10)
                    {
                        LARGE_INTEGER frequency;
                        FLOAT volumeTrimTime;
                        FLOAT fileTrimTime;

                        PhQueryPerformanceFrequency(&frequency);
                        volumeTrimTime = (FLOAT)buffer->NtfsStatistics.VolumeTrimTime / (FLOAT)frequency.QuadPart;
                        fileTrimTime = (FLOAT)buffer->NtfsStatistics.FileLevelTrimTime / (FLOAT)frequency.QuadPart;

                        PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_VOLUME_TRIM_COUNT, column,
                            PhaFormatUInt64(buffer->NtfsStatistics.VolumeTrimCount, TRUE)->Buffer);
                        PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_VOLUME_TRIM_TIME, column,
                            PhaFormatString(L"%.2f 秒", volumeTrimTime)->Buffer);
                        PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_VOLUME_TRIM_BYTES, column,
                            PhaFormatSize(buffer->NtfsStatistics.VolumeTrimByteCount, ULONG_MAX)->Buffer);
                        PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_VOLUME_TRIM_SKIPPED_COUNT, column,
                            PhaFormatUInt64(buffer->NtfsStatistics.VolumeTrimSkippedCount, TRUE)->Buffer);
                        PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_VOLUME_TRIM_SKIPPED_BYTES, column,
                            PhaFormatSize(buffer->NtfsStatistics.VolumeTrimSkippedByteCount, ULONG_MAX)->Buffer);

                        PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_FILE_TRIM_COUNT, column,
                            PhaFormatUInt64(buffer->NtfsStatistics.FileLevelTrimCount, TRUE)->Buffer);
                        PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_FILE_TRIM_TIME, column,
                            PhaFormatString(L"%.2f 秒", fileTrimTime)->Buffer);
                        PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_FILE_TRIM_BYTES, column,
                            PhaFormatSize(buffer->NtfsStatistics.FileLevelTrimByteCount, ULONG_MAX)->Buffer);
                    }
                }
                break;
            case FILESYSTEM_STATISTICS_TYPE_EXFAT:
                {
                    //PEXFAT_FILESYSTEM_STATISTICS buffer = fsInfoBuffer;
                }
                break;
            }

            PhFree(fsInfoBuffer);
        }
        else
        {
            // Windows 7/8 statistics...

            if (DiskDriveQueryFileSystemInfo(entry->DeviceHandle, &fsInfoType, &fsInfoBuffer))
            {
                DiskDeviceQueryVolumeInfo(Context, fsInfoType, column, entry->DeviceHandle);

                switch (fsInfoType)
                {
                case FILESYSTEM_STATISTICS_TYPE_NTFS:
                case FILESYSTEM_STATISTICS_TYPE_FAT:
                case FILESYSTEM_STATISTICS_TYPE_REFS:
                    {
                        PNTFS_FILESYSTEM_STATISTICS buffer = fsInfoBuffer;

                        // File System

                        PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_FILE_READS, column,
                            PhaFormatUInt64(buffer->FileSystemStatistics.UserFileReads, TRUE)->Buffer);
                        PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_FILE_WRITES, column,
                            PhaFormatUInt64(buffer->FileSystemStatistics.UserFileWrites, TRUE)->Buffer);
                        PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_DISK_READS, column,
                            PhaFormatUInt64(buffer->FileSystemStatistics.UserDiskReads, TRUE)->Buffer);
                        PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_DISK_WRITES, column,
                            PhaFormatUInt64(buffer->FileSystemStatistics.UserDiskWrites, TRUE)->Buffer);
                        PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_FILE_READ_BYTES, column,
                            PhaFormatSize(buffer->FileSystemStatistics.UserFileReadBytes, ULONG_MAX)->Buffer);
                        PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_FILE_WRITE_BYTES, column,
                            PhaFormatSize(buffer->FileSystemStatistics.UserFileWriteBytes, ULONG_MAX)->Buffer);
                        PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_METADATA_READS, column,
                            PhaFormatUInt64(buffer->FileSystemStatistics.MetaDataReads, TRUE)->Buffer);
                        PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_METADATA_WRITES, column,
                            PhaFormatUInt64(buffer->FileSystemStatistics.MetaDataWrites, TRUE)->Buffer);
                        PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_METADATA_DISK_READS, column,
                            PhaFormatUInt64(buffer->FileSystemStatistics.MetaDataDiskReads, TRUE)->Buffer);
                        PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_METADATA_DISK_WRITES, column,
                            PhaFormatUInt64(buffer->FileSystemStatistics.MetaDataDiskWrites, TRUE)->Buffer);
                        PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_METADATA_READ_BYTES, column,
                            PhaFormatSize(buffer->FileSystemStatistics.MetaDataReadBytes, ULONG_MAX)->Buffer);
                        PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_METADATA_WRITE_BYTES, column,
                            PhaFormatSize(buffer->FileSystemStatistics.MetaDataWriteBytes, ULONG_MAX)->Buffer);

                        // NTFS specific

                        PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_MFT_READS, column,
                            PhaFormatUInt64(buffer->NtfsStatistics.MftReads, TRUE)->Buffer);
                        PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_MFT_WRITES, column,
                            PhaFormatUInt64(buffer->NtfsStatistics.MftWrites, TRUE)->Buffer);
                        PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_MFT_READ_BYTES, column,
                            PhaFormatSize(buffer->NtfsStatistics.MftReadBytes, ULONG_MAX)->Buffer);
                        PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_MFT_WRITE_BYTES, column,
                            PhaFormatSize(buffer->NtfsStatistics.MftWriteBytes, ULONG_MAX)->Buffer);

                        PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_OTHER_EXCEPTIONS, column,
                            PhaFormatUInt64(UInt32Add32To64(buffer->NtfsStatistics.LogFileFullExceptions, buffer->NtfsStatistics.OtherExceptions), TRUE)->Buffer);
                    }
                    break;
                case FILESYSTEM_STATISTICS_TYPE_EXFAT:
                    {
                        //PEXFAT_FILESYSTEM_STATISTICS buffer = fsInfoBuffer;
                    }
                    break;
                }

                PhFree(fsInfoBuffer);
            }
        }
    }

    DiskDriveCloseMountPointHandles(deviceMountHandles);
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI DiskDeviceProcessesUpdatedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PDV_DISK_PAGE_CONTEXT context = Context;

    if (context && context->WindowHandle)
    {
        PostMessage(context->WindowHandle, WM_PH_UPDATE_DIALOG, 0, 0);
    }
}

INT_PTR CALLBACK DiskDeviceFileSystemDetailsDlgProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PDV_DISK_PAGE_CONTEXT context = NULL;
    LPPROPSHEETPAGE propSheetPage;
    PPV_PROPPAGECONTEXT propPageContext;

    if (!PvPropPageDlgProcHeader(WindowHandle, WindowMessage, lParam, &propSheetPage, &propPageContext))
        return FALSE;

    if (WindowMessage == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(DV_DISK_PAGE_CONTEXT));
        context->PageContext = (PCOMMON_PAGE_CONTEXT)propPageContext->Context;

        PhSetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!context)
        return FALSE;

    switch (WindowMessage)
    {
    case WM_INITDIALOG:
        {
            context->WindowHandle = context->PageContext->SysInfoContext->DetailsWindowDialogHandle = WindowHandle;
            context->ListViewHandle = GetDlgItem(WindowHandle, IDC_DETAILS_LIST);

            PhSetListViewStyle(context->ListViewHandle, FALSE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 290, L"属性");
            PhSetExtendedListView(context->ListViewHandle);

            DiskDeviceAddListViewItems(context);
            DiskDeviceQueryFileSystem(context, TRUE);

            // Note: Load settings after querying devices. (dmex)
            PhLoadListViewColumnsFromSetting(SETTING_NAME_DISK_COUNTERS_COLUMNS, context->ListViewHandle);

            if (!!PhGetIntegerSetting(SETTING_ENABLE_THEME_SUPPORT)) // TODO: Required for compat (dmex)
                PhInitializeWindowTheme(GetParent(WindowHandle), !!PhGetIntegerSetting(SETTING_ENABLE_THEME_SUPPORT));
            else
                PhInitializeWindowTheme(WindowHandle, FALSE);

            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackProcessProviderUpdatedEvent),
                DiskDeviceProcessesUpdatedHandler,
                context,
                &context->ProcessesUpdatedRegistration
                );
        }
        break;
    case WM_DESTROY:
        {
            PhUnregisterCallback(PhGetGeneralCallback(GeneralCallbackProcessProviderUpdatedEvent), &context->ProcessesUpdatedRegistration);

            PhSaveListViewColumnsToSetting(SETTING_NAME_DISK_COUNTERS_COLUMNS, context->ListViewHandle);
        }
        break;
    case WM_NCDESTROY:
        {
            PhRemoveWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);
            PhFree(context);
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (!propPageContext->LayoutInitialized)
            {
                PPH_LAYOUT_ITEM dialogItem;

                dialogItem = PvAddPropPageLayoutItemEx(WindowHandle, WindowHandle, PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL, FALSE,
                    SETTING_NAME_DISK_POSITION, SETTING_NAME_DISK_SIZE);
                PvAddPropPageLayoutItem(WindowHandle, context->ListViewHandle, dialogItem, PH_ANCHOR_ALL);
                PvDoPropPageLayout(WindowHandle);

                propPageContext->LayoutInitialized = TRUE;
            }
        }
        break;
    case WM_PH_SHOW_DIALOG:
        {
            if (IsMinimized(WindowHandle))
                ShowWindow(WindowHandle, SW_RESTORE);
            else
                ShowWindow(WindowHandle, SW_SHOW);

            SetForegroundWindow(WindowHandle);
        }
        break;
    case WM_PH_UPDATE_DIALOG:
        {
            DiskDeviceQueryFileSystem(context, FALSE);

            ListView_RedrawItems(context->ListViewHandle, 0, INT_MAX);
            UpdateWindow(context->ListViewHandle);
        }
        break;
    case WM_NOTIFY:
        {
            PhHandleListViewNotifyBehaviors(lParam, context->ListViewHandle, PH_LIST_VIEW_DEFAULT_1_BEHAVIORS);
        }
        break;
    case WM_CONTEXTMENU:
        {
            if ((HWND)wParam == context->ListViewHandle)
            {
                POINT point;
                PPH_EMENU menu;
                PPH_EMENU item;
                PVOID *listviewItems;
                ULONG numberOfItems;

                point.x = GET_X_LPARAM(lParam);
                point.y = GET_Y_LPARAM(lParam);

                if (point.x == -1 && point.y == -1)
                    PhGetListViewContextMenuPoint(context->ListViewHandle, &point);

                if (PhGetSelectedListViewItemParams(context->ListViewHandle, &listviewItems, &numberOfItems))
                {
                    menu = PhCreateEMenu();
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PHAPP_IDC_COPY, L"复制(&C)", NULL, NULL), ULONG_MAX);
                    PhInsertCopyListViewEMenuItem(menu, PHAPP_IDC_COPY, context->ListViewHandle);

                    item = PhShowEMenu(
                        menu,
                        WindowHandle,
                        PH_EMENU_SHOW_SEND_COMMAND | PH_EMENU_SHOW_LEFTRIGHT,
                        PH_ALIGN_LEFT | PH_ALIGN_TOP,
                        point.x,
                        point.y
                        );

                    if (item)
                    {
                        if (!PhHandleCopyListViewEMenuItem(item))
                        {
                            switch (item->Id)
                            {
                            case PHAPP_IDC_COPY:
                                {
                                    PhCopyListView(context->ListViewHandle);
                                }
                                break;
                            }
                        }
                    }

                    PhDestroyEMenu(menu);
                }
                    PhFree(listviewItems);
                }
        }
        break;
    }

    return FALSE;
}

INT_PTR CALLBACK DiskDeviceSmartDetailsDlgProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PDV_DISK_PAGE_CONTEXT context = NULL;
    LPPROPSHEETPAGE propSheetPage;
    PPV_PROPPAGECONTEXT propPageContext;

    if (!PvPropPageDlgProcHeader(WindowHandle, WindowMessage, lParam, &propSheetPage, &propPageContext))
        return FALSE;

    if (WindowMessage == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(DV_DISK_PAGE_CONTEXT));
        context->PageContext = (PCOMMON_PAGE_CONTEXT)propPageContext->Context;

        PhSetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!context)
        return FALSE;

    switch (WindowMessage)
    {
    case WM_INITDIALOG:
        {
            context->WindowHandle = context->PageContext->SysInfoContext->DetailsWindowDialogHandle = WindowHandle;
            context->ListViewHandle = GetDlgItem(WindowHandle, IDC_DETAILS_LIST);

            PhSetListViewStyle(context->ListViewHandle, FALSE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 240, L"属性");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 50, L"值");
            PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 50, L"最佳");
            PhAddListViewColumn(context->ListViewHandle, 3, 3, 3, LVCFMT_LEFT, 80, L"原始值");
            PhAddListViewColumn(context->ListViewHandle, 4, 4, 4, LVCFMT_LEFT, 80, L"原始值 (十六进制)");
            //PhAddListViewColumn(context->ListViewHandle, 5, 5, 5, LVCFMT_LEFT, 80, L"Advisory");
            //PhAddListViewColumn(context->ListViewHandle, 6, 6, 6, LVCFMT_LEFT, 80, L"Failure Imminent");
            PhSetExtendedListView(context->ListViewHandle);
            //ExtendedListView_SetItemColorFunction(context->ListViewHandle, PhpColorItemColorFunction);
            PhLoadListViewColumnsFromSetting(SETTING_NAME_SMART_COUNTERS_COLUMNS, context->ListViewHandle);

            DiskDeviceQuerySmart(context);

            PhInitializeWindowTheme(WindowHandle, !!PhGetIntegerSetting(SETTING_ENABLE_THEME_SUPPORT));
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(SETTING_NAME_SMART_COUNTERS_COLUMNS, context->ListViewHandle);
        }
        break;
    case WM_NCDESTROY:
        {
            PhRemoveWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);
            PhFree(context);
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (!propPageContext->LayoutInitialized)
            {
                PPH_LAYOUT_ITEM dialogItem;

                dialogItem = PvAddPropPageLayoutItem(WindowHandle, WindowHandle, PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);
                PvAddPropPageLayoutItem(WindowHandle, context->ListViewHandle, dialogItem, PH_ANCHOR_ALL);
                PvDoPropPageLayout(WindowHandle);

                propPageContext->LayoutInitialized = TRUE;
            }
        }
        break;
    case WM_PH_SHOW_DIALOG:
        {
            if (IsMinimized(WindowHandle))
                ShowWindow(WindowHandle, SW_RESTORE);
            else
                ShowWindow(WindowHandle, SW_SHOW);

            SetForegroundWindow(WindowHandle);
        }
        break;
    case WM_NOTIFY:
        {
            PhHandleListViewNotifyBehaviors(lParam, context->ListViewHandle, PH_LIST_VIEW_DEFAULT_1_BEHAVIORS);
        }
        break;
    case WM_CONTEXTMENU:
        {
            if ((HWND)wParam == context->ListViewHandle)
            {
                POINT point;
                PPH_EMENU menu;
                PPH_EMENU item;
                PVOID *listviewItems;
                ULONG numberOfItems;

                point.x = GET_X_LPARAM(lParam);
                point.y = GET_Y_LPARAM(lParam);

                if (point.x == -1 && point.y == -1)
                    PhGetListViewContextMenuPoint(context->ListViewHandle, &point);

                if (PhGetSelectedListViewItemParams(context->ListViewHandle, &listviewItems, &numberOfItems))
                {
                    menu = PhCreateEMenu();
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PHAPP_IDC_COPY, L"复制(&C)", NULL, NULL), ULONG_MAX);
                    PhInsertCopyListViewEMenuItem(menu, PHAPP_IDC_COPY, context->ListViewHandle);

                    item = PhShowEMenu(
                        menu,
                        WindowHandle,
                        PH_EMENU_SHOW_SEND_COMMAND | PH_EMENU_SHOW_LEFTRIGHT,
                        PH_ALIGN_LEFT | PH_ALIGN_TOP,
                        point.x,
                        point.y
                        );

                    if (item)
                    {
                        if (!PhHandleCopyListViewEMenuItem(item))
                        {
                            switch (item->Id)
                            {
                            case PHAPP_IDC_COPY:
                                {
                                    PhCopyListView(context->ListViewHandle);
                                }
                                break;
                            }
                        }
                    }

                    PhDestroyEMenu(menu);
                }
                    PhFree(listviewItems);
                }
        }
        break;
    }

    return FALSE;
}

VOID DiskDeviceFreeDetailsContext(
    _In_ PCOMMON_PAGE_CONTEXT Context
    )
{
    if (Context->DeviceHandle)
        NtClose(Context->DeviceHandle);

    DeleteDiskId(&Context->DiskId);
    PhClearReference(&Context->DiskName);
    PhFree(Context);
}

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS ShowDiskDeviceDetailsDialogThread(
    _In_ PVOID Parameter
    )
{
    PPV_PROPCONTEXT propContext;
    PCOMMON_PAGE_CONTEXT pageContext = Parameter;

    PhSetEvent(&pageContext->SysInfoContext->DetailsWindowInitializedEvent);

    if (propContext = HdCreatePropContext(L"磁盘设备"))
    {
        PPV_PROPPAGECONTEXT newPage;

        // FileSystem page
        newPage = PvCreatePropPageContext(
            MAKEINTRESOURCE(IDD_DISKDRIVE_DETAILS_FILESYSTEM),
            DiskDeviceFileSystemDetailsDlgProc,
            pageContext);
        PvAddPropPage(propContext, newPage);

        // Smart page
        newPage = PvCreatePropPageContext(
            MAKEINTRESOURCE(IDD_DISKDRIVE_DETAILS_SMART),
            DiskDeviceSmartDetailsDlgProc,
            pageContext);
        PvAddPropPage(propContext, newPage);

        PhModalPropertySheet(&propContext->PropSheetHeader);
        PhDereferenceObject(propContext);
    }

    PhResetEvent(&pageContext->SysInfoContext->DetailsWindowInitializedEvent);

    if (pageContext->SysInfoContext->DetailsWindowThreadHandle)
    {
        NtClose(pageContext->SysInfoContext->DetailsWindowThreadHandle);
        pageContext->SysInfoContext->DetailsWindowThreadHandle = NULL;
    }

    return STATUS_SUCCESS;
}

VOID ShowDiskDeviceDetailsDialog(
    _In_ PDV_DISK_SYSINFO_CONTEXT Context
    )
{
    if (!Context->DetailsWindowThreadHandle)
    {
        HANDLE threadHandle;
        PCOMMON_PAGE_CONTEXT pageContext;

        pageContext = PhAllocateZero(sizeof(COMMON_PAGE_CONTEXT));
        //pageContext->ParentHandle = GetParent(GetParent(Context->WindowHandle));
        pageContext->SysInfoContext = Context;
        pageContext->DiskIndex = Context->DiskEntry->DiskIndex;
        //pageContext->Length = Context->DiskEntry->DiskLength;

        PhSetReference(&pageContext->DiskName, Context->DiskEntry->DiskName);
        CopyDiskId(&pageContext->DiskId, &Context->DiskEntry->Id);

        if (!NT_SUCCESS(PhCreateThreadEx(&threadHandle, ShowDiskDeviceDetailsDialogThread, pageContext)))
        {
            PhShowError2(Context->WindowHandle, L"无法创建窗口。", L"%s", L"");
            return;
        }

        PhWaitForEvent(&Context->DetailsWindowInitializedEvent, NULL);

        Context->DetailsWindowThreadHandle = threadHandle;
    }

    PostMessage(Context->DetailsWindowDialogHandle, WM_PH_SHOW_DIALOG, 0, 0);
}
