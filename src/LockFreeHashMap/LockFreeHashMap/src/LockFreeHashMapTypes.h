#pragma once
#include "LockFreeHashMapPolicy.h"
#include <memory>
#include <exception>
#include <stdint.h>
#include <stddef.h>

/** BASE-LF-1.0 操作结果，只有 m_committed 表示本次数据修改已经提交
*/
enum LockFreeHashMapStatus
{
    // 查询成功或修改已经提交
    LockFreeHashMapOk = 0,
    // 条目已存在，insertIfAbsent 的输出为竞争胜者快照
    LockFreeHashMapExists,
    // 观察代次内没有该键
    LockFreeHashMapNotFound,
    // 实例身份或条目版本与期望不同
    LockFreeHashMapVersionMismatch,
    // 已保留与正在预留容量达到硬上限，当前没有可安全借用的槽位
    LockFreeHashMapCapacityLimit,
    // 竞争重试未完成；不表示容量、预留或系统内存不足
    LockFreeHashMapReservationBusy,
    // 扩容策略返回 0，拒绝本次增长
    LockFreeHashMapGrowthDeclined,
    // 系统内存申请失败
    LockFreeHashMapNoMemory,
    // 用户哈希、比较、复制或回调抛出异常
    LockFreeHashMapUserException,
    // 池配置或桶数量不合法
    LockFreeHashMapInvalidArgument,
    // 固定保护记录全部被借用，本次未接纳
    LockFreeHashMapProtectionUnavailable,
    // 单调身份空间耗尽，不允许回绕复用
    LockFreeHashMapVersionExhausted,
    // 尚未提交的操作被 clear 切换代次
    LockFreeHashMapCleared,
    // 所需原子后端非无锁或类型对齐不受支持
    LockFreeHashMapUnsupported,
    // 独立的固定块目录预算已满
    LockFreeHashMapDirectoryFull,
    // 配置池时表逻辑非空，调用方还必须保证外部静止
    LockFreeHashMapNotEmpty
};

/** 下标赋值失败的专用异常，不为保存错误码再次分配内存
*/
class LockFreeHashMapAssignmentError : public std::exception
{
public:
    /** 保存未提交赋值的错误码
    @param [in] status 未提交的具体原因
    */
    explicit LockFreeHashMapAssignmentError(LockFreeHashMapStatus status) throw();

    /** 返回失败原因
    @return 未提交的错误码
    */
    LockFreeHashMapStatus status() const throw();

    /** 返回固定消息，不执行内存申请
    @return 具有静态生命周期的错误消息
    */
    virtual const char* what() const throw();
private:
    // 本次下标赋值的失败原因
    LockFreeHashMapStatus m_status;
};

/** 单次调用结果，不使用跨线程共享的最后错误状态
*/
struct LockFreeHashMapResult
{
public:
    // 本次调用状态
    LockFreeHashMapStatus m_status;
    // 本次调用是否已改变逻辑表
    bool m_committed;
    // 遍历中正常完成的回调数，或维护中实际释放的槽位数
    size_t m_count;
public:
    /** 构造一次调用结果
    @param [in] status 状态码
    @param [in] committed 本次是否已提交数据修改
    @param [in] count 已完成回调数或释放槽位数，按具体接口解释
    */
    LockFreeHashMapResult(LockFreeHashMapStatus status = LockFreeHashMapOk, bool committed = false, size_t count = 0);
};

/** 由表和令牌共享所有权的实例身份，令牌存活时地址不会被复用
*/
struct LockFreeHashMapIdentity
{
    // 不使用进程全局计数或 DLL 私有登记，身份由对象地址区分
    unsigned char m_identity;
};

/** 与值同次观察取得的可复制版本，不允许调用方伪造有效令牌
*/
struct LockFreeHashMapVersion
{
public:
    // 令牌独立持有实例身份，允许比表本身活得更久
    std::shared_ptr<const LockFreeHashMapIdentity> m_instance;
    // 实例内永不重复的值身份，0 表示无效
    uint64_t m_sequence;
public:
    /** 构造无效令牌，不分配实例身份
    */
    LockFreeHashMapVersion();
};

/** 拥有独立值副本的快照，不借用池，也不要求值类型可默认构造
*/
template <typename ValueType>
struct LockFreeHashMapSnapshot
{
    // 不可变值副本，值若是 shared_ptr 则继续持有业务对象
    std::shared_ptr<const ValueType> m_value;
    // 在同一值受保护时取得的版本身份
    LockFreeHashMapVersion m_version;
};

/** 扩容策略的弱一致观测，容量单位为统一的键值槽位，保留共同契约字段名
*/
struct LockFreeHashMapGrowContext
{
    // 配置的初始槽位数，也是自动缩容下限
    size_t initialCapacity;
    // 初始化后累计成功发布的非零扩容批次数，饱和后不回绕
    uint64_t successfulGrowCount;
    // 已发布且仍保留的槽位数，不包含未发布预留
    size_t currentCapacity;
};

