#include "CEncodeDecodeMacro.h"
#include "CEncodeDecodeTlsState.h"
#include "CEncodeDecode.h"
#include "CEncodeDecodeTls.h"
#include <mbedtls/platform_util.h>
#include <algorithm>
#include <cstring>
#include <new>

CEncodeDecodeTlsQueue::CEncodeDecodeTlsQueue(int64_t limit) :
m_head(0),
m_inlineSize(0),
m_limit(limit),
m_size(0),
m_first(nullptr),
m_last(nullptr)
{

}

CEncodeDecodeTlsQueue::~CEncodeDecodeTlsQueue()
{
    while (m_first)
    {
        Block* old = m_first;
        m_first = old->next;
        delete old;
    }
}

bool CEncodeDecodeTlsQueue::append(const uint8_t* data, uint32_t size, uint32_t& accepted)
{
    accepted = 0;
    const uint64_t available = (m_limit < 0 ? UINT64_MAX : static_cast<uint64_t>(m_limit)) - m_size;
    const uint32_t wanted = static_cast<uint32_t>((std::min<uint64_t>)(size, available));
    if (!m_first && wanted != 0)
    {
        accepted = (std::min)(wanted, static_cast<uint32_t>(sizeof(m_data)) - m_inlineSize);
        const uint32_t tail = (m_head + m_inlineSize) % sizeof(m_data);
        const uint32_t first = (std::min)(accepted, static_cast<uint32_t>(sizeof(m_data)) - tail);
        std::memcpy(m_data + tail, data, first);
        std::memcpy(m_data, data + first, accepted - first);
        m_inlineSize += accepted;
        m_size += accepted;
    }
    while (accepted < wanted)
    {
        if (!m_last || m_last->end == sizeof(m_last->bytes))
        {
            Block* next = new (std::nothrow) Block;
            if (!next)
            {
                return false;
            }
            next->begin = next->end = 0;
            next->next = nullptr;
            if (m_last)
            {
                m_last->next = next;
            }
            else
            {
                m_first = next;
            }
            m_last = next;
        }
        const uint32_t count = (std::min)(wanted - accepted, static_cast<uint32_t>(sizeof(m_last->bytes)) - m_last->end);
        std::memcpy(m_last->bytes + m_last->end, data + accepted, count);
        m_last->end += count;
        accepted += count;
        m_size += count;
    }
    return true;
}

uint32_t CEncodeDecodeTlsQueue::take(uint8_t* output, uint32_t size)
{
    uint32_t taken = 0;
    const uint8_t* data = nullptr;
    while (taken < size)
    {
        const uint32_t count = (std::min)(size - taken, peek(data));
        if (count == 0)
        {
            break;
        }
        std::memcpy(output + taken, data, count);
        consume(count);
        taken += count;
    }
    return taken;
}

uint32_t CEncodeDecodeTlsQueue::peek(const uint8_t*& data) const
{
    if (m_inlineSize != 0)
    {
        data = m_data + m_head;
        return (std::min)(m_inlineSize, static_cast<uint32_t>(sizeof(m_data)) - m_head);
    }
    data = m_first ? m_first->bytes + m_first->begin : nullptr;
    return m_first ? m_first->end - m_first->begin : 0;
}

bool CEncodeDecodeTlsQueue::consume(uint32_t size)
{
    const uint8_t* data = nullptr;
    if (size > peek(data))
    {
        return false;
    }
    if (m_inlineSize != 0)
    {
        m_head = (m_head + size) % sizeof(m_data);
        m_inlineSize -= size;
    }
    else if (m_first && size != 0)
    {
        m_first->begin += size;
        if (m_first->begin == m_first->end)
        {
            Block* old = m_first;
            m_first = old->next;
            if (!m_first)
            {
                m_last = nullptr;
            }
            delete old;
        }
    }
    m_size -= size;
    return true;
}

