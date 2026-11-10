#pragma once
#include "LockFreeHashMapMacro.h"
#include "HashMapValueProxy.h"
#include "HashMapNode.h"
#include "LockFreeHashMapPool.h"
#include "LockFreeHashMapValueCache.h"
#include <type_traits>
#include <utility>
#include <vector>

/** 固定桶并发哈希表，拥有节点池，不保证键排序；数据操作可与 clear 并发，析构和配置须外部静止，键值析构不得抛异常
哈希器与比较器必须支持并发 const 调用，等价键必须得到相同哈希值，已发布键的身份不得改变
*/
template <typename KeyType, typename ValueType, typename HashType = std::hash<KeyType>, typename EqualType = std::equal_to<KeyType>>
class LockFreeHashMap
{
public:
    typedef LockFreeHashMapSnapshot<ValueType> Snapshot;
    typedef LockFreeHashMapNode<KeyType, ValueType> Node;
    typedef LockFreeHashMapPoolSlot<Node> Slot;
    typedef LockFreeHashMapPool<Node> Storage;

    /** 创建桶表和默认池，内部失败由 status 查询；用户哈希器及比较器构造异常由调用方处理
    @param [in] bucketCount 桶数量，0 使用 1024，其余向上取整为 2 的幂
    */
    explicit LockFreeHashMap(size_t bucketCount = 1024);

    /** 直接按指定参数建立节点池，避免先构造默认池再替换；失败由 status 查询
    @param [in] bucketCount 桶数量，0 使用 1024，其余向上取整为 2 的幂
    @param [in] config 池容量和策略，构造期间复制，规则与 configurePool 一致
    */
    LockFreeHashMap(size_t bucketCount, const LockFreeHashMapPoolConfig& config);

    /** 外部停止所有操作和回调后销毁资源，独立快照可以继续存活
    */
    ~LockFreeHashMap();

    /** 查询构造时的初始化结果
    @return 初始化状态，Ok 表示可用
    */
    LockFreeHashMapStatus status() const;

    /** 外部静止且逻辑为空时替换池，失败保留原池
    @param [in] config 复制容量和策略所有权句柄，不借用配置对象
    @return 配置状态，m_committed 为 false，不修改逻辑数据
    */
    LockFreeHashMapResult configurePool(const LockFreeHashMapPoolConfig& config);

    /** 外部静止时替换策略所有权句柄，不复制用户回调对象
    @param [in] grow 扩容策略，空句柄恢复默认算法
    @param [in] shrink 缩容策略，空句柄关闭自动缩容
    @return 配置状态，m_committed 为 false
    */
    LockFreeHashMapResult setPoolPolicies(const LockFreeHashMapGrowPolicy& grow,
        const LockFreeHashMapShrinkPolicy& shrink);

    /** 同次观察取得独立值副本和条目版本，失败保持输出不变
    @param [in] key 调用期间借用的键
    @param [out] output 成功时写入独立快照，失败保持原值
    @return Ok、NotFound 或明确失败原因，不修改表
    */
    LockFreeHashMapResult find(const KeyType& key, Snapshot& output) const;

    /** 仅当键不存在时发布完整候选，Exists 时返回竞争胜者，其他失败保持输出不变
    @param [in] key 调用期间借用的键
    @param [in] value 待复制的值，调用期间借用
    @param [out] output 提交时返回新值，Exists 时返回胜者，其余失败保持原值
    @return m_committed 表示插入，Exists 表示已有条目，其余为失败原因
    */
    LockFreeHashMapResult insertIfAbsent(const KeyType& key, const ValueType& value, Snapshot& output);

    /** 仅替换同一实例的指定条目版本，提交前准备可选快照，失败保持输出不变
    @param [in] key 调用期间借用的键
    @param [in] version 与值同次读取的同一实例条目身份，不可伪造
    @param [in] value 待复制的值，调用期间借用
    @param [out] output 可为空以跳过快照，提交前准备，失败保持原值
    @return m_committed 表示替换成功，过期版本返回 VersionMismatch
    */
    LockFreeHashMapResult compareExchange(const KeyType& key, const LockFreeHashMapVersion& version,
        const ValueType& value, Snapshot* output = nullptr);

