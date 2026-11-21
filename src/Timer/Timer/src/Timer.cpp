#include "Timer.h"
#include "TimerState.h"
#include <utility>

Timer::Timer(std::function<void()> callback, uint64_t interval_ms) :
    m_state(std::make_shared<TimerState>(std::move(callback), interval_ms))
{

}

#ifdef __EMSCRIPTEN__
Timer::~Timer()
{
    stop();
}

void Timer::start()
{
    if (!m_state->m_running)
    {
        m_state->m_running = true;
        m_state->schedule(m_state->m_interval);
    }
}

void Timer::stop()
{
    m_state->m_running = false;
    m_state->cancel();
}

void Timer::set_interval(uint64_t interval_ms)
{
    if (interval_ms != 0)
    {
        m_state->m_interval = interval_ms;
    }
}

bool Timer::is_running() const
{
    return m_state->m_running;
}
#else
Timer::~Timer()
{
    {
        std::lock_guard<std::mutex> lock(m_state->m_mutex);
        m_state->m_closed = true;
        m_state->m_running = false;
        m_state->m_changed.notify_one();
    }
    if (m_worker)
    {
        if (m_worker->get_id() == std::this_thread::get_id())
        {
            m_worker->detach();
        }
        else
        {
            m_worker->join();
        }
    }
}

void Timer::start()
{
    std::lock_guard<std::mutex> lock(m_state->m_mutex);
    if (m_state->m_running || m_state->m_closed)
    {
        return;
    }
    if (!m_worker)
    {
        m_worker = std::make_shared<std::thread>(&TimerState::run, m_state);
    }
    ++m_state->m_generation;
    m_state->m_startInterval = m_state->m_interval;
    m_state->m_running = true;
    m_state->m_changed.notify_one();
}

void Timer::stop()
{
    std::lock_guard<std::mutex> lock(m_state->m_mutex);
    if (!m_state->m_running)
    {
        return;
    }
    ++m_state->m_generation;
    m_state->m_running = false;
    m_state->m_changed.notify_one();
}

void Timer::set_interval(uint64_t interval_ms)
{
    if (interval_ms == 0)
    {
        return;
    }
    std::lock_guard<std::mutex> lock(m_state->m_mutex);
    m_state->m_interval = interval_ms;
}

bool Timer::is_running() const
{
    return m_state->m_running.load(std::memory_order_acquire);
}
#endif

// 使用示例：在本翻译单元末尾启用以下 main，单独构建控制台程序
// #include <cstdio>
// #ifndef __EMSCRIPTEN__
// #include <chrono>
// #include <condition_variable>
// #include <mutex>
// #else
// #include <emscripten/emscripten.h>
// #endif
//
// int main()
// {
// #ifdef __EMSCRIPTEN__
//     static Timer timer([]() {
//         std::puts("Timer demo tick");
//         timer.stop();
//         emscripten_force_exit(0);
//     }, 20);
//     timer.set_interval(5);
//     timer.start();
//     return timer.is_running() ? 0 : 1;
// #else
//     std::mutex mutex;
//     std::condition_variable fired;
//     bool ready = false;
//     Timer timer([&]() {
//         std::lock_guard<std::mutex> lock(mutex);
//         ready = true;
//         fired.notify_one();
//     }, 20);
//     timer.set_interval(5);
//     timer.start();
//     if (!timer.is_running())
//     {
//         return 1;
//     }
//     std::unique_lock<std::mutex> lock(mutex);
//     const bool delivered = fired.wait_for(lock, std::chrono::seconds(3), [&]() { return ready; });
//     lock.unlock();
//     timer.stop();
//     return delivered && !timer.is_running() ? 0 : 1;
// #endif
// }