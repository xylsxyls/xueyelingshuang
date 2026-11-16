#include "NetServiceTestAccess.h"
#include "NetSenderTestAccess.h"
#include <LibuvTcp/LibuvTcpAPI.h>
#include "NetWorkConcurrencyTestCases.h"
#include "NetWorkTestAccess.h"
#include "NetSenderTestAccess.h"
#include "NetTestTlsCertificates.h"
#include "CSystem/CSystemAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "NetWorkTestCases.h"
#include <stdexcept>
#include <cstdio>
#include <cstring>
#include <chrono>
#include <algorithm>
#include <cstdlib>
#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#endif

NetSenderServiceQueueTest::NetSenderServiceQueueTest() :
m_mode(0),
m_service(nullptr),
m_admitted(0),
m_closedCount(0),
m_allow(false),
m_healthy(0),
m_bad(false)
{

}

void NetSenderServiceQueueTest::onServiceReady(uint64_t endpoint)
{
    (void)endpoint; m_bad = true;
}

void NetSenderServiceQueueTest::onServiceClosed(int32_t reason)
{
    ++m_closedCount;
    if (m_mode != 3 || reason != NI_CANCELLED)
    {
        m_bad = true;
    }
}

bool NetSenderServiceQueueTest::onServiceFrame(const NetInterfaceFrame& frame, const NetInterfaceOrigin& origin)
{
    if (m_mode != 0)
    {
        const uint32_t size = m_mode == 2 ? 1048576U : 1U;
        if (frame.m_kind != NI_EVENT || frame.m_payloadSize != size || frame.m_payload[0] != 0x59)
        {
            m_bad = true;
            return true;
        }
        if (m_mode == 1 || m_mode == 2)
        {
            const uint32_t count = m_mode == 1 ? 256U : 8U;
            for (uint32_t index = 0; index < count; ++index)
            {
                const int32_t result = ((*m_service).*netServiceTestMember(NetServiceTestManagedPush()))(0, 1, frame, origin);
                if (result == NI_OK)
                {
                    ++m_admitted;
                }
                else if (result != NI_LIMIT_EXCEEDED)
                {
                    m_bad = true;
                }
            }
        }
        else if (m_mode == 3)
        {
            ((*m_service).*netServiceTestMember(NetServiceTestManagedServiceClosed()))(NI_CANCELLED);
            m_bad = m_bad || ((*m_service).*netServiceTestMember(NetServiceTestManagedQueue())).size() != 1 || ((*m_service).*netServiceTestMember(NetServiceTestManagedQueue())).bytes() != size;
        }
        else if (m_mode == 4)
        {
            throw std::runtime_error("controlled service callback exception");
        }
        m_bad = m_bad || frame.m_payload[0] != 0x59 || frame.m_payload[size - 1] != 0x59;
        return false;
    }
    NetInterfaceRpcView rpc = {};
    if (NetInterfaceRpc::parse(frame, rpc) != NI_OK || rpc.m_size != 1 ||
        frame.m_productId != 101 || (origin.m_endpoint != 41 && origin.m_endpoint != 42))
    {
        m_bad = true; return true;
    }
    if (origin.m_endpoint == 42) { ++m_healthy; return true; }
    if (!m_allow) { return false; }
    m_seen.push_back(rpc.m_data[0]);
    return true;
}

