#include "NetSubmissionConcurrentTest.h"
#include "NetWorkTest.h"
#include "CSystem/CSystemAPI.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>

/** 检查独立结果并交给统一runner
@param [in] condition 必须成立的条件
@param [in] reason 失败位置
*/
static void SubmissionCheck(bool condition, const char* reason)
{
    netTestCheckpoint();
    if (!condition)
    {
        throw std::runtime_error(reason);
    }
}

NetSubmissionConcurrentContext::NetSubmissionConcurrentContext() :
m_start(false), m_stop(false), m_failure(NI_OK), m_done(0)
{
    for (uint32_t index = 0; index < 16; ++index)
    {
        m_received[index].store(0);
    }
}

NetSubmissionConcurrentTask::NetSubmissionConcurrentTask(NetSubmissionConcurrentContext& context, uint32_t producer) :
m_context(context), m_producer(producer)
{

}

void NetSubmissionConcurrentTask::stopTask()
{
    m_context.m_stop.store(true);
}

void NetSubmissionConcurrentTask::doTask()
{
    while (!m_context.m_start.load() && !m_context.m_stop.load())
    {
        CSystem::Sleep(0);
    }
    int32_t result = NI_OK;
    for (uint32_t base = 0; base < 512 && !m_context.m_stop.load() && result == NI_OK; base += 4)
    {
        uint64_t tickets[4] = {};
        for (uint32_t index = 0; index < 4 && result == NI_OK && !m_context.m_stop.load(); ++index)
        {
            uint64_t id = 0;
            result = m_context.m_submission.begin(101, 701, 60000, id);
            if (result == NI_OK)
            {
                const uint32_t words[2] = {m_producer, base + index};
                const uint8_t* bytes = reinterpret_cast<const uint8_t*>(words);
                result = index % 2 == 0 ? m_context.m_submission.send(id, bytes, sizeof(words)) :
                    m_context.m_submission.sendSynchronous(id, bytes, sizeof(words), tickets[index]);
            }
        }
        while (result == NI_OK && !m_context.m_stop.load() && m_context.m_received[m_producer].load() < base + 4)
        {
            CSystem::Sleep(0);
        }
        if (m_context.m_stop.load())
        {
            break;
        }
        for (uint32_t index = 0; index < 4 && result == NI_OK; ++index)
        {
            if (tickets[index] != 0)
            {
                result = m_context.m_submission.synchronousResult(tickets[index], true);
                if (result == NI_OK && m_context.m_submission.synchronousResult(tickets[index], true) != NI_NOT_FOUND)
                {
                    result = NI_INVALID_STATE;
                }
            }
        }
    }
    if (result != NI_OK)
    {
        int32_t expected = NI_OK;
        m_context.m_failure.compare_exchange_strong(expected, result);
        m_context.m_stop.store(true);
    }
    m_context.m_done.fetch_add(1);
}

NetSubmissionConcurrentOwner::NetSubmissionConcurrentOwner(NetSubmissionConcurrentContext& context) :
m_context(context)
{
    std::memset(m_threads, 0, sizeof(m_threads));
}

NetSubmissionConcurrentOwner::~NetSubmissionConcurrentOwner()
{
    m_context.m_stop.store(true);
    for (uint32_t index = 0; index < 16; ++index)
    {
        if (m_threads[index] != 0)
        {
            CTaskThreadManager::instance().uninit(m_threads[index]);
        }
    }
}

void NetSubmissionConcurrentTest::run()
{
    NetSubmissionConcurrentContext context;
    NetSubmissionConcurrentOwner owner(context);
    SubmissionCheck(context.m_submission.start(1, 101) == NI_OK && context.m_submission.ready(1, 128, 4096) == NI_OK,
        "concurrent submission initialize");
    CTaskThreadManager& manager = CTaskThreadManager::instance();
    for (uint32_t index = 0; index < 16; ++index)
    {
        owner.m_threads[index] = manager.init();
        SubmissionCheck(owner.m_threads[index] != 0, "concurrent producer thread initialize");
        std::shared_ptr<CTaskThread> thread = manager.getThreadInterface(owner.m_threads[index]);
        std::shared_ptr<CTask> task(new NetSubmissionConcurrentTask(context, index));
        SubmissionCheck(thread && thread->postTask(task), "concurrent producer postTask");
    }
    const uint64_t started = NetSenderSubmission::now();
    uint64_t last[16];
    for (uint32_t index = 0; index < 16; ++index)
    {
        last[index] = started;
    }
    uint64_t maximumGap = 0;
    uint32_t received = 0;
    context.m_start.store(true);
    while (context.m_done.load() != 16 && context.m_failure.load() == NI_OK)
    {
        SubmissionCheck(NetSenderSubmission::now() - started < 60000, "concurrent producer progress deadline");
        SubmissionCheck(context.m_submission.beginPoll() == NI_OK, "concurrent beginPoll");
        for (uint32_t step = 0; step < 128; ++step)
        {
            NetSenderSubmissionCommand command = {};
            const int32_t next = context.m_submission.next(command);
            if (next == NI_NEED_MORE)
            {
                break;
            }
            SubmissionCheck(next == NI_READY, "concurrent next");
            if (command.m_kind == NET_SUBMIT_BEGIN)
            {
                context.m_submission.finish(NI_OK);
                continue;
            }
            SubmissionCheck(command.m_kind == NET_SUBMIT_SEND && command.m_size == 8, "unexpected close, terminal or payload size");
            uint32_t words[2];
            std::memcpy(words, command.m_bytes, sizeof(words));
            SubmissionCheck(words[0] < 16 && words[1] == context.m_received[words[0]].load(), "per-producer FIFO or payload corruption");
            const uint64_t current = NetSenderSubmission::now();
            const uint64_t gap = current - last[words[0]];
            if (gap > maximumGap)
            {
                maximumGap = gap;
            }
            last[words[0]] = current;
            context.m_submission.finish(NI_OK);
            context.m_submission.advanceSynchronous(NI_OK);
            NetSenderSubmissionCommand terminal = {};
            SubmissionCheck(context.m_submission.complete(command.m_generation, command.m_id, terminal), "unique request terminal");
            context.m_received[words[0]].fetch_add(1);
            ++received;
        }
        CSystem::Sleep(0);
    }
    std::printf("submission producers=16 received=%u expected=8192 result=%d max_gap_ms=%llu elapsed_ms=%llu\n",
        received, context.m_failure.load(), static_cast<unsigned long long>(maximumGap),
        static_cast<unsigned long long>(NetSenderSubmission::now() - started));
    SubmissionCheck(context.m_failure.load() == NI_OK && received == 8192, "contention returned false resource failure");
    SubmissionCheck(context.m_submission.beginPoll() == NI_OK && context.m_submission.idle() &&
        context.m_submission.queuedBytes() == 0, "concurrent submission did not return all budgets");
}