#include "NetSenderMacro.h"
#include "NetSenderServiceQueue.h"
#include <cstring>

NetSenderServiceManagedFrame::NetSenderServiceManagedFrame() :
m_frame(),
m_origin(),
m_capacity(0),
m_endpoint(0),
m_created(0),
m_blocked(0),
m_channel(0),
m_next(256),
m_previous(256),
m_lane(6)
{

}

NetSenderServiceManagedFrame::~NetSenderServiceManagedFrame()
{
    if (m_bytes)
    {
        NetInterface::wipe(m_bytes.get(), m_frame.m_payloadSize);
    }
}

int32_t NetSenderServiceManagedFrame::view(uint64_t now, NetInterfaceFrame& frame, std::vector<uint8_t>& scratch) const
{
    frame = m_frame;
    frame.m_payload = m_bytes.get();
    if (frame.m_kind != NI_REQUEST)
    {
        return NI_READY;
    }
    NetInterfaceRpcView rpc = {};
    if (NetInterfaceRpc::parse(frame, rpc) != NI_OK || now < m_created)
    {
        return NI_PROTOCOL_ERROR;
    }
    if (now - m_created >= rpc.m_value)
    {
        return NI_TIMEOUT;
    }
    try
    {
        scratch.resize(m_frame.m_payloadSize);
        uint32_t required = 0;
        const int32_t encoded = NetInterfaceRpc::encode(NI_REQUEST, rpc.m_value - static_cast<uint32_t>(now - m_created),
            rpc.m_parent, rpc.m_data, rpc.m_size, scratch.data(), static_cast<uint32_t>(scratch.size()), required);
        if (encoded != NI_OK)
        {
            return encoded;
        }
        frame.m_payload = scratch.data();
        frame.m_payloadSize = required;
        return NI_READY;
    }
    catch (...)
    {
        frame = NetInterfaceFrame();
        return NI_NO_MEMORY;
    }
}

NetSenderServiceQueue::NetSenderServiceQueue() :
m_free(0),
m_count(0),
m_bytes(0),
m_spareCapacity(0)
{
    for (uint32_t lane = 0; lane < 3; ++lane)
    {
        m_heads[lane] = m_tails[lane] = 256;
        m_sizes[lane] = 0;
    }
    for (uint32_t index = 0; index < 256; ++index)
    {
        m_items[index].m_next = index + 1;
    }
}

NetSenderServiceQueue::~NetSenderServiceQueue()
{
    clear();
    for (uint32_t index = 0; index < 256; ++index)
    {
        release(index);
    }
}

bool NetSenderServiceQueue::room(uint32_t count, uint64_t bytes) const
{
    return count <= 256 - m_count && bytes <= 8 * 1048576 - m_bytes;
}

