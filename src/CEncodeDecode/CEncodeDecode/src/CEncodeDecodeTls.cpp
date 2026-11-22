#include "CEncodeDecodeMacro.h"
#include "CEncodeDecodeTls.h"
#include "CEncodeDecodeTlsState.h"
#include <algorithm>
#include <cstring>
#include <new>

/** 复制已验证且不重叠的明文，短片段使用定长复制避免通用memcpy调度
@param [out] output 至少size字节的TLS自有存储
@param [in] input 至少size字节；size为0时允许为空
@param [in] size 本段长度，调用方已完成边界校验
*/
static inline void CopyTlsPlaintext(uint8_t* output, const uint8_t* input, uint32_t size)
{
    if (size >= 8)
    {
        std::memcpy(output, input, size);
    }
    else if (size >= 4)
    {
        std::memcpy(output, input, 4);
        std::memcpy(output + size - 4, input + size - 4, 4);
    }
    else if (size >= 2)
    {
        std::memcpy(output, input, 2);
        std::memcpy(output + size - 2, input + size - 2, 2);
    }
    else if (size != 0)
    {
        *output = *input;
    }
}

CEncodeDecodeTls::CEncodeDecodeTls(const CEncodeDecodeTlsOptions& options) :
CEncodeDecodeTls(options, CEncodeDecodeTlsQueueLimits())
{

}

CEncodeDecodeTls::CEncodeDecodeTls(const CEncodeDecodeTlsOptions& options, const CEncodeDecodeTlsQueueLimits& limits) :
m_state(new (std::nothrow) CEncodeDecodeTlsState(limits))
{
    if (m_state != nullptr)
    {
        m_state->configure(options);
    }
}

CEncodeDecodeTls::~CEncodeDecodeTls()
{
    delete m_state;
}

int32_t CEncodeDecodeTls::feed(const uint8_t* data, uint32_t size, uint32_t& consumed)
{
    consumed = 0;
    if (status() < 0)
    {
        return status();
    }
    if (size != 0 && data == nullptr)
    {
        return CENCODE_TLS_ARGUMENT;
    }
    if (m_state->m_inputFinished || m_state->m_status == CENCODE_TLS_CLOSED)
    {
        return CENCODE_TLS_STATE;
    }
    if (!m_state->m_input.append(data, size, consumed))
    {
        return m_state->fail(CENCODE_TLS_MEMORY, 0);
    }
    return consumed == size ? CENCODE_TLS_OK : CENCODE_TLS_WANT_IO;
}

int32_t CEncodeDecodeTls::progress(uint64_t monotonicMs)
{
    if (status() < 0 || status() == CENCODE_TLS_CLOSED)
    {
        return status();
    }
    if (!m_state->m_clockStarted)
    {
        m_state->m_clockStarted = true;
        m_state->m_startedMs = monotonicMs;
    }
    else if (monotonicMs < m_state->m_lastMs)
    {
        return m_state->fail(CENCODE_TLS_ARGUMENT, 0);
    }
    m_state->m_lastMs = monotonicMs;
    if (m_state->m_status == CENCODE_TLS_HANDSHAKE)
    {
        if (monotonicMs - m_state->m_startedMs >= m_state->m_timeoutMs)
        {
            return m_state->fail(CENCODE_TLS_TIMEOUT, 0);
        }
        int result = mbedtls_ssl_handshake(&m_state->m_ssl);
        if (result == MBEDTLS_ERR_SSL_WANT_READ || result == MBEDTLS_ERR_SSL_WANT_WRITE)
        {
            return CENCODE_TLS_HANDSHAKE;
        }
        if (result != 0)
        {
            return m_state->fail(result == MBEDTLS_ERR_SSL_CONN_EOF ? CENCODE_TLS_TRUNCATED : CENCODE_TLS_PROTOCOL, result);
        }
        m_state->m_status = CENCODE_TLS_READY;
    }
    int32_t result = m_state->flush();
    return result < 0 ? result : m_state->m_status;
}

