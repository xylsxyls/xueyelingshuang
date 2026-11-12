#include "LibuvChannelState.h"
#include <cstring>
#include <new>

LibuvChannelState::LibuvChannelState() :
m_loopInitialized(false),
m_readBufferBytes(16384),
m_loopStatus(0),
m_callback(nullptr),
m_freeWrites(nullptr),
m_cachedWrites(0),
m_cachedWriteBytes(0),
m_nextPeer(1),
m_maxPeers(64),
m_port(0),
m_started(false),
m_listenerInitialized(false),
m_stopping(false),
m_polling(false),
m_rejectPending(false),
m_acceptPending(false)
{

}

LibuvChannelState::~LibuvChannelState()
{
    if (!m_loopInitialized)
    {
        return;
    }
    shutdown();
    // No resolver, timer or foreign handles are admitted into this private loop.
    // All TCP handles are closing; libuv delivers write cancellation before close.
    // onClosed允许请求shutdown；析构排空也必须沿用poll重入保护。
    m_polling = true;
    uv_run(&m_loop, UV_RUN_DEFAULT);
    m_polling = false;
    uv_loop_close(&m_loop);
}

int32_t LibuvChannelState::initialize()
{
    if (!m_loopInitialized)
    {
        m_loopStatus = uv_loop_init(&m_loop);
        m_loopInitialized = m_loopStatus == 0;
    }
    return m_loopStatus;
}

int32_t LibuvChannelState::parseAddress(const char* address, uint32_t port, sockaddr_storage& result)
{
    if (address == nullptr || port > 65535)
    {
        return UV_EINVAL;
    }
    std::memset(&result, 0, sizeof(result));
    int resultCode = uv_ip4_addr(address, static_cast<int>(port), reinterpret_cast<sockaddr_in*>(&result));
    if (resultCode != 0)
    {
        resultCode = uv_ip6_addr(address, static_cast<int>(port), reinterpret_cast<sockaddr_in6*>(&result));
    }
    return resultCode;
}

LibuvChannelPeer* LibuvChannelState::createPeer(bool rejectOnly)
{
    if (!m_loopInitialized || m_loopStatus != 0 || m_nextPeer == 0 || (!rejectOnly && m_peers.size() >= m_maxPeers))
    {
        return nullptr;
    }
    LibuvChannelPeer* peer = new (std::nothrow) LibuvChannelPeer(this, rejectOnly ? 0 : m_nextPeer++);
    if (peer == nullptr || peer->m_input == nullptr)
    {
        delete peer;
        return nullptr;
    }
    if (uv_tcp_init(&m_loop, &peer->m_tcp) != 0)
    {
        delete peer;
        return nullptr;
    }
    peer->m_tcp.data = peer;
    peer->m_connect.data = peer;
    if (rejectOnly)
    {
        m_rejectPending = true;
    }
    if (!rejectOnly)
    {
        try
        {
            m_peers.insert(std::make_pair(peer->m_id, peer));
        }
        catch (...)
        {
            peer->m_id = 0;
            peer->m_reportClose = false;
            m_rejectPending = true;
            closePeer(peer, UV_ENOMEM);
            return nullptr;
        }
    }
    return peer;
}

LibuvChannelWrite* LibuvChannelState::prepareWrite(LibuvChannelPeer* peer, const uint8_t* data, uint32_t size)
{
    LibuvChannelWrite* request = m_freeWrites;
    if (request != nullptr)
    {
        m_freeWrites = request->m_next;
        --m_cachedWrites;
        m_cachedWriteBytes -= request->m_bytes.capacity();
        request->m_next = nullptr;
    }
    try
    {
        if (request == nullptr)
        {
            request = new LibuvChannelWrite();
        }
        // capacity足够时复用原缓冲；始终复制本次有效字节，不借用调用方内存。
        request->m_bytes.assign(data, data + size);
        request->m_peer = peer;
        request->m_request.data = request;
        return request;
    }
    catch (...)
    {
        releaseWrite(request);
        return nullptr;
    }
}

void LibuvChannelState::releaseWrite(LibuvChannelWrite* request)
{
    if (request == nullptr)
    {
        return;
    }
    const size_t capacity = request->m_bytes.capacity();
    if (m_stopping || m_cachedWrites >= m_writeConfig.m_maximumCachedWrites ||
        capacity > m_writeConfig.m_maximumCachedBytes - m_cachedWriteBytes)
    {
        delete request;
        return;
    }
    request->m_peer = nullptr;
    request->m_bytes.clear();
    request->m_next = m_freeWrites;
    m_freeWrites = request;
    ++m_cachedWrites;
    m_cachedWriteBytes += capacity;
}

int32_t LibuvChannelState::startRead(LibuvChannelPeer* peer)
{
    return uv_read_start(reinterpret_cast<uv_stream_t*>(&peer->m_tcp), &allocateRead, &read);
}

void LibuvChannelState::closePeer(LibuvChannelPeer* peer, int32_t reason)
{
    if (peer->m_closing)
    {
        return;
    }
    peer->m_closing = true;
    peer->m_reason = reason;
    uv_read_stop(reinterpret_cast<uv_stream_t*>(&peer->m_tcp));
    uv_close(reinterpret_cast<uv_handle_t*>(&peer->m_tcp), &closed);
}

