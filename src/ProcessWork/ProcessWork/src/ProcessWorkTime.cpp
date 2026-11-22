#include "ProcessWorkTime.h"
#ifdef _WIN32
#include <Windows.h>
#else
#include <chrono>
#endif
#include "CSystem/CSystemAPI.h"

uint64_t ProcessWorkTime::milliseconds()
{
#ifdef _WIN32
    return GetTickCount64();
#else
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
#endif
}

bool ProcessWorkTime::waitCapacity(uint32_t& attempts)
{
    return waitCapacity(attempts, 4096, 1);
}

bool ProcessWorkTime::waitCapacity(uint32_t& attempts, uint32_t fastAttempts, uint32_t sleepMilliseconds)
{
    if (sleepMilliseconds == 0)
    {
        return false;
    }
    if (attempts < fastAttempts)
    {
        ++attempts;
        return true;
    }
    try
    {
        CSystem::Sleep(sleepMilliseconds);
        return true;
    }
    catch (...)
    {
        return false;
    }
}