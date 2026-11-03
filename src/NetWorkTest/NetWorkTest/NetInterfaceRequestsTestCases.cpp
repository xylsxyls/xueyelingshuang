#include "NetInterfaceRequestsTestCases.h"
#include "NetWorkTestCases.h"
#include "NetInterface/NetInterfaceAPI.h"
#include <stdexcept>
#include <cstring>
#include <vector>

/** 检查独立路由预期，同时遵守统一取消/时限
@param [in] expected 预期条件
@param [in] reason 失败原因
*/
static void CheckRequest(bool expected, const char* reason)
{
    netTestCheckpoint();
    if (!expected)
    {
        throw std::runtime_error(reason);
    }
}

/** 登记两个外部端点、Cook以及只允许内部调用的Account
@param [in,out] requests 新等待表
@param [in] limit 每端点限制
*/
static void RegisterRequestPeers(NetInterfaceRequests& requests, uint32_t limit = 32)
{
    CheckRequest(requests.addEndpoint(1, false, 0, limit) == NI_OK &&
        requests.addEndpoint(2, true, 4, limit) == NI_OK &&
        requests.addEndpoint(3, true, 0, limit, true) == NI_OK &&
        requests.addEndpoint(4, false, 0, limit) == NI_OK, "register request peer generations");
}

void NetInterfaceRequestsTestCases::nestedRouting()
{
    NetInterfaceRequests requests;
    RegisterRequestPeers(requests);
    NetInterfaceRequest outer = {};
    NetInterfaceRequest inner = {};
    NetInterfaceRequest result = {};
    const uint64_t original = UINT64_C(0xFEDCBA9876543210);
    CheckRequest(requests.begin(1, original, 2, 4, 700, 0, 1000, 1000, outer) == NI_OK,
        "external request A");
    CheckRequest(requests.begin(2, 1, 3, 4, 900, outer.m_route, 1000, 500, inner) == NI_OK &&
        inner.m_route != outer.m_route && inner.m_origin == 1 && inner.m_parent == outer.m_route,
        "independent internal request B");
    CheckRequest(requests.complete(1, inner.m_route, 4, 900, 1000, result) == NI_PROTOCOL_ERROR &&
        result.m_route == 0 && requests.count() == 2, "external caller spoofed Account response");
    CheckRequest(requests.begin(4, 1, 3, 4, 900, outer.m_route, 1000, 100, result) == NI_PROTOCOL_ERROR &&
        requests.begin(1, original + 1, 3, 4, 900, 0, 1000, 100, result) == NI_PROTOCOL_ERROR,
        "external caller entered internal-only service");
    CheckRequest(requests.begin(2, 2, 3, 5, 900, outer.m_route, 1000, 100, result) == NI_PROTOCOL_ERROR,
        "internal query changed parent product");
    CheckRequest(requests.begin(3, 1, 2, 4, 900, inner.m_route, 1000, 100, result) == NI_PROTOCOL_ERROR,
        "unbounded internal recursion");
    CheckRequest(requests.complete(3, inner.m_route, 4, 900, 1000, result) == NI_READY &&
        result.m_source == 2 && result.m_original == 1 && result.m_parent == outer.m_route && requests.count() == 1,
        "Account response did not return to Cook request B");
    CheckRequest(requests.complete(2, outer.m_route, 4, 700, 1000, result) == NI_READY &&
        result.m_source == 1 && result.m_original == original && requests.count() == 0,
        "Cook response did not return to original full uint64 request A");
}

