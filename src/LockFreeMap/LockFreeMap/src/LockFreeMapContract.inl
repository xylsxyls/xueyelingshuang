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
maintenanceBudget(64)
{

}

#endif