/** 验证真实服务队列存储、扩容中借用和显式小池回收，不连接网络
*/
static void ServiceQueuePoolCheck()
{
    const auto require = [](bool passed, const char* message)
    {
        if (!passed) { throw std::runtime_error(message); }
    };
    NetSenderServiceQueue queue;
    NetInterfaceOrigin origin = {};
    NetInterfaceFrame frame = {};
    frame.m_kind = NI_EVENT;
    frame.m_productId = 101;
    frame.m_messageType = 707;
    const uint8_t saved[] = {'1', '2', '3', '4', '5', '6'};
    frame.m_payload = saved;
    frame.m_payloadSize = sizeof(saved);
    require(queue.push(0, 1, frame, origin, 1) == NI_OK, "prepare service queue borrowed frame");
    const uint32_t held = queue.first(0);
    NetSenderServiceManagedFrame* borrowed = queue.borrow(held);
    require(borrowed != nullptr && queue.size() == 1 && queue.size(0) == 0, "borrow remains accounted");
    for (uint32_t round = 0; round < 2; ++round)
    {
        uint64_t total = sizeof(saved);
        for (uint32_t index = 0; index < 4096; ++index)
        {
            const uint32_t length = index < 400 ? 16384 : index < 432 ? 131072 : 6;
            std::vector<uint8_t> body(length, static_cast<uint8_t>(index % 251));
            frame.m_payload = body.data();
            frame.m_payloadSize = length;
            frame.m_requestId = index + 1;
            require(queue.push(index % 3, 1, frame, origin, 1) == NI_OK, "default service pools accept beyond old fixed gates");
            total += length;
        }
        require(queue.size() == 4097 && queue.bytes() == total && total > 8 * 1048576 &&
            queue.get(held) == borrowed && std::memcmp(borrowed->m_bytes.data(), saved, sizeof(saved)) == 0,
            "growing directory preserves borrowed object and exact accounting");
        for (uint32_t lane = 0; lane < 3; ++lane)
        {
            for (uint32_t index = lane; index < 4096; index += 3)
            {
                const uint32_t entry = queue.first(lane);
                const NetSenderServiceManagedFrame* value = queue.get(entry);
                const uint32_t length = index < 400 ? 16384 : index < 432 ? 131072 : 6;
                require(value != nullptr && value->m_frame.m_requestId == index + 1 && value->m_frame.m_payloadSize == length &&
                    value->m_bytes.data()[0] == index % 251 && value->m_bytes.data()[length - 1] == index % 251,
                    "each service lane remains FIFO with owned bytes");
                queue.release(entry);
            }
        }
        queue.clear();
        require(queue.size() == 1 && queue.bytes() == sizeof(saved) && queue.get(held) == borrowed,
            "queue clear does not release active callback borrow");
    }
    require(queue.restore(held) && queue.first(0) == held, "borrow can return without allocation");
    queue.release(held);
    require(queue.size() == 0 && queue.bytes() == 0 && queue.get(held) == nullptr && queue.first(0) == UINT32_MAX,
        "final loan releases its storage");
    NetBufferPoolConfig bodies;
    LockFreeHashMapPoolConfig records = NetSenderServiceQueue::defaultRecords();
    records.initialCapacity = records.maximumCapacity = 2;
    require(queue.configurePools(bodies, records) == NI_INVALID_STATE, "used queue cannot replace pool");
    NetSenderServiceQueue bounded;
    require(bounded.configurePools(bodies, records) == NI_OK, "configure explicit two-slot fixture");
    records.initialCapacity = 3;
    require(bounded.configurePools(bodies, records) == NI_INVALID_ARGUMENT, "invalid service queue pool rolls back");
    frame.m_payload = saved;
    frame.m_payloadSize = sizeof(saved);
    for (uint32_t round = 0; round < 4; ++round)
    {
        require(bounded.room(2, 12) == NI_OK && bounded.size() == 0 && bounded.bytes() == 0,
            "broadcast storage probe releases its temporary leases");
        require(bounded.push(0, 1, frame, origin, 1) == NI_OK && bounded.room(2, 12) == NI_LIMIT_EXCEEDED &&
            bounded.room(1, 6) == NI_OK && bounded.push(1, 1, frame, origin, 1) == NI_OK &&
            bounded.push(2, 1, frame, origin, 1) == NI_LIMIT_EXCEEDED && bounded.size() == 2 && bounded.bytes() == 12,
            "actual full pool has no partial admission or overwritten bytes");
        bounded.clear();
    }
    require(bounded.room(0, 0) == NI_OK && bounded.room(0, 1) == NI_INVALID_ARGUMENT &&
        bounded.room(2, 3) == NI_INVALID_ARGUMENT && bounded.room(1, 1048705) == NI_INVALID_ARGUMENT,
        "broadcast storage check validates exact lengths");
    NetSenderLocalInbox inbox;
    NetSenderService service(inbox);
    records.initialCapacity = 2;
    require(service.configureQueuePools(bodies, records) == NI_OK, "public managed service pool configuration");
    records.initialCapacity = 3;
    require(service.configureQueuePools(bodies, records) == NI_INVALID_ARGUMENT, "public invalid pool preserves prior config");
    std::printf("SERVICE_QUEUE_POOL rounds=2 frames_per_round=4096 borrowed=1 PASS\n");
}

