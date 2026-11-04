#pragma once
#include "LockFreeHashMapTypes.h"
#include <stdint.h>
#include <cstring>
#include <type_traits>
#include <atomic>

/** 标量值与版本，版本为 0 时表示关闭或后继转移描述，不拥有资源
*/
struct LockFreeHashMapScalarState
{
    // 值位编码，版本为 0 时为后继槽地址，0 地址表示关闭
    uint64_t m_bits;
    // 完整值身份，0 表示非活动值状态
    uint64_t m_version;
};

/** 可移植 64 位标量字，每次槽复用只设置一个不可变版本前缀
*/
struct LockFreeHashMapAtomicWord
{
public:
    // 奇数字的高 32 位为值、低部为 31 位版本；偶数非零字为后继地址，0 永久关闭
    std::atomic<uint64_t> m_word;
    // 本次槽生命周期内不变的高位版本前缀
    uint64_t m_versionBase;
public:
    /** 检查所需原子宽度，与池和桶的后端一致
    @return 原生 64 位原子真正无锁时为 true
    */
    static bool supported();

    /** 初始化尚未发布槽位的前缀和值
    @param [in] value 初始完整状态，必须由槽的唯一拥有者调用
    */
    void initialize(const LockFreeHashMapScalarState& value);

    /** 原子读取标量值及完整版本
    @return 原子字与不可变前缀组成的一致状态
    */
    LockFreeHashMapScalarState load();

    /** 判断此槽的版本前缀能否表示候选状态
    @param [in] value 新值、关闭状态或转移描述
    @return 无需改变前缀即可编码时为 true
    */
    bool accepts(const LockFreeHashMapScalarState& value) const;

    /** 比较完整标量状态并替换
    @param [in,out] expected 期望状态，返回时更新为观察到的旧状态
    @param [in] desired 新状态，前缀不兼容时拒绝且不写入
    @return 本次原子替换成功为 true
    */
    bool compareExchange(LockFreeHashMapScalarState& expected, const LockFreeHashMapScalarState& desired);

    /** 编码与当前槽兼容的状态，指针状态不使用前缀
    @param [in] value 标量有效位不超过 32 位，指针状态必须是有效偶地址或 0
    @return 原子字编码，调用方负责 accepts 校验
    */
    static uint64_t encode(const LockFreeHashMapScalarState& value);

    /** 使用此槽不可变前缀解码原子字
    @param [in] word 本次槽生命周期内读取的原子字
    @return 完整值身份或关闭、转移描述
    */
    LockFreeHashMapScalarState decode(uint64_t word) const;
};

/** 非标量类型的空后端，无原子字和扩展对齐开销，不拥有资源
*/
struct LockFreeHashMapDisabledWord
{
public:
    /** 禁用后端忽略初始化
    @param [in] value 未使用的状态，不借用任何地址
    */
    void initialize(const LockFreeHashMapScalarState& value);

    /** 返回无效状态，不读取值
    @return 全零的关闭状态
    */
    LockFreeHashMapScalarState load();

    /** 拒绝标量状态编码
    @param [in] value 未使用的候选状态
    @return 始终为 false
    */
    bool accepts(const LockFreeHashMapScalarState& value) const;

    /** 拒绝禁用后端的替换，不修改参数
    @param [in,out] expected 保持原值
    @param [in] desired 未使用的候选状态
    @return 始终为 false
    */
    bool compareExchange(LockFreeHashMapScalarState& expected, const LockFreeHashMapScalarState& desired);
};

/** 非适用值类型的编码接口，不要求默认构造或按位复制
*/
template <typename ValueType, bool Enabled = std::is_scalar<ValueType>::value && sizeof(ValueType) <= 4 &&
    !std::is_const<ValueType>::value && !std::is_volatile<ValueType>::value>
struct LockFreeHashMapScalar
{
public:
    typedef LockFreeHashMapDisabledWord Word;

    /** 查询禁用后端
    @return 始终为 false
    */
    static bool enabled();

    /** 为模板通用分支提供不执行用户代码的占位编码
    @param [in] value 不读取的值
    @return 始终为 0，不代表有效标量
    */
    static uint64_t encode(const ValueType& value);

    /** 拒绝不可达的禁用解码分支，调用点由不可变后端配置隔离
    @param [in] bits 未使用的位编码
    @return 不返回，误用抛出 Unsupported，由上层异常边界处理
    */
    static ValueType decode(uint64_t bits);

    /** 禁用类型不进行按位输出
    @param [out] output 保持原值
    @param [in] bits 未使用的位编码
    @return 始终为 false
    */
    static bool copy(ValueType& output, uint64_t bits);
};

/** 不超过四字节的非 cv 标量编码，无资源所有权，兼容大小端
*/
template <typename ValueType>
struct LockFreeHashMapScalar<ValueType, true>
{
public:
    typedef LockFreeHashMapAtomicWord Word;

    /** 检查标量后端能力
    @return 64 位原子真正无锁时为 true
    */
    static bool enabled();

    /** 通过 32 位对象保存值的对象表示
    @param [in] value 待编码标量
    @return 低 32 位有效的编码，高位为 0
    */
    static uint64_t encode(const ValueType& value);

    /** 还原此前由相同值类型编码的标量
    @param [in] bits 有效编码的低 32 位
    @return 原始标量值，不执行用户构造函数
    */
    static ValueType decode(uint64_t bits);

    /** 将标量编码还原到调用方对象
    @param [out] output 写入解码值
    @param [in] bits 同类型的有效编码
    @return 始终为 true，不执行用户赋值函数
    */
    static bool copy(ValueType& output, uint64_t bits);
};

#include "LockFreeHashMapAtomic.inl"