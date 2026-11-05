#pragma once
#include "LockFreeMapMacro.h"
#include "LockFreeMapContract.h"
#include "LockFreeMapIndex.h"
#include "MapValueProxy.h"
#include <atomic>
#include <functional>
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
@note MaxLevel 仅为旧源码兼容保留，当前实现不使用跳表层数
*/
template <typename KeyType, typename ValueType, typename LessType = std::less<KeyType>, size_t MaxLevel = 16>
class LockFreeMap
{
public:
    /** 在首次使用前或显式释放后配置双池，调用期间要求外部静止
    @param [in] config 独立池预算及有界保护记录配置
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

    /** 发布完整候选，键已存在时复制实际胜出的条目
    @param [in] key 按比较器查询的键
    @param [in] candidate 已构造候选；调用方在所有结果下均保留原对象的所有权
    @param [out] value 返回 COMMITTED 或 EXISTS 时的实际值副本
    @param [out] token 返回 COMMITTED 或 EXISTS 时的实际条目凭据
    @return 返回 COMMITTED、EXISTS、CONFLICT 或未提交错误；内部不调用工厂
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
    @return 返回 COMMITTED、NOT_FOUND、CONFLICT 或未提交错误
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
    @return 返回 COMMITTED、CONFLICT 或未提交的错误状态
    */
    LockFreeMapStatus lockFreeMapSet(const KeyType& key, const ValueType& value);

    /** 并发执行有预算的回收，不等待暂停读者退出
    @param [in] budget 本次最多释放的退役索引条目数
    @param [out] releasedCapacity 本次实际释放整块内存所归还的槽位数
    @return 返回 OK 或策略、保护记录不足错误
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

    /** 构造函数
    */
    LockFreeMap();

    /** 析构函数，释放所有保留节点和值版本
    @note 析构时外部必须保证没有其它线程继续访问本对象
    */
    ~LockFreeMap();

    /** 返回下标访问代理
    @param [in] key 键
    @return 返回代理对象，支持map[key] = value写法
    @note 未提交时抛LockFreeMapError；使用lockFreeMapSet可取得详细结果
    */
    MapValueProxy<KeyType, ValueType, LessType, MaxLevel> operator[](const KeyType& key);

    /** 插入或覆盖键值
    @param [in] key 键
    @param [in] value 值
    @return 返回是否写入成功，内存分配失败或比较函数抛异常时返回false
    @note 可并发调用。旧值在最后一个受保护索引释放后析构，后续写入或维护分批回收
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
    @return 删除成功返回true，键不存在返回false
    @note 可并发调用。CAS移除当前索引项，旧索引由安全回收释放；分配失败也返回false
    */
    bool erase(const KeyType& key);

    /** 删除键，erase别名
    @param [in] key 键
    @return 删除成功返回true，键不存在返回false
    */
    bool remove(const KeyType& key);

    /** 查询键对应的值
    @param [in] key 键
    @param [out] value 查询成功时写入值副本
    @return 找到返回true，否则返回false
    @note 可并发调用
    */
    bool get(const KeyType& key, ValueType& value) const;

    /** 查询键对应的值，不存在时返回默认值
    @param [in] key 键
    @param [in] defaultValue 默认值
    @return 返回查到的值或默认值
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

    /** 按key升序遍历当前可见键值
    @param [in] function 回调函数，形如function(const KeyType&, const ValueType&)
    @return 返回回调次数
    @note 回调引用仅本次受保护调用内有效。允许重入但占用额外保护记录；异常返回已完成回调次数
    */
    template <typename FunctionType>
    size_t forEach(FunctionType function) const;

    /** 读取所有当前可见键
    @return 返回按key升序排列的键列表
    @note 可并发调用；并发写入时返回弱一致快照
    */
    std::vector<KeyType> keys() const;

    /** 读取所有当前可见值
    @return 返回按key升序排列的值列表
    @note 可并发调用；并发写入时返回弱一致快照
    */
    std::vector<ValueType> values() const;

    /** 读取所有当前可见键值对
    @return 返回按key升序排列的键值对列表
    @note 可并发调用；并发写入时返回弱一致快照
    */
    std::vector<std::pair<KeyType, ValueType>> toVector() const;

    /** 获取第一个可见键值
    @param [out] key 查询成功时写入最小键
    @param [out] value 查询成功时写入最小键对应的值
    @return 找到返回true，否则返回false
    @note 可并发调用；并发写入时返回弱一致结果
    */
    bool first(KeyType& key, ValueType& value) const;

    /** 获取最后一个可见键值
    @param [out] key 查询成功时写入最大键
    @param [out] value 查询成功时写入最大键对应的值
    @return 找到返回true，否则返回false
    @note 可并发调用；并发写入时返回弱一致结果
    */
    bool last(KeyType& key, ValueType& value) const;

    /** 查询第一个不小于指定键的可见键值
    @param [in] key 查询键
    @param [out] foundKey 查询成功时写入找到的键
    @param [out] value 查询成功时写入找到的值
    @return 找到返回true，否则返回false
    @note 可并发调用；并发写入时返回弱一致结果
    */
    bool lowerBound(const KeyType& key, KeyType& foundKey, ValueType& value) const;

    /** 查询第一个大于指定键的可见键值
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
    /** 执行一次写入尝试，所有可能抛异常的输出复制先于提交
    @param [in] key 待查键，仅最大项提取时允许 nullptr
    @param [in] value 候选值，删除时为 nullptr
    @param [in] token 可选条件凭据，无条件操作时为 nullptr
    @param [out] outputKey 可选提取键输出，不需要时为 nullptr
    @param [out] outputValue 可选实际值或提取值输出，不需要时为 nullptr
    @param [out] outputToken 可选实际条目凭据，不需要时为 nullptr
    @tparam Mode 0 覆盖，1 插入，2 删除，3 条件更新，4 条件删除，5 最大项提取
    @return 提交成功或明确的未提交状态
    */
    template <uint32_t Mode>
    LockFreeMapStatus write(const KeyType* key, const ValueType* value, const LockFreeMapToken* token,
        KeyType* outputKey, ValueType* outputValue, LockFreeMapToken* outputToken)
#ifdef __EMSCRIPTEN__
        noexcept
#endif
        ;

    /** 复制一个受保护条目
    @param [in] key 可选查询键，首尾查询时为 nullptr
    @param [out] outputKey 可选键输出，不需要时为 nullptr
    @param [out] outputValue 可选值输出，不需要时为 nullptr
    @param [out] outputToken 可选版本凭据输出，不需要时为 nullptr
    @param [in] mode 0 查询，1 首项，2 尾项，3 下界，4 上界
    @return 返回 OK、NOT_FOUND 或明确错误
    */
    LockFreeMapStatus read(const KeyType* key, KeyType* outputKey, ValueType* outputValue,
        LockFreeMapToken* outputToken, uint32_t mode) const
#ifdef __EMSCRIPTEN__
        noexcept
#endif
        ;

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