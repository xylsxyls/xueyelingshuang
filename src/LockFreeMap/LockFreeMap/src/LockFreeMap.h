#pragma once
#include "LockFreeMapMacro.h"
#include "LockFreeMapContract.h"
#include "LockFreeMapIndex.h"
#include "MapValueProxy.h"
#include <atomic>
#include <functional>
#include <memory>
#include <new>
#include <stdint.h>
#include <stddef.h>
#include <utility>
#include <vector>

/** 遵循 BASE-LF-1.0 的无锁有序容器
@note 有序键索引的查询与覆盖为 O(log n)，结构变更为 O(n)，逻辑清空为 O(1)
@note 键和值使用独立池回收旧对象；元数据有界，不使用线程局部登记或后台线程
@note 析构、配置及 clearAndFreeMemory 必须由调用方保证外部静止
@note 比较器和策略必须线程安全、短小；用户类型的析构函数不得抛异常
@note 多个输出需使用独立存储。查找插入、按键提取和条件提取支持输入与值或键输出共用存储
@note MaxLevel 仅为旧源码兼容保留，当前实现不使用跳表层数
@note 普通接口内部重试 CAS 竞争，没有重试次数上限；CONFLICT 只用于条件凭据失效
*/
template <typename KeyType, typename ValueType, typename LessType = std::less<KeyType>, size_t MaxLevel = 16>
class LockFreeMap
{
public:
    /** 尝试插入或覆盖键值，按开关决定是否重试竞争及等待容量
    @param [in] key 按LessType查询的键；输入只在本次同步调用期间借用
    @param [in] value 待复制的候选值，仅本次同步调用期间借用
    @param [in] retryContention true持续重试发布竞争，false在观察到可重试竞争时返回BUSY
    @param [in] retryCapacity true重试CAPACITY、RESERVED、RECORD_LIMIT；按 setCapacityWait 的设置重试，单线程Wasm或重入自等待返回原状态
    @return COMMITTED已提交；BUSY为未重试的竞争，其他失败返回具体状态
    @note 调用同步完成；输入不转移所有权
    */
    LockFreeMapStatus trySet(const KeyType& key, const ValueType& value, bool retryContention = false, bool retryCapacity = false);

    /** 尝试插入新键；已有等价键时保留原值
    @param [in] key 按LessType查询的键；输入只在本次同步调用期间借用
    @param [in] value 待复制的候选值，仅本次同步调用期间借用
    @param [in] retryContention true持续重试发布竞争，false在观察到可重试竞争时返回BUSY
    @param [in] retryCapacity true重试CAPACITY、RESERVED、RECORD_LIMIT；按 setCapacityWait 的设置重试，单线程Wasm或重入自等待返回原状态
    @return COMMITTED已插入；EXISTS已有条目；其他失败返回具体状态
    @note 调用同步完成；输入不转移所有权
    */
    LockFreeMapStatus tryInsert(const KeyType& key, const ValueType& value, bool retryContention = false, bool retryCapacity = false);

    /** 尝试查找或插入候选，并复制实际条目及凭据
    @param [in] key 按LessType查询的键；输入只在本次同步调用期间借用
    @param [in] candidate 已构造候选，所有返回状态下原对象仍归调用方
    @param [out] value 相应成功状态下的实际值副本；失败时可能部分赋值
    @param [out] token 返回COMMITTED或EXISTS时的实际条目凭据
    @param [in] retryContention true持续重试发布竞争，false在观察到可重试竞争时返回BUSY
    @param [in] retryCapacity true重试CAPACITY、RESERVED、RECORD_LIMIT；按 setCapacityWait 的设置重试，单线程Wasm或重入自等待返回原状态
    @return COMMITTED已插入或EXISTS已有条目；仅这两种状态下输出有效
    @note 调用同步完成；输入不转移所有权，失败输出可能已被复制或部分赋值，不得作为成功结果使用
    */
    LockFreeMapStatus tryGetOrInsert(const KeyType& key, const ValueType& candidate, ValueType& value, LockFreeMapToken& token, bool retryContention = false, bool retryCapacity = false);

