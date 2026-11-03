#include "NetWorkClientState.h"
#include "NetWork.h"
#include "CSystem/CSystemAPI.h"
#include <cstring>
#include <memory>

NetWorkClientOpaque::NetWorkClientOpaque(const NetWorkClientCallbacks& callbacks) :
m_notified(false),
m_running(false),
m_entered(false),
m_owner(0),
m_error(NI_OK),
m_callbacks(callbacks),
m_peer(nullptr),
m_thread(0)
{
    NetSenderSubmissionGate gate = {};
    gate.m_context = this;
    gate.m_enter = enter;
    gate.m_leave = leave;
    gate.m_wake = wake;
    m_error = m_submitMutex.valid() && m_wakeGate.valid() && m_sendGate.valid() ?
        m_submission.configureGate(gate) : NI_INVALID_STATE;
}

NetWorkClientOpaque::~NetWorkClientOpaque()
{

}

void NetWorkClientOpaque::enter(void* context)
{
    static_cast<NetWorkClientOpaque*>(context)->m_submitMutex.lock();
}

void NetWorkClientOpaque::leave(void* context)
{
    static_cast<NetWorkClientOpaque*>(context)->m_submitMutex.unlock();
}

void NetWorkClientOpaque::wake(void* context)
{
    NetWorkClientOpaque* client = static_cast<NetWorkClientOpaque*>(context);
    {
        // 与owner检查标志及wait的原子解锁配对，不能丢失检查后、等待前的通知。
        std::lock_guard<NetWorkGate> lock(client->m_wakeGate);
        client->m_notified.store(true);
    }
    client->m_wakeGate.notify();
}

uintptr_t NetWorkClientOpaque::currentThreadId()
{
#ifdef __EMSCRIPTEN__
    return reinterpret_cast<uintptr_t>(pthread_self());
#else
    return static_cast<uintptr_t>(CSystem::SystemThreadId());
#endif
}

bool NetWorkClientOpaque::onOwner() const
{
    return m_owner.load() == NetWorkClientOpaque::currentThreadId();
}

int32_t NetWorkClientOpaque::reap()
{
    if (m_running.load() || onOwner())
    {
        return NI_INVALID_STATE;
    }
    if (m_thread != 0)
    {
        CTaskThreadManager::Instance().Uninit(m_thread);
        m_thread = 0;
    }
    return NI_OK;
}

int32_t NetWorkClientOpaque::start()
{
    if (m_thread != 0 || m_running.load())
    {
        return NI_INVALID_STATE;
    }
    try
    {
        std::shared_ptr<CTask> task(new NetWorkClientTask(this));
        m_thread = CTaskThreadManager::Instance().Init();
        std::shared_ptr<CTaskThread> thread = CTaskThreadManager::Instance().GetThreadInterface(m_thread);
        if (!thread)
        {
            m_submission.close(NI_INVALID_STATE);
            m_error = NI_INVALID_STATE;
            return NI_INVALID_STATE;
        }
        m_entered = false;
        m_error = NI_OK;
        m_running = true;
        // 新建且独占的线程，生命期owner不会并发停止；协议就绪仍只认实际握手。
        thread->PostTask(task);
        return NI_OK;
    }
    catch (...)
    {
        m_running = false;
        m_error = NI_NO_MEMORY;
        m_submission.close(NI_NO_MEMORY);
        return NI_NO_MEMORY;
    }
}

NetWorkClientTask::NetWorkClientTask(NetWorkClientOpaque* client) :
m_client(client)
{

}

void NetWorkClientTask::StopTask()
{
    NetWork::instance().clientClose(m_client, NI_CANCELLED);
}

void NetWorkClientTask::DoTask()
{
    m_client->m_owner = NetWorkClientOpaque::currentThreadId();
    m_client->m_entered = true;
    for (;;)
    {
        const int32_t result = NetPeer::instance().clientPoll(m_client->m_peer);
        if (result < 0)
        {
            m_client->m_error = result;
            m_client->m_submission.close(result);
        }
        {
            // 回执检查和通知使用同一门；不持submission门，不执行业务回调。
            std::lock_guard<NetWorkGate> lock(m_client->m_sendGate);
            m_client->m_sendGate.notifyAll();
        }
        bool waited = true;
        {
            // 协议owner独占推进，生产者只需完成短提交；等待超时仍驱动协议期限。
            std::lock_guard<NetWorkGate> lock(m_client->m_wakeGate);
            if (result != NI_OK && !m_client->m_notified.exchange(false))
            {
                waited = m_client->m_wakeGate.waitFor(2);
            }
        }
        if (!waited)
        {
            m_client->m_error = NI_INVALID_STATE;
            NetPeer::instance().clientClose(m_client->m_peer, NI_INVALID_STATE);
        }
        if (result == NI_OK)
        {
            break;
        }
    }
    m_client->m_owner = 0;
    // 最后一条对client的访问；生命期owner先join独占CTask再释放上下文。
    m_client->m_running = false;
}

void NetWorkClientOpaque::ready(void* context, uint64_t generation, uint32_t requests, uint32_t chunk)
{
    NetWorkClientOpaque* client = static_cast<NetWorkClientOpaque*>(context);
    if (!client->m_running.load())
    {
        return;
    }
    client->m_callbacks.ready(client->m_callbacks.context, generation, requests, chunk);
}

void NetWorkClientOpaque::result(void* context, uint64_t generation, uint64_t id, uint32_t product, uint32_t type, uint32_t status, const uint8_t* bytes, uint32_t size)
{
    NetWorkClientOpaque* client = static_cast<NetWorkClientOpaque*>(context);
    if (!client->m_running.load())
    {
        return;
    }
    client->m_callbacks.result(client->m_callbacks.context, generation, id, product, type, status, bytes, size);
}

int32_t NetWorkClientOpaque::frame(void* context, uint64_t generation, const NetInterfaceFrame* value)
{
    NetWorkClientOpaque* client = static_cast<NetWorkClientOpaque*>(context);
    if (!client->m_running.load())
    {
        return 1;
    }
    return client->m_callbacks.frame(client->m_callbacks.context, generation, value);
}

void NetWorkClientOpaque::closed(void* context, uint64_t generation, int32_t reason, int32_t detail)
{
    NetWorkClientOpaque* client = static_cast<NetWorkClientOpaque*>(context);
    if (!client->m_running.load())
    {
        return;
    }
    client->m_callbacks.closed(client->m_callbacks.context, generation, reason, detail);
}

int32_t NetWorkClientOpaque::waitSynchronous(uint64_t ticket)
{
    for (;;)
    {
        const int32_t result = m_submission.synchronousResult(ticket, true);
        if (result != NI_READY)
        {
            return result;
        }
        bool waited = true;
        {
            // 只查询内部回执，不执行协议；wake在释放submission门以后调用。
            std::lock_guard<NetWorkGate> lock(m_sendGate);
            if (m_submission.synchronousResult(ticket, false) == NI_READY)
            {
                waited = m_sendGate.waitFor(2);
            }
        }
        if (!waited)
        {
            m_submission.close(NI_INVALID_STATE);
        }
    }
}