#include "../../../common/PlatformTest/LibraryCaseSelection.h"
#include "TimerTest.h"
#include "Timer/TimerAPI.h"
#include "../../../common/PlatformTest/LibraryBenchmark.h"
#include <cstring>
#include <memory>
#include <utility>
#include <chrono>
#ifndef __EMSCRIPTEN__
#include <condition_variable>
#include <mutex>
#endif

/** 对一个已启动过、现已停止的实例重复启停并检查每次状态变化。
@param [in] name 记录当前工作者状态的场景名称
@param [in,out] timer 计时前已完成首次启动的实例
@return 每次均观察到启动并以停止状态结束时返回 true
*/
static bool MeasureStartStop(const char* name, Timer& timer)
{
    bool valid = true;
    LibraryBenchmark::run(name, [&](uint32_t count) -> uint64_t {
        uint64_t started = 0;
        for (uint32_t i = 0; i < count; ++i)
        {
            timer.start();
            started += timer.is_running();
            timer.stop();
        }
        valid = valid && started == count && !timer.is_running();
        return started;
    });
    return valid;
}

#ifndef __EMSCRIPTEN__
/** 让回调在另一把锁上等待，测量工作者不竞争调度锁时的调用方启停成本。
@return 回调仅进入一次、观察窗内完成且每次状态变化正确时返回 true
*/
static bool MeasureWhileInCallback()
{
    std::mutex mutex;
    std::condition_variable changed;
    bool release = false;
    bool timedOut = false;
    unsigned calls = 0;
    bool valid = false;
    {
        Timer timer([&]() {
            std::unique_lock<std::mutex> lock(mutex);
            ++calls;
            changed.notify_all();
            if (!changed.wait_for(lock, std::chrono::seconds(30), [&]() { return release; }))
            {
                timedOut = true;
            }
        }, 1);
        timer.start();
        std::unique_lock<std::mutex> lock(mutex);
        if (changed.wait_for(lock, std::chrono::seconds(5), [&]() { return calls == 1; }))
        {
            lock.unlock();
            timer.stop();
            timer.set_interval(UINT64_MAX);
            valid = MeasureStartStop("Timer.start-stop.inflight", timer);
            lock.lock();
        }
        release = true;
        changed.notify_all();
    }
    return valid && !timedOut && calls == 1;
}
#endif

/** 运行本工程的接口性能用例
@return 输入和结果满足各用例约定时返回0
*/
static int RunBenchmarks()
{
    Timer running([]() {}, UINT64_MAX);
    Timer idle([]() {}, UINT64_MAX);
    running.start();
    Timer* timers[] = {&running, &idle};
    uint32_t invalid = 0;
    LibraryBenchmark::run("Timer.is_running", [&](uint32_t count) -> uint64_t {
        uint64_t sum = 0;
        for (uint32_t i = 0; i < count; ++i)
        {
            sum += timers[i & 1]->is_running();
        }
        invalid |= sum != (count + 1) / 2;
        return sum;
    });
    // void 接口经 volatile 成员函数指针逐次派发，允许计时包含一次间接调用。
    void (Timer::* volatile setInterval)(uint64_t) = &Timer::set_interval;
    LibraryBenchmark::run("Timer.set_interval.idle", [&](uint32_t count) -> uint64_t {
        for (uint32_t i = 0; i < count; ++i)
        {
            (idle.*setInterval)(1000 + (i & 1));
        }
        return count;
    });
    running.stop();
    Timer cycled([]() {}, UINT64_MAX);
    cycled.start();
    cycled.stop();
    const bool cycledCorrectly = MeasureStartStop("Timer.start-stop.cycle", cycled);
#ifndef __EMSCRIPTEN__
    if (!MeasureWhileInCallback())
    {
        return 1;
    }
#endif
    if (!cycledCorrectly || invalid != 0 || running.is_running())
    {
        return 1;
    }
    return 0;
}

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <emscripten/eventloop.h>
#include <emscripten/html5.h>
#include <stdexcept>
#include <cstdio>

