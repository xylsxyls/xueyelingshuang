#include "ProcessWorkTime.h"
#include "CSystem/CSystemAPI.h"

uint64_t ProcessWorkTime::milliseconds()
{
    return CSystem::monotonicMilliseconds();
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