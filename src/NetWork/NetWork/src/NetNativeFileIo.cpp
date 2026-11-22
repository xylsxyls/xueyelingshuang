#include "NetNativeFileIo.h"
#include "NetWorkMacro.h"

#ifndef __EMSCRIPTEN__
#include "NetNativeFileIoState.h"
#include <cstring>

std::unique_ptr<NetWorkFilePools> NetNativeFileIo::m_pools;

int32_t NetNativeFileIo::configurePools(const NetWorkFilePools& pools)
{
    if (NetFileIoState::m_instances.load() != 0)
    {
        return NI_INVALID_STATE;
    }
    try
    {
        std::unique_ptr<NetWorkFilePools> copy(new NetWorkFilePools(pools));
        NetFileIoState probe(pools);
        if (probe.m_poolStatus != NI_OK)
        {
            return probe.m_poolStatus;
        }
        m_pools.swap(copy);
        return NI_OK;
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

NetNativeFileIo::NetNativeFileIo() :
m_thread(0)
{

}

NetNativeFileIo::~NetNativeFileIo()
{
    uninit();
}

NetFileIoBackend* NetNativeFileIo::create()
{
    return new NetNativeFileIo();
}

int32_t NetNativeFileIo::init()
{
    if (m_thread != 0 || m_state)
    {
        return NI_INVALID_STATE;
    }
    try
    {
        m_state.reset(new NetFileIoState(m_pools ? *m_pools : NetWorkFilePools()));
        if (m_state->m_poolStatus != NI_OK)
        {
            const int32_t result = m_state->m_poolStatus;
            m_state.reset();
            return result;
        }
        m_thread = CTaskThreadManager::instance().init();
        auto executor = CTaskThreadManager::instance().getThreadInterface(m_thread);
        if (!executor)
        {
            uninit();
            return NI_INVALID_STATE;
        }
        const CTaskThreadResult posted = executor->tryPostTaskResult(std::shared_ptr<CTask>(new NetFileIoTask(m_state)), 1, true, false);
        if (!posted.m_committed)
        {
            executor.reset();
            uninit();
            return posted.m_status == CTaskThreadNoMemory ? NI_NO_MEMORY : NI_INVALID_STATE;
        }
        executor.reset();
        return NI_OK;
    }
    catch (...)
    {
        uninit();
        return NI_NO_MEMORY;
    }
}

int32_t NetNativeFileIo::nextRequestId(uint64_t& id)
{
    id = 0;
    if (!m_state || m_state->m_stop.load())
    {
        return NI_INVALID_STATE;
    }
    if (m_state->m_allocatedRequest == UINT64_MAX)
    {
        return NI_LIMIT_EXCEEDED;
    }
    id = ++m_state->m_allocatedRequest;
    return NI_OK;
}

int32_t NetNativeFileIo::submit(const NetFileIoRequest& request)
{
    if (!m_state || m_state->m_stop.load())
    {
        return NI_INVALID_STATE;
    }
    if (request.id == 0 || request.operation < NET_FILE_IO_OPEN_READ || request.operation > NET_FILE_IO_OPEN_TEMPORARY || request.size > 65536 ||
        (request.operation == NET_FILE_IO_WRITE && request.size != 0 && request.bytes == nullptr) || request.flags > 1 ||
        (request.path != nullptr && std::strlen(request.path) > 4096) ||
        (request.destination != nullptr && std::strlen(request.destination) > 4096))
    {
        return NI_INVALID_ARGUMENT;
    }
    const uint32_t inputSize = request.operation == NET_FILE_IO_WRITE ? request.size : 0;
    // 关闭与接纳共享一个原子字；成功的在途租约允许本次提交完成，执行者不能提前退出。
    NetFileIoAdmission admission(*m_state);
    if (!admission.accepted())
    {
        return NI_INVALID_STATE;
    }
    if (request.id <= m_state->m_lastRequest)
    {
        return NI_INVALID_ARGUMENT;
    }
    m_state->m_lastRequest = request.id;
    if (request.id > m_state->m_allocatedRequest)
    {
        m_state->m_allocatedRequest = request.id;
    }
    try
    {
        NetObjectReference<NetFileIoEntry> entry;
        int32_t prepared = m_state->m_entryPool.create(entry, request);
        if (prepared != NI_OK)
        {
            return prepared;
        }
        prepared = m_state->m_bodies.allocate(inputSize, entry->m_input);
        if (prepared != NI_OK)
        {
            return prepared;
        }
        entry->m_inputSize = inputSize;
        if (inputSize != 0)
        {
            std::memcpy(entry->m_input.data(), request.bytes, inputSize);
        }
        const uint32_t outputSize = request.operation == NET_FILE_IO_READ ? request.size :
            (request.operation == NET_FILE_IO_HASH ? 32U : 0U);
        prepared = m_state->m_bodies.allocate(outputSize, entry->m_output);
        if (prepared != NI_OK)
        {
            return prepared;
        }
        m_state->m_entries.insert(std::make_pair(request.id, entry));
        // 不等待容量，文件owner必须继续消费结果才能归还对象与队列存储。
        const LockFreeQueueResult queued = m_state->m_queued.tryPush(entry.get(), true, false);
        if (queued != LockFreeQueueSuccess)
        {
            m_state->m_entries.erase(request.id);
            if (queued == LockFreeQueueCapacityLimit || queued == LockFreeQueueResourceUnavailable ||
                queued == LockFreeQueueGrowthDeclined || queued == LockFreeQueueSequenceExhausted)
            {
                return NI_LIMIT_EXCEEDED;
            }
            return queued == LockFreeQueueAllocationFailure ? NI_NO_MEMORY : NI_INVALID_STATE;
        }
        m_state->m_inputBytes += inputSize;
        return NI_OK;
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t NetNativeFileIo::cancel(uint64_t id)
{
    if (!m_state)
    {
        return NI_INVALID_STATE;
    }
    const auto entry = m_state->m_entries.find(id);
    if (entry == m_state->m_entries.end())
    {
        return NI_NOT_FOUND;
    }
    entry->second->m_cancelled.store(true);
    m_state->wake();
    return NI_OK;
}

int32_t NetNativeFileIo::peek(NetFileIoResult& result)
{
    std::memset(&result, 0, sizeof(result));
    if (!m_state)
    {
        return NI_INVALID_STATE;
    }
    if (m_state->m_entries.empty())
    {
        return NI_NEED_MORE;
    }
    const auto found = m_state->m_entries.begin();
    if (!found->second->m_completed.load(std::memory_order_acquire))
    {
        return NI_NEED_MORE;
    }
    result = found->second->m_result;
    result.bytes = found->second->m_outputSize == 0 ? nullptr : found->second->m_output.data();
    result.size = found->second->m_outputSize;
    return NI_READY;
}

int32_t NetNativeFileIo::consume(uint64_t id)
{
    if (!m_state)
    {
        return NI_INVALID_STATE;
    }
    if (m_state->m_entries.empty())
    {
        return NI_INVALID_STATE;
    }
    const auto entry = m_state->m_entries.begin();
    if (entry->first != id || !entry->second->m_completed.load(std::memory_order_acquire))
    {
        return NI_INVALID_STATE;
    }
    m_state->m_inputBytes -= entry->second->m_inputSize;
    m_state->m_entries.erase(entry);
    return NI_OK;
}

void NetNativeFileIo::stop()
{
    if (m_state)
    {
        m_state->stop();
    }
}

bool NetNativeFileIo::done() const
{
    return !m_state || m_state->m_done.load();
}

void NetNativeFileIo::uninit()
{
    stop();
    if (m_thread != 0)
    {
        CTaskThreadManager::instance().waitForEnd(m_thread);
        m_thread = 0;
    }
    m_state.reset();
}


#endif // !__EMSCRIPTEN__