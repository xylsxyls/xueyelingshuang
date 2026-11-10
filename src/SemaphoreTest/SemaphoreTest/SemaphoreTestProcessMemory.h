#pragma once
#include "SemaphoreResult.h"
#include <string>
#include <cerrno>
#define SEMAPHORE_SHARED_COUNTER 1

/** Windows移动后端模拟器使用的POSIX相对时间布局 */
struct timespec
{
    // 整秒
    long tv_sec;
    // 纳秒余数
    long tv_nsec;
};

/** 将生产路径的短片段休眠映射到Windows系统睡眠
@param [in] requested 本库使用的不足一秒的片段
@param [out] remaining 本替身不产生EINTR，允许为空
@return 0成功，-1参数超出本替身的模拟范围
*/
int nanosleep(const timespec* requested, timespec* remaining);

/** Windows上的移动计数映射替身，仅验证计数算法和部分关闭恢复。
    不代表POSIX文件、目录权限或手机资源生命周期验证。
    测试构建将Semaphore实现中的映射类型绑定到此替身，不编入正式库。
*/
class SemaphoreProcessMemory
{
public:
    // 模拟共享布局中的计数和上限
    int32_t* m_words;
    /** 初始化无资源状态 */
    SemaphoreProcessMemory();
    /** 正常释放映射及句柄，失败计入诊断 */
    ~SemaphoreProcessMemory();
    /** 原子读取共享字段 */
    static int32_t load(int32_t* value);
    /** 对共享字段执行一次CAS */
    static bool compareExchange(int32_t* value, int32_t expected, int32_t desired);
    /** 正上限创建映射，零上限仅打开 */
    SemaphoreResult open(const std::string& logicalName, int32_t limit);
    /** Windows映射没有独立撤名操作 */
    SemaphoreResult unlinkName();
    /** 允许注入关闭失败，剩余资源供下次重试 */
    SemaphoreResult close();
private:
    /** 映射替身也禁止复制资源所有权 */
    SemaphoreProcessMemory(const SemaphoreProcessMemory& other);
    /** 不提供资源赋值 */
    SemaphoreProcessMemory& operator=(const SemaphoreProcessMemory& other);
private:
    // 测试进程拥有的Windows文件映射句柄
    void* m_handle;
};