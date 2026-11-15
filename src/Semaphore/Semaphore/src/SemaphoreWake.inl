#pragma once

inline SemaphoreResult SemaphoreWake::initializationResult() const
{
    return SemaphoreResult(m_initializationStatus, false, m_initializationError);
}

inline SemaphoreStatus SemaphoreWake::initializationStatus() const
{
    return m_initializationStatus;
}

inline bool SemaphoreWake::hasWaiters() const
{
    return m_sleepers.load() != 0;
}