    /** 按指定条目版本删除，不申请池槽位，不误删后来版本
    @param [in] key 调用期间借用的键
    @param [in] version 与值同次读取的同一实例条目身份，不可伪造
    @return m_committed 表示删除成功，失败不删除其他版本
    */
    LockFreeHashMapResult eraseIf(const KeyType& key, const LockFreeHashMapVersion& version);

    /** 删除当前条目并取得独立副本，复制失败时原条目和输出不变
    @param [in] key 调用期间借用的键
    @param [out] output 成功时写入独立快照，失败保持原值
    @return m_committed 表示取走条目，失败保留原条目
    */
    LockFreeHashMapResult take(const KeyType& key, Snapshot& output);

    /** 插入或替换完整值，提交后不伪报可重试失败，失败不消费输入所有权
    @param [in] key 调用期间借用的键
    @param [in] value 待复制的值，调用期间借用
    @param [out] output 可为空以跳过快照，提交前准备，失败保持原值
    @return m_committed 表示本次写入，失败不消费输入所有权
    */
    LockFreeHashMapResult trySet(const KeyType& key, const ValueType& value, Snapshot* output = nullptr);

    /** 删除当前条目，不申请池槽位，不存在时返回 NotFound
    @param [in] key 调用期间借用的键
    @return 删除成功时 m_committed 为 true，不存在为 NotFound
    */
    LockFreeHashMapResult tryErase(const KeyType& key);

    /** 原子切换可见代次；耗尽时仍清空并永久拒绝后续写入
    @return m_committed 表示清空已生效，代次耗尽同时返回 VersionExhausted
    */
    LockFreeHashMapResult tryClear();

    /** 由唯一外部维护者分摊回收，可与数据操作并发，摘除旧桶时处理整个旧链
    @param [in] workBudget 清理桶数和回收位图字数的预算，零不维护；整条标记前缀不按节点分割，缩容另扫描固定目录与记录
    @return 维护状态及整块实际释放的槽位数 m_count
    */
    LockFreeHashMapResult maintain(size_t workBudget = 64);

    /** 取得池的弱一致观测，查询失败保持输出不变
    @param [out] output 成功时写入统计，失败保持原值
    @return 查询状态，并发字段不构成事务快照
    */
    LockFreeHashMapResult poolStats(LockFreeHashMapPoolStats& output) const;

    /** 创建借用本表的下标代理，表须存活到代理最后使用；键复制异常由调用方处理
    @param [in] key 调用期间借用的键
    @return 借用本表的代理，赋值未提交时通过 LockFreeHashMapAssignmentError 提供失败状态
    */
    HashMapValueProxy<KeyType, ValueType, HashType, EqualType> operator[](const KeyType& key);

    /** 兼容写入包装，详细失败原因使用 trySet
    @param [in] key 调用期间借用的键
    @param [in] value 待复制的值，调用期间借用
    @return 仅本次提交成功为 true
    */
    bool set(const KeyType& key, const ValueType& value);

    /** 兼容仅插入包装，已存在或失败均返回 false
    @param [in] key 调用期间借用的键
    @param [in] value 待复制的值，调用期间借用
    @return 仅本次提交成功为 true
    */
    bool insert(const KeyType& key, const ValueType& value);

    /** 兼容删除包装
    @param [in] key 调用期间借用的键
    @return 仅本次提交成功为 true
    */
    bool erase(const KeyType& key);

    /** 提供 erase 的兼容别名
    @param [in] key 调用期间借用的键
    @return 仅本次提交成功为 true
    */
    bool remove(const KeyType& key);

    /** 保护当前版本并赋值，用户赋值抛异常可能部分修改输出
    @param [in] key 调用期间借用的键
    @param [out] value 找到时赋值，用户赋值异常可能部分修改输出
    @return 查找并赋值成功为 true，异常或不存在为 false
    */
    bool get(const KeyType& key, ValueType& value) const;

    /** 取得值或默认值副本，默认值及返回值构造异常由调用方处理
    @param [in] key 调用期间借用的键
    @param [in] defaultValue 未找到时使用的默认值
    @return 值或默认值副本，用户赋值失败可能保留部分修改结果；默认值构造和返回值复制异常由调用者处理
    */
    ValueType value(const KeyType& key, const ValueType& defaultValue = ValueType()) const;