int32_t NetSenderServiceQueue::push(uint32_t lane, uint64_t endpoint, const NetInterfaceFrame& frame,
    const NetInterfaceOrigin& origin, uint64_t now)
{
    if (lane >= 3 || (frame.m_payloadSize != 0 && frame.m_payload == nullptr))
    {
        return NI_INVALID_ARGUMENT;
    }
    if (frame.m_kind == NI_CANCEL)
    {
        for (uint32_t index = m_heads[lane]; index != 256; index = m_items[index].m_next)
        {
            const NetSenderServiceManagedFrame& pending = m_items[index];
            if (pending.m_endpoint == endpoint && pending.m_frame.m_kind == NI_REQUEST &&
                pending.m_frame.m_requestId == frame.m_requestId && pending.m_frame.m_productId == frame.m_productId &&
                pending.m_frame.m_messageType == frame.m_messageType)
            {
                release(index);
                return NI_OK;
            }
        }
    }
    if (!room(1, frame.m_payloadSize))
    {
        return NI_LIMIT_EXCEEDED;
    }
    const uint32_t index = m_free;
    NetSenderServiceManagedFrame& item = m_items[index];
    try
    {
        if (frame.m_payloadSize != 0)
        {
            if (m_spare && frame.m_payloadSize <= m_spareCapacity)
            {
                item.m_bytes.swap(m_spare);
                item.m_capacity = m_spareCapacity;
                m_spareCapacity = 0;
            }
            else
            {
                item.m_bytes.reset(new uint8_t[frame.m_payloadSize]);
                item.m_capacity = frame.m_payloadSize;
            }
            std::memcpy(item.m_bytes.get(), frame.m_payload, frame.m_payloadSize);
        }
    }
    catch (...)
    {
        item.m_bytes.reset();
        item.m_capacity = 0;
        return NI_NO_MEMORY;
    }
    item.m_frame = frame;
    item.m_frame.m_payload = nullptr;
    item.m_origin = origin;
    item.m_endpoint = endpoint;
    item.m_created = now;
    item.m_blocked = item.m_channel = 0;
    m_free = item.m_next;
    item.m_next = 256;
    item.m_previous = m_tails[lane];
    item.m_lane = lane;
    if (m_tails[lane] != 256)
    {
        m_items[m_tails[lane]].m_next = index;
    }
    else
    {
        m_heads[lane] = index;
    }
    m_tails[lane] = index;
    ++m_sizes[lane];
    ++m_count;
    m_bytes += frame.m_payloadSize;
    return NI_OK;
}

uint32_t NetSenderServiceQueue::first(uint32_t lane) const
{
    return lane < 3 ? m_heads[lane] : 256;
}

uint32_t NetSenderServiceQueue::last(uint32_t lane) const
{
    return lane < 3 ? m_tails[lane] : 256;
}

NetSenderServiceManagedFrame* NetSenderServiceQueue::get(uint32_t index)
{
    return index < 256 && m_items[index].m_lane != 6 ? &m_items[index] : nullptr;
}

const NetSenderServiceManagedFrame* NetSenderServiceQueue::get(uint32_t index) const
{
    return index < 256 && m_items[index].m_lane != 6 ? &m_items[index] : nullptr;
}

void NetSenderServiceQueue::detach(uint32_t index)
{
    NetSenderServiceManagedFrame& item = m_items[index];
    const uint32_t lane = item.m_lane;
    if (item.m_previous != 256)
    {
        m_items[item.m_previous].m_next = item.m_next;
    }
    else
    {
        m_heads[lane] = item.m_next;
    }
    if (item.m_next != 256)
    {
        m_items[item.m_next].m_previous = item.m_previous;
    }
    else
    {
        m_tails[lane] = item.m_previous;
    }
    --m_sizes[lane];
    item.m_next = item.m_previous = 256;
    item.m_lane += 3;
}

NetSenderServiceManagedFrame* NetSenderServiceQueue::borrow(uint32_t index)
{
    NetSenderServiceManagedFrame* item = get(index);
    if (item == nullptr || item->m_lane >= 3)
    {
        return nullptr;
    }
    detach(index);
    return item;
}

bool NetSenderServiceQueue::restore(uint32_t index)
{
    NetSenderServiceManagedFrame* item = get(index);
    if (item == nullptr || item->m_lane < 3)
    {
        return false;
    }
    const uint32_t lane = item->m_lane - 3;
    item->m_lane = lane;
    item->m_next = m_heads[lane];
    item->m_previous = 256;
    if (m_heads[lane] != 256)
    {
        m_items[m_heads[lane]].m_previous = index;
    }
    else
    {
        m_tails[lane] = index;
    }
    m_heads[lane] = index;
    ++m_sizes[lane];
    return true;
}

