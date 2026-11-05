#pragma once
#include "SemaphoreResult.h"
#include <atomic>

/** 仅作睡眠提示的系统后端，不拥有业务许可，不用通知原语保护临界区
    没有等待记录或线程登记池；对象必须活到全部等待/通知退出
*/
class SemaphoreWake
{
public:
    /** 创建系统后端，失败抛system_error；不启动线程
    */
    SemaphoreWake();

    /** 静止期回收系统资源，清理失败计入cleanupFailures
    */
    ~SemaphoreWake();

    /** 读取睡眠比较字；先取此值，再检查业务谓词
    @return 当前比较字，不是事件代次
    */
    uint32_t snapshot() const;

    /** 更新比较字并提示所有系统等待者
    @param [in] address 非空时直接提示该稳定事件记录地址，不修改公共比较字；借用期由事件池保护
    @return 提示结果；调用方保留自己的提交状态
    */
    SemaphoreResult notify(std::atomic<uint32_t>* address = nullptr);

    /** 当前稳定后端是否支持对独立事件记录直接比较睡眠
    @return Windows地址后端、Linux/Android及Wasm线程后端为true
    */
    bool canWaitAddress() const;

    /** 外部静止期准备组合通知/等待后端；失败保留原后端，不改变业务状态
    @return Windows准备事件后端，其余可阻塞平台保留原后端；失败可见
    */
    SemaphoreResult prepareSignalAndWait();

    /** 判断两个稳定后端能否使用一个原生通知/等待调用
    @param [in] waitOn 要睡眠的目标实例
    @return 两端均已准备Windows事件后端时为true
    */
    bool canSignalAndWait(const SemaphoreWake& waitOn) const;

    /** 提示当前对象并睡眠于目标；调用方已发布业务状态及登记目标等待者
    @param [in,out] waitOn 已准备并登记的目标后端
    @param [in] expected 目标提示字快照，取得后调用方已重查目标业务谓词
    @param [in] milliseconds 单次等待预算，1至32毫秒
    @return 原生提示/超时/错误，不代表业务许可消费；失败不可重发业务许可
    */
    SemaphoreResult signalAndSleep(SemaphoreWake& waitOn, uint32_t expected, uint32_t milliseconds);

    /** 登记慢速等待；成功后必须重新检查业务谓词，再决定睡眠
    @return OK或宿主/容量错误
    */
    SemaphoreResult enterWait();

    /** 结束慢速等待登记；不能与本次sleep并发
    */
    void leaveWait();

    /** 在比较字未变时阻塞；公共比较字须先enterWait，独立事件地址由池登记并持有引用
    @param [in] expected 睡眠前快照
    @param [in] milliseconds 本次相对预算，1至32毫秒
    @param [in] address 非空时在该稳定事件记录地址等待，等待者持有记录引用；否则使用公共比较字
    @return 提示/分片超时/系统错误，不表示业务成功
    */
    SemaphoreResult sleep(uint32_t expected, uint32_t milliseconds, std::atomic<uint32_t>* address = nullptr);

    /** 判断当前调用线程能否阻塞
    @return Wasm主线程/普通模块为false，原生为true
    */
    static bool canBlock();

    /** 查询析构无法返回的系统清理失败累计数
    @return 饱和计数；仅诊断，不作本次操作的返回值
    */
    static uint32_t cleanupFailures();

    /** 记录一次无法从析构返回的系统错误
    */
    static void recordCleanupFailure();

private:
    /** 提示等待指定地址的线程，不积存系统许可
    @param [in,out] word 生命周期已受调用者保护的32位原子字
    @return 系统提示结果，不改变业务状态
    */
    SemaphoreResult notifyAddress(std::atomic<uint32_t>& word);

    /** 仅在指定比较字仍为expected时进入系统等待
    @param [in,out] word 生命周期已受调用者保护的32位原子字
    @param [in] expected 入睡前比较值
    @param [in] milliseconds 单片预算，1至32毫秒
    @return 系统提示、超时或错误
    */
    SemaphoreResult sleepAddress(std::atomic<uint32_t>& word, uint32_t expected, uint32_t milliseconds);

    /** 执行一次原生比较等待，调用方已登记系统睡眠参与者
    @param [in] expected 比较字快照
    @param [in] milliseconds 本次相对预算
    @return 原生提示/超时/失败
    */
    SemaphoreResult sleepNative(uint32_t expected, uint32_t milliseconds);

    /** 禁止复制系统资源
    @param [in] other 不使用
    */
    SemaphoreWake(const SemaphoreWake& other);

    /** 禁止赋值系统资源
    @param [in] other 不使用
    @return 不提供实现
    */
    SemaphoreWake& operator=(const SemaphoreWake& other);

private:
    // 只增不回绕的提示比较字，饱和后仍有有界系统等待补查
    std::atomic<uint32_t> m_word;
    // Windows兼容事件或Apple dispatch信号量
    void* m_handle;
    // 动态解析的Windows地址等待函数
    void* m_waitAddress;
    // 动态解析的Windows广播函数
    void* m_wakeAddress;
    // Apple未消费提示至多一个；不是互斥门
    std::atomic<uint32_t> m_hint;
    // 慢速等待参与数；登记后重查谓词，发布后无人等待可省略整个提示更新
    std::atomic<uint32_t> m_sleepers;
    // 析构失败的无分配诊断
    static std::atomic<uint32_t> s_cleanupFailures;
    // 已成功解析的Windows系统函数缓存；并发首次解析互不等待，只发布同一系统地址
    static std::atomic<void*> s_cachedWaitAddress;
    // 先发布wake地址，再以release发布wait地址，读取者用acquire取得完整函数对
    static std::atomic<void*> s_cachedWakeAddress;
};