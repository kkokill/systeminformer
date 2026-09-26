/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     dmex    2017-2026
 *
 */

#include <ph.h>
#include <secedit.h>
#include <wbemcli.h>
#include <wtsapi32.h>

#define ACCESS_ENTRIES(Type) static const PH_ACCESS_ENTRY Ph##Type##AccessEntries[] =
#define ACCESS_ENTRY(Type, HasSynchronize) \
   { TEXT(#Type), (PPH_ACCESS_ENTRY)Ph##Type##AccessEntries, sizeof(Ph##Type##AccessEntries), HasSynchronize }

typedef struct _PH_SPECIFIC_TYPE
{
    PCWSTR Type;
    PPH_ACCESS_ENTRY AccessEntries;
    ULONG SizeOfAccessEntries;
    BOOLEAN HasSynchronize;
} PH_SPECIFIC_TYPE, *PPH_SPECIFIC_TYPE;

ACCESS_ENTRIES(Standard)
{
    { L"同步", SYNCHRONIZE, FALSE, TRUE, NULL },
    { L"删除", DELETE, FALSE, TRUE, NULL },
    { L"读取权限", READ_CONTROL, FALSE, TRUE, L"读取控制" },
    { L"更改权限", WRITE_DAC, FALSE, TRUE, L"写入 DAC" },
    { L"取得所有权", WRITE_OWNER, FALSE, TRUE, L"写入所有者" }
};

ACCESS_ENTRIES(AlpcPort)
{
    { L"完全控制", PORT_ALL_ACCESS, TRUE, TRUE, NULL },
    { L"连接", PORT_CONNECT, TRUE, TRUE, NULL }
};

ACCESS_ENTRIES(DebugObject)
{
    { L"完全控制", DEBUG_ALL_ACCESS, TRUE, TRUE, NULL },
    { L"读取事件", DEBUG_READ_EVENT, TRUE, TRUE, NULL },
    { L"指派进程", DEBUG_PROCESS_ASSIGN, TRUE, TRUE, NULL },
    { L"查询信息", DEBUG_QUERY_INFORMATION, TRUE, TRUE, NULL },
    { L"设置信息", DEBUG_SET_INFORMATION, TRUE, TRUE, NULL }
};

ACCESS_ENTRIES(Desktop)
{
    { L"完全控制", DESKTOP_ALL_ACCESS, TRUE, TRUE, NULL },
    { L"读取", DESKTOP_GENERIC_READ, TRUE, FALSE, NULL },
    { L"写入", DESKTOP_GENERIC_WRITE, TRUE, FALSE, NULL },
    { L"执行", DESKTOP_GENERIC_EXECUTE, TRUE, FALSE, NULL },
    { L"枚举", DESKTOP_ENUMERATE, FALSE, TRUE, NULL },
    { L"读取对象", DESKTOP_READOBJECTS, FALSE, TRUE, NULL },
    { L"播放日志", DESKTOP_JOURNALPLAYBACK, FALSE, TRUE, NULL },
    { L"写入对象", DESKTOP_WRITEOBJECTS, FALSE, TRUE, NULL },
    { L"创建窗口", DESKTOP_CREATEWINDOW, FALSE, TRUE, NULL },
    { L"创建菜单", DESKTOP_CREATEMENU, FALSE, TRUE, NULL },
    { L"创建窗口挂钩", DESKTOP_HOOKCONTROL, FALSE, TRUE, NULL },
    { L"记录日志", DESKTOP_JOURNALRECORD, FALSE, TRUE, NULL },
    { L"切换桌面", DESKTOP_SWITCHDESKTOP, FALSE, TRUE, NULL }
};

ACCESS_ENTRIES(Directory)
{
    { L"完全控制", DIRECTORY_ALL_ACCESS, TRUE, TRUE, NULL },
    { L"查询", DIRECTORY_QUERY, TRUE, TRUE, NULL },
    { L"遍历", DIRECTORY_TRAVERSE, TRUE, TRUE, NULL },
    { L"创建对象", DIRECTORY_CREATE_OBJECT, TRUE, TRUE, NULL },
    { L"创建子目录", DIRECTORY_CREATE_SUBDIRECTORY, TRUE, TRUE, NULL }
};

ACCESS_ENTRIES(EtwConsumer)
{
    { L"完全控制", WMIGUID_ALL_ACCESS, TRUE, TRUE, NULL },
    { L"查询", WMIGUID_QUERY, TRUE, TRUE, NULL },
    { L"读取", WMIGUID_SET, TRUE, TRUE, NULL },
    { L"通知", WMIGUID_NOTIFICATION, TRUE, TRUE, NULL },
    { L"读取描述", WMIGUID_READ_DESCRIPTION, TRUE, TRUE, NULL },
    { L"执行", WMIGUID_EXECUTE, TRUE, TRUE, NULL },
    { L"创建实时会话", TRACELOG_CREATE_REALTIME, TRUE, TRUE, NULL },
    { L"创建日志文件", TRACELOG_CREATE_ONDISK, TRUE, TRUE, NULL },
    { L"启用 GUID", TRACELOG_GUID_ENABLE, TRUE, TRUE, NULL },
    { L"访问内核记录器", TRACELOG_ACCESS_KERNEL_LOGGER, TRUE, TRUE, NULL },
    { L"记录事件", TRACELOG_LOG_EVENT, TRUE, TRUE, NULL },
    { L"访问实时会话", TRACELOG_ACCESS_REALTIME, TRUE, TRUE, NULL },
    { L"注册 GUID", TRACELOG_REGISTER_GUIDS, TRUE, TRUE, NULL },
    { L"加入组", TRACELOG_JOIN_GROUP, TRUE, TRUE, NULL }
};

ACCESS_ENTRIES(EtwRegistration)
{
    { L"完全控制", WMIGUID_ALL_ACCESS, TRUE, TRUE, NULL },
    { L"查询", WMIGUID_QUERY, TRUE, TRUE, NULL },
    { L"读取", WMIGUID_SET, TRUE, TRUE, NULL },
    { L"通知", WMIGUID_NOTIFICATION, TRUE, TRUE, NULL },
    { L"读取描述", WMIGUID_READ_DESCRIPTION, TRUE, TRUE, NULL },
    { L"执行", WMIGUID_EXECUTE, TRUE, TRUE, NULL },
    { L"创建实时会话", TRACELOG_CREATE_REALTIME, TRUE, TRUE, NULL },
    { L"创建日志文件", TRACELOG_CREATE_ONDISK, TRUE, TRUE, NULL },
    { L"启用 GUID", TRACELOG_GUID_ENABLE, TRUE, TRUE, NULL },
    { L"访问内核记录器", TRACELOG_ACCESS_KERNEL_LOGGER, TRUE, TRUE, NULL },
    { L"记录事件", TRACELOG_LOG_EVENT, TRUE, TRUE, NULL },
    { L"访问实时会话", TRACELOG_ACCESS_REALTIME, TRUE, TRUE, NULL },
    { L"注册 GUID", TRACELOG_REGISTER_GUIDS, TRUE, TRUE, NULL },
    { L"加入组", TRACELOG_JOIN_GROUP, TRUE, TRUE, NULL }
};

ACCESS_ENTRIES(Event)
{
    { L"完全控制", EVENT_ALL_ACCESS, TRUE, TRUE, NULL },
    { L"查询", EVENT_QUERY_STATE, TRUE, TRUE, NULL },
    { L"修改", EVENT_MODIFY_STATE, TRUE, TRUE, NULL }
};

ACCESS_ENTRIES(EventPair)
{
    { L"完全控制", EVENT_PAIR_ALL_ACCESS, TRUE, TRUE, NULL }
};

ACCESS_ENTRIES(File)
{
    { L"完全控制", FILE_ALL_ACCESS, TRUE, TRUE, NULL },
    { L"读取和执行", FILE_GENERIC_READ | FILE_GENERIC_EXECUTE, TRUE, FALSE, NULL },
    { L"读取", FILE_GENERIC_READ, TRUE, FALSE, NULL },
    { L"写入", FILE_GENERIC_WRITE, TRUE, FALSE, NULL },
    { L"遍历文件夹/执行文件", FILE_EXECUTE, FALSE, TRUE, L"执行" },
    { L"列出文件夹/读取数据", FILE_READ_DATA, FALSE, TRUE, L"读取数据" },
    { L"读取属性", FILE_READ_ATTRIBUTES, FALSE, TRUE, NULL },
    { L"读取扩展属性", FILE_READ_EA, FALSE, TRUE, L"读取 EA" },
    { L"创建文件/写入数据", FILE_WRITE_DATA, FALSE, TRUE, L"写入数据" },
    { L"创建文件夹/追加数据", FILE_APPEND_DATA, FALSE, TRUE, L"追加数据" },
    { L"写入属性", FILE_WRITE_ATTRIBUTES, FALSE, TRUE, NULL },
    { L"写入扩展属性", FILE_WRITE_EA, FALSE, TRUE, L"写入 EA" },
    { L"删除子文件夹及文件", FILE_DELETE_CHILD, FALSE, TRUE, L"删除子项" }
};

ACCESS_ENTRIES(FilterConnectionPort)
{
    { L"完全控制", FLT_PORT_ALL_ACCESS, TRUE, TRUE, NULL },
    { L"连接", FLT_PORT_CONNECT, TRUE, TRUE, NULL }
};

ACCESS_ENTRIES(IoCompletion)
{
    { L"完全控制", IO_COMPLETION_ALL_ACCESS, TRUE, TRUE, NULL },
    { L"查询", IO_COMPLETION_QUERY_STATE, TRUE, TRUE, NULL },
    { L"修改", IO_COMPLETION_MODIFY_STATE, TRUE, TRUE, NULL }
};

ACCESS_ENTRIES(Job)
{
    { L"完全控制", JOB_OBJECT_ALL_ACCESS, TRUE, TRUE, NULL },
    { L"查询", JOB_OBJECT_QUERY, TRUE, TRUE, NULL },
    { L"指派进程", JOB_OBJECT_ASSIGN_PROCESS, TRUE, TRUE, NULL },
    { L"设置属性", JOB_OBJECT_SET_ATTRIBUTES, TRUE, TRUE, NULL },
    { L"设置安全属性", JOB_OBJECT_SET_SECURITY_ATTRIBUTES, TRUE, TRUE, NULL },
    { L"终止", JOB_OBJECT_TERMINATE, TRUE, TRUE, NULL }
};

ACCESS_ENTRIES(Key)
{
    { L"完全控制", KEY_ALL_ACCESS, TRUE, TRUE, NULL },
    { L"读取", KEY_READ, TRUE, FALSE, NULL },
    { L"写入", KEY_WRITE, TRUE, FALSE, NULL },
    //{ L"Execute", KEY_EXECUTE, TRUE, FALSE }, // KEY_EXECUTE has the same value as KEY_READ (dmex)
    { L"枚举子项", KEY_ENUMERATE_SUB_KEYS, FALSE, TRUE, NULL },
    { L"查询数值", KEY_QUERY_VALUE, FALSE, TRUE, NULL },
    { L"通知", KEY_NOTIFY, FALSE, TRUE, NULL },
    { L"设置数值", KEY_SET_VALUE, FALSE, TRUE, NULL },
    { L"创建子项", KEY_CREATE_SUB_KEY, FALSE, TRUE, NULL },
    { L"创建链接", KEY_CREATE_LINK, FALSE, TRUE, NULL }
};

ACCESS_ENTRIES(KeyedEvent)
{
    { L"完全控制", KEYEDEVENT_ALL_ACCESS, TRUE, TRUE, NULL },
    { L"等待", KEYEDEVENT_WAIT, TRUE, TRUE, NULL },
    { L"唤醒", KEYEDEVENT_WAKE, TRUE, TRUE, NULL }
};

ACCESS_ENTRIES(LsaAccount)
{
    { L"完全控制", ACCOUNT_ALL_ACCESS, TRUE, TRUE, NULL },
    { L"读取", ACCOUNT_READ, TRUE, FALSE, NULL },
    { L"写入", ACCOUNT_WRITE, TRUE, FALSE, NULL },
    { L"执行", ACCOUNT_EXECUTE, TRUE, FALSE, NULL },
    { L"查看", ACCOUNT_VIEW, FALSE, TRUE, NULL },
    { L"调整特权", ACCOUNT_ADJUST_PRIVILEGES, FALSE, TRUE, NULL },
    { L"调整配额", ACCOUNT_ADJUST_QUOTAS, FALSE, TRUE, NULL },
    { L"调整系统访问权限", ACCOUNT_ADJUST_SYSTEM_ACCESS, FALSE, TRUE, NULL }
};

ACCESS_ENTRIES(LsaPolicy)
{
    { L"完全控制", POLICY_ALL_ACCESS | POLICY_NOTIFICATION, TRUE, TRUE, NULL },
    { L"读取", POLICY_READ, TRUE, FALSE, NULL },
    { L"写入", POLICY_WRITE, TRUE, FALSE, NULL },
    { L"执行", POLICY_EXECUTE | POLICY_NOTIFICATION, TRUE, FALSE, NULL },
    { L"查看本地信息", POLICY_VIEW_LOCAL_INFORMATION, FALSE, TRUE, NULL },
    { L"查看审核信息", POLICY_VIEW_AUDIT_INFORMATION, FALSE, TRUE, NULL },
    { L"获取专用信息", POLICY_GET_PRIVATE_INFORMATION, FALSE, TRUE, NULL },
    { L"管理信任", POLICY_TRUST_ADMIN, FALSE, TRUE, NULL },
    { L"创建账户", POLICY_CREATE_ACCOUNT, FALSE, TRUE, NULL },
    { L"创建机密", POLICY_CREATE_SECRET, FALSE, TRUE, NULL },
    { L"创建特权", POLICY_CREATE_PRIVILEGE, FALSE, TRUE, NULL },
    { L"设置默认配额限制", POLICY_SET_DEFAULT_QUOTA_LIMITS, FALSE, TRUE, NULL },
    { L"设置审核要求", POLICY_SET_AUDIT_REQUIREMENTS, FALSE, TRUE, NULL },
    { L"管理审核日志", POLICY_AUDIT_LOG_ADMIN, FALSE, TRUE, NULL },
    { L"管理服务器", POLICY_SERVER_ADMIN, FALSE, TRUE, NULL },
    { L"查找名称", POLICY_LOOKUP_NAMES, FALSE, TRUE, NULL },
    { L"获取通知", POLICY_NOTIFICATION, FALSE, TRUE, NULL }
};

ACCESS_ENTRIES(LsaSecret)
{
    { L"完全控制", SECRET_ALL_ACCESS, TRUE, TRUE, NULL },
    { L"读取", SECRET_READ, TRUE, FALSE, NULL },
    { L"写入", SECRET_WRITE, TRUE, FALSE, NULL },
    { L"执行", SECRET_EXECUTE, TRUE, FALSE, NULL },
    { L"设置值", SECRET_SET_VALUE, FALSE, TRUE, NULL },
    { L"查询值", SECRET_QUERY_VALUE, FALSE, TRUE, NULL }
};

ACCESS_ENTRIES(LsaTrusted)
{
    { L"完全控制", TRUSTED_ALL_ACCESS, TRUE, TRUE, NULL },
    { L"读取", TRUSTED_READ, TRUE, FALSE, NULL },
    { L"写入", TRUSTED_WRITE, TRUE, FALSE, NULL },
    { L"执行", TRUSTED_EXECUTE, TRUE, FALSE, NULL },
    { L"查询域名", TRUSTED_QUERY_DOMAIN_NAME, FALSE, TRUE, NULL },
    { L"查询域控制器", TRUSTED_QUERY_CONTROLLERS, FALSE, TRUE, NULL },
    { L"设置域控制器", TRUSTED_SET_CONTROLLERS, FALSE, TRUE, NULL },
    { L"查询 POSIX", TRUSTED_QUERY_POSIX, FALSE, TRUE, NULL },
    { L"设置 POSIX", TRUSTED_SET_POSIX, FALSE, TRUE, NULL },
    { L"查询身份验证", TRUSTED_QUERY_AUTH, FALSE, TRUE, NULL },
    { L"设置身份验证", TRUSTED_SET_AUTH, FALSE, TRUE, NULL }
};

ACCESS_ENTRIES(Mutant)
{
    { L"完全控制", MUTANT_ALL_ACCESS, TRUE, TRUE, NULL },
    { L"查询", MUTANT_QUERY_STATE, TRUE, TRUE, NULL }
};

ACCESS_ENTRIES(Partition)
{
    { L"完全控制", MEMORY_PARTITION_ALL_ACCESS, TRUE, TRUE, NULL },
    { L"查询", MEMORY_PARTITION_QUERY_ACCESS, TRUE, TRUE, NULL },
    { L"修改", MEMORY_PARTITION_MODIFY_ACCESS, TRUE, TRUE, NULL }
};

ACCESS_ENTRIES(Process)
{
    { L"完全控制", STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0xfff, TRUE, TRUE, NULL },
    { L"查询信息", PROCESS_QUERY_INFORMATION, TRUE, TRUE, NULL },
    { L"设置信息", PROCESS_SET_INFORMATION, TRUE, TRUE, NULL },
    { L"设置配额", PROCESS_SET_QUOTA, TRUE, TRUE, NULL },
    { L"设置会话 ID", PROCESS_SET_SESSIONID, TRUE, TRUE, NULL },
    { L"创建线程", PROCESS_CREATE_THREAD, TRUE, TRUE, NULL },
    { L"创建进程", PROCESS_CREATE_PROCESS, TRUE, TRUE, NULL },
    { L"修改内存", PROCESS_VM_OPERATION, TRUE, TRUE, L"虚拟内存操作" },
    { L"读取内存", PROCESS_VM_READ, TRUE, TRUE, L"读取虚拟内存" },
    { L"写入内存", PROCESS_VM_WRITE, TRUE, TRUE, L"写入虚拟内存" },
    { L"复制句柄", PROCESS_DUP_HANDLE, TRUE, TRUE, NULL },
    { L"挂起/恢复/设置端口", PROCESS_SUSPEND_RESUME, TRUE, TRUE, L"挂起/恢复" },
    { L"终止", PROCESS_TERMINATE, TRUE, TRUE, NULL }
};

ACCESS_ENTRIES(Process60)
{
    { L"完全控制", STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | SPECIFIC_RIGHTS_ALL, TRUE, TRUE, NULL }, // PROCESS_ALL_ACCESS
    { L"查询有限信息", PROCESS_QUERY_LIMITED_INFORMATION, TRUE, TRUE, NULL },
    { L"查询信息", PROCESS_QUERY_INFORMATION | PROCESS_QUERY_LIMITED_INFORMATION, TRUE, TRUE, NULL },
    { L"设置信息", PROCESS_SET_INFORMATION, TRUE, TRUE, NULL },
    { L"设置有限信息", PROCESS_SET_LIMITED_INFORMATION, TRUE, TRUE, NULL },
    { L"设置配额", PROCESS_SET_QUOTA, TRUE, TRUE, NULL },
    { L"设置会话 ID", PROCESS_SET_SESSIONID, TRUE, TRUE, NULL },
    { L"创建线程", PROCESS_CREATE_THREAD, TRUE, TRUE, NULL },
    { L"创建进程", PROCESS_CREATE_PROCESS, TRUE, TRUE, NULL },
    { L"修改内存", PROCESS_VM_OPERATION, TRUE, TRUE, L"虚拟内存操作" },
    { L"读取内存", PROCESS_VM_READ, TRUE, TRUE, L"读取虚拟内存" },
    { L"写入内存", PROCESS_VM_WRITE, TRUE, TRUE, L"写入虚拟内存" },
    { L"复制句柄", PROCESS_DUP_HANDLE, TRUE, TRUE, NULL },
    { L"挂起/恢复/设置端口", PROCESS_SUSPEND_RESUME, TRUE, TRUE, L"挂起/恢复" },
    { L"终止", PROCESS_TERMINATE, TRUE, TRUE, NULL }
};

ACCESS_ENTRIES(Profile)
{
    { L"完全控制", PROFILE_ALL_ACCESS, TRUE, TRUE, NULL },
    { L"控制", PROFILE_CONTROL, TRUE, TRUE, NULL }
};

ACCESS_ENTRIES(SamAlias)
{
    { L"完全控制", ALIAS_ALL_ACCESS, TRUE, TRUE, NULL },
    { L"读取", ALIAS_READ, TRUE, FALSE, NULL },
    { L"写入", ALIAS_WRITE, TRUE, FALSE, NULL },
    { L"执行", ALIAS_EXECUTE, TRUE, FALSE },
    { L"读取信息", ALIAS_READ_INFORMATION, FALSE, TRUE, NULL },
    { L"写入账户", ALIAS_WRITE_ACCOUNT, FALSE, TRUE, NULL },
    { L"添加成员", ALIAS_ADD_MEMBER, FALSE, TRUE, NULL },
    { L"移除成员", ALIAS_REMOVE_MEMBER, FALSE, TRUE, NULL },
    { L"列出成员", ALIAS_LIST_MEMBERS, FALSE, TRUE, NULL }
};

ACCESS_ENTRIES(SamDomain)
{
    { L"完全控制", DOMAIN_ALL_ACCESS, TRUE, TRUE, NULL },
    { L"读取", DOMAIN_READ, TRUE, FALSE, NULL },
    { L"写入", DOMAIN_WRITE, TRUE, FALSE, NULL },
    { L"执行", DOMAIN_EXECUTE, TRUE, FALSE, NULL },
    { L"读取密码参数", DOMAIN_READ_PASSWORD_PARAMETERS, FALSE, TRUE, NULL },
    { L"写入密码参数", DOMAIN_WRITE_PASSWORD_PARAMS, FALSE, TRUE, NULL },
    { L"读取其他参数", DOMAIN_READ_OTHER_PARAMETERS, FALSE, TRUE, NULL },
    { L"写入其他参数", DOMAIN_WRITE_OTHER_PARAMETERS, FALSE, TRUE, NULL },
    { L"创建用户", DOMAIN_CREATE_USER, FALSE, TRUE, NULL },
    { L"创建组", DOMAIN_CREATE_GROUP, FALSE, TRUE, NULL },
    { L"创建别名", DOMAIN_CREATE_ALIAS, FALSE, TRUE, NULL },
    { L"获取别名成员身份", DOMAIN_GET_ALIAS_MEMBERSHIP, FALSE, TRUE, NULL },
    { L"列出账户", DOMAIN_LIST_ACCOUNTS, FALSE, TRUE, NULL },
    { L"查找", DOMAIN_LOOKUP, FALSE, TRUE, NULL },
    { L"管理服务器", DOMAIN_ADMINISTER_SERVER, FALSE, TRUE, NULL }
};

ACCESS_ENTRIES(SamGroup)
{
    { L"完全控制", GROUP_ALL_ACCESS, TRUE, TRUE, NULL },
    { L"读取", GROUP_READ, TRUE, FALSE, NULL },
    { L"写入", GROUP_WRITE, TRUE, FALSE, NULL },
    { L"执行", GROUP_EXECUTE, TRUE, FALSE, NULL },
    { L"读取信息", GROUP_READ_INFORMATION, FALSE, TRUE, NULL },
    { L"写入账户", GROUP_WRITE_ACCOUNT, FALSE, TRUE, NULL },
    { L"添加成员", GROUP_ADD_MEMBER, FALSE, TRUE, NULL },
    { L"移除成员", GROUP_REMOVE_MEMBER, FALSE, TRUE, NULL },
    { L"列出成员", GROUP_LIST_MEMBERS, FALSE, TRUE, NULL }
};

ACCESS_ENTRIES(SamServer)
{
    { L"完全控制", SAM_SERVER_ALL_ACCESS, TRUE, TRUE, NULL },
    { L"读取", SAM_SERVER_READ, TRUE, FALSE, NULL },
    { L"写入", SAM_SERVER_WRITE, TRUE, FALSE, NULL },
    { L"执行", SAM_SERVER_EXECUTE, TRUE, FALSE, NULL },
    { L"连接", SAM_SERVER_CONNECT, FALSE, TRUE, NULL },
    { L"关闭", SAM_SERVER_SHUTDOWN, FALSE, TRUE, NULL },
    { L"初始化", SAM_SERVER_INITIALIZE, FALSE, TRUE, NULL },
    { L"创建域", SAM_SERVER_CREATE_DOMAIN, FALSE, TRUE, NULL },
    { L"枚举域", SAM_SERVER_ENUMERATE_DOMAINS, FALSE, TRUE, NULL },
    { L"查找域", SAM_SERVER_LOOKUP_DOMAIN, FALSE, TRUE, NULL }
};

ACCESS_ENTRIES(SamUser)
{
    { L"完全控制", USER_ALL_ACCESS, TRUE, TRUE, NULL },
    { L"读取", USER_READ, TRUE, FALSE, NULL },
    { L"写入", USER_WRITE, TRUE, FALSE, NULL },
    { L"执行", USER_EXECUTE, TRUE, FALSE, NULL },
    { L"读取常规信息", USER_READ_GENERAL, FALSE, TRUE, NULL },
    { L"读取首选项", USER_READ_PREFERENCES, FALSE, TRUE, NULL },
    { L"写入首选项", USER_WRITE_PREFERENCES, FALSE, TRUE, NULL },
    { L"读取登录信息", USER_READ_LOGON, FALSE, TRUE, NULL },
    { L"读取账户", USER_READ_ACCOUNT, FALSE, TRUE, NULL },
    { L"写入账户", USER_WRITE_ACCOUNT, FALSE, TRUE, NULL },
    { L"更改密码", USER_CHANGE_PASSWORD, FALSE, TRUE, NULL },
    { L"强制更改密码", USER_FORCE_PASSWORD_CHANGE, FALSE, TRUE, NULL },
    { L"列出组", USER_LIST_GROUPS, FALSE, TRUE, NULL },
    { L"读取组信息", USER_READ_GROUP_INFORMATION, FALSE, TRUE, NULL },
    { L"写入组信息", USER_WRITE_GROUP_INFORMATION, FALSE, TRUE, NULL }
};

ACCESS_ENTRIES(Section)
{
    { L"完全控制", SECTION_ALL_ACCESS, TRUE, TRUE, NULL },
    { L"查询", SECTION_QUERY, TRUE, TRUE, NULL },
    { L"映射以读取", SECTION_MAP_READ, TRUE, TRUE, L"读取映射" },
    { L"映射以写入", SECTION_MAP_WRITE, TRUE, TRUE, L"写入映射" },
    { L"映射以执行", SECTION_MAP_EXECUTE, TRUE, TRUE, L"执行映射" },
    { L"映射以执行（显式）", SECTION_MAP_EXECUTE_EXPLICIT, TRUE, TRUE, L"执行映射（显式）" },
    { L"扩展大小", SECTION_EXTEND_SIZE, TRUE, TRUE, NULL }
};

ACCESS_ENTRIES(Semaphore)
{
    { L"完全控制", SEMAPHORE_ALL_ACCESS, TRUE, TRUE, NULL },
    { L"查询", SEMAPHORE_QUERY_STATE, TRUE, TRUE, NULL },
    { L"修改", SEMAPHORE_MODIFY_STATE, TRUE, TRUE, NULL }
};

ACCESS_ENTRIES(Service)
{
    { L"完全控制", SERVICE_ALL_ACCESS, TRUE, TRUE, NULL },
    { L"查询状态", SERVICE_QUERY_STATUS, TRUE, TRUE, NULL },
    { L"查询配置", SERVICE_QUERY_CONFIG, TRUE, TRUE, NULL },
    { L"修改配置", SERVICE_CHANGE_CONFIG, TRUE, TRUE, NULL },
    { L"枚举依赖项", SERVICE_ENUMERATE_DEPENDENTS, TRUE, TRUE, NULL },
    { L"启动", SERVICE_START, TRUE, TRUE, NULL },
    { L"停止", SERVICE_STOP, TRUE, TRUE, NULL },
    { L"暂停/继续", SERVICE_PAUSE_CONTINUE, TRUE, TRUE, L"暂停/继续" },
    { L"询问", SERVICE_INTERROGATE, TRUE, TRUE, NULL },
    { L"用户定义的控制", SERVICE_USER_DEFINED_CONTROL, TRUE, TRUE, NULL }
};

ACCESS_ENTRIES(SCManager)
{
    { L"完全控制", SC_MANAGER_ALL_ACCESS, TRUE, TRUE, NULL },
    { L"创建服务", SC_MANAGER_CREATE_SERVICE, TRUE, TRUE, NULL },
    { L"连接", SC_MANAGER_CONNECT, TRUE, TRUE, NULL },
    { L"枚举服务", SC_MANAGER_ENUMERATE_SERVICE, TRUE, TRUE, NULL },
    { L"锁定", SC_MANAGER_LOCK, TRUE, TRUE, NULL },
    { L"修改启动配置", SC_MANAGER_MODIFY_BOOT_CONFIG, TRUE, TRUE, NULL },
    { L"查询锁定状态", SC_MANAGER_QUERY_LOCK_STATUS, TRUE, TRUE, NULL }
};

ACCESS_ENTRIES(Session)
{
    { L"完全控制", SESSION_ALL_ACCESS, TRUE, TRUE, NULL },
    { L"查询", SESSION_QUERY_ACCESS, TRUE, TRUE, NULL },
    { L"修改", SESSION_MODIFY_ACCESS, TRUE, TRUE, NULL }
};

ACCESS_ENTRIES(SymbolicLink)
{
    { L"完全控制", SYMBOLIC_LINK_ALL_ACCESS, TRUE, TRUE, NULL },
    { L"完全控制（扩展）", SYMBOLIC_LINK_ALL_ACCESS_EX, TRUE, TRUE, NULL },
    { L"查询", SYMBOLIC_LINK_QUERY, TRUE, TRUE, NULL }
};

ACCESS_ENTRIES(Thread)
{
    { L"完全控制", STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0x3ff, TRUE, TRUE, NULL },
    { L"查询信息", THREAD_QUERY_INFORMATION, TRUE, TRUE, NULL },
    { L"设置信息", THREAD_SET_INFORMATION, TRUE, TRUE, NULL },
    { L"获取上下文", THREAD_GET_CONTEXT, TRUE, TRUE, NULL },
    { L"设置上下文", THREAD_SET_CONTEXT, TRUE, TRUE, NULL },
    { L"设置令牌", THREAD_SET_THREAD_TOKEN, TRUE, TRUE, NULL },
    { L"警报", THREAD_ALERT, TRUE, TRUE, NULL },
    { L"模拟", THREAD_IMPERSONATE, TRUE, TRUE, NULL },
    { L"直接模拟", THREAD_DIRECT_IMPERSONATION, TRUE, TRUE, NULL },
    { L"挂起/恢复", THREAD_SUSPEND_RESUME, TRUE, TRUE, L"挂起/恢复" },
    { L"终止", THREAD_TERMINATE, TRUE, TRUE, NULL },
};

ACCESS_ENTRIES(Thread60)
{
    { L"完全控制", STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | SPECIFIC_RIGHTS_ALL, TRUE, TRUE, NULL }, // THREAD_ALL_ACCESS
    { L"查询有限信息", THREAD_QUERY_LIMITED_INFORMATION, TRUE, TRUE, NULL },
    { L"查询信息", THREAD_QUERY_INFORMATION | THREAD_QUERY_LIMITED_INFORMATION, TRUE, TRUE, NULL },
    { L"设置有限信息", THREAD_SET_LIMITED_INFORMATION, TRUE, TRUE, NULL },
    { L"设置信息", THREAD_SET_INFORMATION | THREAD_SET_LIMITED_INFORMATION, TRUE, TRUE, NULL },
    { L"获取上下文", THREAD_GET_CONTEXT, TRUE, TRUE, NULL },
    { L"设置上下文", THREAD_SET_CONTEXT, TRUE, TRUE, NULL },
    { L"设置令牌", THREAD_SET_THREAD_TOKEN, TRUE, TRUE, NULL },
    { L"警报", THREAD_ALERT, TRUE, TRUE, NULL },
    { L"模拟", THREAD_IMPERSONATE, TRUE, TRUE, NULL },
    { L"直接模拟", THREAD_DIRECT_IMPERSONATION, TRUE, TRUE, NULL },
    { L"挂起/恢复", THREAD_SUSPEND_RESUME, TRUE, TRUE, L"挂起/恢复" },
    { L"终止", THREAD_TERMINATE, TRUE, TRUE, NULL },
};

ACCESS_ENTRIES(Timer)
{
    { L"完全控制", TIMER_ALL_ACCESS, TRUE, TRUE, NULL },
    { L"查询", TIMER_QUERY_STATE, TRUE, TRUE, NULL },
    { L"修改", TIMER_MODIFY_STATE, TRUE, TRUE, NULL }
};

ACCESS_ENTRIES(TmEn)
{
    { L"完全控制", ENLISTMENT_ALL_ACCESS, TRUE, TRUE, NULL },
    { L"读取", ENLISTMENT_GENERIC_READ, TRUE, FALSE, NULL },
    { L"写入", ENLISTMENT_GENERIC_WRITE, TRUE, FALSE, NULL },
    { L"执行", ENLISTMENT_GENERIC_EXECUTE, TRUE, FALSE, NULL },
    { L"查询信息", ENLISTMENT_QUERY_INFORMATION, FALSE, TRUE, NULL },
    { L"设置信息", ENLISTMENT_SET_INFORMATION, FALSE, TRUE, NULL },
    { L"恢复", ENLISTMENT_RECOVER, FALSE, TRUE, NULL },
    { L"下级权限", ENLISTMENT_SUBORDINATE_RIGHTS, FALSE, TRUE, NULL },
    { L"上级权限", ENLISTMENT_SUPERIOR_RIGHTS, FALSE, TRUE, NULL }
};

ACCESS_ENTRIES(TmRm)
{
    { L"完全控制", RESOURCEMANAGER_ALL_ACCESS, TRUE, TRUE, NULL },
    { L"读取", RESOURCEMANAGER_GENERIC_READ, TRUE, FALSE, NULL },
    { L"写入", RESOURCEMANAGER_GENERIC_WRITE, TRUE, FALSE, NULL },
    { L"执行", RESOURCEMANAGER_GENERIC_EXECUTE, TRUE, FALSE, NULL },
    { L"查询信息", RESOURCEMANAGER_QUERY_INFORMATION, FALSE, TRUE, NULL },
    { L"设置信息", RESOURCEMANAGER_SET_INFORMATION, FALSE, TRUE, NULL },
    { L"获取通知", RESOURCEMANAGER_GET_NOTIFICATION, FALSE, TRUE, NULL },
    { L"登记", RESOURCEMANAGER_ENLIST, FALSE, TRUE, NULL },
    { L"恢复", RESOURCEMANAGER_RECOVER, FALSE, TRUE, NULL },
    { L"注册协议", RESOURCEMANAGER_REGISTER_PROTOCOL, FALSE, TRUE, NULL },
    { L"完成传播", RESOURCEMANAGER_COMPLETE_PROPAGATION, FALSE, TRUE, NULL }
};

ACCESS_ENTRIES(TmTm)
{
    { L"完全控制", TRANSACTIONMANAGER_ALL_ACCESS, TRUE, TRUE, NULL },
    { L"读取", TRANSACTIONMANAGER_GENERIC_READ, TRUE, FALSE, NULL },
    { L"写入", TRANSACTIONMANAGER_GENERIC_WRITE, TRUE, FALSE, NULL },
    { L"执行", TRANSACTIONMANAGER_GENERIC_EXECUTE, TRUE, FALSE, NULL },
    { L"查询信息", TRANSACTIONMANAGER_QUERY_INFORMATION, FALSE, TRUE, NULL },
    { L"设置信息", TRANSACTIONMANAGER_SET_INFORMATION, FALSE, TRUE, NULL },
    { L"恢复", TRANSACTIONMANAGER_RECOVER, FALSE, TRUE, NULL },
    { L"重命名", TRANSACTIONMANAGER_RENAME, FALSE, TRUE, NULL },
    { L"创建资源管理器", TRANSACTIONMANAGER_CREATE_RM, FALSE, TRUE, NULL },
    { L"绑定事务", TRANSACTIONMANAGER_BIND_TRANSACTION, FALSE, TRUE, NULL }
};

ACCESS_ENTRIES(TmTx)
{
    { L"完全控制", TRANSACTION_ALL_ACCESS, TRUE, TRUE, NULL },
    { L"读取", TRANSACTION_GENERIC_READ, TRUE, FALSE, NULL },
    { L"写入", TRANSACTION_GENERIC_WRITE, TRUE, FALSE, NULL },
    { L"执行", TRANSACTION_GENERIC_EXECUTE, TRUE, FALSE, NULL },
    { L"查询信息", TRANSACTION_QUERY_INFORMATION, FALSE, TRUE, NULL },
    { L"设置信息", TRANSACTION_SET_INFORMATION, FALSE, TRUE, NULL },
    { L"登记", TRANSACTION_ENLIST, FALSE, TRUE, NULL },
    { L"提交", TRANSACTION_COMMIT, FALSE, TRUE, NULL },
    { L"回滚", TRANSACTION_ROLLBACK, FALSE, TRUE, NULL },
    { L"传播", TRANSACTION_PROPAGATE, FALSE, TRUE, NULL }
};

ACCESS_ENTRIES(Token)
{
    { L"完全控制", TOKEN_ALL_ACCESS, TRUE, TRUE, NULL },
    { L"读取", TOKEN_READ, FALSE, FALSE, NULL },
    { L"写入", TOKEN_WRITE, FALSE, FALSE, NULL },
    { L"执行", TOKEN_EXECUTE, FALSE, FALSE, NULL },
    { L"调整特权", TOKEN_ADJUST_PRIVILEGES, TRUE, TRUE, NULL },
    { L"调整组", TOKEN_ADJUST_GROUPS, TRUE, TRUE, NULL },
    { L"调整默认值", TOKEN_ADJUST_DEFAULT, TRUE, TRUE, NULL },
    { L"调整会话 ID", TOKEN_ADJUST_SESSIONID, TRUE, TRUE, NULL },
    { L"指派为主令牌", TOKEN_ASSIGN_PRIMARY, TRUE, TRUE, L"指派主令牌" },
    { L"复制", TOKEN_DUPLICATE, TRUE, TRUE, NULL },
    { L"模拟", TOKEN_IMPERSONATE, TRUE, TRUE, NULL },
    { L"查询", TOKEN_QUERY, TRUE, TRUE, NULL },
    { L"查询源", TOKEN_QUERY_SOURCE, FALSE, TRUE, NULL }
};

ACCESS_ENTRIES(TokenDefault)
{
    { L"完全控制", GENERIC_ALL, TRUE, TRUE, NULL },
    { L"读取", GENERIC_READ, TRUE, TRUE, NULL },
    { L"写入", GENERIC_WRITE, TRUE, TRUE, NULL },
    { L"执行", GENERIC_EXECUTE, TRUE, TRUE, NULL }
};

ACCESS_ENTRIES(TpWorkerFactory)
{
    { L"完全控制", WORKER_FACTORY_ALL_ACCESS, TRUE, TRUE, NULL },
    { L"释放工作线程", WORKER_FACTORY_RELEASE_WORKER, FALSE, TRUE, NULL },
    { L"使工作线程就绪", WORKER_FACTORY_READY_WORKER, FALSE, TRUE, NULL },
    { L"等待", WORKER_FACTORY_WAIT, FALSE, TRUE, NULL },
    { L"设置信息", WORKER_FACTORY_SET_INFORMATION, FALSE, TRUE, NULL },
    { L"查询信息", WORKER_FACTORY_QUERY_INFORMATION, FALSE, TRUE, NULL },
    { L"关闭", WORKER_FACTORY_SHUTDOWN, FALSE, TRUE, NULL }
};

ACCESS_ENTRIES(Type)
{
    { L"完全控制", OBJECT_TYPE_ALL_ACCESS, TRUE, TRUE, NULL },
    { L"创建", OBJECT_TYPE_CREATE, TRUE, TRUE, NULL }
};

ACCESS_ENTRIES(WaitCompletionPacket)
{
    { L"完全控制", OBJECT_TYPE_ALL_ACCESS, TRUE, TRUE, NULL },
    { L"修改状态", OBJECT_TYPE_CREATE, TRUE, TRUE, NULL }
};

ACCESS_ENTRIES(Wbem)
{
    { L"启用账户", WBEM_ENABLE, TRUE, TRUE, NULL },
    { L"执行方法", WBEM_METHOD_EXECUTE, TRUE, TRUE, NULL },
    { L"完全写入", WBEM_FULL_WRITE_REP, TRUE, TRUE, NULL },
    { L"部分写入", WBEM_PARTIAL_WRITE_REP, TRUE, TRUE, NULL },
    { L"提供程序写入", WBEM_WRITE_PROVIDER, TRUE, TRUE, NULL },
    { L"远程启用", WBEM_REMOTE_ACCESS, TRUE, TRUE, NULL },
    { L"获取通知", WBEM_RIGHT_SUBSCRIBE, TRUE, TRUE, NULL },
    { L"读取描述", WBEM_RIGHT_PUBLISH, TRUE, TRUE, NULL }
};

ACCESS_ENTRIES(WindowStation)
{
    { L"完全控制", WINSTA_ALL_ACCESS | STANDARD_RIGHTS_REQUIRED, TRUE, TRUE, NULL },
    { L"读取", WINSTA_GENERIC_READ, TRUE, FALSE, NULL },
    { L"写入", WINSTA_GENERIC_WRITE, TRUE, FALSE, NULL },
    { L"执行", WINSTA_GENERIC_EXECUTE, TRUE, FALSE, NULL },
    { L"枚举", WINSTA_ENUMERATE, FALSE, TRUE, NULL },
    { L"枚举桌面", WINSTA_ENUMDESKTOPS, FALSE, TRUE, NULL },
    { L"读取属性", WINSTA_READATTRIBUTES, FALSE, TRUE, NULL },
    { L"读取屏幕", WINSTA_READSCREEN, FALSE, TRUE, NULL },
    { L"访问剪贴板", WINSTA_ACCESSCLIPBOARD, FALSE, TRUE, NULL },
    { L"访问全局原子", WINSTA_ACCESSGLOBALATOMS, FALSE, TRUE, NULL },
    { L"创建桌面", WINSTA_CREATEDESKTOP, FALSE, TRUE, NULL },
    { L"写入属性", WINSTA_WRITEATTRIBUTES, FALSE, TRUE, NULL },
    { L"退出 Windows", WINSTA_EXITWINDOWS, FALSE, TRUE, NULL }
};

ACCESS_ENTRIES(WmiGuid)
{
    { L"完全控制", WMIGUID_ALL_ACCESS, TRUE, TRUE, NULL },
    { L"读取", WMIGUID_GENERIC_READ, TRUE, FALSE, NULL },
    { L"写入", WMIGUID_GENERIC_WRITE, TRUE, FALSE, NULL },
    { L"执行", WMIGUID_GENERIC_EXECUTE, TRUE, FALSE, NULL },
    { L"查询信息", WMIGUID_QUERY, FALSE, TRUE, NULL },
    { L"设置信息", WMIGUID_SET, FALSE, TRUE, NULL },
    { L"获取通知", WMIGUID_NOTIFICATION, FALSE, TRUE, NULL },
    { L"读取描述", WMIGUID_READ_DESCRIPTION, FALSE, TRUE, NULL },
    { L"执行", WMIGUID_EXECUTE, FALSE, TRUE, NULL },
    { L"创建实时日志", TRACELOG_CREATE_REALTIME, FALSE, TRUE, L"创建实时" },
    { L"创建磁盘日志", TRACELOG_CREATE_ONDISK, FALSE, TRUE, L"创建磁盘" },
    { L"启用提供程序 GUID", TRACELOG_GUID_ENABLE, FALSE, TRUE, L"启用 GUID" },
    { L"访问内核记录器", TRACELOG_ACCESS_KERNEL_LOGGER, FALSE, TRUE, NULL },
    { L"记录事件", TRACELOG_LOG_EVENT, FALSE, TRUE, NULL },
    { L"访问实时事件", TRACELOG_ACCESS_REALTIME, FALSE, TRUE, L"访问实时" },
    { L"注册提供程序 GUID", TRACELOG_REGISTER_GUIDS, FALSE, TRUE, L"注册 GUID" }
};

ACCESS_ENTRIES(Rdp)
{
    { L"完全控制", WTS_SECURITY_ALL_ACCESS, TRUE, TRUE, NULL },
    { L"查询信息", WTS_SECURITY_QUERY_INFORMATION, TRUE, TRUE, NULL },
    { L"设置信息", WTS_SECURITY_SET_INFORMATION, TRUE, TRUE, NULL },
    { L"重置", WTS_SECURITY_RESET, FALSE, TRUE, NULL },
    { L"虚拟通道", WTS_SECURITY_VIRTUAL_CHANNELS, FALSE, TRUE, NULL },
    { L"远程控制", WTS_SECURITY_REMOTE_CONTROL, FALSE, TRUE, NULL },
    { L"登录", WTS_SECURITY_LOGON, FALSE, TRUE, NULL },
    { L"注销", WTS_SECURITY_LOGOFF, FALSE, TRUE, NULL },
    { L"消息", WTS_SECURITY_MESSAGE, FALSE, TRUE, NULL },
    { L"连接", WTS_SECURITY_CONNECT, FALSE, TRUE, NULL },
    { L"断开连接", WTS_SECURITY_DISCONNECT, FALSE, TRUE, NULL },
    { L"来宾访问", WTS_SECURITY_GUEST_ACCESS, FALSE, TRUE, NULL },
    { L"来宾访问（当前）", WTS_SECURITY_CURRENT_GUEST_ACCESS, FALSE, TRUE, NULL },
    { L"用户访问", WTS_SECURITY_USER_ACCESS, FALSE, TRUE, NULL },
    { L"用户访问（当前）", WTS_SECURITY_SET_INFORMATION | WTS_SECURITY_RESET | WTS_SECURITY_VIRTUAL_CHANNELS | WTS_SECURITY_LOGOFF | WTS_SECURITY_DISCONNECT, FALSE, TRUE, NULL }, // WTS_SECURITY_CURRENT_USER_ACCESS
};

ACCESS_ENTRIES(ComAccess)
{
    { L"完全控制", COM_RIGHTS_EXECUTE | COM_RIGHTS_EXECUTE_LOCAL | COM_RIGHTS_EXECUTE_REMOTE, TRUE, TRUE, NULL },
    { L"执行", COM_RIGHTS_EXECUTE, TRUE, TRUE, NULL },
    { L"本地执行", COM_RIGHTS_EXECUTE_LOCAL, TRUE, TRUE, NULL },
    { L"远程执行", COM_RIGHTS_EXECUTE_REMOTE, TRUE, TRUE, NULL },
};

ACCESS_ENTRIES(ComLaunch)
{
    { L"完全控制", COM_RIGHTS_EXECUTE | COM_RIGHTS_EXECUTE_LOCAL | COM_RIGHTS_EXECUTE_REMOTE | COM_RIGHTS_ACTIVATE_LOCAL | COM_RIGHTS_ACTIVATE_REMOTE, TRUE, TRUE, NULL },
    { L"执行", COM_RIGHTS_EXECUTE, TRUE, TRUE, NULL },
    { L"本地执行", COM_RIGHTS_EXECUTE_LOCAL, TRUE, TRUE, NULL },
    { L"远程执行", COM_RIGHTS_EXECUTE_REMOTE, TRUE, TRUE, NULL },
    { L"本地激活", COM_RIGHTS_ACTIVATE_LOCAL, TRUE, TRUE, NULL },
    { L"远程激活", COM_RIGHTS_ACTIVATE_REMOTE, TRUE, TRUE, NULL },
};

static const PH_SPECIFIC_TYPE PhSpecificTypes[] =
{
    ACCESS_ENTRY(AlpcPort, TRUE),
    ACCESS_ENTRY(DebugObject, TRUE),
    ACCESS_ENTRY(Desktop, FALSE),
    ACCESS_ENTRY(Directory, FALSE),
    ACCESS_ENTRY(EtwConsumer, FALSE),
    ACCESS_ENTRY(EtwRegistration, FALSE),
    ACCESS_ENTRY(Event, TRUE),
    ACCESS_ENTRY(EventPair, TRUE),
    ACCESS_ENTRY(File, TRUE),
    ACCESS_ENTRY(FilterConnectionPort, FALSE),
    ACCESS_ENTRY(IoCompletion, TRUE),
    ACCESS_ENTRY(Job, TRUE),
    ACCESS_ENTRY(Key, FALSE),
    ACCESS_ENTRY(KeyedEvent, FALSE),
    ACCESS_ENTRY(LsaAccount, FALSE),
    ACCESS_ENTRY(LsaPolicy, FALSE),
    ACCESS_ENTRY(LsaSecret, FALSE),
    ACCESS_ENTRY(LsaTrusted, FALSE),
    ACCESS_ENTRY(Mutant, TRUE),
    ACCESS_ENTRY(Partition, TRUE),
    ACCESS_ENTRY(Process, TRUE),
    ACCESS_ENTRY(Process60, TRUE),
    ACCESS_ENTRY(Profile, FALSE),
    ACCESS_ENTRY(SamAlias, FALSE),
    ACCESS_ENTRY(SamDomain, FALSE),
    ACCESS_ENTRY(SamGroup, FALSE),
    ACCESS_ENTRY(SamServer, FALSE),
    ACCESS_ENTRY(SamUser, FALSE),
    ACCESS_ENTRY(Section, FALSE),
    ACCESS_ENTRY(Semaphore, TRUE),
    ACCESS_ENTRY(Service, FALSE),
    ACCESS_ENTRY(SCManager, FALSE),
    ACCESS_ENTRY(Session, FALSE),
    ACCESS_ENTRY(SymbolicLink, FALSE),
    ACCESS_ENTRY(Thread, TRUE),
    ACCESS_ENTRY(Thread60, TRUE),
    ACCESS_ENTRY(Timer, TRUE),
    ACCESS_ENTRY(TmEn, FALSE),
    ACCESS_ENTRY(TmRm, FALSE),
    ACCESS_ENTRY(TmTm, FALSE),
    ACCESS_ENTRY(TmTx, FALSE),
    ACCESS_ENTRY(Token, FALSE),
    ACCESS_ENTRY(TokenDefault, FALSE),
    ACCESS_ENTRY(TpWorkerFactory, FALSE),
    ACCESS_ENTRY(Type, FALSE),
    ACCESS_ENTRY(WaitCompletionPacket, FALSE),
    ACCESS_ENTRY(Wbem, FALSE),
    ACCESS_ENTRY(WindowStation, FALSE),
    ACCESS_ENTRY(WmiGuid, TRUE),
    ACCESS_ENTRY(Rdp, FALSE),
    ACCESS_ENTRY(ComAccess, FALSE),
    ACCESS_ENTRY(ComLaunch, FALSE),
};

/**
 * Gets access entries for an object type.
 *
 * \param Type The name of the object type.
 * \param AccessEntries A variable which receives an array of access entry structures. You must free
 * the buffer with PhFree() when you no longer need it.
 * \param NumberOfAccessEntries A variable which receives the number of access entry structures
 * returned in
 * \a AccessEntries.
 */
BOOLEAN PhGetAccessEntries(
    _In_ PCWSTR Type,
    _Out_ PPH_ACCESS_ENTRY *AccessEntries,
    _Out_ PULONG NumberOfAccessEntries
    )
{
    ULONG i;
    PPH_SPECIFIC_TYPE specificType = NULL;
    PPH_ACCESS_ENTRY accessEntries;

    if (PhEqualStringZ(Type, L"ALPC Port", TRUE))
    {
        Type = L"AlpcPort";
    }
    else if (PhEqualStringZ(Type, L"Port", TRUE))
    {
        Type = L"AlpcPort";
    }
    else if (PhEqualStringZ(Type, L"WaitablePort", TRUE))
    {
        Type = L"AlpcPort";
    }
    else if (PhEqualStringZ(Type, L"Process", TRUE))
    {
        Type = L"Process60";
    }
    else if (PhEqualStringZ(Type, L"Thread", TRUE))
    {
        Type = L"Thread60";
    }
    else if (PhEqualStringZ(Type, L"FileObject", TRUE))
    {
        Type = L"File";
    }
    else if (PhEqualStringZ(Type, L"Device", TRUE))
    {
        Type = L"File";
    }
    else if (PhEqualStringZ(Type, L"Driver", TRUE))
    {
        Type = L"File";
    }
    else if (PhEqualStringZ(Type, L"PowerDefault", TRUE))
    {
        Type = L"Key";
    }
    else if (PhEqualStringZ(Type, L"RdpDefault", TRUE))
    {
        Type = L"Rdp";
    }
    else if (PhEqualStringZ(Type, L"WmiDefault", TRUE))
    {
        // WBEM doesn't allow StandardAccessEntries (dmex)
        for (i = 0; i < RTL_NUMBER_OF(PhSpecificTypes); i++)
        {
            if (PhEqualStringZ(PhSpecificTypes[i].Type, L"Wbem", TRUE))
            {
                specificType = (PPH_SPECIFIC_TYPE)&PhSpecificTypes[i];
                break;
            }
        }

        if (specificType)
        {
            accessEntries = PhAllocate(specificType->SizeOfAccessEntries);
            memcpy(accessEntries, specificType->AccessEntries, specificType->SizeOfAccessEntries);

            *AccessEntries = accessEntries;
            *NumberOfAccessEntries = specificType->SizeOfAccessEntries / sizeof(PH_ACCESS_ENTRY);
            return TRUE;
        }
    }

    // Find the specific type.
    for (i = 0; i < sizeof(PhSpecificTypes) / sizeof(PH_SPECIFIC_TYPE); i++)
    {
        if (PhEqualStringZ(PhSpecificTypes[i].Type, Type, TRUE))
        {
            specificType = (PPH_SPECIFIC_TYPE)&PhSpecificTypes[i];
            break;
        }
    }

    if (specificType)
    {
        ULONG sizeOfEntries;

        // Copy the specific access entries and append the standard access entries.

        if (specificType->HasSynchronize)
            sizeOfEntries = specificType->SizeOfAccessEntries + sizeof(PhStandardAccessEntries);
        else
            sizeOfEntries = specificType->SizeOfAccessEntries + sizeof(PhStandardAccessEntries) - sizeof(PH_ACCESS_ENTRY);

        accessEntries = PhAllocate(sizeOfEntries);
        memcpy(accessEntries, specificType->AccessEntries, specificType->SizeOfAccessEntries);

        if (specificType->HasSynchronize)
        {
            memcpy(
                PTR_ADD_OFFSET(accessEntries, specificType->SizeOfAccessEntries),
                PhStandardAccessEntries,
                sizeof(PhStandardAccessEntries)
                );
        }
        else
        {
            memcpy(
                PTR_ADD_OFFSET(accessEntries, specificType->SizeOfAccessEntries),
                &PhStandardAccessEntries[1],
                sizeof(PhStandardAccessEntries) - sizeof(PH_ACCESS_ENTRY)
                );
        }

        *AccessEntries = accessEntries;
        *NumberOfAccessEntries = sizeOfEntries / sizeof(PH_ACCESS_ENTRY);
    }
    else
    {
        *AccessEntries = PhAllocateCopy((PVOID)PhStandardAccessEntries, sizeof(PhStandardAccessEntries));
        *NumberOfAccessEntries = sizeof(PhStandardAccessEntries) / sizeof(PH_ACCESS_ENTRY);
    }

    return TRUE;
}

static int __cdecl PhpAccessEntryCompare(
    _In_ const void *elem1,
    _In_ const void *elem2
    )
{
    const PH_ACCESS_ENTRY* entry1 = (const PH_ACCESS_ENTRY*)elem1;
    const PH_ACCESS_ENTRY* entry2 = (const PH_ACCESS_ENTRY*)elem2;

    return uintcmp(PhCountBits(entry2->Access), PhCountBits(entry1->Access));
}

/**
 * Creates a string representation of an access mask.
 *
 * \param Access The access mask.
 * \param AccessEntries An array of access entry structures. You can call PhGetAccessEntries() to
 * retrieve the access entry structures for a standard object type.
 * \param NumberOfAccessEntries The number of elements in \a AccessEntries.
 *
 * \return The string representation of \a Access.
 */
PPH_STRING PhGetAccessString(
    _In_ ACCESS_MASK Access,
    _In_ PPH_ACCESS_ENTRY AccessEntries,
    _In_ ULONG NumberOfAccessEntries
    )
{
    PH_STRING_BUILDER stringBuilder;
    PPH_ACCESS_ENTRY accessEntries;
    BOOLEAN matchedStack[64];
    PBOOLEAN matched;
    ULONG i;
    ULONG j;

    PhInitializeStringBuilder(&stringBuilder, 32);

    // Sort the access entries according to how many access rights they include.
    accessEntries = PhAllocateCopy(AccessEntries, NumberOfAccessEntries * sizeof(PH_ACCESS_ENTRY));
    qsort(accessEntries, NumberOfAccessEntries, sizeof(PH_ACCESS_ENTRY), PhpAccessEntryCompare);

    if (NumberOfAccessEntries <= RTL_NUMBER_OF(matchedStack))
    {
        matched = matchedStack;
        memset(matched, 0, sizeof(matchedStack));
    }
    else
    {
        matched = PhAllocateZero(NumberOfAccessEntries * sizeof(BOOLEAN));
    }

    for (i = 0; i < NumberOfAccessEntries; i++)
    {
        // We make sure we haven't matched this access entry yet. This ensures that we won't get
        // duplicates, e.g. FILE_GENERIC_READ includes FILE_READ_DATA, and we don't want to display
        // both to the user.
        if (
            !matched[i] &&
            ((Access & accessEntries[i].Access) == accessEntries[i].Access)
            )
        {
            if (accessEntries[i].ShortName)
                PhAppendStringBuilder2(&stringBuilder, accessEntries[i].ShortName);
            else
                PhAppendStringBuilder2(&stringBuilder, accessEntries[i].Name);

            PhAppendStringBuilder2(&stringBuilder, L", ");

            // Disable equal or more specific entries.
            for (j = i; j < NumberOfAccessEntries; j++)
            {
                if ((accessEntries[i].Access | accessEntries[j].Access) == accessEntries[i].Access)
                    matched[j] = TRUE;
            }
        }
    }

    // Remove the trailing ", ".
    if (PhEndsWithString2(stringBuilder.String, L", ", FALSE))
        PhRemoveEndStringBuilder(&stringBuilder, 2);

    if (matched != matchedStack)
        PhFree(matched);
    PhFree(accessEntries);

    return PhFinalStringBuilderString(&stringBuilder);
}
