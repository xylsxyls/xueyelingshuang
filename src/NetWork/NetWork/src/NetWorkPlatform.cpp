#include "NetWorkPlatform.h"
#include "NetWorkMacro.h"

#ifndef _WIN32
#include <errno.h>
#include <time.h>
#endif

NetWorkGate::NetWorkGate() :
m_mutexReady(false),
m_conditionReady(false)
{
#ifdef _WIN32
    InitializeSRWLock(&m_mutex);
    InitializeConditionVariable(&m_changed);
    m_mutexReady = true;
    m_conditionReady = true;
#else
    m_mutexReady = pthread_mutex_init(&m_mutex, nullptr) == 0;
    if (m_mutexReady)
    {
#ifdef __APPLE__
        m_conditionReady = pthread_cond_init(&m_changed, nullptr) == 0;
#else
        pthread_condattr_t attributes;
        if (pthread_condattr_init(&attributes) == 0)
        {
            // 超时用单调时间，系统校时不能延长两毫秒推进等待。
            if (pthread_condattr_setclock(&attributes, CLOCK_MONOTONIC) == 0)
            {
                m_conditionReady = pthread_cond_init(&m_changed, &attributes) == 0;
            }
            pthread_condattr_destroy(&attributes);
        }
#endif
    }
#endif
}

NetWorkGate::~NetWorkGate()
{
#ifndef _WIN32
    if (m_conditionReady)
    {
        pthread_cond_destroy(&m_changed);
    }
    if (m_mutexReady)
    {
        pthread_mutex_destroy(&m_mutex);
    }
#endif
}

bool NetWorkGate::valid() const
{
    return m_mutexReady && m_conditionReady;
}

void NetWorkGate::lock()
{
#ifdef _WIN32
    AcquireSRWLockExclusive(&m_mutex);
#else
    pthread_mutex_lock(&m_mutex);
#endif
}

void NetWorkGate::unlock()
{
#ifdef _WIN32
    ReleaseSRWLockExclusive(&m_mutex);
#else
    pthread_mutex_unlock(&m_mutex);
#endif
}

bool NetWorkGate::wait()
{
#ifdef _WIN32
    return SleepConditionVariableSRW(&m_changed, &m_mutex, INFINITE, 0) != FALSE;
#else
    return pthread_cond_wait(&m_changed, &m_mutex) == 0;
#endif
}

void NetWorkGate::notify()
{
#ifdef _WIN32
    WakeConditionVariable(&m_changed);
#else
    pthread_cond_signal(&m_changed);
#endif
}

bool NetWorkGate::waitFor(uint32_t milliseconds)
{
    if (milliseconds == UINT32_MAX)
    {
        return false;
    }
#ifdef _WIN32
    return SleepConditionVariableSRW(&m_changed, &m_mutex, milliseconds, 0) != FALSE || GetLastError() == ERROR_TIMEOUT;
#else
    timespec deadline = {};
    deadline.tv_sec = static_cast<time_t>(milliseconds / 1000);
    deadline.tv_nsec = static_cast<long>(milliseconds % 1000) * 1000000L;
#ifdef __APPLE__
    const int result = pthread_cond_timedwait_relative_np(&m_changed, &m_mutex, &deadline);
#else
    timespec current = {};
    if (clock_gettime(CLOCK_MONOTONIC, &current) != 0)
    {
        return false;
    }
    deadline.tv_sec += current.tv_sec;
    deadline.tv_nsec += current.tv_nsec;
    if (deadline.tv_nsec >= 1000000000L)
    {
        ++deadline.tv_sec;
        deadline.tv_nsec -= 1000000000L;
    }
    const int result = pthread_cond_timedwait(&m_changed, &m_mutex, &deadline);
#endif
    return result == 0 || result == ETIMEDOUT;
#endif
}

void NetWorkGate::notifyAll()
{
#ifdef _WIN32
    WakeAllConditionVariable(&m_changed);
#else
    pthread_cond_broadcast(&m_changed);
#endif
}