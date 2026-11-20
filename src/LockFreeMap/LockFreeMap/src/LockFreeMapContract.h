#pragma once
#include <stddef.h>
#include <stdint.h>
#include <functional>
#include <memory>
#include <new>
#include <exception>

/** BASE-LF-1.0 条目操作结果，只有 COMMITTED 表示写入已提交
*/
enum LockFreeMapStatus
{
    // 查询、配置或维护成功完成
    LOCK_FREE_MAP_OK = 0,
    // 根指针或值指针 CAS 已提交本次变更
    LOCK_FREE_MAP_COMMITTED = 1,
    // 插入遇到已存在的胜出条目
    LOCK_FREE_MAP_EXISTS = 2,
    // 不存在匹配条目
    LOCK_FREE_MAP_NOT_FOUND = 3,
    // 条件凭据的实例、条目版本或最大项条件已失效；普通接口内部重试 CAS 竞争
    LOCK_FREE_MAP_CONFLICT = 4,
    // 已无可复用槽位，且保留容量已达到上限
    LOCK_FREE_MAP_CAPACITY = 5,
    // 已无可复用槽位，且剩余容量额度已由其他并发申请预留
    LOCK_FREE_MAP_RESERVED = 6,
    // 扩容策略返回零，拒绝本次增长
    LOCK_FREE_MAP_GROW_DENIED = 7,
    // 用户扩容或缩容策略抛出异常
    LOCK_FREE_MAP_POLICY_EXCEPTION = 8,
    // 系统内存申请失败
    LOCK_FREE_MAP_ALLOCATION_FAILURE = 9,
    // 键值构造、复制、比较或回调抛出异常
    LOCK_FREE_MAP_USER_EXCEPTION = 10,
    // 配置无效或实例尚未初始化；无效条件凭据由CONFLICT报告
    LOCK_FREE_MAP_INVALID = 11,
    // 有界操作记录、索引记录或块记录全部占用
    LOCK_FREE_MAP_RECORD_LIMIT = 12,
    // 当前目标不支持所需原子类型的无锁实现
    LOCK_FREE_MAP_UNSUPPORTED = 13,
    // 版本计数达到不允许回绕的上限
    LOCK_FREE_MAP_VERSION_EXHAUSTED = 14,
    // 仅try接口：未提交尝试被并发发布冲突打断，不表示空间不足
    LOCK_FREE_MAP_BUSY = 15,
    // 容量等待的系统休眠失败
    LOCK_FREE_MAP_SYSTEM_ERROR = 16
};

/** 旧赋值运算符无法返回状态，以异常报告未提交；新代码优先使用有结果接口
*/
class LockFreeMapError : public std::exception
{
public:
    // 导致赋值未提交的原始状态
    LockFreeMapStatus m_status;

public:
    /** 保存错误状态，不额外申请错误消息内存
    @param [in] status 未提交的失败状态
    */
    explicit LockFreeMapError(LockFreeMapStatus status);

    /** 说明旧赋值接口未提交
    @return 静态消息，具体原因由 m_status 给出
    */
    virtual const char* what() const throw();
};

/** 实例身份；凭据仅持有此小对象，不延长键节点或值版本的生命期
*/
struct LockFreeMapIdentity
{
    // 与共享控制块合并分配时，冷数据隔开引用计数和后续分配的索引头
    // 只影响存储开销，不依赖缓存行大小保证并发正确性
    unsigned char m_padding[64];
};

/** 回收后仍可比较的凭据，仅对原实例及版本有效
*/
struct LockFreeMapToken
{
public:
    // 普通复制的身份共享引用，不使用 atomic shared_ptr
    std::shared_ptr<const LockFreeMapIdentity> m_identity;
    // 零表示凭据无效
    uint64_t m_version;

public:
    /** 构造无效凭据
    */
    LockFreeMapToken();
};

/** 扩容观察上下文，不代表容量预留
*/
struct LockFreeMapGrowContext
{
    // 初始槽位容量，也是自动缩容下限
    size_t initialCapacity;
    // 初始化后成功发布批次的饱和累计数
    uint64_t successfulGrowCount;
    // 已发布且仍保留的槽位数，不含扩容预留
    size_t currentCapacity;
};