    /** 查询键是否存在，不复制值；兼容接口将查询失败映射为 false
    @param [in] key 调用期间借用的键
    @return 找到为 true，不存在及查询失败为 false
    */
    bool contains(const KeyType& key) const;

    /** 查询单键存在数量，失败与不存在均映射为 0
    @param [in] key 调用期间借用的键
    @return 弱一致条目数量，单键查询为 0 或 1
    */
    size_t count(const KeyType& key) const;

    /** 弱一致地统计条目数量，外部静止时准确，紧凑计数失效后回退遍历
    @return 非负条目观察值，并发记账未完成或遍历失败时可能不完整
    */
    size_t count() const;

    /** 提供 count 的兼容别名
    @return 弱一致条目数量
    */
    size_t size() const;

    /** 弱一致地观察表是否为空，不构成写入预留
    @return count 为 0 时为 true
    */
    bool empty() const;

    /** 兼容清空入口，代次耗尽后保持空表并拒绝写入
    */
    void clear();

    /** 外部静止时清空并释放全部批次，保留元数据和配置，之后可重新增长
    */
    void clearAndFreeMemory();

    /** 遍历安全借用，回调可以重入，借用不得带出回调
    @param [in] function 接收仅在回调期间有效的 const KeyType& 和 const ValueType&
    @return 已正常返回的回调次数
    */
    template <typename FunctionType>
    size_t forEach(const FunctionType& function) const;

    /** 转发遍历回调，保留临时对象及只可移动回调的兼容性
    @param [in,out] function 左值在内部复制，右值在内部移动，移动后的对象不应继续使用
    @return 已正常返回的回调次数，准备或执行失败返回已完成数量
    */
    template <typename FunctionType>
    size_t forEach(FunctionType&& function) const;

    /** 单次向前弱遍历，失效链终止该桶，已完成回调不重放；回调复制和执行异常均在内部捕获
    @param [in] function 接收仅在回调期间有效的 const KeyType& 和 const ValueType&
    @return 状态及已正常完成的回调数 m_count，回调异常被捕获
    */
    template <typename FunctionType>
    LockFreeHashMapResult forEachResult(const FunctionType& function) const;

    /** 在内部异常边界复制或移动回调，再执行单次向前弱遍历
    @param [in,out] function 左值保持原对象，右值允许转移；引用参数仅在回调期间借用
    @return 状态及已完成回调数，复制、移动及调用异常均被转换
    */
    template <typename FunctionType>
    LockFreeHashMapResult forEachResult(FunctionType&& function) const;

    /** 复制弱一致键集合，遍历失败清空结果，返回向量构造异常由调用方处理
    @return 独立键向量
    */
    std::vector<KeyType> keys() const;

    /** 复制弱一致值集合，遍历失败清空结果，返回向量构造异常由调用方处理
    @return 独立值向量
    */
    std::vector<ValueType> values() const;

    /** 复制弱一致键值集合，遍历失败清空结果，返回向量构造异常由调用方处理
    @return 独立键值对向量
    */
    std::vector<std::pair<KeyType, ValueType>> toVector() const;

    /** 查询固定桶数量，与池容量无关
    @return 归一化桶数，非法参数时为 0
    */
    size_t bucketCount() const;

    /** 观察候选、使用中及退役键值占用槽位数，排除可复用空闲槽位
    @return 占用槽位数，查询失败为 0
    */
    size_t retainedNodeCount() const;

private:
    /** 仅内置整数及默认比较器使用整数混合，避免旧标准库逐字节散列
    @param [in] key 完整整数键，高低位都参与混合
    @param [in] integral 编译期选择标记
    @return 内部桶哈希，不承诺与标准库桶位置一致
    */
    size_t hashKey(const KeyType& key, std::true_type integral) const;

    /** 保留用户哈希器和比较器的调用语义
    @param [in] key 查询键
    @param [in] integral 编译期选择标记
    @return 用户哈希值，异常由公共操作捕获
    */
    size_t hashKey(const KeyType& key, std::false_type integral) const;

