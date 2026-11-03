#include "NetNativeFileIo.h"
#include "NetWorkMacro.h"

#ifndef __EMSCRIPTEN__
#include "NetNativeFileIoState.h"
#include <cstring>

NetNativeFileIo::NetNativeFileIo() :
m_thread(0)
{

}

NetNativeFileIo::~NetNativeFileIo()
{
    uninit();
}

NetFileIoBackend* NetNativeFileIo::create()
{
    return new NetNativeFileIo();
}

int32_t NetNativeFileIo::init()
{
    if (m_thread != 0 || m_state)
    {
        return NI_INVALID_STATE;
    }
    try
    {
        m_state.reset(new NetFileIoState());
        if (!m_state->m_gate.valid())
        {
            m_state.reset();
            return NI_INVALID_STATE;
        }
        m_thread = CTaskThreadManager::Instance().Init();
        auto executor = CTaskThreadManager::Instance().GetThreadInterface(m_thread);
        if (!executor)
        {
            uninit();
            return NI_INVALID_STATE;
        }
        executor->PostTask(std::shared_ptr<CTask>(new NetFileIoTask(m_state)));
        executor.reset();
        return NI_OK;
    }
    catch (...)
    {
        uninit();
        return NI_NO_MEMORY;
    }
}

int32_t NetNativeFileIo::nextRequestId(uint64_t& id)
{
    id = 0;
    if (!m_state || m_state->m_stop.load())
    {
        return NI_INVALID_STATE;
    }
    std::lock_guard<NetWorkGate> lock(m_state->m_gate);
    if (m_state->m_stop.load()) { return NI_INVALID_STATE; }
    if (m_state->m_allocatedRequest == UINT64_MAX)
    {
        return NI_LIMIT_EXCEEDED;
    }
    id = ++m_state->m_allocatedRequest;
    return NI_OK;
}

