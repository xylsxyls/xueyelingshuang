#pragma once

#include <atomic>
#include <stdint.h>

// VS2013 x86's standard 64-bit load uses a locked RMW. A compile target that
// already requires SSE2 can read an aligned quadword with one MOVQ instead.
// Other compilers/architectures, including /arch:IA32, retain std::atomic.
#if defined(_MSC_VER) && !defined(__clang__) && _MSC_VER == 1800 && defined(_M_IX86) && _M_IX86_FP >= 2
#include <intrin.h>
#include <emmintrin.h>
#endif

// One scalar, with no padding bytes participating in compare/exchange.
template<class ValueType>
struct LockFreeQueueAtomicStorage
{
    // Complete scalar representation; the wrapper constructor rejects padding.
    ValueType m_value;
};

// std::atomic of a trivial scalar wrapper preserves standard atomic semantics.
// VS2013's integral/pointer specializations initialize with SC exchanges, even
// before an object is shared. Its generic constructor initializes this value
// directly; operations still use the same checked 4/8-byte atomic primitives.
template<class ValueType>
class LockFreeQueueAtomic
{
public:
    /** Initialize unpublished storage; never resets a concurrently used object
    @param [in] value Initial scalar bits
    */
    explicit LockFreeQueueAtomic(ValueType value = ValueType());

    /** Load with the requested standard memory order
    @param [in] order Valid standard atomic load order
    @return Observed scalar
    */
    ValueType load(std::memory_order order = std::memory_order_seq_cst) const;

    /** Publish a value with the requested standard memory order
    @param [in] value Scalar to publish
    @param [in] order Valid standard atomic store order
    */
    void store(ValueType value, std::memory_order order = std::memory_order_seq_cst);

    /** Transfer the value atomically
    @param [in] value Scalar to publish
    @param [in] order Valid standard atomic RMW order
    @return Previous scalar
    */
    ValueType exchange(ValueType value, std::memory_order order = std::memory_order_seq_cst);

    /** Weak compare/exchange
    @param [in,out] expected Expected scalar; receives observed scalar on failure
    @param [in] desired New scalar on success
    @param [in] success Valid success order
    @param [in] failure Valid failure order
    @return Whether replacement committed
    */
    bool compare_exchange_weak(ValueType& expected, ValueType desired,
        std::memory_order success, std::memory_order failure);

    /** Weak compare/exchange using the standard single-order overload
    @param [in,out] expected Expected scalar; receives observed scalar on failure
    @param [in] desired New scalar on success
    @param [in] order Valid RMW order; standard failure order is derived by the backend
    @return Whether replacement committed
    */
    bool compare_exchange_weak(ValueType& expected, ValueType desired,
        std::memory_order order = std::memory_order_seq_cst);

    /** Strong compare/exchange
    @param [in,out] expected Expected scalar; receives observed scalar on failure
    @param [in] desired New scalar on success
    @param [in] success Valid success order
    @param [in] failure Valid failure order
    @return Whether replacement committed
    */
    bool compare_exchange_strong(ValueType& expected, ValueType desired,
        std::memory_order success, std::memory_order failure);

    /** Strong compare/exchange using the standard single-order overload
    @param [in,out] expected Expected scalar; receives observed scalar on failure
    @param [in] desired New scalar on success
    @param [in] order Valid RMW order; standard failure order is derived by the backend
    @return Whether replacement committed
    */
    bool compare_exchange_strong(ValueType& expected, ValueType desired,
        std::memory_order order = std::memory_order_seq_cst);

    /** Query this actual atomic representation, including backend alignment
    */
    bool is_lock_free() const;
private:
    // Portable standard atomic backend, including its required alignment.
    std::atomic<LockFreeQueueAtomicStorage<ValueType>> m_value;

    /** Atomic ownership cannot be copied
    */
    LockFreeQueueAtomic(const LockFreeQueueAtomic& other) = delete;

    /** Atomic ownership cannot be assigned
    */
    LockFreeQueueAtomic& operator=(const LockFreeQueueAtomic& other) = delete;
};

#if defined(_MSC_VER) && !defined(__clang__) && _MSC_VER == 1800 && defined(_M_IX86) && _M_IX86_FP >= 2
template<>
class __declspec(align(8)) LockFreeQueueAtomic<uint64_t>
{
public:
    /** Initialize before publication
    @param [in] value Initial bits
    */
    explicit LockFreeQueueAtomic(uint64_t value = 0);

    /** Load one aligned quadword with compiler barriers and x86 load ordering
    @param [in] order Standard load order; implemented at least as acquire
    @return Observed value
    */
    uint64_t load(std::memory_order order = std::memory_order_seq_cst) const;

    /** Store through a full-barrier interlocked exchange
    @param [in] value New bits
    @param [in] order Requested order; strengthened to sequential consistency
    */
    void store(uint64_t value, std::memory_order order = std::memory_order_seq_cst);

    /** Exchange with full hardware and compiler barriers
    @param [in] value New bits
    @param [in] order Requested order; strengthened to sequential consistency
    @return Previous value
    */
    uint64_t exchange(uint64_t value, std::memory_order order = std::memory_order_seq_cst);

    /** Strong compare/exchange, with a full barrier on either outcome
    @param [in,out] expected Expected bits; receives observed bits on failure
    @param [in] desired New bits on success
    @param [in] success Requested success order
    @param [in] failure Requested failure order
    @return Whether replacement committed
    */
    bool compare_exchange_strong(uint64_t& expected, uint64_t desired,
        std::memory_order success, std::memory_order failure);

    /** Single-order strong compare/exchange
    @param [in,out] expected Expected bits; receives observed bits on failure
    @param [in] desired New bits on success
    @param [in] order Requested order
    @return Whether replacement committed
    */
    bool compare_exchange_strong(uint64_t& expected, uint64_t desired,
        std::memory_order order = std::memory_order_seq_cst);

    /** Weak compare/exchange implemented without spurious failure
    @param [in,out] expected Expected bits; receives observed bits on failure
    @param [in] desired New bits on success
    @param [in] success Requested success order
    @param [in] failure Requested failure order
    @return Whether replacement committed
    */
    bool compare_exchange_weak(uint64_t& expected, uint64_t desired,
        std::memory_order success, std::memory_order failure);

    /** Single-order weak compare/exchange
    @param [in,out] expected Expected bits; receives observed bits on failure
    @param [in] desired New bits on success
    @param [in] order Requested order
    @return Whether replacement committed
    */
    bool compare_exchange_weak(uint64_t& expected, uint64_t desired,
        std::memory_order order = std::memory_order_seq_cst);

    /** Check actual storage alignment required by both MOVQ and CMPXCHG8B
    @return Whether this instance satisfies the atomic backend contract
    */
    bool is_lock_free() const;
private:
    // This backend owns its representation; it never aliases std::atomic storage.
    __int64 m_value;

    /** Ownership cannot be copied
    */
    LockFreeQueueAtomic(const LockFreeQueueAtomic& other) = delete;

    /** Ownership cannot be assigned
    */
    LockFreeQueueAtomic& operator=(const LockFreeQueueAtomic& other) = delete;
};
#endif

#include "LockFreeQueueAtomic.inl"