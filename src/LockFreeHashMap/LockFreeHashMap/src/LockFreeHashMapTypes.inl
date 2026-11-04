#pragma once

inline LockFreeHashMapResult::LockFreeHashMapResult(LockFreeHashMapStatus status, bool committed, size_t count) :
m_status(status),
m_committed(committed),
m_count(count)
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
lookupCacheCapacity(256)
{

}