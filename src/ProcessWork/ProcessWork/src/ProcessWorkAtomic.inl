#pragma once
#ifdef _MSC_VER
#include <intrin.h>
#endif

inline int32_t ProcessWorkAtomic::load(const int32_t* value)
{
#if defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86))
    // x86的对齐普通读取不可撕裂，TSO与编译器屏障共同提供acquire。
    const int32_t result = *reinterpret_cast<volatile const int32_t*>(value);
    _ReadWriteBarrier();
    return result;
#elif defined(_MSC_VER)
    return _InterlockedCompareExchange(reinterpret_cast<volatile long*>(const_cast<int32_t*>(value)), 0, 0);
#else
    return __atomic_load_n(value, __ATOMIC_ACQUIRE);
#endif
}

inline int64_t ProcessWorkAtomic::load(const int64_t* value)
{
#if defined(_MSC_VER) && defined(_M_X64)
    const int64_t result = *reinterpret_cast<volatile const int64_t*>(value);
    _ReadWriteBarrier();
    return result;
#elif defined(_MSC_VER)
    // 32位Windows不可用两次普通读取拼成64位快照。
    return _InterlockedCompareExchange64(const_cast<volatile int64_t*>(value), 0, 0);
#else
    return __atomic_load_n(value, __ATOMIC_ACQUIRE);
#endif
}

inline void ProcessWorkAtomic::store(int64_t* value, int64_t desired)
{
#if defined(_MSC_VER) && defined(_M_X64)
    _ReadWriteBarrier();
    *reinterpret_cast<volatile int64_t*>(value) = desired;
#elif defined(_MSC_VER)
    // v120的32位intrin.h没有Exchange64；使用不可撕裂的CAS交换。
    int64_t expected = _InterlockedCompareExchange64(value, 0, 0);
    for (;;)
    {
        const int64_t observed = _InterlockedCompareExchange64(value, desired, expected);
        if (observed == expected)
        {
            return;
        }
        expected = observed;
    }
#else
    __atomic_store_n(value, desired, __ATOMIC_RELEASE);
#endif
}

inline bool ProcessWorkAtomic::replace(int32_t* value, int32_t expected, int32_t desired)
{
#ifdef _MSC_VER
    return _InterlockedCompareExchange(reinterpret_cast<volatile long*>(value), desired, expected) == expected;
#else
    return __atomic_compare_exchange_n(value, &expected, desired, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE);
#endif
}

inline bool ProcessWorkAtomic::replace(int64_t* value, int64_t expected, int64_t desired)
{
    return compareExchange(value, expected, desired) == expected;
}

inline int64_t ProcessWorkAtomic::compareExchange(int64_t* value, int64_t expected, int64_t desired)
{
#ifdef _MSC_VER
    return _InterlockedCompareExchange64(value, desired, expected);
#else
    __atomic_compare_exchange_n(value, &expected, desired, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE);
    return expected;
#endif
}

inline int64_t ProcessWorkAtomic::increment(int64_t* value)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    // v120 的 x86 intrinsic 不提供 Increment64；用同一原生 CAS 实现整宽加一。
    int64_t expected = load(value);
    for (;;)
    {
        const int64_t desired = static_cast<int64_t>(static_cast<uint64_t>(expected) + 1);
        const int64_t observed = compareExchange(value, expected, desired);
        if (observed == expected)
        {
            return desired;
        }
        expected = observed;
        relax();
    }
#elif defined(_MSC_VER)
    return _InterlockedIncrement64(value);
#else
    return __atomic_add_fetch(value, int64_t(1), __ATOMIC_ACQ_REL);
#endif
}

inline void ProcessWorkAtomic::relax()
{
#if defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86))
    _mm_pause();
#elif defined(__i386__) || defined(__x86_64__)
    __builtin_ia32_pause();
#elif defined(__aarch64__) || defined(__arm__)
    __asm__ __volatile__("yield");
#elif defined(_MSC_VER)
    _ReadWriteBarrier();
#endif
}