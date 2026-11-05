#pragma once
#include "SemaphoreResult.h"
#include <atomic>
#include <string>
#ifdef __APPLE__
#include <TargetConditionals.h>
#endif
#if defined(__ANDROID__) || (defined(__APPLE__) && TARGET_OS_IPHONE) || defined(PROCESS_WORK_PORTABLE_NOTIFY)
#define SEMAPHORE_SHARED_COUNTER 1
#endif

/** 移动端两int32字段的既有跨进程协议；对象只在外部静止期创建/关闭
*/
class SemaphoreProcessMemory
{
public:
    // 借用映射中的计数、上限，销毁前不得仍有在途操作
    int32_t* m_words;

public:
    /** 初始化无资源状态
    */
    SemaphoreProcessMemory();

    /** 清理本次持有资源；错误进入无分配诊断计数
    */
    ~SemaphoreProcessMemory();

    /** 启动期共享目录
    @return 本模块配置引用，仅静止期访问
    */
    static std::string& directory();

    /** 目录配置封存状态
    @return 首次使用后为true
    */
    static std::atomic<bool>& directorySealed();

    /** 验证并打开目录
    @param [in] path 当前UID拥有的绝对目录
    @return 描述符，失败-1
    */
    static int32_t openDirectory(const std::string& path);

    /** 跨进程原子读取
    @param [in] value 对齐的共享字段
    @return 字段快照
    */
    static int32_t load(int32_t* value);

    /** 跨进程CAS
    @param [in,out] value 对齐的共享字段
    @param [in] expected 预期值
    @param [in] desired 新值
    @return 是否提交
    */
    static bool compareExchange(int32_t* value, int32_t expected, int32_t desired);

    /** 取得映射，失败保留局部所有权供析构回滚
    @param [in] logicalName 已验证名称
    @param [in] limit 正数创建，0仅打开
    @return OK或原生错误
    */
    SemaphoreResult open(const std::string& logicalName, int32_t limit);

    /** 撤销本代名称，已有映射有效；成功后不能再次撤销同名新代
    @return 撤名结果
    */
    SemaphoreResult unlinkName();

    /** 静止期关闭，失败引用留存供重试
    @return 清理结果
    */
    SemaphoreResult close();

private:
    // Windows桌面算法验证用映射句柄
    void* m_handle;
    // POSIX目录描述符
    int32_t m_directoryDescriptor;
    // POSIX映射文件描述符
    int32_t m_descriptor;
    // 只有独占创建者在关闭时自动撤名
    bool m_created;
    // 本代名称，成功撤名即清空
    std::string m_name;
};