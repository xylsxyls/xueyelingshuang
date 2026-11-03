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

// FILE_BACKEND_UNIT_TEST_BEGIN
//#include "NetPeer/NetFileIoTest.h"
//#include "NetInterface/NetInterfaceAPI.h"
//#include <cstdio>
//#include <cstring>
//#include <vector>
//
//uint32_t NetFileIoTestBackend::m_mode = 0;
//uint32_t NetFileIoTestBackend::m_destroyed = 0;
//NetFileIoTestBackend* NetFileIoTestBackend::m_current = nullptr;
//NetFileIo* NetFileIoTestBackend::m_owner = nullptr;
//
//NetFileIoTestBackend::NetFileIoTestBackend() :
//m_next(0),
//m_submitted(0),
//m_throw(false),
//m_stopped(false),
//m_done(false)
//{
//    m_current = this;
//}
//
//NetFileIoTestBackend::~NetFileIoTestBackend()
//{
//    ++m_destroyed;
//}
//
//int32_t NetFileIoTestBackend::init()
//{
//    if (m_mode == 5) { m_done = true; return NI_INVALID_ARGUMENT; }
//    if (m_mode == 6) { m_done = true; throw 6; }
//    if (m_mode == 19) { m_owner->stop(); }
//    return NI_OK;
//}
//
//int32_t NetFileIoTestBackend::nextRequestId(uint64_t& id)
//{
//    id = ++m_next;
//    if (m_mode == 4) { return NI_LIMIT_EXCEEDED; }
//    if (m_mode == 7) { id = 0; return NI_OK; }
//    if (m_throw)
//    {
//        throw 1;
//    }
//    return NI_OK;
//}
//
//int32_t NetFileIoTestBackend::submit(const NetFileIoRequest& request)
//{
//    (void)request;
//    ++m_submitted;
//    if (m_mode == 8) { throw 8; }
//    if (m_mode == 9) { return NI_READY; }
//    if (m_mode == 18)
//    {
//        m_owner->uninit();
//        m_owner->stop();
//        uint64_t id = 99;
//        if (m_owner->nextRequestId(id) != NI_INVALID_STATE || id != 0 || m_owner->submit(request) != NI_INVALID_STATE) { return NI_PROTOCOL_ERROR; }
//    }
//    return NI_OK;
//}
//
//int32_t NetFileIoTestBackend::cancel(uint64_t id)
//{
//    (void)id;
//    if (m_mode == 10) { throw 10; }
//    return NI_NOT_FOUND;
//}
//
//int32_t NetFileIoTestBackend::peek(NetFileIoResult& result)
//{
//    std::memset(&result, 0, sizeof(result));
//    if (m_throw)
//    {
//        result.id = 0x4343;
//        result.bytes = reinterpret_cast<const uint8_t*>(1);
//        throw 1;
//    }
//    if (m_mode == 4) { result.id = 44; result.bytes = reinterpret_cast<const uint8_t*>(1); }
//    if (m_mode >= 11 && m_mode <= 15)
//    {
//        result.id = m_mode == 11 ? 0 : 7;
//        result.operation = m_mode == 12 ? 0 : NET_FILE_IO_READ;
//        result.result = m_mode == 15 ? NI_READY : NI_OK;
//        result.size = m_mode == 13 ? 65537 : m_mode == 14 ? 1 : 0;
//        return NI_READY;
//    }
//    return NI_NEED_MORE;
//}
//
//int32_t NetFileIoTestBackend::consume(uint64_t id)
//{
//    (void)id;
//    if (m_mode == 16) { throw 16; }
//    return NI_INVALID_STATE;
//}
//
//void NetFileIoTestBackend::stop()
//{
//    m_stopped = true;
//    if (m_mode == 3) { throw 3; }
//}
//
//bool NetFileIoTestBackend::done() const
//{
//    if (m_mode == 17) { throw 17; }
//    return m_done;
//}
//
//void NetFileIoTestBackend::uninit()
//{
//    if (m_mode == 20) { throw 20; }
//}
//
//NetFileIoBackend* NetFileIoTestBackend::create()
//{
//    if (m_mode == 1)
//    {
//        return nullptr;
//    }
//    if (m_mode == 2)
//    {
//        throw 1;
//    }
//    return new NetFileIoTestBackend();
//}
//
//
//#if defined(_WIN32) && defined(_DEBUG)
//#include <crtdbg.h>
//#endif
//int main()
//{
//    uint32_t assertions = 0;
//#define FILE_CHECK(condition) { ++assertions; if (!(condition)) { std::printf("FAIL line %u: %s\n", static_cast<uint32_t>(__LINE__), #condition); return 1; } }
//#if defined(_WIN32) && defined(_DEBUG)
//    _CrtMemState before;
//    _CrtMemCheckpoint(&before);
//#endif
//    {
//    NetFileIo io;
//    FILE_CHECK(io.init() == NI_NOT_SUPPORTED && io.status() == NI_NOT_SUPPORTED && io.done());
//    FILE_CHECK(NetFileIo::configureBackend(nullptr) == NI_INVALID_ARGUMENT);
//    FILE_CHECK(NetFileIo::configureBackend(NetFileIoTestBackend::create) == NI_OK);
//    FILE_CHECK(NetFileIo::configureBackend(NetFileIoTestBackend::create) == NI_INVALID_STATE);
//    for (uint32_t mode = 1; mode <= 2; ++mode)
//    {
//        NetFileIoTestBackend::m_mode = mode;
//        FILE_CHECK(io.init() == NI_NO_MEMORY && io.done());
//        io.uninit();
//    }
//    NetFileIoTestBackend::m_mode = 0;
//    FILE_CHECK(io.init() == NI_OK && io.status() == NI_OK && !io.done());
//    NetFileIoTestBackend* first = NetFileIoTestBackend::m_current;
//    FILE_CHECK(io.init() == NI_INVALID_STATE);
//    uint64_t id = 44;
//    FILE_CHECK(io.nextRequestId(id) == NI_OK && id == 1);
//    NetFileIoRequest request = {};
//    FILE_CHECK(io.submit(request) == NI_INVALID_ARGUMENT && first->m_submitted == 0);
//    request.id = id;
//    request.operation = NET_FILE_IO_WRITE;
//    request.size = 1;
//    FILE_CHECK(io.submit(request) == NI_INVALID_ARGUMENT && first->m_submitted == 0);
//    std::vector<uint8_t> bytes(65536, 0x41);
//    request.bytes = bytes.data();
//    request.size = 65537;
//    FILE_CHECK(io.submit(request) == NI_INVALID_ARGUMENT && first->m_submitted == 0);
//    request.size = 65536;
//    FILE_CHECK(io.submit(request) == NI_OK && first->m_submitted == 1);
//    FILE_CHECK(io.cancel(id) == NI_NOT_FOUND && io.consume(id) == NI_INVALID_STATE);
//    NetFileIoResult result = {};
//    FILE_CHECK(io.peek(result) == NI_NEED_MORE && result.id == 0 && result.bytes == nullptr);
//    first->m_throw = true;
//    FILE_CHECK(io.nextRequestId(id) == NI_NO_MEMORY && id == 0);
//    FILE_CHECK(io.peek(result) == NI_NO_MEMORY && result.id == 0 && result.bytes == nullptr);
//    first->m_throw = false;
//    io.stop();
//    io.uninit();
//    FILE_CHECK(first->m_stopped && !io.done() && io.status() == NI_INVALID_STATE && NetFileIoTestBackend::m_destroyed == 0);
//    first->m_done = true;
//    FILE_CHECK(io.done());
//    io.uninit();
//    FILE_CHECK(NetFileIoTestBackend::m_destroyed == 1 && io.done());
//    io.uninit();
//    FILE_CHECK(NetFileIoTestBackend::m_destroyed == 1);
//    FILE_CHECK(io.init() == NI_OK);
//    first = NetFileIoTestBackend::m_current;
//    NetFileIo other;
//    FILE_CHECK(other.init() == NI_OK && NetFileIoTestBackend::m_current != first);
//    NetFileIoTestBackend* second = NetFileIoTestBackend::m_current;
//    FILE_CHECK(io.nextRequestId(id) == NI_OK && id == 1);
//    FILE_CHECK(other.nextRequestId(id) == NI_OK && id == 1);
//    first->m_done = second->m_done = true;
//    io.uninit();
//    other.uninit();
//    FILE_CHECK(NetFileIoTestBackend::m_destroyed == 3);
//    FILE_CHECK(io.nextRequestId(id) == NI_INVALID_STATE && id == 0);
//
//    for (uint32_t mode = 3; mode <= 20; ++mode)
//    {
//        NetFileIo test;
//        NetFileIoTestBackend::m_owner = &test;
//        NetFileIoTestBackend::m_mode = mode >= 5 && mode <= 6 ? mode : 0;
//        const uint32_t destroyed = NetFileIoTestBackend::m_destroyed;
//        const int32_t initialized = test.init();
//        if (mode == 5 || mode == 6)
//        {
//            FILE_CHECK(initialized == (mode == 5 ? NI_INVALID_ARGUMENT : NI_NO_MEMORY));
//            FILE_CHECK(test.done() && NetFileIoTestBackend::m_destroyed == destroyed + 1);
//            continue;
//        }
//        FILE_CHECK(initialized == NI_OK);
//        NetFileIoTestBackend* backend = NetFileIoTestBackend::m_current;
//        NetFileIoTestBackend::m_mode = mode;
//        NetFileIoRequest request = {};
//        request.id = 7;
//        request.file = 9;
//        request.operation = NET_FILE_IO_CLOSE;
//        NetFileIoResult result = {};
//        uint64_t id = 99;
//        if (mode == 3)
//        {
//            test.stop();
//            FILE_CHECK(test.status() == NI_INVALID_STATE && test.submit(request) == NI_INVALID_STATE);
//            FILE_CHECK(test.nextRequestId(id) == NI_INVALID_STATE && id == 0 && backend->m_submitted == 0);
//        }
//        else if (mode == 4)
//        {
//            FILE_CHECK(test.nextRequestId(id) == NI_LIMIT_EXCEEDED && id == 0);
//            FILE_CHECK(test.peek(result) == NI_NEED_MORE && result.id == 0 && result.bytes == nullptr);
//        }
//        else if (mode == 7)
//        {
//            FILE_CHECK(test.nextRequestId(id) == NI_INVALID_STATE && id == 0);
//        }
//        else if (mode == 8 || mode == 9)
//        {
//            FILE_CHECK(test.submit(request) == (mode == 8 ? NI_NO_MEMORY : NI_INVALID_STATE));
//        }
//        else if (mode == 10)
//        {
//            FILE_CHECK(test.cancel(7) == NI_NO_MEMORY);
//        }
//        else if (mode >= 11 && mode <= 15)
//        {
//            std::memset(&result, 0x41, sizeof(result));
//            FILE_CHECK(test.peek(result) == NI_INVALID_STATE && result.id == 0 && result.size == 0 && result.bytes == nullptr);
//        }
//        else if (mode == 16)
//        {
//            FILE_CHECK(test.consume(7) == NI_NO_MEMORY);
//        }
//        else if (mode == 17)
//        {
//            backend->m_done = true;
//            FILE_CHECK(!test.done());
//            test.uninit();
//            FILE_CHECK(NetFileIoTestBackend::m_destroyed == destroyed && test.status() == NI_INVALID_STATE);
//        }
//        else if (mode == 18)
//        {
//            FILE_CHECK(test.submit(request) == NI_OK && backend->m_stopped && backend->m_submitted == 1);
//            FILE_CHECK(test.submit(request) == NI_INVALID_STATE && test.nextRequestId(id) == NI_INVALID_STATE && id == 0);
//        }
//        else if (mode == 19)
//        {
//            backend->m_done = true;
//            test.uninit();
//            FILE_CHECK(test.init() == NI_CANCELLED && test.nextRequestId(id) == NI_INVALID_STATE && id == 0);
//            backend = NetFileIoTestBackend::m_current;
//        }
//        else if (mode == 20)
//        {
//            backend->m_done = true;
//            test.uninit();
//            FILE_CHECK(NetFileIoTestBackend::m_destroyed == destroyed && test.status() == NI_INVALID_STATE);
//            FILE_CHECK(test.submit(request) == NI_INVALID_STATE);
//        }
//        NetFileIoTestBackend::m_mode = 0;
//        backend->m_done = true;
//        test.uninit();
//        FILE_CHECK(test.done() && NetFileIoTestBackend::m_destroyed == destroyed + (mode == 19 ? 2 : 1));
//        FILE_CHECK(test.init() == NI_OK);
//        NetFileIoTestBackend::m_current->m_done = true;
//        test.uninit();
//    }
//    NetFileIoTestBackend::m_mode = 0;
//    {
//        NetFileIo test;
//        FILE_CHECK(test.init() == NI_OK);
//        NetFileIoTestBackend* backend = NetFileIoTestBackend::m_current;
//        NetFileIoRequest request = {};
//        request.id = 7;
//        request.operation = NET_FILE_IO_CLOSE;
//        std::vector<char> path(4098, 'a');
//        path[4096] = 0;
//        request.path = path.data();
//        FILE_CHECK(test.submit(request) == NI_OK);
//        path[4096] = 'b';
//        path[4097] = 0;
//        FILE_CHECK(test.submit(request) == NI_INVALID_ARGUMENT);
//        test.stop();
//        FILE_CHECK(test.cancel(7) == NI_NOT_FOUND && test.consume(7) == NI_INVALID_STATE);
//        NetFileIoResult result = {};
//        FILE_CHECK(test.peek(result) == NI_NEED_MORE);
//        backend->m_done = true;
//        test.uninit();
//    }
//
//    }
//#if defined(_WIN32) && defined(_DEBUG)
//    _CrtMemState after;
//    _CrtMemCheckpoint(&after);
//    FILE_CHECK(before.lCounts[_NORMAL_BLOCK] == after.lCounts[_NORMAL_BLOCK] && before.lSizes[_NORMAL_BLOCK] == after.lSizes[_NORMAL_BLOCK]);
//#endif
//    std::printf("File facade checks=%u PASS\n", assertions);
//    return 0;
//#undef FILE_CHECK
//}
// FILE_BACKEND_UNIT_TEST_END