    /** 仅在实例和条目版本匹配时尝试覆盖
    @param [in] key 按LessType查询的键；输入只在本次同步调用期间借用
    @param [in] token 原实例及条目版本凭据，失效时返回CONFLICT且不自动刷新
    @param [in] value 待复制的候选值，仅本次同步调用期间借用
    @param [in] retryContention true持续重试发布竞争，false在观察到可重试竞争时返回BUSY
    @param [in] retryCapacity true重试CAPACITY、RESERVED、RECORD_LIMIT；按 setCapacityWait 的设置重试，单线程Wasm或重入自等待返回原状态
    @return COMMITTED已覆盖；CONFLICT凭据失效；其他失败返回具体状态
    @note 调用同步完成；输入不转移所有权
    */
    LockFreeMapStatus tryUpdate(const KeyType& key, const LockFreeMapToken& token, const ValueType& value, bool retryContention = false, bool retryCapacity = false);

    /** 尝试删除比较器等价的键
    @param [in] key 按LessType查询的键；输入只在本次同步调用期间借用
    @param [in] retryContention true持续重试发布竞争，false在观察到可重试竞争时返回BUSY
    @param [in] retryCapacity true重试CAPACITY、RESERVED、RECORD_LIMIT；按 setCapacityWait 的设置重试，单线程Wasm或重入自等待返回原状态
    @return COMMITTED已删除；NOT_FOUND不存在；其他失败返回具体状态
    @note 调用同步完成；输入不转移所有权
    */
    LockFreeMapStatus tryErase(const KeyType& key, bool retryContention = false, bool retryCapacity = false);

    /** 仅在实例和条目版本匹配时尝试删除
    @param [in] key 按LessType查询的键；输入只在本次同步调用期间借用
    @param [in] token 原实例及条目版本凭据，失效时返回CONFLICT且不自动刷新
    @param [in] retryContention true持续重试发布竞争，false在观察到可重试竞争时返回BUSY
    @param [in] retryCapacity true重试CAPACITY、RESERVED、RECORD_LIMIT；按 setCapacityWait 的设置重试，单线程Wasm或重入自等待返回原状态
    @return COMMITTED已删除；CONFLICT凭据失效；其他失败返回具体状态
    @note 调用同步完成；输入不转移所有权
    */
    LockFreeMapStatus tryEraseIf(const KeyType& key, const LockFreeMapToken& token, bool retryContention = false, bool retryCapacity = false);

    /** 尝试复制值并删除条目；输出复制失败不提交删除
    @param [in] key 按LessType查询的键；输入只在本次同步调用期间借用
    @param [out] value 相应成功状态下的实际值副本；失败时可能部分赋值
    @param [in] retryContention true持续重试发布竞争，false在观察到可重试竞争时返回BUSY
    @param [in] retryCapacity true重试CAPACITY、RESERVED、RECORD_LIMIT；按 setCapacityWait 的设置重试，单线程Wasm或重入自等待返回原状态
    @return COMMITTED已提取；NOT_FOUND不存在；其他失败返回具体状态
    @note 调用同步完成；输入不转移所有权，失败输出可能已被复制或部分赋值，不得作为成功结果使用
    */
    LockFreeMapStatus tryExtract(const KeyType& key, ValueType& value, bool retryContention = false, bool retryCapacity = false);

    /** 尝试提取比较器排序的末项，始终使用调用方原凭据
    @param [in] token 原实例及条目版本凭据，失效时返回CONFLICT且不自动刷新
    @param [out] key 返回COMMITTED时实际提取的键副本
    @param [out] value 相应成功状态下的实际值副本；失败时可能部分赋值
    @param [in] retryContention true持续重试发布竞争，false在观察到可重试竞争时返回BUSY
    @param [in] retryCapacity true重试CAPACITY、RESERVED、RECORD_LIMIT；按 setCapacityWait 的设置重试，单线程Wasm或重入自等待返回原状态
    @return COMMITTED已提取；CONFLICT凭据或末项条件失效；其他失败返回具体状态
    @note 调用同步完成；输入不转移所有权，失败输出可能已被复制或部分赋值，不得作为成功结果使用
    */
    LockFreeMapStatus tryExtractLast(const LockFreeMapToken& token, KeyType& key, ValueType& value, bool retryContention = false, bool retryCapacity = false);

    /** 在外部静止且容器为空时替换双池配置
    @param [in] config 独立池预算及有界保护记录配置；已有非空表或活动记录时返回INVALID
    @return 成功返回 OK；失败不提交配置，保留原配置及数据
    */
    LockFreeMapStatus lockFreeMapConfigure(const LockFreeMapConfig& config);

