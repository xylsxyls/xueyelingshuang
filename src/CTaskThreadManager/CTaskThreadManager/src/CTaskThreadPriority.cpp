#include "CTaskThreadPriority.h"
#include <system_error>
#ifdef _WIN32
#include <Windows.h>
#else
#include <pthread.h>
#include <sched.h>
#include <errno.h>
#endif

CTaskThreadResult CTaskThreadPriority::apply(std::thread& thread, CTaskSystemPriority priority)
{
    if (priority < CTaskPriorityInherited || priority > CTaskPriorityHighest)
    {
        return CTaskThreadResult(CTaskThreadInvalid);
    }
    if (!thread.joinable())
    {
        return CTaskThreadResult(CTaskThreadClosed);
    }
    if (priority == CTaskPriorityInherited)
    {
        return CTaskThreadResult(CTaskThreadOk, true);
    }
    try
    {
#ifdef _WIN32
        int value = THREAD_PRIORITY_NORMAL;
        switch (priority)
        {
        case CTaskPriorityIdle:
        {
            value = THREAD_PRIORITY_IDLE;
            break;
        }
        case CTaskPriorityLow:
        {
            value = THREAD_PRIORITY_BELOW_NORMAL;
            break;
        }
        case CTaskPriorityHigh:
        {
            value = THREAD_PRIORITY_ABOVE_NORMAL;
            break;
        }
        case CTaskPriorityHighest:
        {
            value = THREAD_PRIORITY_HIGHEST;
            break;
        }
        default:
        {
            break;
        }
        }
        if (!SetThreadPriority(thread.native_handle(), value))
        {
            return CTaskThreadResult(CTaskThreadSystemFailure, false, static_cast<int32_t>(GetLastError()));
        }
#elif defined(__EMSCRIPTEN__)
        return CTaskThreadResult(CTaskThreadUnsupported);
#else
        int policy = SCHED_OTHER;
        sched_param parameter = {};
        if (priority == CTaskPriorityIdle || priority == CTaskPriorityLow)
        {
            // POSIX 普通策略的 priority 必须为 0；不把无法表达的相对低优先级静默设成普通。
            return CTaskThreadResult(CTaskThreadUnsupported);
        }
        if (priority == CTaskPriorityHigh || priority == CTaskPriorityHighest)
        {
            policy = SCHED_RR;
            const int minimum = sched_get_priority_min(policy);
            const int maximum = sched_get_priority_max(policy);
            if (minimum < 0 || maximum < minimum)
            {
                return CTaskThreadResult(CTaskThreadSystemFailure, false, errno);
            }
            parameter.sched_priority = priority == CTaskPriorityHigh ? minimum : minimum + (maximum - minimum) / 2;
        }
        const int error = pthread_setschedparam(thread.native_handle(), policy, &parameter);
        if (error != 0)
        {
            return CTaskThreadResult(CTaskThreadSystemFailure, false, error);
        }
#endif
        return CTaskThreadResult(CTaskThreadOk, true);
    }
    catch (const std::system_error& error)
    {
        return CTaskThreadResult(CTaskThreadSystemFailure, false, error.code().value());
    }
    catch (...)
    {
        return CTaskThreadResult(CTaskThreadSystemFailure);
    }
}