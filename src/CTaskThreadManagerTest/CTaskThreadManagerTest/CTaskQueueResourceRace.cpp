#include "CTaskQueueResourceRace.h"
#include "LockFreeQueue/LockFreeQueueAPI.h"
#include <iostream>
#include <cstdlib>
#include <string>

LockFreeQueue<int32_t>* CTaskQueueResourceRace::m_queue = nullptr;
bool CTaskQueueResourceRace::m_armed = false;
bool CTaskQueueResourceRace::m_valid = true;
LockFreeQueueEntry<int32_t>* CTaskQueueResourceRace::m_pendingEntry = nullptr;
LockFreeQueueResult CTaskQueueResourceRace::m_pendingResult = LockFreeQueueResourceUnavailable;

void CTaskQueueResourceRace::afterEmptyScan()
{
    if (!m_armed)
    {
        return;
    }
    m_armed = false;
    for (int32_t i = 0; i < 63; ++i)
    {
        int32_t value = -1;
        if (!m_queue->pop(&value) || value != i)
        {
            m_valid = false;
        }
    }
}

bool CTaskQueueResourceRace::run(bool entryOnly)
{
    LockFreeQueue<int32_t> queue(false);
    LockFreeQueuePoolConfig config;
    config.initialCapacity = 64;
    config.maximumCapacity = -1;
    config.growThreshold = 0;
    config.maximumProducers = 4;
    if (queue.configurePool(config) != LockFreeQueueSuccess ||
        queue.configureEntryOnly(entryOnly) != LockFreeQueueSuccess || queue.tryInit() != LockFreeQueueSuccess)
    {
        return false;
    }
    LockFreeQueueEntry<int32_t> entries[64];
    for (int32_t i = 0; i < 63; ++i)
    {
        entries[i].m_value = i;
        const LockFreeQueueResult result = entryOnly ? queue.tryPushEntry(entries[i]) : queue.push(i);
        if (result != LockFreeQueueSuccess)
        {
            return false;
        }
    }
    m_queue = &queue;
    m_armed = true;
    m_valid = true;
    entries[63].m_value = 63;
    const LockFreeQueueResult result = entryOnly ? queue.tryPushEntry(entries[63]) : queue.push(63);
    const bool triggered = !m_armed;
    m_armed = false;
    m_queue = nullptr;
    int32_t value = -1;
    const bool valid = triggered && m_valid && result == LockFreeQueueSuccess &&
        queue.pop(&value) && value == 63 && queue.empty();
    std::cout << "entry_only=" << entryOnly << " triggered=" << triggered << " result=" << result
        << " valid=" << valid << '\n';
    return valid;
}

void CTaskQueueResourceRace::afterReturnPublished()
{
    LockFreeQueueEntry<int32_t>* entry = m_pendingEntry;
    if (entry != nullptr)
    {
        m_pendingEntry = nullptr;
        m_pendingResult = m_queue->tryPushEntry(*entry);
    }
}

bool CTaskQueueResourceRace::runPendingReturn()
{
    LockFreeQueue<int32_t> queue(false);
    LockFreeQueuePoolConfig config;
    config.initialCapacity = 64;
    config.maximumCapacity = -1;
    config.growThreshold = 0;
    config.maximumProducers = 4;
    if (queue.configurePool(config) != LockFreeQueueSuccess || queue.configureEntryOnly(true) != LockFreeQueueSuccess ||
        queue.tryInit() != LockFreeQueueSuccess)
    {
        return false;
    }
    LockFreeQueueEntry<int32_t> entries[64];
    for (int32_t i = 0; i < 64; ++i)
    {
        entries[i].m_value = i;
        if (i < 63 && queue.tryPushEntry(entries[i]) != LockFreeQueueSuccess)
        {
            return false;
        }
    }
    m_queue = &queue;
    m_pendingEntry = &entries[63];
    m_pendingResult = LockFreeQueueResourceUnavailable;
    LockFreeQueueEntry<int32_t>* consumedEntry = nullptr;
    size_t consumed = 0;
    const bool popped = queue.popReadyEntryPrefix(&consumedEntry, 1, consumed);
    const bool triggered = m_pendingEntry == nullptr;
    bool valid = popped && consumed == 1 && consumedEntry == &entries[0] && triggered &&
        m_pendingResult == LockFreeQueueSuccess;
    m_pendingEntry = nullptr;
    int32_t value = -1;
    for (int32_t i = 1; valid && i < 64; ++i)
    {
        valid = queue.pop(&value) && value == i;
    }
    m_queue = nullptr;
    valid = valid && queue.empty();
    std::cout << "pending_return triggered=" << triggered << " result=" << m_pendingResult << " valid=" << valid << '\n';
    return valid;
}

#ifdef CTTM_QUEUE_RESOURCE_RACE_MAIN
int main(int argc, char** argv)
{
    try
    {
        if (argc > 2 || (argc == 2 && std::string(argv[1]) != "1" &&
            std::string(argv[1]) != "2" && std::string(argv[1]) != "3"))
        {
            return 2;
        }
        const int32_t selected = argc == 2 ? std::atoi(argv[1]) : 0;
        const bool value = selected != 0 && selected != 1 ? true : CTaskQueueResourceRace::run(false);
        const bool entry = selected != 0 && selected != 2 ? true : CTaskQueueResourceRace::run(true);
        const bool pending = selected != 0 && selected != 3 ? true : CTaskQueueResourceRace::runPendingReturn();
        return value && entry && pending ? 0 : 1;
    }
    catch (...)
    {
        return 2;
    }
}
#endif