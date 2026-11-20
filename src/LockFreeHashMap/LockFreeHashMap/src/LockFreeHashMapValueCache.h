#pragma once
#include "LockFreeHashMapTypes.h"
#include "LockFreeHashMapMacro.h"
#include <atomic>
#include <cstring>
#include <limits>
#include <new>
#include <type_traits>

/** 固定副本格，只保存原子整数，不借用池节点或用户资源
身份按顺序一致执行；载荷逐字段发布和获取，使读到新载荷必然在末次身份检查中看见此前失效
发布占用只影响缓存，不阻塞容器操作
*/
struct LockFreeHashMapValueCacheEntry
{
public:
    // 零表示无效；非零偶数为写入前失效身份，奇数为该身份的已发布副本
    LOCKFREEHASHMAP_ALIGN_64 std::atomic<uint64_t> m_stamp;
    // 完整清空代次，不与键值打包截断
    LOCKFREEHASHMAP_ALIGN_64 std::atomic<uint64_t> m_generation;
    // 完整整数键，不截断高位
    LOCKFREEHASHMAP_ALIGN_64 std::atomic<uint64_t> m_key;
    // 完整整数值对象表示，未使用字节为零
    LOCKFREEHASHMAP_ALIGN_64 std::atomic<uint64_t> m_value;
    // 单次尝试的发布权，持有者暂停时其他操作跳过副本发布
    std::atomic<bool> m_publishing;
public:
    /** 初始化空格，不申请资源
    */
    LockFreeHashMapValueCacheEntry();
};

/** 窄整数键值共用一个原子字的固定副本格，省去未使用的宽值字段
完整身份、清空代次和发布权仍独立保存，协议与宽副本格一致
*/
struct LockFreeHashMapPackedValueCacheEntry
{
public:
    // 偶数表示失效身份，奇数表示该身份的已发布副本
    LOCKFREEHASHMAP_ALIGN_64 std::atomic<uint64_t> m_stamp;
    // 完整清空代次，不压缩或截断
    LOCKFREEHASHMAP_ALIGN_64 std::atomic<uint64_t> m_generation;
    // 低32位为完整窄键，高32位为窄值的对象表示
    LOCKFREEHASHMAP_ALIGN_64 std::atomic<uint64_t> m_key;
    // 单次尝试的发布权，持有者暂停时其他操作跳过副本发布
    std::atomic<bool> m_publishing;
public:
    /** 构造空副本格，不申请资源
    */
    LockFreeHashMapPackedValueCacheEntry();
};

/** 不支持原子整数副本的类型使用空实现，不增加任何系统分配
*/
template <typename KeyType, typename ValueType, typename EqualType,
    bool Enabled = std::is_integral<KeyType>::value && std::is_integral<ValueType>::value &&
    sizeof(KeyType) <= sizeof(uint64_t) && sizeof(ValueType) <= sizeof(uint64_t) &&
    std::is_same<EqualType, std::equal_to<KeyType>>::value>
class LockFreeHashMapValueCache
{
public:
    /** 不满足整数副本条件的类型保持关闭，配置检查由池统一执行
    @param [in] count 请求的固定格数
    @return Ok，不申请内存
    */
    LockFreeHashMapStatus initialize(size_t count);

    /** 未启用类型的原子代次查询直接返回，不读取容器代次
    @param [in] key 查询键
    @param [in] generation 容器代次原子变量，不读取
    @param [out] value 可空的输出，不修改
    @return false
    */
    bool lookup(const KeyType& key, const std::atomic<uint64_t>& generation, ValueType* value) const;

    /** 无缓存时不执行失效操作
    @param [in] key 查询键，不调用用户哈希或比较器
    @param [in] version 非零候选身份，删除使用零
    */
    void invalidate(const KeyType& key, uint64_t version);

    /** 无缓存时不执行发布操作
    @param [in] key 已提交键
    @param [in] value 已提交值
    @param [in] generation 提交代次
    @param [in] version 本次提交身份
    */
    void publish(const KeyType& key, const ValueType& value, uint64_t generation, uint64_t version);

    /** 交换两个空配置
    @param [in,out] other 同类型对象
    */
    void swap(LockFreeHashMapValueCache& other);

    /** 查询实际缓存格数
    @return 零
    */
    size_t count() const;

    /** 查询实际副本数组字节数
    @return 零，未启用副本时不申请数组
    */
    size_t bytes() const;
};

/** 内置整数副本缓存，失效先于结构提交，提交后单次尝试发布，不提供排序
*/
template <typename KeyType, typename ValueType, typename EqualType>
class LockFreeHashMapValueCache<KeyType, ValueType, EqualType, true>
{
private:
    // 只打包可完整装入一个64位原子的键值，不压缩版本或清空代次
    static const bool kPacked = sizeof(KeyType) <= sizeof(uint32_t) && sizeof(ValueType) <= sizeof(uint32_t);
    // 按实际载荷选择副本格，窄类型不保留未使用的独立值原子
    typedef typename std::conditional<kPacked, LockFreeHashMapPackedValueCacheEntry, LockFreeHashMapValueCacheEntry>::type Entry;
    // 与实际存储位宽一致，memcpy往返保留大小端机器上的窄整数对象表示
    typedef typename std::conditional<kPacked, uint32_t, uint64_t>::type ValueWord;
public:
    /** 构造关闭的缓存
    */
    LockFreeHashMapValueCache();

