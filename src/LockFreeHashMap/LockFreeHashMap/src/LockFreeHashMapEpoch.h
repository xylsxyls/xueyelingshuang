#pragma once
#include "LockFreeHashMapContention.h"
#include "LockFreeHashMapMacro.h"
#include <atomic>
#include <type_traits>
#include <stdint.h>
#include <stddef.h>
#include <memory>

class LockFreeHashMapEpoch;
class LockFreeHashMapReadGuard;
struct LockFreeHashMapIdentity;

/** 固定登记记录，由一次操作独占，代际为零表示空闲
批次提示仅用于选择扫描起点，不拥有块或节点
*/
struct LockFreeHashMapEpochRecord
{
public:
    // 活动操作的保护代际；接纳 CAS 同时完成独占和保护发布
    LOCKFREEHASHMAP_ALIGN_64 std::atomic<uint64_t> m_epoch;
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
    LOCKFREEHASHMAP_ALIGN_64 std::atomic<uint64_t> m_countGeneration;
    // 当前代次内由本记录提交的插入减删除，可为负数
    LOCKFREEHASHMAP_ALIGN_64 std::atomic<int64_t> m_countDelta;
    // 固定四槽申请缓存，只有当前记录所有者可以读写地址；槽位始终占用所属块
    uintptr_t m_cachedSlots[4];
    // 已构造载荷为零的缓存槽数，统计只读取这个原子数，不读取地址数组
    std::atomic<size_t> m_cachedCount;
    // 只有当前借用者读写的身份别名；最多一个，快照只共享其控制块，不借用本记录
    std::shared_ptr<const LockFreeHashMapIdentity> m_snapshotIdentity;
    // 已建立身份拥有者的载荷字节估算，供并发统计读取，不读取上面的非原子句柄
    std::atomic<size_t> m_snapshotBytes;

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

    /** 检查固定记录区及对齐余量的字节数能否用 size_t 表示
    @param [in] count 同时借用记录数，必须为正数
    @return 单次申请的长度合法时为 true
    */
    static bool validCount(size_t count);

    /** 查询本后端要求的原子是否真正无锁
    @return 代际、计数差量及槽数原子均无锁时为 true
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

    /** 记住本线程成功借用的下标，只更新扫描提示，不延长任何记录寿命
    @param [in] index 本次实际取得的记录下标，下一次仍须通过 CAS 接纳
    */
    static void remember(size_t index);

    /** 在至多一次完整记录扫描中接纳调用，普通保护和无回调保护共用此入口
    @param [in] rememberHint 是否把成功下标作为下一次扫描提示；内部辅助维护可保留调用者提示
    @return 独占且已经发布保护代际的记录；未初始化或本轮未找到空闲记录时为空
    */
    LockFreeHashMapEpochRecord* acquire(bool rememberHint = true) throw();

    /** 判断借用记录是否属于本域，只用于罕见容量等待前的同线程重入判断
    @param [in] candidate 可空记录地址，只比较地址范围，不解引用候选
    @return 地址正好对应本域某条记录时为true；未初始化的域返回false
    */
    bool owns(const LockFreeHashMapEpochRecord* candidate) const;

private:
    /** 首选记录被占用后扫描其余记录，隔离不常执行的循环，避免扩大每个调用点
    @param [in] index 已经尝试过的合法首选下标
    @param [in] epoch 本次接纳开始时读取的非零保护代际
    @param [in] rememberHint 是否记住本次成功下标，不影响记录的独占和回收保护
    @return 本轮取得的独占记录，否则为空；不重复尝试首选记录
    */
    LockFreeHashMapEpochRecord* acquireRemaining(size_t index, uint64_t epoch, bool rememberHint) throw();

    /** 当前线程的纯数值扫描提示，首次初始化前保存size_t最大值
    @return 无所有权的提示存储，不保存实例或记录地址
    */
    static size_t& hintSlot();

    /** 首次使用时计算当前线程的数值提示，将异常处理留在初始化路径
    @return 计算所得提示；线程标识哈希失败时使用零，不持有任何资源
    */
    static size_t initializeHint() throw();

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
    // 记录类型在编译期确定，间距向上取整至缓存行，无需每次借用再读取实例字段。
    static const size_t kRecordStride = (sizeof(LockFreeHashMapEpochRecord) + 63) & ~size_t(63);
    // 非零且不回绕的实例内退休时钟
    LOCKFREEHASHMAP_ALIGN_64 std::atomic<uint64_t> m_clock;
    // 原始申请地址，用于正确匹配 delete[]
    unsigned char* m_allocation;
    // 手动对齐至缓存行的记录起点
    unsigned char* m_records;
    // 已完整构造的记录数量
    size_t m_count;
};

