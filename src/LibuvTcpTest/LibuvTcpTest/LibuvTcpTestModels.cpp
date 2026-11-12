#include "LibuvTcpTestModels.h"
#include "uv.h"
#include <cstring>
#include <algorithm>
#include <cstdio>
#include "LibuvTcp/LibuvBase.h"

LibuvTestTask::LibuvTestTask(const std::function<void()>& run, const std::function<void()>& stop) :
m_run(run),
m_stop(stop)
{
}

void LibuvTestTask::doTask()
{
    m_run();
}

void LibuvTestTask::stopTask()
{
    m_stop();
}

LibuvTestThread::LibuvTestThread() :
m_id(0)
{
}

LibuvTestThread::~LibuvTestThread()
{
    if (m_id != 0)
    {
        CTaskThreadManager::instance().uninit(m_id);
    }
}

bool LibuvTestThread::start(const std::function<void()>& run, const std::function<void()>& stop)
{
    if (m_id != 0)
    {
        return false;
    }
    std::shared_ptr<CTask> task(new LibuvTestTask(run, stop));
    m_id = CTaskThreadManager::instance().init();
    if (m_id == 0)
    {
        return false;
    }
    const std::shared_ptr<CTaskThread> thread = CTaskThreadManager::instance().getThreadInterface(m_id);
    return thread && thread->tryPostTaskResult(task, 1, true, true).m_committed;
}

bool LibuvTestThread::join()
{
    if (m_id == 0)
    {
        return true;
    }
    const CTaskThreadResult result = CTaskThreadManager::instance().waitForEnd(m_id);
    if (result.m_status != CTaskThreadOk)
    {
        return false;
    }
    m_id = 0;
    return true;
}

LibuvTestClient::LibuvTestClient() :
m_target(nullptr),
m_received(0),
m_failed(false),
m_sendBufferSize(0)
{
}

void LibuvTestClient::uvServerConnected(uv_tcp_t* server)
{
    if (m_sendBufferSize > 0)
    {
        int size = m_sendBufferSize;
        if (uv_send_buffer_size(reinterpret_cast<uv_handle_t*>(server), &size) != 0)
        {
            m_failed.store(true);
        }
    }
    m_target.store(server, std::memory_order_release);
}

void LibuvTestClient::receive(uv_tcp_t* sender, const char* buffer, int32_t length)
{
    (void)sender;
    for (int32_t index = 0; index < length; ++index)
    {
        if (static_cast<unsigned char>(buffer[index]) != 0xA5)
        {
            m_failed.store(true);
        }
    }
    m_received.fetch_add(static_cast<uint64_t>(length));
}

void LibuvTestClient::uvServerNotFind()
{
    m_failed.store(true);
}

void LibuvTestClient::uvServerDisconnected(uv_tcp_t* server)
{
    (void)server;
    m_target.store(nullptr, std::memory_order_release);
}

LibuvTestReceiver::LibuvTestReceiver(uint32_t payload, uint32_t producers, bool timed) :
m_failed(false),
m_exit(false),
m_peer(0),
m_connectedCount(0),
m_closedCount(0),
m_lastReceive(0),
m_payload(payload),
m_producers(producers),
m_timed(timed)
{
    for (size_t index = 0; index < 16; ++index)
    {
        m_counts[index].store(0);
    }
    m_frame.reserve(payload + 4);
    if (timed)
    {
        m_latencies.reserve(65536);
    }
}

void LibuvTestReceiver::onConnected(uint64_t peer)
{
    ++m_connectedCount;
    if (peer == 0 || m_peer != 0)
    {
        m_failed.store(true);
    }
    m_peer = peer;
}

void LibuvTestReceiver::onData(uint64_t peer, const uint8_t* data, uint32_t size)
{
    if (m_failed.load() || peer != m_peer)
    {
        m_failed.store(true);
        return;
    }
    uint32_t offset = 0;
    while (offset < size)
    {
        const size_t copied = (std::min)(size_t(size - offset), size_t(m_payload + 4) - m_frame.size());
        m_frame.insert(m_frame.end(), data + offset, data + offset + copied);
        offset += static_cast<uint32_t>(copied);
        if (m_frame.size() != m_payload + 4)
        {
            continue;
        }
        uint32_t length = 0;
        uint32_t producer = 0;
        uint32_t sequence = 0;
        std::memcpy(&length, &m_frame[0], 4);
        std::memcpy(&producer, &m_frame[4], 4);
        std::memcpy(&sequence, &m_frame[8], 4);
        if (length != m_payload || producer >= m_producers || sequence != m_counts[producer].load())
        {
            std::fprintf(stderr, "frame mismatch length=%u expected=%u producer=%u sequence=%u expectedSequence=%u\n",
                length, m_payload, producer, sequence, producer < m_producers ? m_counts[producer].load() : 0);
            m_failed.store(true);
            return;
        }
        for (size_t index = m_timed ? 20 : 12; index < m_frame.size(); ++index)
        {
            if (m_frame[index] != static_cast<uint8_t>(sequence + producer + index - 4))
            {
                std::fprintf(stderr, "payload mismatch producer=%u sequence=%u offset=%u\n", producer, sequence, static_cast<uint32_t>(index));
                m_failed.store(true);
                return;
            }
        }
        if (m_timed)
        {
            const uint64_t now = uv_hrtime();
            uint64_t sent = 0;
            std::memcpy(&sent, &m_frame[12], sizeof(sent));
            if (sent == 0 || now < sent)
            {
                m_failed.store(true);
                return;
            }
            if ((sequence & 63U) == 0)
            {
                m_latencies.push_back(now - sent);
            }
            m_lastReceive.store(now, std::memory_order_relaxed);
        }
        m_counts[producer].fetch_add(1);
        m_frame.clear();
    }
}

