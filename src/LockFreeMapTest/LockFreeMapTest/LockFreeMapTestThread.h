#pragma once
#include <atomic>
#include <functional>
#include <new>
#include <stdexcept>
#include <utility>
#include <vector>
#ifdef _WIN32
#include <Windows.h>
#include <process.h>
#else
#include <pthread.h>
#endif

/** 基础库的独立线程测试适配器，避免依赖正在整改的上层线程调度器
@note 析构回收已启动线程；工作函数异常记录为失败，不穿越系统线程入口
*/
class LockFreeMapTestThread
{
public:
    // 启动、回收及线程函数异常累计数，由测试入口判定失败
    static std::atomic<size_t> m_errors;

public:
    /** 初始化未启动的线程句柄
    */
    LockFreeMapTestThread();

    /** 启动独立工作函数，失败抛给测试用例边界
    @param [in] function 在线程中执行的可调用对象
    */
    template <typename Function>
    explicit LockFreeMapTestThread(Function function);

    /** 转移唯一线程回收责任
    @param [in,out] other 原句柄持有者
    */
    LockFreeMapTestThread(LockFreeMapTestThread&& other) throw();

    /** 回收已有线程后转移唯一句柄
    @param [in,out] other 原句柄持有者
    @return 当前对象
    */
    LockFreeMapTestThread& operator=(LockFreeMapTestThread&& other) throw();

    /** 回收尚未显式等待的线程
    */
    ~LockFreeMapTestThread();

    /** 等待工作结束并关闭系统资源
    @return 成功回收或原本无句柄时返回 true
    */
    bool join();

private:
    /** 启动已经构造的工作函数
    @param [in,out] function 成功后由工作线程负责释放
    @return 是否成功启动
    */
    bool start(std::function<void()>* function);

    /** 系统线程入口，捕获全部工作函数异常
    @param [in] context 独占工作函数
    @return 系统要求的线程返回值
    */
#ifdef _WIN32
    static unsigned __stdcall entry(void* context);
#else
    static void* entry(void* context);
#endif

    /** 禁止复制线程所有权
    @param [in] other 原对象
    */
    LockFreeMapTestThread(const LockFreeMapTestThread& other) = delete;

    /** 禁止复制线程所有权
    @param [in] other 原对象
    @return 当前对象
    */
    LockFreeMapTestThread& operator=(const LockFreeMapTestThread& other) = delete;

private:
    // 只由持有线程访问的系统句柄
#ifdef _WIN32
    HANDLE m_handle;
#else
    pthread_t m_handle;
#endif
    // 当前对象是否承担回收责任
    bool m_joinable;
};

/** 在被线程捕获的局部对象析构前，释放启动门并回收一组线程
*/
class LockFreeMapTestThreadGroup
{
public:
    /** 绑定已有线程数组和可选启动门，不分配内存
    @param [in,out] threads 持有线程对象的数组
    @param [in,out] gate 异常退出时需要打开的启动门
    */
    LockFreeMapTestThreadGroup(std::vector<LockFreeMapTestThread>& threads, std::atomic<bool>* gate = nullptr);

    /** 打开启动门并回收全部线程
    */
    ~LockFreeMapTestThreadGroup();

private:
    /** 禁止复制回收责任
    @param [in] other 原对象
    */
    LockFreeMapTestThreadGroup(const LockFreeMapTestThreadGroup& other) = delete;

    /** 禁止复制回收责任
    @param [in] other 原对象
    @return 当前对象
    */
    LockFreeMapTestThreadGroup& operator=(const LockFreeMapTestThreadGroup& other) = delete;

private:
    // 借用的线程数组，在守卫析构后才可销毁
    std::vector<LockFreeMapTestThread>* m_threads;
    // 借用的可选启动门
    std::atomic<bool>* m_gate;
};

#include "LockFreeMapTestThread.inl"