// 本模块只在单个宿主事件循环上运行，同一次异步用例贯穿全部回调。
static std::unique_ptr<Timer> active;
static int count = 0;
// 记录回调链实际结束的时间，延迟执行的宿主轮询不会掩盖超时。
static double chainFinishedAt = 0;
static int unexpected = 0;
static int result = -1;
static int watchdog = 0;
// 默认间隔用例自身的生命周期和宿主单调时钟读数
static std::unique_ptr<Timer> defaultPeriod;
static double defaultStart = 0;
static double defaultElapsed = 0;
// 与省略周期的实例同时启动，单独核验显式传 0 的行为
static std::unique_ptr<Timer> zeroPeriod;
static double zeroElapsed = 0;
// 保留在回调中停止的活动对象，观察是否被错误地再次调度
static std::unique_ptr<Timer> stoppedTimer;
static int stoppedCalls = 0;
static double stoppedAt = 0;
// 在其他定时器完成前一直保持运行，验证超长周期没有在宿主中截短成极短等待。
static std::unique_ptr<Timer> longPeriod;
// 按实际回调时间验证两次 set_interval(0) 保留非默认周期
static std::unique_ptr<Timer> configuredPeriod;
static double configuredStart = 0;
static double configuredFirst = 0;
static double configuredSecond = 0;

/** 等待被撤销的短时回调到期后，向宿主交付最终结果
@param [in] context 宿主回传的空指针
*/
static void Finish(void* context)
{
    (void)context;
    const double now = emscripten_get_now();
    const bool observingStop = stoppedCalls == 0 || now - stoppedAt < 50;
    if ((defaultPeriod || zeroPeriod || configuredPeriod || active || observingStop) && now - defaultStart < 10000)
    {
        return;
    }
    emscripten_clear_interval(watchdog);
    const bool defaultPassed = !defaultPeriod && defaultElapsed >= 750 && defaultElapsed <= 10000;
    const bool zeroPassed = !zeroPeriod && zeroElapsed >= 750 && zeroElapsed <= 10000;
    const bool configuredPassed = !configuredPeriod && configuredFirst - configuredStart >= 1250 &&
        configuredSecond - configuredFirst >= 1150 && configuredSecond - configuredStart <= 10000;
    result = count == 3 && unexpected == 0 && !active && chainFinishedAt - defaultStart <= 10000 &&
        defaultPassed && zeroPassed && configuredPassed && stoppedCalls == 1 && stoppedAt - defaultStart <= 10000 &&
        !observingStop && !stoppedTimer->is_running() && longPeriod->is_running() ? 0 : 1;
    longPeriod.reset();
    stoppedTimer.reset();
    std::printf("TimerTest callbacks=%d unexpected=%d result=%d\n", count, unexpected, result);
    if (result != 0)
    {
        emscripten_force_exit(result);
    }
}

/** 向 Wasm 宿主报告结果。等待中返回 -1，成功返回 0，失败返回 1。 */
extern "C" EMSCRIPTEN_KEEPALIVE int timerTestResult()
{
    return result;
}

