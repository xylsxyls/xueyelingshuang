#pragma once

inline bool LockFreeHashMapAtomicWord::supported()
{
    std::atomic<uint64_t> word(0);
    return word.is_lock_free();
}

inline void LockFreeHashMapAtomicWord::initialize(const LockFreeHashMapScalarState& value)
{
    m_versionBase = value.m_version & ~UINT64_C(0x7fffffff);
    m_word.store(encode(value), std::memory_order_relaxed);
}

inline uint64_t LockFreeHashMapAtomicWord::encode(const LockFreeHashMapScalarState& value)
{
    return value.m_version == 0 ? value.m_bits : (value.m_bits << 32) | ((value.m_version & UINT64_C(0x7fffffff)) << 1) | 1;
}

inline LockFreeHashMapScalarState LockFreeHashMapAtomicWord::decode(uint64_t word) const
{
    LockFreeHashMapScalarState result = {};
    if ((word & 1) != 0)
    {
        result.m_bits = word >> 32;
        result.m_version = m_versionBase | ((word >> 1) & UINT64_C(0x7fffffff));
    }
    else
    {
        result.m_bits = word;
    }
    return result;
}

inline LockFreeHashMapScalarState LockFreeHashMapAtomicWord::load()
{
    return decode(m_word.load());
}

inline bool LockFreeHashMapAtomicWord::accepts(const LockFreeHashMapScalarState& value) const
{
    return value.m_version == 0 || (value.m_version & ~UINT64_C(0x7fffffff)) == m_versionBase;
}

inline bool LockFreeHashMapAtomicWord::compareExchange(LockFreeHashMapScalarState& expected, const LockFreeHashMapScalarState& desired)
{
    uint64_t old = encode(expected);
    if (!accepts(desired))
    {
        expected = load();
        return false;
    }
    bool changed = m_word.compare_exchange_strong(old, encode(desired));
    expected = decode(old);
    return changed;
}

inline void LockFreeHashMapDisabledWord::initialize(const LockFreeHashMapScalarState& value)
{
    (void)value;
}

inline LockFreeHashMapScalarState LockFreeHashMapDisabledWord::load()
{
    LockFreeHashMapScalarState result = {};
    return result;
}

inline bool LockFreeHashMapDisabledWord::accepts(const LockFreeHashMapScalarState& value) const
{
    (void)value;
    return false;
}

inline bool LockFreeHashMapDisabledWord::compareExchange(LockFreeHashMapScalarState& expected, const LockFreeHashMapScalarState& desired)
{
    (void)expected;
    (void)desired;
    return false;
}

template <typename V, bool Enabled>
bool LockFreeHashMapScalar<V, Enabled>::enabled()
{
    return false;
}

template <typename V, bool Enabled>
uint64_t LockFreeHashMapScalar<V, Enabled>::encode(const V& value)
{
    (void)value;
    return 0;
}

template <typename V, bool Enabled>
V LockFreeHashMapScalar<V, Enabled>::decode(uint64_t bits)
{
    (void)bits;
    // 运行中不可变的后端标记为 false，因此合法调用不会进入此分支
    throw LockFreeHashMapUnsupported;
}

template <typename V>
bool LockFreeHashMapScalar<V, true>::enabled()
{
    return LockFreeHashMapAtomicWord::supported();
}

template <typename V>
uint64_t LockFreeHashMapScalar<V, true>::encode(const V& value)
{
    // 通过 32 位对象复制，保证不足四字节的标量也能在大端目标往返还原
    uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(V));
    return static_cast<uint64_t>(bits);
}

template <typename V>
V LockFreeHashMapScalar<V, true>::decode(uint64_t bits)
{
    V value = V();
    uint32_t storage = static_cast<uint32_t>(bits);
    std::memcpy(&value, &storage, sizeof(V));
    return value;
}

template <typename V, bool Enabled>
bool LockFreeHashMapScalar<V, Enabled>::copy(V& output, uint64_t bits)
{
    (void)output;
    (void)bits;
    return false;
}

template <typename V>
bool LockFreeHashMapScalar<V, true>::copy(V& output, uint64_t bits)
{
    output = decode(bits);
    return true;
}