#include "NetSenderHostC.h"
#include "NetSender.h"

uint32_t netPeerPointerBytes(void)
{
    return NetSender::pointerBytes();
}

int32_t netPeerConfigureHost(const NetHostFunctions* functions)
{
    return functions == nullptr ? NI_INVALID_ARGUMENT : NetSender::backend().configureHost(*functions);
}

int32_t netPeerRandomBytes(uint8_t* output, uint32_t size)
{
    return NetSender::randomBytes(output, size);
}