int32_t CEncodeDecodeTls::peekOutput(const uint8_t*& data, uint32_t& size) const
{
    data = nullptr;
    size = 0;
    if (m_state == nullptr)
    {
        return CENCODE_TLS_MEMORY;
    }
    size = m_state->m_output.peek(data);
    return m_state->m_status < 0 ? m_state->m_status : CENCODE_TLS_OK;
}

int32_t CEncodeDecodeTls::consumeOutput(uint32_t size)
{
    if (m_state == nullptr)
    {
        return CENCODE_TLS_MEMORY;
    }
    return m_state->m_output.consume(size) ? CENCODE_TLS_OK : CENCODE_TLS_ARGUMENT;
}

int32_t CEncodeDecodeTls::write(const uint8_t* data, uint32_t size, uint32_t& accepted)
{
    accepted = 0;
    if (status() < 0)
    {
        return status();
    }
    if (status() != CENCODE_TLS_READY || m_state->m_closeStarted)
    {
        return CENCODE_TLS_STATE;
    }
    if (size != 0 && data == nullptr)
    {
        return CENCODE_TLS_ARGUMENT;
    }
    if (m_state->m_plaintextSize != 0)
    {
        return CENCODE_TLS_WANT_IO;
    }
    accepted = (std::min)(size, static_cast<uint32_t>(sizeof(m_state->m_plaintext)));
    if (accepted != 0)
    {
        std::memcpy(m_state->m_plaintext, data, accepted);
        m_state->m_plaintextSize = accepted;
    }
    // 接受只表示复制入队；progress负责转换，避免返回失败却已消费的歧义
    return CENCODE_TLS_OK;
}

int32_t CEncodeDecodeTls::append(const uint8_t* data, uint32_t size, uint32_t& accepted)
{
    accepted = 0;
    const int32_t current = status();
    if (current < 0)
    {
        return current;
    }
    if (current != CENCODE_TLS_READY || m_state->m_closeStarted)
    {
        return CENCODE_TLS_STATE;
    }
    if (size != 0 && (data == nullptr || reinterpret_cast<uintptr_t>(data) > UINTPTR_MAX - size))
    {
        return CENCODE_TLS_ARGUMENT;
    }
    if (m_state->m_plaintextFrozen)
    {
        return CENCODE_TLS_WANT_IO;
    }
    const uint32_t available = static_cast<uint32_t>(sizeof(m_state->m_plaintext)) - m_state->m_plaintextSize;
    accepted = (std::min)(size, available);
    if (accepted != 0)
    {
        std::memcpy(m_state->m_plaintext + m_state->m_plaintextSize, data, accepted);
        m_state->m_plaintextSize += accepted;
    }
    return size != 0 && accepted == 0 ? CENCODE_TLS_WANT_IO : CENCODE_TLS_OK;
}

int32_t CEncodeDecodeTls::appendExact(const uint8_t* data, uint32_t size)
{
    return appendExact(data, size, nullptr, 0);
}

int32_t CEncodeDecodeTls::appendExact(const uint8_t* first, uint32_t firstSize, const uint8_t* second, uint32_t secondSize)
{
    const int32_t current = status();
    if (current < 0)
    {
        return current;
    }
    if (current != CENCODE_TLS_READY || m_state->m_closeStarted)
    {
        return CENCODE_TLS_STATE;
    }
    if ((firstSize != 0 && (first == nullptr || reinterpret_cast<uintptr_t>(first) > UINTPTR_MAX - firstSize)) ||
        (secondSize != 0 && (second == nullptr || reinterpret_cast<uintptr_t>(second) > UINTPTR_MAX - secondSize)))
    {
        return CENCODE_TLS_ARGUMENT;
    }
    const uint64_t total = static_cast<uint64_t>(firstSize) + secondSize;
    if (m_state->m_plaintextFrozen || total > sizeof(m_state->m_plaintext))
    {
        return CENCODE_TLS_WANT_IO;
    }
    const uint32_t size = static_cast<uint32_t>(total);
    if (size > sizeof(m_state->m_plaintext) - m_state->m_plaintextSize)
    {
        // 只转换已经拥有的一条记录；密文仍由物理池承载，不调用宿主或等待网络。
        // WANT_IO冻结旧记录，当前新输入仍是零接纳，调用方可安全完整重试。
        const int32_t flushed = m_state->flush();
        if (flushed != CENCODE_TLS_OK)
        {
            return flushed;
        }
    }
    uint8_t* output = m_state->m_plaintext + m_state->m_plaintextSize;
    if (firstSize != 0)
    {
        CopyTlsPlaintext(output, first, firstSize);
    }
    if (secondSize != 0)
    {
        CopyTlsPlaintext(output + firstSize, second, secondSize);
    }
    m_state->m_plaintextSize += size;
    return CENCODE_TLS_OK;
}

