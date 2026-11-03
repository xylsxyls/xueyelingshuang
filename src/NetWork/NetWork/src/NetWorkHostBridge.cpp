#include "NetWorkMacro.h"
#include "NetWorkHostBridge.h"
#include <NetPeer/NetPeerAPI.h>
#include <cstring>

NetHostFunctions NetWorkHostBridge::m_provider = {};
bool NetWorkHostBridge::m_configured = false;

NetWorkHostEvent::NetWorkHostEvent() :
m_kind(0), m_peer(0), m_reason(0), m_next(2048), m_size(0)
{

}

NetWorkHostEvent::~NetWorkHostEvent()
{
    clear();
}

void NetWorkHostEvent::clear()
{
    volatile uint8_t* bytes = m_bytes.get();
    for (uint32_t i = 0; i < m_size; ++i)
    {
        bytes[i] = 0;
    }
    m_size = 0;
    m_bytes.reset();
}

NetWorkHostPeer::NetWorkHostPeer() :
m_id(0), m_head(2048), m_tail(2048), m_connected(false), m_announced(false), m_paused(false), m_writable(false),
m_closed(false), m_failed(false), m_reason(0)
{

}

NetWorkHostBridge::NetWorkHostBridge(const NetHostFunctions& functions, void* observer, NetHostConnected connected, NetHostData data, NetHostWritable writable, NetHostClosed closed) :
m_functions(functions), m_instance(nullptr), m_observer(observer), m_connected(connected), m_data(data),
m_writable(writable), m_closed(closed), m_free(0), m_peerCount(0), m_announcements(0), m_cursor(0), m_bytes(0),
m_connecting(false), m_stopping(false), m_shutdownAccepted(false), m_fault(NI_OK), m_faultReported(false), m_polling(false)
{
    for (uint32_t i = 0; i < 2048; ++i)
    {
        m_events[i].m_next = i + 1;
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
        std::unique_ptr<NetWorkHostBridge> bridge(new NetWorkHostBridge(functions, observer, connected, data, writable, closed));
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
                WriteLock lock(bridge->m_mutex);
                bridge->m_stopping = true;
                bridge->m_faultReported = true;
                for (uint32_t i = 0; i < 256; ++i)
                {
                    bridge->discard(bridge->m_peers[i]);
                    bridge->m_peers[i] = NetWorkHostPeer();
                }
                bridge->m_peerCount = 0;
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

uint32_t NetWorkHostBridge::find(uint64_t peer) const
{
    for (uint32_t i = 0; i < 256; ++i)
    {
        if (m_peers[i].m_id == peer)
        {
            return i;
        }
    }
    return 256;
}

uint32_t NetWorkHostBridge::insert(uint64_t peer)
{
    const uint32_t found = find(peer);
    if (found != 256)
    {
        return found;
    }
    if (m_peerCount == 256)
    {
        return 256;
    }
    const uint32_t index = find(0);
    m_peers[index].m_id = peer;
    ++m_peerCount;
    return index;
}

void NetWorkHostBridge::discard(NetWorkHostPeer& peer)
{
    while (peer.m_head != 2048)
    {
        const uint32_t index = peer.m_head;
        NetWorkHostEvent& event = m_events[index];
        peer.m_head = event.m_next;
        m_bytes -= event.m_size;
        event.clear();
        event.m_next = m_free;
        m_free = index;
    }
    peer.m_tail = 2048;
    peer.m_writable = false;
}

void NetWorkHostBridge::fail(NetWorkHostPeer& peer, int32_t reason)
{
    if (peer.m_failed)
    {
        return;
    }
    discard(peer);
    peer.m_failed = true;
    peer.m_reason = reason;
}

void NetWorkHostBridge::enqueue(uint32_t kind, uint64_t peer, int32_t reason, const uint8_t* bytes, uint32_t size)
{
    if (m_fault.load() != NI_OK)
    {
        return;
    }
    try
    {
        WriteLock lock(m_mutex);
        if (m_fault.load() != NI_OK)
        {
            return;
        }
        if (peer == 0 || kind > 3)
        {
            m_fault.store(NI_INVALID_ARGUMENT);
            return;
        }
        uint32_t index = find(peer);
        if (index == 256)
        {
            // 迟到字节、可写和终态不能重新登记已关闭连接。
            if (m_stopping || (kind != 0 && !(kind == 3 && m_connecting)))
            {
                return;
            }
            index = insert(peer);
            if (index == 256)
            {
                m_fault.store(NI_LIMIT_EXCEEDED);
                return;
            }
        }
        NetWorkHostPeer& state = m_peers[index];
        if (state.m_closed)
        {
            return;
        }
        if (kind == 3)
        {
            // 关闭独立于2048个数据/连接/可写槽，即使队列满也不丢失终态。
            state.m_closed = true;
            if (!state.m_failed)
            {
                state.m_reason = reason;
            }
            return;
        }
        if (state.m_failed || m_stopping || (kind == 0 && state.m_connected) || (kind == 2 && state.m_writable))
        {
            return;
        }
        if (kind == 0)
        {
            // 新连接独立登记，不占数据槽；owner按原始递增连接号通告。
            // 后续输入超限也保留这一步，使单连接关闭不变成全宿主协议错误。
            state.m_connected = true;
            ++m_announcements;
            return;
        }
        if ((kind == 1 && (!state.m_connected || size == 0 || size > 16384 || bytes == nullptr)) ||
            m_free == 2048 || size > 4194304U - m_bytes)
        {
            fail(state, NI_LIMIT_EXCEEDED);
            return;
        }
        try
        {
            std::unique_ptr<uint8_t[]> copy;
            if (size != 0)
            {
                copy.reset(new uint8_t[size]);
                std::memcpy(copy.get(), bytes, size);
            }
            const uint32_t slot = m_free;
            NetWorkHostEvent& event = m_events[slot];
            m_free = event.m_next;
            event.m_next = 2048;
            event.m_kind = kind;
            event.m_peer = peer;
            event.m_reason = reason;
            event.m_size = size;
            event.m_bytes.swap(copy);
            if (state.m_tail == 2048)
            {
                state.m_head = slot;
            }
            else
            {
                m_events[state.m_tail].m_next = slot;
            }
            state.m_tail = slot;
            m_bytes += size;
            if (kind == 2)
            {
                state.m_writable = true;
            }
        }
        catch (...)
        {
            fail(state, NI_NO_MEMORY);
        }
    }
    catch (...)
    {
        m_fault.store(NI_INVALID_STATE);
    }
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
    if (m_fault.load() != NI_OK)
    {
        uint64_t peers[256] = {};
        uint32_t count = 0;
        {
            WriteLock lock(m_mutex);
            if (m_faultReported)
            {
                return;
            }
            m_faultReported = true;
            m_stopping = true;
            for (uint32_t i = 0; i < 256; ++i)
            {
                if (m_peers[i].m_id != 0)
                {
                    peers[count++] = m_peers[i].m_id;
                    discard(m_peers[i]);
                    m_peers[i] = NetWorkHostPeer();
                }
            }
            m_peerCount = 0;
            m_announcements = 0;
        }
        try
        {
            if (!m_shutdownAccepted)
            {
                m_shutdownAccepted = m_functions.shutdown(m_instance) == 0;
            }
        }
        catch (...)
        {
            // 已进入不可恢复的宿主故障，仍逐个通知原有连接。
        }
        for (uint32_t i = 0; i < count; ++i)
        {
            try
            {
                m_closed(m_observer, peers[i], m_fault.load());
            }
            catch (...)
            {
                // 一个观察者异常不能吞掉其余连接的终态。
            }
        }
        return;
    }
    for (uint32_t step = 0; step < 256; ++step)
    {
        NetWorkHostEvent event;
        bool selected = false;
        bool closeRaw = false;
        {
            WriteLock lock(m_mutex);
            if (m_fault.load() != NI_OK)
            {
                return;
            }
            if (m_announcements != 0)
            {
                uint32_t first = 256;
                for (uint32_t index = 0; index < 256; ++index)
                {
                    const NetWorkHostPeer& peer = m_peers[index];
                    if (peer.m_id != 0 && peer.m_connected && !peer.m_announced &&
                        (first == 256 || peer.m_id < m_peers[first].m_id))
                    {
                        first = index;
                    }
                }
                if (first != 256)
                {
                    NetWorkHostPeer& peer = m_peers[first];
                    peer.m_announced = true;
                    --m_announcements;
                    event.m_kind = 0;
                    event.m_peer = peer.m_id;
                    selected = true;
                }
            }
            // 数据继续逐连接轮询；复用低位槽不能让新连接通告被游标重排。
            for (uint32_t scan = 0; !selected && scan < 256; ++scan)
            {
                const uint32_t index = (m_cursor + scan) % 256;
                NetWorkHostPeer& peer = m_peers[index];
                if (peer.m_id == 0)
                {
                    continue;
                }
                if (peer.m_failed || (peer.m_head == 2048 && peer.m_closed))
                {
                    event.m_kind = 3;
                    event.m_peer = peer.m_id;
                    event.m_reason = peer.m_reason;
                    closeRaw = peer.m_failed && !peer.m_closed;
                    peer = NetWorkHostPeer();
                    --m_peerCount;
                }
                else
                {
                    if (peer.m_head == 2048)
                    {
                        continue;
                    }
                    const uint32_t slot = peer.m_head;
                    NetWorkHostEvent& queued = m_events[slot];
                    if (peer.m_paused && queued.m_kind == 1)
                    {
                        continue;
                    }
                    peer.m_head = queued.m_next;
                    if (peer.m_head == 2048)
                    {
                        peer.m_tail = 2048;
                    }
                    event.m_kind = queued.m_kind;
                    event.m_peer = queued.m_peer;
                    event.m_reason = queued.m_reason;
                    event.m_size = queued.m_size;
                    event.m_bytes.swap(queued.m_bytes);
                    m_bytes -= queued.m_size;
                    queued.m_size = 0;
                    queued.m_next = m_free;
                    m_free = slot;
                    if (event.m_kind == 2)
                    {
                        peer.m_writable = false;
                    }
                }
                m_cursor = (index + 1) % 256;
                selected = true;
                break;
            }
        }
        if (!selected)
        {
            return;
        }
        if (closeRaw)
        {
            // 锁外请求原始关闭，允许宿主同步回调；此peer已退役，迟到事件直接忽略。
            try
            {
                if (m_functions.close(m_instance, event.m_peer, event.m_reason) != 0)
                {
                    m_fault.store(NI_INVALID_STATE);
                }
            }
            catch (...)
            {
                m_fault.store(NI_INVALID_STATE);
            }
        }
        if (event.m_kind == 0)
        {
            m_connected(m_observer, event.m_peer);
        }
        else if (event.m_kind == 1)
        {
            m_data(m_observer, event.m_peer, event.m_bytes.get(), event.m_size);
        }
        else if (event.m_kind == 2)
        {
            m_writable(m_observer, event.m_peer);
        }
        else
        {
            m_closed(m_observer, event.m_peer, event.m_reason);
        }
    }
}

int32_t NetWorkHostBridge::listen(void* instance, const char* address, uint32_t port, uint32_t maxPeers)
{
    try
    {
        NetWorkHostBridge* bridge = static_cast<NetWorkHostBridge*>(instance);
        if (bridge->m_fault.load() != NI_OK || bridge->m_stopping || bridge->m_functions.listen == nullptr || maxPeers == 0 || maxPeers > 256)
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
    if (peer == nullptr)
    {
        return -1;
    }
    *peer = 0;
    NetWorkHostBridge* bridge = static_cast<NetWorkHostBridge*>(instance);
    try
    {
        {
            WriteLock lock(bridge->m_mutex);
            if (bridge->m_stopping || bridge->m_fault.load() != NI_OK || bridge->m_peerCount == 256 || bridge->m_functions.connect == nullptr)
            {
                return -1;
            }
            bridge->m_connecting = true;
        }
        int32_t result = bridge->m_functions.connect(bridge->m_instance, address, port, peer);
        {
            WriteLock lock(bridge->m_mutex);
            bridge->m_connecting = false;
            if (result == 0 && (*peer == 0 || bridge->insert(*peer) == 256))
            {
                bridge->m_fault.store(NI_LIMIT_EXCEEDED);
                result = -1;
            }
        }
        if (result != 0)
        {
            *peer = 0;
        }
        return result;
    }
    catch (...)
    {
        WriteLock lock(bridge->m_mutex);
        bridge->m_connecting = false;
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
        WriteLock lock(bridge->m_mutex);
        if (bridge->m_fault.load() != NI_OK)
        {
            return -1;
        }
        return result == 0 && bridge->m_peerCount != 0 ? 1 : result;
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
        {
            WriteLock lock(bridge->m_mutex);
            const uint32_t index = bridge->find(peer);
            if (index != 256)
            {
                NetWorkHostPeer& state = bridge->m_peers[index];
                state.m_paused = paused != 0;
                if (state.m_closed || state.m_failed)
                {
                    return 0;
                }
            }
        }
        const int32_t result = bridge->m_functions.pauseRead(bridge->m_instance, peer, paused);
        if (result != 0)
        {
            WriteLock lock(bridge->m_mutex);
            const uint32_t index = bridge->find(peer);
            if (index != 256 && (bridge->m_peers[index].m_closed || bridge->m_peers[index].m_failed))
            {
                return 0;
            }
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
            WriteLock lock(bridge->m_mutex);
            bridge->m_stopping = true;
            for (uint32_t i = 0; i < 256; ++i)
            {
                NetWorkHostPeer& peer = bridge->m_peers[i];
                peer.m_paused = false;
                // 停止丢弃待交付正文，连接/可写标量仍先于已有终态交付。
                uint32_t previous = 2048;
                uint32_t slot = peer.m_head;
                while (slot != 2048)
                {
                    NetWorkHostEvent& event = bridge->m_events[slot];
                    const uint32_t next = event.m_next;
                    if (event.m_kind == 1)
                    {
                        if (previous == 2048)
                        {
                            peer.m_head = next;
                        }
                        else
                        {
                            bridge->m_events[previous].m_next = next;
                        }
                        if (peer.m_tail == slot)
                        {
                            peer.m_tail = previous;
                        }
                        bridge->m_bytes -= event.m_size;
                        event.clear();
                        event.m_next = bridge->m_free;
                        bridge->m_free = slot;
                    }
                    else
                    {
                        previous = slot;
                    }
                    slot = next;
                }
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

#if 0 // NET_WORK_HOST_UNIT_BEGIN
#include "NetWorkHostBridgeTest.h"
#include <CSystem/CSystemAPI.h>
#include <cstdio>
#include <cstring>
#if defined(_WIN32) && defined(_DEBUG) && !defined(NET_WORK_HOST_NO_HEAP)
#include <crtdbg.h>
static int32_t s_bridgeRejectAfter = -1;
static uint32_t s_bridgeAllocations = 0;
/** 隔离main中拒绝指定普通堆分配，统计无分配控制路径
@param [in] operation CRT分配操作，其他参数沿用CRT回调原型
@param [in] pointer CRT传入的堆块，只在钩子返回前借用；本用例未使用时忽略
@param [in] size CRT请求的字节数，是否参与注入判定由本用例决定
@param [in] block CRT堆块类别，只对普通业务堆块注入
@param [in] request CRT分配序号，本用例不使用
@param [in] filename CRT传入的来源文件名，本用例不读取
@param [in] line 来源代码行号，本用例不使用
@return 非零允许分配，0持续拒绝
*/
static int __cdecl BridgeMemoryHook(int operation, void* pointer, size_t size, int block, long request, const unsigned char* filename, int line)
{
    (void)pointer;
    (void)size;
    (void)request;
    (void)filename;
    (void)line;
    if (block == _NORMAL_BLOCK && (operation == _HOOK_ALLOC || operation == _HOOK_REALLOC))
    {
        ++s_bridgeAllocations;
        if (s_bridgeRejectAfter == 0) { return 0; }
        if (s_bridgeRejectAfter > 0) { --s_bridgeRejectAfter; }
    }
    return 1;
}

#endif

NetWorkHostBridgeTest::NetWorkHostBridgeTest() :
m_bridge(nullptr), m_target(nullptr), m_connected(nullptr), m_data(nullptr), m_writable(nullptr), m_closed(nullptr),
m_owner(CSystem::SystemThreadId()), m_connections(0), m_lastConnected(0), m_checkConnectionOrder(false), m_inputs(0), m_writes(0), m_closes(0), m_stops(0), m_allocated(0), m_peerInputs(), m_peerCloses(), m_reason(0), m_pause(true), m_checkPayload(true), m_sequence(false), m_expected(), m_producersDone(0), m_stopped(false), m_throwPoll(false), m_bad(false), m_done(false)
{

}

NetHostFunctions NetWorkHostBridgeTest::functions()
{
    NetHostFunctions value = {};
    value.structSize = sizeof(value); value.version = 1; value.context = this;
    value.create = create; value.connect = connect; value.poll = poll; value.write = write; value.pauseRead = pause;
    value.close = close; value.shutdown = shutdown; value.destroy = operation; value.localPort = query; value.peerCount = query;
    return value;
}

int32_t NetWorkHostBridgeTest::create(void* provider, void* observer, NetHostConnected connected, NetHostData data, NetHostWritable writable, NetHostClosed closed, void** instance)
{
    NetWorkHostBridgeTest* test = static_cast<NetWorkHostBridgeTest*>(provider);
    test->m_target = observer; test->m_connected = connected; test->m_data = data; test->m_writable = writable; test->m_closed = closed;
    *instance = test; return 0;
}

int32_t NetWorkHostBridgeTest::operation(void* instance)
{
    (void)instance; return 0;
}

int32_t NetWorkHostBridgeTest::poll(void* instance)
{
    NetWorkHostBridgeTest* test = static_cast<NetWorkHostBridgeTest*>(instance);
    if (test->m_throwPoll) { throw std::bad_alloc(); }
    return test->m_stopped ? 0 : 1;
}

int32_t NetWorkHostBridgeTest::shutdown(void* instance)
{
    NetWorkHostBridgeTest* test = static_cast<NetWorkHostBridgeTest*>(instance); test->m_stopped = true; ++test->m_stops; return 0;
}

int32_t NetWorkHostBridgeTest::close(void* instance, uint64_t peer, int32_t reason)
{
    NetWorkHostBridgeTest* test = static_cast<NetWorkHostBridgeTest*>(instance); test->m_closed(test->m_target, peer, reason); return 0;
}

int32_t NetWorkHostBridgeTest::pause(void* instance, uint64_t peer, int32_t paused)
{
    (void)instance; (void)peer; (void)paused; return 0;
}

int32_t NetWorkHostBridgeTest::connect(void* instance, const char* address, uint32_t port, uint64_t* peer)
{
    (void)address; (void)port; *peer = ++static_cast<NetWorkHostBridgeTest*>(instance)->m_allocated; return 0;
}

int32_t NetWorkHostBridgeTest::write(void* instance, uint64_t peer, const uint8_t* bytes, uint32_t size)
{
    (void)instance; (void)peer; (void)bytes; (void)size; return 0;
}

uint32_t NetWorkHostBridgeTest::query(void* instance)
{
    (void)instance; return 0;
}

void NetWorkHostBridgeTest::connected(void* context, uint64_t peer)
{
    NetWorkHostBridgeTest* test = static_cast<NetWorkHostBridgeTest*>(context);
    ++test->m_connections;
    if (test->m_checkConnectionOrder && peer <= test->m_lastConnected)
    {
        test->m_bad = true;
    }
    test->m_lastConnected = peer;
    test->m_bad = test->m_bad || CSystem::SystemThreadId() != test->m_owner || NetWorkHostBridge::destroy(test->m_bridge) != -1;
}

void NetWorkHostBridgeTest::data(void* context, uint64_t peer, const uint8_t* bytes, uint32_t size)
{
    NetWorkHostBridgeTest* test = static_cast<NetWorkHostBridgeTest*>(context); ++test->m_inputs;
    test->m_bad = test->m_bad || CSystem::SystemThreadId() != test->m_owner ||
        (test->m_checkPayload && (size != 1 || bytes == nullptr || bytes[0] != 7));
    if (peer >= 1 && peer <= 2) { ++test->m_peerInputs[peer - 1]; }
    if (test->m_sequence)
    {
        uint32_t record[2] = {};
        if (size != sizeof(record) || bytes == nullptr) { test->m_bad = true; }
        else
        {
            std::memcpy(record, bytes, sizeof(record));
            if (record[0] >= 4 || record[1] != test->m_expected[record[0]]++) { test->m_bad = true; }
        }
    }
    if (test->m_pause && peer == 1) { NetWorkHostBridge::pauseRead(test->m_bridge, peer, 1); }
}

void NetWorkHostBridgeTest::writable(void* context, uint64_t peer)
{
    (void)peer; ++static_cast<NetWorkHostBridgeTest*>(context)->m_writes;
}

void NetWorkHostBridgeTest::closed(void* context, uint64_t peer, int32_t reason)
{
    NetWorkHostBridgeTest* test = static_cast<NetWorkHostBridgeTest*>(context); ++test->m_closes;
    test->m_reason = reason;
    if (peer >= 1 && peer <= 2) { ++test->m_peerCloses[peer - 1]; }
    test->m_bad = test->m_bad || CSystem::SystemThreadId() != test->m_owner ||
        (test->m_checkConnectionOrder && peer > test->m_lastConnected);
}

NetWorkHostBridgeTestTask::NetWorkHostBridgeTestTask(NetWorkHostBridgeTest* test) : m_test(test)
{

}

void NetWorkHostBridgeTestTask::DoTask()
{
    m_test->m_connected(m_test->m_target, 1); m_test->m_connected(m_test->m_target, 2);
    uint8_t bytes = 7;
    m_test->m_data(m_test->m_target, 1, &bytes, 1); m_test->m_data(m_test->m_target, 1, &bytes, 1); m_test->m_data(m_test->m_target, 2, &bytes, 1);
    for (uint32_t i = 0; i < 100; ++i) { m_test->m_writable(m_test->m_target, 2); }
    m_test->m_closed(m_test->m_target, 1, 0); m_test->m_closed(m_test->m_target, 2, 0);
    bytes = 99; m_test->m_done = true;
}

NetWorkHostBridgeSequenceTask::NetWorkHostBridgeSequenceTask(NetWorkHostBridgeTest* test, uint32_t producer) :
m_test(test), m_producer(producer)
{

}

void NetWorkHostBridgeSequenceTask::DoTask()
{
    for (uint32_t i = 0; i < 400; ++i)
    {
        const uint32_t record[2] = {m_producer, i};
        m_test->m_data(m_test->m_target, 1, reinterpret_cast<const uint8_t*>(record), sizeof(record));
    }
    m_test->m_producersDone.fetch_add(1);
}

int32_t NetWorkHostBridgeTest::audit()
{
    uint32_t checks = 0;
#define HOST_CHECK(condition) { ++checks; if (!(condition)) { std::printf("FAIL line=%u %s\n", static_cast<uint32_t>(__LINE__), #condition); return 1; } }
    // 第一条连接退役后，轮询游标位于槽1；第二、三条连接复用槽0、1。
    // 新连接通告仍须2在3之前，不能把轮询公平性变成代次倒序。
    NetWorkHostBridgeTest connectionOrder;
    connectionOrder.m_checkConnectionOrder = true;
    connectionOrder.m_pause = false;
    NetHostFunctions host = connectionOrder.functions();
    HOST_CHECK(NetWorkHostBridge::create(&host, &connectionOrder, connectionOrder.connected, connectionOrder.data,
        connectionOrder.writable, connectionOrder.closed, &connectionOrder.m_bridge) == 0);
    connectionOrder.m_connected(connectionOrder.m_target, 1);
    HOST_CHECK(NetWorkHostBridge::poll(connectionOrder.m_bridge) == 1 && connectionOrder.m_connections == 1);
    connectionOrder.m_closed(connectionOrder.m_target, 1, NI_OK);
    HOST_CHECK(NetWorkHostBridge::poll(connectionOrder.m_bridge) == 1 && connectionOrder.m_closes == 1);
    connectionOrder.m_connected(connectionOrder.m_target, 2);
    connectionOrder.m_connected(connectionOrder.m_target, 3);
    HOST_CHECK(NetWorkHostBridge::poll(connectionOrder.m_bridge) == 1 && connectionOrder.m_connections == 3 &&
        connectionOrder.m_lastConnected == 3 && !connectionOrder.m_bad);
    connectionOrder.m_closed(connectionOrder.m_target, 2, NI_OK);
    connectionOrder.m_closed(connectionOrder.m_target, 3, NI_OK);
    HOST_CHECK(NetWorkHostBridge::poll(connectionOrder.m_bridge) == 1 && connectionOrder.m_closes == 3);
    HOST_CHECK(NetWorkHostBridge::shutdown(connectionOrder.m_bridge) == 0 &&
        NetWorkHostBridge::destroy(connectionOrder.m_bridge) == 0);
    NetWorkHostBridgeTest test;
    host = test.functions();
    HOST_CHECK(NetWorkHostBridge::create(&host, &test, test.connected, test.data, test.writable, test.closed, &test.m_bridge) == 0);
    const uint32_t thread = CTaskThreadManager::Instance().Init();
    CTaskThreadManager::Instance().GetThreadInterface(thread)->PostTask(std::shared_ptr<CTask>(new NetWorkHostBridgeTestTask(&test)));
    const uint64_t deadline = GetTickCount64() + 5000;
    while (!test.m_done.load() && GetTickCount64() < deadline) { CSystem::sleepPrecise(1); }
    HOST_CHECK(test.m_done.load());
    CTaskThreadManager::Instance().Uninit(thread);
    HOST_CHECK(test.m_connections == 0 && test.m_inputs == 0 && test.m_closes == 0);
    HOST_CHECK(NetWorkHostBridge::poll(test.m_bridge) == 1);
    HOST_CHECK(test.m_connections == 2 && test.m_inputs == 2 && test.m_writes == 1 && test.m_closes == 1 && !test.m_bad);
    test.m_pause = false;
    HOST_CHECK(NetWorkHostBridge::pauseRead(test.m_bridge, 1, 0) == 0);
    HOST_CHECK(NetWorkHostBridge::poll(test.m_bridge) == 1 && test.m_inputs == 3 && test.m_closes == 2 && !test.m_bad);
    HOST_CHECK(NetWorkHostBridge::shutdown(test.m_bridge) == 0 && test.m_stops == 1);
    HOST_CHECK(NetWorkHostBridge::destroy(test.m_bridge) == 0);
    // 满4MiB数据之后终态仍有独立事件位置，不会被丢掉。
    NetWorkHostBridgeTest full; full.m_pause = false; full.m_checkPayload = false; host = full.functions();
    HOST_CHECK(NetWorkHostBridge::create(&host, &full, full.connected, full.data, full.writable, full.closed, &full.m_bridge) == 0);
    full.m_connected(full.m_target, 1);
    uint8_t bytes[16384] = {};
    for (uint32_t i = 0; i < 256; ++i) { full.m_data(full.m_target, 1, bytes, sizeof(bytes)); }
    full.m_closed(full.m_target, 1, 0);
    // 已接纳EOF之后的迟到数据忽略，不影响已排队尾部与其他连接。
    full.m_data(full.m_target, 1, bytes, 1);
    for (uint32_t i = 0; i < 3; ++i) { NetWorkHostBridge::poll(full.m_bridge); }
    HOST_CHECK(full.m_inputs == 256 && full.m_closes == 1 && full.m_stops == 0 && !full.m_bad);
    HOST_CHECK(NetWorkHostBridge::shutdown(full.m_bridge) == 0);
    HOST_CHECK(NetWorkHostBridge::destroy(full.m_bridge) == 0);
    // shutdown锁外转交事件时也处于受保护回调段，不能在其中释放bridge。
    NetWorkHostBridgeTest stopping; host = stopping.functions();
    HOST_CHECK(NetWorkHostBridge::create(&host, &stopping, stopping.connected, stopping.data, stopping.writable, stopping.closed, &stopping.m_bridge) == 0);
    stopping.m_connected(stopping.m_target, 1);
    stopping.m_closed(stopping.m_target, 1, 0);
    HOST_CHECK(NetWorkHostBridge::shutdown(stopping.m_bridge) == 0 && stopping.m_connections == 1 && stopping.m_closes == 1 && !stopping.m_bad);
    HOST_CHECK(NetWorkHostBridge::destroy(stopping.m_bridge) == 0);
    // 宿主C++错误不能永久遗留polling，外层捕获后仍可收敛和销毁。
    NetWorkHostBridgeTest throwing; host = throwing.functions();
    HOST_CHECK(NetWorkHostBridge::create(&host, &throwing, throwing.connected, throwing.data, throwing.writable, throwing.closed, &throwing.m_bridge) == 0);
    throwing.m_throwPoll = true;
    HOST_CHECK(NetWorkHostBridge::poll(throwing.m_bridge) == -1);
    HOST_CHECK(NetWorkHostBridge::shutdown(throwing.m_bridge) == 0);
    HOST_CHECK(NetWorkHostBridge::destroy(throwing.m_bridge) == 0);
    // 所有2048个槽都被暂停连接占用，两个连接的EOF仍可接纳。
    NetWorkHostBridgeTest slots; slots.m_pause = false; host = slots.functions();
    HOST_CHECK(NetWorkHostBridge::create(&host, &slots, slots.connected, slots.data, slots.writable, slots.closed, &slots.m_bridge) == 0);
    slots.m_connected(slots.m_target, 1); slots.m_connected(slots.m_target, 2);
    NetWorkHostBridge::poll(slots.m_bridge);
    NetWorkHostBridge::pauseRead(slots.m_bridge, 1, 1);
    uint8_t seven = 7;
    for (uint32_t i = 0; i < 2048; ++i) { slots.m_data(slots.m_target, 1, &seven, 1); }
    slots.m_closed(slots.m_target, 1, NI_OK); slots.m_closed(slots.m_target, 2, NI_OK);
    HOST_CHECK(NetWorkHostBridge::poll(slots.m_bridge) >= 0 && slots.m_peerCloses[1] == 1 && slots.m_peerCloses[0] == 0 && slots.m_stops == 0);
    HOST_CHECK(NetWorkHostBridge::pauseRead(slots.m_bridge, 1, 0) == 0);
    for (uint32_t i = 0; i < 9; ++i) { NetWorkHostBridge::poll(slots.m_bridge); }
    HOST_CHECK(slots.m_peerInputs[0] == 2048 && slots.m_peerCloses[0] == 1 && !slots.m_bad);
    for (uint32_t i = 0; i < 4096; ++i)
    {
        slots.m_data(slots.m_target, 1, &seven, 1);
        slots.m_writable(slots.m_target, 1);
        slots.m_closed(slots.m_target, 1, NI_OK);
    }
    HOST_CHECK(NetWorkHostBridge::poll(slots.m_bridge) >= 0 && slots.m_closes == 2 && slots.m_stops == 0);
    NetWorkHostBridge::shutdown(slots.m_bridge);
    HOST_CHECK(NetWorkHostBridge::destroy(slots.m_bridge) == 0);
    // 两种容量上限分别覆盖通告前后过载；关闭不能先于新连接通告。
    for (uint32_t mode = 0; mode < 4; ++mode)
    {
        NetWorkHostBridgeTest overflow; overflow.m_pause = false; overflow.m_checkConnectionOrder = true; host = overflow.functions();
        HOST_CHECK(NetWorkHostBridge::create(&host, &overflow, overflow.connected, overflow.data, overflow.writable, overflow.closed, &overflow.m_bridge) == 0);
        overflow.m_connected(overflow.m_target, 1); overflow.m_connected(overflow.m_target, 2);
        if (mode < 2) { NetWorkHostBridge::poll(overflow.m_bridge); }
        for (uint32_t i = 0; i < (mode % 2 == 0 ? 2048U : 256U); ++i)
        {
            overflow.m_data(overflow.m_target, 1, mode % 2 == 0 ? &seven : bytes, mode % 2 == 0 ? 1 : sizeof(bytes));
        }
        overflow.m_data(overflow.m_target, 1, &seven, 1);
        overflow.m_data(overflow.m_target, 2, &seven, 1);
        HOST_CHECK(NetWorkHostBridge::poll(overflow.m_bridge) >= 0 && overflow.m_peerCloses[0] == 1 && overflow.m_peerCloses[1] == 0 && overflow.m_peerInputs[1] == 1 && overflow.m_reason == NI_LIMIT_EXCEEDED && overflow.m_connections == 2 && overflow.m_stops == 0 && !overflow.m_bad);
        overflow.m_data(overflow.m_target, 2, &seven, 1);
        NetWorkHostBridge::poll(overflow.m_bridge);
        HOST_CHECK(overflow.m_peerInputs[1] == 2 && overflow.m_closes == 1 && !overflow.m_bad);
        NetWorkHostBridge::shutdown(overflow.m_bridge);
        HOST_CHECK(NetWorkHostBridge::destroy(overflow.m_bridge) == 0);
    }
    // 主动connect与事件入口共享256个连接槽，满时不调用原始宿主。
    NetWorkHostBridgeTest limit; limit.m_pause = false; host = limit.functions();
    HOST_CHECK(NetWorkHostBridge::create(&host, &limit, limit.connected, limit.data, limit.writable, limit.closed, &limit.m_bridge) == 0);
    uint64_t peer = 0;
    for (uint32_t i = 0; i < 256; ++i)
    {
        HOST_CHECK(NetWorkHostBridge::connect(limit.m_bridge, "127.0.0.1", 1, &peer) == 0 && peer == i + 1);
    }
    HOST_CHECK(NetWorkHostBridge::connect(limit.m_bridge, "127.0.0.1", 1, &peer) < 0 && peer == 0 && limit.m_allocated == 256);
    for (uint32_t i = 0; i < 256; ++i) { limit.m_closed(limit.m_target, i + 1, NI_OK); }
    NetWorkHostBridge::poll(limit.m_bridge);
    HOST_CHECK(limit.m_closes == 256 && limit.m_stops == 0);
    HOST_CHECK(NetWorkHostBridge::connect(limit.m_bridge, "127.0.0.1", 1, &peer) == 0 && peer == 257);
    NetWorkHostBridge::shutdown(limit.m_bridge);
    HOST_CHECK(NetWorkHostBridge::destroy(limit.m_bridge) == 0);
    NetWorkHostBridgeTest ordered; ordered.m_pause = false; ordered.m_checkPayload = false; ordered.m_sequence = true;
    host = ordered.functions();
    HOST_CHECK(NetWorkHostBridge::create(&host, &ordered, ordered.connected, ordered.data, ordered.writable, ordered.closed, &ordered.m_bridge) == 0);
    ordered.m_connected(ordered.m_target, 1);
    uint32_t producers[4] = {};
    for (uint32_t i = 0; i < 4; ++i)
    {
        producers[i] = CTaskThreadManager::Instance().Init();
        CTaskThreadManager::Instance().GetThreadInterface(producers[i])->PostTask(std::shared_ptr<CTask>(new NetWorkHostBridgeSequenceTask(&ordered, i)));
    }
    const uint64_t orderedDeadline = GetTickCount64() + 5000;
    while (ordered.m_producersDone.load() != 4 && GetTickCount64() < orderedDeadline) { CSystem::sleepPrecise(1); }
    for (uint32_t i = 0; i < 4; ++i) { CTaskThreadManager::Instance().Uninit(producers[i]); }
    HOST_CHECK(ordered.m_producersDone.load() == 4 && ordered.m_inputs == 0);
    ordered.m_closed(ordered.m_target, 1, NI_OK);
    for (uint32_t i = 0; i < 8; ++i) { NetWorkHostBridge::poll(ordered.m_bridge); }
    HOST_CHECK(ordered.m_inputs == 1600 && ordered.m_closes == 1 && ordered.m_stops == 0 && !ordered.m_bad);
    for (uint32_t i = 0; i < 4; ++i) { HOST_CHECK(ordered.m_expected[i] == 400); }
    NetWorkHostBridge::shutdown(ordered.m_bridge);
    HOST_CHECK(NetWorkHostBridge::destroy(ordered.m_bridge) == 0);
#if defined(_WIN32) && defined(_DEBUG) && !defined(NET_WORK_HOST_NO_HEAP)
    _CrtMemState before, after;
    _CrtMemCheckpoint(&before);
    for (int32_t position = 0; position < 4; ++position)
    {
        NetWorkHostBridgeTest allocation; allocation.m_pause = false; host = allocation.functions();
        s_bridgeRejectAfter = position;
        _CRT_ALLOC_HOOK oldHook = _CrtSetAllocHook(BridgeMemoryHook);
        const int32_t result = NetWorkHostBridge::create(&host, &allocation, allocation.connected, allocation.data, allocation.writable, allocation.closed, &allocation.m_bridge);
        _CrtSetAllocHook(oldHook); s_bridgeRejectAfter = -1;
        HOST_CHECK(position == 0 ? result < 0 && allocation.m_bridge == nullptr : result == 0 && allocation.m_bridge != nullptr);
        if (allocation.m_bridge != nullptr)
        {
            NetWorkHostBridge::shutdown(allocation.m_bridge);
            HOST_CHECK(NetWorkHostBridge::destroy(allocation.m_bridge) == 0);
        }
    }
    for (int32_t position = 0; position < 3; ++position)
    {
        NetWorkHostBridgeTest allocation; allocation.m_pause = false; host = allocation.functions();
        HOST_CHECK(NetWorkHostBridge::create(&host, &allocation, allocation.connected, allocation.data, allocation.writable, allocation.closed, &allocation.m_bridge) == 0);
        allocation.m_checkConnectionOrder = true;
        allocation.m_connected(allocation.m_target, 1); allocation.m_connected(allocation.m_target, 2);
        // 未向owner通告时注入输入分配失败，仍须先连接后关闭。
        s_bridgeRejectAfter = position;
        _CRT_ALLOC_HOOK oldHook = _CrtSetAllocHook(BridgeMemoryHook);
        allocation.m_data(allocation.m_target, 1, &seven, 1);
        _CrtSetAllocHook(oldHook); s_bridgeRejectAfter = -1;
        allocation.m_data(allocation.m_target, 2, &seven, 1);
        NetWorkHostBridge::poll(allocation.m_bridge);
        HOST_CHECK(allocation.m_connections == 2 && allocation.m_peerInputs[1] == 1 && allocation.m_peerCloses[1] == 0 && allocation.m_stops == 0 && !allocation.m_bad);
        HOST_CHECK(position == 0 ? allocation.m_reason == NI_NO_MEMORY && allocation.m_peerCloses[0] == 1 && allocation.m_peerInputs[0] == 0 : allocation.m_peerCloses[0] == 0 && allocation.m_peerInputs[0] == 1);
        s_bridgeRejectAfter = 0; s_bridgeAllocations = 0;
        oldHook = _CrtSetAllocHook(BridgeMemoryHook);
        for (uint32_t i = 0; i < 10000; ++i) { allocation.m_writable(allocation.m_target, 2); }
        allocation.m_closed(allocation.m_target, 2, NI_OK);
        NetWorkHostBridge::poll(allocation.m_bridge);
        _CrtSetAllocHook(oldHook); s_bridgeRejectAfter = -1;
        HOST_CHECK(s_bridgeAllocations == 0 && allocation.m_peerCloses[1] == 1 && allocation.m_writes == 1 && !allocation.m_bad);
        NetWorkHostBridge::shutdown(allocation.m_bridge);
        HOST_CHECK(NetWorkHostBridge::destroy(allocation.m_bridge) == 0);
    }
    _CrtMemCheckpoint(&after);
    HOST_CHECK(before.lCounts[_NORMAL_BLOCK] == after.lCounts[_NORMAL_BLOCK] && before.lSizes[_NORMAL_BLOCK] == after.lSizes[_NORMAL_BLOCK]);
    std::printf("HOST_BRIDGE allocation_faults=7 control_allocations=0 heap_delta=0\n");
#endif
    std::printf("PASS NetWork native foreign-event/pause/fault checks=%u\n", checks);
    return 0;
}

#ifndef NET_WORK_HOST_NO_MAIN
int main()
{
    const int32_t result = NetWorkHostBridgeTest::audit();
    if (result == 0) { std::printf("PASS\n"); }
    return result;
}

#endif
#endif // NET_WORK_HOST_UNIT_END