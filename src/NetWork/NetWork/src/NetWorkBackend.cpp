#include "NetWorkBackend.h"
#include "NetWork.h"

int32_t NetWorkBackend::configureHost(const NetHostFunctions& functions)
{
    return NetWork::configureHost(functions);
}

int32_t NetWorkBackend::configureGateways(NetWorkSenderState* client, const char* seeds, const char* cachePath)
{
    return NetWork::instance().configureGateways(client, seeds, cachePath);
}

int32_t NetWorkBackend::configureNativeFileIo()
{
    return NetWork::configureNativeFileIo();
}