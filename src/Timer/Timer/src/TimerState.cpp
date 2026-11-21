#include "TimerState.h"
#include <chrono>
#include <exception>
#include <cstdio>
#include <stdexcept>
#include <utility>
#ifdef __EMSCRIPTEN__
#include <emscripten/eventloop.h>
#endif

TimerState::TimerState(std::function<void()> callback, uint64_t interval) :
    m_callback(std::move(callback)), m_interval(interval == 0 ? 1000 : interval),
#ifdef __EMSCRIPTEN__
    m_remaining(0), m_handle(0), m_pending(false), m_running(false)
#else
    m_generation(0), m_running(false), m_closed(false), m_startInterval(0)
#endif
{
    if (!m_callback)
    {
        throw std::invalid_argument("Timer: callback cannot be empty");
    }
}

void TimerState::invoke()
{
    try
    {
        m_callback();
    }
    catch (const std::exception& error)
    {
        std::fprintf(stderr, "Timer callback: %s\n", error.what());
    }
    catch (...)
    {
        std::fputs("Timer callback failed\n", stderr);
    }
}

#ifdef __EMSCRIPTEN__
void TimerState::schedule(uint64_t delay)
{
    const uint64_t part = delay > INT32_MAX ? INT32_MAX : delay;
    m_remaining = delay - part;
    m_handle = emscripten_set_interval(&TimerState::tick, static_cast<double>(part), this);
    m_pending = true;
}

void TimerState::cancel()
{
    if (m_pending)
    {
        m_pending = false;
        emscripten_clear_interval(m_handle);
    }
}

void TimerState::tick(void* context)
{
    std::shared_ptr<TimerState> state = static_cast<TimerState*>(context)->shared_from_this();
    state->cancel();
    if (state->m_remaining != 0)
    {
        state->schedule(state->m_remaining);
        return;
    }
    state->invoke();
    // 用户回调可能已销毁、停止或重启定时器，只给仍待续约的周期重新登记
    if (state->m_running && !state->m_pending)
    {
        state->schedule(state->m_interval);
    }
}
#else
void TimerState::run()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    while (!m_closed)
    {
        m_changed.wait(lock, [this]() { return m_running.load() || m_closed; });
        if (m_closed)
        {
            return;
        }
        const uint64_t generation = m_generation;
        uint64_t remaining = m_startInterval == 0 ? m_interval : m_startInterval;
        m_startInterval = 0;
        while (remaining != 0 && m_generation == generation && !m_closed)
        {
            const uint64_t part = remaining > INT32_MAX ? INT32_MAX : remaining;
            m_changed.wait_for(lock, std::chrono::milliseconds(part), [this, generation]() {
                return m_closed || m_generation != generation;
            });
            remaining -= part;
        }
        if (!m_closed && m_generation == generation)
        {
            lock.unlock();
            invoke();
            lock.lock();
        }
    }
}
#endif