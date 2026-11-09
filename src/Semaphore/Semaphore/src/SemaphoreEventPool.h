#pragma once
#include "SemaphoreResult.h"
#include <atomic>
#include <functional>
#include <stddef.h>

class SemaphoreWake;

/** 扩容策略只计算新增记录数量，快照不包含未发布预留
*/
struct SemaphoreGrowContext
{
    // 配置中的初始记录数
    size_t initialCapacity;
    // 初始化之后成功提交的非零增长批次数
    uint64_t successfulGrowCount;
    // 已转为池所有权、尚未物理释放的记录容量快照
    size_t currentCapacity;
};

/** 缩容快照；没有成功扩容历史时hasLastGrow为false
*/
struct SemaphoreShrinkContext
{
    // 自动缩容的最低记录容量
    size_t initialCapacity;
    // 累计成功增长批次数，缩容不扣减
    uint64_t successfulGrowCount;
    // 池已持有容量快照
    size_t currentCapacity;
    // 当前可复用空闲记录数快照
    size_t freeCapacity;
    // 单调时间差，无增长历史时配合hasLastGrow判断
    uint64_t millisecondsSinceLastGrow;
    // 是否存在初始化之后的成功增长
    bool hasLastGrow;
};

/** 缩容决策；idleMilliseconds为0禁用自动缩容
*/
struct SemaphoreShrinkDecision
{
    // 本次希望释放的记录数
    size_t shrinkCount;
    // 候选须连续空闲的毫秒数；0禁用缩容
    uint64_t idleMilliseconds;
};

/** 等待记录配置；事件池与计数池分别设置，只在构造或外部静止期更换，回调可并发/重复评估
    回调必须短小、线程安全、不递归调用同一Semaphore；回调内部锁不属于库保证
*/
struct SemaphoreAPI SemaphorePoolConfig
{
public:
    // 默认16个记录，按对应计数池或事件池分别计量
    size_t initialCapacity;
    // 默认0：取走最后记录不扩容，下一次确实无记录时评估
    size_t freeThreshold;
    // 默认-1无配置上限；仍受size_t和分配失败限制
    int64_t maxCapacity;
#if defined(_MSC_VER) && !defined(STATIC_LIB)
    // 保留已交付的回调配置契约，只豁免这两个载体；跨DLL复制须匹配工具链和CRT
#pragma warning(push)
#pragma warning(disable : 4251)
#endif
    // 空回调使用默认max(16,current/2)增长，返回新增数
    std::function<size_t(const SemaphoreGrowContext&)> grow;
    // 空回调关闭自动缩容；实际回收按空闲时间及初始下限
    std::function<SemaphoreShrinkDecision(const SemaphoreShrinkContext&)> shrink;
#if defined(_MSC_VER) && !defined(STATIC_LIB)
#pragma warning(pop)
#endif

public:
    /** 初始化默认策略与容量
    */
    SemaphorePoolConfig();
};

/** 观测快照；并发字段不构成原子预算承诺
*/
struct SemaphorePoolStats
{
    // 使用中、空闲和退役容量之和的观测值
    size_t capacity;
    // 可立即复用记录的观测值
    size_t freeCapacity;
    // 已退出但尚待安全回收的记录数
    size_t retiredCapacity;
    // 申请已预留、尚未转交池所有权的额度
    size_t reservedCapacity;
    // 固定池对象加全部已保留/预留记录字节的保守预算
    size_t retainedBytes;
    // 成功增长历史，饱和不回绕
    uint64_t successfulGrowCount;
    // 最近扩容评估结果；提前增长失败不撤销已取得的等待记录
    SemaphoreStatus lastGrowStatus;
};

/** 内部记录。next低位标记已退出；只有安全回收后才可复用地址
*/
struct SemaphoreEventRecord
{
public:
    // 登记链，低位为删除标记
    std::atomic<uintptr_t> m_next;
    // 0等待，1已提示，2取消，3等待者已结束访问，4已预留物理释放，5退役大计数正文
    std::atomic<uint32_t> m_state;
    // 退役栈引用；最高位为重新入栈责任，低位为栈及临时借用引用
    std::atomic<uint32_t> m_retiredReferences;
    // 空闲/退役链只在相应独占阶段修改
    SemaphoreEventRecord* m_freeNext;
    // 与freeNext分开，旧空闲链读者可能仍借用freeNext
    SemaphoreEventRecord* m_retiredNext;
    // 连续安全空闲的起点
    uint64_t m_idleSince;

public:
    /** 初始化尚未发布记录
    */
    SemaphoreEventRecord();
};

/** 事件登记表和有预算的记录池，Harris删除标记与读者静止回收
    暂停扫描者只阻止退役记录复用，不阻止其他通知/登记；有限预算满则明确拒绝
*/
class SemaphoreEventPool
{
public:
    /** 准备初始容量，非法配置/资源失败抛异常并回滚
    @param [in] config 静止期配置副本
    */
    explicit SemaphoreEventPool(const SemaphorePoolConfig& config);

    /** 仅在所有等待者/通知者退出后销毁
    */
    ~SemaphoreEventPool();

    /** 分配并登记等待者，成功CAS到head是登记线性化点
    @param [out] record 成功后的独占等待记录
    @return OK或未登记失败
    */
    SemaphoreResult begin(SemaphoreEventRecord*& record);

