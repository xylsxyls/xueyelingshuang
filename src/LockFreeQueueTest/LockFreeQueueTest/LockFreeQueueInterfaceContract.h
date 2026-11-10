#pragma once

#include "LockFreeQueueProducer.h"
#include "LockFreeQueueAPI.h"
#include <type_traits>

// One compile-time public contract for every native and Wasm test build.
// No platform branch, C wrapper, runtime object or exported symbol is introduced.
typedef LockFreeQueue<int32_t> LockFreeQueueContractType;

static_assert(std::is_same<decltype(&LockFreeQueueContractType::init),
    void (LockFreeQueueContractType::*)()>::value, "init signature changed");
static_assert(std::is_same<decltype(&LockFreeQueueContractType::destroy),
    void (LockFreeQueueContractType::*)()>::value, "destroy signature changed");
static_assert(std::is_same<decltype(&LockFreeQueueContractType::clear),
    void (LockFreeQueueContractType::*)()>::value, "clear signature changed");
static_assert(std::is_same<decltype(&LockFreeQueueContractType::push),
    LockFreeQueueResult (LockFreeQueueContractType::*)(const int32_t&)>::value, "push must return status on every platform");
static_assert(std::is_same<decltype(&LockFreeQueueContractType::tryPush),
    LockFreeQueueResult (LockFreeQueueContractType::*)(const int32_t&)>::value, "tryPush signature changed");
static_assert(std::is_same<decltype(&LockFreeQueueContractType::pop),
    bool (LockFreeQueueContractType::*)(int32_t*)>::value, "pop signature changed");
static_assert(std::is_same<decltype(&LockFreeQueueContractType::empty),
    bool (LockFreeQueueContractType::*)() const>::value, "empty signature changed");
static_assert(std::is_same<decltype(&LockFreeQueueContractType::size),
    uint64_t (LockFreeQueueContractType::*)() const>::value, "size signature changed");
static_assert(std::is_same<decltype(&LockFreeQueueContractType::tryInit),
    LockFreeQueueResult (LockFreeQueueContractType::*)()>::value, "tryInit signature changed");
static_assert(std::is_same<decltype(&LockFreeQueueContractType::configurePool),
    LockFreeQueueResult (LockFreeQueueContractType::*)(const LockFreeQueuePoolConfig&)>::value,
    "configurePool signature changed");
static_assert(std::is_same<decltype(&LockFreeQueueContractType::setGrowPolicy),
    LockFreeQueueResult (LockFreeQueueContractType::*)(const LockFreeQueueGrowPolicy&)>::value,
    "setGrowPolicy signature changed");
static_assert(std::is_same<decltype(&LockFreeQueueContractType::setShrinkPolicy),
    LockFreeQueueResult (LockFreeQueueContractType::*)(const LockFreeQueueShrinkPolicy&)>::value,
    "setShrinkPolicy signature changed");
static_assert(std::is_same<decltype(&LockFreeQueueContractType::maintain),
    LockFreeQueueMaintenanceResult (LockFreeQueueContractType::*)(size_t)>::value,
    "maintain signature changed");
static_assert(std::is_same<decltype(&LockFreeQueueContractType::poolStatistics),
    LockFreeQueuePoolStatistics (LockFreeQueueContractType::*)() const>::value,
    "poolStatistics signature changed");
static_assert(std::is_same<decltype(&LockFreeQueueContractType::atomicsLockFree),
    bool (LockFreeQueueContractType::*)() const>::value, "atomicsLockFree signature changed");

static_assert(std::is_default_constructible<LockFreeQueueContractType>::value, "Default construction required");
static_assert(std::is_same<decltype(LockFreeQueuePoolConfig::initialCapacity), size_t>::value,
    "initialCapacity type changed");
static_assert(std::is_same<decltype(LockFreeQueuePoolConfig::maximumCapacity), int64_t>::value,
    "maximumCapacity type changed");
static_assert(std::is_same<decltype(LockFreeQueuePoolConfig::growThreshold), size_t>::value,
    "growThreshold type changed");
static_assert(std::is_same<decltype(LockFreeQueuePoolConfig::maximumProducers), size_t>::value,
    "maximumProducers type changed");
static_assert(std::is_same<LockFreeQueueGrowPolicy,
    std::function<size_t(const LockFreeQueueGrowContext&)>>::value, "Growth policy signature changed");
static_assert(std::is_same<LockFreeQueueShrinkPolicy,
    std::function<LockFreeQueueShrinkDecision(const LockFreeQueueShrinkContext&)>>::value,
    "Shrink policy signature changed");

static_assert(LockFreeQueueClockUnavailable == 10 && LockFreeQueueSequenceExhausted == 11,
    "Existing result values and appended sequence result must remain stable");

typedef LockFreeQueueProducer<int32_t> LockFreeQueueProducerContract;
static_assert(std::is_same<decltype(&LockFreeQueueContractType::tryAcquireProducer),
    LockFreeQueueResult (LockFreeQueueContractType::*)(LockFreeQueueProducerContract&)>::value,
    "Producer acquisition signature changed");
static_assert(std::is_same<decltype(&LockFreeQueueProducerContract::tryPush),
    LockFreeQueueResult (LockFreeQueueProducerContract::*)(const int32_t&)>::value,
    "Producer tryPush signature changed");
static_assert(std::is_same<decltype(&LockFreeQueueProducerContract::push),
    LockFreeQueueResult (LockFreeQueueProducerContract::*)(const int32_t&)>::value, "Producer push must return status");
static_assert(std::is_same<decltype(&LockFreeQueueProducerContract::release),
    void (LockFreeQueueProducerContract::*)()>::value, "Producer release signature changed");
static_assert(std::is_same<decltype(&LockFreeQueueProducerContract::attached),
    bool (LockFreeQueueProducerContract::*)() const>::value, "Producer attached signature changed");
static_assert(!std::is_copy_constructible<LockFreeQueueProducerContract>::value &&
    !std::is_move_constructible<LockFreeQueueProducerContract>::value, "Producer session must have one stable owner");