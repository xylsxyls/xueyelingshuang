#ifndef LOCK_FREE_MAP_CONTRACT_INL
#define LOCK_FREE_MAP_CONTRACT_INL

inline LockFreeMapError::LockFreeMapError(LockFreeMapStatus status) :
m_status(status)
{

}

inline const char* LockFreeMapError::what() const throw()
{
    return "LockFreeMap assignment was not committed";
}

inline LockFreeMapToken::LockFreeMapToken() :
m_version(0)
{

}

inline LockFreeMapShrinkDecision::LockFreeMapShrinkDecision(size_t count, uint64_t milliseconds) :
shrinkCount(count),
idleMilliseconds(milliseconds)
{

}

inline LockFreeMapPoolConfig::LockFreeMapPoolConfig() :
initialCapacity(16),
growFreeThreshold(0),
maximumCapacity(-1),
blockRecordLimit(256),
/** 计算默认增长增量，由池按剩余额度裁剪
@param [in] context 借用的扩缩容观察上下文，不代表容量预留
@return 当前容量非零时返回当前容量，否则返回初始容量或16
*/
grow([](const LockFreeMapGrowContext& context) -> size_t
{
    // 返回新增槽位数，池按剩余额度截断；输入为观察上下文
    return context.currentCapacity == 0 ? (context.initialCapacity == 0 ? 16 : context.initialCapacity) : context.currentCapacity;
}),
shrink(&LockFreeMapPoolConfig::defaultShrink)
{

}

inline LockFreeMapShrinkDecision LockFreeMapPoolConfig::defaultShrink(const LockFreeMapShrinkContext& context)
{
    (void)context;
    return LockFreeMapShrinkDecision();
}

inline LockFreeMapConfig::LockFreeMapConfig() :
operationRecordLimit(64),
maintenanceBudget(64),
maintenanceInterval(64),
capacityFastRetries(4096),
capacitySleepMilliseconds(1)
{

}

template <typename Target, typename Function>
inline LockFreeMapStatus LockFreeMapPoolConfig::copyPolicy(Target& target, const Function& policy)
{
    try
    {
        Target replacement(policy);
        target.swap(replacement);
        return LOCK_FREE_MAP_OK;
    }
    catch (const std::bad_alloc&)
    {
        return LOCK_FREE_MAP_ALLOCATION_FAILURE;
    }
    catch (...)
    {
        return LOCK_FREE_MAP_USER_EXCEPTION;
    }
}

inline LockFreeMapStatus LockFreeMapPoolConfig::setCapacity(size_t initial, int64_t maximum)
{
    if (maximum < -1 || (maximum >= 0 && initial > static_cast<uint64_t>(maximum)))
    {
        return LOCK_FREE_MAP_INVALID;
    }
    initialCapacity = initial;
    maximumCapacity = maximum;
    return LOCK_FREE_MAP_OK;
}

inline void LockFreeMapPoolConfig::setGrowFreeThreshold(size_t threshold)
{
    growFreeThreshold = threshold;
}

inline LockFreeMapStatus LockFreeMapPoolConfig::setBlockRecordLimit(size_t count)
{
    if (count == 0)
    {
        return LOCK_FREE_MAP_INVALID;
    }
    blockRecordLimit = count;
    return LOCK_FREE_MAP_OK;
}

template <typename Function>
inline LockFreeMapStatus LockFreeMapPoolConfig::setGrow(const Function& policy)
{
    return copyPolicy(grow, policy);
}

template <typename Function>
inline LockFreeMapStatus LockFreeMapPoolConfig::setShrink(const Function& policy)
{
    return copyPolicy(shrink, policy);
}

template <typename Function>
inline LockFreeMapStatus LockFreeMapPoolConfig::setOnGrowth(const Function& policy)
{
    return copyPolicy(m_onGrowth, policy);
}

inline LockFreeMapStatus LockFreeMapConfig::setOperationRecordLimit(size_t count)
{
    if (count == 0)
    {
        return LOCK_FREE_MAP_INVALID;
    }
    operationRecordLimit = count;
    return LOCK_FREE_MAP_OK;
}

inline LockFreeMapStatus LockFreeMapConfig::setMaintenanceBudget(size_t count)
{
    if (count == 0)
    {
        return LOCK_FREE_MAP_INVALID;
    }
    maintenanceBudget = count;
    return LOCK_FREE_MAP_OK;
}

inline LockFreeMapStatus LockFreeMapConfig::setMaintenanceInterval(uint32_t attempts)
{
    if (attempts == 0)
    {
        return LOCK_FREE_MAP_INVALID;
    }
    maintenanceInterval = attempts;
    return LOCK_FREE_MAP_OK;
}

inline LockFreeMapStatus LockFreeMapConfig::setCapacityWait(uint32_t fastRetries, int32_t sleepMilliseconds)
{
    if (sleepMilliseconds <= 0)
    {
        return LOCK_FREE_MAP_INVALID;
    }
    capacityFastRetries = fastRetries;
    capacitySleepMilliseconds = sleepMilliseconds;
    return LOCK_FREE_MAP_OK;
}

#endif