CEncodeDecodeTlsState::CEncodeDecodeTlsState(const CEncodeDecodeTlsQueueLimits& limits) :
m_entropy(nullptr),
m_entropyContext(nullptr),
m_input(limits.inputBytes()),
m_output(limits.outputBytes()),
m_plaintextSize(0),
m_plaintextOffset(0),
m_plaintextFrozen(false),
m_status(CENCODE_TLS_HANDSHAKE),
m_detail(0),
m_inputFinished(false),
m_closeStarted(false),
m_closeSent(false),
m_clockStarted(false),
m_startedMs(0),
m_lastMs(0),
m_timeoutMs(0)
{
    mbedtls_ssl_init(&m_ssl);
    mbedtls_ssl_config_init(&m_config);
    mbedtls_ctr_drbg_init(&m_random);
    mbedtls_x509_crt_init(&m_certificate);
    mbedtls_pk_init(&m_key);
}

CEncodeDecodeTlsState::~CEncodeDecodeTlsState()
{
    mbedtls_ssl_free(&m_ssl);
    mbedtls_ssl_config_free(&m_config);
    mbedtls_x509_crt_free(&m_certificate);
    mbedtls_pk_free(&m_key);
    mbedtls_ctr_drbg_free(&m_random);
    mbedtls_platform_zeroize(m_plaintext, sizeof(m_plaintext));
}

