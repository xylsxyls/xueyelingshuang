#include "NetFileIo.h"
#include "NetPeerFileIoState.h"
#include "CStringManager/CStringManagerAPI.h"
#include "NetPeerMacro.h"
#include <cstring>
#include <new>

NetFileIoBackendFactory NetFileIo::m_factory = nullptr;

NetPeerFileIoState::NetPeerFileIoState() :
m_backend(nullptr),
m_error(NI_OK),
m_phase(NET_FILE_BACKEND_STARTING),
m_calling(false)
{

}

NetFileIo::NetFileIo() :
m_state(nullptr),
m_idleError(NI_OK)
{

}

NetFileIo::~NetFileIo()
{
    uninit();
    if (m_state != nullptr)
    {
        // 普通 uninit 保留尚未停止的后端。生命周期负责人退出时由后端析构收尾。
        m_state->m_phase = NET_FILE_BACKEND_RELEASING;
        m_state->m_calling = true;
        delete m_state->m_backend;
        delete m_state;
    }
}

int32_t NetFileIo::configureBackend(NetFileIoBackendFactory factory)
{
    if (factory == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (m_factory != nullptr)
    {
        return NI_INVALID_STATE;
    }
    m_factory = factory;
    return NI_OK;
}

int32_t NetFileIo::init()
{
    if (m_state != nullptr)
    {
        return NI_INVALID_STATE;
    }
    if (m_factory == nullptr)
    {
        m_idleError = NI_NOT_SUPPORTED;
        return m_idleError;
    }
    m_state = new (std::nothrow) NetPeerFileIoState;
    if (m_state == nullptr)
    {
        m_idleError = NI_NO_MEMORY;
        return m_idleError;
    }
    m_state->m_calling = true;
    int32_t result = NI_NO_MEMORY;
    try
    {
        m_state->m_backend = m_factory();
        if (m_state->m_backend != nullptr)
        {
            result = m_state->m_phase == NET_FILE_BACKEND_STARTING ? m_state->m_backend->init() : NI_CANCELLED;
        }
    }
    catch (...)
    {
        result = NI_NO_MEMORY;
    }
    m_state->m_calling = false;
    if (result > NI_OK)
    {
        result = NI_INVALID_STATE;
    }
    if (result == NI_OK && m_state->m_phase != NET_FILE_BACKEND_STARTING)
    {
        result = NI_CANCELLED;
    }
    m_state->m_error = result;
    if (result == NI_OK)
    {
        m_state->m_phase = NET_FILE_BACKEND_ACTIVE;
    }
    else
    {
        uninit();
    }
    return result;
}

void NetFileIo::finishCall() const
{
    m_state->m_calling = false;
    if (m_state->m_phase != NET_FILE_BACKEND_STOP_PENDING || m_state->m_backend == nullptr)
    {
        return;
    }
    m_state->m_phase = NET_FILE_BACKEND_STOPPING;
    m_state->m_calling = true;
    try
    {
        m_state->m_backend->stop();
    }
    catch (...)
    {
        m_state->m_error = NI_INVALID_STATE;
        m_state->m_phase = NET_FILE_BACKEND_STOP_PENDING;
    }
    m_state->m_calling = false;
}

int32_t NetFileIo::nextRequestId(uint64_t& id)
{
    id = 0;
    if (m_state == nullptr || m_state->m_phase != NET_FILE_BACKEND_ACTIVE || m_state->m_backend == nullptr || m_state->m_calling)
    {
        return NI_INVALID_STATE;
    }
    uint64_t candidate = 0;
    int32_t result = NI_NO_MEMORY;
    m_state->m_calling = true;
    try
    {
        result = m_state->m_backend->nextRequestId(candidate);
    }
    catch (...)
    {
        m_state->m_error = NI_NO_MEMORY;
    }
    if (result > NI_OK || (result == NI_OK && candidate == 0))
    {
        result = NI_INVALID_STATE;
        m_state->m_error = result;
    }
    if (result == NI_OK)
    {
        id = candidate;
    }
    finishCall();
    return result;
}

int32_t NetFileIo::submit(const NetFileIoRequest& request)
{
    if (m_state == nullptr || m_state->m_phase != NET_FILE_BACKEND_ACTIVE || m_state->m_backend == nullptr || m_state->m_calling)
    {
        return NI_INVALID_STATE;
    }
    if (request.id == 0 || request.operation < NET_FILE_IO_OPEN_READ || request.operation > NET_FILE_IO_OPEN_TEMPORARY ||
        request.size > 65536 || request.flags > 1 ||
        (request.operation == NET_FILE_IO_WRITE && request.size != 0 && request.bytes == nullptr) ||
        CStringManager::boundedLength(request.path, 4097) > 4096 || CStringManager::boundedLength(request.destination, 4097) > 4096)
    {
        return NI_INVALID_ARGUMENT;
    }
    int32_t result = NI_NO_MEMORY;
    m_state->m_calling = true;
    try
    {
        result = m_state->m_backend->submit(request);
    }
    catch (...)
    {
        m_state->m_error = NI_NO_MEMORY;
    }
    if (result > NI_OK)
    {
        result = NI_INVALID_STATE;
        m_state->m_error = result;
    }
    finishCall();
    return result;
}

int32_t NetFileIo::cancel(uint64_t id)
{
    if (m_state == nullptr || m_state->m_backend == nullptr || m_state->m_calling ||
        (m_state->m_phase != NET_FILE_BACKEND_ACTIVE && m_state->m_phase != NET_FILE_BACKEND_STOPPING && m_state->m_phase != NET_FILE_BACKEND_STOP_PENDING))
    {
        return NI_INVALID_STATE;
    }
    int32_t result = NI_NO_MEMORY;
    m_state->m_calling = true;
    try
    {
        result = m_state->m_backend->cancel(id);
    }
    catch (...)
    {
        m_state->m_error = NI_NO_MEMORY;
    }
    if (result > NI_OK)
    {
        result = NI_INVALID_STATE;
        m_state->m_error = result;
    }
    finishCall();
    return result;
}

int32_t NetFileIo::peek(NetFileIoResult& result)
{
    std::memset(&result, 0, sizeof(result));
    if (m_state == nullptr || m_state->m_backend == nullptr || m_state->m_calling ||
        (m_state->m_phase != NET_FILE_BACKEND_ACTIVE && m_state->m_phase != NET_FILE_BACKEND_STOPPING && m_state->m_phase != NET_FILE_BACKEND_STOP_PENDING))
    {
        return NI_INVALID_STATE;
    }
    NetFileIoResult candidate = {};
    int32_t status = NI_NO_MEMORY;
    m_state->m_calling = true;
    try
    {
        status = m_state->m_backend->peek(candidate);
    }
    catch (...)
    {
        m_state->m_error = NI_NO_MEMORY;
    }
    if (status == NI_READY)
    {
        if (candidate.id == 0 || candidate.operation < NET_FILE_IO_OPEN_READ || candidate.operation > NET_FILE_IO_OPEN_TEMPORARY ||
            candidate.size > 65536 || (candidate.size != 0 && candidate.bytes == nullptr) || candidate.result > NI_OK)
        {
            status = NI_INVALID_STATE;
            m_state->m_error = status;
        }
        else
        {
            result = candidate;
        }
    }
    else if (status >= NI_OK && status != NI_NEED_MORE)
    {
        status = NI_INVALID_STATE;
        m_state->m_error = status;
    }
    finishCall();
    return status;
}

int32_t NetFileIo::consume(uint64_t id)
{
    if (m_state == nullptr || m_state->m_backend == nullptr || m_state->m_calling ||
        (m_state->m_phase != NET_FILE_BACKEND_ACTIVE && m_state->m_phase != NET_FILE_BACKEND_STOPPING && m_state->m_phase != NET_FILE_BACKEND_STOP_PENDING))
    {
        return NI_INVALID_STATE;
    }
    int32_t result = NI_NO_MEMORY;
    m_state->m_calling = true;
    try
    {
        result = m_state->m_backend->consume(id);
    }
    catch (...)
    {
        m_state->m_error = NI_NO_MEMORY;
    }
    if (result > NI_OK)
    {
        result = NI_INVALID_STATE;
        m_state->m_error = result;
    }
    finishCall();
    return result;
}

void NetFileIo::stop()
{
    if (m_state == nullptr || m_state->m_phase == NET_FILE_BACKEND_RELEASING || m_state->m_phase == NET_FILE_BACKEND_STOPPING)
    {
        return;
    }
    m_state->m_phase = NET_FILE_BACKEND_STOP_PENDING;
    if (!m_state->m_calling)
    {
        finishCall();
    }
}

int32_t NetFileIo::queryCompletion() const
{
    if (m_state == nullptr)
    {
        return NI_READY;
    }
    if (m_state->m_calling)
    {
        return NI_INVALID_STATE;
    }
    if (m_state->m_backend == nullptr)
    {
        return NI_READY;
    }
    int32_t result = NI_INVALID_STATE;
    m_state->m_calling = true;
    try
    {
        result = m_state->m_backend->done() ? NI_READY : NI_NEED_MORE;
    }
    catch (...)
    {
        m_state->m_error = NI_INVALID_STATE;
    }
    if (m_state->m_phase == NET_FILE_BACKEND_STOP_PENDING)
    {
        result = NI_INVALID_STATE;
    }
    finishCall();
    return result;
}

bool NetFileIo::accepting() const
{
    return m_state != nullptr && m_state->m_phase == NET_FILE_BACKEND_ACTIVE && queryCompletion() == NI_NEED_MORE;
}

bool NetFileIo::done() const
{
    return queryCompletion() == NI_READY;
}

void NetFileIo::uninit()
{
    if (m_state == nullptr)
    {
        return;
    }
    if (m_state->m_calling || m_state->m_phase == NET_FILE_BACKEND_RELEASING)
    {
        m_state->m_error = NI_INVALID_STATE;
        return;
    }
    if (m_state->m_backend != nullptr)
    {
        stop();
        if (!done())
        {
            m_state->m_error = NI_INVALID_STATE;
            return;
        }
        m_state->m_phase = NET_FILE_BACKEND_RELEASING;
        m_state->m_calling = true;
        try
        {
            m_state->m_backend->uninit();
        }
        catch (...)
        {
            m_state->m_calling = false;
            m_state->m_phase = NET_FILE_BACKEND_STOPPING;
            m_state->m_error = NI_INVALID_STATE;
            return;
        }
        delete m_state->m_backend;
    }
    m_idleError = m_state->m_error;
    delete m_state;
    m_state = nullptr;
}

bool NetFileIo::released() const
{
    return m_state == nullptr;
}

int32_t NetFileIo::status() const
{
    return m_state != nullptr ? m_state->m_error : m_idleError;
}