void NetInterfaceRequestsTestCases::budgets()
{
    NetInterfaceRequests requests(2);
    RegisterRequestPeers(requests, 2);
    NetInterfaceRequest first = {};
    NetInterfaceRequest second = {};
    NetInterfaceRequest result = {};
    CheckRequest(requests.begin(1, 1, 2, 4, 700, 0, 1000, 100, first) == NI_OK &&
        requests.begin(1, 2, 2, 4, 700, 0, 1000, 100, second) == NI_OK,
        "two requests admitted within budgets");
    CheckRequest(requests.begin(1, 3, 2, 4, 700, 0, 1000, 100, result) == NI_LIMIT_EXCEEDED &&
        requests.count() == 2 && result.m_route == 0, "full request budget");
    CheckRequest(requests.complete(2, first.m_route, 5, 700, 1000, result) == NI_PROTOCOL_ERROR &&
        requests.complete(2, first.m_route, 4, 701, 1000, result) == NI_PROTOCOL_ERROR && requests.count() == 2,
        "response changed product or operation");
    CheckRequest(requests.setEndpointLimit(1, 1) == NI_INVALID_ARGUMENT &&
        requests.setEndpointLimit(99, 1) == NI_NOT_FOUND &&
        requests.setEndpointLimit(1, 0) == NI_INVALID_ARGUMENT, "invalid negotiated endpoint limit");
    CheckRequest(requests.complete(2, first.m_route, 4, 700, 1000, result) == NI_READY &&
        requests.complete(2, first.m_route, 4, 700, 1000, result) == NI_NOT_FOUND, "response completed twice");
    CheckRequest(requests.begin(1, 3, 2, 4, 700, 0, 1000, 100, result) == NI_PROTOCOL_ERROR &&
        requests.begin(1, 4, 2, 4, 700, 0, 1000, 100, result) == NI_OK &&
        result.m_route != first.m_route, "reused source or gateway request ID");
    CheckRequest(requests.setEndpointLimit(1, 2) == NI_OK, "valid negotiated limit update");
    NetInterfaceRequests targetBound(16);
    RegisterRequestPeers(targetBound, 1);
    CheckRequest(targetBound.begin(1, 1, 2, 4, 700, 0, 1000, 100, first) == NI_OK &&
        targetBound.begin(4, 1, 2, 4, 700, 0, 1000, 100, second) == NI_LIMIT_EXCEEDED,
        "target service budget not enforced across clients");
}

void NetInterfaceRequestsTestCases::deadlines()
{
    NetInterfaceRequests requests;
    RegisterRequestPeers(requests);
    NetInterfaceRequest outer = {};
    NetInterfaceRequest inner = {};
    NetInterfaceRequest result = {};
    CheckRequest(requests.begin(1, 1, 2, 4, 700, 0, 1000, 100, outer) == NI_OK &&
        requests.begin(2, 1, 3, 4, 900, outer.m_route, 1050, 1000, inner) == NI_OK && inner.m_deadline == 1100,
        "child deadline exceeded parent");
    CheckRequest(requests.takeExpired(1099, result) == NI_NOT_FOUND, "request expired early");
    CheckRequest(requests.complete(2, outer.m_route, 4, 700, 1100, result) == NI_TIMEOUT &&
        result.m_route == outer.m_route, "late reply beat expiry cleanup");
    CheckRequest(requests.takeExpired(1100, result) == NI_READY && result.m_route == inner.m_route &&
        requests.takeExpired(1100, result) == NI_NOT_FOUND, "child fixed deadline cleanup");
    CheckRequest(requests.begin(1, 2, 2, 4, 700, 0, 1200, 1000, outer) == NI_OK &&
        requests.begin(2, 2, 3, 4, 900, outer.m_route, 1200, 500, inner) == NI_OK &&
        requests.complete(2, outer.m_route, 4, 700, 1201, result) == NI_READY &&
        requests.complete(3, inner.m_route, 4, 900, 1202, result) == NI_CANCELLED,
        "orphaned Account response returned to a finished parent");
}