void NetSenderServiceQueueTest::run()
{
    ServiceQueuePoolCheck();
    NetSenderLocalInbox inbox;
    NetSenderServiceManaged service(inbox);
    NetSenderServiceQueueTest observer;
    ((service).*netServiceTestMember(NetServiceTestManagedListener())) = &observer;
    uint8_t payload[17] = {};
    NetInterfaceFrame frame = {};
    frame.m_kind = NI_REQUEST; frame.m_productId = 101; frame.m_messageType = 700;
    frame.m_payload = payload; frame.m_payloadSize = sizeof(payload);
    NetInterfaceOrigin origin = {};
    origin.m_endpoint = 41; origin.m_platform = 1; origin.m_device[0] = 7;
    for (uint32_t index = 0; index < 101; ++index)
    {
        const uint8_t byte = static_cast<uint8_t>(index);
        uint32_t required = 0;
        if (NetInterfaceRpc::encode(NI_REQUEST, 5000, 0, &byte, 1, payload, sizeof(payload), required) != NI_OK)
        {
            throw std::runtime_error("service FIFO fixture encoding failed");
        }
        frame.m_requestId = index + 1;
        if (index == 100) { origin.m_endpoint = 42; }
        if (((service).*netServiceTestMember(NetServiceTestManagedPush()))(0, 1, frame, origin) != NI_OK)
        {
            throw std::runtime_error("service FIFO fixture admission failed");
        }
    }
    ((service).*netServiceTestMember(NetServiceTestManagedPump()))(0, NetSenderSubmission::now());
    const uint32_t healthyDuringPressure = observer.m_healthy;
    observer.m_allow = true;
    for (uint32_t round = 0; round < 8 && ((service).*netServiceTestMember(NetServiceTestManagedQueue())).size(0) != 0; ++round)
    {
        ((service).*netServiceTestMember(NetServiceTestManagedPump()))(0, NetSenderSubmission::now());
    }
    bool ordered = observer.m_seen.size() == 100;
    for (uint32_t index = 0; ordered && index < observer.m_seen.size(); ++index) { ordered = observer.m_seen[index] == index; }
    std::printf("SERVICE_QUEUE_FIFO healthy_while_blocked=%u first_after_resume=%u count=%u ordered=%u\n",
        healthyDuringPressure, observer.m_seen.empty() ? 999U : observer.m_seen[0],
        static_cast<uint32_t>(observer.m_seen.size()), ordered ? 1U : 0U);
    if (healthyDuringPressure != 1 || !ordered || observer.m_bad || ((service).*netServiceTestMember(NetServiceTestManagedQueue())).size(0) != 0 || ((service).*netServiceTestMember(NetServiceTestManagedQueue())).bytes() != 0)
    {
        throw std::runtime_error("service backpressure reordered a source or starved a healthy source");
    }
}