/** 缩容观察；仅 hasSuccessfulGrow 为真时存在成功扩容历史
*/
struct LockFreeMapShrinkContext
{
    // 初始槽位容量，也是自动缩容下限
    size_t initialCapacity;
    // 初始化后成功发布批次的饱和累计数
    uint64_t successfulGrowCount;
    // 已发布且仍保留的槽位数，不含扩容预留
    size_t currentCapacity;
    // 当前可复用槽位观察值，不含已关闭的退役块
    size_t reusableCapacity;
    // 初始化后是否发布过扩容批次
    bool hasSuccessfulGrow;
    // 距上次增长的单调毫秒数，时间不可用时为零
    uint64_t millisecondsSinceLastGrow;
};

/** 期望释放容量及连续空闲时间，时间为零表示关闭自动缩容
*/
struct LockFreeMapShrinkDecision
{
public:
    // 期望按整块释放的槽位数
    size_t shrinkCount;
    // 所需连续空闲毫秒数，零表示禁用自动缩容
    uint64_t idleMilliseconds;

public:
    /** 构造一次缩容决策
    @param [in] count 期望释放槽位数
    @param [in] milliseconds 连续空闲毫秒数，零表示禁用
    */
    LockFreeMapShrinkDecision(size_t count = 0, uint64_t milliseconds = 0);
};

/** 外部静止期配置一个池；策略在运行期可能并发调用
*/
struct LockFreeMapPoolConfig
{
public:
    // 初始槽位容量，也是自动缩容下限
    size_t initialCapacity;
    // 零仅在实际取槽失败后增长，正数允许预扩容
    size_t growFreeThreshold;
    // 保留加预留槽位上限，-1 表示不设置配置上限
    int64_t maximumCapacity;
    // 私有、活动及退役块共用的固定目录上限
    size_t blockRecordLimit;
    // 返回新增槽数的线程安全策略，可能并发或重复评估
    std::function<size_t(const LockFreeMapGrowContext&)> grow;
    // 返回期望释放容量与空闲时间的线程安全策略
    std::function<LockFreeMapShrinkDecision(const LockFreeMapShrinkContext&)> shrink;
    // 实际发布新增槽数及成功计数快照；不含初始化，可并发，不可重入所属Map，异常忽略
    std::function<void(size_t, uint64_t)> m_onGrowth;

public:
    /** 默认初始 16、阈值 0、上限 -1、块记录 256，按当前容量倍增
    */
    LockFreeMapPoolConfig();

    /** 设置初始槽数及保留容量上限，初始容量也是自动缩容下限
    @param [in] initial 初始槽数，允许零
    @param [in] maximum 槽数上限，-1表示由地址空间和块目录约束
    @return OK已保存，参数不合法返回INVALID且保留原设置，配置时另行验证目标可分配范围
    */
    LockFreeMapStatus setCapacity(size_t initial, int64_t maximum);

    /** 设置剩余空槽触发预增长的阈值
    @param [in] threshold 以槽为单位，零表示只在取槽失败后增长
    */
    void setGrowFreeThreshold(size_t threshold);

    /** 设置池块目录的固定记录数
    @param [in] count 可同时管理的块数，必须为正
    @return OK已保存，参数不合法返回INVALID且保留原设置，配置时另行验证目标可分配范围
    */
    LockFreeMapStatus setBlockRecordLimit(size_t count);

    /** 保存增长策略副本
    @param [in] policy 接受增长上下文并返回新增槽数，空策略拒绝增长
    @return OK已保存，复制失败返回ALLOCATION_FAILURE或USER_EXCEPTION，原策略保留
    */
    template <typename Function>
    LockFreeMapStatus setGrow(const Function& policy);

    /** 保存缩容策略副本
    @param [in] policy 接受缩容上下文并返回整块释放决策，空策略关闭缩容
    @return OK已保存，复制失败返回ALLOCATION_FAILURE或USER_EXCEPTION，原策略保留
    */
    template <typename Function>
    LockFreeMapStatus setShrink(const Function& policy);

    /** 保存成功扩容后的通知副本
    @param [in] policy 接受实际新增槽数和成功增长计数，空回调关闭通知
    @return OK已保存，复制失败返回ALLOCATION_FAILURE或USER_EXCEPTION，原策略保留
    */
    template <typename Function>
    LockFreeMapStatus setOnGrowth(const Function& policy);