void NetInterfaceRequestsTestCases::cancellationOrder()
{
    NetInterfaceRequests requests(2);
    RegisterRequestPeers(requests, 2);
    NetInterfaceRequest first = {};
    NetInterfaceRequest second = {};
    NetInterfaceRequest result = {};
    CheckRequest(requests.cancel(1, 3, result) == NI_OK && result.m_route == 0, "remember future cancellation");
    CheckRequest(requests.begin(1, 1, 2, 4, 700, 0, 1000, 100, first) == NI_OK &&
        requests.begin(1, 2, 2, 4, 700, 0, 1000, 100, second) == NI_OK &&
        requests.begin(1, 3, 2, 4, 700, 0, 1000, 100, result) == NI_CANCELLED && requests.count() == 2,
        "priority cancellation skipped earlier legitimate requests or allowed cancelled execution");
    CheckRequest(requests.cancel(1, 1, result) == NI_READY && result.m_route == first.m_route &&
        requests.cancel(1, 1, result) == NI_NOT_FOUND &&
        requests.complete(2, first.m_route, 4, 700, 1000, result) == NI_NOT_FOUND, "cancelled request completed");
    CheckRequest(requests.cancel(1, 4, result) == NI_OK && requests.cancel(1, 4, result) == NI_OK &&
        requests.cancel(4, 1, result) == NI_OK && requests.cancel(4, 2, result) == NI_LIMIT_EXCEEDED,
        "future cancellation global budget");
    CheckRequest(requests.begin(1, 4, 2, 4, 700, 0, 1000, 100, result) == NI_CANCELLED &&
        requests.cancel(4, 2, result) == NI_OK, "future cancellation did not release budget");
    CheckRequest(requests.begin(1, 1, 2, 4, 700, 0, 1000, 100, result) == NI_PROTOCOL_ERROR,
        "expired tombstone let delayed old request execute");
}

void NetInterfaceRequestsTestCases::endpointGeneration()
{
    NetInterfaceRequests requests;
    RegisterRequestPeers(requests);
    NetInterfaceRequest outer = {};
    NetInterfaceRequest inner = {};
    NetInterfaceRequest other = {};
    NetInterfaceRequest result = {};
    CheckRequest(requests.begin(1, 1, 2, 4, 700, 0, 1000, 100, outer) == NI_OK &&
        requests.begin(2, 1, 3, 4, 900, outer.m_route, 1000, 100, inner) == NI_OK &&
        requests.begin(4, 1, 2, 4, 700, 0, 1000, 100, other) == NI_OK, "generation requests");
    CheckRequest(requests.removeEndpoint(1) == NI_INVALID_STATE, "removed endpoint with owned work");
    uint32_t removed = 0;
    while (requests.takeEndpoint(1, result) == NI_READY)
    {
        CheckRequest(result.m_origin == 1 && ++removed <= 2, "disconnected another client");
    }
    CheckRequest(removed == 2 && requests.count() == 1 && requests.removeEndpoint(1) == NI_OK &&
        requests.addEndpoint(1, false, 0, 32) == NI_INVALID_ARGUMENT, "endpoint generation reused");
    CheckRequest(requests.addEndpoint(5, false, 0, 32) == NI_OK &&
        requests.complete(3, inner.m_route, 4, 900, 1000, result) == NI_NOT_FOUND &&
        requests.complete(2, other.m_route, 4, 700, 1000, result) == NI_READY && result.m_source == 4,
        "late reply reached new client or disturbed another client");
    CheckRequest(requests.begin(5, 1, 2, 4, 700, 0, 1000, 100, outer) == NI_OK &&
        requests.begin(2, 2, 3, 4, 900, outer.m_route, 1000, 100, inner) == NI_OK &&
        requests.cancel(5, 1, result) == NI_READY &&
        requests.takeChild(outer.m_route, result) == NI_READY && result.m_route == inner.m_route &&
        requests.takeChild(outer.m_route, result) == NI_NOT_FOUND, "parent cancel child cleanup");
}

void NetInterfaceRequestsTestCases::argumentBounds()
{
    NetInterfaceRequests invalid(4097);
    CheckRequest(invalid.addEndpoint(1, false, 0, 32) == NI_INVALID_ARGUMENT, "invalid global capacity");
    NetInterfaceRequests requests(1);
    CheckRequest(requests.addEndpoint(1, false, 1, 32) == NI_INVALID_ARGUMENT &&
        requests.addEndpoint(1, false, 0, 32, true) == NI_INVALID_ARGUMENT &&
        requests.addEndpoint(1, true, 0, 129) == NI_INVALID_ARGUMENT, "invalid endpoint declaration");
    for (uint64_t id = 1; id <= 1024; ++id)
    {
        CheckRequest(requests.addEndpoint(id, true, 0, 1) == NI_OK, "endpoint budget below maximum");
    }
    CheckRequest(requests.addEndpoint(1025, true, 0, 1) == NI_LIMIT_EXCEEDED &&
        requests.removeEndpoint(1024) == NI_OK && requests.addEndpoint(1025, true, 0, 1) == NI_OK,
        "endpoint bound or recovery");
    NetInterfaceRequest value = {};
    CheckRequest(requests.begin(1, 1, 2, 4, 1, 0, 1000, 60001, value) == NI_INVALID_ARGUMENT &&
        requests.begin(1, 1, 2, 4, 1, 0, 1000, 100, value) == NI_OK,
        "invalid deadline admitted or poisoned valid ID");
    CheckRequest(requests.takeExpired(999, value) == NI_INVALID_ARGUMENT &&
        requests.complete(2, 1, 4, 1, 1000, value) == NI_INVALID_ARGUMENT,
        "clock rollback recovered");
    CheckRequest(requests.takeEndpoint(1, value) == NI_READY && requests.count() == 0,
        "clock failure prevented resource cleanup");
}

