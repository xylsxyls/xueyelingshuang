#include "NetSenderServiceGatewayHelper.h"
#include "CSystem/CSystemAPI.h"
#include <chrono>

int32_t NetSenderServiceGatewayHelper::serviceGatewayEntropy(void* context, uint8_t* output, uint32_t size)
{
    (void)context;
    return CSystem::secureRandomBytes(output, size) ? 0 : -1;
}

uint64_t NetSenderServiceGatewayHelper::serviceGatewayNow()
{
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}