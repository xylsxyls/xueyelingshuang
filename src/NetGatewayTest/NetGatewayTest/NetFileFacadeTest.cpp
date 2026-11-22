#include "NetFileFacadeTest.h"
#include "NetFileIoTest.h"
#include "NetInterface/NetInterfaceAPI.h"
#include "NetIsolatedTest.h"
#include <stdexcept>
#include <cstdio>
#include <cstring>
#include <vector>
#if defined(_WIN32) && defined(_DEBUG)
#include <crtdbg.h>
#endif
int NetFileFacadeTest::check()
{
    uint32_t assertions = 0;
#define FILE_CHECK(condition) { ++assertions; if (!(condition)) { std::printf("FAIL line %u: %s\n", static_cast<uint32_t>(__LINE__), #condition); return 1; } }
#if defined(_WIN32) && defined(_DEBUG)
    _CrtMemState before;
    _CrtMemCheckpoint(&before);
#endif
    {
    NetFileIo io;
    FILE_CHECK(io.init() == NI_NOT_SUPPORTED && io.status() == NI_NOT_SUPPORTED && io.done());
    FILE_CHECK(NetFileIo::configureBackend(nullptr) == NI_INVALID_ARGUMENT);
    FILE_CHECK(NetFileIo::configureBackend(NetFileIoTestBackend::create) == NI_OK);
    FILE_CHECK(NetFileIo::configureBackend(NetFileIoTestBackend::create) == NI_INVALID_STATE);
    for (uint32_t mode = 1; mode <= 2; ++mode)
    {
        NetFileIoTestBackend::m_mode = mode;
        FILE_CHECK(io.init() == NI_NO_MEMORY && io.done());
        io.uninit();
    }
    NetFileIoTestBackend::m_mode = 0;
    FILE_CHECK(io.init() == NI_OK && io.status() == NI_OK && !io.done());
    NetFileIoTestBackend* first = NetFileIoTestBackend::m_current;
    FILE_CHECK(io.init() == NI_INVALID_STATE);
    uint64_t id = 44;
    FILE_CHECK(io.nextRequestId(id) == NI_OK && id == 1);
    NetFileIoRequest request = {};
    FILE_CHECK(io.submit(request) == NI_INVALID_ARGUMENT && first->m_submitted == 0);
    request.id = id;
    request.operation = NET_FILE_IO_WRITE;
    request.size = 1;
    FILE_CHECK(io.submit(request) == NI_INVALID_ARGUMENT && first->m_submitted == 0);
    std::vector<uint8_t> bytes(65536, 0x41);
    request.bytes = bytes.data();
    request.size = 65537;
    FILE_CHECK(io.submit(request) == NI_INVALID_ARGUMENT && first->m_submitted == 0);
    request.size = 65536;
    FILE_CHECK(io.submit(request) == NI_OK && first->m_submitted == 1);
    FILE_CHECK(io.cancel(id) == NI_NOT_FOUND && io.consume(id) == NI_INVALID_STATE);
    NetFileIoResult result = {};
    FILE_CHECK(io.peek(result) == NI_NEED_MORE && result.id == 0 && result.bytes == nullptr);
    first->m_throw = true;
    FILE_CHECK(io.nextRequestId(id) == NI_NO_MEMORY && id == 0);
    FILE_CHECK(io.peek(result) == NI_NO_MEMORY && result.id == 0 && result.bytes == nullptr);
    first->m_throw = false;
    io.stop();
    io.uninit();
    FILE_CHECK(first->m_stopped && !io.done() && io.status() == NI_INVALID_STATE && NetFileIoTestBackend::m_destroyed == 0);
    first->m_done = true;
    FILE_CHECK(io.done());
    io.uninit();
    FILE_CHECK(NetFileIoTestBackend::m_destroyed == 1 && io.done());
    io.uninit();
    FILE_CHECK(NetFileIoTestBackend::m_destroyed == 1);
    FILE_CHECK(io.init() == NI_OK);
    first = NetFileIoTestBackend::m_current;
    NetFileIo other;
    FILE_CHECK(other.init() == NI_OK && NetFileIoTestBackend::m_current != first);
    NetFileIoTestBackend* second = NetFileIoTestBackend::m_current;
    FILE_CHECK(io.nextRequestId(id) == NI_OK && id == 1);
    FILE_CHECK(other.nextRequestId(id) == NI_OK && id == 1);
    first->m_done = second->m_done = true;
    io.uninit();
    other.uninit();
    FILE_CHECK(NetFileIoTestBackend::m_destroyed == 3);
    FILE_CHECK(io.nextRequestId(id) == NI_INVALID_STATE && id == 0);

    for (uint32_t mode = 3; mode <= 20; ++mode)
    {
        NetFileIo test;
        NetFileIoTestBackend::m_owner = &test;
        NetFileIoTestBackend::m_mode = mode >= 5 && mode <= 6 ? mode : 0;
        const uint32_t destroyed = NetFileIoTestBackend::m_destroyed;
        const int32_t initialized = test.init();
        if (mode == 5 || mode == 6)
        {
            FILE_CHECK(initialized == (mode == 5 ? NI_INVALID_ARGUMENT : NI_NO_MEMORY));
            FILE_CHECK(test.done() && NetFileIoTestBackend::m_destroyed == destroyed + 1);
            continue;
        }
        FILE_CHECK(initialized == NI_OK);
        NetFileIoTestBackend* backend = NetFileIoTestBackend::m_current;
        NetFileIoTestBackend::m_mode = mode;
        NetFileIoRequest request = {};
        request.id = 7;
        request.file = 9;
        request.operation = NET_FILE_IO_CLOSE;
        NetFileIoResult result = {};
        uint64_t id = 99;
        if (mode == 3)
        {
            test.stop();
            FILE_CHECK(test.status() == NI_INVALID_STATE && test.submit(request) == NI_INVALID_STATE);
            FILE_CHECK(test.nextRequestId(id) == NI_INVALID_STATE && id == 0 && backend->m_submitted == 0);
        }
        else if (mode == 4)
        {
            FILE_CHECK(test.nextRequestId(id) == NI_LIMIT_EXCEEDED && id == 0);
            FILE_CHECK(test.peek(result) == NI_NEED_MORE && result.id == 0 && result.bytes == nullptr);
        }
        else if (mode == 7)
        {
            FILE_CHECK(test.nextRequestId(id) == NI_INVALID_STATE && id == 0);
        }
        else if (mode == 8 || mode == 9)
        {
            FILE_CHECK(test.submit(request) == (mode == 8 ? NI_NO_MEMORY : NI_INVALID_STATE));
        }
        else if (mode == 10)
        {
            FILE_CHECK(test.cancel(7) == NI_NO_MEMORY);
        }
        else if (mode >= 11 && mode <= 15)
        {
            std::memset(&result, 0x41, sizeof(result));
            FILE_CHECK(test.peek(result) == NI_INVALID_STATE && result.id == 0 && result.size == 0 && result.bytes == nullptr);
        }
        else if (mode == 16)
        {
            FILE_CHECK(test.consume(7) == NI_NO_MEMORY);
        }
        else if (mode == 17)
        {
            backend->m_done = true;
            FILE_CHECK(!test.done());
            test.uninit();
            FILE_CHECK(NetFileIoTestBackend::m_destroyed == destroyed && test.status() == NI_INVALID_STATE);
        }
        else if (mode == 18)
        {
            FILE_CHECK(test.submit(request) == NI_OK && backend->m_stopped && backend->m_submitted == 1);
            FILE_CHECK(test.submit(request) == NI_INVALID_STATE && test.nextRequestId(id) == NI_INVALID_STATE && id == 0);
        }
        else if (mode == 19)
        {
            backend->m_done = true;
            test.uninit();
            FILE_CHECK(test.init() == NI_CANCELLED && test.nextRequestId(id) == NI_INVALID_STATE && id == 0);
            backend = NetFileIoTestBackend::m_current;
        }
        else if (mode == 20)
        {
            backend->m_done = true;
            test.uninit();
            FILE_CHECK(NetFileIoTestBackend::m_destroyed == destroyed && test.status() == NI_INVALID_STATE);
            FILE_CHECK(test.submit(request) == NI_INVALID_STATE);
        }
        NetFileIoTestBackend::m_mode = 0;
        backend->m_done = true;
        test.uninit();
        FILE_CHECK(test.done() && NetFileIoTestBackend::m_destroyed == destroyed + (mode == 19 ? 2 : 1));
        FILE_CHECK(test.init() == NI_OK);
        NetFileIoTestBackend::m_current->m_done = true;
        test.uninit();
    }
    NetFileIoTestBackend::m_mode = 0;
    {
        NetFileIo test;
        FILE_CHECK(test.init() == NI_OK);
        NetFileIoTestBackend* backend = NetFileIoTestBackend::m_current;
        NetFileIoRequest request = {};
        request.id = 7;
        request.operation = NET_FILE_IO_CLOSE;
        std::vector<char> path(4098, 'a');
        path[4096] = 0;
        request.path = path.data();
        FILE_CHECK(test.submit(request) == NI_OK);
        path[4096] = 'b';
        path[4097] = 0;
        FILE_CHECK(test.submit(request) == NI_INVALID_ARGUMENT);
        test.stop();
        FILE_CHECK(test.cancel(7) == NI_NOT_FOUND && test.consume(7) == NI_INVALID_STATE);
        NetFileIoResult result = {};
        FILE_CHECK(test.peek(result) == NI_NEED_MORE);
        backend->m_done = true;
        test.uninit();
    }

    }
#if defined(_WIN32) && defined(_DEBUG)
    _CrtMemState after;
    _CrtMemCheckpoint(&after);
    FILE_CHECK(before.lCounts[_NORMAL_BLOCK] == after.lCounts[_NORMAL_BLOCK] && before.lSizes[_NORMAL_BLOCK] == after.lSizes[_NORMAL_BLOCK]);
#endif
    std::printf("File facade checks=%u PASS\n", assertions);
    return 0;
#undef FILE_CHECK
}

void NetFileFacadeTest::run()
{
    if (NetIsolatedTest::run("--isolated-file-case=facade") != 0)
    {
        throw std::runtime_error("file facade isolated test failed");
    }
}