void NetInterfaceRequestsTestCases::cBoundary()
{
    NetInterfaceRequestsHandle handle = nullptr;
    CheckRequest(NetInterface::requestsCreate(0, &handle) == NI_INVALID_ARGUMENT && handle == nullptr &&
        NetInterface::requestsCreate(1, &handle) == NI_OK && handle != nullptr, "C request allocation");
    try
    {
        CheckRequest(NetInterface::requestsAddEndpoint(handle, 1, 0, 0, 0, 2, 0) == NI_OK &&
            NetInterface::requestsAddEndpoint(handle, 2, 0, 1, 0, 2, 0) == NI_OK, "C request endpoint registration");
        uint32_t input[13] = { 1, 0, 0, 0, 2, 0, 4, 700, 0, 0, 1000, 0, 10 };
        uint32_t output[16] = {};
        CheckRequest(NetInterface::requestsBegin(handle, input, 13, 1, output) == NI_OK &&
            output[0] == 1 && output[4] == 1 && output[12] == 1010, "C auto request ID");
        CheckRequest(NetInterface::requestsBegin(handle, input, 13, 1, output) == NI_LIMIT_EXCEEDED &&
            output[0] == 0 && NetInterface::requestsCount(handle) == 1, "C full request budget");
        uint32_t completion[10] = { 1, 0, 1, 0, 2, 0, 4, 700, 1000, 0 };
        CheckRequest(NetInterface::requestsComplete(handle, completion, 10, output) == NI_READY &&
            output[4] == 1, "C complete original request");
        CheckRequest(NetInterface::requestsBegin(handle, input, 13, 1, output) == NI_OK &&
            output[0] == 2 && output[4] == 3, "rejected input must not confuse original and route IDs");
        completion[2] = 3;
        std::memcpy(output, completion, sizeof(completion));
        CheckRequest(NetInterface::requestsComplete(handle, output, 10, output) == NI_READY &&
            output[0] == 2 && output[4] == 3, "aliased C completion or original correlation");
        CheckRequest(NetInterface::requestsBegin(handle, input, 13, 1, output) == NI_OK &&
            NetInterface::requestsTake(handle, 0, 1010, 0, output) == NI_READY && output[4] == 4 &&
            NetInterface::requestsCount(handle) == 0, "C expiry delivery");
        input[2] = input[3] = UINT32_MAX;
        input[10] = 1011;
        CheckRequest(NetInterface::requestsBegin(handle, input, 13, 0, output) == NI_OK &&
            output[4] == UINT32_MAX && output[5] == UINT32_MAX, "C uint64 request retained");
        input[2] = input[3] = 0;
        CheckRequest(NetInterface::requestsBegin(handle, input, 13, 1, output) == NI_LIMIT_EXCEEDED,
            "C auto ID wrapped after uint64 maximum");
        CheckRequest(NetInterface::requestsCancel(handle, 1, 0, UINT32_MAX, UINT32_MAX, output) == NI_READY &&
            output[4] == UINT32_MAX && output[5] == UINT32_MAX && NetInterface::requestsCount(handle) == 0,
            "C exact uint64 cancellation");
        CheckRequest(NetInterface::requestsRemoveEndpoint(handle, 1, 0) == NI_OK &&
            NetInterface::requestsTake(nullptr, 0, 1012, 0, output) == NI_INVALID_ARGUMENT && output[4] == 0,
            "C cleanup and failed output clearing");
    }
    catch (...)
    {
        NetInterface::requestsDestroy(handle);
        throw;
    }
    NetInterface::requestsDestroy(handle);
    NetInterface::requestsDestroy(nullptr);
}

