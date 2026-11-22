#include "ProcessWorkCoreTests.h"
#include "ProcessWorkTest.h"
#include "ProcessWork/ProcessWorkAtomic.h"
#include "ProcessWork/ProcessWorkStorage.h"
#include "ProcessWork/ProcessWorkSharedQueue.h"
#include <algorithm>
#include <iostream>
#include <mutex>
#include <thread>
#ifdef _WIN32
#include <Windows.h>
#include <Psapi.h>
#endif

void runProcessWorkRuntimeControl()
{
#ifdef _WIN32
    // 对照组完全不调用ProcessWork及基础库，单独观察VS2013线程/互斥运行库。
    DWORD baseline = 0, peak = 0;
    for (int32_t cycle = 0; cycle < 240; ++cycle)
    {
        std::mutex mutex;
        std::atomic<int32_t> phase(0);
        std::thread worker([&mutex, &phase]()
        {
            std::lock_guard<std::mutex> held(mutex);
            phase.store(1);
            Sleep(1);
        });
        while (phase.load() == 0)
        {
            std::this_thread::yield();
        }
        {
            std::lock_guard<std::mutex> held(mutex);
        }
        worker.join();
        DWORD handles = 0;
        bbCheck(GetProcessHandleCount(GetCurrentProcess(), &handles) != FALSE, "runtime handle query failed");
        if (cycle == 19)
        {
            baseline = handles;
        }
        if (cycle >= 20)
        {
            peak = (std::max)(peak, handles);
            if (cycle % 20 == 0 || handles > baseline + 4)
            {
                std::cout << "runtime_control_cycle=" << cycle << " handles=" << handles << " baseline=" << baseline << std::endl;
            }
        }
    }
    std::cout << "runtime_control_baseline=" << baseline << " peak=" << peak << std::endl;
#endif
}

/** 改写已提交正文的协议头，验证接收端停止且不把损坏消息当作已消费
@param [in] bytes 正文长度，分别覆盖内联和独立映射
@param [in] field 要改写的包头字段，0为PID、1为类型、2为长度
@param [in] value 损坏的字段值
*/
static void CheckCorruptHeader(int32_t bytes, int32_t field, int32_t value)
{
    PublicChannelScope channel(2, 0, 0, 0, 0);
    const std::string body(static_cast<size_t>(bytes), 'x');
    bbCheck(channel.m_sender.trySend(body.data(), bytes, 1) == PROCESS_WORK_SENT, "header fixture send failed");
    SharedMemory directoryMemory("P3D" + bbNumber(bbPid()));
    ProcessWorkSharedDirectory* directory = static_cast<ProcessWorkSharedDirectory*>(directoryMemory.writeWithoutLock());
    bbCheck(directory != nullptr, "header fixture directory missing");
    const uint32_t session = static_cast<uint32_t>(ProcessWorkAtomic::load(&directory->m_session));
    ProcessWorkStorage storage(bbPid(), directory->m_identity, session, 76, 2, false, ProcessWorkMemoryConfig());
    ProcessWorkSharedStream* stream = storage.stream(1);
    bbCheck(stream != nullptr, "header fixture stream missing");
    // 初始流使用随登记创建的首段，发送端尚未填满或切换该段。
    const uint64_t cell = static_cast<uint64_t>(ProcessWorkAtomic::load(&stream->m_first.m_cells[0]));
    const uint32_t index = static_cast<uint32_t>(cell & 0xffffff);
    bbCheck(index > 0 && index <= 2, "header fixture body index invalid");
    ProcessWorkSharedBlock* block = storage.block(index);
    bbCheck(block != nullptr, "header fixture body missing");
    std::unique_ptr<SharedMemory> temporary;
    char* packet = (cell & 0x1000000) != 0 ? block->m_inlineData :
        storage.data(index, static_cast<uint32_t>(ProcessWorkAtomic::load(&block->m_generation)), temporary);
    bbCheck(packet != nullptr, "header fixture mapping missing");
    std::memcpy(packet + static_cast<size_t>(field) * sizeof(int32_t), &value, sizeof(value));
    ProcessWorkReceiveQueue received;
    bbCheck(channel.m_receiver.pump(received, channel.m_full) == 0 && !channel.m_receiver.ready(),
        "invalid committed header not closed");
    char* delivered = nullptr;
    bbCheck(!received.take(&delivered, 0), "corrupt header was delivered");
    bbCheck(channel.m_sender.trySend("x", 1, 2) == PROCESS_WORK_UNAVAILABLE, "corrupt header channel accepted new message");
}

