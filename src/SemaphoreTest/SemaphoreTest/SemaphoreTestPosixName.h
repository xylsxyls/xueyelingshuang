#pragma once
#if defined(_WIN32) && defined(SEMAPHORE_TEST_POSIX_NAME)
#include <stddef.h>
#include <stdint.h>
#include <fcntl.h>
#include <cerrno>
#include <Windows.h>
#ifndef EOVERFLOW
#define EOVERFLOW 132
#endif

/** 测试命名表条目；命名引用和进程打开引用分开管理
*/
struct SemaphoreTestPosixSlot
{
    // 固定表避免测试名字处理引入额外堆申请
    char m_name[128];
    // 真实系统通知句柄
    HANDLE m_handle;
    // 本进程打开引用数
    uint32_t m_references;
    // 名称是否仍可查找
    bool m_named;
};
typedef SemaphoreTestPosixSlot sem_t;
#define SEM_FAILED reinterpret_cast<sem_t*>(-1)

/** 统计夹具持有的内核句柄，也用于在资源基线之前完成测试互斥量的运行库初始化
@return 活跃句柄数；统计失败返回UINT32_MAX
*/
uint32_t SemaphoreTestPosixNameHandles();

/** Windows测试宿主的POSIX相对睡眠参数，仅用于编译原生产IPC分支
*/
struct timespec
{
    // 秒数
    long tv_sec;
    // 纳秒数
    long tv_nsec;
};

/** 测试命名空间模拟POSIX创建/打开和引用，通知本身仍使用Windows内核信号量
    仅验证生产POSIX分支的所有权流程，不代表Linux/macOS系统调用实测
@param [in] name 原生命名字符串
@param [in] flags 创建及独占标志；创建时后续参数为权限及初始许可
@return 引用或SEM_FAILED，错误存入errno
*/
sem_t* sem_open(const char* name, int flags, ...);

/** 从测试命名空间撤销名称，保留已有引用
@param [in] name 待撤销名称
@return 0成功，-1失败
*/
int sem_unlink(const char* name);

/** 释放一个测试引用，最后一个已撤名引用释放内核句柄
@param [in] value 本进程持有的引用
@return 0成功，-1失败
*/
int sem_close(sem_t* value);

/** 对测试引用发布一个许可
@param [in] value 本进程持有的引用
@return 0成功，-1失败
*/
int sem_post(sem_t* value);

/** 对测试引用无限等待一个许可
@param [in] value 本进程持有的引用
@return 0成功，-1失败
*/
int sem_wait(sem_t* value);

/** 对测试引用立即探测一个许可
@param [in] value 本进程持有的引用
@return 0成功，-1失败
*/
int sem_trywait(sem_t* value);

/** 用系统睡眠模拟POSIX相对睡眠，仅支持本库使用的短片段
@param [in] requested 相对时间
@param [out] remaining 本夹具不模拟EINTR，允许为空
@return 0成功，-1非法参数
*/
int nanosleep(const timespec* requested, timespec* remaining);
#endif