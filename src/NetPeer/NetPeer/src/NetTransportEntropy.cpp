#include "NetPeerMacro.h"
#include "NetTransportEntropy.h"
#include "NetHostSocket.h"

int32_t NetTransportEntropy(void* context, uint8_t* output, uint32_t size)
{
    (void)context;
    return NetHostSocket::randomBytes(output, size) == NI_OK ? 0 : -1;
}