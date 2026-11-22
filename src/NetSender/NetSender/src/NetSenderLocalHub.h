#pragma once
#include "NetSenderLocal.h"
#include <atomic>

// 接收器归属的唯一阶段；回调登记只在IDLE到REGISTERED/运行阶段之间发生。
enum NetSenderLocalHubPhase
{
    // 没有本Hub回调登记及租约。
    NET_LOCAL_HUB_IDLE,
    // 已登记稳定回调，但init失败或还未取得接收器，允许原地重试。
    NET_LOCAL_HUB_REGISTERED,
    // 执行登记/init事务期间，拒绝其他生命周期修改。
    NET_LOCAL_HUB_STARTING,
    // Hub拥有接收器；最后租约和收件箱都归还后才停止。
    NET_LOCAL_HUB_MANAGED,
    // 宿主拥有接收器，prepare已登记回调，但接收器不一定已启动。
    NET_LOCAL_HUB_EXTERNAL,
    // 等待自有接收器退出；异常时保持拒绝接纳，不冒充已停止。
    NET_LOCAL_HUB_STOPPING
};

// 池内稳定借用槽：地址先初始化再开放；关闭后等所有已进入回调归还才允许销毁收件箱
struct NetSenderLocalHubSlot
{
public:
    /** 创建空闲且禁止回调进入的槽
    */
    NetSenderLocalHubSlot();

    /** 多读者取得非独占借用，关闭或计数耗尽立即失败
    @return 是否取得本槽借用，成功须release
    */
    bool acquire();

    /** 回调已不再访问收件箱后归还，不进行阻塞等待
    */
    void release();
public:
    // 原子地址只作身份查询；解引用必须先取得下面计数借用
    std::atomic<NetSenderLocalInbox*> m_box;
    // 初始化后只读的后继地址；所属槽直到全部租约/箱退出并配置或析构时才归还
    NetSenderLocalHubSlot* m_next;
    // 当前稳定槽实际占有的池引用，只由Hub整体回收
    void* m_storage;
    // 0..0x3fffffff为开放读者数，0x40000000准备，0x80000000关闭，0xc0000000空闲
    std::atomic<uint32_t> m_readers;
};

// 模块内稳定ProcessWork回调；拥有接收器租约和有界借用表，不拥有业务收件箱。
class NetSenderLocalHub : public ProcessReceiveCallback
{
public:
    /** 取得模块生命周期入口，不创建接收线程
    @return 稳定实例，首次调用由生命周期owner完成
    */
    static NetSenderLocalHub& instance();

    /** 无租约/收件箱/回调时替换稳定借用槽的实际池，只由启动owner调用
    @param [in] config 真实槽容量和增长策略；失败保留原池
    @return NI_OK或配置/内存/状态错误
    */
    int32_t configurePool(const LockFreeHashMapPoolConfig& config);

    /** 附加一个收件箱，必须已取得acquire租约；同一箱的attach/detach由其owner串行，不在此方法登记原IPC回调
    @param [in] inbox 非空，在detach返回前存活，重复附加幂等
    @return OK或实际池/生命周期错误，无固定收件箱数量限制
    */
    int32_t attach(NetSenderLocalInbox* inbox);

    /** 移除借用；外部模式不替宿主停止，管理模式必要时等待退出；不能从原ProcessWork回调调用
    @param [in] inbox 已收敛协议的箱，允许未附加/空指针
    */
    void detach(NetSenderLocalInbox* inbox);

    /** 取得接收租约；计数只保护生命周期，不作为业务容量；首个管理租约创建接收器
    @return OK、INVALID_STATE未就绪、BUFFER_TOO_SMALL容量不足或有界错误；失败不取得租约
    */
    int32_t acquire();

    /** 宿主在自己的initReceive之前登记稳定回调；重复prepare幂等
    @return OK或错误，不创建接收器；随后由宿主按ProcessWork规范串行启动
    */
    int32_t prepareExternal();

    /** 全部租约/箱归还且宿主uninitReceive后，清除本Hub登记状态
    @return OK或INVALID_STATE，不替宿主关闭接收器
    */
    int32_t externalStopped();

    /** 归还一个租约；生命周期owner调用，不在协议或ProcessWork回调中等待自身
    */
    void release();

    /** 稳定原回调；快速过滤无关类型，仅在有限借用表内分派有界复制
    @param [in] sendPid 原传输PID，不作为业务授权
    @param [in] bytes 回调期间借用的完整NIP2字节
    @param [in] size 字节数
    @param [in] type 原ProcessWork消息类型
    */
    virtual void receive(int32_t sendPid, const char* bytes, int32_t size, MessageType type);

private:
    /** 建立空借用表，不登记回调或创建线程
    */
    NetSenderLocalHub();

    /** 使用者应已显式归还租约，不跨静态析构次序停止其他组件
    */
    ~NetSenderLocalHub();

    /** 共用的登记/启动事务，失败保留已经成功登记的回调供重试
    @param [in] external true只准备宿主模式；false取得管理或已就绪外部租约
    @return OK或负数，不关闭其他组件的接收器
    */
    int32_t prepare(bool external);

    /** 最后租约与借用均退出后，停止仅由本Hub拥有的接收器
    */
    void stopUnused();

    /** 禁止复制稳定回调和接收器所有权
    @param [in] other 原实例
    */
    NetSenderLocalHub(const NetSenderLocalHub& other);

    /** 禁止复制稳定回调和接收器所有权
    @param [in] other 原实例
    @return 不可调用
    */
    NetSenderLocalHub& operator=(const NetSenderLocalHub& other);

private:
    // 稳定槽实际存储，借用链不额外申请节点
    std::unique_ptr<NetObjectPool<NetSenderLocalHubSlot>> m_slots;
    // 已分配槽的只追加链，未占用槽可复用，读者不承担动态节点回收
    std::atomic<NetSenderLocalHubSlot*> m_first;
    // 单次CAS维护阶段/租约/箱数：0..2阶段，3..31租约，32..63箱数
    std::atomic<uint64_t> m_ownership;
};