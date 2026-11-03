#include "NetInterfaceCryptoState.h"
#include "NetInterfaceMacro.h"
#include "NetInterface.h"
#include "mbedtls/pkcs5.h"
#include "mbedtls/sha256.h"
#include "mbedtls/platform_util.h"
#include <cstring>
#include <new>

NetInterfaceHashOpaque::NetInterfaceHashOpaque() :
m_size(0),
m_status(NI_OK),
m_finished(false)
{
    mbedtls_sha256_init(&m_context);
    if (mbedtls_sha256_starts(&m_context, 0) != 0)
    {
        m_status = NI_INVALID_STATE;
    }
}

NetInterfaceHashOpaque::~NetInterfaceHashOpaque()
{
    mbedtls_sha256_free(&m_context);
}