void NetInterfaceRequestsTestCases::indexedLifetime()
{
    NetInterfaceRequests requests(4096);
    NetInterfaceRequest actual = {};
    std::vector<NetInterfaceRequest> reference(4096);
    for (uint64_t endpoint = 1; endpoint <= 64; ++endpoint)
    {
        CheckRequest(requests.addEndpoint(endpoint, endpoint > 32, endpoint > 32 ? 4 : 0, 128) == NI_OK,
            "indexed request endpoint setup");
    }
    for (uint32_t index = 0; index < 4096; ++index)
    {
        const uint64_t source = index / 128 + 1;
        const uint32_t timeout = (index * 31337U) % 1000 + 1;
        CheckRequest(requests.begin(source, index % 128 + 1, source + 32, 4, index, 0, 1000, timeout, reference[index]) == NI_OK,
            "indexed request full table admission");
    }
    // 不按插入顺序移除；同一节点同时在来源、根端点集合中，只能删除一次。
    for (uint32_t index = 0; index < 4096; index += 3)
    {
        CheckRequest(requests.cancel(reference[index].m_source, reference[index].m_original, actual) == NI_READY &&
            actual.m_route == reference[index].m_route, "indexed request arbitrary cancellation");
        reference[index].m_route = 0;
    }
    uint32_t disconnected = 0;
    while (requests.takeEndpoint(33, actual) == NI_READY)
    {
        CheckRequest(actual.m_source == 1 && actual.m_target == 33 && actual.m_origin == 1 && ++disconnected <= 128,
            "indexed request disconnect touched another service");
        CheckRequest(actual.m_type < reference.size() && reference[actual.m_type].m_route == actual.m_route,
            "indexed request disconnected an already completed request");
        reference[actual.m_type].m_route = 0;
    }
    CheckRequest(disconnected == 85 && requests.removeEndpoint(33) == NI_OK && requests.removeEndpoint(1) == NI_OK,
        "indexed request disconnect did not converge");
    // 独立线性模型选最早截止及同期限最小路由，覆盖堆上浮/下沉和并列期限。
    uint64_t nowMs = 1000;
    uint32_t remaining = 4096 - 1366 - 85;
    while (remaining != 0)
    {
        netTestCheckpoint();
        size_t selected = reference.size();
        for (size_t index = 0; index < reference.size(); ++index)
        {
            const NetInterfaceRequest& value = reference[index];
            if (value.m_route != 0 && (selected == reference.size() || value.m_deadline < reference[selected].m_deadline ||
                (value.m_deadline == reference[selected].m_deadline && value.m_route < reference[selected].m_route)))
            {
                selected = index;
            }
        }
        CheckRequest(selected != reference.size(), "indexed request reference lost a live entry");
        const NetInterfaceRequest expected = reference[selected];
        if (expected.m_deadline > nowMs)
        {
            CheckRequest(requests.takeExpired(expected.m_deadline - 1, actual) == NI_NOT_FOUND,
                "indexed request expired before fixed deadline");
        }
        nowMs = expected.m_deadline;
        CheckRequest(requests.takeExpired(nowMs, actual) == NI_READY && actual.m_route == expected.m_route &&
            actual.m_original == expected.m_original && actual.m_source == expected.m_source && actual.m_type == expected.m_type,
            "indexed request deadline ordering or identity");
        reference[selected].m_route = 0;
        CheckRequest(requests.count() == --remaining, "indexed request quota mismatch");
    }
    CheckRequest(requests.takeExpired(2000, actual) == NI_NOT_FOUND && requests.count() == 0,
        "indexed request retained completed metadata");
    for (uint64_t endpoint = 2; endpoint <= 64; ++endpoint)
    {
        if (endpoint != 33)
        {
            CheckRequest(requests.removeEndpoint(endpoint) == NI_OK, "indexed request endpoint index not empty");
        }
    }
}