void testProcessWorkCorruption()
{
    // 每种损坏使用独立会话，避免上一次停止状态让后续检查无条件通过。
    for (int32_t bytes = 6; bytes <= 37; bytes += 31)
    {
        CheckCorruptHeader(bytes, 0, 0);
        CheckCorruptHeader(bytes, 1, -1);
        CheckCorruptHeader(bytes, 2, -1);
        CheckCorruptHeader(bytes, 2, bytes == 6 ? 37 : 65);
    }
    PublicChannelScope channel(2, 0, 0, 0, 0);
    bbCheck(channel.m_sender.trySend("x", 1, 1) == PROCESS_WORK_SENT, "corruption fixture send failed");
    SharedMemory directoryMemory("P3D" + bbNumber(bbPid()));
    ProcessWorkSharedDirectory* directory = static_cast<ProcessWorkSharedDirectory*>(directoryMemory.writeWithoutLock());
    bbCheck(directory != nullptr, "corruption fixture directory missing");
    const uint32_t session = static_cast<uint32_t>(ProcessWorkAtomic::load(&directory->m_session));
    SharedMemory controlMemory(ProcessWorkChannel::objectName('C', bbPid(), directory->m_identity, session));
    ProcessWorkSharedControl* control = static_cast<ProcessWorkSharedControl*>(controlMemory.writeWithoutLock());
    bbCheck(control != nullptr, "corruption fixture control missing");
    ProcessWorkStorage storage(bbPid(), directory->m_identity, session, 76, 2, false, ProcessWorkMemoryConfig());
    ProcessWorkSharedStream* stream = storage.stream(1);
    bbCheck(stream != nullptr && ProcessWorkAtomic::load(&stream->m_owner) > 0, "corruption fixture stream missing");
    ProcessWorkSharedQueue queue(storage, *control);
    const uint32_t head = static_cast<uint32_t>(ProcessWorkAtomic::load(&stream->m_head));
    ProcessWorkSharedNode* sentinel = queue.node(head - 1);
    bbCheck(sentinel != nullptr, "corruption fixture sentinel missing");
    // 仅测试进程改写真实协议，不在生产代码嵌入注入入口。
    int64_t* cell = &sentinel->m_cells[stream->m_headSlot];
    const uint64_t value = static_cast<uint64_t>(ProcessWorkAtomic::load(cell));
    ProcessWorkAtomic::store(cell, static_cast<int64_t>((value & 0xffffffff00000000ULL) | 3));
    bbCheck(channel.pump() == 0 && !channel.m_receiver.ready(), "invalid committed index not closed");
    for (int32_t i = 0; i < 10; ++i)
    {
        bbCheck(channel.pump() == 0, "stopped corrupt channel delivered a packet");
    }
    bbCheck(channel.m_sender.trySend("x", 1, 2) == PROCESS_WORK_UNAVAILABLE, "corrupt channel accepted new message");
}

void testProcessWorkMappingFailure()
{
    PublicChannelScope channel(1, 0, 0, 0, 0);
    SharedMemory directoryMemory("P3D" + bbNumber(bbPid()));
    ProcessWorkSharedDirectory* directory = static_cast<ProcessWorkSharedDirectory*>(directoryMemory.writeWithoutLock());
    bbCheck(directory != nullptr, "mapping fixture directory missing");
    const uint32_t session = static_cast<uint32_t>(ProcessWorkAtomic::load(&directory->m_session));
    ProcessWorkStorage storage(bbPid(), directory->m_identity, session, 76, 1, false, ProcessWorkMemoryConfig());
    ProcessWorkSharedBlock* block = storage.block(1);
    bbCheck(block != nullptr && ProcessWorkAtomic::load(&block->m_owner) == 0, "mapping fixture not idle");
    const int64_t generation = ProcessWorkAtomic::load(&block->m_generation);
    // 仅测试映射指定一个不存在的正文代次，真实发送入口必须处理系统打开失败。
    ProcessWorkAtomic::store(&block->m_generation, generation + 1);
    const std::string body(37, 'x');
    const int32_t result = channel.m_sender.send(body.data(), static_cast<int32_t>(body.size()), 7);
    const int64_t ownerAfterFailure = ProcessWorkAtomic::load(&block->m_owner);
    ProcessWorkAtomic::store(&block->m_generation, generation);
    bbCheck(result == PROCESS_WORK_NO_MEMORY && ownerAfterFailure == 0, "mapping failure became Busy, hung, or leaked reservation");
    bbCheck(channel.m_sender.send(body.data(), static_cast<int32_t>(body.size()), 8) == PROCESS_WORK_SENT,
        "restored mapping could not send");
    ProcessWorkReceiveQueue received;
    bbCheck(channel.m_receiver.pump(received, channel.m_full) == 1, "mapping failure committed or lost a packet");
    char* packet = nullptr;
    bbCheck(received.take(&packet, 0) && packet != nullptr, "restored message missing");
    int32_t header[3] = {};
    std::memcpy(header, packet, sizeof(header));
    const bool valid = header[0] == bbPid() && header[1] == 8 && header[2] == 37 && std::memcmp(packet + 12, body.data(), 37) == 0;
    received.release(packet);
    bbCheck(valid && !received.take(&packet, 0), "mapping retry duplicated or corrupted delivery");
}

