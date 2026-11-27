#pragma once
#include "NetInterfaceMacro.h"
#include "NetHostTypes.h"
#include <atomic>

/** 协议执行者的可选事件等待；不依赖网络库或线程库，不负责消息同步
*/
class NetInterfaceAPI NetExecution
{
public:
    /** 使用启动期登记的宿主创建独占执行循环；失败由status报告
    */
    NetExecution();

    /** 外层已结束任务、销毁通道、退出所有借用唤醒者后释放
    */
    ~NetExecution();

    /** 启动期登记全空或完整事件宿主表，首次创建执行者之前调用
    @param [in] functions 生命周期内固定的函数指针，不拥有业务上下文
    @return NI_OK有效，其他为参数错误
    */
    static int32_t configure(const NetHostExecutionFunctions& functions);

    /** 查询宿主创建及运行结果，宿主异常留下 NI_INVALID_STATE；未提供事件宿主返回 NI_OK
    @return NI_OK 表示可用，NI_INVALID_STATE 表示宿主创建失败或回调抛出异常
    */
    int32_t status() const;

    /** 本轮检查队列、socket和停止标志之前调用，唯一执行者使用
    */
    void begin();

    /** 无新进展时等待事件，回调和其他线程只能wake
    @param [in] timeoutMs 下一次维护期限的最大等待毫秒，不是每轮固定休眠
    @return NI_OK 表示宿主正常结束等待，NI_NEED_MORE 表示没有事件宿主，NI_INVALID_STATE 表示宿主失败
    */
    int32_t wait(uint32_t timeoutMs);

    /** 发布数据或停止状态后通知，允许多线程调用；禁止与析构并发
    */
    void wake();

    /** 查询当前执行作用域，可被已封闭生命周期的IPC入口借用
    @return 当前执行者或空
    */
    static NetExecution* current();

    /** 唯一执行者已消费本地工作，标记下一轮仍应检查
    */
    static void progress();

    /** 本轮登记协议最早截止，唯一执行者调用；无执行作用域时忽略
    @param [in] expiresMs 协议自身时钟下的绝对截止毫秒
    @param [in] nowMs 与expiresMs同一时钟的当前毫秒
    */
    static void deadline(uint64_t expiresMs, uint64_t nowMs);

    /** 交换当前线程的执行归属；显式切换的调用方负责在退出时恢复
    @param [in] execution 新的借用执行者或空
    @return 之前的借用执行者
    */
    static NetExecution* exchange(NetExecution* execution);

    /** 切换执行者并保存宿主在线程上绑定的循环，供作用域精确恢复
    @param [in] execution 新借用的执行者或空
    @param [out] previousHost 切换前的实际宿主循环，未登记宿主时为空
    @return 之前借用的协议执行者，宿主独立运行时为空
    */
    static NetExecution* exchange(NetExecution* execution, void*& previousHost);

    /** 将本实例与自身宿主绑定到当前线程，作用域退出前须恢复原归属
    @param [out] previousHost 成功时为之前借用的宿主循环，失败时为空
    @return 绑定成功为true；实例状态失败或宿主切换异常为false，均不接管协议归属
    */
    bool bind(void*& previousHost);

    /** 按先前保存的协议与宿主配对恢复归属；通常使用NetExecutionScope自动管理
    @param [in] execution 先前借用的协议执行者或空，恢复期间保持存活
    @param [in] host 对应的有效宿主循环或空，必须来自先前的exchange或bind输出
    @param [out] previousHost 成功时为被替换的宿主循环，失败时为空
    @return 切换成功为true；宿主异常时保留当前协议归属、记录错误并返回false
    */
    static bool bind(NetExecution* execution, void* host, void*& previousHost);

private:
    /** 禁止复制拥有的宿主循环
    @param [in] other 原对象
    */
    NetExecution(const NetExecution& other);

    /** 禁止复制拥有的宿主循环
    @param [in] other 原对象
    @return 不可调用
    */
    NetExecution& operator=(const NetExecution& other);

private:
    // 启动后只读的宿主函数副本；所有权不跨库释放
    NetHostExecutionFunctions m_functions;
    // 宿主拥有的循环，由destroy回收；空表示无宿主或创建失败
    void* m_instance;
    // 唯一执行者本轮开始前的事件代次
    uint64_t m_observed;
    // 转换成本机单调时间的最早截止；每轮begin重置，不延长业务原期限
    uint64_t m_waitUntil;
    // 创建或回调异常由等待执行者及并发唤醒者共同观察。
    std::atomic<int32_t> m_status;
    // 当前owner轮次已经消费过工作；只影响本轮wait，不向自己的宿主循环发通知
    bool m_progressed;
};

/** 串行移交执行归属；析构恢复上层作用域，不驱动循环
*/
class NetInterfaceAPI NetExecutionScope
{
public:
    /** 借用执行者到本作用域退出
    @param [in,out] execution 本线程即将推进的执行者
    */
    explicit NetExecutionScope(NetExecution& execution);

    /** 恢复先前执行归属
    */
    ~NetExecutionScope();

private:
    /** 禁止复制借用
    @param [in] other 原作用域
    */
    NetExecutionScope(const NetExecutionScope& other);

    /** 禁止复制借用
    @param [in] other 原作用域
    @return 不可调用
    */
    NetExecutionScope& operator=(const NetExecutionScope& other);

private:
    // 进入作用域前宿主线程的实际循环，可能由协议库以外的调用方绑定
    void* m_previousHost;
    // 外层执行者，只借用至恢复
    NetExecution* m_previous;
    // 本层是否成功接管宿主归属，只有成功进入才需要恢复。
    bool m_bound;
};