/** 启动宿主回调链，由 timerTestResult 汇报完成结果。 */
static int TestEventLoop()
{
    try { Timer empty(std::function<void()>(), 1); return 1; }
    catch (const std::invalid_argument&) {}
    defaultPeriod.reset(new Timer([]() {
        defaultElapsed = emscripten_get_now() - defaultStart;
        defaultPeriod.reset();
    }));
    zeroPeriod.reset(new Timer([]() {
        zeroElapsed = emscripten_get_now() - defaultStart;
        zeroPeriod.reset();
    }, 0));
    if (defaultPeriod->is_running())
    {
        return 1;
    }
    defaultStart = emscripten_get_now();
    defaultPeriod->start();
    zeroPeriod->start();
    configuredPeriod.reset(new Timer([]() {
        if (configuredFirst == 0)
        {
            configuredFirst = emscripten_get_now();
            configuredPeriod->set_interval(1300);
            configuredPeriod->set_interval(0);
        }
        else
        {
            configuredSecond = emscripten_get_now();
            configuredPeriod.reset();
        }
    }, UINT64_MAX));
    configuredPeriod->set_interval(1400);
    configuredPeriod->set_interval(0);
    configuredStart = emscripten_get_now();
    configuredPeriod->start();
    configuredPeriod->set_interval(UINT64_MAX);
    stoppedTimer.reset(new Timer([]() {
        ++stoppedCalls;
        stoppedAt = emscripten_get_now();
        stoppedTimer->stop();
    }, 1));
    stoppedTimer->start();
    longPeriod.reset(new Timer([]() {
        ++unexpected;
        longPeriod->stop();
    }, UINT64_MAX));
    longPeriod->start();
    defaultPeriod->set_interval(1);
    Timer cancelled([&]() { ++unexpected; }, 1);
    cancelled.start();
    cancelled.stop();
    cancelled.stop();
    {
        Timer destroyed([]() { ++unexpected; }, 1);
        destroyed.start();
        Timer longDelay([]() { ++unexpected; }, UINT64_MAX);
        longDelay.start();
    }
    active.reset(new Timer([]() {
        ++count;
        if (count == 1)
        {
            active->set_interval(0);
            active->set_interval(2);
            active->stop();
            active->start();
            active->start();
            active->set_interval(UINT64_MAX);
        }
        else if (count == 2)
        {
            active->set_interval(2);
            throw std::runtime_error("TimerTest callback exception");
        }
        else
        {
            chainFinishedAt = emscripten_get_now();
            active.reset();
        }
    }, UINT64_MAX));
    active->set_interval(1);
    active->set_interval(0);
    active->start();
    // 定时器因回归而停止调度时，宿主仍能读到失败。
    watchdog = emscripten_set_interval(Finish, 50, nullptr);
    return 0;
}

int main(int argc, char** argv)
{
    if (argc == 2 && std::strcmp(argv[1], "--benchmark") == 0)
    {
        return RunBenchmarks();
    }
    const LibraryTestCase cases[] = {
        {2, false, TestEventLoop},
    };
    return LibraryCaseSelection::run(argc, argv, cases);
}
#else
#if defined(_MSC_VER)
static_assert(sizeof(Timer) == 4 * sizeof(void*), "Native MSVC Timer ABI size changed");
static_assert(__alignof(Timer) == __alignof(void*), "Native MSVC Timer ABI alignment changed");
#endif
#include <atomic>
#include <thread>
#include <cstdio>
#include <stdexcept>