void NetSenderServiceQueueTest::reentrant()
{
    for (uint32_t mode = 1; mode <= 4; ++mode)
    {
        NetSenderLocalInbox inbox;
        NetSenderServiceManaged service(inbox);
        // 显式配置实际池，验证回调借用仍占槽，不把旧256/8计数门槛带回生产代码。
        NetBufferPoolConfig bodies;
        bodies.m_classes[4].maximumCapacity = 8;
        LockFreeHashMapPoolConfig records = NetSenderServiceQueue::defaultRecords();
        records.initialCapacity = records.maximumCapacity = 256;
        if (service.configureQueuePools(bodies, records) != NI_OK)
        {
            throw std::runtime_error("service reentrant physical pool fixture");
        }
        NetSenderServiceQueueTest observer;
        observer.m_mode = mode;
        observer.m_service = &service;
        ((service).*netServiceTestMember(NetServiceTestManagedListener())) = &observer;
        std::vector<uint8_t> payload(mode == 2 ? 1048576U : 1U, 0x59);
        NetInterfaceFrame frame = {};
        frame.m_kind = NI_EVENT;
        frame.m_productId = 101;
        frame.m_messageType = 700;
        frame.m_payload = payload.data();
        frame.m_payloadSize = static_cast<uint32_t>(payload.size());
        NetInterfaceOrigin origin = {};
        origin.m_endpoint = 41;
        const uint32_t count = mode >= 3 ? 2U : 1U;
        for (uint32_t index = 0; index < count; ++index)
        {
            if (((service).*netServiceTestMember(NetServiceTestManagedPush()))(0, 1, frame, origin) != NI_OK)
            {
                throw std::runtime_error("service reentrant fixture admission failed");
            }
        }
        bool caught = false;
        try
        {
            ((service).*netServiceTestMember(NetServiceTestManagedPump()))(0, NetSenderSubmission::now());
        }
        catch (...)
        {
            caught = true;
        }
        const uint32_t expected = mode == 1 ? 256U : mode == 2 ? 8U : mode == 3 ? 0U : 1U;
        if (observer.m_bad || caught != (mode == 4) || ((service).*netServiceTestMember(NetServiceTestManagedQueue())).size() != expected ||
            ((service).*netServiceTestMember(NetServiceTestManagedQueue())).size(0) != expected || ((service).*netServiceTestMember(NetServiceTestManagedQueue())).bytes() != static_cast<uint64_t>(expected) * payload.size() ||
            (mode <= 2 && observer.m_admitted != expected - 1) || observer.m_closedCount != (mode == 3 ? 1U : 0U))
        {
            throw std::runtime_error("service reentrant capacity, borrowed lifetime or exception cleanup failed");
        }
        netTestCheckpoint();
    }
}