    /** 默认整数哈希无用户调用，命中独立副本时不构造保护对象
    @param [in] key 查询键
    @param [out] value 可空，空表示仅查询存在性
    @param [in] integral 编译期默认整数标记
    @return 查询成功为 true
    */
    bool lookupValue(const KeyType& key, ValueType* value, std::true_type integral) const;

    /** 用户哈希在统一异常边界内执行，再查询副本或受保护节点
    @param [in] key 查询键
    @param [out] value 可空，用户赋值异常可能部分修改输出
    @param [in] integral 编译期通用标记
    @return 查询成功为 true，异常为 false
    */
    bool lookupValue(const KeyType& key, ValueType* value, std::false_type integral) const;

    /** 通用查询的唯一保护及异常边界，复制期间持有回收保护
    @param [in] key 查询键
    @param [in] hash 已计算哈希
    @param [out] value 可空，空表示不复制值
    @return 查询或赋值失败返回 false
    */
    bool lookupProtected(const KeyType& key, size_t hash, ValueType* value) const;

    // 仅默认整数键身份可以使用内建混合，不绕过任何用户哈希或等价回调
    typedef std::integral_constant<bool, std::is_integral<KeyType>::value &&
        std::is_same<HashType, std::hash<KeyType>>::value &&
        std::is_same<EqualType, std::equal_to<KeyType>>::value> IntegralHash;

    /** 执行条件写入，候选和输出在提交之前准备
    @param [in] key 借用键
    @param [in] value 写入值，删除为空
    @param [in] mode 零为覆盖，一为插入，二为条件替换，三为删除，四为条件删除
    @param [in] version 可选条件凭据
    @param [out] output 可选快照，失败保持不变
    @return 操作状态及提交标志
    */
    LockFreeHashMapResult modify(const KeyType& key, const ValueType* value, int32_t mode,
        const LockFreeHashMapVersion* version, Snapshot* output);

    /** 在保护内执行一次修改，调用者捕获异常并分摊回收
    @param [in] key 借用键
    @param [in] value 可选写入值
    @param [in] hash 已计算哈希
    @param [in] mode 修改模式
    @param [in] version 可选条件凭据
    @param [out] output 可选快照
    @param [in,out] record 独占登记记录
    @return 状态，提交后不调用用户代码
    */
    LockFreeHashMapResult modifyProtected(const KeyType& key, const ValueType* value, size_t hash, int32_t mode,
        const LockFreeHashMapVersion* version, Snapshot* output, LockFreeHashMapEpochRecord& record);

    /** 查找并摘除替换、删除和旧代节点，不分配
    @param [in] key 键，空表示只清理整桶
    @param [in] hash 哈希或清理桶下标
    @param [in] generation 观察代次
    @param [out] found 匹配的受保护槽，未找到为空
    @param [out] link 指向 found 的桶头或前驱链接
    @param [out] head 不存在时发布校验所用的桶头
    @param [in] retryLimit 非零时只供维护限制重启次数；数据操作为零并保留请求
    @return 找到、未找到或 clear 冲突，比较异常向操作边界传播
    */
    LockFreeHashMapStatus locate(const KeyType* key, size_t hash, uint64_t generation,
        Slot*& found, LockFreeHashMapLink*& link, uintptr_t& head, size_t retryLimit = 0) const;

    /** 摘除标记节点，删除桶头前等待旧观察者退出，不能恢复零或旧前缀地址制造 ABA
    @param [in] slot 受保护的已标记节点
    @param [in,out] link 桶头或受保护前驱链接
    @param [in,out] successor 摘除后的后继地址，安全头前缀可以合并摘除
    @param [in] bucketLink 当前链接是否为桶头
    @return 零为竞争失败或没有可写前驱，一为摘除成功，二为保留头部删除标记等待代际推进
    */
    int32_t unlink(Slot* slot, LockFreeHashMapLink* link, uintptr_t& successor, bool bucketLink) const;

    /** 在已登记保护中只读查找，不复制值或执行结构写入
    @param [in] key 查询键
    @param [in] hash 已计算哈希
    @param [in] generation 观察代次
    @return 有效节点，不存在为空，比较异常交由调用方捕获
    */
    Node* read(const KeyType& key, size_t hash, uint64_t generation) const;

