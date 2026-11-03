#include "NetInterfaceMacro.h"
#include "NetInterface.h"
#include "NetInterfaceRequests.h"
#include <cstring>

/** 无精度损失组合宿主低/高字
@param [in] words 至少两项
@return uint64
*/
static uint64_t RequestPair(const uint32_t* words)
{
    return (static_cast<uint64_t>(words[1]) << 32) | words[0];
}

/** 输出固定记录，零记录同样填零
@param [in] request POD元数据
@param [out] output 十六项，不能空
*/
static void PackRequest(const NetInterfaceRequest& request, uint32_t* output)
{
    const uint64_t values[7] = {request.m_route,  request.m_source, request.m_original, request.m_target,
                                request.m_origin, request.m_parent, request.m_deadline};
    for (uint32_t index = 0; index < 7; ++index)
    {
        output[index * 2] = static_cast<uint32_t>(values[index]);
        output[index * 2 + 1] = static_cast<uint32_t>(values[index] >> 32);
    }
    output[14] = request.m_product;
    output[15] = request.m_type;
}

int32_t NetInterface::requestsCreate(uint32_t capacity, NetInterfaceRequestsHandle* handle)
{
    if (handle == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    *handle = nullptr;
    if (capacity == 0 || capacity > 4096)
    {
        return NI_INVALID_ARGUMENT;
    }
    try
    {
        *handle = reinterpret_cast<NetInterfaceRequestsHandle>(new NetInterfaceRequests(capacity));
        return NI_OK;
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

void NetInterface::requestsDestroy(NetInterfaceRequestsHandle handle)
{
    delete reinterpret_cast<NetInterfaceRequests*>(handle);
}

int32_t NetInterface::requestsAddEndpoint(NetInterfaceRequestsHandle handle, uint32_t low, uint32_t high, uint32_t service, uint32_t product,
                                        uint32_t limit, uint32_t internalOnly)
{
    if (handle == nullptr || service > 1 || internalOnly > 1)
    {
        return NI_INVALID_ARGUMENT;
    }
    return reinterpret_cast<NetInterfaceRequests*>(handle)->addEndpoint((static_cast<uint64_t>(high) << 32) | low, service != 0, product,
                                                                        limit, internalOnly != 0);
}

int32_t NetInterface::requestsRemoveEndpoint(NetInterfaceRequestsHandle handle, uint32_t low, uint32_t high)
{
    return handle == nullptr ? NI_INVALID_ARGUMENT
                             : reinterpret_cast<NetInterfaceRequests*>(handle)->removeEndpoint((static_cast<uint64_t>(high) << 32) | low);
}

int32_t NetInterface::requestsBegin(NetInterfaceRequestsHandle handle, const uint32_t* input, uint32_t inputCount, uint32_t autoId,
                                  uint32_t* output)
{
    uint32_t snapshot[13] = {};
    if (input != nullptr && inputCount == 13)
    {
        std::memcpy(snapshot, input, sizeof(snapshot));
        input = snapshot;
    }
    if (output != nullptr)
    {
        std::memset(output, 0, 16 * sizeof(uint32_t));
    }
    if (handle == nullptr || input == nullptr || output == nullptr || inputCount != 13 || autoId > 1 ||
        (autoId != 0 && RequestPair(input + 2) != 0))
    {
        return NI_INVALID_ARGUMENT;
    }
    NetInterfaceRequest request = {};
    NetInterfaceRequests* requests = reinterpret_cast<NetInterfaceRequests*>(handle);
    const int32_t result = autoId ? requests->beginNext(RequestPair(input), RequestPair(input + 4), input[6], input[7],
                                                        RequestPair(input + 8), RequestPair(input + 10), input[12], request)
                                  : requests->begin(RequestPair(input), RequestPair(input + 2), RequestPair(input + 4), input[6], input[7],
                                                    RequestPair(input + 8), RequestPair(input + 10), input[12], request);
    PackRequest(request, output);
    return result;
}

int32_t NetInterface::requestsComplete(NetInterfaceRequestsHandle handle, const uint32_t* input, uint32_t inputCount, uint32_t* output)
{
    uint32_t snapshot[10] = {};
    if (input != nullptr && (inputCount == 8 || inputCount == 10))
    {
        std::memcpy(snapshot, input, inputCount * sizeof(uint32_t));
        input = snapshot;
    }
    if (output != nullptr)
    {
        std::memset(output, 0, 16 * sizeof(uint32_t));
    }
    if (handle == nullptr || input == nullptr || output == nullptr || (inputCount != 8 && inputCount != 10))
    {
        return NI_INVALID_ARGUMENT;
    }
    NetInterfaceRequest request = {};
    NetInterfaceRequests* requests = reinterpret_cast<NetInterfaceRequests*>(handle);
    const int32_t result = inputCount == 8 ? requests->complete(RequestPair(input), RequestPair(input + 2), input[4], input[5],
                                                                RequestPair(input + 6), request)
                                           : requests->completeOriginal(RequestPair(input), RequestPair(input + 2), RequestPair(input + 4),
                                                                        input[6], input[7], RequestPair(input + 8), request);
    PackRequest(request, output);
    return result;
}

int32_t NetInterface::requestsCancel(NetInterfaceRequestsHandle handle, uint32_t sourceLow, uint32_t sourceHigh, uint32_t originalLow,
                                   uint32_t originalHigh, uint32_t* output)
{
    if (output != nullptr)
    {
        std::memset(output, 0, 16 * sizeof(uint32_t));
    }
    if (handle == nullptr || output == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    NetInterfaceRequest request = {};
    const int32_t result = reinterpret_cast<NetInterfaceRequests*>(handle)->cancel(
        (static_cast<uint64_t>(sourceHigh) << 32) | sourceLow, (static_cast<uint64_t>(originalHigh) << 32) | originalLow, request);
    PackRequest(request, output);
    return result;
}

int32_t NetInterface::requestsTake(NetInterfaceRequestsHandle handle, uint32_t operation, uint32_t keyLow, uint32_t keyHigh, uint32_t* output)
{
    if (output != nullptr)
    {
        std::memset(output, 0, 16 * sizeof(uint32_t));
    }
    if (handle == nullptr || output == nullptr || operation > 2)
    {
        return NI_INVALID_ARGUMENT;
    }
    NetInterfaceRequest request = {};
    NetInterfaceRequests* requests = reinterpret_cast<NetInterfaceRequests*>(handle);
    const uint64_t key = (static_cast<uint64_t>(keyHigh) << 32) | keyLow;
    const int32_t result = operation == 0 ? requests->takeExpired(key, request)
                                          : (operation == 1 ? requests->takeEndpoint(key, request) : requests->takeChild(key, request));
    PackRequest(request, output);
    return result;
}

int32_t NetInterface::requestsCount(NetInterfaceRequestsHandle handle)
{
    return handle == nullptr ? NI_INVALID_ARGUMENT : static_cast<int32_t>(reinterpret_cast<NetInterfaceRequests*>(handle)->count());
}