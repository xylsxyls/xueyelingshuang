#include "LockFreeHashMapMigration.h"
#include <algorithm>

size_t LockFreeHashMapMigration::fixedGrowth(const LockFreeHashMapGrowContext& context)
{
    (void)context;
    return 64;
}

size_t LockFreeHashMapMigrationGrow::operator()(const LockFreeHashMapGrowContext& context) const
{
    return (std::max)(size_t(64), context.currentCapacity / 2);
}

LockFreeHashMapMigrationChannel::LockFreeHashMapMigrationChannel(int pid, uint64_t identity, uint64_t generation) :
m_pid(pid), m_processIdentity(identity), m_receiverGeneration(generation), m_mapped(true)
{

}

bool LockFreeHashMapMigration::run()
{
    typedef std::shared_ptr<LockFreeHashMapMigrationChannel> Channel;
    LockFreeHashMap<int, Channel> pidCache;
    LockFreeHashMap<std::string, Channel> nameCache;
    LockFreeHashMapPoolConfig config;
    config.growPolicy = fixedGrowth;
    if (pidCache.configurePool(config).m_status != LockFreeHashMapOk ||
        nameCache.setPoolPolicies(LockFreeHashMapMigrationGrow(), LockFreeHashMapShrinkPolicy()).m_status != LockFreeHashMapOk)
    {
        return false;
    }
    typedef std::function<size_t(const LockFreeHashMapGrowContext& context)> ErasedGrow;
    std::shared_ptr<ErasedGrow> ownedPolicy = std::make_shared<ErasedGrow>(fixedGrowth);
    // 共享已有类型擦除回调，避免进入 v120 不安全的目标复制路径
    if (pidCache.setPoolPolicies(LockFreeHashMapGrowPolicy::fromShared(ownedPolicy), LockFreeHashMapShrinkPolicy()).m_status != LockFreeHashMapOk)
    {
        return false;
    }
    LockFreeHashMap<int, Channel>::Snapshot observed;
    Channel initialized = std::make_shared<LockFreeHashMapMigrationChannel>(42, 100, 1);
    // 真实发送者须先打开、映射并校验长度，再发布；失败候选始终由调用方拥有
    if (!pidCache.insertIfAbsent(42, initialized, observed).m_committed)
    {
        return false;
    }
    Channel inFlight = *observed.m_value;
    Channel replacement = std::make_shared<LockFreeHashMapMigrationChannel>(42, 200, 2);
    if (!pidCache.compareExchange(42, observed.m_version, replacement).m_committed ||
        pidCache.eraseIf(42, observed.m_version).m_status != LockFreeHashMapVersionMismatch)
    {
        return false;
    }
    LockFreeHashMap<std::string, Channel>::Snapshot mapping;
    if (!nameCache.insertIfAbsent("worker", replacement, mapping).m_committed)
    {
        return false;
    }
    Channel newest = std::make_shared<LockFreeHashMapMigrationChannel>(42, 300, 3);
    if (!nameCache.compareExchange("worker", mapping.m_version, newest).m_committed ||
        nameCache.eraseIf("worker", mapping.m_version).m_status != LockFreeHashMapVersionMismatch)
    {
        return false;
    }
    nameCache.clear();
    if (!mapping.m_value || !(*mapping.m_value)->m_mapped || inFlight->m_receiverGeneration != 1)
    {
        return false;
    }
    LockFreeHashMap<int, Channel>::Snapshot removed;
    // 线程注册表或请求表中，只有成功 take 的调用者取得终态处理权
    if (!pidCache.take(42, removed).m_committed || pidCache.contains(42))
    {
        return false;
    }
    return (*removed.m_value)->m_receiverGeneration == 2 && inFlight->m_mapped;
}