/** 缩容策略的弱一致观测，无增长历史时 hasSuccessfulGrow 为 false
*/
struct LockFreeHashMapShrinkContext
{
    // 配置的初始槽位数，也是自动缩容下限
    size_t initialCapacity;
    // 累计成功扩容批次数，缩容不减少此值
    uint64_t successfulGrowCount;
    // 已发布且仍保留的槽位数，包含等待释放的关闭块
    size_t currentCapacity;
    // 当前可复用空闲槽位的弱观测，不包含仍受保护的退役值
    size_t reusableFreeCapacity;
    // 是否曾成功发布非初始扩容批次
    bool hasSuccessfulGrow;
    // 距离上次成功扩容的单调毫秒数，无历史为 0
    uint64_t millisecondsSinceLastGrow;
};

/** 请求按整块缩容，idleMilliseconds 为 0 表示关闭自动缩容
*/
struct LockFreeHashMapShrinkDecision
{
public:
    // 希望释放的槽位数，库按安全可释放整块截断
    size_t shrinkCount;
    // 候选块必须连续空闲的毫秒数，0 不释放新候选块
    uint64_t idleMilliseconds;
public:
    /** 构造缩容请求
    @param [in] count 希望释放的槽位数，0 不缩容
    @param [in] idle 连续空闲毫秒数，0 关闭自动缩容
    */
    LockFreeHashMapShrinkDecision(size_t count = 0, uint64_t idle = 0);
};

/** 策略复制只共享句柄所有权，不复制用户回调对象
*/
typedef LockFreeHashMapPolicy<size_t, LockFreeHashMapGrowContext> LockFreeHashMapGrowPolicy;
typedef LockFreeHashMapPolicy<LockFreeHashMapShrinkDecision, LockFreeHashMapShrinkContext> LockFreeHashMapShrinkPolicy;

/** 仅外部静止时复制配置，回调可以并发执行，保留共同契约字段名
*/
struct LockFreeHashMapPoolConfig
{
public:
    // 初始槽位数，也是自动缩容下限
    size_t initialCapacity;
    // 成功申请后剩余空闲不高于此值才评估增长，0 表示用尽后下次申请才增长
    size_t growFreeThreshold;
    // 所有已保留和待申请预留的合并上限，-1 表示无配置上限，其余负值非法
    int64_t maximumCapacity;
    // 固定块目录数量，不是桶数，每块最多 65535 个槽位
    size_t directoryCapacity;
    // 同时操作和回调重入可借用的记录数，固定分配且无 TLS 登记
    size_t protectionRecordCount;
    // 类型和原子后端允许时启用不超过八字节的整数键值副本缓存
    bool inlineScalarValues;
    // 固定缓存格数量，必须是 2 的幂，0 关闭缓存
    size_t lookupCacheCapacity;
    // 扩容策略所有权句柄，空表示默认 max(64, 当前容量/2)
    LockFreeHashMapGrowPolicy growPolicy;
    // 缩容策略所有权句柄，空表示关闭自动缩容
    LockFreeHashMapShrinkPolicy shrinkPolicy;
public:
    /** 默认 64 槽、阈值 0、上限 -1、1024 块目录、128 保护记录，整数副本缓存 4096 格
    */
    LockFreeHashMapPoolConfig();
};

/** 弱一致资源观测，包含候选及退役占用，不等于逻辑条目数或进程 RSS
*/
struct LockFreeHashMapPoolStats
{
    // 已发布且尚未释放的槽位数
    size_t capacity;
    // 尚未完成发布的增长预留槽位数
    size_t reservedCapacity;
    // 单次原子观察的已保留容量与预留之和
    size_t budget;
    // 合并预算的历史高水位
    size_t peakBudget;
    // 位图及固定记录缓存中的可复用槽数；活动记录缓存由当前操作暂占
    size_t reusableFree;
    // 包含在 reusableFree 内，空闲记录中的槽可由其他操作转移，活动记录最多暂占四槽
    size_t cachedFree;
    // 候选、使用中和待回收占用槽位数
    size_t occupied;
    // 已关闭且等待读者退出后释放的空闲块容量，不计为可复用或占用槽位
    size_t closedCapacity;
    // 已退役或正在回收的槽位数
    size_t retired;
    // 使用中或已退役但尚未析构的键槽数
    size_t keyNodes;
    // 使用中或已退役但尚未析构的外置值版本槽数
    size_t valueVersions;
    // 保留的块数量，包含等待安全释放的关闭块
    size_t blockCount;
    // 固定保护记录总数量
    size_t protectionRecords;
    // 当前借用中的记录数，包含本次统计操作
    size_t activeProtectionRecords;
    // 结构性保留字节估算，含对象和元数据，不含键值内部另行申请及分配器额外开销
    size_t retainedBytes;
    // 当前是否实际分配了整数键值副本缓存
    bool inlineScalarValues;
    // 固定查找缓存格数量
    size_t lookupCacheEntries;
    // 固定副本数组的元素字节数，不含系统分配器开销
    size_t lookupCacheBytes;
    // 非初始批次成功发布次数，饱和后不回绕
    uint64_t successfulGrowCount;
    // 块及槽数组的系统申请尝试次数，不含固定元数据和键值内部申请
    uint64_t systemAllocationCount;
    // 保留诊断字段；当前未采样内部 CAS 重试，零不表示没有竞争
    uint64_t casRetries;
};

#include "LockFreeHashMapTypes.inl"