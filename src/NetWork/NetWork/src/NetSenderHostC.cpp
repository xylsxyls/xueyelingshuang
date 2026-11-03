#include "NetSenderHostC.h"
#include "NetSender.h"

uint32_t netWorkPointerBytes(void)
{
    return NetSender::pointerBytes();
}

int32_t netWorkConfigureHost(const NetHostFunctions* functions)
{
    return functions == nullptr ? NI_INVALID_ARGUMENT : NetSender::backend().configureHost(*functions);
}

int32_t netWorkRandomBytes(uint8_t* output, uint32_t size)
{
    return NetSender::randomBytes(output, size);
}

int32_t netWorkConfigureNativeFileIo(void)
{
    return NetSender::backend().configureNativeFileIo();
}