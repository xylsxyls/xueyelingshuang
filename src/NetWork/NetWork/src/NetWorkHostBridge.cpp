#include "NetWorkMacro.h"
#include "NetWorkHostBridge.h"
#ifdef __EMSCRIPTEN__
#include <thread>
#else
#include "CSystem/CSystemAPI.h"
#endif
#include <cstring>

NetHostFunctions NetWorkHostBridge::m_provider = {};
bool NetWorkHostBridge::m_configured = false;
std::unique_ptr<NetWorkHostPools> NetWorkHostBridge::m_pools;
std::atomic<size_t> NetWorkHostBridge::m_instances(0);

/** 入口借用竞争时周期性让出执行机会，不将CAS重试作为资源失败
@param [in,out] attempts 当前连续失败次数
*/
static void NetWorkHostContention(uint32_t& attempts)
{
    if (++attempts == 64)
    {
        attempts = 0;
#ifdef __EMSCRIPTEN__
        std::this_thread::yield();
#else
        CSystem::Sleep(0);
#endif
    }
}

NetWorkHostMail::NetWorkHostMail(uint64_t peer) :
m_peer(peer), m_calls(0), m_pending(0), m_connected(false), m_writable(false),
m_closed(0), m_failure(NI_OK), m_retired(false)
{

}

bool NetWorkHostMail::enter()
{
    uint32_t count = m_calls.load(std::memory_order_acquire);
    uint32_t attempts = 0;
    while (count < 0x7fffffffU)
    {
        if (m_calls.compare_exchange_weak(count, count + 1, std::memory_order_acquire, std::memory_order_acquire))
        {
            return true;
        }
        NetWorkHostContention(attempts);
    }
    return false;
}

void NetWorkHostMail::leave()
{
    m_calls.fetch_sub(1, std::memory_order_release);
}

NetWorkHostEvent::NetWorkHostEvent() :
m_kind(0), m_peer(0), m_reason(0), m_next(nullptr), m_storage(nullptr), m_size(0)
{

}

NetWorkHostEvent::~NetWorkHostEvent()
{
    clear();
}

void NetWorkHostEvent::clear()
{
    NetInterface::wipe(m_bytes.data(), m_size);
    m_size = 0;
    m_bytes.reset();
    m_mail.reset();
}

NetWorkHostPeer::NetWorkHostPeer() :
m_id(0), m_head(nullptr), m_tail(nullptr), m_connected(false), m_announced(false), m_paused(false), m_writable(false),
m_closed(false), m_failed(false), m_reason(0)
{

}

NetWorkHostBridge::NetWorkHostBridge(const NetHostFunctions& functions, void* observer, NetHostConnected connected,
    NetHostData data, NetHostWritable writable, NetHostClosed closed, const NetWorkHostPools& pools) :
m_functions(functions), m_instance(nullptr), m_observer(observer), m_connected(connected), m_data(data),
m_writable(writable), m_closed(closed), m_poolStatus(NI_OK), m_bodies(pools.m_bodies), m_mailPool(pools.m_mail),
m_records(pools.m_records), m_mailboxes(256, pools.m_index), m_incoming(false), m_mailDirty(false),
m_announcements(0), m_announcementBurst(0), m_cursor(0), m_connecting(false), m_stopping(false), m_shutdownAccepted(false),
m_fault(NI_OK), m_faultReported(false), m_polling(false)
{
    m_poolStatus = m_bodies.status();
    if (m_poolStatus == NI_OK) { m_poolStatus = m_mailPool.status(); }
    if (m_poolStatus == NI_OK) { m_poolStatus = m_records.status(); }
    if (m_poolStatus == NI_OK && m_mailboxes.status() != LockFreeHashMapOk)
    {
        m_poolStatus = m_mailboxes.status() == LockFreeHashMapNoMemory ? NI_NO_MEMORY : NI_INVALID_ARGUMENT;
    }
    const LockFreeQueueResult configured = m_incoming.configurePool(pools.m_incoming);
    const LockFreeQueueResult initialized = configured == LockFreeQueueSuccess ? m_incoming.tryInit() : configured;
    if (m_poolStatus == NI_OK && initialized != LockFreeQueueSuccess)
    {
        m_poolStatus = initialized == LockFreeQueueAllocationFailure ? NI_NO_MEMORY : NI_INVALID_ARGUMENT;
    }
    m_instances.fetch_add(1);
}

