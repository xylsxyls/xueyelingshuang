#pragma once

#include "LockFreeQueueModels.h"
#include "LockFreeQueueProducer.h"

// MPSC FIFO; one consumer owns pop/clear/maintain. Lifecycle/policy changes
// require external quiescence. No batch reversal and no producer link debt.
template <class T>
#if defined(_MSC_VER)
class __declspec(align(8)) LockFreeQueue
#else
class alignas(8) LockFreeQueue
#endif
{
public:
    /** 建立本对象确定的初始状态；所属类型约定其所有权与生命周期
    */
    LockFreeQueue();

    /** 可延迟节点池初始化，供先设置池参数的所属对象避免重复分配
    @param [in] initializeImmediately true 保持默认构造行为，false 待配置后显式 tryInit
    */
    explicit LockFreeQueue(bool initializeImmediately);

    /** 结束本对象拥有或借用的状态；调用前必须满足所属类型的静止条件
    */
    ~LockFreeQueue();

    /** 禁止复制或移动；所有权、登记与原子状态不能按位转移
    @param [in] other 禁止复制或移动的源对象
    */
    LockFreeQueue(const LockFreeQueue& other) = delete;

    /** 禁止复制或移动；所有权、登记与原子状态不能按位转移
    @param [in] other 禁止复制或移动的源对象
    */
    LockFreeQueue& operator=(const LockFreeQueue& other) = delete;

    /** 清空旧代并重新初始化；外部静止，失败抛出 LockFreeQueueError
    */
    void init();

    /** 清空旧代并重新初始化；外部静止，失败保持不可接纳状态
    @return 成功或明确失败状态；失败不接纳当前载荷
    */
    LockFreeQueueResult tryInit();

    /** 唯一消费者逐项丢弃直到观察为空；可与 push 并发，持续生产时不保证有限结束
    */
    void clear();

    /** 外部静止下析构有效载荷并释放本实例全部存储；可重复调用
    */
    void destroy();

    /** 设置节点池参数；仅允许未初始化且外部静止时调用
    @param [in] config 节点数配置，最大容量仅允许 -1 或不小于初始容量；借用至返回
    @return 成功或明确失败状态；失败不接纳当前载荷
    */
    LockFreeQueueResult configurePool(const LockFreeQueuePoolConfig& config);

    /** 静止且未初始化时选择计数方式，默认保持统一计数
    @param [in] distributed true 减少生产者共同写入；size/empty 改为遍历已使用的生产者位置
    @return 已初始化则拒绝，成功后仍保持近似 size 和精确静止计数语义
    */
    LockFreeQueueResult configureProducerCounters(bool distributed);

    /** 静止且未初始化时选择可取消预约的 FIFO；默认关闭，旧发布 CAS 排序保持不变
    @param [in] enabled true 时重叠投递按最终有效预约排序，未填充预约可被消费者取消并由投递方重试
    @return 已初始化则拒绝；两种方式均保持非重叠操作 FIFO，不等待某个暂停生产者
    */
    LockFreeQueueResult configureCancelableReservations(bool enabled);

    /** 静止且未初始化时选择仅借用外部载荷模式；默认关闭
    @param [in] enabled true 时只接纳 tryPushEntry 系列，普通 push 返回配置错误
    @return 配置结果；容量和扩缩容规则保持不变，每个外部载荷仍占一个池额度
    */
    LockFreeQueueResult configureEntryOnly(bool enabled);

    /** 外部静止下替换扩容算法；空策略恢复默认，复制失败保留原策略
    @param [in] policy 线程安全、短小、不递归调用本池的策略；保存其副本
    @return 成功或明确失败状态；失败不接纳当前载荷
    */
    LockFreeQueueResult setGrowPolicy(const LockFreeQueueGrowPolicy& policy);

    /** 外部静止下替换缩容算法；空策略禁用自动缩容，已退役块继续回收
    @param [in] policy 线程安全、短小、不递归调用本池的策略；保存其副本
    @return 成功或明确失败状态；失败不接纳当前载荷
    */
    LockFreeQueueResult setShrinkPolicy(const LockFreeQueueShrinkPolicy& policy);

    /** 由本线程复制并发布一个元素；竞争持续重试并定期让出执行机会
    资源不足、未初始化、配置或复制失败返回明确状态；不返回 Busy，不因竞争次数多而失败
    @param [in] value 待复制的载荷或待更新的原子计数；借用至本次调用返回
    @return Success 已提交，其他状态均未提交；本入口不传播分配或载荷复制异常
    */
    LockFreeQueueResult push(const T& value);

    /** 尝试复制并发布；竞争失败为 Busy，额度已满为 CapacityLimit；失败未接纳
    @param [in] value 待复制的载荷或待更新的原子计数；借用至本次调用返回
    @return 成功或明确失败状态；失败不接纳当前载荷
    */
    LockFreeQueueResult tryPush(const T& value);

    /** 复制并发布一项，成功路径保证发布先于调用者后续的原子读取
    @param [in] value 借用至返回，与 tryPush 使用相同的复制及失败语义
    @return 成功时附加完整发布屏障，可与消费者的 SC 屏障及复查配对；不排序返回后的新写入
    */
    LockFreeQueueResult tryPushAndFence(const T& value);

    /** 发布调用方拥有的外部载荷；容量、扩缩容、计数、FIFO 与复制入队共用同一队列
    @param [in,out] entry 不得重复提交在队对象；成功后地址和字段保持不变至消费返回或静止 destroy
    @return 失败未接纳；值输出接口复制临时 T 后赋值，Entry 地址接口不复制载荷
    */
    LockFreeQueueResult tryPushEntry(LockFreeQueueEntry<T>& entry);

    /** 发布外部载荷，并完成发布到调用者后续原子读取之间的屏障
    @param [in,out] entry 借用与失败语义同 tryPushEntry；队列不析构其中的 T
    @return 成功时具有与 tryPushAndFence 相同的发布屏障保证
    */
    LockFreeQueueResult tryPushEntryAndFence(LockFreeQueueEntry<T>& entry);

    /** 取得可选生产者上下文；成功后必须在队列换代或析构前释放
    @param [in,out] producer 未绑定的上下文；成功后借用本队列，须在换代前 release
    @return 成功或明确失败状态；失败不接纳当前载荷
    */
    LockFreeQueueResult tryAcquireProducer(LockFreeQueueProducer<T>& producer);

    /** 唯一消费者取出一项；nullptr 表示丢弃，输出赋值异常不移除元素
    @param [out] output 接收值，可为 nullptr；赋值抛异常时输出可能部分改变
    @return true 消费一项，false 观察为空或队列未初始化
    */
    bool pop(T* output);

    /** 仅检查并消费已发布首项，空观察不主动扫描退役段
    @param [out] output 可为空；与 pop 保持相同的赋值及 FIFO 语义
    @return 是否消费一项；唯一消费者需定期调用 maintain 或普通 pop 回收退役段
    */
    bool popReady(T* output);

    /** 唯一消费者有界取出一个 FIFO 前缀并合并归还节点；不进行反转
    @param [out] output 连续输出空间，可为空表示丢弃；至少容纳 min(maximum,16) 项
    @param [in] maximum 本轮预算，内部限制为至多 16 项，0 不取出
    @param [out] consumed 已提交的前缀项数；赋值抛异常时仍准确，异常项留在队首
    @return 是否取到至少一项；异常传播前归还已经成功消费的节点
    */
    bool popReadyBatch(T* output, size_t maximum, size_t& consumed);

    /** 唯一消费者取出至多 64 项的 FIFO 前缀，合并池额度归还；不等待发布者
    @param [out] output 可空，非空时至少容纳 min(maximum,64) 项
    @param [in] maximum 本轮消费预算，0 不取出
    @param [out] consumed 已提交数量；异常保证与 popReadyBatch 相同
    @return 是否取到至少一项；调用方仍须定期维护退役段
    */
    bool popReadyPrefix(T* output, size_t maximum, size_t& consumed);

    /** 唯一消费者在仅外部载荷模式下取出至多 64 个 Entry 地址，不复制或移动其中的 T
    @param [out] output 可空表示丢弃，非空时至少容纳 min(maximum,64) 个地址；返回后 Entry 重新归调用方控制
    @param [in] maximum 本轮预算，0 不取出；与普通前缀接口使用相同 FIFO
    @param [out] consumed 实际取出数量，对应池额度在返回前全部归还
    @return 是否取到至少一项；未配置 EntryOnly 时返回 false 且不消费元素；调用方仍须定期维护
    */
    bool popReadyEntryPrefix(LockFreeQueueEntry<T>** output, size_t maximum, size_t& consumed);

    /** 唯一消费者只读探测发布首槽，不消费、不维护；不得与其他消费者操作并发
    @return false 为本次空观察；true 也可能是段边界，须调用 pop 系列确认；不能作为销毁许可
    */
    bool possiblyReady() const;

    /** 近似观察队列是否为空；不能用作容量判定或销毁许可
    @return 近似数量是否为零
    */
    bool empty() const;

    /** 近似观察未消费数量；可能计入正在发布项，静止时精确
    @return 未消费项近似数，正在发布或并发观察可能影响结果
    */
    uint64_t size() const;

    /** 唯一消费者执行有预算回收；不启动后台线程，纯空闲需要 owner 主动调用
    @param [in] workBudget 本轮维护访问次数上限，零不做工作
    @return 策略或时钟结果、实际归还缓存节点数及物理释放容量
    */
    LockFreeQueueMaintenanceResult maintain(size_t workBudget = 64);

    /** 取得弱一致资源快照；不得作为严格接纳额度
    @return 实际容量、预留、空闲、已使用及退役容量、字节与诊断计数
    */
    LockFreeQueuePoolStatistics poolStatistics() const;

    /** 核实当前目标使用的原子宽度是否由无锁后端支持
    @return 是否满足上述条件或完成上述操作
    */
    bool atomicsLockFree() const;

private:
    typedef LockFreeQueueArena<T> Arena;
    typedef typename Arena::Node Node;
    typedef typename Arena::Block Block;
    typedef typename Arena::Record Record;
    typedef LockFreeQueueChannelPool<T> SegmentPool;

    /** 消费已确认的外部 Entry 前缀，空队列探测不进入本路径
    @param [in] node 当前已发布首项的编码地址，要求外部载荷模式
    @param [out] output 可空，非空时容纳 min(maximum,64) 个 Entry 地址
    @param [in] maximum 非零本轮预算
    @param [out] consumed 成功消费数量，调用前为零
    @return 是否消费至少一项
    */
    bool consumeEntryPrefix(Node* node, LockFreeQueueEntry<T>** output, size_t maximum, size_t& consumed);

    /** 成功发布后补足完整屏障；已由后端发布 CAS 完成时只保留编译器屏障
    */
    static void publicationFence();

    /** 申请并构造载荷后在槽位 CAS 发布；失败回滚节点及私有缓存
    @param [in] value 待复制的载荷或待更新的原子计数；借用至本次调用返回
    @param [in] wait true 持续重试纯竞争，false 可在第一次竞争失败时返回
    @param [in,out] retained 可空的已绑定登记槽；非空时调用者保留租约
    @return 成功或明确失败状态；失败不接纳当前载荷
    */
    LockFreeQueueResult pushImpl(const T& value, bool wait, Record* retained);

    /** 领取一个物理额度并发布外部载荷地址，不在额度节点中再构造 T
    @param [in,out] entry 调用方独占且尚未入队的载荷
    @param [in,out] retained 可空的已登记生产者位置
    @return 发布或具体失败状态；成功后不再访问 entry
    */
    LockFreeQueueResult pushEntryImpl(LockFreeQueueEntry<T>& entry, Record* retained);

    /** 共同的发布记账与失败回滚，发布地址可为带外部载荷标志的地址
    @param [in] address 发布到通道的地址
    @param [in,out] credit 本次独占的节点池额度
    @param [in,out] record 本次独占的生产者位置
    @return 成功或段池失败状态；失败归还当前额度，纯竞争失败可保留可复用私有缓存
    */
    LockFreeQueueResult publishNode(Node* address, Node* credit, Record& record);

    /** 读取一个完整发布项，外部载荷只复制并直接返回额度所属块
    @param [in] address 通道中的完整发布地址
    @param [out] output 可空输出；赋值异常时不提交消费
    @param [out] externalOwner 外部载荷的额度所属块；普通载荷为 nullptr
    @return 本项占用的真实节点池额度
    */
    static Node* copyFront(Node* address, T* output, Block*& externalOwner);

    /** 复制外部载荷到临时值，再按普通 pop 的赋值规则输出；不移动或修改源对象
    @param [out] output 接收值
    @param [in] source 借用的外部值
    */
    static void assignEntry(T& output, const T& source);

    /** 完成已取得首项的输出、消费和有界维护；空返回路径不进入此函数
    @param [in,out] node 唯一消费者借用的非空首节点
    @param [out] output 可为空的输出；赋值失败时保留首项
    @return 成功消费一项
    */
    bool consumeNode(Node* node, T* output, size_t* consumed = nullptr);

    /** 消费已确认非空的同段前缀；异常边界不进入空队列热路径
    @param [out] output 可为空，接收成功赋值的前缀
    @param [in] maximum 非零消费预算，至多 64
    @param [in,out] consumed 初始为 0，异常时仍返回已提交的数量
    @return 是否成功消费至少一项
    */
    bool consumeBatch(T* output, size_t maximum, size_t& consumed);

    /** 合并归还已提交前缀，并将异常前提交的项也计入自动维护预算
    @param [in] nodes 本消费者拥有的节点数组，不与其他消费调用重入
    @param [in] externalOwners 与 nodes 对应的块数组，普通载荷对应 nullptr
    @param [in] count 有效节点数，0 至 64
    */
    void releaseBatch(Node* const* nodes, Block* const* externalOwners, size_t count);

    /** 归还已知同一块的外部额度，并保持原有自动维护预算
    @param [in,out] owner 已取出 Entry 的共同归属块；必须处于 entry-only 模式
    @param [in] count 有效额度数，0 至 64
    */
    void releaseBatch(Block& owner, size_t count);

    /** 归还上下文缓存与登记槽；队列在调用结束前必须存活
    @param [in,out] record 本次操作独占的登记槽；借用至返回
    */
    void releaseProducer(Record& record);

    /** 按赋值异常保证选择 noexcept 移动或复制；不提交消费
    @param [out] output 接收值；pop 可为 nullptr，赋值抛异常时可能已被部分修改
    @param [in,out] source 队列仍拥有的载荷；仅在 noexcept 移动路径改变其内容
    @param [in] moveTag 编译期赋值策略标签
    */
    static void assign(T& output, T& source, std::true_type moveTag);

    /** 按赋值异常保证选择 noexcept 移动或复制；不提交消费
    @param [out] output 接收值；pop 可为 nullptr，赋值抛异常时可能已被部分修改
    @param [in,out] source 队列仍拥有的载荷；仅在 noexcept 移动路径改变其内容
    @param [in] copyTag 编译期赋值策略标签
    */
    static void assign(T& output, T& source, std::false_type copyTag);

    friend class LockFreeQueueProducer<T>;

private:
    // 唯一发布通道，固定池和动态池共用同一 FIFO 算法
    LockFreeQueueChannel<Node, SegmentPool, Record> m_channel;
    // 生命周期之间保留的发布方式；初始化后冻结，默认使用原 CAS 发布顺序
    bool m_cancelableReservations;
    // 生产者已准备发布项的模计数；失败回滚，与消费计数求差
    std::atomic<size_t> m_prepared;
    // 初始化后冻结；true 使用租约位置的私有累计预留
    bool m_distributedCounters;
    // 缓存行隔离填充，不承载业务状态
    char m_preparedPadding[64];
    // 唯一消费者自动维护的摊销计数
    size_t m_maintenance;
    // 唯一消费者的短前缀探测间隔；浅队列先逐项读取，定期重新尝试批量
    size_t m_prefixSingleBudget;
    // 缓存行隔离填充，不承载业务状态
    char m_consumedPadding[64];
    // 本队列拥有的统一节点池，destroy 释放其全部存储
    Arena m_arena;
};

#include "LockFreeQueue.inl"