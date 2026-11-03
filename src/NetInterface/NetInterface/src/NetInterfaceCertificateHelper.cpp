#include "NetInterfaceCertificateHelper.h"
#include "NetInterfaceCertificateEntropy.h"
#include "NetInterface.h"

int NetInterfaceCertificateHelper::certificateEntropyBytes(void* context, unsigned char* output, size_t size)
{
    NetInterfaceCertificateEntropy* source = static_cast<NetInterfaceCertificateEntropy*>(context);
    return size > UINT32_MAX ? -1 : NetInterface::entropyBytes(source->m_function, source->m_context, output, static_cast<uint32_t>(size));
}