NetWorkHostBridge::~NetWorkHostBridge()
{
    // 原始宿主已确认不再回调，先归还FIFO再销毁它们借用的池。
    NetWorkHostEvent* input = nullptr;
    while (m_incoming.pop(&input))
    {
        m_records.release(input->m_storage);
    }
    for (auto peer = m_peers.begin(); peer != m_peers.end(); ++peer)
    {
        discard(peer->second);
    }
    m_instances.fetch_sub(1);
}

int32_t NetWorkHostBridge::configurePools(const NetWorkHostPools& pools)
{
    if (m_instances.load() != 0)
    {
        return NI_INVALID_STATE;
    }
    try
    {
        std::unique_ptr<NetWorkHostPools> prepared(new NetWorkHostPools(pools));
        NetHostFunctions empty = {};
        // 使用真实组件验证整组配置；失败不能部分替换后续实例的池。
        NetWorkHostBridge probe(empty, nullptr, nullptr, nullptr, nullptr, nullptr, *prepared);
        if (probe.m_poolStatus != NI_OK)
        {
            return probe.m_poolStatus;
        }
        m_pools.swap(prepared);
        return NI_OK;
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t NetWorkHostBridge::configure(const NetHostFunctions* functions)
{
    if (functions == nullptr || functions->structSize != sizeof(*functions) || functions->version != 1 || functions->create == nullptr ||
        (functions->listen == nullptr && functions->connect == nullptr) || functions->poll == nullptr || functions->write == nullptr ||
        functions->pauseRead == nullptr || functions->close == nullptr || functions->shutdown == nullptr || functions->destroy == nullptr ||
        functions->localPort == nullptr || functions->peerCount == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (m_configured)
    {
        return NI_INVALID_STATE;
    }
    NetHostFunctions wrapper = {};
    wrapper.structSize = sizeof(wrapper);
    wrapper.version = 1;
    wrapper.context = &m_provider;
    wrapper.create = create;
    wrapper.listen = functions->listen == nullptr ? nullptr : listen;
    wrapper.connect = functions->connect == nullptr ? nullptr : connect;
    wrapper.poll = poll;
    wrapper.write = write;
    wrapper.pauseRead = pauseRead;
    wrapper.close = close;
    wrapper.shutdown = shutdown;
    wrapper.destroy = destroy;
    wrapper.localPort = localPort;
    wrapper.peerCount = peerCount;
    wrapper.entropy = functions->entropy == nullptr ? nullptr : entropy;
    wrapper.execution = functions->execution;
    m_provider = *functions;
    const int32_t result = NetPeer::configureHost(wrapper);
    if (result == NI_OK)
    {
        m_configured = true;
    }
    return result;
}

int32_t NetWorkHostBridge::create(void* provider, void* observer, NetHostConnected connected, NetHostData data, NetHostWritable writable, NetHostClosed closed, void** instance)
{
    if (instance != nullptr)
    {
        *instance = nullptr;
    }
    if (provider == nullptr || instance == nullptr || connected == nullptr || data == nullptr || writable == nullptr || closed == nullptr)
    {
        return -1;
    }
    try
    {
        const NetHostFunctions& functions = *static_cast<NetHostFunctions*>(provider);
        if (functions.structSize != sizeof(functions) || functions.version != 1 || functions.create == nullptr ||
            functions.poll == nullptr || functions.write == nullptr || functions.pauseRead == nullptr ||
            functions.close == nullptr || functions.shutdown == nullptr || functions.destroy == nullptr ||
            functions.localPort == nullptr || functions.peerCount == nullptr)
        {
            return -1;
        }
        NetWorkHostPools defaults;
        const NetWorkHostPools& pools = m_pools ? *m_pools : defaults;
        std::unique_ptr<NetWorkHostBridge> bridge(new NetWorkHostBridge(functions, observer, connected, data, writable, closed, pools));
        if (bridge->m_poolStatus != NI_OK)
        {
            return -1;
        }
        int32_t result = -1;
        try
        {
            result = functions.create(functions.context, bridge.get(), NetWorkHostBridge::connected,
                NetWorkHostBridge::data, NetWorkHostBridge::writable, NetWorkHostBridge::closed, &bridge->m_instance);
        }
        catch (...)
        {
            result = -1;
        }
        if (result == 0 && bridge->m_instance != nullptr)
        {
            *instance = bridge.release();
            return 0;
        }
        if (bridge->m_instance != nullptr)
        {
            // 宿主可能仍持有回调上下文；销毁确认之前不能释放桥。
            bridge->m_fault.store(NI_INVALID_STATE);
            {
                bridge->m_stopping = true;
                bridge->m_faultReported = true;
                for (auto peer = bridge->m_peers.begin(); peer != bridge->m_peers.end(); ++peer)
                {
                    bridge->discard(peer->second);
                }
                bridge->m_peers.clear();
                bridge->m_announcements = 0;
            }
            try
            {
                bridge->m_shutdownAccepted = functions.shutdown(bridge->m_instance) == 0;
            }
            catch (...)
            {
                // 仍尝试destroy，返回清理句柄时允许owner重试。
            }
            int32_t destroyed = -1;
            try
            {
                destroyed = functions.destroy(bridge->m_instance);
            }
            catch (...)
            {
                // destroy未确认成功，桥及其回调目标都必须保留。
            }
            if (destroyed != 0)
            {
                *instance = bridge.release();
            }
        }
        return result == 0 ? -1 : result;
    }
    catch (...)
    {
        return -1;
    }
}

NetWorkHostPeer* NetWorkHostBridge::find(uint64_t peer)
{
    const auto found = m_peers.find(peer);
    return found == m_peers.end() ? nullptr : &found->second;
}

NetWorkHostPeer* NetWorkHostBridge::insert(const NetObjectReference<NetWorkHostMail>& mail)
{
    if (!mail || mail->m_retired.load())
    {
        return nullptr;
    }
    try
    {
        NetWorkHostPeer& peer = m_peers[mail->m_peer];
        peer.m_id = mail->m_peer;
        peer.m_mail = mail;
        return &peer;
    }
    catch (...)
    {
        m_fault.store(NI_NO_MEMORY);
        return nullptr;
    }
}

void NetWorkHostBridge::discard(NetWorkHostPeer& peer)
{
    while (peer.m_head != nullptr)
    {
        NetWorkHostEvent* input = peer.m_head;
        peer.m_head = input->m_next;
        m_records.release(input->m_storage);
    }
    peer.m_tail = nullptr;
}

void NetWorkHostBridge::fail(NetWorkHostPeer& peer, int32_t reason)
{
    if (peer.m_failed)
    {
        return;
    }
    discard(peer);
    peer.m_writable = false;
    peer.m_failed = true;
    peer.m_reason = reason;
}

NetObjectReference<NetWorkHostMail> NetWorkHostBridge::mailbox(uint64_t peer, bool create)
{
    NetObjectReference<NetWorkHostMail> mail;
    if (m_mailboxes.get(peer, mail))
    {
        return mail;
    }
    // 兼容get也会将资源错误映射成false，慢路径区分不存在与读取失败。
    decltype(m_mailboxes)::Snapshot existing;
    const LockFreeHashMapResult lookup = m_mailboxes.find(peer, existing);
    if (lookup.m_status == LockFreeHashMapOk)
    {
        return *existing.m_value;
    }
    if (lookup.m_status != LockFreeHashMapNotFound)
    {
        m_fault.store(NI_NO_MEMORY);
        return mail;
    }
    if (!create || m_stopping.load() || m_fault.load() != NI_OK)
    {
        return mail;
    }
    const int32_t allocated = m_mailPool.create(mail, peer);
    if (allocated == NI_OK)
    {
        decltype(m_mailboxes)::Snapshot winner;
        const LockFreeHashMapResult result = m_mailboxes.tryInsertIfAbsent(peer, mail, winner, true, false);
        if (result.m_committed)
        {
            m_mailDirty.store(true, std::memory_order_release);
            return mail;
        }
        if (result.m_status == LockFreeHashMapExists)
        {
            return *winner.m_value;
        }
        m_fault.store(result.m_status == LockFreeHashMapNoMemory ? NI_NO_MEMORY : NI_LIMIT_EXCEEDED);
    }
    else
    {
        // 容量失败前另一个生产者可能刚发布了同一身份，优先取得它的实际槽。
        if (m_mailboxes.get(peer, mail))
        {
            return mail;
        }
        m_fault.store(allocated);
    }
    return NetObjectReference<NetWorkHostMail>();
}

void NetWorkHostBridge::enqueue(uint32_t kind, uint64_t peer, int32_t reason, const uint8_t* bytes, uint32_t size)
{
    if (m_fault.load() != NI_OK)
    {
        return;
    }
    if (peer == 0 || kind > 3)
    {
        m_fault.store(NI_INVALID_ARGUMENT);
        return;
    }
    NetObjectReference<NetWorkHostMail> mail = mailbox(peer, kind == 0 || (kind == 3 && m_connecting.load()));
    if (!mail || mail->m_retired.load())
    {
        return;
    }
    if (kind == 3)
    {
        uint64_t expected = 0;
        mail->m_closed.compare_exchange_strong(expected, (UINT64_C(1) << 32) | static_cast<uint32_t>(reason));
        mail->m_calls.fetch_or(0x80000000U, std::memory_order_acq_rel);
        return;
    }
    if (m_stopping.load() || mail->m_failure.load() < 0 || !mail->enter())
    {
        return;
    }
    if (kind == 0 || kind == 2)
    {
        // 连接、可写及关闭通知都不竞争正文池，容量不足不会吞掉终态。
        if (kind == 0) { mail->m_connected.store(true); }
        else { mail->m_writable.store(true, std::memory_order_release); }
        mail->leave();
        return;
    }
    int32_t failure = NI_OK;
    if (!mail->m_connected.load() || size == 0 || size > 16384 || bytes == nullptr)
    {
        failure = NI_INVALID_ARGUMENT;
    }
    else
    {
        NetObjectReference<NetWorkHostEvent> input;
        failure = m_records.create(input);
        if (failure == NI_OK)
        {
            failure = m_bodies.allocate(size, input->m_bytes);
        }
        if (failure == NI_OK)
        {
            input->m_mail = mail;
            input->m_kind = 1;
            input->m_peer = peer;
            input->m_size = size;
            input->m_storage = input.storage();
            std::memcpy(input->m_bytes.data(), bytes, size);
            mail->m_pending.fetch_add(1);
            const LockFreeQueueResult queued = m_incoming.tryPush(input.get(), true, false);
            if (queued == LockFreeQueueSuccess)
            {
                // owner可立即消费，发布之后只能放弃本地引用，不能解引用input。
                input.release();
            }
            else
            {
                mail->m_pending.fetch_sub(1);
                failure = queued == LockFreeQueueAllocationFailure ? NI_NO_MEMORY :
                    queued == LockFreeQueueInvalidState ? NI_INVALID_STATE : NI_LIMIT_EXCEEDED;
            }
        }
    }
    if (failure != NI_OK)
    {
        int32_t expected = NI_OK;
        mail->m_failure.compare_exchange_strong(expected, failure);
    }
    mail->leave();
}

void NetWorkHostBridge::retireMail(uint64_t peer)
{
    NetWorkHostPeer* state = find(peer);
    if (state == nullptr || !state->m_mail)
    {
        return;
    }
    state->m_mail->m_retired.store(true);
    state->m_mail->m_calls.fetch_or(0x80000000U);
    if (!m_mailboxes.tryErase(peer, true, false).m_committed)
    {
        m_fault.store(NI_INVALID_STATE);
    }
}

void NetWorkHostBridge::pumpMail()
{
    if (m_faultReported)
    {
        return;
    }
    if (m_mailDirty.exchange(false, std::memory_order_acq_rel))
    {
        // id为借用身份，mail为遍历保护的实际连接对象；只在owner创建派发目录。
        const LockFreeHashMapResult scanned = m_mailboxes.forEachResult([this](const uint64_t& id, const NetObjectReference<NetWorkHostMail>& mail)
        {
            (void)id;
            if (!mail->m_retired.load()) { insert(mail); }
        });
        if (scanned.m_status != LockFreeHashMapOk)
        {
            m_fault.store(NI_NO_MEMORY);
        }
    }
    NetWorkHostEvent* incoming = nullptr;
    // 这是每轮导入的公平工作量；余项继续持有池槽，并未限制总接纳量。
    for (uint32_t step = 0; step < 4096 && m_incoming.pop(&incoming); ++step)
    {
        NetObjectReference<NetWorkHostEvent> input;
        input.adopt(m_records, incoming->m_storage);
        NetWorkHostPeer* state = nullptr;
        if (!input->m_mail->m_retired.load() && input->m_mail->m_failure.load() == NI_OK)
        {
            state = insert(input->m_mail);
        }
        input->m_mail->m_pending.fetch_sub(1);
        if (state != nullptr && !state->m_failed && !state->m_closed && !m_stopping.load())
        {
            if (state->m_tail == nullptr) { state->m_head = input.get(); }
            else { state->m_tail->m_next = input.get(); }
            state->m_tail = input.release();
        }
    }
    for (auto it = m_peers.begin(); it != m_peers.end(); ++it)
    {
        NetWorkHostPeer& state = it->second;
        NetWorkHostMail& mail = *state.m_mail;
        // 关闭状态先acquire，保证不会漏读已退出生产者最后发布的connected。
        const uint32_t calls = mail.m_calls.load(std::memory_order_acquire);
        if (mail.m_connected.load() && !state.m_connected)
        {
            state.m_connected = true;
            ++m_announcements;
        }
        const int32_t failure = mail.m_failure.load();
        if (failure < 0) { fail(state, failure); }
        if (!state.m_failed && !state.m_closed && mail.m_writable.exchange(false, std::memory_order_acq_rel))
        {
            state.m_writable = true;
        }
        if (calls == 0x80000000U && mail.m_pending.load() == 0 && mail.m_closed.load() != 0)
        {
            state.m_closed = true;
            if (!state.m_failed) { state.m_reason = static_cast<int32_t>(static_cast<uint32_t>(mail.m_closed.load())); }
        }
    }
    m_incoming.maintain(16);
    m_mailboxes.maintain(16);
}

void NetWorkHostBridge::connected(void* context, uint64_t peer)
{
    static_cast<NetWorkHostBridge*>(context)->enqueue(0, peer, 0, nullptr, 0);
}

void NetWorkHostBridge::data(void* context, uint64_t peer, const uint8_t* bytes, uint32_t size)
{
    static_cast<NetWorkHostBridge*>(context)->enqueue(1, peer, 0, bytes, size);
}

void NetWorkHostBridge::writable(void* context, uint64_t peer)
{
    static_cast<NetWorkHostBridge*>(context)->enqueue(2, peer, 0, nullptr, 0);
}

void NetWorkHostBridge::closed(void* context, uint64_t peer, int32_t reason)
{
    static_cast<NetWorkHostBridge*>(context)->enqueue(3, peer, reason, nullptr, 0);
}

void NetWorkHostBridge::drain()
{
    pumpMail();
    if (m_fault.load() != NI_OK)
    {
        if (m_faultReported) { return; }
        m_faultReported = true;
        m_stopping = true;
        try
        {
            if (!m_shutdownAccepted) { m_shutdownAccepted = m_functions.shutdown(m_instance) == 0; }
        }
        catch (...)
        {
            // 宿主未确认停止时仍保留清理句柄，已知连接必须逐个收到终态。
        }
        while (!m_peers.empty())
        {
            auto it = m_peers.begin();
            const uint64_t id = it->first;
            discard(it->second);
            retireMail(id);
            m_peers.erase(it);
            try { m_closed(m_observer, id, m_fault.load()); }
            catch (...)
            {
                // 单个观察者异常不能吞掉其余连接终态。
            }
        }
        m_announcements = 0;
        return;
    }
    // 每轮最多256次回调，避免无限生产者饿死宿主poll；不限制队列容量。
    for (uint32_t step = 0; step < 256 && !m_peers.empty(); ++step)
    {
        uint32_t kind = 0;
        uint64_t id = 0;
        int32_t reason = 0;
        bool closeRaw = false;
        NetObjectReference<NetWorkHostEvent> input;
        if (m_fault.load() != NI_OK) { return; }
        if (m_announcements != 0 && m_announcementBurst < 32)
        {
            for (auto it = m_peers.begin(); it != m_peers.end(); ++it)
            {
                NetWorkHostPeer& state = it->second;
                if (state.m_connected && !state.m_announced)
                {
                    state.m_announced = true;
                    --m_announcements;
                    ++m_announcementBurst;
                    id = it->first;
                    break;
                }
            }
        }
        if (id == 0)
        {
            auto it = m_peers.upper_bound(m_cursor);
            for (size_t scan = 0, count = m_peers.size(); scan < count; ++scan)
            {
                if (it == m_peers.end()) { it = m_peers.begin(); }
                NetWorkHostPeer& state = it->second;
                if (state.m_connected && !state.m_announced)
                {
                    ++it;
                    continue;
                }
                if (state.m_failed || (state.m_closed && state.m_head == nullptr && !state.m_writable))
                {
                    kind = 3;
                    id = it->first;
                    reason = state.m_reason;
                    closeRaw = state.m_failed && !state.m_closed;
                    discard(state);
                    retireMail(id);
                    m_peers.erase(it);
                }
                else if (state.m_writable)
                {
                    kind = 2;
                    id = it->first;
                    state.m_writable = false;
                }
                else if (state.m_head != nullptr && !state.m_paused)
                {
                    kind = 1;
                    id = it->first;
                    input.adopt(m_records, state.m_head->m_storage);
                    state.m_head = input->m_next;
                    if (state.m_head == nullptr) { state.m_tail = nullptr; }
                    input->m_next = nullptr;
                }
                if (id != 0)
                {
                    m_cursor = id;
                    m_announcementBurst = 0;
                    break;
                }
                ++it;
            }
        }
        if (id == 0)
        {
            m_announcementBurst = 0;
            if (m_announcements != 0) { continue; }
            return;
        }
        if (closeRaw)
        {
            try
            {
                if (m_functions.close(m_instance, id, reason) != 0) { m_fault.store(NI_INVALID_STATE); }
            }
            catch (...)
            {
                m_fault.store(NI_INVALID_STATE);
            }
        }
        if (kind == 0) { m_connected(m_observer, id); }
        else if (kind == 1) { m_data(m_observer, id, input->m_bytes.data(), input->m_size); }
        else if (kind == 2) { m_writable(m_observer, id); }
        else { m_closed(m_observer, id, reason); }
        // input在观察者退出后才归还记录与正文，回调内的重入不能复用借用缓冲。
    }
}

int32_t NetWorkHostBridge::listen(void* instance, const char* address, uint32_t port, uint32_t maxPeers)
{
    try
    {
        NetWorkHostBridge* bridge = static_cast<NetWorkHostBridge*>(instance);
        if (bridge->m_fault.load() != NI_OK || bridge->m_stopping || bridge->m_functions.listen == nullptr || maxPeers == 0)
        {
            return -1;
        }
        return bridge->m_functions.listen(bridge->m_instance, address, port, maxPeers);
    }
    catch (...)
    {
        return -1;
    }
}

int32_t NetWorkHostBridge::connect(void* instance, const char* address, uint32_t port, uint64_t* peer)
{
    if (peer == nullptr) { return -1; }
    *peer = 0;
    NetWorkHostBridge* bridge = static_cast<NetWorkHostBridge*>(instance);
    try
    {
        if (bridge->m_stopping || bridge->m_fault.load() != NI_OK || bridge->m_functions.connect == nullptr)
        {
            return -1;
        }
        bridge->m_connecting = true;
        int32_t result = bridge->m_functions.connect(bridge->m_instance, address, port, peer);
        bridge->m_connecting = false;
        if (result == 0 && (*peer == 0 || bridge->insert(bridge->mailbox(*peer, true)) == nullptr))
        {
            bridge->m_fault.store(NI_NO_MEMORY);
            result = -1;
        }
        if (result != 0) { *peer = 0; }
        return result;
    }
    catch (...)
    {
        bridge->m_connecting = false;
        bridge->m_fault.store(NI_NO_MEMORY);
        *peer = 0;
        return -1;
    }
}

int32_t NetWorkHostBridge::poll(void* instance)
{
    NetWorkHostBridge* bridge = static_cast<NetWorkHostBridge*>(instance);
    if (bridge->m_polling)
    {
        return -1;
    }
    bridge->m_polling = true;
    try
    {
        const int32_t result = bridge->m_functions.poll(bridge->m_instance);
        bridge->drain();
        bridge->m_polling = false;
        if (bridge->m_fault.load() != NI_OK)
        {
            return -1;
        }
        return result == 0 && !bridge->m_peers.empty() ? 1 : result;
    }
    catch (...)
    {
        bridge->m_polling = false;
        bridge->m_fault.store(NI_INVALID_STATE);
        return -1;
    }
}

int32_t NetWorkHostBridge::write(void* instance, uint64_t peer, const uint8_t* bytes, uint32_t size)
{
    try
    {
        NetWorkHostBridge* bridge = static_cast<NetWorkHostBridge*>(instance);
        if (bridge->m_fault.load() != NI_OK || bridge->m_stopping)
        {
            return -1;
        }
        return bridge->m_functions.write(bridge->m_instance, peer, bytes, size);
    }
    catch (...)
    {
        return -1;
    }
}

int32_t NetWorkHostBridge::pauseRead(void* instance, uint64_t peer, int32_t paused)
{
    try
    {
        NetWorkHostBridge* bridge = static_cast<NetWorkHostBridge*>(instance);
        NetWorkHostPeer* state = bridge->find(peer);
        if (state != nullptr)
        {
            state->m_paused = paused != 0;
            if (state->m_closed || state->m_failed || state->m_mail->m_closed.load() != 0)
            {
                return 0;
            }
        }
        const int32_t result = bridge->m_functions.pauseRead(bridge->m_instance, peer, paused);
        if (result != 0 && state != nullptr &&
            (state->m_closed || state->m_failed || state->m_mail->m_closed.load() != 0))
        {
            return 0;
        }
        return result;
    }
    catch (...)
    {
        return -1;
    }
}

int32_t NetWorkHostBridge::close(void* instance, uint64_t peer, int32_t reason)
{
    try
    {
        NetWorkHostBridge* bridge = static_cast<NetWorkHostBridge*>(instance);
        return bridge->m_functions.close(bridge->m_instance, peer, reason);
    }
    catch (...)
    {
        return -1;
    }
}

int32_t NetWorkHostBridge::shutdown(void* instance)
{
    NetWorkHostBridge* bridge = static_cast<NetWorkHostBridge*>(instance);
    const bool drive = !bridge->m_polling;
    try
    {
        {
            bridge->m_stopping = true;
            for (auto peer = bridge->m_peers.begin(); peer != bridge->m_peers.end(); ++peer)
            {
                peer->second.m_paused = false;
                bridge->discard(peer->second);
            }
        }
        if (drive)
        {
            bridge->m_polling = true;
        }
        const int32_t result = bridge->m_shutdownAccepted ? 0 : bridge->m_functions.shutdown(bridge->m_instance);
        if (result == 0)
        {
            bridge->m_shutdownAccepted = true;
        }
        if (drive)
        {
            bridge->drain();
            bridge->m_polling = false;
        }
        return result;
    }
    catch (...)
    {
        if (drive)
        {
            bridge->m_polling = false;
        }
        return -1;
    }
}

int32_t NetWorkHostBridge::destroy(void* instance)
{
    if (instance == nullptr)
    {
        return 0;
    }
    try
    {
        NetWorkHostBridge* bridge = static_cast<NetWorkHostBridge*>(instance);
        if (bridge->m_polling)
        {
            return -1;
        }
        const int32_t result = bridge->m_functions.destroy(bridge->m_instance);
        if (result == 0)
        {
            delete bridge;
        }
        return result;
    }
    catch (...)
    {
        return -1;
    }
}

uint32_t NetWorkHostBridge::localPort(void* instance)
{
    try
    {
        NetWorkHostBridge* bridge = static_cast<NetWorkHostBridge*>(instance);
        return bridge->m_functions.localPort(bridge->m_instance);
    }
    catch (...)
    {
        return 0;
    }
}

uint32_t NetWorkHostBridge::peerCount(void* instance)
{
    try
    {
        NetWorkHostBridge* bridge = static_cast<NetWorkHostBridge*>(instance);
        return bridge->m_functions.peerCount(bridge->m_instance);
    }
    catch (...)
    {
        return 0;
    }
}

int32_t NetWorkHostBridge::entropy(void* provider, uint8_t* bytes, uint32_t size)
{
    try
    {
        const NetHostFunctions* functions = static_cast<NetHostFunctions*>(provider);
        return functions->entropy == nullptr ? -1 : functions->entropy(functions->context, bytes, size);
    }
    catch (...)
    {
        return -1;
    }
}