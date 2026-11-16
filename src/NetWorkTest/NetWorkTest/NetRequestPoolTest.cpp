#include "NetRequestPoolTest.h"
#include "NetInterface/NetInterfaceAPI.h"
#include "NetInterface/NetSessionState.h"
#include "NetInterface/NetSenderDispatchMap.h"
#include <stdexcept>

/** 核对请求池与索引的一致性
@param [in] condition 预期条件
@param [in] detail 失败信息
*/
static void RequestPoolCheck(bool condition, const char* detail)
{
    if (!condition)
    {
        throw std::runtime_error(detail);
    }
}

void NetRequestPoolTest::run()
{
    NetSessionState state;
    state.resetRequests(1);
    for (uint64_t index = 1; index <= 2048; ++index)
    {
        uint64_t id = 0;
        RequestPoolCheck(state.reserve(index, 101, 21, 5000 - index, id) == NI_OK && id == index,
            "Protocol request pool retained handshake/count window");
    }
    RequestPoolCheck(state.m_pending.size() == 2048 && state.m_expiryCount == 2048 &&
        state.expired(2951) == 0 && state.expired(2952) == 2048, "Growing deadline heap ordering");
    NetSessionPending removed = {};
    for (uint64_t index = 1; index <= 2048; index += 2)
    {
        RequestPoolCheck(state.take(index, removed) && removed.m_request.m_original == index &&
            !state.take(index, removed), "Deadline heap arbitrary or repeated removal");
    }
    for (uint64_t index = 2048; index != 0; index -= 2)
    {
        RequestPoolCheck(state.expired(5000) == index && state.take(index, removed), "Deadline heap minimum after arbitrary removal");
    }
    RequestPoolCheck(state.m_pending.empty() && state.m_expiry.empty() && state.expired(5000) == 0 &&
        state.m_records->statistics().occupied == 0, "Protocol record leak after final completion");

    LockFreeHashMapPoolConfig config;
    config.initialCapacity = 2;
    config.maximumCapacity = 2;
    config.lookupCacheCapacity = 0;
    state.m_records.reset(new NetObjectPool<NetSessionPending>(config));
    state.resetRequests(UINT32_MAX);
    uint64_t first = 0, second = 0, third = 1;
    RequestPoolCheck(state.reserve(0, 101, 21, 5000, first) == NI_OK && state.reserve(0, 101, 21, 5000, second) == NI_OK &&
        state.reserve(0, 101, 21, 5000, third) == NI_LIMIT_EXCEEDED && third == 0 &&
        state.m_expiryCount == 2 && state.m_pending.size() == 2, "Protocol physical pool exhaustion rollback");
    RequestPoolCheck(state.take(first, removed) && state.reserve(0, 101, 21, 5000, third) == NI_OK && third > second,
        "Protocol pool reuse or identity reuse");
    state.resetRequests(1);
    RequestPoolCheck(state.m_records->statistics().occupied == 0, "Protocol reset retained live requests");

    NetSenderDispatchMap mapping;
    for (uint64_t index = 1; index <= 4096; ++index)
    {
        RequestPoolCheck(mapping.insert(index, 5000 - index) == NI_OK, "Dispatch mapping retained fixed128 limit");
    }
    RequestPoolCheck(mapping.insert(1, 9000) == NI_INVALID_ARGUMENT && mapping.insert(9000, 4999) == NI_INVALID_ARGUMENT && mapping.insert(0, 9000) == NI_INVALID_ARGUMENT &&
        mapping.insert(9000, 0) == NI_INVALID_ARGUMENT && mapping.size() == 4096, "Dispatch duplicate/invalid partial insertion");
    for (uint64_t index = 4096; index != 0; --index)
    {
        RequestPoolCheck(mapping.local(index) == 5000 - index && mapping.wire(5000 - index) == index,
            "Dispatch bidirectional identity");
        mapping.erase(index);
        mapping.erase(index);
        RequestPoolCheck(mapping.local(index) == 0 && mapping.wire(5000 - index) == 0 && mapping.size() == index - 1,
            "Dispatch erase left a reverse mapping");
    }
    RequestPoolCheck(mapping.insert(1, 1) == NI_OK, "Dispatch reuse after exhaustion");
    mapping.clear();
    RequestPoolCheck(mapping.size() == 0 && mapping.local(1) == 0 && mapping.wire(1) == 0,
        "Dispatch clear retained identity");
}