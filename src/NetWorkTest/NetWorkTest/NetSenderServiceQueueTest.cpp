#include "NetSender/NetSenderAPI.h"
#include <LibuvTcp/LibuvTcpAPI.h>
#include "NetWorkConcurrencyTestCases.h"
#include <NetWork/NetWorkAPI.h>
#include <NetSender/NetSenderAPI.h>
#include <test/certs.h>
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
                const int32_t result = m_service->push(0, 1, frame, origin);
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
            m_service->onGatewayServiceClosed(NI_CANCELLED);
            m_bad = m_bad || m_service->m_queue.size() != 1 || m_service->m_queue.bytes() != size;
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

void NetSenderServiceQueueTest::run()
{
    NetSenderLocalInbox inbox;
    NetSenderServiceManaged service(inbox);
    NetSenderServiceQueueTest observer;
    service.m_listener = &observer;
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
        if (service.push(0, 1, frame, origin) != NI_OK)
        {
            throw std::runtime_error("service FIFO fixture admission failed");
        }
    }
    service.pumpQueue(0, NetSenderSubmission::now());
    const uint32_t healthyDuringPressure = observer.m_healthy;
    observer.m_allow = true;
    for (uint32_t round = 0; round < 8 && service.m_queue.size(0) != 0; ++round)
    {
        service.pumpQueue(0, NetSenderSubmission::now());
    }
    bool ordered = observer.m_seen.size() == 100;
    for (uint32_t index = 0; ordered && index < observer.m_seen.size(); ++index) { ordered = observer.m_seen[index] == index; }
    std::printf("SERVICE_QUEUE_FIFO healthy_while_blocked=%u first_after_resume=%u count=%u ordered=%u\n",
        healthyDuringPressure, observer.m_seen.empty() ? 999U : observer.m_seen[0],
        static_cast<uint32_t>(observer.m_seen.size()), ordered ? 1U : 0U);
    if (healthyDuringPressure != 1 || !ordered || observer.m_bad || service.m_queue.size(0) != 0 || service.m_queue.bytes() != 0)
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
        NetSenderServiceQueueTest observer;
        observer.m_mode = mode;
        observer.m_service = &service;
        service.m_listener = &observer;
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
            if (service.push(0, 1, frame, origin) != NI_OK)
            {
                throw std::runtime_error("service reentrant fixture admission failed");
            }
        }
        bool caught = false;
        try
        {
            service.pumpQueue(0, NetSenderSubmission::now());
        }
        catch (...)
        {
            caught = true;
        }
        const uint32_t expected = mode == 1 ? 256U : mode == 2 ? 8U : mode == 3 ? 0U : 1U;
        if (observer.m_bad || caught != (mode == 4) || service.m_queue.size() != expected ||
            service.m_queue.size(0) != expected || service.m_queue.bytes() != static_cast<uint64_t>(expected) * payload.size() ||
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
        service.m_listener = &observer;
        service.m_product = 101;
        if (service.m_router.addHandler(101, 128, service.m_handler) != NI_OK)
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
        const int32_t result = service.addPeer(11, NI_LINK_CLIENT, origin, fields, endpoint);
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
            if (result != NI_NO_MEMORY || endpoint != 0 || !service.m_peers.empty() || !service.m_roots.empty())
            {
                throw std::runtime_error("service registration failed without atomic rollback");
            }
            service.m_router.poll();
            if (service.addPeer(11, NI_LINK_CLIENT, origin, fields, endpoint) != NI_OK)
            {
                throw std::runtime_error("service registration leaked route or blocked retry after rollback");
            }
        }
        const uint64_t registered = endpoint;
        if (endpoint == 0 || service.m_peers.size() != 1 || service.m_roots.size() != 1 ||
            service.attach(11, NI_LINK_CLIENT, origin, endpoint) != NI_OK || endpoint != registered)
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
            if (service.attach(mode == 0 ? 12 : 11, mode == 1 ? NI_LINK_SERVICE : NI_LINK_CLIENT,
                changed, endpoint) != NI_PROTOCOL_ERROR || endpoint != 0)
            {
                throw std::runtime_error("service reused an origin from a different authenticated channel or identity");
            }
        }
        for (uint32_t index = 0; index < 16; ++index)
        {
            origin.m_endpoint = 100 + index;
            if (service.addPeer(index % 2 == 0 ? 11 : 12, NI_LINK_CLIENT, origin, fields, endpoint) != NI_OK)
            {
                throw std::runtime_error("service close fixture failed");
            }
        }
#if defined(_WIN32) && defined(_DEBUG) && defined(NET_SERVICE_MANAGED_HEAP)
        s_serviceManagedRejectAfter = 0;
        s_serviceManagedAllocations = 0;
        previous = _CrtSetAllocHook(ServiceManagedMemoryHook);
#endif
        service.onGatewayClientClosed(11, NI_TRUNCATED);
#if defined(_WIN32) && defined(_DEBUG) && defined(NET_SERVICE_MANAGED_HEAP)
        _CrtSetAllocHook(previous);
        s_serviceManagedRejectAfter = -1;
        if (s_serviceManagedAllocations != 0)
        {
            throw std::runtime_error("service channel cleanup attempted allocation");
        }
#endif
        if (service.m_peers.size() != 8 || service.m_roots.size() != 8)
        {
            throw std::runtime_error("service channel cleanup affected an unrelated channel or retained closed roots");
        }
        service.onGatewayClientClosed(12, NI_TRUNCATED);
        service.m_router.poll();
        if (!service.m_peers.empty() || !service.m_roots.empty() || service.m_router.pendingCount() != 0)
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