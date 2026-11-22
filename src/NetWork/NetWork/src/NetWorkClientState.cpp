#include "NetWorkClientState.h"
#include "NetWorkWorker.h"
#include "NetWork.h"
#ifdef __EMSCRIPTEN__
#include <chrono>
#include <thread>
#else
#include "CSystem/CSystemAPI.h"
#endif
#include <cstring>
#include <memory>

NetWorkClientOpaque::NetWorkClientOpaque(const NetWorkClientCallbacks& callbacks) :
m_parallel(true),
m_notified(false),
m_running(false),
m_entered(false),
m_owner(0),
m_error(NI_OK),
m_callbacks(callbacks),
m_peer(nullptr)
{
    NetSenderSubmissionGate gate = {};
    gate.m_context = this;
    gate.m_pause = pause;
    gate.m_wake = wake;
    m_error = m_submission.configureGate(gate);

}

NetWorkClientOpaque::~NetWorkClientOpaque()
{

}

void NetWorkClientOpaque::pause(void* context)
{
    (void)context;
#ifdef __EMSCRIPTEN__
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
#else
    CSystem::Sleep(1);
#endif
}

void NetWorkClientOpaque::wake(void* context)
{
    NetWorkClientOpaque* client = static_cast<NetWorkClientOpaque*>(context);
    // 一个未消费通知已保证owner再检查队列；只在false→true时唤醒宿主。
    // 入口发布屏障与owner清通知后的SC屏障配对：要么本次重新唤醒，要么本轮看见入口。
    if (!client->m_notified.load(std::memory_order_acquire) &&
        !client->m_notified.exchange(true, std::memory_order_acq_rel) && client->m_worker)
    {
        client->m_worker->wake();
    }
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
    return m_running.load() || onOwner() ? NI_INVALID_STATE : NI_OK;
}

int32_t NetWorkClientOpaque::start()
{
    if (m_running.load() || !m_worker) { return NI_INVALID_STATE; }
    m_error = NI_OK;
    return m_worker->add(this);
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