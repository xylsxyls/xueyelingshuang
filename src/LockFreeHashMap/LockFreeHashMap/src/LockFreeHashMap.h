#pragma once
#include "LockFreeHashMapMacro.h"
#include "HashMapValueProxy.h"
#include "LockFreeHashMapStorage.h"
#include "LockFreeHashMapDirectGuard.h"
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
    typedef LockFreeHashMapSlot<KeyType, ValueType> Slot;
    typedef LockFreeHashMapStorage<KeyType, ValueType> Storage;
    typedef LockFreeHashMapCache<KeyType, ValueType> Cache;

    /** 创建桶表和默认池，内部失败由 status 查询；用户哈希器及比较器构造异常由调用方处理
    @param [in] bucketCount 桶数量，0 使用 1024，其余向上取整为 2 的幂
    */
    explicit LockFreeHashMap(size_t bucketCount = 1024);

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
    @param [in] workBudget 桶、槽位和块检查各自的预算，整条旧链摘除不按节点分割
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
    @return 借用本表的代理，赋值不提供失败状态
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

    /** 弱一致地统计条目数量，外部静止时准确，失败可能得到部分计数
    @return 已正常完成的遍历回调总数，不限于 0 或 1
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
    // 仅测试构建在静止期注入极限计数，不改变布局或运行算法
#ifdef LOCKFREEHASHMAP_TEST_ACCESS
    friend struct LockFreeHashMapTestAccess;
#endif
    /** 尝试直接覆盖标量，避免构造快照和候选值；false 要求进入通用引擎
    @param [in] key 调用期间借用的键
    @param [in] value 待复制的值，调用期间借用
    @param [out] result 返回 true 时有效的最终状态
    @return 已处理为 true，false 要求回退通用引擎
    */
    bool updateScalar(const KeyType& key, const ValueType& value, LockFreeHashMapResult& result);

    /** 尝试直接替换非标量值并批量回收旧版本；false 要求进入通用引擎
    @param [in] key 调用期间借用的键
    @param [in] value 待复制的值，调用期间借用
    @param [out] result 返回 true 时有效的最终状态
    @return 已处理为 true，false 要求回退通用引擎
    */
    bool updateValue(const KeyType& key, const ValueType& value, LockFreeHashMapResult& result);

    /** 共享遍历引擎，回调准备及执行都位于异常捕获范围
    @param [in,out] function 按原值类别转发的回调，准备完成后只使用内部副本
    @return 状态和已正常完成的回调次数
    */
    template <typename FunctionType>
    LockFreeHashMapResult forEachProtected(FunctionType&& function) const;

    /** 尝试不构造候选对象的标量删除；false 要求进入通用引擎
    @param [in] key 调用期间借用的键
    @param [out] result 返回 true 时有效的最终状态
    @return 已处理为 true，false 要求回退通用引擎
    */
    bool eraseScalar(const KeyType& key, LockFreeHashMapResult& result);

    /** 执行共享的条件发布和删除协议，返回前释放失败候选
    @param [in] key 调用期间借用的键
    @param [in] value 写入值，删除模式必须为空
    @param [in] mode 0 写入、1 仅插入、2 条件替换、3 删除、4 条件删除
    @param [in] version 同次读取的条目身份，内部指针参数在无条件操作时可为空
    @param [out] output 可为空以跳过快照，提交前准备，失败保持原值
    @return 本次状态及提交标志；查询或维护不表示数据提交，m_count 为回调数或释放槽位数
    */
    LockFreeHashMapResult modify(const KeyType& key, const ValueType* value, int32_t mode,
        const LockFreeHashMapVersion* version, Snapshot* output);

    /** 保护查找路径并帮助摘除关闭节点，成功后保持节点和值保护
    @param [in] key 待查键，空表示仅清理桶
    @param [in] hash 已计算的哈希或缓存指纹，内建小整数的独立缓存路径可传 0
    @param [in] generation 本次观察或准备发布的可见代次
    @param [in,out] guard 当前操作独占的保护记录，保存游标和值保护
    @param [out] node 找到的受保护槽位，未找到为空
    @param [out] value 找到的受保护槽位，未找到为空
    @param [out] head 链查找观察到的桶头，缓存命中时不使用
    @param [in] pinHead 是否保持原桶头保护以校验不存在时插入
    @return Ok、NotFound 或用户比较异常
    */
    LockFreeHashMapStatus search(const KeyType* key, size_t hash, uint64_t generation,
        LockFreeHashMapGuard& guard, Slot*& node, Slot*& value, uint64_t& head, bool pinHead = false) const;

    /** 将已经保护的版本复制到独立快照，失败保持输出不变
    @param [in] value 已保护的值槽，仅 snapshot 的标量模式允许为空
    @param [out] output 成功时写入独立快照，失败保持原值
    @param [in] scalar 可为空，非空时提供原子读取的标量及值版本
    @return 复制及分配状态
    */
    LockFreeHashMapStatus snapshot(Slot* value, Snapshot& output, const LockFreeHashMapScalarState* scalar = nullptr) const;

    /** 从固定缓存读取标量，不借用可回收池地址，未命中保持输出不变
    @param [in] key 调用期间借用的键
    @param [in] hash 已计算的哈希或缓存指纹，内建小整数的独立缓存路径可传 0
    @param [out] value 找到时赋值，用户赋值异常可能部分修改输出
    @return 命中并复制成功为 true
    */
    bool cachedRead(const KeyType& key, size_t hash, ValueType& value) const;

    /** 在源 CAS 之前使旧读缓存失效，完整条目版本仍为最终依据
    @param [in] key 调用期间借用的键
    @param [in] hash 已计算的哈希或缓存指纹，内建小整数的独立缓存路径可传 0
    @param [in] generation 本次观察或准备发布的可见代次
    @param [in] version 候选值版本，0 表示删除
    @return 提交后发布使用的标签，0 表示不可缓存
    */
    uint64_t invalidateRead(const KeyType& key, size_t hash, uint64_t generation, uint64_t version) const;

    /** 源 CAS 成功后发布标量副本，迟到发布不能恢复旧版本有效性
    @param [in] key 调用期间借用的键
    @param [in] hash 已计算的哈希或缓存指纹，内建小整数的独立缓存路径可传 0
    @param [in] value 待复制的值，调用期间借用
    @param [in] tag 源 CAS 前取得的唯一失效标签，0 不发布
    */
    void publishRead(const KeyType& key, size_t hash, const ValueType& value, uint64_t tag) const;

    /** 判断固定缓存是否能保存完整小键与不可变值提示
    @return 类型、等价关系及配置均支持时为 true
    */
    bool usesValueCache() const;

    /** 合并借用记录与值保护再校验不可变值提示，赋值异常不重试
    @param [in] key 调用期间借用的键
    @param [in] hash 已计算的哈希或缓存指纹，内建小整数的独立缓存路径可传 0
    @param [out] output 命中后赋值，用户赋值异常可能部分修改
    @return NotFound 要求回退，其余为最终结果，不重复失败赋值
    */
    LockFreeHashMapStatus cachedValueRead(const KeyType& key, size_t hash, ValueType& output) const;

    /** 发布源校验后的值提示，在放下保护前撤回失效的迟到发布
    @param [in] key 调用期间借用的键
    @param [in] hash 已计算的哈希或缓存指纹，内建小整数的独立缓存路径可传 0
    @param [in] value 已保护的值槽，仅 snapshot 的标量模式允许为空
    @param [in] tag 源 CAS 前取得的唯一失效标签，0 不发布
    */
    void publishValue(const KeyType& key, size_t hash, Slot* value, uint64_t tag) const;

    /** 在退役可见前精确移除旧值提示
    @param [in] key 调用期间借用的键
    @param [in] hash 已计算的哈希或缓存指纹，内建小整数的独立缓存路径可传 0
    @param [in] value 已保护的值槽，仅 snapshot 的标量模式允许为空
    */
    void forgetValue(const KeyType& key, size_t hash, Slot* value) const;

    /** 转移已有热值提示，调用方保护新值直到复查结束
    @param [in] key 调用期间借用的键
    @param [in] hash 已计算的哈希或缓存指纹，内建小整数的独立缓存路径可传 0
    @param [in] old 已保护的原值槽，退役前保持有效
    @param [in] value 已保护的值槽，仅 snapshot 的标量模式允许为空
    @param [in] tag 源 CAS 前取得的唯一失效标签，0 不发布
    */
    void refreshValue(const KeyType& key, size_t hash, Slot* old, Slot* value, uint64_t tag) const;

    /** 判断是否可缓存小整数键和标量值副本
    @return 类型、等价关系及配置均支持时为 true
    */
    bool usesReadCache() const;

    /** 计算缓存专用指纹，不改变真实桶哈希，自定义等价关系继续使用用户哈希
    @param [in] key 调用期间借用的键
    @param [in] hash 已计算的哈希或缓存指纹，内建小整数的独立缓存路径可传 0
    @return 与等价关系一致的缓存指纹
    */
    size_t lookupHash(const KeyType& key, size_t hash) const;

    /** 捕获用户哈希异常，再尝试缓存和受保护查找
    @param [in] key 调用期间借用的键
    @param [out] value 找到时赋值，用户赋值异常可能部分修改输出
    @return 查找并赋值成功为 true
    */
    bool getHashed(const KeyType& key, ValueType& value) const;

    /** 缓存未命中后保护节点和值，再直接向调用方赋值
    @param [in] key 调用期间借用的键
    @param [in] hash 已计算的哈希或缓存指纹，内建小整数的独立缓存路径可传 0
    @param [out] value 找到时赋值，用户赋值异常可能部分修改输出
    @return 查找并赋值成功为 true
    */
    bool getProtected(const KeyType& key, size_t hash, ValueType& value) const;

    /** 选择两个固定节点提示位置之一，不改变真实桶
    @param [in] hash 已计算的哈希或缓存指纹，内建小整数的独立缓存路径可传 0
    @param [in] probe 0 为首选，1 为备用，要求缓存已启用
    @return 有效缓存下标
    */
    size_t cacheIndex(size_t hash, size_t probe) const;

    /** 在节点保持保护时发布弱提示，并撤回已经关闭的节点
    @param [in] node 非空且已保护或唯一拥有的键节点
    */
    void remember(Slot* node) const;

    /** 保护当前值并捕获一致标量身份，帮助完成已发布转移
    @param [in] node 非空且已保护或唯一拥有的键节点
    @param [out] value 当前值槽，返回 true 时保持保护
    @param [in,out] guard 当前操作独占的保护记录，保存游标和值保护
    @return 当前值可用为 true，完成转移或关闭后需重试为 false
    */
    bool captureValue(Slot* node, Slot*& value, LockFreeHashMapGuard& guard) const;

    /** 帮助推进已发布标量描述，胜者退役旧外置值
    @param [in] node 非空且已保护或唯一拥有的键节点
    @param [in] old 已保护的原值槽，退役前保持有效
    @param [in] next 已完整构造的后继槽
    */
    void helpValue(Slot* node, Slot* old, Slot* next) const;

    /** 关闭键节点和值，并永久标记后继链接
    @param [in] node 非空且已保护或唯一拥有的键节点
    @return 去掉标志的后继地址，尚未借用后继
    */
    uintptr_t closeNode(Slot* node) const;

    /** 读取桶头或受保护前驱的链接
    @param [in] predecessor 已保护的前驱，空表示桶头
    @param [in] bucket 有效桶下标
    @return 链接值，奇数桶头为唯一空桶身份
    */
    uint64_t loadLink(Slot* predecessor, size_t bucket) const;

    /** 摘除关闭节点，旧代桶整体摘除以阻止空桶 ABA
    @param [in] predecessor 已保护的前驱，空表示桶头
    @param [in] bucket 有效桶下标
    @param [in] node 非空且已保护或唯一拥有的键节点
    @param [in,out] next 去标志的后继，整链摘除或终态哨兵时改为 0
    @return 本次物理摘除成功为 true
    */
    bool unlinkNode(Slot* predecessor, size_t bucket, Slot* node, uintptr_t& next) const;

    /** 申请永不重复的奇数空桶身份，耗尽后不复用
    @return 新奇数身份，耗尽为 0
    */
    uint64_t nextEmptyIdentity() const;

    /** 不调用用户回调地轮转清理桶，结束清除键值保护
    @param [in,out] guard 当前操作独占的保护记录，保存游标和值保护
    @param [in] bucketLimit 本次检查的桶数量
    */
    void cleanBuckets(LockFreeHashMapGuard& guard, size_t bucketLimit);

    /** 从当前记录的独占范围取得永不复用的值版本
    @param [in,out] guard 当前操作独占的保护记录，保存游标和值保护
    @return 非零新版本，实例版本空间耗尽为 0
    */
    uint64_t nextVersion(LockFreeHashMapGuard& guard);

    /** 检查位移和字节预算后归一化桶数
    @param [in] count 申请桶数，0 使用默认值
    @return 向上取整的 2 的幂，超出地址空间为 0
    */
    static size_t normalizeBucketCount(size_t count);

    /** 禁止复制独占桶表和池
    @param [in] other 不可复制或赋值的源表
    */
    LockFreeHashMap(const LockFreeHashMap& other);

    /** 禁止以赋值共享独占池
    @param [in] other 不可复制或赋值的源表
    @return 无实现，不可调用
    */
    LockFreeHashMap& operator=(const LockFreeHashMap& other);
private:
    // 拥有桶数组，偶数为节点地址，奇数为不重复的空桶身份
    std::atomic<uint64_t>* m_buckets;
    // 归一化桶数，与池容量无关
    size_t m_bucketCount;
    // 用户提供的线程安全哈希器
    HashType m_hash;
    // 用户提供且与哈希一致的线程安全等价比较器
    EqualType m_equal;
    // 独占节点池，仅外部静止时替换
    Storage* m_storage;
    // 与版本令牌共享所有权的实例身份
    std::shared_ptr<const LockFreeHashMapIdentity> m_identity;
    // 可见代次，耗尽后的 0 永久关闭写入
    std::atomic<uint64_t> m_generation;
    // 已分配且永不重复的版本范围上界
    std::atomic<uint64_t> m_sequence;
    // 构造完成后不再改变的初始化结果
    LockFreeHashMapStatus m_status;
    // 仅由外部唯一维护者推进的桶游标
    size_t m_maintenanceBucket;
    // 数据操作共同推进的清理游标，不作为身份使用
    std::atomic<size_t> m_cleanupBucket;
};

#include "LockFreeHashMap.inl"