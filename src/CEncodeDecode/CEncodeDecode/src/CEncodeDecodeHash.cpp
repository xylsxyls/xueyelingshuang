#include "CEncodeDecodeHash.h"

CEncodeDecodeHash::CEncodeDecodeHash() :
m_size(0),
m_status(CENCODE_OK),
m_finished(false)
{
    mbedtls_sha256_init(&m_context);
    if (mbedtls_sha256_starts(&m_context, 0) != 0)
    {
        m_status = CENCODE_STATE;
    }
}

CEncodeDecodeHash::~CEncodeDecodeHash()
{
    mbedtls_sha256_free(&m_context);
}