void NetSenderServiceQueueTest::registration()
{
    uint32_t rejected = 0;
    bool completed = false;
    for (uint32_t step = 0; step < 64 && !completed; ++step)
    {
        NetSenderLocalInbox inbox;
        NetSenderServiceManaged service(inbox);
        NetSenderServiceQueueTest observer;
        ((service).*netServiceTestMember(NetServiceTestManagedListener())) = &observer;
        ((service).*netServiceTestMember(NetServiceTestManagedProduct())) = 101;
        if (((service).*netServiceTestMember(NetServiceTestManagedRouter())).addHandler(101, 128, ((service).*netServiceTestMember(NetServiceTestManagedHandler()))) != NI_OK)
        {
            throw std::runtime_error("service registration handler fixture failed");
        }
        NetInterfaceOrigin origin = {};
        origin.m_endpoint = 71;
        origin.m_platform = 1;
        origin.m_device[0] = 9;
        uint32_t fields[4] = {1, 7, 128, 65536};
        uint64_t endpoint = 99;
#if defined(_WIN32) && defined(_DEBUG) && defined(NET_SERVICE_MANAGED_HEAP)
        s_serviceManagedRejectAfter = static_cast<int32_t>(step);
        _CRT_ALLOC_HOOK previous = _CrtSetAllocHook(ServiceManagedMemoryHook);
#endif
        const int32_t result = ((service).*netServiceTestMember(NetServiceTestManagedAdd()))(11, NI_LINK_CLIENT, origin, fields, endpoint);
#if defined(_WIN32) && defined(_DEBUG) && defined(NET_SERVICE_MANAGED_HEAP)
        _CrtSetAllocHook(previous);
        s_serviceManagedRejectAfter = -1;
#endif
        if (result == NI_OK)
        {
            completed = true;
        }
        else
        {
            ++rejected;
            if (result != NI_NO_MEMORY || endpoint != 0 || !((service).*netServiceTestMember(NetServiceTestManagedPeers())).empty() || !((service).*netServiceTestMember(NetServiceTestManagedRoots())).empty())
            {
                throw std::runtime_error("service registration failed without atomic rollback");
            }
            ((service).*netServiceTestMember(NetServiceTestManagedRouter())).poll();
            if (((service).*netServiceTestMember(NetServiceTestManagedAdd()))(11, NI_LINK_CLIENT, origin, fields, endpoint) != NI_OK)
            {
                throw std::runtime_error("service registration leaked route or blocked retry after rollback");
            }
        }
        const uint64_t registered = endpoint;
        if (endpoint == 0 || ((service).*netServiceTestMember(NetServiceTestManagedPeers())).size() != 1 || ((service).*netServiceTestMember(NetServiceTestManagedRoots())).size() != 1 ||
            ((service).*netServiceTestMember(NetServiceTestManagedAttach()))(11, NI_LINK_CLIENT, origin, endpoint) != NI_OK || endpoint != registered)
        {
            throw std::runtime_error("service registration did not publish consistent route and indexes");
        }
        for (uint32_t mode = 0; mode < 4; ++mode)
        {
            NetInterfaceOrigin changed = origin;
            if (mode == 2)
            {
                changed.m_platform = 2;
            }
            if (mode == 3)
            {
                ++changed.m_device[0];
            }
            if (((service).*netServiceTestMember(NetServiceTestManagedAttach()))(mode == 0 ? 12 : 11, mode == 1 ? NI_LINK_SERVICE : NI_LINK_CLIENT,
                changed, endpoint) != NI_PROTOCOL_ERROR || endpoint != 0)
            {
                throw std::runtime_error("service reused an origin from a different authenticated channel or identity");
            }
        }
        for (uint32_t index = 0; index < 16; ++index)
        {
            origin.m_endpoint = 100 + index;
            if (((service).*netServiceTestMember(NetServiceTestManagedAdd()))(index % 2 == 0 ? 11 : 12, NI_LINK_CLIENT, origin, fields, endpoint) != NI_OK)
            {
                throw std::runtime_error("service close fixture failed");
            }
        }
#if defined(_WIN32) && defined(_DEBUG) && defined(NET_SERVICE_MANAGED_HEAP)
        s_serviceManagedRejectAfter = 0;
        s_serviceManagedAllocations = 0;
        previous = _CrtSetAllocHook(ServiceManagedMemoryHook);
#endif
        ((service).*netServiceTestMember(NetServiceTestManagedClientClosed()))(11, NI_TRUNCATED);
#if defined(_WIN32) && defined(_DEBUG) && defined(NET_SERVICE_MANAGED_HEAP)
        _CrtSetAllocHook(previous);
        s_serviceManagedRejectAfter = -1;
        if (s_serviceManagedAllocations != 0)
        {
            throw std::runtime_error("service channel cleanup attempted allocation");
        }
#endif
        if (((service).*netServiceTestMember(NetServiceTestManagedPeers())).size() != 8 || ((service).*netServiceTestMember(NetServiceTestManagedRoots())).size() != 8)
        {
            throw std::runtime_error("service channel cleanup affected an unrelated channel or retained closed roots");
        }
        ((service).*netServiceTestMember(NetServiceTestManagedClientClosed()))(12, NI_TRUNCATED);
        ((service).*netServiceTestMember(NetServiceTestManagedRouter())).poll();
        if (!((service).*netServiceTestMember(NetServiceTestManagedPeers())).empty() || !((service).*netServiceTestMember(NetServiceTestManagedRoots())).empty() || ((service).*netServiceTestMember(NetServiceTestManagedRouter())).pendingCount() != 0)
        {
            throw std::runtime_error("service close failed to release registration");
        }
        netTestCheckpoint();
    }
    if (!completed)
    {
        throw std::runtime_error("service registration never reached successful allocation");
    }
#if defined(_WIN32) && defined(_DEBUG) && defined(NET_SERVICE_MANAGED_HEAP)
    if (rejected < 4)
    {
        throw std::runtime_error("service registration fault sweep did not exercise all ownership layers");
    }
#endif
    std::printf("Service registration rejected=%u PASS\n", rejected);
}