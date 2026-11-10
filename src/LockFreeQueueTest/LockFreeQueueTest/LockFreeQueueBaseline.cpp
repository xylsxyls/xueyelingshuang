#include <atomic>
#include <thread>
#include <chrono>
#include <iostream>
#include <stdint.h>

std::atomic<bool> g_lockFreeQueueBaselineReached(false);
std::atomic<bool> g_lockFreeQueueBaselineRelease(false);
std::atomic<bool> g_lockFreeQueueBaselineTimedOut(false);
__declspec(thread) bool g_lockFreeQueueBaselinePause = false;

/** Sole instrumentation inserted immediately after the actual old tail.exchange */
void LockFreeQueueBaselinePause()
{
    if (!g_lockFreeQueueBaselinePause)
    {
        return;
    }
    g_lockFreeQueueBaselineReached.store(true);
    const std::chrono::steady_clock::time_point deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!g_lockFreeQueueBaselineRelease.load() && std::chrono::steady_clock::now() < deadline)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    g_lockFreeQueueBaselineTimedOut.store(!g_lockFreeQueueBaselineRelease.load());
}

#include "LockFreeQueue.h"

int LockFreeQueueBaselineRun()
{
    LockFreeQueue<int32_t> queue;
    std::atomic<bool> returned(false), consumerStarted(false);
    std::atomic<bool> error(false);
    int32_t value = -1;
    std::thread first([&]() {
        try
        {
            g_lockFreeQueueBaselinePause = true;
            queue.push(1);
        }
        catch (...)
        {
            error.store(true);
        }
    });
    const std::chrono::steady_clock::time_point deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (!g_lockFreeQueueBaselineReached.load() && std::chrono::steady_clock::now() < deadline)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    const bool reached = g_lockFreeQueueBaselineReached.load();
    try
    {
        queue.push(2);
    }
    catch (...)
    {
        g_lockFreeQueueBaselineRelease.store(true);
        first.join();
        throw;
    }
    std::thread consumer;
    try
    {
        consumer = std::thread([&]() { consumerStarted.store(true); queue.pop(&value); returned.store(true); });
    }
    catch (...)
    {
        g_lockFreeQueueBaselineRelease.store(true);
        first.join();
        throw;
    }
    while (!consumerStarted.load())
    {
        std::this_thread::yield();
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    const bool blocked = !returned.load();
    g_lockFreeQueueBaselineRelease.store(true);
    first.join();
    consumer.join();
    const bool valid = reached && blocked && !error.load() && !g_lockFreeQueueBaselineTimedOut.load() && value == 1 &&
        queue.pop(&value) && value == 2 && queue.empty();
    std::cout << (valid ? "OLD_DEFECT_DETECTED" : "BASELINE_TEST_FAIL") <<
        " id=24 second_producer_completed=1 observation_ms=150 consumer_blocked=" << blocked << std::endl;
    return valid ? 0 : 1;
}

int main()
{
    try
    {
        return LockFreeQueueBaselineRun();
    }
    catch (const std::exception& error)
    {
        std::cerr << "BASELINE_TEST_FAIL exception=" << error.what() << std::endl;
        return 1;
    }
}