int32_t CEncodeDecodeTlsState::configure(const CEncodeDecodeTlsOptions& options)
{
    CEncodeDecodeTlsOptions validated = {};
    if ((options.server != 0 && options.server != 1) || options.entropy == nullptr ||
        !validated.setHandshakeTimeout(options.handshakeTimeoutMs))
    {
        return fail(CENCODE_TLS_ARGUMENT, 0);
    }
    if (options.server != 0)
    {
        if (options.certificate == nullptr || options.certificateSize == 0 || options.certificateSize > 131072 ||
            options.privateKey == nullptr || options.privateKeySize == 0 || options.privateKeySize > 32768)
        {
            return fail(CENCODE_TLS_ARGUMENT, 0);
        }
    }
    else
    {
        if (options.authority == nullptr || options.authoritySize == 0 || options.authoritySize > 131072 || options.hostname == nullptr)
        {
            return fail(CENCODE_TLS_ARGUMENT, 0);
        }
        size_t hostnameSize = 0;
        for (; hostnameSize <= 253 && options.hostname[hostnameSize] != 0; ++hostnameSize)
        {
            char ch = options.hostname[hostnameSize];
            if (!((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '-' || ch == '.'))
            {
                return fail(CENCODE_TLS_ARGUMENT, 0);
            }
        }
        if (hostnameSize == 0 || hostnameSize > 253)
        {
            return fail(CENCODE_TLS_ARGUMENT, 0);
        }
    }
    m_entropy = options.entropy;
    m_entropyContext = options.entropyContext;
    m_timeoutMs = validated.handshakeTimeoutMs == 0 ? 10000 : validated.handshakeTimeoutMs;
    const unsigned char personalization[] = "NetInterface TLS v2";
    int result = mbedtls_ctr_drbg_seed(&m_random, entropyBytes, this, personalization, sizeof(personalization) - 1);
    if (result != 0)
    {
        return fail(CENCODE_TLS_PROTOCOL, result);
    }
    result = mbedtls_ssl_config_defaults(&m_config, options.server != 0 ? MBEDTLS_SSL_IS_SERVER : MBEDTLS_SSL_IS_CLIENT,
        MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
    if (result != 0)
    {
        return fail(CENCODE_TLS_PROTOCOL, result);
    }
    mbedtls_ssl_conf_rng(&m_config, mbedtls_ctr_drbg_random, &m_random);
    mbedtls_ssl_conf_min_tls_version(&m_config, MBEDTLS_SSL_VERSION_TLS1_2);
    mbedtls_ssl_conf_max_tls_version(&m_config, MBEDTLS_SSL_VERSION_TLS1_2);
    mbedtls_ssl_conf_authmode(&m_config, options.server != 0 ? MBEDTLS_SSL_VERIFY_NONE : MBEDTLS_SSL_VERIFY_REQUIRED);
    result = mbedtls_x509_crt_parse(&m_certificate, options.server != 0 ? options.certificate : options.authority,
        options.server != 0 ? options.certificateSize : options.authoritySize);
    // 正数表示证书链中有部分解析失败，也必须拒绝整个配置
    if (result != 0)
    {
        return fail(CENCODE_TLS_PROTOCOL, result);
    }
    if (options.server != 0)
    {
        result = mbedtls_pk_parse_key(&m_key, options.privateKey, options.privateKeySize, nullptr, 0, mbedtls_ctr_drbg_random, &m_random);
        if (result == 0)
        {
            result = mbedtls_pk_check_pair(&m_certificate.pk, &m_key, mbedtls_ctr_drbg_random, &m_random);
        }
        if (result == 0)
        {
            result = mbedtls_ssl_conf_own_cert(&m_config, &m_certificate, &m_key);
        }
    }
    else
    {
        mbedtls_ssl_conf_ca_chain(&m_config, &m_certificate, nullptr);
    }
    if (result == 0)
    {
        result = mbedtls_ssl_setup(&m_ssl, &m_config);
    }
    if (result == 0 && options.server == 0)
    {
        result = mbedtls_ssl_set_hostname(&m_ssl, options.hostname);
    }
    if (result != 0)
    {
        return fail(CENCODE_TLS_PROTOCOL, result);
    }
    mbedtls_ssl_set_bio(&m_ssl, this, sendBytes, receiveBytes, nullptr);
    return m_status;
}

int32_t CEncodeDecodeTlsState::fail(int32_t status, int32_t detail)
{
    if (m_status >= 0)
    {
        m_status = status;
        m_detail = detail;
        mbedtls_platform_zeroize(m_plaintext, sizeof(m_plaintext));
        m_plaintextSize = 0;
        m_plaintextOffset = 0;
    }
    return m_status;
}

int32_t CEncodeDecodeTlsState::flush()
{
    if (m_status < 0)
    {
        return m_status;
    }
    if (m_plaintextSize == 0)
    {
        return CENCODE_TLS_OK;
    }
    m_plaintextFrozen = true;
    int result = mbedtls_ssl_write(&m_ssl, m_plaintext + m_plaintextOffset, m_plaintextSize - m_plaintextOffset);
    if (result == MBEDTLS_ERR_SSL_WANT_READ || result == MBEDTLS_ERR_SSL_WANT_WRITE)
    {
        return CENCODE_TLS_WANT_IO;
    }
    if (result <= 0)
    {
        return fail(CENCODE_TLS_PROTOCOL, result);
    }
    m_plaintextOffset += static_cast<uint32_t>(result);
    if (m_plaintextOffset == m_plaintextSize)
    {
        mbedtls_platform_zeroize(m_plaintext, m_plaintextSize);
        m_plaintextSize = 0;
        m_plaintextOffset = 0;
        m_plaintextFrozen = false;
    }
    return m_plaintextSize == 0 ? CENCODE_TLS_OK : CENCODE_TLS_WANT_IO;
}

int CEncodeDecodeTlsState::sendBytes(void* context, const unsigned char* data, size_t size)
{
    CEncodeDecodeTlsState* state = static_cast<CEncodeDecodeTlsState*>(context);
    uint32_t count = 0;
    if (!state->m_output.append(data, static_cast<uint32_t>((std::min<size_t>)(size, INT32_MAX)), count))
    {
        state->fail(CENCODE_TLS_MEMORY, MBEDTLS_ERR_SSL_ALLOC_FAILED);
        return MBEDTLS_ERR_SSL_ALLOC_FAILED;
    }
    return count == 0 ? MBEDTLS_ERR_SSL_WANT_WRITE : static_cast<int>(count);
}

int CEncodeDecodeTlsState::receiveBytes(void* context, unsigned char* data, size_t size)
{
    CEncodeDecodeTlsState* state = static_cast<CEncodeDecodeTlsState*>(context);
    uint32_t count = state->m_input.take(data, static_cast<uint32_t>((std::min<size_t>)(size, INT32_MAX)));
    if (count != 0)
    {
        return static_cast<int>(count);
    }
    return state->m_inputFinished ? 0 : MBEDTLS_ERR_SSL_WANT_READ;
}

int CEncodeDecodeTlsState::entropyBytes(void* context, unsigned char* data, size_t size)
{
    CEncodeDecodeTlsState* state = static_cast<CEncodeDecodeTlsState*>(context);
    if (size > UINT32_MAX || !CEncodeDecode::entropyBytes(state->m_entropy, state->m_entropyContext, data, static_cast<uint32_t>(size)))
    {
        return MBEDTLS_ERR_CTR_DRBG_ENTROPY_SOURCE_FAILED;
    }
    return 0;
}