#pragma once
#include <atomic>
#include <stdint.h>
#include <stddef.h>

/** 固定登记记录，由一次操作独占，代际为零表示空闲
批次提示仅用于选择扫描起点，不拥有块或节点
*/
struct LockFreeHashMapEpochRecord
{
public:
    // 活动操作的保护代际；接纳 CAS 同时完成独占和保护发布
    std::atomic<uint64_t> m_epoch;
    // 上次成功申请的批次下标，不截断 size_t
    size_t m_blockHint;
    // 上次成功申请的位图字下标
    size_t m_wordHint;
    // 当前记录已发出的最后一个条目版本，记录独占期间访问
    uint64_t m_versionNext;
    // 当前记录已经预留的版本上界，池重新配置时未使用区间直接丢弃
    uint64_t m_versionEnd;
    // 当前记录累计写操作数，仅决定分摊回收时机，不作身份
    uint64_t m_operations;
    // 分片条目计数所属的逻辑代次，零为尚未使用
    std::atomic<uint64_t> m_countGeneration;
    // 当前代次内由本记录提交的插入减删除，可为负数
    std::atomic<int64_t> m_countDelta;
    // 固定四槽申请缓存，只有当前记录所有者可以读写地址；槽位始终占用所属块
    uintptr_t m_cachedSlots[4];
    // 已构造载荷为零的缓存槽数，统计只读取这个原子数，不读取地址数组
    std::atomic<size_t> m_cachedCount;
public:
    /** 构造空闲记录，不申请线程资源
    */
    LockFreeHashMapEpochRecord();
};

/** 表实例拥有的固定保护域，不保存线程指针，不依赖线程退出回调
读者一次登记保护整段遍历；暂停读者可延迟旧资源回收，但不阻塞其他操作退出
回收必须先冻结退休批次或代际上界，再扫描 minimum，不能把扫描后才退休的对象混入
*/
class LockFreeHashMapEpoch
{
public:
    /** 构造未初始化的保护域，初始代际为一
    */
    LockFreeHashMapEpoch();

    /** 外部静止时释放记录数组，不释放调用者的节点
    */
    ~LockFreeHashMapEpoch();

    /** 一次性分配固定数量的缓存行隔离记录
    @param [in] count 同时活动操作数量，必须大于零
    @return 参数、原子后端及分配均成功为 true；失败不留半初始化数组
    */
    bool initialize(size_t count);

    /** 查询本后端要求的原子是否真正无锁
    @return 六十四位代际原子无锁为 true
    */
    bool supported() const;

    /** 查询保护记录数量
    @return 初始化成功时固定数量，否则为零
    */
    size_t count() const;

    /** 获取当前退休代际上界，不推进时钟
    @return 当前非零代际，用作回收扫描开始前的严格上界
    */
    uint64_t current() const;

    /** 回收扫描开始前推进时钟，固定本轮允许回收的退役代际上界
    @return 推进前代际；到上限后饱和，不能回绕
    */
    uint64_t advance();

    /** 扫描活动登记的最小代际，不等待暂停线程
    @return 最小活动代际；零表示此次扫描没有观察到活动登记
    */
    uint64_t minimum() const;

    /** 返回指定下标记录，记录存活至保护域析构
    @param [in] index 小于 count 的固定下标
    @return 合法下标返回记录，否则为空
    */
    LockFreeHashMapEpochRecord* record(size_t index) const;

    /** 取得线程本地的纯数值扫描提示，不登记资源或实例身份
    @return 可取模的线程哈希提示，不参与所有权或 ABA 判断
    */
    static size_t hint();
private:
    /** 禁止共享记录数组的隐式拷贝
    @param [in] other 不可复制的源对象
    */
    LockFreeHashMapEpoch(const LockFreeHashMapEpoch& other);

    /** 禁止隐式转移记录数组所有权
    @param [in] other 不可赋值的源对象
    @return 不提供实现
    */
    LockFreeHashMapEpoch& operator=(const LockFreeHashMapEpoch& other);
private:
    // 非零且不回绕的实例内退休时钟
    std::atomic<uint64_t> m_clock;
    // 原始申请地址，用于正确匹配 delete[]
    unsigned char* m_allocation;
    // 手动对齐至缓存行的记录起点
    unsigned char* m_records;
    // 缓存行倍数的记录间距
    size_t m_stride;
    // 已完整构造的记录数量
    size_t m_count;
};

/** 一次非阻塞接纳保护整段操作，固定记录耗尽时立即返回未接纳
线程本地只缓存整数提示，不保存表地址，因此没有跨实例、DLL 或线程退出悬挂引用
*/
class LockFreeHashMapEpochGuard
{
public:
    /** 在至多一次完整数组扫描中尝试接纳，允许同线程回调重入
    @param [in] domain 调用期间存活的保护域，空值表示未接纳
    */
    explicit LockFreeHashMapEpochGuard(LockFreeHashMapEpoch* domain);

    /** 发布读区结束，返回记录；调用方此后不得继续解引用受保护资源
    */
    ~LockFreeHashMapEpochGuard();

    /** 查询独占记录，失败不会伪装成已取得保护
    @return 接纳成功的记录，或空值
    */
    LockFreeHashMapEpochRecord* record() const;
private:
    /** 禁止复制保护所有权
    @param [in] other 不可复制的保护对象
    */
    LockFreeHashMapEpochGuard(const LockFreeHashMapEpochGuard& other);

    /** 禁止覆盖尚未释放的保护
    @param [in] other 不可赋值的保护对象
    @return 不提供实现
    */
    LockFreeHashMapEpochGuard& operator=(const LockFreeHashMapEpochGuard& other);
private:
    // 唯一借用的记录，为空表示未接纳
    LockFreeHashMapEpochRecord* m_record;
};

#include "LockFreeHashMapEpoch.inl"