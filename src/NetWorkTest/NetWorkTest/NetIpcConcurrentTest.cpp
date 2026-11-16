#include "NetIpcConcurrentTest.h"
#include "NetIpcAdapterTest.h"
#include "NetInboxTestAccess.h"
#include "NetSender/NetSenderLocalState.h"
#include "CSystem/CSystemAPI.h"
#include <chrono>
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <stdexcept>

NetIpcConcurrentContext::NetIpcConcurrentContext() :
m_done(0),
m_ready(0),
m_start(false),
m_stop(false),
m_bad(false)
{
    for (uint32_t index = 0; index < 16; ++index)
    {
        m_received[index] = 0;
        m_sent[index] = 0;
        m_threads[index] = 0;
    }
}

NetIpcConcurrentContext::~NetIpcConcurrentContext()
{
    join();
    m_inbox.stop();
}

bool NetIpcConcurrentContext::join()
{
    m_stop = true;
    bool good = true;
    for (uint32_t index = 0; index < 16; ++index)
    {
        if (m_threads[index] == 0)
        {
            continue;
        }
        const CTaskThreadResult result = CTaskThreadManager::instance().uninit(m_threads[index]);
        if (result.m_status == CTaskThreadOk || result.m_status == CTaskThreadNotFound)
        {
            m_threads[index] = 0;
        }
        else
        {
            good = false;
        }
    }
    return good;
}

NetIpcConcurrentProducer::NetIpcConcurrentProducer(NetIpcConcurrentContext& context, uint32_t index) :
m_context(&context),
m_index(index)
{
}

void NetIpcConcurrentProducer::stopTask()
{
    m_context->m_stop = true;
}

void NetIpcConcurrentProducer::doTask()
{
    ++m_context->m_ready;
    while (!m_context->m_start && !m_context->m_stop)
    {
        CSystem::Sleep(1);
    }
    for (uint32_t sequence = 0; sequence < 256 && !m_context->m_stop; ++sequence)
    {
        if (m_context->m_stop)
        {
            break;
        }
        uint8_t bytes[72] = {};
        uint8_t body[16];
        for (uint32_t offset = 0; offset < sizeof(body); ++offset)
        {
            body[offset] = static_cast<uint8_t>(m_index * 31 + sequence * 7 + offset);
        }
        NetSenderLocalPacketView packet = {};
        packet.m_kind = NET_LOCAL_DATA;
        packet.m_packet = sequence + 1;
        packet.m_source[0] = 77;
        packet.m_target[0] = static_cast<uint8_t>(m_index + 1);
        packet.m_data = body;
        packet.m_size = sizeof(body);
        uint32_t size = 0;
        if (NetSenderLocalPacket::encode(packet, bytes, sizeof(bytes), size) != NI_OK)
        {
            m_context->m_bad = true;
            break;
        }
        m_context->m_inbox.receive(1001 + m_index, reinterpret_cast<const char*>(bytes), size,
            static_cast<MessageType>(NET_SENDER_LOCAL_MESSAGE));
        ++m_context->m_sent[m_index];
        std::memset(bytes, 0xcc, sizeof(bytes));
    }
    ++m_context->m_done;
}