    /** 超时与通知竞争后标记退出，状态3发布之前等待者仍拥有记录访问权
    @param [in] record 本次等待独占记录
    @return true通知先获胜，false超时先获胜
    */
    bool finish(SemaphoreEventRecord* record);

    /** 发布事件；广播固定扫描开始时的登记链，新登记只加在head之前
    @param [in] all 是否提示全部快照登记者
    @param [in] wake 非空时直接唤醒每个已提交记录；调用者已确认地址后端可用，指针仅本次借用
    @return 是否至少提示一位；不分配内存、不运行策略
    */
    SemaphoreResult notify(bool all, SemaphoreWake* wake = nullptr);

    /** 有界维护，最多检查64个缩容候选，两次回收合计最多尝试摘取64条退役记录
    CAS竞争重试和用户策略执行不承诺硬实时上限；没有其他读者时才实际复用或释放
    @param [out] released 实际释放记录数
    @return 维护结果，策略异常可观察
    */
    SemaphoreResult maintain(size_t& released);

    /** 取得池观测快照
    @return 包含退役、预留及元数据字节，不包含用户回调自身申请
    */
    SemaphorePoolStats statistics() const;

protected:
    // 内部计数等待复用以下分配与回收能力，不向业务对象开放。
    /** 在受保护的快照链中选择最早仍在等待的记录，不让新登记者越过旧等待者
    @param [in] head 本次通知开始时的链头
    @return 最早合格记录或空；调用者须保持读者保护
    */
    SemaphoreEventRecord* oldest(uintptr_t head) const;

    /** 保护链读取，计数上限时拒绝
    @return 是否成功进入
    */
    bool enter();

    /** 离开链读取保护，不等待其他参与者
    */
    void leave();

    /** 取得一个可复用记录，调用者已enter
    @return 记录或空
    */
    SemaphoreEventRecord* takeFree();

    /** 发布一串已经安全的空闲记录
    @param [in] first 链头
    @param [in] last 链尾
    */
    void putFree(SemaphoreEventRecord* first, SemaphoreEventRecord* last);

    /** 追加新退役记录，调用者持有其唯一摘链权，发布之后不再以摘链者身份访问
    @param [in] record 已摘除记录；状态小于3时等待者仍可能持有访问权
    */
    void retire(SemaphoreEventRecord* record);

    /** 将独占退役记录交回栈；旧借用者尚未退出时由最后一份引用帮助发布，不等待
    @param [in] record 已计入退役容量、当前不在退役栈中的记录
    */
    void queueRetired(SemaphoreEventRecord* record);

    /** 发布已经没有退役栈借用者的记录，成功前保留重新入栈责任
    @param [in] record 引用计数为零且已设置重新入栈标记的记录
    */
    void publishRetired(SemaphoreEventRecord* record);

    /** 在读者保护内摘取一条退役记录，借用引用保证next不被并发重写，避免栈ABA
    @param [in,out] budget 本次剩余摘取尝试数，每次观察候选都扣减
    @return 独占记录或空；其他线程仍可能持有只读借用，尚不能直接释放
    */
    SemaphoreEventRecord* takeRetired(size_t& budget);

    /** 帮助摘除已标记记录，调用者已enter
    */
    void sweep();

    /** 按预算摘取退役记录，不扫描剩余链尾，不等待读者；静止时才复用或释放
    @param [in,out] budget 剩余摘取尝试数，单批至多64条，调用方可在多次回收间共享预算
    @param [in] entered 是否转交调用者现有的读者保护；无论是否回收，本函数都会释放这份保护
    @return 本次物理释放记录数
    */
    size_t reclaim(size_t& budget, bool entered = false);

    /** 申请并完整发布一批记录，先原子预留总预算
    @param [in] count 新增记录数
    @param [in] initial 是否初始化批次
    @return 结果，失败无半发布批次
    */
    SemaphoreResult allocate(size_t count, bool initial);

    /** 每次需求只评估一次策略，异常转为POLICY
    @return 扩容结果
    */
    SemaphoreResult grow();

protected:
    // 策略在对象活动期只读
    SemaphorePoolConfig m_config;
    // 登记链入口，低位始终为0
    std::atomic<uintptr_t> m_head;
    // 已安全空闲链
    std::atomic<SemaphoreEventRecord*> m_free;
    // 已摘除、尚不安全的链
    std::atomic<SemaphoreEventRecord*> m_retired;
    // 共享链遍历前增加读者数，全部seq_cst；finish对自身记录另有状态3发布前的访问权
#ifdef _MSC_VER
    // 池也允许直接栈构造，显式对齐64位原子并使MSVC x86重对齐包含它的栈帧
    __declspec(align(8))
#endif
    std::atomic<uint64_t> m_readers;
    // 已分配和未发布预留的总预算
    std::atomic<size_t> m_allocated;
    // 已发布且尚未物理释放的容量
    std::atomic<size_t> m_capacity;
    // 安全可复用空闲数量
    std::atomic<size_t> m_freeCount;
    // 等待回收数量
    std::atomic<size_t> m_retiredCount;
    // 已发布容量减已预留缩容；独立原子预算防止并发释放使初始下限失守
    std::atomic<size_t> m_unreservedCapacity;
    // 提前增长失败也可通过统计观察
    std::atomic<uint32_t> m_lastGrowStatus;
    // 成功扩容次数，饱和不回绕
    std::atomic<uint64_t> m_grows;
    // 最近成功扩容时刻，grows为0表示无历史
    std::atomic<uint64_t> m_lastGrow;
};