    /** 在保护范围内复制当前不可变值版本及其凭据
    @param [in] key 按比较器查询的键
    @param [out] value 值副本；用户赋值抛异常时可能已部分修改输出
    @param [out] token 仅在成功返回 OK 时写入凭据
    @return 返回 OK、NOT_FOUND 或未提交的错误状态
    */
    LockFreeMapStatus lockFreeMapGet(const KeyType& key, ValueType& value, LockFreeMapToken& token) const;

    /** 在读保护范围内访问一个值，省去值副本及shared_ptr引用计数竞争
    @param [in] key 查询键
    @param [in] function 接受const ValueType&的回调，每次成功最多调用一次；引用不能逃逸
    @return OK已调用；NOT_FOUND未找到；其他状态均不表示调用成功，回调异常返回USER_EXCEPTION
    @note 回调可重入数据操作，但不得析构、重新配置或clearAndFreeMemory；引用保护持续到回调结束
    */
    template <typename FunctionType>
    LockFreeMapStatus lockFreeMapVisit(const KeyType& key, const FunctionType& function) const;

    /** 发布完整候选，键已存在时复制实际胜出的条目
    @param [in] key 按比较器查询的键
    @param [in] candidate 已构造候选；调用方在所有结果下均保留原对象的所有权
    @param [out] value 返回 COMMITTED 或 EXISTS 时的实际值副本
    @param [out] token 返回 COMMITTED 或 EXISTS 时的实际条目凭据
    @return 返回 COMMITTED、EXISTS 或未提交错误；内部不调用工厂
    */
    LockFreeMapStatus lockFreeMapGetOrInsert(const KeyType& key, const ValueType& candidate, ValueType& value, LockFreeMapToken& token);

    /** 仅替换凭据仍匹配的条目版本
    @param [in] key 按比较器查询的键
    @param [in] token 原实例及条目版本凭据
    @param [in] value 已完整构造的候选值
    @return 返回 COMMITTED、CONFLICT 或未提交的错误状态
    */
    LockFreeMapStatus lockFreeMapUpdate(const KeyType& key, const LockFreeMapToken& token, const ValueType& value);

    /** 仅删除凭据仍匹配的条目版本
    @param [in] key 按比较器查询的键
    @param [in] token 原条目凭据
    @return 返回 COMMITTED、CONFLICT 或未提交错误；不申请键槽或值槽
    */
    LockFreeMapStatus lockFreeMapErase(const KeyType& key, const LockFreeMapToken& token);

    /** 先复制输出再原子删除；复制失败不会删除条目
    @param [in] key 按比较器查询的键
    @param [out] value 返回 COMMITTED 时由调用方拥有的值副本
    @return 返回 COMMITTED、NOT_FOUND 或未提交错误
    */
    LockFreeMapStatus lockFreeMapExtract(const KeyType& key, ValueType& value);

    /** 在受保护根验证时刻选择 LessType 排序的最后一个键
    @param [out] key 选中的键副本
    @param [out] value 由调用方拥有的值副本
    @param [out] token 选中条目的版本凭据
    @return 返回 OK、NOT_FOUND 或未提交的错误状态
    */
    LockFreeMapStatus lockFreeMapLast(KeyType& key, ValueType& value, LockFreeMapToken& token) const;

    /** 仅在本次根 CAS 时凭据仍对应最大条目时提取
    @param [in] token 此前受保护查询得到的凭据
    @param [out] key 返回 COMMITTED 时实际提取的键
    @param [out] value 返回 COMMITTED 时由调用方拥有的值副本
    @return 返回 COMMITTED、CONFLICT 或未提交的错误状态
    @note 根 CAS 前插入更高键会导致 CONFLICT；选桶本身不预留桶内任务
    */
    LockFreeMapStatus lockFreeMapExtractLast(const LockFreeMapToken& token, KeyType& key, ValueType& value);

    /** 不附带版本条件地发布键值版本
    @param [in] key 按比较器查询的键
    @param [in] value 待复制的候选值
    @return 返回 COMMITTED 或未提交的资源、配置、用户异常等错误；竞争在内部重试
    */
    LockFreeMapStatus lockFreeMapSet(const KeyType& key, const ValueType& value);

    /** 并发执行有预算的回收，不等待暂停读者退出
    @param [in] budget 索引引用清理及每个池目录访问分别使用的预算；并非总指令或析构次数上限
    @param [out] releasedCapacity 本次实际释放整块内存所归还的槽位数
    @return OK或初始化、策略、记录等错误；失败前完成的安全回收不会撤销
    */
    LockFreeMapStatus lockFreeMapMaintain(size_t budget, size_t& releasedCapacity);