void NetSenderServiceQueue::release(uint32_t index)
{
    NetSenderServiceManagedFrame* item = get(index);
    if (item == nullptr)
    {
        return;
    }
    if (item->m_lane < 3)
    {
        detach(index);
    }
    m_bytes -= item->m_frame.m_payloadSize;
    --m_count;
    if (item->m_bytes)
    {
        NetInterface::wipe(item->m_bytes.get(), item->m_frame.m_payloadSize);
    }
    if (item->m_capacity <= 4096 && item->m_capacity > m_spareCapacity)
    {
        m_spare = std::move(item->m_bytes);
        m_spareCapacity = item->m_capacity;
    }
    item->m_bytes.reset();
    item->m_capacity = 0;
    item->m_frame = NetInterfaceFrame();
    item->m_origin = NetInterfaceOrigin();
    item->m_endpoint = item->m_created = item->m_blocked = item->m_channel = 0;
    item->m_lane = 6;
    item->m_previous = 256;
    item->m_next = m_free;
    m_free = index;
}

void NetSenderServiceQueue::clear(uint32_t lane)
{
    if (lane < 3)
    {
        while (m_heads[lane] != 256)
        {
            release(m_heads[lane]);
        }
    }
}

void NetSenderServiceQueue::clear()
{
    for (uint32_t lane = 0; lane < 3; ++lane)
    {
        clear(lane);
    }
}

uint32_t NetSenderServiceQueue::size(uint32_t lane) const
{
    return lane < 3 ? m_sizes[lane] : 0;
}

uint32_t NetSenderServiceQueue::size() const
{
    return m_count;
}

uint64_t NetSenderServiceQueue::bytes() const
{
    return m_bytes;
}

NetSenderServiceQueueLoan::NetSenderServiceQueueLoan(NetSenderServiceQueue& queue, uint32_t index) :
m_queue(queue),
m_index(index)
{

}

NetSenderServiceQueueLoan::~NetSenderServiceQueueLoan()
{
    release();
}

void NetSenderServiceQueueLoan::release()
{
    m_queue.release(m_index);
    m_index = 256;
}

bool NetSenderServiceQueueLoan::restore()
{
    if (!m_queue.restore(m_index))
    {
        return false;
    }
    m_index = 256;
    return true;
}

NetSenderServiceQueueScratch::~NetSenderServiceQueueScratch()
{
    if (!m_request.empty())
    {
        NetInterface::wipe(m_request.data(), static_cast<uint32_t>(m_request.size()));
    }
    if (!m_event.empty())
    {
        NetInterface::wipe(m_event.data(), static_cast<uint32_t>(m_event.size()));
    }
    if (!m_wrapped.empty())
    {
        NetInterface::wipe(m_wrapped.data(), static_cast<uint32_t>(m_wrapped.size()));
    }
}