void LibuvChannelState::shutdown()
{
    if (m_stopping)
    {
        return;
    }
    m_stopping = true;
    while (m_freeWrites != nullptr)
    {
        LibuvChannelWrite* request = m_freeWrites;
        m_freeWrites = request->m_next;
        delete request;
    }
    m_cachedWrites = 0;
    m_cachedWriteBytes = 0;
    if (m_listenerInitialized && !uv_is_closing(reinterpret_cast<uv_handle_t*>(&m_listener)))
    {
        uv_close(reinterpret_cast<uv_handle_t*>(&m_listener), nullptr);
    }
    for (auto peer = m_peers.begin(); peer != m_peers.end(); ++peer)
    {
        closePeer(peer->second, UV_ECANCELED);
    }
}

void LibuvChannelState::acceptPeer(uv_stream_t* server, int status)
{
    LibuvChannelState* owner = static_cast<LibuvChannelState*>(server->data);
    if (status < 0 || owner->m_stopping)
    {
        return;
    }
    const bool reject = owner->m_peers.size() >= owner->m_maxPeers;
    if (owner->m_rejectPending)
    {
        owner->m_acceptPending = true;
        return;
    }
    owner->m_acceptPending = false;
    LibuvChannelPeer* peer = owner->createPeer(reject);
    if (peer == nullptr)
    {
        // 对象或登记节点分配失败时，uv_accept尚未消费该次就绪，保留重试责任。
        owner->m_acceptPending = true;
        return;
    }
    // 入站连接尚未交给上层，接纳或启动读取失败只回收资源，不能上报未知连接的终态。
    peer->m_reportClose = false;
    const int result = uv_accept(server, reinterpret_cast<uv_stream_t*>(&peer->m_tcp));
    if (result != 0 || reject)
    {
        owner->closePeer(peer, result != 0 ? result : UV_ENOBUFS);
        return;
    }
    peer->m_connected = true;
    uv_tcp_nodelay(&peer->m_tcp, 1);
    const int readResult = owner->startRead(peer);
    if (readResult != 0)
    {
        owner->closePeer(peer, readResult);
        return;
    }
    // 从发布连接事件起才承担配对终态责任；回调抛异常也必须随后关闭。
    peer->m_reportClose = true;
    try
    {
        owner->m_callback->onConnected(peer->m_id);
    }
    catch (...)
    {
        owner->closePeer(peer, UV_EPROTO);
    }
}

void LibuvChannelState::connected(uv_connect_t* request, int status)
{
    LibuvChannelPeer* peer = static_cast<LibuvChannelPeer*>(request->data);
    LibuvChannelState* owner = peer->m_owner;
    if (peer->m_closing)
    {
        return;
    }
    if (status != 0 || owner->m_stopping)
    {
        owner->closePeer(peer, status != 0 ? status : UV_ECANCELED);
        return;
    }
    peer->m_connected = true;
    uv_tcp_nodelay(&peer->m_tcp, 1);
    const int result = owner->startRead(peer);
    if (result != 0)
    {
        owner->closePeer(peer, result);
        return;
    }
    try
    {
        owner->m_callback->onConnected(peer->m_id);
    }
    catch (...)
    {
        owner->closePeer(peer, UV_EPROTO);
    }
}

void LibuvChannelState::allocateRead(uv_handle_t* handle, size_t suggestedSize, uv_buf_t* buffer)
{
    (void)suggestedSize;
    LibuvChannelPeer* peer = static_cast<LibuvChannelPeer*>(handle->data);
    *buffer = uv_buf_init(reinterpret_cast<char*>(peer->m_input), peer->m_owner->m_readBufferBytes);
}

void LibuvChannelState::read(uv_stream_t* stream, ssize_t size, const uv_buf_t* buffer)
{
    LibuvChannelPeer* peer = static_cast<LibuvChannelPeer*>(stream->data);
    LibuvChannelState* owner = peer->m_owner;
    if (peer->m_closing || size == 0)
    {
        return;
    }
    if (size < 0)
    {
        owner->closePeer(peer, static_cast<int32_t>(size));
        return;
    }
    try
    {
        owner->m_callback->onData(peer->m_id, reinterpret_cast<const uint8_t*>(buffer->base),
            static_cast<uint32_t>(size));
    }
    catch (...)
    {
        owner->closePeer(peer, UV_EPROTO);
    }
}

void LibuvChannelState::written(uv_write_t* request, int status)
{
    LibuvChannelWrite* write = static_cast<LibuvChannelWrite*>(request->data);
    LibuvChannelPeer* peer = write->m_peer;
    LibuvChannelState* owner = peer->m_owner;
    peer->m_queuedBytes -= static_cast<uint32_t>(write->m_bytes.size());
    --peer->m_queuedWrites;
    owner->releaseWrite(write);
    if (peer->m_closing)
    {
        return;
    }
    if (status != 0)
    {
        owner->closePeer(peer, status);
        return;
    }
    try
    {
        owner->m_callback->onWritable(peer->m_id);
    }
    catch (...)
    {
        owner->closePeer(peer, UV_EPROTO);
    }
}

void LibuvChannelState::closed(uv_handle_t* handle)
{
    LibuvChannelPeer* peer = static_cast<LibuvChannelPeer*>(handle->data);
    LibuvChannelState* owner = peer->m_owner;
    const uint64_t id = peer->m_id;
    const int32_t reason = peer->m_reason;
    const bool report = peer->m_reportClose;
    owner->m_peers.erase(id);
    delete peer;
    if (id == 0)
    {
        owner->m_rejectPending = false;
        // 不在关闭回调里再次接纳。持续分配失败时，立即重试会再次生成关闭句柄，
        // 使libuv同一轮关闭回调始终排不空；保留m_acceptPending交下一次poll重试。
    }
    if (report)
    {
        try
        {
            owner->m_callback->onClosed(id, reason);
        }
        catch (...)
        {
            // Closing already completed; no exception may cross a libuv callback.
        }
    }
}