void NetIpcAdapterTest::concurrentInbox()
{
    NetIpcConcurrentContext context;
    (context.m_inbox.*netInboxTestMember(NetInboxTestStart()))(false);
    for (uint32_t index = 0; index < 16; ++index)
    {
        uint8_t generation[16] = {};
        generation[0] = static_cast<uint8_t>(index + 1);
        if ((context.m_inbox.*netInboxTestMember(NetInboxTestAllow()))(generation, 1001 + index, index + 1, nullptr) != NI_OK)
        {
            throw std::runtime_error("IPC concurrent route registration");
        }
        context.m_threads[index] = CTaskThreadManager::instance().init();
        std::shared_ptr<CTaskThread> thread = CTaskThreadManager::instance().getThreadInterface(context.m_threads[index]);
        if (!thread || !thread->tryPostTaskResult(std::shared_ptr<CTask>(new NetIpcConcurrentProducer(context, index)),
            1, true, false).m_committed)
        {
            throw std::runtime_error("IPC concurrent producer start");
        }
    }
    uint32_t received = 0;
    uint32_t lastStep[16] = {};
    uint32_t maximumGap = 0;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (context.m_ready != 16 && std::chrono::steady_clock::now() < deadline)
    {
        CSystem::Sleep(1);
    }
    if (context.m_ready != 16)
    {
        throw std::runtime_error("IPC producer readiness deadline");
    }
    context.m_start = true;
    while (received < 4096 && !context.m_bad && std::chrono::steady_clock::now() < deadline)
    {
        NetSenderLocalInput input;
        if ((context.m_inbox.*netInboxTestMember(NetInboxTestTake()))(input) != NI_READY)
        {
            CSystem::Sleep(0);
            continue;
        }
        if (input.m_reason != NI_OK || input.m_peer < 1 || input.m_peer > 16)
        {
            std::printf("IPC_LANE_FAILURE reason=%d peer=%llu received=%u done=%u\n", input.m_reason,
                static_cast<unsigned long long>(input.m_peer), received, context.m_done.load());
            context.join();
            NetSenderLocalInboxState* state = context.m_inbox.*netInboxTestMember(NetInboxTestState());
            const LockFreeHashMapPoolStats stats = state->m_dataPool.statistics(1);
            std::printf("IPC_POOL_FAILURE capacity=%llu reserved=%llu\n",
                static_cast<unsigned long long>(stats.capacity), static_cast<unsigned long long>(stats.reservedCapacity));
            throw std::runtime_error("IPC concurrent lane error");
        }
        const uint32_t index = static_cast<uint32_t>(input.m_peer - 1);
        const uint32_t sequence = context.m_received[index].load();
        NetSenderLocalPacketView packet = {};
        if (input.m_pid != static_cast<int32_t>(1001 + index) ||
            NetSenderLocalPacket::parse(input.m_bytes.data(), static_cast<uint32_t>(input.m_bytes.size()), packet) != NI_OK ||
            packet.m_packet != sequence + 1 || packet.m_size != 16)
        {
            throw std::runtime_error("IPC per-producer FIFO or identity");
        }
        for (uint32_t offset = 0; offset < 16; ++offset)
        {
            if (packet.m_data[offset] != static_cast<uint8_t>(index * 31 + sequence * 7 + offset))
            {
                throw std::runtime_error("IPC independent payload mismatch");
            }
        }
        ++received;
        maximumGap = (std::max)(maximumGap, received - lastStep[index]);
        lastStep[index] = received;
        ++context.m_received[index];
    }
    if (received != 4096 || context.m_bad || !context.join())
    {
        throw std::runtime_error("IPC concurrent progress deadline or task reclamation");
    }
    for (uint32_t index = 0; index < 16; ++index)
    {
        if (context.m_received[index] != 256 || context.m_sent[index] != 256)
        {
            throw std::runtime_error("IPC producer starvation or missing callback");
        }
    }
    NetSenderLocalInput extra;
    if ((context.m_inbox.*netInboxTestMember(NetInboxTestTake()))(extra) != NI_NEED_MORE)
    {
        throw std::runtime_error("IPC duplicate after all producers joined");
    }
    std::printf("IPC concurrent producers=16 each=256 received=%u max_interleaved_gap=%u PASS\n", received, maximumGap);
}

void NetIpcAdapterTest::concurrentStop()
{
    NetIpcConcurrentContext context;
    (context.m_inbox.*netInboxTestMember(NetInboxTestStart()))(false);
    for (uint32_t index = 0; index < 16; ++index)
    {
        uint8_t generation[16] = {};
        generation[0] = static_cast<uint8_t>(index + 1);
        if ((context.m_inbox.*netInboxTestMember(NetInboxTestAllow()))(generation, 1001 + index, index + 1, nullptr) != NI_OK)
        {
            throw std::runtime_error("IPC stop route registration");
        }
        context.m_threads[index] = CTaskThreadManager::instance().init();
        std::shared_ptr<CTaskThread> thread = CTaskThreadManager::instance().getThreadInterface(context.m_threads[index]);
        if (!thread || !thread->tryPostTaskResult(std::shared_ptr<CTask>(new NetIpcConcurrentProducer(context, index)),
            1, true, false).m_committed)
        {
            throw std::runtime_error("IPC stop producer start");
        }
    }
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (context.m_ready != 16 && std::chrono::steady_clock::now() < deadline)
    {
        CSystem::Sleep(1);
    }
    if (context.m_ready != 16)
    {
        throw std::runtime_error("IPC stop readiness deadline");
    }
    context.m_start = true;
    uint32_t submitted = 0;
    while (submitted < 32 && std::chrono::steady_clock::now() < deadline)
    {
        submitted = 0;
        for (uint32_t index = 0; index < 16; ++index)
        {
            submitted += context.m_sent[index].load();
        }
        CSystem::Sleep(0);
    }
    if (submitted < 32)
    {
        throw std::runtime_error("IPC stop producers made no progress");
    }
    context.m_inbox.stop();
    if (!context.join() || context.m_done != 16 || context.m_bad)
    {
        throw std::runtime_error("IPC stop callback/task reclamation");
    }
    NetSenderLocalInput input;
    if ((context.m_inbox.*netInboxTestMember(NetInboxTestTake()))(input) != NI_NEED_MORE)
    {
        throw std::runtime_error("IPC stopped inbox retained input");
    }
    (context.m_inbox.*netInboxTestMember(NetInboxTestStart()))(false);
    if ((context.m_inbox.*netInboxTestMember(NetInboxTestTake()))(input) != NI_NEED_MORE)
    {
        throw std::runtime_error("IPC old generation revived after restart");
    }
    std::printf("IPC stop active_producers=16 submitted_before_stop=%u joined=16 empty_after_restart=1 PASS\n", submitted);
}