    /** 默认策略，永久关闭自动整块释放
    @param [in] context 本次观察，此固定策略不使用上下文
    @return 释放数量及空闲时间均为零
    */
    static LockFreeMapShrinkDecision defaultShrink(const LockFreeMapShrinkContext& context);

private:
    /** 在临时函数对象中完成复制，再替换配置中的回调
    @param [in,out] target 待更新的配置项
    @param [in] policy 同步复制的可调用对象或空指针
    @return OK表示成功，申请失败及其他复制异常分别转换为状态
    */
    template <typename Target, typename Function>
    static LockFreeMapStatus copyPolicy(Target& target, const Function& policy);
};

/** 静止期配置，键槽和值槽分别预算
*/
struct LockFreeMapConfig
{
public:
    // 独立于值版本池的键对象池
    LockFreeMapPoolConfig nodes;
    // 值版本池，预算包含未提交候选及待回收旧值
    LockFreeMapPoolConfig values;
    // 同时使用保护记录的并发及重入调用上限
    size_t operationRecordLimit;
    // 每次池维护预算；结构写入另计索引大小的回收工作
    size_t maintenanceBudget;
    // 每条操作记录触发周期维护的写入尝试间隔，首次写入计为一
    uint32_t maintenanceInterval;
    // 显式等待容量时，首次休眠前允许的快速重试数
    uint32_t capacityFastRetries;
    // 快速重试耗尽后的每次休眠毫秒数，必须为正
    int32_t capacitySleepMilliseconds;

public:
    /** 默认 64 个并发调用，每条记录每 64 次写入尝试触发周期维护，每次预算 64 项
    */
    LockFreeMapConfig();

    /** 设置并发及同步重入可占用的操作记录数
    @param [in] count 固定记录数，必须为正
    @return OK已保存，参数不合法返回INVALID且保留原设置，配置时另行验证目标可分配范围
    */
    LockFreeMapStatus setOperationRecordLimit(size_t count);

    /** 设置写入触发的每次维护预算
    @param [in] count 索引及各池分别使用的正预算，不是整次调用的析构上限
    @return OK已保存，参数不合法返回INVALID且保留原设置，配置时另行验证目标可分配范围
    */
    LockFreeMapStatus setMaintenanceBudget(size_t count);

    /** 设置周期维护间隔；结构变更和容量压力仍可提前触发维护
    @param [in] attempts 每条操作记录的写入尝试次数，重新领取记录的重试计数，同一记录内的值 CAS 重试不另计，必须为正
    @return OK 表示已保存，零返回 INVALID 并保留原设置
    */
    LockFreeMapStatus setMaintenanceInterval(uint32_t attempts);

    /** 设置启用容量等待时的重试节奏，默认快速重试 4096 次，之后每次休眠 1 毫秒
    @param [in] fastRetries 首次休眠前的重试次数，零表示直接休眠
    @param [in] sleepMilliseconds 每次休眠的正毫秒数
    @return OK 表示已保存，休眠时间非正时返回 INVALID 并保留两个原设置
    */
    LockFreeMapStatus setCapacityWait(uint32_t fastRetries, int32_t sleepMilliseconds);
};

/** 池统计为各项独立原子观察，不提供容量预留
*/
struct LockFreeMapPoolStats
{
    // 已发布保留槽位，包含等待释放的关闭块
    size_t capacity;
    // 尚未计入已发布容量观察值的预留额度
    size_t reserved;
    // 已构造且尚未析构的对象数，不是业务元素数
    size_t retained;
    // 开放容量减去占用槽数，观察不一致时最低取零
    size_t reusable;
    // 块和槽申请字节，不包含分配器及用户对象内部堆开销
    size_t bytes;
    // 初始化后成功发布批次的饱和累计数
    uint64_t successfulGrowCount;
    // 成功块及槽数组申请次数，测试另计索引数组申请
    uint64_t systemAllocations;
};

/** 双池以外的索引存储及有界元数据单独计入统计
*/
struct LockFreeMapStats
{
    // 独立于值版本池的键对象池
    LockFreeMapPoolStats nodes;
    // 值版本池，预算包含未提交候选及待回收旧值
    LockFreeMapPoolStats values;
    // 全部保留引用数组的申请字节
    size_t indexBytes;
    // 固定结构及登记表字节，不含分配器和 shared_ptr 控制块开销
    size_t metadataBytes;
    // 本库观测组件字节的饱和合计，不是进程内存
    size_t totalBytes;
    // 当前占用保护记录数的观察值
    size_t activeOperations;
    // 根或值 CAS 冲突次数的饱和累计值
    uint64_t casRetries;
};

#include "LockFreeMapContract.inl"