void testProcessWorkSendFailureKinds()
{
    ProcessWorkChannel receiver, sender;
    ProcessWorkReceiveQueue received;
    std::atomic<int32_t> full(0);
    bbCheck(receiver.openReceiver(76, 1, 0, 0, 0, 0), "failure-kind receiver setup");
    receiver.publish();
    bbCheck(sender.openSender(bbPid()), "failure-kind sender setup");
    bbCheck(sender.trySend("123456", 6, 1) == PROCESS_WORK_RESOURCE, "unapproved registration mislabeled CAS Busy");
    bbCheck(receiver.pump(received, full) == 0, "unapproved trySend committed");
    bbCheck(sender.trySend("123456", 6, 2) == PROCESS_WORK_SENT, "approved trySend failed");
    for (int32_t attempt = 0; attempt < 100; ++attempt)
    {
        bbCheck(sender.trySend("123456", 6, 3) == PROCESS_WORK_CAPACITY, "full capacity mislabeled CAS Busy");
        bbCheck(sender.trySend("123456", 6, 3, true, false) == PROCESS_WORK_CAPACITY, "competition retry must preserve capacity result");
    }
    bbCheck(receiver.pump(received, full) == 1, "full retries altered admission");
    char* packet = nullptr;
    bbCheck(received.take(&packet, 0) && packet != nullptr, "admitted message absent");
    int32_t header[3] = {};
    std::memcpy(header, packet, sizeof(header));
    const bool valid = header[1] == 2 && header[2] == 6 && std::memcmp(packet + 12, "123456", 6) == 0;
    received.release(packet);
    bbCheck(valid && !received.take(&packet, 0), "failure-kind retries duplicated or corrupted payload");
    bbCheck(sender.send("123456", 6, 4) == PROCESS_WORK_SENT, "ordinary send failed after space restored");
    bbCheck(receiver.pump(received, full) == 1 && received.take(&packet, 0), "ordinary send missing after restore");
    received.release(packet);
}

void testProcessWorkResourcePlateau()
{
    ProcessWork& ipc = ProcessWork::instance();
    uint64_t baselineHandles = 0, maximumHandles = 0, baselineBytes = 0, maximumBytes = 0;
    for (int32_t cycle = 0; cycle < 120; ++cycle)
    {
        PacketLog log;
        IpcScope scope;
        scope.start(&log, receiveName("resource"), 76, 8);
        bbCheck(ipc.initPostThread(), "post worker initialization failed");
        for (int32_t i = 0; i < 16; ++i)
        {
            bbCheck(ipc.post(receiveName("resource"), std::string("resource"), static_cast<MessageType>(i)), "resource post rejected");
        }
        ipc.waitForPostEnd();
        waitPackets(log, 16);
        ipc.uninitPostThread();
        scope.close();
        ipc.clear();
        bbCheck(ipc.postPendingBytes() == 0 && ipc.receivePendingBytes() == 0 &&
            CTaskThreadManager::instance().count() == 0, "cycle retained charged packets or workers");
#ifdef _WIN32
        DWORD handles = 0;
        PROCESS_MEMORY_COUNTERS_EX memory = {};
        memory.cb = sizeof(memory);
        bbCheck(GetProcessHandleCount(GetCurrentProcess(), &handles) != FALSE &&
            GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&memory), sizeof(memory)) != FALSE,
            "resource query failed");
        if (cycle == 19)
        {
            baselineHandles = handles;
            baselineBytes = memory.PrivateUsage;
        }
        if (cycle >= 20)
        {
            const DWORD immediate = handles;
            // VS2013的std::thread/std::mutex使用Concurrency运行库；线程join后，
            // ExternalContextBase仍由系统线程池异步关闭其复制的线程句柄。
            // 不再调用任何ProcessWork维护接口，只等待OS可观测值收敛，阈值不放宽。
            if (handles > baselineHandles + 4)
            {
                bbWait([&handles, baselineHandles]()
                {
                    bbCheck(GetProcessHandleCount(GetCurrentProcess(), &handles) != FALSE, "settled handle query failed");
                    return handles <= baselineHandles + 4;
                }, "runtime asynchronous handle cleanup", 1000);
                std::cout << "cycle=" << cycle << " immediate_handles=" << immediate << " settled_handles=" << handles << std::endl;
            }
            maximumHandles = (std::max)(maximumHandles, static_cast<uint64_t>(handles));
            maximumBytes = (std::max)(maximumBytes, static_cast<uint64_t>(memory.PrivateUsage));
            if (cycle % 10 == 0 || handles > baselineHandles + 4)
            {
                std::cout << "cycle=" << cycle << " handles=" << handles << " baseline=" << baselineHandles
                    << " private_bytes=" << memory.PrivateUsage << std::endl;
            }
            bbCheck(handles <= baselineHandles + 4, "shared memory/semaphore/thread handle growth");
            bbCheck(memory.PrivateUsage <= baselineBytes + 16ULL * 1024 * 1024, "private memory failed to plateau after warmup");
        }
#endif
    }
#ifdef _WIN32
    std::cout << "warmup_handles=" << baselineHandles << " maximum_handles=" << maximumHandles
        << " warmup_private_bytes=" << baselineBytes << " maximum_private_bytes=" << maximumBytes << std::endl;
#endif
}