int32_t NetNativeFileIo::submit(const NetFileIoRequest& request)
{
    if (!m_state || m_state->m_stop.load())
    {
        return NI_INVALID_STATE;
    }
    if (request.id == 0 || request.operation < NET_FILE_IO_OPEN_READ || request.operation > NET_FILE_IO_OPEN_TEMPORARY || request.size > 65536 ||
        (request.operation == NET_FILE_IO_WRITE && request.size != 0 && request.bytes == nullptr) || request.flags > 1 ||
        (request.path != nullptr && std::strlen(request.path) > 4096) ||
        (request.destination != nullptr && std::strlen(request.destination) > 4096))
    {
        return NI_INVALID_ARGUMENT;
    }
    const uint32_t inputSize = request.operation == NET_FILE_IO_WRITE ? request.size : 0;
    std::lock_guard<NetWorkGate> lock(m_state->m_gate);
    // StopTask可与接纳竞争；锁外预检不能允许停止之后追加无人消费的工作。
    if (m_state->m_stop.load()) { return NI_INVALID_STATE; }
    if (request.id <= m_state->m_lastRequest)
    {
        return NI_INVALID_ARGUMENT;
    }
    m_state->m_lastRequest = request.id;
    if (request.id > m_state->m_allocatedRequest)
    {
        m_state->m_allocatedRequest = request.id;
    }
    if (m_state->m_entries.size() >= 64 || inputSize > 2097152U - m_state->m_inputBytes)
    {
        return NI_LIMIT_EXCEEDED;
    }
    try
    {
        const std::shared_ptr<NetFileIoEntry> entry(new NetFileIoEntry(request));
        m_state->m_entries.insert(std::make_pair(request.id, entry));
        try
        {
            m_state->m_queued.push_back(entry);
        }
        catch (...)
        {
            m_state->m_entries.erase(request.id);
            return NI_NO_MEMORY;
        }
        m_state->m_inputBytes += inputSize;
        m_state->m_gate.notify();
        return NI_OK;
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t NetNativeFileIo::cancel(uint64_t id)
{
    if (!m_state)
    {
        return NI_INVALID_STATE;
    }
    std::lock_guard<NetWorkGate> lock(m_state->m_gate);
    const auto entry = m_state->m_entries.find(id);
    if (entry == m_state->m_entries.end())
    {
        return NI_NOT_FOUND;
    }
    entry->second->m_cancelled.store(true);
    m_state->m_gate.notify();
    return NI_OK;
}

int32_t NetNativeFileIo::peek(NetFileIoResult& result)
{
    std::memset(&result, 0, sizeof(result));
    if (!m_state)
    {
        return NI_INVALID_STATE;
    }
    std::lock_guard<NetWorkGate> lock(m_state->m_gate);
    if (m_state->m_entries.empty())
    {
        return NI_NEED_MORE;
    }
    const auto found = m_state->m_entries.begin();
    if (!found->second->m_completed)
    {
        return NI_NEED_MORE;
    }
    result = found->second->m_result;
    result.bytes = found->second->m_output.empty() ? nullptr : found->second->m_output.data();
    result.size = static_cast<uint32_t>(found->second->m_output.size());
    return NI_READY;
}

int32_t NetNativeFileIo::consume(uint64_t id)
{
    if (!m_state)
    {
        return NI_INVALID_STATE;
    }
    std::lock_guard<NetWorkGate> lock(m_state->m_gate);
    if (m_state->m_entries.empty())
    {
        return NI_INVALID_STATE;
    }
    const auto entry = m_state->m_entries.begin();
    if (entry->first != id || !entry->second->m_completed)
    {
        return NI_INVALID_STATE;
    }
    m_state->m_inputBytes -= static_cast<uint32_t>(entry->second->m_input.size());
    m_state->m_entries.erase(entry);
    return NI_OK;
}

void NetNativeFileIo::stop()
{
    if (m_state)
    {
        m_state->stop();
    }
}

bool NetNativeFileIo::done() const
{
    return !m_state || m_state->m_done.load();
}

void NetNativeFileIo::uninit()
{
    stop();
    if (m_thread != 0)
    {
        CTaskThreadManager::Instance().WaitForEnd(m_thread);
        m_thread = 0;
    }
    m_state.reset();
}

#if 0 // NATIVE_FILE_BACKEND_UNIT_TEST_BEGIN
#include "NetNativeFileIo.h"
#include "CSystem/CSystemAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include <chrono>
#include <cstdio>
#include <vector>

/** 向真实后端提交一个工作并有界等待，返回元数据，借用字节不带出consume
@param [in,out] io 本用例独占后端
@param [in,out] request 分配编号并提交的请求
@param [out] result 完成元数据，bytes清零
@return 是否取得本次唯一完成，不表示result.result成功
*/
static bool NativeFileTestWork(NetNativeFileIo& io, NetFileIoRequest& request, NetFileIoResult& result)
{
    if (io.nextRequestId(request.id) != NI_OK || io.submit(request) != NI_OK)
    {
        return false;
    }
    const std::chrono::steady_clock::time_point deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (std::chrono::steady_clock::now() < deadline)
    {
        NetFileIoResult current = {};
        if (io.peek(current) == NI_READY)
        {
            if (current.id != request.id)
            {
                return false;
            }
            result = current;
            result.bytes = nullptr;
            return io.consume(current.id) == NI_OK;
        }
        CSystem::Sleep(1);
    }
    return false;
}

/** 等待自己的后端关闭句柄，不阻塞产品网络执行者
@param [in,out] io 用例独占后端
@return 5秒内是否完成
*/
static bool NativeFileTestStop(NetNativeFileIo& io)
{
    io.stop();
    const std::chrono::steady_clock::time_point deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!io.done() && std::chrono::steady_clock::now() < deadline)
    {
        CSystem::Sleep(1);
    }
    return io.done();
}

int main()
{
    uint32_t assertions = 0;
#define NATIVE_FILE_CHECK(condition) { ++assertions; if (!(condition)) { std::printf("FAIL line %u: %s\n", static_cast<uint32_t>(__LINE__), #condition); return 1; } }
    const std::string root = CSystem::joinPath(CStringManager::AnsiToUtf8(CSystem::GetCurrentExePath()), "native-temp-unit");
    const std::string first = CSystem::joinPath(root, "new-parent/first.tmp");
    const std::string second = CSystem::joinPath(root, "second.tmp");
    const std::string published = CSystem::joinPath(root, "published.bin");
    const std::wstring firstWide = CStringManager::Utf8ToUnicode(first);
    const std::wstring secondWide = CStringManager::Utf8ToUnicode(second);
    const std::wstring publishedWide = CStringManager::Utf8ToUnicode(published);
    NetNativeFileIo io;
    NATIVE_FILE_CHECK(io.init() == NI_OK);
    NetFileIoRequest request = {};
    NetFileIoResult result = {};
    request.operation = NET_FILE_IO_OPEN_TEMPORARY;
    request.path = first.c_str();
    NATIVE_FILE_CHECK(NativeFileTestWork(io, request, result) && result.result == NI_OK && result.file != 0);
    NATIVE_FILE_CHECK(CSystem::fileExists(firstWide));
    uint64_t file = result.file;
    const uint8_t expected[] = { 0, 7, 0xff, 0, 0x51, 0x82, 0x36, 1 };
    request = NetFileIoRequest();
    request.operation = NET_FILE_IO_WRITE;
    request.file = file;
    request.flags = 1;
    request.bytes = expected;
    request.size = sizeof(expected);
    NATIVE_FILE_CHECK(NativeFileTestWork(io, request, result) && result.result == NI_OK && result.length == sizeof(expected));
    request = NetFileIoRequest();
    request.operation = NET_FILE_IO_PUBLISH;
    request.file = file;
    request.path = first.c_str();
    request.destination = published.c_str();
    NATIVE_FILE_CHECK(NativeFileTestWork(io, request, result) && result.result == NI_OK);
    NATIVE_FILE_CHECK(!CSystem::fileExists(firstWide) && CSystem::fileExists(publishedWide));
    std::vector<uint8_t> actual;
    NATIVE_FILE_CHECK(CSystem::readBinaryFile(publishedWide, 64, actual) && actual == std::vector<uint8_t>(expected, expected + sizeof(expected)));
    // 既有文件独占创建失败后不能获得删除权
    request = NetFileIoRequest();
    request.operation = NET_FILE_IO_OPEN_TEMPORARY;
    request.path = published.c_str();
    NATIVE_FILE_CHECK(NativeFileTestWork(io, request, result) && result.result != NI_OK);
    NATIVE_FILE_CHECK(CSystem::readBinaryFile(publishedWide, 64, actual) && actual == std::vector<uint8_t>(expected, expected + sizeof(expected)));
    // 错配源路径不能提交或删除另一份文件，拥有的临时文件必须回收
    request.path = second.c_str();
    NATIVE_FILE_CHECK(NativeFileTestWork(io, request, result) && result.result == NI_OK);
    file = result.file;
    request = NetFileIoRequest();
    request.operation = NET_FILE_IO_PUBLISH;
    request.file = file;
    request.path = published.c_str();
    request.destination = first.c_str();
    NATIVE_FILE_CHECK(NativeFileTestWork(io, request, result) && result.result != NI_OK);
    NATIVE_FILE_CHECK(!CSystem::fileExists(secondWide) && CSystem::fileExists(publishedWide));
    // 取消未发布的句柄，stop必须删除自己创建的临时文件
    request = NetFileIoRequest();
    request.operation = NET_FILE_IO_OPEN_TEMPORARY;
    request.path = second.c_str();
    NATIVE_FILE_CHECK(NativeFileTestWork(io, request, result) && result.result == NI_OK);
    NATIVE_FILE_CHECK(CSystem::fileExists(secondWide));
    NATIVE_FILE_CHECK(NativeFileTestStop(io));
    io.uninit();
    NATIVE_FILE_CHECK(!CSystem::fileExists(secondWide) && CSystem::fileExists(publishedWide));
    NATIVE_FILE_CHECK(CSystem::deleteFile(publishedWide));
    std::printf("Native file temporary ownership PASS %u assertions\n", assertions);
    return 0;
#undef NATIVE_FILE_CHECK
}

#endif // NATIVE_FILE_BACKEND_UNIT_TEST_END

#if 0 // NATIVE_FILE_CLEANUP_UNIT_BEGIN
#include "NetNativeFileIoState.h"
#include <CStringManager/CStringManagerAPI.h>
#include <chrono>
#include <cstdio>
#if defined(_WIN32) && defined(_DEBUG) && !defined(NATIVE_CLEANUP_NO_HEAP)
#include <crtdbg.h>
// 测试专用分配预算：-1不限，0开始拒绝普通堆分配
static std::atomic<int32_t> s_cleanupRejectAfter(-1);
// 当前注入窗口观察到的普通堆分配次数
static std::atomic<uint32_t> s_cleanupAllocations(0);
/** 隔离main统计或拒绝普通堆分配
@param [in] operation CRT操作
@param [in] pointer CRT传入的堆块，只在钩子返回前借用；本用例未使用时忽略
@param [in] size CRT请求的字节数，是否参与注入判定由本用例决定
@param [in] block CRT堆块类别，只对普通业务堆块注入
@param [in] request CRT分配序号，本用例不使用
@param [in] filename CRT传入的来源文件名，本用例不读取
@param [in] line 来源代码行号，本用例不使用
@return 非零允许分配，0拒绝
*/
static int __cdecl CleanupMemoryHook(int operation, void* pointer, size_t size, int block, long request, const unsigned char* filename, int line)
{
    (void)pointer;
    (void)size;
    (void)request;
    (void)filename;
    (void)line;
    if ((operation == _HOOK_ALLOC || operation == _HOOK_REALLOC) && block == _NORMAL_BLOCK)
    {
        ++s_cleanupAllocations;
        if (s_cleanupRejectAfter == 0)
        {
            return 0;
        }
        if (s_cleanupRejectAfter > 0)
        {
            --s_cleanupRejectAfter;
        }
    }
    return 1;
}

#endif

/** 在独占测试目录检查Unicode临时文件的低内存关闭和工作线程回收
@return 是否全部通过
*/
static bool NativeCleanupAudit()
{
    uint32_t checks = 0;
#define CLEANUP_CHECK(value) do { ++checks; if (!(value)) { std::printf("Native cleanup line=%d FAIL\n", __LINE__); return false; } } while (0)
#if defined(_WIN32) && defined(_DEBUG) && !defined(NATIVE_CLEANUP_NO_HEAP)
    {
        NetFileIoState cold;
        CLEANUP_CHECK(cold.m_gate.valid());
        bool escaped = false;
        s_cleanupRejectAfter = 0;
        s_cleanupAllocations = 0;
        const _CRT_ALLOC_HOOK oldHook = _CrtSetAllocHook(CleanupMemoryHook);
        try
        {
            cold.stop();
            cold.stop();
        }
        catch (...)
        {
            escaped = true;
        }
        _CrtSetAllocHook(oldHook);
        s_cleanupRejectAfter = -1;
        CLEANUP_CHECK(!escaped && cold.m_stop.load() && s_cleanupAllocations == 0);
    }
#endif
    const std::string path = CSystem::canonicalPathUtf8(CStringManager::UnicodeToUtf8(L"native-cleanup-\u6e05\u7406.tmp"), false);
    const std::wstring wide = CStringManager::Utf8ToUnicode(path);
    CLEANUP_CHECK(!path.empty() && !CSystem::fileExists(wide));
    for (uint32_t mode = 0; mode < 3; ++mode)
    {
        std::unique_ptr<NetFileIoHandle> file(new NetFileIoHandle());
        CLEANUP_CHECK(file->openTemporary(path.c_str()) && CSystem::fileExists(wide));
        bool escaped = false;
        bool cleaned = false;
#if defined(_WIN32) && defined(_DEBUG) && !defined(NATIVE_CLEANUP_NO_HEAP)
        s_cleanupRejectAfter = 0;
        s_cleanupAllocations = 0;
        const _CRT_ALLOC_HOOK oldHook = _CrtSetAllocHook(CleanupMemoryHook);
#endif
        try
        {
            if (mode == 0)
            {
                cleaned = file->close() && file->close();
            }
            else
            {
                file.reset();
                cleaned = true;
            }
        }
        catch (...)
        {
            escaped = true;
        }
#if defined(_WIN32) && defined(_DEBUG) && !defined(NATIVE_CLEANUP_NO_HEAP)
        _CrtSetAllocHook(oldHook);
        s_cleanupRejectAfter = -1;
        CLEANUP_CHECK(s_cleanupAllocations == 0);
#endif
        CLEANUP_CHECK(!escaped && cleaned && !CSystem::fileExists(wide));
    }
#if defined(_WIN32) && defined(_DEBUG) && !defined(NATIVE_CLEANUP_NO_HEAP)
    _CrtMemState before, after;
    _CrtMemCheckpoint(&before);
    uint32_t rejected = 0;
    uint32_t accepted = 0;
    for (int32_t position = 0; position < 80; ++position)
    {
        NetFileIoHandle file;
        bool opened = false;
        s_cleanupRejectAfter = position;
        const _CRT_ALLOC_HOOK oldHook = _CrtSetAllocHook(CleanupMemoryHook);
        try
        {
            opened = file.openTemporary(path.c_str());
        }
        catch (...)
        {
            // 创建路径的分配允许失败，但尚未取得临时文件删除责任。
        }
        _CrtSetAllocHook(oldHook);
        s_cleanupRejectAfter = -1;
        if (opened)
        {
            ++accepted;
            CLEANUP_CHECK(CSystem::fileExists(wide));
        }
        else
        {
            ++rejected;
            CLEANUP_CHECK(!CSystem::fileExists(wide));
        }
        CLEANUP_CHECK(file.close() && !CSystem::fileExists(wide));
    }
    _CrtMemCheckpoint(&after);
    CLEANUP_CHECK(rejected != 0 && accepted != 0);
    CLEANUP_CHECK(before.lCounts[_NORMAL_BLOCK] == after.lCounts[_NORMAL_BLOCK] && before.lSizes[_NORMAL_BLOCK] == after.lSizes[_NORMAL_BLOCK]);
    std::printf("NATIVE_CLEANUP allocation_positions=80 rejected=%u accepted=%u cleanup_allocations=0 heap_delta=0\n", rejected, accepted);
#endif
    // 实际异步文件执行者持有临时句柄；在持续分配失败期间停止并释放。
    NetNativeFileIo io;
    CLEANUP_CHECK(io.init() == NI_OK);
    NetFileIoRequest request = {};
    request.operation = NET_FILE_IO_OPEN_TEMPORARY;
    request.path = path.c_str();
    CLEANUP_CHECK(io.nextRequestId(request.id) == NI_OK && io.submit(request) == NI_OK);
    NetFileIoResult result = {};
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (io.peek(result) != NI_READY && std::chrono::steady_clock::now() < deadline)
    {
        CSystem::sleepPrecise(1);
    }
    CLEANUP_CHECK(result.id == request.id && result.result == NI_OK && result.file != 0);
    CLEANUP_CHECK(io.consume(result.id) == NI_OK && CSystem::fileExists(wide));
#if defined(_WIN32) && defined(_DEBUG) && !defined(NATIVE_CLEANUP_NO_HEAP)
    s_cleanupRejectAfter = 0;
    const _CRT_ALLOC_HOOK oldHook = _CrtSetAllocHook(CleanupMemoryHook);
#endif
    const auto stopDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    io.stop();
    while (!io.done() && std::chrono::steady_clock::now() < stopDeadline)
    {
        CSystem::sleepPrecise(1);
    }
    const bool done = io.done();
#if defined(_WIN32) && defined(_DEBUG) && !defined(NATIVE_CLEANUP_NO_HEAP)
    _CrtSetAllocHook(oldHook);
    s_cleanupRejectAfter = -1;
#endif
    io.uninit();
    CLEANUP_CHECK(done && !CSystem::fileExists(wide));
    // 覆盖通知先到/执行者先等待两种时序；每次生命周期独立，不保留后台线程。
    for (uint32_t round = 0; round < 16; ++round)
    {
        NetNativeFileIo idle;
        CLEANUP_CHECK(idle.init() == NI_OK);
        if ((round & 1U) != 0)
        {
            CSystem::sleepPrecise(1);
        }
        idle.stop();
        idle.stop();
        const auto until = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while (!idle.done() && std::chrono::steady_clock::now() < until)
        {
            CSystem::sleepPrecise(1);
        }
        CLEANUP_CHECK(idle.done());
        idle.uninit();
    }
    // 结果未consume前仍计入容量；执行者再快也不能使未消费结果无限增长。
    for (uint32_t mode = 0; mode < 2; ++mode)
    {
        NetNativeFileIo bounded;
        CLEANUP_CHECK(bounded.init() == NI_OK);
        const uint32_t capacity = mode == 0 ? 64U : 32U;
        std::vector<uint8_t> input(mode == 0 ? 0 : 65536, 0x53);
        for (uint32_t item = 0; item < capacity; ++item)
        {
            NetFileIoRequest queued = {};
            queued.operation = mode == 0 ? NET_FILE_IO_SIZE : NET_FILE_IO_WRITE;
            queued.file = UINT64_MAX;
            queued.size = static_cast<uint32_t>(input.size());
            queued.bytes = input.empty() ? nullptr : input.data();
            CLEANUP_CHECK(bounded.nextRequestId(queued.id) == NI_OK && queued.id == item + 1);
            CLEANUP_CHECK(bounded.submit(queued) == NI_OK);
        }
        NetFileIoRequest overflow = {};
        overflow.operation = mode == 0 ? NET_FILE_IO_SIZE : NET_FILE_IO_WRITE;
        overflow.size = static_cast<uint32_t>(input.size());
        overflow.bytes = input.empty() ? nullptr : input.data();
        CLEANUP_CHECK(bounded.nextRequestId(overflow.id) == NI_OK);
        CLEANUP_CHECK(bounded.submit(overflow) == NI_LIMIT_EXCEEDED);
        bounded.stop();
        const auto until = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while (!bounded.done() && std::chrono::steady_clock::now() < until)
        {
            CSystem::sleepPrecise(1);
        }
        CLEANUP_CHECK(bounded.done());
        for (uint32_t item = 0; item < capacity; ++item)
        {
            NetFileIoResult completed = {};
            CLEANUP_CHECK(bounded.peek(completed) == NI_READY && completed.id == item + 1);
            CLEANUP_CHECK(completed.result == NI_NOT_FOUND || completed.result == NI_CANCELLED);
            CLEANUP_CHECK(bounded.consume(completed.id) == NI_OK);
        }
        NetFileIoResult empty = {};
        CLEANUP_CHECK(bounded.peek(empty) == NI_NEED_MORE);
        CLEANUP_CHECK(bounded.submit(overflow) == NI_INVALID_STATE);
        bounded.uninit();
    }
    std::printf("Native file cleanup checks=%u PASS\n", checks);
    return true;
#undef CLEANUP_CHECK
}

#ifndef NATIVE_CLEANUP_NO_MAIN
int main()
{
    try
    {
        return NativeCleanupAudit() ? 0 : 1;
    }
    catch (...)
    {
        std::puts("Native file cleanup unexpected exception FAIL");
        return 2;
    }
}

#endif
#endif // NATIVE_FILE_CLEANUP_UNIT_END
#endif // !__EMSCRIPTEN__