void LibuvTestReceiver::onWritable(uint64_t peer)
{
    (void)peer;
}

void LibuvTestReceiver::onClosed(uint64_t peer, int32_t reason)
{
    (void)reason;
    ++m_closedCount;
    if (peer == 0 || peer != m_peer)
    {
        m_failed.store(true);
    }
    if (peer == m_peer)
    {
        m_peer = 0;
    }
}

LibuvTestHost::LibuvTestHost() :
m_instance(nullptr),
m_peer(0),
m_received(0),
m_connected(0),
m_written(0),
m_closed(0),
m_throwOn(0),
m_failed(false)
{
}

LibuvTestHost::~LibuvTestHost()
{
    finish();
}

bool LibuvTestHost::initialize()
{
    return m_instance == nullptr && LibuvTcp::hostCreate(nullptr, this, connected, data, writable, closed, &m_instance) == 0;
}

bool LibuvTestHost::finish()
{
    if (LibuvTcp::hostDestroy(m_instance) != 0)
    {
        return false;
    }
    m_instance = nullptr;
    return true;
}

void LibuvTestHost::connected(void* context, uint64_t peer)
{
    LibuvTestHost* observer = static_cast<LibuvTestHost*>(context);
    observer->m_peer = peer;
    ++observer->m_connected;
    observer->m_failed = observer->m_failed || LibuvTcp::hostPoll(observer->m_instance) >= 0 ||
        LibuvTcp::hostDestroy(observer->m_instance) >= 0;
    if (observer->m_throwOn == 1)
    {
        throw 1;
    }
}

void LibuvTestHost::data(void* context, uint64_t peer, const uint8_t* data, uint32_t size)
{
    LibuvTestHost* observer = static_cast<LibuvTestHost*>(context);
    observer->m_failed = observer->m_failed || peer != observer->m_peer;
    for (uint32_t index = 0; index < size; ++index)
    {
        observer->m_failed = observer->m_failed || data[index] != 0x5A;
    }
    observer->m_received += size;
    if (observer->m_throwOn == 2)
    {
        throw 2;
    }
}

void LibuvTestHost::writable(void* context, uint64_t peer)
{
    LibuvTestHost* observer = static_cast<LibuvTestHost*>(context);
    observer->m_failed = observer->m_failed || peer != observer->m_peer;
    ++observer->m_written;
    if (observer->m_throwOn == 3)
    {
        throw 3;
    }
}

void LibuvTestHost::closed(void* context, uint64_t peer, int32_t reason)
{
    (void)peer;
    (void)reason;
    LibuvTestHost* observer = static_cast<LibuvTestHost*>(context);
    ++observer->m_closed;
    observer->m_failed = observer->m_failed || LibuvTcp::hostPoll(observer->m_instance) >= 0 ||
        LibuvTcp::hostDestroy(observer->m_instance) >= 0;
    if (observer->m_throwOn == 4)
    {
        throw 4;
    }
}

LibuvTestWindowProbe::LibuvTestWindowProbe() :
m_full(false),
m_canceled(false),
m_checkOwnerWait(false),
m_ownerWaitChecked(false),
m_ownerWaitRejected(false),
m_pauseLoop(false),
m_loopPaused(false),
m_ticks(0),
m_client(nullptr)
{
}

bool LibuvTestWindowProbe::start(LibuvTcp& client)
{
    m_client = &client;
    if (uv_timer_init(client.loopPtr(), &m_timer) != 0)
    {
        return false;
    }
    m_timer.data = this;
    if (uv_timer_start(&m_timer, &LibuvTestWindowProbe::tick, 1, 1) != 0)
    {
        uv_close(reinterpret_cast<uv_handle_t*>(&m_timer), nullptr);
        return false;
    }
    return true;
}

void LibuvTestWindowProbe::tick(uv_timer_t* timer)
{
    LibuvTestWindowProbe* probe = static_cast<LibuvTestWindowProbe*>(timer->data);
    LibuvBase* state = probe->m_client->m_libuv;
    probe->m_ticks.fetch_add(1);
    if (probe->m_pauseLoop.load())
    {
        probe->m_loopPaused.store(true);
        while (probe->m_pauseLoop.load() && !state->m_stopping.load())
        {
            uv_sleep(1);
        }
    }
#ifndef LIBUV_TEST_BASELINE
    if (probe->m_checkOwnerWait && !probe->m_ownerWaitChecked.load() &&
        state->m_pendingBytes.load() > 63U * 1024U * 1024U && !state->m_stopping.load())
    {
        try
        {
            const int32_t length = 1024 * 1024;
            uv_tcp_t* target = state->mainTcp();
            char* buffer = probe->m_client->memoryForSend(static_cast<int32_t>(sizeof(target) + 4 + length));
            std::memcpy(buffer, &target, sizeof(target));
            std::memcpy(buffer + sizeof(target), &length, 4);
            const int32_t result = probe->m_client->trySend(buffer, true, true);
            if (result != 0)
            {
                delete[] buffer;
            }
            probe->m_ownerWaitRejected.store(result == UV_EPERM);
        }
        catch (...)
        {
            probe->m_ownerWaitRejected.store(false);
        }
        probe->m_ownerWaitChecked.store(true);
    }
    if (state->m_activeWrites == 256)
    {
        probe->m_full.store(true);
    }
    if (probe->m_full.load() && state->m_activeWrites == 0 && state->m_pendingBytes.load() == 0 &&
        state->m_ownerConnections.empty())
    {
        probe->m_canceled.store(true);
    }
#endif
    if (state->m_stopping.load())
    {
        uv_close(reinterpret_cast<uv_handle_t*>(timer), nullptr);
    }
}