/** 一次非阻塞接纳保护整段操作，固定记录耗尽时立即返回未接纳
线程局部链仅保存尚在调用栈上的保护对象，退出保护时解除链接，不登记线程退出资源
*/
class LockFreeHashMapEpochGuard
{
public:
    /** 在至多一次完整数组扫描中尝试接纳，允许同线程回调重入
    @param [in] domain 调用期间存活的保护域，空值表示未接纳
    @param [in] trackThread 为 false 仍完整保护资源，但执行任何用户代码之前必须调用 track
    @param [in] rememberHint 为 false 只是不记住本次接纳下标，适用于嵌套的内部维护
    */
    explicit LockFreeHashMapEpochGuard(LockFreeHashMapEpoch* domain, bool trackThread = true, bool rememberHint = true) throw();

    /** 将只读保护转换成可执行回调的保护，原记录持续占用，不重新接纳
    @param [in,out] source 同线程的只读保护，转换后不再拥有记录
    */
    explicit LockFreeHashMapEpochGuard(LockFreeHashMapReadGuard& source) throw();

    /** 发布读区结束，返回记录；调用方此后不得继续解引用受保护资源
    */
    ~LockFreeHashMapEpochGuard() throw();

    /** 查询独占记录，失败不会伪装成已取得保护
    @return 接纳成功的记录，或空值
    */
    LockFreeHashMapEpochRecord* record() const;

    /** 受保护数据已经转成独立所有权时提前退出，后续不得再解引用节点
    重复调用及未接纳时无副作用，析构仍可正常执行；同线程内已登记保护须按嵌套顺序退出
    */
    void release() throw();

    /** 在即将执行用户代码前登记重入链，重复调用无副作用，不分配资源
    接纳失败时不登记；调用及析构必须在构造保护对象的同一线程
    */
    void track() throw();

    /** 检查容量等待是否依赖当前线程退出外层保护
    @param [in] domain 本次操作的保护域
    @return 本线程仍持有同域外层记录时为true
    */
    static bool heldByCurrentThread(const LockFreeHashMapEpoch* domain);

private:
    /** 当前线程活动保护链，只有本线程访问且不分配内存
    @return 当前线程链头的引用
    */
    static LockFreeHashMapEpochGuard*& current();

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
    // 本线程调用栈的上一层保护，退出时恢复，避免为重入检测写入池记录缓存行
    LockFreeHashMapEpochGuard* m_previous;
    // 只借用当前线程的链头槽地址，保护不可跨线程转移，析构无需重新查找 TLS。
    LockFreeHashMapEpochGuard** m_threadHead;
};

/** 无用户回调的内部读取保护，仅持有记录，不能用于分配器、用户复制或删除器
使用同一保护域和接纳协议；省去不需要的重入链字段，不改变回收时机
*/
class LockFreeHashMapReadGuard
{
public:
    /** 尝试借用一条完整保护记录，不登记回调重入链
    @param [in] domain 调用期间存活的保护域，可空；本保护必须在该域析构前退出
    */
    explicit LockFreeHashMapReadGuard(LockFreeHashMapEpoch* domain) throw();

    /** 为共享所有权读取内联接纳，保持同一保护协议
    @param [in] domain 调用期间存活的保护域，可空
    @param [in] inlineAdmission 编译期选择接纳路径，不改变接纳和退出语义
    */
    LockFreeHashMapReadGuard(LockFreeHashMapEpoch* domain, std::true_type inlineAdmission) throw();

    /** 退出读区并归还记录，析构后不得继续访问借用节点
    */
    ~LockFreeHashMapReadGuard() throw();

    /** 查询本次独占记录，不转移所有权
    @return 接纳成功的记录，否则为空
    */
    LockFreeHashMapEpochRecord* record() const;

    /** 独立结果已准备完成时提前退出保护，后续不能继续解引用节点
    重复调用及未接纳时无副作用；必须在取得保护的线程调用
    */
    void release() throw();

    /** 将记录交给同线程的普通保护构造函数，不发布退出，不能单独丢弃返回值
    @return 原借用记录；本对象立即变空，由接收保护负责最终归还
    */
    LockFreeHashMapEpochRecord* transfer() throw();

private:
    /** 独立完成接纳计算，只返回借用记录，不取得调用方栈对象的地址
    @param [in] domain 借用的保护域，允许为空；非空时须覆盖整个保护期
    @return 成功借用的记录，或空值；返回非空后由保护对象唯一归还
    */
    static LockFreeHashMapEpochRecord* acquireRecord(LockFreeHashMapEpoch* domain) throw();

    /** 禁止复制保护所有权
    @param [in] other 不可复制的源对象
    */
    LockFreeHashMapReadGuard(const LockFreeHashMapReadGuard& other);

    /** 禁止覆盖尚未释放的保护
    @param [in] other 不可赋值的源对象
    @return 不提供实现
    */
    LockFreeHashMapReadGuard& operator=(const LockFreeHashMapReadGuard& other);
private:
    // 唯一借用的记录，不含线程链或实例所有权
    LockFreeHashMapEpochRecord* m_record;
};

#include "LockFreeHashMapEpoch.inl"