#pragma once
#include <stdint.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

/** NetWork内部短锁和条件通知；创建后锁定、停止及唤醒不分配C++堆内存
*/
class NetWorkGate
{
public:
    /** 初始化平台同步原语，失败由valid显式报告
    */
    NetWorkGate();

    /** 全部等待者和调用者退出后释放原语
    */
    ~NetWorkGate();

    /** 查询原语是否创建成功
    @return 是否可用
    */
    bool valid() const;

    /** 非递归取得内部短锁，仅在valid后调用
    */
    void lock();

    /** 释放本线程持有的短锁
    */
    void unlock();

    /** 原子释放锁并等待通知，返回时重新持锁；谓词由调用者循环检查
    @return false表示系统等待失败，调用者应停止并收敛已接纳任务
    */
    bool wait();

    /** 原子释放锁并等待有限时间，返回时仍持锁；超时也返回true
    @param [in] milliseconds 有限相对毫秒，不接受UINT32_MAX
    @return false仅表示系统错误；谓词必须重新检查
    */
    bool waitFor(uint32_t milliseconds);

    /** 唤醒一个等待者，状态修改和谓词检查必须使用同一把锁
    */
    void notify();

    /** 唤醒本门全部等待者，通知本身不代表谓词成立
    */
    void notifyAll();
private:
    /** 禁止复制平台资源
    @param [in] other 原对象
    */
    NetWorkGate(const NetWorkGate& other);

    /** 禁止转移正在等待的同步原语
    @param [in] other 原对象
    @return 不可调用
    */
    NetWorkGate& operator=(const NetWorkGate& other);
private:
#ifdef _WIN32
    // Vista及以上SRW锁，与既有ReadWriteMutex的Windows目标一致
    SRWLOCK m_mutex;
    // 与SRW锁配对的本进程通知，不持有业务资源
    CONDITION_VARIABLE m_changed;
#else
    // Linux、Android和Apple平台均使用pthread条件等待及配对互斥锁
    pthread_mutex_t m_mutex;
    // 与同一POSIX互斥锁配对的条件变量
    pthread_cond_t m_changed;
#endif
    // 分步初始化的所有权标记；失败构造也能安全析构
    bool m_mutexReady;
    // 条件变量初始化成功，析构据此决定是否销毁
    bool m_conditionReady;
};