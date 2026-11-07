#pragma once
#include "CTaskStorageModels.h"
#include "CTaskReadDomain.h"
#include "CTaskThreadModels.h"
#include <functional>

/** 任务记录的位图内存池；同一记录不再另建 HashMap 键和值控制块
*/
class CTaskStorage
{
public:
    /** 借用生命周期更长的读侧保护域
    @param [in,out] domain 生产、观察及回收共同使用的保护域
    */
    explicit CTaskStorage(CTaskReadDomain& domain);

    /** 外部静止析构，释放全部任务引用和批次
    */
    ~CTaskStorage();

    /** 静止建立目录和初始槽位；失败后对象仍可安全析构
    @param [in] config 兼容任务记录池的容量及动态策略
    @return 初始化结果
    */
    CTaskThreadResult initialize(const LockFreeHashMapPoolConfig& config);

    /** 取得对应租约的缓存，最后一个位置仅归唯一消费者使用
    @param [in] index 小于等于保护域 capacity 的位置
    @return 对应独占缓存；调用方保证位置范围和所有权
    */
    CTaskStorageCache& cache(size_t index);

    /** 汇总各借用位置的累计任务预留，不写生产者缓存行
    @return 无符号累计量，允许自然回绕后作模差
    */
    uint64_t reservedCount() const;

    /** 汇总各位置的累计入队预留
    @param [in] level 0 为全部，1 至 16 为常用级别
    @return 无符号累计量
    */
    uint64_t publishedCount(int32_t level = 0) const;

    /** 在独占缓存和读侧保护下取得一个未发布槽；唯一消费者也可调用
    @param [in,out] cache 本次独占的缓存
    @param [out] result 失败原因，成功为 Ok
    @param [in] level 分配局部性提示，不参与调度或容量判定
    @return 独占槽位或 nullptr
    */
    CTaskStorageSlot* acquire(CTaskStorageCache& cache, CTaskThreadResult& result, int32_t level = 1);

    /** 归还从未发布的槽位，不执行退休等待
    @param [in,out] slot 独占槽位
    */
    void abandon(CTaskStorageSlot& slot);

    /** 唯一消费者退休已从观察入口逻辑摘除的槽
    @param [in,out] slot phase 已经为 3 的槽位
    */
    void retire(CTaskStorageSlot& slot);

    /** 唯一消费者有界回收，暂停读者只阻止对应记录的复用
    @param [in] budget 本次最多释放的槽位数量
    @return 实际释放数量
    */
    size_t collect(size_t budget);

    /** 归还独占缓存尚未使用的位，不访问已发出的任务
    @param [in,out] cache 持有其对应读侧位置或已经外部静止
    */
    void flush(CTaskStorageCache& cache);

    /** 有界借用空闲读侧位置，释放队列生产者上下文，不等待在途投递
    @return 实际释放的上下文数量；调用方自身持有的位置自动跳过
    */
    size_t releaseIdleProducers();

    /** 持有读侧保护时枚举已发布槽，回调内不得销毁本池
    @param [in] visit 接收排队或执行中的记录；其借用只在回调期间有效
    */
    void forEach(const std::function<void(CTaskStorageSlot&)>& visit);

    /** 唯一消费者评估缩容；空闲缓存应先由外部有界清理
    @return 维护结果，异常不越过接口
    */
    CTaskThreadResult maintain();

    /** 外部静止释放所有批次，保留目录以便析构
    */
    void clear();

    /** 查询合并已发布与在途容量预算
    @return 当前物理槽预算
    */
    size_t capacity() const;

private:
    /** 尝试从当前开放批次领取小批次可复用位
    @param [in,out] cache 本次独占缓存
    @param [in,out] allocation 本位置独占的分级游标
    @return 是否取得非零缓存位
    */
    bool scan(CTaskStorageCache& cache, CTaskStorageAllocation& allocation);

    /** 并发预留预算并私有构造批次，再一次发布
    @param [in] requested 期望新增数量，内部截断上限和单块大小
    @param [in] initial 是否属于初始容量准备
    @return 成功或具体失败状态
    */
    CTaskThreadResult grow(size_t requested, bool initial);

    /** 根据当前上下文评估动态扩容，不等待正在申请的其他生产者
    @return 增长结果
    */
    CTaskThreadResult evaluateGrowth();

    /** 不等待地借用空闲位置，归还其缓存供容量恢复和缩容
    */
    void flushIdleCaches();

    /** 在保护下计算策略所需容量和可复用槽弱观测
    @param [out] retained 已发布容量，不含在途分配
    @param [out] available 已发布可复用槽，含空闲缓存
    */
    void observe(size_t& retained, size_t& available) const;

    /** 唯一消费者合并发布本轮归还位及预算
    */
    void flushReturns();

    /** 在位置独占或外部静止时归还全部队列上下文
    @param [in,out] cache 本次独占的位置
    @return 释放数量
    */
    static size_t releaseProducers(CTaskStorageCache& cache);

    /** 统计非零位图最低位置
    @param [in] bits 非零位图
    @return 0 至 63 的下标
    */
    static size_t firstBit(uint64_t bits);

    /** 统计位图有效位数
    @param [in] bits 位图
    @return 0 至 64 的数量
    */
    static size_t bitCount(uint64_t bits);

    /** 获取缩容用单调毫秒时间
    @return 非零毫秒，失败返回 0
    */
    static uint64_t now();

private:
    // 借用保护域，生命周期长于本池
    CTaskReadDomain* m_domain;
    // 初始化后冻结的策略与预算
    LockFreeHashMapPoolConfig m_config;
    // 固定目录：0 空，1 申请中，偶数开放地址，奇数为已关闭地址
    std::atomic<uintptr_t>* m_directory;
    // 独立保存槽数，观察已关闭批次时不解引用即将回收的地址
    std::atomic<size_t>* m_counts;
    // 固定目录长度
    size_t m_directoryCount;
    // 保护位置数量加一个消费者专用缓存
    CTaskStorageCache* m_caches;
    // 曾使用目录上界
    std::atomic<size_t> m_high;
    // 已发布和在途申请统一预算
    std::atomic<size_t> m_budget;
    // 成功扩容次数
    std::atomic<uint64_t> m_grows;
    // 最近一次成功扩容的单调毫秒
    std::atomic<uint64_t> m_lastGrow;
    // 生产者读取的目录及缓存地址与消费者逐任务更新的退休链隔离
    unsigned char m_retirementPadding[64];
    // 本地退休窗口溢出后的链首，窗口中的记录先于此链回收
    CTaskStorageSlot* m_retiredHead;
    // 唯一消费者退休链尾
    CTaskStorageSlot* m_retiredTail;
    // 唯一消费者的有界退休窗口，常见路径不逐任务改写槽位的链字段
    CTaskStorageSlot* m_retiredWindow[1024];
    // 与窗口地址对应的退休 epoch，不从仍被生产者观察的槽位读取
    uint64_t m_retiredEpochs[1024];
    // 唯一消费者退休窗口的下一个读取位置
    size_t m_retiredRead;
    // 唯一消费者退休窗口内尚待回收的记录数
    size_t m_retiredCount;
    // 溢出链存在时继续追加链；窗口和链全部排空后恢复本地入窗
    bool m_retiredOverflow;
    // 唯一消费者待归还页链
    CTaskStoragePage* m_dirtyPages;
};