    /** 观察本库管理的存储，不包含键值内部的堆申请
    @return 各项独立观察结果，不是事务快照或容量预留
    */
    LockFreeMapStats lockFreeMapStats() const;

    /** 检查本实现使用的全部原子宽度是否无锁
    @return 指针、size_t、uint64_t 和 uint32_t 原子均无锁时返回 true
    */
    bool lockFreeMapAtomicsSupported() const;

    /** 尝试建立默认双池；初始化失败保存状态供后续状态接口查询
    */
    LockFreeMap();

    /** 选择是否立即建立默认池，延迟模式由 lockFreeMapConfigure 完成初始化
    @param [in] initializeImmediately true 保持默认构造行为；false 不申请池及保护记录
    @note 延迟模式在成功配置前，数据操作返回 INVALID 或旧接口的失败值
    */
    explicit LockFreeMap(bool initializeImmediately);

    /** 析构函数，释放所有保留节点和值版本
    @note 析构时外部必须保证没有其它线程继续访问本对象
    */
    ~LockFreeMap();

    /** 返回下标访问代理
    @param [in] key 键
    @return 返回代理对象，支持map[key] = value写法
    @note 创建代理会复制键且可能抛用户异常；代理赋值未提交时抛LockFreeMapError，状态接口使用lockFreeMapSet
    */
    MapValueProxy<KeyType, ValueType, LessType, MaxLevel> operator[](const KeyType& key);

    /** 插入或覆盖键值
    @param [in] key 键
    @param [in] value 值
    @return 返回是否写入成功，内存分配失败或比较函数抛异常时返回false
    @note 可并发调用。旧值在拥有引用和有效值借用均结束后可析构，后续写入或维护分批回收
    */
    bool set(const KeyType& key, const ValueType& value);

    /** 插入键值，键已存在时不覆盖
    @param [in] key 键
    @param [in] value 值
    @return 插入成功返回true，键已存在、内存分配失败或比较函数抛异常时返回false
    @note 可并发调用。删除或clear后允许重新插入，旧凭据不会重新有效
    */
    bool insert(const KeyType& key, const ValueType& value);

    /** 删除键
    @param [in] key 键
    @return 已提交删除返回true；键不存在、资源不足或用户异常等失败返回false
    @note 可并发调用。CAS移除当前索引项，旧索引由安全回收释放；分配失败也返回false
    */
    bool erase(const KeyType& key);

    /** 删除键，erase别名
    @param [in] key 键
    @return 已提交删除返回true；键不存在、资源不足或用户异常等失败返回false
    */
    bool remove(const KeyType& key);

    /** 查询键对应的值
    @param [in] key 键
    @param [out] value 成功时写入值副本；失败时可能已部分赋值
    @return 找到返回true，否则返回false
    @note 可并发调用
    */
    bool get(const KeyType& key, ValueType& value) const;

    /** 按值查询，未找到时保留默认副本；需要区分错误时使用状态接口
    @param [in] key 键
    @param [in] defaultValue 查询前复制的默认值；默认参数构造及按值复制、返回可能抛异常
    @return 先复制默认值再尝试查询；查询失败但输出曾部分赋值时返回该对象当前内容
    @note 可并发调用
    */
    ValueType value(const KeyType& key, const ValueType& defaultValue = ValueType()) const;

    /** 判断键是否存在
    @param [in] key 键
    @return 存在返回true
    @note 可并发调用
    */
    bool contains(const KeyType& key) const;

    /** 返回键是否存在，语义对齐std::map::count(key)
    @param [in] key 键
    @return 存在返回1，不存在返回0
    @note 可并发调用
    */
    size_t count(const KeyType& key) const;

    /** 返回当前键数量
    @return 返回当前可见键数量
    @note 可并发调用；无并发写入时精确，有并发写入时返回弱一致结果
    */
    size_t count() const;

    /** 返回当前键数量，count()别名
    @return 返回当前可见键数量
    */
    size_t size() const;

    /** 判断是否为空
    @return 为空返回true
    @note 可并发调用；有并发写入时结果只代表调用瞬间附近状态
    */
    bool empty() const;

    /** 逻辑清空Map
    @note 可并发调用。O(1)交换空根；旧快照由后续写入/维护回收，不删除clear之后提交的项
    */
    void clear();

    /** 清空并释放已保留节点和值版本
    @note 要求外部静止；释放包括初始容量在内的全部有效载荷块，保留固定元数据。下次申请按扩容策略增长
    */
    void clearAndFreeMemory();