/** 在线程和回调内验证启动、取消及销毁，成功返回 0。 */
static int TestWorker()
{
    {
        std::mutex guard;
        std::condition_variable wake;
        bool delivered = false;
        bool zeroDelivered = false;
        std::chrono::steady_clock::time_point zeroObserved;
        std::chrono::steady_clock::time_point observed;
        Timer defaultTimer([&]() {
            std::lock_guard<std::mutex> lock(guard);
            if (!delivered)
            {
                observed = std::chrono::steady_clock::now();
            }
            delivered = true;
            wake.notify_one();
        });
        Timer zeroTimer([&]() {
            std::lock_guard<std::mutex> lock(guard);
            if (!zeroDelivered)
            {
                zeroObserved = std::chrono::steady_clock::now();
            }
            zeroDelivered = true;
            wake.notify_one();
        }, 0);
        if (defaultTimer.is_running())
        {
            return 1;
        }
        const auto start = std::chrono::steady_clock::now();
        defaultTimer.start();
        zeroTimer.start();
        defaultTimer.set_interval(1);
        std::unique_lock<std::mutex> lock(guard);
        const bool fired = wake.wait_for(lock, std::chrono::seconds(10), [&]() { return delivered && zeroDelivered; });
        const auto earliest = start + std::chrono::milliseconds(750);
        const auto latest = start + std::chrono::seconds(10);
        if (!fired || observed < earliest || observed > latest || zeroObserved < earliest || zeroObserved > latest)
        {
            return 1;
        }
    }
    bool rejected = false;
    try { Timer invalid(std::function<void()>(), 1); }
    catch (const std::invalid_argument&) { rejected = true; }
    if (!rejected)
    {
        return 1;
    }
    std::mutex mutex;
    std::condition_variable changed;
    unsigned calls = 0;
    {
        Timer timer([&]() {
            std::lock_guard<std::mutex> lock(mutex);
            ++calls;
            changed.notify_one();
        }, UINT64_MAX);
        if (timer.is_running())
        {
            return 2;
        }
        timer.stop();
        timer.set_interval(2);
        timer.set_interval(0);
        timer.start();
        if (!timer.is_running())
        {
            return 3;
        }
        timer.start();
        std::unique_lock<std::mutex> lock(mutex);
        const bool fired = changed.wait_for(lock, std::chrono::seconds(3), [&]() { return calls >= 3; });
        lock.unlock();
        timer.stop();
        timer.stop();
        if (!fired || timer.is_running())
        {
            return 4;
        }
    }
    // 超长等待必须能立即取消；停止后重启只能有一个工作者执行回调。
    std::atomic<unsigned> unexpected(0);
    {
        Timer veryLong([&]() { ++unexpected; }, UINT64_MAX);
        veryLong.start();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        veryLong.stop();
    }
    if (unexpected.load() != 0)
    {
        return 5;
    }
    std::unique_ptr<Timer> self;
    calls = 0;
    self.reset(new Timer([&]() {
        std::lock_guard<std::mutex> lock(mutex);
        if (!self)
        {
            return;
        }
        ++calls;
        if (calls == 1)
        {
            self->stop();
            self->set_interval(2);
            self->start();
            self->start();
            self->set_interval(UINT64_MAX);
            throw std::runtime_error("TimerTest");
        }
        if (calls == 2)
        {
            self->set_interval(2);
            throw 2;
        }
        if (calls == 3)
        {
            self.reset(); changed.notify_one();
        }
    }, 2));
    self->start();
    std::unique_lock<std::mutex> lock(mutex);
    if (!changed.wait_for(lock, std::chrono::seconds(3), [&]() { return calls == 3 && !self; }))
    {
        // 超时时先在锁内接管对象，让已获准执行的回调也能安全退出。
        std::unique_ptr<Timer> pending = std::move(self);
        lock.unlock();
        pending.reset();
        return 6;
    }
    return 0;
}

/** 回调尚未结束时从外部停止。stop 应返回，析构完成后回调必须已经退出。 */
static int TestStopInFlight()
{
    std::mutex mutex;
    std::condition_variable changed;
    bool entered = false;
    bool released = false;
    bool stopped = false;
    bool exited = false;
    bool releasedInTime = false;
    unsigned calls = 0;
    bool returnedWhileActive = false;
    bool destroying = false;
    bool destroyed = false;
    bool joinedCallback = false;
    {
        std::unique_ptr<Timer> timer(new Timer([&]() {
            std::unique_lock<std::mutex> lock(mutex);
            ++calls;
            entered = true;
            changed.notify_all();
            releasedInTime = changed.wait_for(lock, std::chrono::seconds(15), [&]() { return released; });
            exited = true;
            changed.notify_all();
        }, 1));
        timer->start();
        std::unique_lock<std::mutex> lock(mutex);
        if (!changed.wait_for(lock, std::chrono::seconds(5), [&]() { return entered; }))
        {
            released = true;
            changed.notify_all();
            return 1;
        }
        std::thread stopper([&]() {
            timer->stop();
            std::lock_guard<std::mutex> guard(mutex);
            stopped = true;
            changed.notify_all();
        });
        returnedWhileActive = changed.wait_for(lock, std::chrono::seconds(5), [&]() { return stopped; }) &&
            !exited && !timer->is_running();
        if (!returnedWhileActive)
        {
            released = true;
            changed.notify_all();
        }
        lock.unlock();
        stopper.join();
        std::thread destroyer([&]() {
            {
                std::lock_guard<std::mutex> guard(mutex);
                destroying = true;
                changed.notify_all();
            }
            timer.reset();
            std::lock_guard<std::mutex> guard(mutex);
            joinedCallback = exited;
            destroyed = true;
            changed.notify_all();
        });
        lock.lock();
        changed.wait(lock, [&]() { return destroying; });
        changed.wait_for(lock, std::chrono::milliseconds(100), [&]() { return destroyed; });
        released = true;
        changed.notify_all();
        changed.wait(lock, [&]() { return exited; });
        lock.unlock();
        destroyer.join();
    }
    std::lock_guard<std::mutex> guard(mutex);
    return returnedWhileActive && joinedCallback && releasedInTime && calls == 1 ? 0 : 1;
}

