#pragma once
#include <stdint.h>
#include <string>
#include <vector>
#include <atomic>
#include <memory>

class NetGatewayTask;

// 本进程唯一的启动与退出协调者；主线程拥有租约和任务，网络状态仍归任务执行者
class NetServerManager
{
public:
    /** 取得进程单例，main中首次调用；只初始化管理状态，不启动线程或网络
    @return 与进程同寿命的管理对象，不得delete
    */
    static NetServerManager& instance();

    /** 主线程校验命令并准备资源；成功后调用run，结束后调用uninit
    @param [in] arguments UTF8命令行参数，不含可执行文件名；仅在调用期间借用
    @return 0为准备完成或帮助/复用命令成功，1为运行准备失败，2为参数错误；失败自动回收，重复init拒绝
    */
    int init(const std::vector<std::string>& arguments);

    /** 主线程驱动进程生命周期，直到任务完成或请求停止；不与init/uninit并发或重入
    @return 进程退出码；帮助/复用命令为0，未初始化或重复run为1
    */
    int run();

    /** 任何普通线程均可重复调用，仅提交当前代停止意图；不等待、不访问任务或释放资源
    */
    void stop();

    /** 主线程幂等收尾：停止接纳、等待任务、恢复信号处理、释放进程租约；之后允许再次init
    @return 已完成回收；run期间调用只请求停止并返回false，其他线程不得调用
    */
    bool uninit();

private:
    /** 初始化空状态，不取得业务资源
    */
    NetServerManager();

    /** 主线程析构兜底，正常main必须已显式uninit
    */
    ~NetServerManager();

    /** 禁止复制进程资源所有权
    @param [in] other 原对象
    */
    NetServerManager(const NetServerManager& other);

    /** 禁止赋值进程资源所有权
    @param [in] other 原对象
    @return 不可调用
    */
    NetServerManager& operator=(const NetServerManager& other);

    /** 主线程准备本次命令；部分资源始终登记在本对象，异常由init统一回收
    @param [in] arguments UTF8命令行参数，不含程序名
    @return 0成功，1准备失败，2非法参数
    */
    int prepare(const std::vector<std::string>& arguments);

    /** 主线程安装停止通知，保存原处理器供uninit恢复
    @return 所需处理器均安装成功
    */
    bool installSignals();

    /** 仅主线程停止任务并加入执行线程，允许系统会话退出回调重复调用
    @return 任务线程已收敛；失败保留资源供后续uninit重试
    */
    bool stopTask();

private:
    // 原子停止意图，init开始新一代时重置，其余线程只通过stop写入
    std::atomic<bool> m_stop;
    // 传输宿主表按进程登记一次，uninit只释放本代实例，不能重复注册全局宿主
    bool m_hostConfigured;
    // 本次命令已准备完成，帮助及复用命令也属于已初始化
    bool m_initialized;
    // 主线程正在run中，拒绝同线程重入或提前销毁状态
    bool m_running;
    // 本代run已结束，必须uninit后才能重新准备命令
    bool m_completed;
    // 等待任务失败，不能将本代运行报告成成功
    bool m_cleanupFailed;
    // 本进程租约，任务及回调全部收敛后才能释放
    std::vector<uint64_t> m_lease;
    // 唯一任务执行线程ID，0表示没有待回收的线程
    uint32_t m_thread;
    // 本代任务强引用，线程回收完成后由主线程释放
    std::shared_ptr<NetGatewayTask> m_task;
    // 本代显式停止文件路径，主线程检查；不改变业务退出后常驻规则
    std::string m_stopFile;
    // 安装前的SIGINT处理器，只有对应安装位为true时有效
    void (*m_previousInterrupt)(int);
    // 安装前的SIGTERM处理器，只有对应安装位为true时有效
    void (*m_previousTerminate)(int);
    // SIGINT处理器已被本代接管，uninit时恢复
    bool m_interruptInstalled;
    // SIGTERM处理器已被本代接管，uninit时恢复
    bool m_terminateInstalled;
    // Windows控制台回调已登记，其他平台保持false
    bool m_consoleInstalled;
};