    /** 外部静止后释放固定副本数组
    */
    ~LockFreeHashMapValueCache();

    /** 仅对未分配数组的实例初始化；已有数组时拒绝重复初始化并保持原内容
    @param [in] count 二次幂格数，零表示关闭
    @return 初始化状态，不向外抛异常；首次申请失败保留空配置
    */
    LockFreeHashMapStatus initialize(size_t count);

    /** 只检查一次缓存容量；关闭缓存时不读取容器代次
    @param [in] key 完整查询键
    @param [in] generation 容器代次原子变量，仅在副本有效且键匹配后读取
    @param [out] value 非空时写入整数副本，未命中不修改
    @return 同身份、同代次且完整键相同时为 true
    */
    bool lookup(const KeyType& key, const std::atomic<uint64_t>& generation, ValueType* value) const;

    /** 每次结构提交尝试前先使对应副本格失效
    @param [in] key 完整键，缓存位置独立于结构桶位置
    @param [in] version 本次唯一候选身份；零及超出副本编码范围时只失效
    */
    void invalidate(const KeyType& key, uint64_t version);

    /** 结构提交成功后尝试发布；失去发布权或失效身份已变化时跳过，不等待其他线程
    @param [in] key 已提交完整整数键
    @param [in] value 已提交完整整数值
    @param [in] generation 提交代次
    @param [in] version 本次唯一候选身份
    */
    void publish(const KeyType& key, const ValueType& value, uint64_t generation, uint64_t version);

    /** 外部静止时交换数组所有权，不分配
    @param [in,out] other 同类型独占缓存
    */
    void swap(LockFreeHashMapValueCache& other);

    /** 查询实际固定格数
    @return 已分配格数
    */
    size_t count() const;

    /** 查询已经申请的副本数组字节数，包含对齐余量，不含拥有者对象本身
    @return 固定格载荷加六十三字节对齐余量；关闭时为零
    */
    size_t bytes() const;
private:
    /** 从已经读取的打包键值字提取窄值，不再次访问共享内存
    @param [in] entry 当前受身份协议保护的副本格，不额外读取
    @param [in] observedKey 已经读取的完整打包字
    @param [in] copy 是否需要输出值，由调用者决定是否使用返回值
    @return 窄值对象表示
    */
    static ValueWord loadValue(const LockFreeHashMapPackedValueCacheEntry& entry, uint64_t observedKey, bool copy);

    /** 仅在需要输出时读取独立宽值字段
    @param [in] entry 当前受身份协议保护的副本格
    @param [in] observedKey 已经读取的键，不参与值提取
    @param [in] copy 是否需要输出值
    @return 完整值对象表示；不需要输出时为零
    */
    static ValueWord loadValue(const LockFreeHashMapValueCacheEntry& entry, uint64_t observedKey, bool copy);

    /** 发布已合并的窄键值字，最终有效身份仍由调用者提交
    @param [in,out] entry 本次取得发布权的格
    @param [in] key 完整窄键
    @param [in] value 完整窄值
    */
    static void storeValue(LockFreeHashMapPackedValueCacheEntry& entry, const KeyType& key, const ValueType& value);

    /** 分别发布完整宽键和值，最终有效身份仍由调用者提交
    @param [in,out] entry 本次取得发布权的格
    @param [in] key 完整键
    @param [in] value 完整值
    */
    static void storeValue(LockFreeHashMapValueCacheEntry& entry, const KeyType& key, const ValueType& value);

    /** 生成缓存索引位；窄整数直接使用有效位，宽整数保留高低位折叠
    @param [in] key 完整整数键，负数到无符号的转换按标准取模
    @return 尚未按容量掩码截取的索引位，完整键仍由查询核对
    */
    static size_t keyBits(const KeyType& key);

    /** 禁止复制独占副本数组
    @param [in] other 不可复制对象
    */
    LockFreeHashMapValueCache(const LockFreeHashMapValueCache& other);

    /** 禁止复制覆盖数组所有权
    @param [in] other 不可复制对象
    @return 无实现
    */
    LockFreeHashMapValueCache& operator=(const LockFreeHashMapValueCache& other);
private:
    // 独占固定数组，运行期不扩展或释放
    Entry* m_entries;
    // 固定格数，关闭时为零
    size_t m_count;
    // 原始字符区所有者；m_entries在其中按缓存行对齐，释放时仍使用原始地址
    unsigned char* m_allocation;
};

#include "LockFreeHashMapValueCache.inl"