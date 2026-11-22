#include "NetWorkWorker.h"
#include "NetWorkClientState.h"
#ifdef __EMSCRIPTEN__
#include <chrono>
#include <thread>
#else
#include "CSystem/CSystemAPI.h"
#endif
#include <algorithm>

std::vector<std::weak_ptr<NetWorkWorker>> NetWorkWorker::s_workers;
uint32_t NetWorkWorker::s_count = 4;
uint32_t NetWorkWorker::s_next = 0;

NetWorkWorkerCall::NetWorkWorkerCall(const std::function<int32_t()>& action) :
m_action(&action), m_done(false), m_result(NI_INVALID_STATE)
{

}

NetWorkWorker::NetWorkWorker() :
m_commands(false), m_thread(0), m_owner(0), m_invoking(0), m_stopping(false)
{

}

NetWorkWorker::~NetWorkWorker()
{
    stopTask();
    if (m_thread != 0) { CTaskThreadManager::instance().uninit(m_thread); }
}

int32_t NetWorkWorker::configure(uint32_t count)
{
    if (count == 0 || count == UINT32_MAX) { return NI_INVALID_ARGUMENT; }
    for (size_t index = 0; index < s_workers.size(); ++index)
    {
        if (!s_workers[index].expired()) { return NI_INVALID_STATE; }
    }
    s_workers.clear();
    s_count = count;
    s_next = 0;
    return NI_OK;
}

std::shared_ptr<NetWorkWorker> NetWorkWorker::acquire(bool parallel)
{
    try
    {
        if (s_workers.empty()) { s_workers.resize(static_cast<size_t>(s_count) + 1); }
        const uint32_t index = parallel ? s_next++ % s_count : s_count;
        std::shared_ptr<NetWorkWorker> worker = s_workers[index].lock();
        if (!worker)
        {
            worker.reset(new NetWorkWorker());
            if (worker->start() != NI_OK) { return std::shared_ptr<NetWorkWorker>(); }
            s_workers[index] = worker;
        }
        return worker;
    }
    catch (...) { return std::shared_ptr<NetWorkWorker>(); }
}

int32_t NetWorkWorker::start()
{
    if (m_execution.status() != NI_OK) { return m_execution.status(); }
    if (m_commands.tryInit() != LockFreeQueueSuccess) { return NI_NO_MEMORY; }
    m_thread = CTaskThreadManager::instance().init();
    std::shared_ptr<CTaskThread> thread = CTaskThreadManager::instance().getThreadInterface(m_thread);
    if (!thread) { return NI_NO_MEMORY; }
    // CTask只借用this，生命周期owner持有工作者并在析构中join。
    std::shared_ptr<CTask> task(this, [](CTask*) {});
    const CTaskThreadResult posted = thread->tryPostTaskResult(task, 1, true, false);
    return posted.m_committed ? NI_OK : NI_NO_MEMORY;
}

bool NetWorkWorker::onOwner() const
{
    return m_owner.load(std::memory_order_acquire) == NetWorkClientOpaque::currentThreadId();
}

int32_t NetWorkWorker::invoke(const std::function<int32_t()>& action)
{
    if (onOwner()) { return NI_INVALID_STATE; }
    m_invoking.fetch_add(1);
    if (m_stopping.load())
    {
        m_invoking.fetch_sub(1);
        wake();
        return NI_INVALID_STATE;
    }
    NetWorkWorkerCall call(action);
    const LockFreeQueueResult queued = m_commands.tryPushAndFence(&call, true, true);
    if (queued != LockFreeQueueSuccess)
    {
        m_invoking.fetch_sub(1);
        wake();
        return NI_NO_MEMORY;
    }
    wake();
    // 仅创建/连接/销毁走此等待；正常send/post没有等待对象和完成票据。
    while (!call.m_done.load(std::memory_order_acquire))
    {
#ifdef __EMSCRIPTEN__
        std::this_thread::yield();
#else
        CSystem::Sleep(0);
#endif
    }
    m_invoking.fetch_sub(1);
    wake();
    return call.m_result;
}

int32_t NetWorkWorker::prepareClient()
{
    try
    {
        if (m_clients.size() == m_clients.capacity())
        {
            const size_t available = m_clients.max_size() - m_clients.size();
            if (available == 0) { return NI_NO_MEMORY; }
            const size_t grow = m_clients.empty() ? 1 : (std::min)(m_clients.size(), available);
            m_clients.reserve(m_clients.size() + grow);
        }
        return NI_OK;
    }
    catch (...) { return NI_NO_MEMORY; }
}

int32_t NetWorkWorker::add(NetWorkClientOpaque* client)
{
    try { m_clients.push_back(client); }
    catch (...) { return NI_NO_MEMORY; }
    client->m_owner = m_owner.load(std::memory_order_relaxed);
    client->m_entered = true;
    client->m_running = true;
    return NI_OK;
}

void NetWorkWorker::wake()
{
    m_execution.wake();
}

void NetWorkWorker::stopTask()
{
    m_stopping.store(true);
    wake();
}

void NetWorkWorker::doTask()
{
    NetExecutionScope scope(m_execution);
    m_owner.store(NetWorkClientOpaque::currentThreadId(), std::memory_order_release);
    for (;;)
    {
        m_execution.begin();
        NetWorkWorkerCall* call = nullptr;
        // 有限轮次是调度量子，不是接纳上限，避免生命周期命令饿死收发。
        for (uint32_t turn = 0; turn < 64 && m_commands.popReady(&call); ++turn)
        {
            try { call->m_result = (*call->m_action)(); }
            catch (...) { call->m_result = NI_INVALID_STATE; }
            call->m_done.store(true, std::memory_order_release);
        }
        bool notified = m_commands.possiblyReady();
        for (size_t index = 0; index < m_clients.size();)
        {
            NetWorkClientOpaque* client = m_clients[index];
            notified = client->m_notified.exchange(false, std::memory_order_acq_rel) || notified;
            // 与生产者发布后再检查通知的屏障配对，禁止双方都观察不到对方的更新。
            std::atomic_thread_fence(std::memory_order_seq_cst);
            if (m_stopping.load(std::memory_order_acquire)) { client->m_submission.close(NI_CANCELLED); }
            const int32_t result = NetPeer::instance().clientPoll(client->m_peer);
            if (result < 0)
            {
                client->m_error = result;
                client->m_submission.close(result);
            }
            if (result == NI_OK)
            {
                // 活动表只由本线程轮询，无跨会话顺序约定；移除时无需平移其余指针。
                m_clients[index] = m_clients.back();
                m_clients.pop_back();
                client->m_owner = 0;
                // 移出容器后最后一次访问，生命周期owner可随后投递销毁命令。
                client->m_running.store(false, std::memory_order_release);
            }
            else { ++index; }
        }
        if (m_stopping.load() && m_invoking.load() == 0 && m_clients.empty() && !m_commands.possiblyReady()) { break; }
        if (!notified)
        {
            const int32_t waited = m_execution.wait(100);
            if (waited == NI_NEED_MORE)
            {
#ifdef __EMSCRIPTEN__
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
#else
                CSystem::sleepPrecise(2);
#endif
            }
            else if (waited < 0)
            {
                for (size_t index = 0; index < m_clients.size(); ++index)
                {
                    m_clients[index]->m_error = waited;
                    m_clients[index]->m_submission.close(waited);
                }
            }
        }
    }
    m_owner.store(0, std::memory_order_release);
}