int32_t CEncodeDecodeTls::read(uint8_t* output, uint32_t capacity, uint32_t& received)
{
    received = 0;
    if (status() < 0 || status() == CENCODE_TLS_CLOSED)
    {
        return status();
    }
    if (status() != CENCODE_TLS_READY)
    {
        return CENCODE_TLS_STATE;
    }
    if (capacity != 0 && output == nullptr)
    {
        return CENCODE_TLS_ARGUMENT;
    }
    if (capacity == 0)
    {
        return CENCODE_TLS_OK;
    }
    int result = mbedtls_ssl_read(&m_state->m_ssl, output, (std::min)(capacity, static_cast<uint32_t>(16384)));
    if (result == MBEDTLS_ERR_SSL_WANT_READ || result == MBEDTLS_ERR_SSL_WANT_WRITE)
    {
        return CENCODE_TLS_WANT_IO;
    }
    if (result == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY)
    {
        m_state->m_status = CENCODE_TLS_CLOSED;
        return m_state->m_status;
    }
    if (result <= 0)
    {
        return m_state->fail(result == 0 || result == MBEDTLS_ERR_SSL_CONN_EOF ? CENCODE_TLS_TRUNCATED : CENCODE_TLS_PROTOCOL, result);
    }
    received = static_cast<uint32_t>(result);
    return CENCODE_TLS_OK;
}

int32_t CEncodeDecodeTls::finishInput()
{
    if (status() < 0)
    {
        return status();
    }
    m_state->m_inputFinished = true;
    return CENCODE_TLS_OK;
}

int32_t CEncodeDecodeTls::close()
{
    if (status() < 0)
    {
        return status();
    }
    if (status() == CENCODE_TLS_HANDSHAKE)
    {
        return CENCODE_TLS_STATE;
    }
    m_state->m_closeStarted = true;
    if (m_state->m_closeSent)
    {
        return CENCODE_TLS_OK;
    }
    int32_t flushed = m_state->flush();
    if (flushed != CENCODE_TLS_OK)
    {
        return flushed;
    }
    int result = mbedtls_ssl_close_notify(&m_state->m_ssl);
    if (result == MBEDTLS_ERR_SSL_WANT_READ || result == MBEDTLS_ERR_SSL_WANT_WRITE)
    {
        return CENCODE_TLS_WANT_IO;
    }
    if (result != 0)
    {
        return m_state->fail(CENCODE_TLS_PROTOCOL, result);
    }
    m_state->m_closeSent = true;
    return CENCODE_TLS_OK;
}

int32_t CEncodeDecodeTls::status() const
{
    return m_state == nullptr ? CENCODE_TLS_MEMORY : m_state->m_status;
}

int32_t CEncodeDecodeTls::detail() const
{
    return m_state == nullptr ? 0 : m_state->m_detail;
}

int32_t CEncodeDecodeTls::outputState() const
{
    const int32_t current = status();
    if (current < 0) { return current; }
    const uint8_t* bytes = nullptr;
    return m_state->m_plaintextSize != m_state->m_plaintextOffset || m_state->m_output.peek(bytes) != 0 ? CENCODE_TLS_READY : CENCODE_TLS_OK;
}