    /** 按LessType顺序遍历当前可见键值
    @param [in] function 按值传入的同步回调，形如function(const KeyType&, const ValueType&)；入参复制发生在函数体外
    @return 正常返回的回调次数；未初始化、记录不足或空表返回零，异常时返回此前完成数
    @note 回调引用仅本次受保护调用内有效。允许重入但占用额外保护记录；异常返回已完成回调次数
    */
    template <typename FunctionType>
    size_t forEach(FunctionType function) const;

    /** 读取所有当前可见键
    @return 返回按LessType顺序排列的键列表
    @note 可并发调用；每个值单独观察，结果非事务快照；资源不足可返回空列表；首次复制异常立即结束并返回空列表
    */
    std::vector<KeyType> keys() const;

    /** 读取所有当前可见值
    @return 返回按LessType顺序排列的值列表
    @note 可并发调用；每个值单独观察，结果非事务快照；资源不足可返回空列表；首次复制异常立即结束并返回空列表
    */
    std::vector<ValueType> values() const;

    /** 读取所有当前可见键值对
    @return 返回按LessType顺序排列的键值对列表
    @note 可并发调用；每个值单独观察，结果非事务快照；资源不足可返回空列表；首次复制异常立即结束并返回空列表
    */
    std::vector<std::pair<KeyType, ValueType>> toVector() const;

    /** 获取第一个可见键值
    @param [out] key 查询成功时写入LessType排序首键
    @param [out] value 查询成功时写入LessType排序首键对应的值
    @return 找到返回true，否则返回false
    @note 可并发调用；并发写入时返回弱一致结果
    */
    bool first(KeyType& key, ValueType& value) const;

    /** 获取最后一个可见键值
    @param [out] key 查询成功时写入LessType排序末键
    @param [out] value 查询成功时写入LessType排序末键对应的值
    @return 找到返回true，否则返回false
    @note 可并发调用；并发写入时返回弱一致结果
    */
    bool last(KeyType& key, ValueType& value) const;

    /** 按LessType查询第一个不小于指定键的可见键值
    @param [in] key 查询键
    @param [out] foundKey 查询成功时写入找到的键
    @param [out] value 查询成功时写入找到的值
    @return 找到返回true，否则返回false
    @note 可并发调用；并发写入时返回弱一致结果
    */
    bool lowerBound(const KeyType& key, KeyType& foundKey, ValueType& value) const;

    /** 按LessType查询第一个严格位于指定键之后的可见键值
    @param [in] key 查询键
    @param [out] foundKey 查询成功时写入找到的键
    @param [out] value 查询成功时写入找到的值
    @return 找到返回true，否则返回false
    @note 可并发调用；并发写入时返回弱一致结果
    */
    bool upperBound(const KeyType& key, KeyType& foundKey, ValueType& value) const;

    /** 返回为源兼容保留的层数参数（当前内部使用有序索引）
    @return 返回模板参数MaxLevel
    */
    size_t maxLevel() const;

    /** 返回实际尚未析构的键对象数量
    @return 存活及待安全回收的键数，不是池容量
    @note 可随并发回收下降；不包含未构造空闲槽位
    */
    size_t retainedNodeCount() const;

    /** 返回已分配但尚未释放的值版本数量
    @return 返回保留值版本数量
    @note 覆盖、erase和clear后的旧版本在安全回收前计入，可随回收下降
    */
    size_t retainedValueCount() const;

private:
    /** 执行一次写操作，按RetryContention选择竞争重试；条件凭据不自动刷新
    @param [in] key 待查键，仅最大项提取时允许 nullptr
    @param [in] value 候选值，删除时为 nullptr
    @param [in] token 可选条件凭据，无条件操作时为 nullptr
    @param [out] outputKey 可选提取键输出，不需要时为 nullptr
    @param [out] outputValue 可选实际值或提取值输出，不需要时为 nullptr
    @param [out] outputToken 可选实际条目凭据，不需要时为 nullptr
    @tparam Mode 0 覆盖，1 插入，2 删除，3 条件更新，4 条件删除，5 最大项提取
    @tparam RetryContention true持续重试CAS竞争，false允许返回未提交竞争状态
    @tparam Outputs 输出位掩码，1为键、2为值、4为版本；置位输出必须非空
    @return 提交成功或明确的未提交状态
    */
    template <uint32_t Mode, bool RetryContention, uint32_t Outputs = 0>
    LockFreeMapStatus write(const KeyType* key, const ValueType* value, const LockFreeMapToken* token,
        KeyType* outputKey, ValueType* outputValue, LockFreeMapToken* outputToken)
#ifdef __EMSCRIPTEN__
        noexcept
#endif
        ;

