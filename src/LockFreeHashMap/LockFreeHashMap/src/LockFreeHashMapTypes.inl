#pragma once

inline LockFreeHashMapAssignmentError::LockFreeHashMapAssignmentError(LockFreeHashMapStatus status) throw() :
m_status(status)
{

}

inline LockFreeHashMapStatus LockFreeHashMapAssignmentError::status() const throw()
{
    return m_status;
}

inline const char* LockFreeHashMapAssignmentError::what() const throw()
{
    return "LockFreeHashMap subscript assignment failed";
}

inline LockFreeHashMapResult::LockFreeHashMapResult(LockFreeHashMapStatus status, bool committed, size_t count) :
m_status(status),
m_committed(committed),
m_count(count)
{

}

inline LockFreeHashMapIdentityReference::LockFreeHashMapIdentityReference(const std::shared_ptr<const LockFreeHashMapIdentity>& identity) throw() :
m_identity(identity),
m_padding()
{

}

inline LockFreeHashMapVersion::LockFreeHashMapVersion() :
m_sequence(0)
{

}

inline LockFreeHashMapShrinkDecision::LockFreeHashMapShrinkDecision(size_t count, uint64_t idle) :
shrinkCount(count),
idleMilliseconds(idle)
{

}

inline LockFreeHashMapPoolConfig::LockFreeHashMapPoolConfig() :
initialCapacity(64),
growFreeThreshold(0),
maximumCapacity(-1),
directoryCapacity(1024),
protectionRecordCount(128),
inlineScalarValues(true),
lookupCacheCapacity(4096)
{

}

inline void LockFreeHashMapPoolConfig::setInitialCapacity(size_t value)
{
    initialCapacity = value;
}

inline void LockFreeHashMapPoolConfig::setMaximumCapacity(int64_t value)
{
    maximumCapacity = value;
}

inline void LockFreeHashMapPoolConfig::setGrowFreeThreshold(size_t value)
{
    growFreeThreshold = value;
}

inline void LockFreeHashMapPoolConfig::setDirectoryCapacity(size_t value)
{
    directoryCapacity = value;
}

inline void LockFreeHashMapPoolConfig::setProtectionRecordCount(size_t value)
{
    protectionRecordCount = value;
}

inline void LockFreeHashMapPoolConfig::setInlineScalarValues(bool value)
{
    inlineScalarValues = value;
}

inline void LockFreeHashMapPoolConfig::setLookupCacheCapacity(size_t value)
{
    lookupCacheCapacity = value;
}

inline void LockFreeHashMapPoolConfig::setGrowPolicy(const LockFreeHashMapGrowPolicy& value)
{
    growPolicy = value;
}

inline void LockFreeHashMapPoolConfig::setShrinkPolicy(const LockFreeHashMapShrinkPolicy& value)
{
    shrinkPolicy = value;
}

inline void LockFreeHashMapPoolConfig::setGrowthObserver(const LockFreeHashMapGrowthObserver& value)
{
    m_onGrowth = value;
}

inline LockFreeHashMapConfig::LockFreeHashMapConfig() :
m_bucketCount(1024)
{

}

inline void LockFreeHashMapConfig::setBucketCount(size_t value)
{
    m_bucketCount = value;
}