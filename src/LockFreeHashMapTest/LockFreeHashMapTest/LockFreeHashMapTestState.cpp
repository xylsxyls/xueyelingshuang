#include "LockFreeHashMapTestState.h"
#include "LockFreeHashMapTestBinding.h"

LockFreeHashMapTestState::IntCounter LockFreeHashMapTestState::s_sequence = nullptr;
LockFreeHashMapTestState::IntCounter LockFreeHashMapTestState::s_generation = nullptr;
LockFreeHashMapTestState::CollisionCounter LockFreeHashMapTestState::s_historySequence = nullptr;
LockFreeHashMapTestState::StoragePointer LockFreeHashMapTestState::s_storage = nullptr;
LockFreeHashMapTestState::ValidPointer LockFreeHashMapTestState::s_countValid = nullptr;
LockFreeHashMapTestState::AccountPointer LockFreeHashMapTestState::s_account = nullptr;
LockFreeHashMapTestState::ClockPointer LockFreeHashMapTestState::s_clock = nullptr;
LockFreeHashMapTestState::EntriesPointer LockFreeHashMapTestState::s_entries = nullptr;

// 访问豁免仅用于显式实例化的实参；函数体始终通过正常类型的成员指针操作真实对象
// 标准依据：https://eel.is/c++draft/temp.spec.general#6
template class LockFreeHashMapTestBinding<LockFreeHashMapTestState::IntCounter,
    &LockFreeHashMapTestState::IntMap::m_sequence, &LockFreeHashMapTestState::s_sequence>;
template class LockFreeHashMapTestBinding<LockFreeHashMapTestState::IntCounter,
    &LockFreeHashMapTestState::IntMap::m_generation, &LockFreeHashMapTestState::s_generation>;
template class LockFreeHashMapTestBinding<LockFreeHashMapTestState::CollisionCounter,
    &LockFreeHashMapTestState::CollisionMap::m_sequence, &LockFreeHashMapTestState::s_historySequence>;
template class LockFreeHashMapTestBinding<LockFreeHashMapTestState::StoragePointer,
    &LockFreeHashMapTestState::IntMap::m_storage, &LockFreeHashMapTestState::s_storage>;
template class LockFreeHashMapTestBinding<LockFreeHashMapTestState::ValidPointer,
    &LockFreeHashMapTestState::IntMap::m_countValid, &LockFreeHashMapTestState::s_countValid>;
template class LockFreeHashMapTestBinding<LockFreeHashMapTestState::AccountPointer,
    &LockFreeHashMapTestState::IntMap::account, &LockFreeHashMapTestState::s_account>;
template class LockFreeHashMapTestBinding<LockFreeHashMapTestState::ClockPointer,
    &LockFreeHashMapEpoch::m_clock, &LockFreeHashMapTestState::s_clock>;
template class LockFreeHashMapTestBinding<LockFreeHashMapTestState::EntriesPointer,
    &LockFreeHashMapTestState::ScalarCache::m_entries, &LockFreeHashMapTestState::s_entries>;

void LockFreeHashMapTestState::require(bool condition)
{
    if (!condition)
    {
        throw LockFreeHashMapTestCheckFailed;
    }
}

std::atomic<uint64_t>& LockFreeHashMapTestState::sequence(IntMap& map)
{
    require(s_sequence != nullptr);
    return map.*s_sequence;
}

std::atomic<uint64_t>& LockFreeHashMapTestState::historySequence(CollisionMap& map)
{
    require(s_historySequence != nullptr);
    return map.*s_historySequence;
}

std::atomic<uint64_t>& LockFreeHashMapTestState::generation(IntMap& map)
{
    require(s_generation != nullptr);
    return map.*s_generation;
}

LockFreeHashMapTestState::IntMap::Storage& LockFreeHashMapTestState::storage(IntMap& map)
{
    require(s_storage != nullptr);
    require(map.*s_storage != nullptr);
    return *(map.*s_storage);
}

std::atomic<bool>& LockFreeHashMapTestState::countValid(IntMap& map)
{
    require(s_countValid != nullptr);
    return map.*s_countValid;
}

void LockFreeHashMapTestState::account(IntMap& map, LockFreeHashMapEpochRecord& record,
    uint64_t generation, int64_t difference)
{
    require(s_account != nullptr);
    (map.*s_account)(record, generation, difference);
}

std::atomic<uint64_t>& LockFreeHashMapTestState::clock(LockFreeHashMapEpoch& domain)
{
    require(s_clock != nullptr);
    return domain.*s_clock;
}

LockFreeHashMapValueCacheEntry& LockFreeHashMapTestState::first(ScalarCache& cache)
{
    require(s_entries != nullptr);
    require(cache.count() != 0 && cache.*s_entries != nullptr);
    return (cache.*s_entries)[0];
}