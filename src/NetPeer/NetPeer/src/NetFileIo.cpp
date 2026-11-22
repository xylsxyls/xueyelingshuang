#include "NetFileIo.h"
#include "NetPeerMacro.h"
#include <cstring>

NetFileIoBackendFactory NetFileIo::m_factory = nullptr;

NetFileIo::NetFileIo() :
m_backend(nullptr),
m_error(NI_OK),
m_phase(NET_FILE_BACKEND_IDLE),
m_calling(false)
{

}

NetFileIo::~NetFileIo()
{
    uninit();
    // 宿主须先停止并等到done；违约时保留仍在工作的后端，不能阻塞网络执行者或释放在途工作。
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
    if (m_phase != NET_FILE_BACKEND_IDLE || m_backend != nullptr || m_calling)
    {
        return NI_INVALID_STATE;
    }
    if (m_factory == nullptr)
    {
        m_error = NI_NOT_SUPPORTED;
        return m_error;
    }
    m_phase = NET_FILE_BACKEND_STARTING;
    m_calling = true;
    int32_t result = NI_NO_MEMORY;
    try
    {
        m_backend = m_factory();
        if (m_backend != nullptr)
        {
            result = m_phase == NET_FILE_BACKEND_STARTING ? m_backend->init() : NI_CANCELLED;
        }
    }
    catch (...)
    {
        result = NI_NO_MEMORY;
    }
    m_calling = false;
    if (result > NI_OK)
    {
        result = NI_INVALID_STATE;
    }
    if (result == NI_OK && m_phase != NET_FILE_BACKEND_STARTING)
    {
        result = NI_CANCELLED;
    }
    m_error = result;
    if (result == NI_OK)
    {
        m_phase = NET_FILE_BACKEND_ACTIVE;
    }
    else
    {
        uninit();
    }
    return result;
}

void NetFileIo::finishCall()
{
    m_calling = false;
    if (m_phase == NET_FILE_BACKEND_STOPPING)
    {
        // 后端回入stop只做标记，等本次调用返回后再真正停止，不能递归释放借用。
        stop();
    }
}

