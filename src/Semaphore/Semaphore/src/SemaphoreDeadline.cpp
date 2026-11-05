#include "SemaphoreDeadline.h"
#include <algorithm>
#ifdef _WIN32
#include <Windows.h>
#else
#include <chrono>
#endif

SemaphoreDeadline::SemaphoreDeadline(bool infinite, uint32_t milliseconds) :
m_infinite(infinite),
m_started(infinite ? 0 : now()),
m_milliseconds(milliseconds)
{

}

uint32_t SemaphoreDeadline::remaining(uint32_t maximum) const
{
    if (m_infinite)
    {
        return maximum;
    }
    const uint64_t elapsed = now() - m_started;
    return elapsed >= m_milliseconds ? 0 : (std::min)(maximum, m_milliseconds - static_cast<uint32_t>(elapsed));
}

uint64_t SemaphoreDeadline::now()
{
#ifdef _WIN32
    LARGE_INTEGER ticks;
    LARGE_INTEGER frequency;
    if (::QueryPerformanceCounter(&ticks) && ::QueryPerformanceFrequency(&frequency) && frequency.QuadPart > 0)
    {
        return static_cast<uint64_t>(static_cast<double>(ticks.QuadPart) * 1000.0 / frequency.QuadPart);
    }
    return ::GetTickCount64();
#else
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
            .count());
#endif
}