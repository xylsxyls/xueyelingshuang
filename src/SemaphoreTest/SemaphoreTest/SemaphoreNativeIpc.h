#pragma once
#include "Semaphore.h"
#include <Windows.h>

/** 测试用Windows原生IPC对照，本地操作仍继承Semaphore
*/
class SemaphoreNativeIpc : public Semaphore
{
public:
    /** 初始化空句柄，创建系统对象在计时区间外
    */
    SemaphoreNativeIpc();

    /** 关闭本实例引用，失败输出诊断
    */
    ~SemaphoreNativeIpc();

    /** 创建初始计数为0的命名信号量
    @param [in] name 逻辑名称
    @param [in] maximum 最大许可数
    */
    void createProcessSemaphore(const std::string& name, int32_t maximum);

    /** 打开既有对象
    @param [in] name 名称
    @return 是否成功
    */
    bool tryOpenProcessSemaphore(const std::string& name);

    /** 直接调用系统发布
    @return 0成功，1计数满，其他错误为-1
    */
    int32_t tryProcessSignal();

    /** 直接调用系统等待
    @param [in] timeout 毫秒，-1无限
    @return 是否取得许可
    */
    bool processWait(int32_t timeout);

    /** 直接使用原生组合调用，仅作同一工作量的诊断对照
    @param [in,out] waitOn 已打开的回复对象
    @param [in] timeout 总预算毫秒，-1无限
    @return 分别报告发送与消费，未知失败不允许自动重发
    */
    SemaphoreSignalWaitResult processSignalAndWaitResult(SemaphoreNativeIpc& waitOn, int32_t timeout);

    /** 对端退出前检查清理错误
    @return 清理失败累计数
    */
    static uint32_t nativeCleanupFailures();

private:
    // 原生对照清理失败计数
    static std::atomic<uint32_t> s_nativeCleanupFailures;
    // 仅由原生对照持有的句柄
    HANDLE m_nativeHandle;
    // 独立NT诊断使用的动态入口，普通系统对照保持为空
    void* m_nativeWait;
    // 独立NT诊断发布入口，不写入正式库
    void* m_nativeRelease;
};