    /** 检查两个对象自身的存储有无重叠，包括成员子对象
    @param [in] input 输入地址，允许空
    @param [in] inputSize 输入对象字节数
    @param [in] output 输出地址，允许空
    @param [in] outputSize 输出对象字节数
    @return 两个非空对象的存储重叠时为true
    */
    static bool overlaps(const void* input, size_t inputSize, const void* output, size_t outputSize);

    /** 稳定与输出重叠的输入，在单次写入释放候选和保护后处理容量重试
    @param [in] key 可空查询键
    @param [in] value 可空候选值
    @param [in] token 可空条件凭据，重试不改变凭据
    @param [out] outputKey 可空输出键
    @param [out] outputValue 可空输出值
    @param [out] outputToken 可空输出凭据
    @param [in] retryContention 是否持续重试发布冲突
    @param [in] retryCapacity 是否等待容量或记录；单线程Wasm和外层借用重入返回原状态，其他后端按实例设置的次数和时长等待
    @tparam Mode 与write的修改模式一致
    @tparam Outputs 与write的输出位掩码一致
    @return 已提交或明确的未提交状态
    */
    template <uint32_t Mode, uint32_t Outputs = 0>
    LockFreeMapStatus writeRetry(const KeyType* key, const ValueType* value, const LockFreeMapToken* token,
        KeyType* outputKey, ValueType* outputValue, LockFreeMapToken* outputToken, bool retryContention, bool retryCapacity);

    /** 复制一个受保护条目
    @param [in] key 可选查询键，首尾查询时为 nullptr
    @param [out] outputKey 可选键输出，不需要时为 nullptr
    @param [out] outputValue 可选值输出，不需要时为 nullptr
    @param [out] outputToken 可选版本凭据输出，不需要时为 nullptr
    @tparam Mode 0 查询，1 首项，2 尾项，3 下界，4 上界，调用点在编译期固定
    @tparam Outputs 输出位掩码，1为键、2为值、4为版本；置位输出必须非空
    @return 返回 OK、NOT_FOUND 或明确错误
    */
    template <uint32_t Mode, uint32_t Outputs>
    LockFreeMapStatus read(const KeyType* key, KeyType* outputKey, ValueType* outputValue,
        LockFreeMapToken* outputToken) const
#ifdef __EMSCRIPTEN__
        noexcept
#endif
        ;

    /** 领取读保护并遍历可见条目，回调异常结束遍历并交由调用入口处理
    @param [in] function 同步接收当前借用键值的回调
    @param [in,out] completed 累加正常返回的回调次数
    */
    template <typename Function>
    void visitAll(Function& function, size_t& completed) const;

    /** 导出键、值或键值对；首次复制失败时释放全部暂存结果
    @param [in,out] result 调用方新建的空容器
    @param [in] append 将当前借用条目复制到 result 的同步回调
    */
    template <typename Element, typename Append>
    void copyAll(std::vector<Element>& result, Append append) const;

    /** 在受保护的固定键视图中遍历；Plain仅用于没有省略项和覆盖项的视图
    @param [in] store 当前实例的稳定存储
    @param [in] root 已完成决策且受guard保护的索引
    @param [in,out] guard 逐项保护值版本的操作记录
    @param [in] function 接受本项键和值的同步回调，引用不能在回调后保留
    @param [in,out] completed 已正常返回的回调数，异常时保留此前进度
    */
    template <bool Plain, typename Function>
    static void visitView(LockFreeMapIndexStore<KeyType, ValueType, LessType>* store,
        LockFreeMapIndex<KeyType, ValueType>* root, LockFreeMapGuard& guard, Function& function, size_t& completed);

    /** 禁止复制拥有独立回收域的容器
    @param [in] other 源对象
    */
    LockFreeMap(const LockFreeMap& other);

    /** 禁止复制拥有独立回收域的容器
    @param [in] other 源对象
    @return 当前容器引用，函数禁用
    */
    LockFreeMap& operator=(const LockFreeMap& other);

private:
    // 独占存储；只读接口通过保护记录访问，不需要 mutable 修饰
    LockFreeMapIndexStore<KeyType, ValueType, LessType>* m_store;
    // 构造失败状态保留至配置成功，供业务查询具体原因
    LockFreeMapStatus m_initialStatus;
};

#include "LockFreeMap.inl"