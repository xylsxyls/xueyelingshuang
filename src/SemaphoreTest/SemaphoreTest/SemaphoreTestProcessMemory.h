#pragma once
#include "SemaphoreResult.h"
#include <string>
#include <cerrno>
#define SEMAPHORE_SHARED_COUNTER 1

/** Windows移动后端模拟器使用的POSIX相对时间布局
*/
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

/** Windows上的移动计数映射替身，仅验证计数算法和部分关闭恢复
    不代表POSIX文件、目录权限或手机资源生命周期验证
    测试构建将Semaphore实现中的映射类型绑定到此替身，不编入正式库
*/
class SemaphoreProcessMemory
{
public:
    // 模拟共享布局中的计数和上限
    int32_t* m_words;

public:
    /** 初始化无资源状态
    */
    SemaphoreProcessMemory();

    /** 外部静止期释放映射及句柄，失败计入诊断
    */
    ~SemaphoreProcessMemory();

    /** 原子读取共享字段
    @param [in] value 非空且对齐的字段地址，映射须活到调用结束
    @return 字段当前快照
    */
    static int32_t load(int32_t* value);

    /** 对共享字段执行一次CAS
    @param [in,out] value 非空且对齐的字段地址，映射须活到调用结束
    @param [in] expected 预期旧值，失败时不回写
    @param [in] desired 匹配后写入的新值
    @return 是否完成替换
    */
    static bool compareExchange(int32_t* value, int32_t expected, int32_t desired);

    /** 在无资源实例上取得映射；失败时已取得资源仍由本实例清理
    @param [in] logicalName 调用方已验证的逻辑名称，仅本次借用
    @param [in] limit 正数为创建上限，0仅打开；已有映射不重置
    @return OK或映射、布局校验错误；异常由测试入口捕获
    */
    SemaphoreResult open(const std::string& logicalName, int32_t limit);

    /** Windows映射没有独立撤名操作
    @return UNSUPPORTED且不改变资源
    */
    SemaphoreResult unlinkName();

    /** 外部静止期关闭；允许注入关闭失败，剩余资源供下次重试
    @return OK或首个系统/注入错误
    */
    SemaphoreResult close();

private:
    /** 映射替身也禁止复制资源所有权
    @param [in] other 不使用
    */
    SemaphoreProcessMemory(const SemaphoreProcessMemory& other);

    /** 不提供资源赋值
    @param [in] other 不使用
    @return 不提供实现
    */
    SemaphoreProcessMemory& operator=(const SemaphoreProcessMemory& other);

private:
    // 测试进程拥有的Windows文件映射句柄
    void* m_handle;
};