    /** 将受保护节点复制为独立快照
    @param [in] node 完整节点
    @param [out] output 成功时替换，失败保持不变
    @return 快照准备状态
    */
    LockFreeHashMapStatus snapshot(const Node& node, Snapshot& output) const;

    /** 交换完整快照所有权，避免临时结果交付时增加引用计数；不执行值复制
    @param [in,out] left 交换一方，旧引用由交换后的持有者正常释放
    @param [in,out] right 交换另一方，版本和值始终一同交换
    */
    static void swapSnapshot(Snapshot& left, Snapshot& right) throw();

    /** 将与结果向量不重叠的节点副本直接构造到末尾，异常由遍历边界捕获
    @param [in,out] output 本次导出的独立向量
    @param [in] value 受保护节点中的键或值，调用期间不属于 output
    */
    template <typename ExportType>
    static void appendExport(std::vector<ExportType>& output, const ExportType& value);

    /** 兼容 C++11 及 v120 的 vector<bool> 特化，其不提供 emplace_back
    @param [in,out] output 本次导出的布尔向量
    @param [in] value 受保护节点中的布尔副本
    */
    static void appendExport(std::vector<bool>& output, const bool& value);

    /** 分片预留不重复的版本，耗尽不回绕
    @param [in,out] record 独占记录
    @return 非零版本，耗尽为零
    */
    uint64_t nextVersion(LockFreeHashMapEpochRecord& record);

    /** 提交后更新独占分片的弱一致计数
    @param [in,out] record 独占记录
    @param [in] generation 修改代次
    @param [in] difference 插入为一，删除为负一
    */
    void account(LockFreeHashMapEpochRecord& record, uint64_t generation, int64_t difference);

    /** 准备回调副本并执行向前遍历，异常不重放回调
    @param [in,out] function 被复制或移动的回调，键值只在回调期间借用
    @return 状态及已成功完成回调的条目数量
    */
    template <typename FunctionType>
    LockFreeHashMapResult forEachProtected(FunctionType&& function) const;

    /** 检查数组字节溢出后向上取整桶数
    @param [in] count 申请桶数，零使用默认一千零二十四
    @return 二次幂桶数，越界为零
    */
    static size_t normalizeBucketCount(size_t count);

    /** 禁止复制独占表
    @param [in] other 不可复制对象
    */
    LockFreeHashMap(const LockFreeHashMap& other);

    /** 禁止隐式共享池所有权
    @param [in] other 不可赋值对象
    @return 无实现
    */
    LockFreeHashMap& operator=(const LockFreeHashMap& other);

private:
    // 固定桶数组，只保存槽地址，桶头不使用逻辑标记
    LockFreeHashMapLink* m_buckets;
    // 桶数与节点池预算无关
    size_t m_bucketCount;
    // 并发安全的用户哈希器
    HashType m_hash;
    // 并发安全的用户等价比较器
    EqualType m_equal;
    // 独占池，仅在外部静止时替换
    Storage* m_storage;
    // 独占固定整数副本缓存，所有权与表相同，不引用池槽位
    LockFreeHashMapValueCache<KeyType, ValueType, EqualType> m_cache;
    // 快照持有实例身份，地址复用不会使旧凭据生效
    std::shared_ptr<const LockFreeHashMapIdentity> m_identity;
    // 逻辑代次，零为耗尽后永久关闭
    LOCKFREEHASHMAP_ALIGN_64 std::atomic<uint64_t> m_generation;
    // 已分配版本区间上界
    LOCKFREEHASHMAP_ALIGN_64 std::atomic<uint64_t> m_sequence;
    // 分片计数耗尽后回退遍历
    std::atomic<bool> m_countValid;
    // 构造时的初始化结果
    LockFreeHashMapStatus m_status;
    // 唯一维护者使用的清理桶下标
    size_t m_maintenanceBucket;
    // 数据操作分摊清理的轮转桶位置，不作为条目身份
    std::atomic<size_t> m_cleanupBucket;
};

#include "LockFreeHashMap.inl"