int32_t NetFileIo::nextRequestId(uint64_t& id)
{
    id = 0;
    if (m_phase != NET_FILE_BACKEND_ACTIVE || m_backend == nullptr || m_calling)
    {
        return NI_INVALID_STATE;
    }
    uint64_t candidate = 0;
    int32_t result = NI_NO_MEMORY;
    m_calling = true;
    try
    {
        result = m_backend->nextRequestId(candidate);
    }
    catch (...)
    {
        m_error = NI_NO_MEMORY;
    }
    if (result > NI_OK || (result == NI_OK && candidate == 0))
    {
        result = NI_INVALID_STATE;
        m_error = result;
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
    if (m_phase != NET_FILE_BACKEND_ACTIVE || m_backend == nullptr || m_calling)
    {
        return NI_INVALID_STATE;
    }
    if (request.id == 0 || request.operation < NET_FILE_IO_OPEN_READ || request.operation > NET_FILE_IO_OPEN_TEMPORARY ||
        request.size > 65536 || request.flags > 1 ||
        (request.operation == NET_FILE_IO_WRITE && request.size != 0 && request.bytes == nullptr) ||
        !validPath(request.path) || !validPath(request.destination))
    {
        return NI_INVALID_ARGUMENT;
    }
    int32_t result = NI_NO_MEMORY;
    m_calling = true;
    try
    {
        result = m_backend->submit(request);
    }
    catch (...)
    {
        m_error = NI_NO_MEMORY;
    }
    if (result > NI_OK)
    {
        result = NI_INVALID_STATE;
        m_error = result;
    }
    finishCall();
    return result;
}

bool NetFileIo::validPath(const char* path)
{
    if (path == nullptr)
    {
        return true;
    }
    for (uint32_t index = 0; index <= 4096; ++index)
    {
        if (path[index] == 0)
        {
            return true;
        }
    }
    return false;
}

int32_t NetFileIo::cancel(uint64_t id)
{
    if (m_backend == nullptr || m_calling ||
        (m_phase != NET_FILE_BACKEND_ACTIVE && m_phase != NET_FILE_BACKEND_STOPPING))
    {
        return NI_INVALID_STATE;
    }
    int32_t result = NI_NO_MEMORY;
    m_calling = true;
    try
    {
        result = m_backend->cancel(id);
    }
    catch (...)
    {
        m_error = NI_NO_MEMORY;
    }
    finishCall();
    return result;
}

int32_t NetFileIo::peek(NetFileIoResult& result)
{
    std::memset(&result, 0, sizeof(result));
    if (m_backend == nullptr || m_calling ||
        (m_phase != NET_FILE_BACKEND_ACTIVE && m_phase != NET_FILE_BACKEND_STOPPING))
    {
        return NI_INVALID_STATE;
    }
    NetFileIoResult candidate = {};
    int32_t status = NI_NO_MEMORY;
    m_calling = true;
    try
    {
        status = m_backend->peek(candidate);
    }
    catch (...)
    {
        m_error = NI_NO_MEMORY;
    }
    if (status == NI_READY)
    {
        if (candidate.id == 0 || candidate.operation < NET_FILE_IO_OPEN_READ || candidate.operation > NET_FILE_IO_OPEN_TEMPORARY ||
            candidate.size > 65536 || (candidate.size != 0 && candidate.bytes == nullptr) || candidate.result > NI_OK)
        {
            status = NI_INVALID_STATE;
            m_error = status;
        }
        else
        {
            result = candidate;
        }
    }
    else if (status >= NI_OK && status != NI_NEED_MORE)
    {
        status = NI_INVALID_STATE;
        m_error = status;
    }
    finishCall();
    return status;
}

int32_t NetFileIo::consume(uint64_t id)
{
    if (m_backend == nullptr || m_calling ||
        (m_phase != NET_FILE_BACKEND_ACTIVE && m_phase != NET_FILE_BACKEND_STOPPING))
    {
        return NI_INVALID_STATE;
    }
    int32_t result = NI_NO_MEMORY;
    m_calling = true;
    try
    {
        result = m_backend->consume(id);
    }
    catch (...)
    {
        m_error = NI_NO_MEMORY;
    }
    finishCall();
    return result;
}

void NetFileIo::stop()
{
    if (m_phase == NET_FILE_BACKEND_IDLE || m_phase == NET_FILE_BACKEND_RELEASING)
    {
        return;
    }
    m_phase = NET_FILE_BACKEND_STOPPING;
    if (m_backend == nullptr || m_calling)
    {
        return;
    }
    m_calling = true;
    try
    {
        m_backend->stop();
    }
    catch (...)
    {
        m_error = NI_INVALID_STATE;
    }
    m_calling = false;
}

bool NetFileIo::done() const
{
    if (m_calling)
    {
        return false;
    }
    try
    {
        return m_backend == nullptr || m_backend->done();
    }
    catch (...)
    {
        return false;
    }
}

void NetFileIo::uninit()
{
    if (m_calling || m_phase == NET_FILE_BACKEND_RELEASING)
    {
        m_error = NI_INVALID_STATE;
        return;
    }
    if (m_backend == nullptr)
    {
        m_phase = NET_FILE_BACKEND_IDLE;
        return;
    }
    stop();
    if (!done())
    {
        m_error = NI_INVALID_STATE;
        return;
    }
    m_phase = NET_FILE_BACKEND_RELEASING;
    m_calling = true;
    try
    {
        m_backend->uninit();
    }
    catch (...)
    {
        m_calling = false;
        m_phase = NET_FILE_BACKEND_STOPPING;
        m_error = NI_INVALID_STATE;
        return;
    }
    delete m_backend;
    m_backend = nullptr;
    m_calling = false;
    m_phase = NET_FILE_BACKEND_IDLE;
}

int32_t NetFileIo::status() const
{
    return m_error;
}