// NET_SERVICE_QUEUE_UNIT_BEGIN
// #include "NetSender/NetSenderServiceQueue.h"
// #include <cstdio>
// #include <cstring>
// #include <stdexcept>
// #if defined(_WIN32) && defined(_DEBUG) && !defined(NET_SERVICE_QUEUE_NO_HEAP)
// #include <crtdbg.h>
// #include <malloc.h>
// #endif
//
// // 只在隔离测试程序中计数，不进入库或业务进程。
// static uint32_t s_serviceQueueChecks = 0;
// #define SERVICE_QUEUE_CHECK(value) { ++s_serviceQueueChecks; if (!(value)) { std::printf("SERVICE_QUEUE_FAIL line=%u %s\n", static_cast<uint32_t>(__LINE__), #value); return false; } }
//
// /** 检查三队列共有额度、拥有期、取消和期限，使用真实队列而非替代实现
// @return 全部边界是否满足
// */
// static bool ServiceQueueBoundaries()
// {
//     NetSenderServiceQueue queue;
//     NetInterfaceFrame frame = {};
//     NetInterfaceOrigin origin = {};
//     origin.m_endpoint = 91;
//     uint8_t bytes[32];
//     std::memset(bytes, 0x59, sizeof(bytes));
//     frame.m_kind = NI_EVENT;
//     frame.m_productId = 101;
//     frame.m_messageType = 700;
//     frame.m_payload = bytes;
//     frame.m_payloadSize = sizeof(bytes);
//     SERVICE_QUEUE_CHECK(queue.size() == 0 && queue.bytes() == 0 && queue.room(256, 8 * 1048576));
//     SERVICE_QUEUE_CHECK(!queue.room(257, 0) && !queue.room(1, UINT64_MAX));
//     SERVICE_QUEUE_CHECK(queue.first(3) == 256 && queue.last(3) == 256 && queue.size(3) == 0);
//     SERVICE_QUEUE_CHECK(queue.get(256) == nullptr && queue.borrow(256) == nullptr && !queue.restore(256));
//     SERVICE_QUEUE_CHECK(queue.push(3, 1, frame, origin, 1000) == NI_INVALID_ARGUMENT);
//     frame.m_payload = nullptr;
//     SERVICE_QUEUE_CHECK(queue.push(0, 1, frame, origin, 1000) == NI_INVALID_ARGUMENT);
//     frame.m_payload = bytes;
//     frame.m_payloadSize = UINT32_MAX;
//     SERVICE_QUEUE_CHECK(queue.push(0, 1, frame, origin, 1000) == NI_LIMIT_EXCEEDED);
//     frame.m_payloadSize = sizeof(bytes);
//     for (uint32_t index = 0; index < 256; ++index)
//     {
//         frame.m_requestId = index + 1;
//         SERVICE_QUEUE_CHECK(queue.push(index % 3, index + 1, frame, origin, 1000) == NI_OK);
//     }
//     SERVICE_QUEUE_CHECK(queue.size() == 256 && queue.bytes() == 256 * sizeof(bytes));
//     SERVICE_QUEUE_CHECK(queue.push(1, 1, frame, origin, 1000) == NI_LIMIT_EXCEEDED);
//     std::memset(bytes, 0xa9, sizeof(bytes));
//     for (uint32_t lane = 0; lane < 3; ++lane)
//     {
//         uint64_t expected = lane + 1;
//         while (queue.first(lane) != 256)
//         {
//             const uint32_t index = queue.first(lane);
//             NetSenderServiceManagedFrame* item = queue.borrow(index);
//             SERVICE_QUEUE_CHECK(item != nullptr && item->m_frame.m_requestId == expected && item->m_bytes[0] == 0x59);
//             SERVICE_QUEUE_CHECK(queue.borrow(index) == nullptr);
//             const uint32_t owned = queue.size();
//             SERVICE_QUEUE_CHECK(queue.restore(index) && queue.size() == owned && queue.first(lane) == index);
//             SERVICE_QUEUE_CHECK(!queue.restore(index));
//             queue.release(index);
//             queue.release(index);
//             SERVICE_QUEUE_CHECK(queue.size() + 1 == owned && queue.get(index) == nullptr);
//             expected += 3;
//         }
//         SERVICE_QUEUE_CHECK(queue.last(lane) == 256 && queue.size(lane) == 0);
//     }
//     SERVICE_QUEUE_CHECK(queue.size() == 0 && queue.bytes() == 0);
//
//     // 无分配归队与清空：清空其他消息仍须保留回调手里的正文及预算。
//     for (uint32_t round = 0; round < 1000; ++round)
//     {
//         SERVICE_QUEUE_CHECK(queue.push(0, 1, frame, origin, 1000) == NI_OK);
//         const uint32_t index = queue.first(0);
//         NetSenderServiceManagedFrame* borrowed = queue.borrow(index);
//         SERVICE_QUEUE_CHECK(borrowed != nullptr);
//         {
//             NetSenderServiceQueueLoan loan(queue, index);
//             SERVICE_QUEUE_CHECK(queue.push(1, 2, frame, origin, 1000) == NI_OK);
//             SERVICE_QUEUE_CHECK(queue.push(2, 3, frame, origin, 1000) == NI_OK);
//             queue.clear();
//             SERVICE_QUEUE_CHECK(queue.size() == 1 && queue.bytes() == sizeof(bytes) && borrowed->m_bytes[0] == 0xa9);
//             SERVICE_QUEUE_CHECK(loan.restore() && queue.size(0) == 1 && queue.size() == 1);
//         }
//         queue.clear();
//         SERVICE_QUEUE_CHECK(queue.size() == 0 && queue.bytes() == 0);
//     }
//     SERVICE_QUEUE_CHECK(queue.push(0, 1, frame, origin, 1000) == NI_OK);
//     {
//         const uint32_t index = queue.first(0);
//         SERVICE_QUEUE_CHECK(queue.borrow(index) != nullptr);
//         NetSenderServiceQueueLoan loan(queue, index);
//         loan.release();
//         SERVICE_QUEUE_CHECK(queue.size() == 0);
//         SERVICE_QUEUE_CHECK(queue.push(0, 2, frame, origin, 1000) == NI_OK);
//     }
//     SERVICE_QUEUE_CHECK(queue.size() == 1 && queue.get(queue.first(0))->m_endpoint == 2);
//     queue.clear();
//     bool caught = false;
//     try
//     {
//         SERVICE_QUEUE_CHECK(queue.push(0, 1, frame, origin, 1000) == NI_OK);
//         const uint32_t index = queue.first(0);
//         SERVICE_QUEUE_CHECK(queue.borrow(index) != nullptr);
//         NetSenderServiceQueueLoan loan(queue, index);
//         throw std::runtime_error("controlled callback failure");
//     }
//     catch (...)
//     {
//         caught = true;
//     }
//     SERVICE_QUEUE_CHECK(caught && queue.size() == 0 && queue.bytes() == 0);
//
//     std::vector<uint8_t> large(1048576, 0x59);
//     frame.m_payload = large.data();
//     frame.m_payloadSize = static_cast<uint32_t>(large.size());
//     for (uint32_t index = 0; index < 8; ++index)
//     {
//         SERVICE_QUEUE_CHECK(queue.push(index % 3, index, frame, origin, 1000) == NI_OK);
//     }
//     const uint32_t borrowedIndex = queue.first(0);
//     SERVICE_QUEUE_CHECK(queue.borrow(borrowedIndex) != nullptr);
//     SERVICE_QUEUE_CHECK(queue.size() == 8 && queue.bytes() == 8 * 1048576);
//     SERVICE_QUEUE_CHECK(queue.push(0, 9, frame, origin, 1000) == NI_LIMIT_EXCEEDED);
//     queue.clear();
//     SERVICE_QUEUE_CHECK(queue.size() == 1 && queue.bytes() == 1048576);
//     queue.release(borrowedIndex);
//     SERVICE_QUEUE_CHECK(queue.size() == 0 && queue.bytes() == 0);
//
//     frame.m_payload = nullptr;
//     frame.m_payloadSize = 0;
//     for (uint32_t index = 0; index < 256; ++index)
//     {
//         SERVICE_QUEUE_CHECK(queue.push(0, index, frame, origin, 1000) == NI_OK);
//     }
//     SERVICE_QUEUE_CHECK(queue.size() == 256 && queue.bytes() == 0);
//     SERVICE_QUEUE_CHECK(queue.push(0, 9, frame, origin, 1000) == NI_LIMIT_EXCEEDED);
//     queue.clear();
//
//     // 取消仅删除完整匹配的待交付请求，不能取消别的端点、产品或命令。
//     uint8_t request[48] = {};
//     uint32_t required = 0;
//     SERVICE_QUEUE_CHECK(NetInterfaceRpc::encode(NI_REQUEST, 5000, 0, bytes, sizeof(bytes), request, sizeof(request), required) == NI_OK);
//     frame.m_kind = NI_REQUEST;
//     frame.m_payload = request;
//     frame.m_payloadSize = required;
//     for (uint32_t index = 0; index < 5; ++index)
//     {
//         frame.m_requestId = index + 1;
//         SERVICE_QUEUE_CHECK(queue.push(0, 7, frame, origin, 1000) == NI_OK);
//     }
//     NetInterfaceFrame cancel = frame;
//     cancel.m_kind = NI_CANCEL;
//     cancel.m_payload = nullptr;
//     cancel.m_payloadSize = 0;
//     const uint32_t cancelled[3] = {1, 3, 5};
//     for (uint32_t index = 0; index < 3; ++index)
//     {
//         cancel.m_requestId = cancelled[index];
//         SERVICE_QUEUE_CHECK(queue.push(0, 7, cancel, origin, 1000) == NI_OK && queue.size() == 4 - index);
//     }
//     SERVICE_QUEUE_CHECK(queue.get(queue.first(0))->m_frame.m_requestId == 2);
//     SERVICE_QUEUE_CHECK(queue.get(queue.last(0))->m_frame.m_requestId == 4);
//     cancel.m_requestId = 2;
//     SERVICE_QUEUE_CHECK(queue.push(0, 8, cancel, origin, 1000) == NI_OK && queue.size() == 3);
//     ++cancel.m_productId;
//     SERVICE_QUEUE_CHECK(queue.push(0, 7, cancel, origin, 1000) == NI_OK && queue.size() == 4);
//     --cancel.m_productId;
//     ++cancel.m_messageType;
//     SERVICE_QUEUE_CHECK(queue.push(0, 7, cancel, origin, 1000) == NI_OK && queue.size() == 5);
//     --cancel.m_messageType;
//     SERVICE_QUEUE_CHECK(queue.push(1, 7, cancel, origin, 1000) == NI_OK && queue.size() == 6);
//     const uint32_t active = queue.first(0);
//     SERVICE_QUEUE_CHECK(queue.borrow(active) != nullptr);
//     SERVICE_QUEUE_CHECK(queue.push(0, 7, cancel, origin, 1000) == NI_OK && queue.size() == 7);
//     queue.clear();
//     SERVICE_QUEUE_CHECK(queue.size() == 1);
//     {
//         NetSenderServiceQueueScratch scratch;
//         NetInterfaceFrame view = {};
//         NetInterfaceRpcView rpc = {};
//         SERVICE_QUEUE_CHECK(queue.get(active)->view(999, view, scratch.m_request) == NI_PROTOCOL_ERROR);
//         SERVICE_QUEUE_CHECK(queue.get(active)->view(2000, view, scratch.m_request) == NI_READY);
//         SERVICE_QUEUE_CHECK(NetInterfaceRpc::parse(view, rpc) == NI_OK && rpc.m_value == 4000 && rpc.m_size == 32);
//         SERVICE_QUEUE_CHECK(queue.restore(active) && queue.borrow(active) != nullptr);
//         SERVICE_QUEUE_CHECK(queue.get(active)->view(5999, view, scratch.m_request) == NI_READY);
//         SERVICE_QUEUE_CHECK(NetInterfaceRpc::parse(view, rpc) == NI_OK && rpc.m_value == 1);
//         SERVICE_QUEUE_CHECK(queue.get(active)->view(6000, view, scratch.m_request) == NI_TIMEOUT);
//     }
//     queue.release(active);
//     frame.m_payloadSize = 1;
//     SERVICE_QUEUE_CHECK(queue.push(0, 7, frame, origin, 1000) == NI_OK);
//     {
//         NetSenderServiceQueueScratch scratch;
//         NetInterfaceFrame view = {};
//         SERVICE_QUEUE_CHECK(queue.get(queue.first(0))->view(1000, view, scratch.m_request) == NI_PROTOCOL_ERROR);
//     }
//     queue.clear();
//     return true;
// }
//
// #if defined(_WIN32) && defined(_DEBUG) && !defined(NET_SERVICE_QUEUE_NO_HEAP)
// // 隔离main的CRT观察器，不在有其他业务线程的NetWorkTest中启用。
// static int32_t s_serviceQueueRejectAfter = -1;
// static uint32_t s_serviceQueueAllocations = 0;
// static uint32_t s_serviceQueueExposed = 0;
//
// /** 只拦截普通堆块，核对失败回滚、零分配清理和释放前正文擦除
// @param [in] operation CRT操作类别
// @param [in] pointer 释放时仍有效的堆块
// @param [in] block 堆块类别，其余形参遵从CRT回调
// @return 允许本次操作时为1
// */
// static int __cdecl ServiceQueueMemoryHook(int operation, void* pointer, size_t, int block, long, const unsigned char*, int)
// {
//     if (block != _NORMAL_BLOCK)
//     {
//         return 1;
//     }
//     if (operation == _HOOK_ALLOC || operation == _HOOK_REALLOC)
//     {
//         ++s_serviceQueueAllocations;
//         if (s_serviceQueueRejectAfter == 0)
//         {
//             return 0;
//         }
//         if (s_serviceQueueRejectAfter > 0)
//         {
//             --s_serviceQueueRejectAfter;
//         }
//     }
//     if (operation == _HOOK_FREE && pointer != nullptr && _msize_dbg(pointer, _NORMAL_BLOCK) >= 32)
//     {
//         const uint8_t* bytes = static_cast<const uint8_t*>(pointer);
//         uint32_t index = 0;
//         while (index < 32 && bytes[index] == 0x59)
//         {
//             ++index;
//         }
//         if (index == 32)
//         {
//             ++s_serviceQueueExposed;
//         }
//     }
//     return 1;
// }
//
// /** 在真实静态库内拒绝分配，回滚后必须可重试；释放借用不准再次分配
// @return 内存路径是否通过
// */
// static bool ServiceQueueMemory()
// {
//     uint8_t bytes[8192];
//     std::memset(bytes, 0x59, sizeof(bytes));
//     NetInterfaceOrigin origin = {};
//     NetInterfaceFrame frame = {};
//     frame.m_kind = NI_EVENT;
//     frame.m_payload = bytes;
//     frame.m_payloadSize = sizeof(bytes);
//     frame.m_productId = 101;
//     frame.m_messageType = 700;
//     frame.m_requestId = 1;
//     NetSenderServiceQueue queue;
//     s_serviceQueueRejectAfter = 0;
//     _CRT_ALLOC_HOOK previous = _CrtSetAllocHook(ServiceQueueMemoryHook);
//     const int32_t rejected = queue.push(0, 1, frame, origin, 1000);
//     _CrtSetAllocHook(previous);
//     s_serviceQueueRejectAfter = -1;
//     SERVICE_QUEUE_CHECK(rejected == NI_NO_MEMORY && queue.size() == 0 && queue.bytes() == 0);
//     SERVICE_QUEUE_CHECK(queue.push(0, 1, frame, origin, 1000) == NI_OK);
//     const uint32_t index = queue.first(0);
//     SERVICE_QUEUE_CHECK(queue.borrow(index) != nullptr);
//     s_serviceQueueRejectAfter = 0;
//     s_serviceQueueAllocations = s_serviceQueueExposed = 0;
//     previous = _CrtSetAllocHook(ServiceQueueMemoryHook);
//     const bool restored = queue.restore(index);
//     queue.clear();
//     _CrtSetAllocHook(previous);
//     s_serviceQueueRejectAfter = -1;
//     SERVICE_QUEUE_CHECK(restored && s_serviceQueueAllocations == 0 && s_serviceQueueExposed == 0 && queue.size() == 0);
//
//     frame.m_payloadSize = 1024;
//     SERVICE_QUEUE_CHECK(queue.push(0, 1, frame, origin, 1000) == NI_OK);
//     queue.clear();
//     s_serviceQueueAllocations = 0;
//     s_serviceQueueRejectAfter = 0;
//     bool warm = true;
//     previous = _CrtSetAllocHook(ServiceQueueMemoryHook);
//     for (uint32_t round = 0; round < 100000; ++round)
//     {
//         warm = warm && queue.push(round % 3, 1, frame, origin, 1000) == NI_OK;
//         const uint32_t current = queue.first(round % 3);
//         warm = warm && queue.borrow(current) != nullptr;
//         queue.release(current);
//     }
//     _CrtSetAllocHook(previous);
//     s_serviceQueueRejectAfter = -1;
//     SERVICE_QUEUE_CHECK(warm && s_serviceQueueAllocations == 0 && queue.size() == 0 && queue.bytes() == 0);
//
//     uint8_t request[48] = {};
//     uint32_t required = 0;
//     SERVICE_QUEUE_CHECK(NetInterfaceRpc::encode(NI_REQUEST, 5000, 0, bytes, 32, request, sizeof(request), required) == NI_OK);
//     frame.m_kind = NI_REQUEST;
//     frame.m_payload = request;
//     frame.m_payloadSize = required;
//     SERVICE_QUEUE_CHECK(queue.push(0, 1, frame, origin, 1000) == NI_OK);
//     {
//         NetSenderServiceQueueScratch scratch;
//         NetInterfaceFrame view = {};
//         s_serviceQueueRejectAfter = 0;
//         previous = _CrtSetAllocHook(ServiceQueueMemoryHook);
//         const int32_t result = queue.get(queue.first(0))->view(1001, view, scratch.m_request);
//         _CrtSetAllocHook(previous);
//         s_serviceQueueRejectAfter = -1;
//         SERVICE_QUEUE_CHECK(result == NI_NO_MEMORY && queue.size() == 1 && queue.bytes() == required);
//         SERVICE_QUEUE_CHECK(queue.get(queue.first(0))->view(1001, view, scratch.m_request) == NI_READY);
//     }
//     queue.clear();
//     bool caught = false;
//     previous = _CrtSetAllocHook(ServiceQueueMemoryHook);
//     try
//     {
//         NetSenderServiceQueueScratch scratch;
//         scratch.m_request.assign(bytes, bytes + sizeof(bytes));
//         scratch.m_event.assign(bytes, bytes + sizeof(bytes));
//         scratch.m_wrapped.assign(bytes, bytes + sizeof(bytes));
//         throw std::runtime_error("controlled scratch failure");
//     }
//     catch (...)
//     {
//         caught = true;
//     }
//     _CrtSetAllocHook(previous);
//     SERVICE_QUEUE_CHECK(caught && s_serviceQueueExposed == 0);
//     return true;
// }
// #endif
//
// /** 汇总真实队列的持有期和资源预算测试
// @return 是否通过
// */
// static bool ServiceQueueAudit()
// {
//     s_serviceQueueChecks = 0;
//     SERVICE_QUEUE_CHECK(ServiceQueueBoundaries());
// #if defined(_WIN32) && defined(_DEBUG) && !defined(NET_SERVICE_QUEUE_NO_HEAP)
//     SERVICE_QUEUE_CHECK(ServiceQueueMemory());
// #endif
//     return true;
// }
//
// #ifndef NET_SERVICE_QUEUE_NO_HEAP
// /** 库内手工测试入口，正式构建保持整段注释
// @return 0通过，其余表示失败
// */
// int main()
// {
//     try
//     {
//         std::printf("Service queue audit starting\n");
// #if defined(_WIN32) && defined(_DEBUG)
//         _CrtMemState before;
//         _CrtMemState after;
//         _CrtMemState difference;
//         _CrtMemCheckpoint(&before);
// #endif
//         if (!ServiceQueueAudit())
//         {
//             return 1;
//         }
// #if defined(_WIN32) && defined(_DEBUG)
//         _CrtMemCheckpoint(&after);
//         _CrtMemDifference(&difference, &before, &after);
//         if (difference.lCounts[_NORMAL_BLOCK] != 0 || difference.lSizes[_NORMAL_BLOCK] != 0)
//         {
//             std::printf("Service queue heap blocks=%ld bytes=%ld FAIL\n", difference.lCounts[_NORMAL_BLOCK], difference.lSizes[_NORMAL_BLOCK]);
//             return 2;
//         }
// #endif
//         std::printf("Service queue audit checks=%u PASS\n", s_serviceQueueChecks);
//         return 0;
//     }
//     catch (...)
//     {
//         return 3;
//     }
// }
// #endif
// #undef SERVICE_QUEUE_CHECK
// NET_SERVICE_QUEUE_UNIT_END