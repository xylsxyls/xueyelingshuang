#pragma once
#include <thread>
#include <chrono>
#if defined(_WIN32) && defined(_MSC_VER) && _MSC_VER < 1900
#include <Windows.h>
#endif

inline bool LockFreeHashMapContention::waitCapacity(uint32_t& retries)
{
    if (retries < 4096)
    {
        ++retries;
        return true;
    }
    try
    {
#if defined(_WIN32) && defined(_MSC_VER) && _MSC_VER < 1900
        // 旧工具链把相对休眠换算成系统日历时钟截止点；用独立单调计数器复核，避免跨时钟刻度提前结束。
        LARGE_INTEGER begin = {}, frequency = {}, current = {};
        if (!QueryPerformanceFrequency(&frequency) || frequency.QuadPart <= 0 ||
            !QueryPerformanceCounter(&begin) || begin.QuadPart < 0)
        {
            return false;
        }
        const LONGLONG interval = frequency.QuadPart / 1000 + (frequency.QuadPart % 1000 != 0);
        do
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            if (!QueryPerformanceCounter(&current) || current.QuadPart < begin.QuadPart)
            {
                return false;
            }
        } while (current.QuadPart - begin.QuadPart < interval);
#else
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
#endif
        return true;
    }
    catch (...)
    {
        return false;
    }
}

inline LockFreeHashMapContention::LockFreeHashMapContention() :
m_attempts(0),
m_jitter(0)
{

}

inline void LockFreeHashMapContention::pause() throw()
{
    ++m_attempts;
    if (m_attempts >= 128)
    {
        reschedule();
    }
}

inline void LockFreeHashMapContention::yieldExecution() throw()
{
#if !defined(__EMSCRIPTEN__) || defined(__EMSCRIPTEN_PTHREADS__)
    std::this_thread::yield();
#endif
}

// inline 保持头文件的单一定义规则；禁止把少见的调度路径膨胀进普通查找
#if defined(_MSC_VER)
__declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
__attribute__((noinline))
#endif
inline void LockFreeHashMapContention::reschedule() throw()
{
#if !defined(__EMSCRIPTEN__) || defined(__EMSCRIPTEN_PTHREADS__)
    if (m_jitter == 0)
    {
        uintptr_t address = reinterpret_cast<uintptr_t>(this);
        m_jitter = static_cast<uint32_t>(address ^ (address >> 16)) | 1;
    }
    m_jitter ^= m_jitter << 13;
    m_jitter ^= m_jitter >> 17;
    m_jitter ^= m_jitter << 5;
    // 错开败者的重试节奏；不依赖系统睡眠计时粒度，也不把每次 CAS 冲突都变成系统调用
    m_attempts = m_jitter & 63;
    yieldExecution();
#else
    // 单线程 Wasm 没有可以并行运行的竞争线程，计数仍保持有界
    m_attempts = 0;
    (void)m_jitter;
#endif
}