/** 旧回调未退出时由外部重启，核对两代回调互斥以及新一代的首个周期。
@return 两次回调按序完成且第二次回调已停止定时器时返回 0
*/
static int TestRestartInFlight()
{
    std::mutex mutex;
    std::condition_variable changed;
    unsigned calls = 0;
    bool release = false;
    bool finishedFirst = false;
    bool overlap = false;
    Timer* activeTimer = nullptr;
    Timer timer([&]() {
        std::unique_lock<std::mutex> lock(mutex);
        ++calls;
        changed.notify_all();
        if (calls == 1)
        {
            finishedFirst = changed.wait_for(lock, std::chrono::seconds(10), [&]() { return release; });
        }
        else
        {
            overlap = overlap || !finishedFirst;
            activeTimer->set_interval(2);
            activeTimer->stop();
        }
        changed.notify_all();
    }, 1);
    activeTimer = &timer;
    timer.start();
    std::unique_lock<std::mutex> lock(mutex);
    if (!changed.wait_for(lock, std::chrono::seconds(3), [&]() { return calls != 0; }))
    {
        release = true;
        changed.notify_all();
        return 1;
    }
    lock.unlock();
    timer.stop();
    timer.set_interval(2);
    timer.start();
    timer.set_interval(UINT64_MAX);
    lock.lock();
    const bool enteredEarly = changed.wait_for(lock, std::chrono::milliseconds(75), [&]() { return calls > 1; });
    release = true;
    changed.notify_all();
    const bool stopped = changed.wait_for(lock, std::chrono::seconds(3), [&]() { return calls >= 2 && !timer.is_running(); });
    const bool repeated = stopped && changed.wait_for(lock, std::chrono::milliseconds(75), [&]() { return calls > 2; });
    return stopped && !enteredEarly && !overlap && finishedFirst && !repeated && calls == 2 ? 0 : 1;
}

/** 验证零值保留启动前及回调内最后设置的非默认周期
@return 两次回调均满足独立时钟下的等待下界，并在十秒内完成时返回 0
*/
static int TestConfiguredInterval()
{
    typedef std::chrono::steady_clock Clock;
    std::mutex mutex;
    std::condition_variable changed;
    Clock::time_point first;
    Clock::time_point second;
    unsigned calls = 0;
    Timer* activeTimer = nullptr;
    Timer timer([&]() {
        std::lock_guard<std::mutex> lock(mutex);
        ++calls;
        if (calls == 1)
        {
            first = Clock::now();
            activeTimer->set_interval(1300);
            activeTimer->set_interval(0);
        }
        else
        {
            second = Clock::now();
            activeTimer->stop();
            changed.notify_one();
        }
    }, UINT64_MAX);
    activeTimer = &timer;
    timer.set_interval(1400);
    timer.set_interval(0);
    std::unique_lock<std::mutex> lock(mutex);
    const Clock::time_point start = Clock::now();
    timer.start();
    timer.set_interval(UINT64_MAX);
    const Clock::time_point deadline = start + std::chrono::seconds(10);
    const bool finished = changed.wait_until(lock, deadline, [&]() { return calls >= 2; });
    return finished && calls == 2 && first - start >= std::chrono::milliseconds(1250) &&
        second - first >= std::chrono::milliseconds(1150) && second <= deadline ? 0 : 1;
}

int main(int argc, char** argv)
{
    if (argc == 2 && std::strcmp(argv[1], "--benchmark") == 0)
    {
        return RunBenchmarks();
    }
    const LibraryTestCase cases[] = {
        {1, false, TestWorker},
        {3, false, TestStopInFlight},
        {4, false, TestRestartInFlight},
        {5, false, TestConfiguredInterval},
    };
    return LibraryCaseSelection::run(argc, argv, cases);
}
#endif