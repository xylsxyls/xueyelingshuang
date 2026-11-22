#include "ProcessWorkBenchmark.h"
#include "CSystem/CSystemAPI.h"
#include <algorithm>
#include <chrono>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>
#ifdef PROCESS_WORK_BENCH_MEMORY_AUDIT
#include "ProcessWorkBenchmarkAudit.h"
#endif
#ifdef _WIN32
#include <Windows.h>
#endif

bool PrepareProcessWorkTestDirectory()
{
#ifdef SHARED_MEMORY_MOBILE
    const char* directory = std::getenv("SHARED_MEMORY_TEST_DIR");
    return directory != nullptr && ProcessWork::setProcessIdentityDirectory(directory) &&
        ProcessWork::setSharedMemoryDirectory(directory) &&
        ProcessWork::setSemaphoreDirectory(directory);
#else
    return true;
#endif
}

static std::atomic<uint64_t> s_storageGrowth(0);

ProcessWorkBenchmarkGrowth::ProcessWorkBenchmarkGrowth(std::atomic<uint64_t>& count) :
m_count(&count)
{

}

uint64_t ProcessWorkBenchmarkGrowth::grow(const ProcessWorkMemoryGrowContext& context)
{
    // 此基准共享池固定不扩容；Post池至少增64槽，其余实际存储按本次所需增量增长。
    return context.m_kind == ProcessWorkPostBuffers ? (std::max)(uint64_t(64), context.m_currentCapacity) : context.m_requiredGrowth;
}

void ProcessWorkBenchmarkGrowth::grown(const ProcessWorkMemoryGrowth& growth)
{
    (void)growth;
    m_count->fetch_add(1, std::memory_order_relaxed);
}

/** 安装实际存储增长观察，环境开关仅选择额度模式，不改变预分配规模
@param [in,out] memory 基准使用的配置，初始化前调用
*/
static void ObserveStorage(ProcessWorkMemoryConfig& memory)
{
    if (std::getenv("PROCESS_WORK_BENCH_UNLIMITED_LOCAL") != nullptr)
    {
        memory.m_localQueueBytes = -1;
    }
    memory.m_policy.reset(new ProcessWorkBenchmarkGrowth(s_storageGrowth));
    // 仅在池真正增长后调用，计时热路径不增加逐消息原子操作。
    memory.m_queuePool.m_onGrowth = [](size_t added, uint64_t sequence)
    {
        (void)added;
        (void)sequence;
        s_storageGrowth.fetch_add(1, std::memory_order_relaxed);
    };
    memory.m_freeQueuePool.m_onGrowth = memory.m_queuePool.m_onGrowth;
}

/** 同一系统启动周期的单调纳秒，用于合并真实进程的起止时刻
@return Windows为QPC计数换算的纳秒，其他平台为steady_clock纳秒
*/
static int64_t BenchmarkNow()
{
#ifdef _WIN32
    LARGE_INTEGER frequency = {}, counter = {};
    bbCheck(QueryPerformanceFrequency(&frequency) != FALSE && frequency.QuadPart > 0 &&
        QueryPerformanceCounter(&counter) != FALSE, "high resolution clock unavailable");
    const int64_t seconds = counter.QuadPart / frequency.QuadPart;
    const int64_t remainder = counter.QuadPart % frequency.QuadPart;
    return seconds * 1000000000LL + static_cast<int64_t>(static_cast<long double>(remainder) * 1000000000.0L / frequency.QuadPart);
#else
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
#endif
}

ProcessWorkBenchmarkTask::ProcessWorkBenchmarkTask(const std::function<void()>& work, std::atomic<bool>& failure) :
m_work(work), m_failure(&failure)
{

}

void ProcessWorkBenchmarkTask::doTask()
{
    try
    {
        m_work();
    }
    catch (const std::exception& error)
    {
        std::cerr << "producer exception: " << error.what() << std::endl;
        m_failure->store(true);
    }
    catch (...)
    {
        std::cerr << "producer unknown exception" << std::endl;
        m_failure->store(true);
    }
}

void ProcessWorkBenchmarkTask::stopTask()
{
    std::cerr << "producer unexpectedly stopped" << std::endl;
    m_failure->store(true);
}

ProcessWorkBenchmarkReceiver::ProcessWorkBenchmarkReceiver(int32_t streams, int32_t count, int32_t bytes) :
m_warmup(0), m_drained(0), m_finished(0), m_bad(0), m_firstBadValue(0), m_firstBadExpected(-1), m_firstBadPid(0),
m_firstBadLength(0), m_firstBadReceived(0), m_armed(false), m_threadsPerProcess(0), m_count(count), m_bytes(bytes),
m_received(0), m_next(streams, 0), m_ended(streams, 0),
m_body(bytes == 6 ? std::string("123456") : std::string(bytes, 'x'))
{

}

void ProcessWorkBenchmarkReceiver::receive(int32_t pid, const char* data, int32_t length, MessageType type)
{
    const int32_t value = static_cast<int32_t>(type);
    if (value == INT_MAX && !m_armed.load(std::memory_order_acquire))
    {
        ++m_warmup;
        return;
    }
    const bool ending = value >= INT_MAX - static_cast<int32_t>(m_next.size()) && value < INT_MAX;
    const int32_t stream = ending ? INT_MAX - 1 - value : value / m_count;
    const int32_t sequence = value % m_count;
    if (!m_armed.load(std::memory_order_acquire) || value < 0 || stream >= static_cast<int32_t>(m_next.size()) ||
        m_threadsPerProcess <= 0 || stream / m_threadsPerProcess >= static_cast<int32_t>(m_pids.size()) ||
        m_ended[stream] || (ending ? m_next[stream] != m_count : sequence != m_next[stream] || sequence >= m_count) || length != m_bytes ||
        pid != m_pids[stream / m_threadsPerProcess] ||
        (length == 0 ? data != nullptr : data == nullptr ||
            std::memcmp(data, m_body.data(), static_cast<size_t>(length)) != 0))
    {
        if (m_bad.load(std::memory_order_relaxed) == 0)
        {
            m_firstBadValue = value;
            m_firstBadExpected = stream >= 0 && stream < static_cast<int32_t>(m_next.size()) ? m_next[stream] : -1;
            m_firstBadPid = pid;
            m_firstBadLength = length;
            m_firstBadReceived = m_received;
        }
        ++m_bad;
        return;
    }
    if (ending)
    {
        m_ended[stream] = 1;
        m_drained.fetch_add(1, std::memory_order_release);
        return;
    }
    ++m_next[stream];
    ++m_received;
    if (m_received == static_cast<int32_t>(m_next.size()) * m_count)
    {
        m_finished.store(BenchmarkNow(), std::memory_order_release);
    }
}

bool ProcessWorkBenchmarkReceiver::verified() const
{
    if (m_bad.load() != 0 || m_received != static_cast<int32_t>(m_next.size()) * m_count ||
        m_drained.load() != static_cast<int32_t>(m_next.size()))
    {
        return false;
    }
    for (size_t i = 0; i < m_next.size(); ++i)
    {
        if (m_next[i] != m_count || !m_ended[i])
        {
            return false;
        }
    }
    return true;
}

void ProcessWorkBenchmarkReceiver::selfTest()
{
    for (int32_t mode = 0; mode < 6; ++mode)
    {
        ProcessWorkBenchmarkReceiver ledger(1, 2, 64);
        ledger.m_pids.push_back(bbPid());
        ledger.m_threadsPerProcess = 1;
        ledger.m_armed.store(true);
        std::string body(64, 'x');
        ledger.receive(bbPid(), body.data(), 64, static_cast<MessageType>(0));
        if (mode == 1)
        {
            body[32] = 'y';
        }
        if (mode != 2)
        {
            ledger.receive(bbPid(), body.data(), 64, static_cast<MessageType>(mode == 3 ? 0 : 1));
        }
        ledger.receive(bbPid(), body.data(), 64, static_cast<MessageType>(INT_MAX - 1));
        if (mode == 4 || mode == 5)
        {
            ledger.receive(bbPid(), body.data(), 64, static_cast<MessageType>(mode == 4 ? 1 : INT_MAX - 1));
        }
        bbCheck(ledger.verified() == (mode == 0), "benchmark ledger accepted damaged, missing or duplicate data");
    }
}

void ProcessWorkBenchmarkReceiver::bufferFull(int32_t count)
{
    if (count <= 0)
    {
        ++m_bad;
    }
}

/** 子进程准备线程和正文后等待统一起点，生产与父进程消费同时发生
@param [in] argv 已由main检查长度的参数
@return 子进程退出状态
*/
static int BenchmarkProducer(char** argv)
{
    const int32_t target = static_cast<int32_t>(bbUnsigned(argv[2], INT_MAX));
    const std::string prefix = argv[3];
    const int32_t process = static_cast<int32_t>(bbUnsigned(argv[4], 7));
    const int32_t threads = static_cast<int32_t>(bbUnsigned(argv[5], 32));
    const int32_t count = static_cast<int32_t>(bbUnsigned(argv[6], INT_MAX));
    const int32_t bytes = static_cast<int32_t>(bbUnsigned(argv[7], 65536));
    const int32_t mode = static_cast<int32_t>(bbUnsigned(argv[8], 3));
    const std::string name = argv[9];
    const int32_t warmupBlocks = static_cast<int32_t>(bbUnsigned(argv[10], 4194304));
    bbCheck(target > 0 && threads > 0 && count > 0 && warmupBlocks > 0 &&
        static_cast<int64_t>(process + 1) * threads * count < INT_MAX - 256, "invalid producer parameters");
    const int32_t warmup = (std::max)(16, mode == 2 ? (warmupBlocks + threads - 1) / threads : warmupBlocks);
    ProcessWork& ipc = ProcessWork::instance();
    CTaskThreadManager& manager = CTaskThreadManager::instance();
    std::atomic<bool> failure(false), go(false);
    std::atomic<int32_t> ready(0);
    std::vector<int64_t> starts(threads, 0), ends(threads, 0);
    std::vector<uint64_t> retries(threads, 0);
    std::vector<uint32_t> workers;
    workers.reserve(static_cast<size_t>(threads));
    const std::string body = bytes == 6 ? std::string("123456") : std::string(bytes, 'x');
    ProcessWorkChannel direct;
    int64_t prepareNanoseconds = 0;
    try
    {
        if (std::getenv("PROCESS_WORK_BENCH_PREPARE_SEND") != nullptr)
        {
            const int64_t started = BenchmarkNow();
            bbCheck(ipc.prepareSend(target, -1, true), "benchmark mapping preparation failed");
            prepareNanoseconds = BenchmarkNow() - started;
        }
        if (mode == 3)
        {
            bbCheck(direct.openSender(target), "diagnostic direct channel setup failed");
        }
        if (mode == 2)
        {
            ProcessWorkMemoryConfig memory;
            memory.m_localQueueInitialCapacity = 16777216;
            memory.m_postBufferReserve = 16777216;
            const char* slotBytes = std::getenv("PROCESS_WORK_BENCH_POST_SLOT_BYTES");
            if (slotBytes != nullptr)
            {
                bbCheck(std::string(slotBytes) == "64" || std::string(slotBytes) == "128", "invalid Post slot bytes");
                memory.m_postBufferSlotBytes = std::string(slotBytes) == "128" ? 128 : 64;
            }
            // 各构建都预留1GiB正文池；变更槽步长不额外增加基准内存预算。
            memory.m_postBufferReserve = static_cast<uint32_t>((uint64_t(1) << 30) / memory.m_postBufferSlotBytes);
            memory.m_localQueueBytes = 4ULL * 1024 * 1024 * 1024;
            ObserveStorage(memory);
            bbCheck(ipc.configurePostMemory(memory), "post pool preparation failed");
            bbCheck(ipc.initPostThread(), "benchmark post thread initialization failed");
        }
        for (int32_t thread = 0; thread < threads; ++thread)
        {
            std::shared_ptr<CTask> task(new ProcessWorkBenchmarkTask([&, thread]()
            {
                for (int32_t i = 0; i < warmup; ++i)
                {
                    const int32_t result = mode == 2 ?
                        ipc.postResult(target, body.data(), bytes, static_cast<MessageType>(INT_MAX)) : mode == 1 ?
                        ipc.sendResult(name, body.data(), bytes, static_cast<MessageType>(INT_MAX)) :
                        ipc.sendResult(target, body.data(), bytes, static_cast<MessageType>(INT_MAX));
                    if (result != (mode == 2 ? PROCESS_WORK_POSTED : PROCESS_WORK_SENT))
                    {
                        std::cerr << "warmup status=" << result << std::endl;
                        bbCheck(false, "ordinary warmup admission failed");
                    }
                }
                ++ready;
                while (!go.load(std::memory_order_acquire) && !failure.load())
                {
                    std::this_thread::yield();
                }
                const int32_t stream = process * threads + thread;
                starts[thread] = BenchmarkNow();
                for (int32_t i = 0; i < count && !failure.load(std::memory_order_relaxed); ++i)
                {
                    const MessageType type = static_cast<MessageType>(stream * count + i);
                    if (mode == 0 || mode == 1)
                    {
                        const int32_t result = mode == 0 ? ipc.sendResult(target, body.data(), bytes, type) :
                            ipc.sendResult(name, body.data(), bytes, type);
                        if (result != PROCESS_WORK_SENT)
                        {
                            bbCheck(false, "ordinary send failed or leaked Busy/capacity");
                        }
                    }
                    else if (mode == 3)
                    {
                        int32_t result = PROCESS_WORK_BUSY;
                        for (;;)
                        {
                            result = direct.trySend(body.data(), bytes, static_cast<int32_t>(type));
                            if (result != PROCESS_WORK_BUSY
                                && result != PROCESS_WORK_CAPACITY && result != PROCESS_WORK_RESOURCE
                                )
                            {
                                break;
                            }
                            bbCheck(direct.waitForSpace(), "diagnostic direct channel closed");
                        }
                        if (result != PROCESS_WORK_SENT)
                        {
                            bbCheck(false, "diagnostic direct send failed");
                        }
                    }
                    else
                    {
                        // 普通接口负责容量和竞争重试，基准不能用外部重试掩盖错误返回。
                        const int32_t result = ipc.postResult(target, body.data(), bytes, type);
                        if (result != PROCESS_WORK_POSTED)
                        {
                            bbCheck(false, "ordinary post failed or leaked Busy/capacity");
                        }
                    }
                }
                ends[thread] = BenchmarkNow();
                // 每个实际发送线程随后发布屏障；不计入吞吐，但必须经过同一发送路径。
                const MessageType ending = static_cast<MessageType>(INT_MAX - 1 - stream);
                int32_t endResult = PROCESS_WORK_INVALID;
                if (mode == 3)
                {
                    do
                    {
                        endResult = direct.trySend(body.data(), bytes, static_cast<int32_t>(ending));
                        if (endResult == PROCESS_WORK_BUSY || endResult == PROCESS_WORK_CAPACITY || endResult == PROCESS_WORK_RESOURCE)
                        {
                            bbCheck(direct.waitForSpace(), "diagnostic end barrier channel closed");
                        }
                    } while (endResult == PROCESS_WORK_BUSY || endResult == PROCESS_WORK_CAPACITY || endResult == PROCESS_WORK_RESOURCE);
                }
                else
                {
                    endResult = mode == 2 ? ipc.postResult(target, body.data(), bytes, ending) :
                        mode == 1 ? ipc.sendResult(name, body.data(), bytes, ending) : ipc.sendResult(target, body.data(), bytes, ending);
                }
                bbCheck(endResult == (mode == 2 ? PROCESS_WORK_POSTED : PROCESS_WORK_SENT), "benchmark end barrier failed");
            }, failure));
            const uint32_t id = manager.init();
            bbCheck(id != 0, "benchmark producer initialization failed");
            workers.push_back(id);
            const std::shared_ptr<CTaskThread> worker = manager.getThreadInterface(id);
            bbCheck(worker && worker->postTask(task, 1), "benchmark producer not admitted");
        }
        const int32_t preparationTimeout = static_cast<int32_t>((std::min)(int64_t(600000),
            int64_t(30000) + static_cast<int64_t>(threads) * warmup / 1000));
        bbWait([&ready, &failure, threads]() { return failure.load() || ready.load() == threads; }, "benchmark warmup", preparationTimeout);
        bbCheck(!failure.load(), "benchmark warmup failed");
        bbWrite(prefix + ".ready" + bbNumber(process), "1");
        bbWait([&prefix]() { return bbRead(prefix + ".go") == "1"; }, "benchmark start", 30000);
#ifdef PROCESS_WORK_BENCH_MEMORY_AUDIT
        ProcessWorkBenchmarkAudit::begin();
#endif
        s_storageGrowth.store(0, std::memory_order_relaxed);
        go.store(true, std::memory_order_release);
        for (size_t i = 0; i < workers.size(); ++i)
        {
            bbCheck(manager.waitForEnd(workers[i]).m_status == CTaskThreadOk, "benchmark producer join failed");
        }
        if (mode == 2)
        {
            ipc.waitForPostEnd();
        }
        bbCheck(!failure.load(), "benchmark producer interrupted/failed");
#ifdef PROCESS_WORK_BENCH_MEMORY_AUDIT
        ProcessWorkBenchmarkAudit::finish();
        ProcessWorkBenchmarkAudit::write(prefix + ".audit" + bbNumber(process));
#endif
        ipc.uninitPostThread();
        ipc.clear();
        std::cout << "mapping_prepare_ns=" << prepareNanoseconds << std::endl;
        std::ostringstream report;
        uint64_t retryCount = 0;
        for (size_t i = 0; i < retries.size(); ++i)
        {
            retryCount += retries[i];
        }
        report << *std::min_element(starts.begin(), starts.end()) << ' ' << *std::max_element(ends.begin(), ends.end()) << ' ' << retryCount;
        report << ' ' << s_storageGrowth.load(std::memory_order_relaxed);
        report << ' ' << prepareNanoseconds;
        bbWrite(prefix + ".result" + bbNumber(process), report.str());
        return 0;
    }
    catch (...)
    {
        failure.store(true);
        go.store(true, std::memory_order_release);
        for (size_t i = 0; i < workers.size(); ++i)
        {
            manager.uninit(workers[i]);
        }
        ipc.uninitPostThread();
        ipc.clear();
        throw;
    }
}

/** 真实多进程生产和接收同时运行，初始化、正文构造和预热不计入吞吐
@param [in] argv 主进程测试参数
@return 主进程退出状态
*/
static int BenchmarkParent(char** argv)
{
    const int32_t processes = static_cast<int32_t>(bbUnsigned(argv[1], 8));
    const int32_t threads = static_cast<int32_t>(bbUnsigned(argv[2], 32));
    const int32_t count = static_cast<int32_t>(bbUnsigned(argv[3], INT_MAX));
    const int32_t bytes = static_cast<int32_t>(bbUnsigned(argv[4], 65536));
    const int32_t blocks = static_cast<int32_t>(bbUnsigned(argv[5], 4194304));
    const int32_t mode = static_cast<int32_t>(bbUnsigned(argv[6], 3));
    const int64_t total = static_cast<int64_t>(processes) * threads * count;
    bbCheck(processes >= 1 && processes <= 8 && threads >= 1 && threads <= 32 && count > 0 &&
        total < INT_MAX - 256 && bytes >= 0 && bytes <= 65536 && blocks >= 1 && blocks <= 4194304 && mode >= 0 && mode <= 3, "invalid benchmark parameters");
    const std::string prefix = bbTempPath("-pw-benchmark");
    const std::string name = "pwbench_" + bbNumber(bbPid());
    ProcessWork& ipc = ProcessWork::instance();
    ProcessWorkBenchmarkReceiver receiver(processes * threads, count, bytes);
    receiver.m_threadsPerProcess = threads;
    bbCheck(ipc.addProcessReceiveCallback(&receiver), "benchmark receive callback registration failed");
    ProcessWorkMemoryConfig memory;
    // 预留256MiB接收批次，避免消费线程短暂被调度出去时把池增长计入吞吐。
    memory.m_receiveBatchReserve = 4096;
    memory.m_localQueueInitialCapacity = 8192;
    // 固定额度对照使用相同128MiB接收上限，初始化预留实际批次和队列节点。
    memory.m_localQueueBytes = 128ULL * 1024 * 1024;
    ObserveStorage(memory);
    bbCheck(ipc.setReceiveName(name) && ipc.initReceive(bytes + 12, blocks, 0, 0, 0, 0, memory), "benchmark receive init failed");
    std::vector<std::shared_ptr<BlackBoxChild>> children;
    try
    {
        for (int32_t process = 0; process < processes; ++process)
        {
            std::vector<std::string> arguments;
            arguments.push_back("--producer");
            arguments.push_back(bbNumber(bbPid()));
            arguments.push_back(prefix);
            arguments.push_back(bbNumber(process));
            arguments.push_back(bbNumber(threads));
            arguments.push_back(bbNumber(count));
            arguments.push_back(bbNumber(bytes));
            arguments.push_back(bbNumber(mode));
            arguments.push_back(name);
            arguments.push_back(bbNumber(blocks));
            std::shared_ptr<BlackBoxChild> child(new BlackBoxChild);
            child->start(arguments, prefix + ".log" + bbNumber(process));
            children.push_back(child);
            receiver.m_pids.push_back(child->pid());
        }
        for (int32_t process = 0; process < processes; ++process)
        {
            bbWait([&prefix, &children, process]()
            {
                const int32_t status = children[process]->poll();
                if (status >= 0)
                {
                    std::ostringstream error;
                    error << "producer exited during preparation: status=" << status;
#ifdef _WIN32
                    HANDLE native = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, children[process]->pid());
                    DWORD code = 0;
                    if (native != nullptr)
                    {
                        if (GetExitCodeProcess(native, &code))
                        {
                            error << " native=0x" << std::hex << code;
                        }
                        CloseHandle(native);
                    }
#endif
                    error << " log=" << prefix << ".log" << process;
                    bbCheck(false, error.str());
                }
                return bbRead(prefix + ".ready" + bbNumber(process)) == "1";
            }, "all benchmark producers ready", static_cast<int32_t>((std::min)(int64_t(600000),
                int64_t(30000) + static_cast<int64_t>(processes) * threads * blocks / 1000)));
        }
        const int32_t warmup = (std::max)(16, mode == 2 ? (blocks + threads - 1) / threads : blocks);
        bbWait([&receiver, processes, threads, warmup]() { return receiver.m_warmup.load() == processes * threads * warmup; }, "warmup drained");
        receiver.m_armed.store(true, std::memory_order_release);
        s_storageGrowth.store(0, std::memory_order_relaxed);
#ifdef PROCESS_WORK_BENCH_MEMORY_AUDIT
        ProcessWorkBenchmarkAudit::begin();
#endif
        bbWrite(prefix + ".go", "1");
        bbWait([&receiver, &children, processes, threads]()
        {
            for (size_t i = 0; i < children.size(); ++i)
            {
                const int code = children[i]->poll();
                bbCheck(code == -1 || code == 0, "benchmark producer exited with failure");
            }
            return receiver.m_bad.load() != 0 || (receiver.m_finished.load(std::memory_order_acquire) != 0 &&
                receiver.m_drained.load(std::memory_order_acquire) == processes * threads);
        }, "benchmark deliveries", 180000);
        if (receiver.m_bad.load() != 0)
        {
            std::cerr << "first mismatch: type=" << receiver.m_firstBadValue << " expected_sequence=" << receiver.m_firstBadExpected
                << " pid=" << receiver.m_firstBadPid << " length=" << receiver.m_firstBadLength
                << " preceding_verified=" << receiver.m_firstBadReceived << std::endl;
        }
        bbCheck(receiver.m_bad.load() == 0, "benchmark sequence/payload/PID mismatch");
#ifdef PROCESS_WORK_BENCH_MEMORY_AUDIT
        ProcessWorkBenchmarkAudit::finish();
        ProcessWorkBenchmarkAudit::write(prefix + ".audit-receiver");
#endif
        int64_t firstStart = INT64_MAX, lastEnd = 0;
        uint64_t retries = 0;
        uint64_t producerGrowth = 0;
        int64_t preparationTotal = 0, preparationMaximum = 0;
        for (int32_t process = 0; process < processes; ++process)
        {
            bbCheck(children[process]->wait(10000) == 0, "benchmark producer failed: " + bbRead(prefix + ".log" + bbNumber(process)));
#ifdef PROCESS_WORK_BENCH_MEMORY_AUDIT
            std::cout << "AUDIT producer " << process << ' ' << bbRead(prefix + ".audit" + bbNumber(process)) << std::endl;
#endif
            std::istringstream input(bbRead(prefix + ".result" + bbNumber(process)));
            int64_t start = 0, end = 0;
            uint64_t retryCount = 0;
            input >> start >> end >> retryCount;
            uint64_t growth = 0;
            input >> growth;
            bbCheck(!input.fail(), "missing producer growth audit");
            producerGrowth += growth;
            int64_t preparation = 0;
            input >> preparation;
            bbCheck(!input.fail() && preparation >= 0, "missing producer preparation duration");
            preparationTotal += preparation;
            preparationMaximum = (std::max)(preparationMaximum, preparation);
            retries += retryCount;
            bbCheck(start > 0 && end > start, "invalid producer timestamps");
            firstStart = (std::min)(firstStart, start);
            lastEnd = (std::max)(lastEnd, end);
        }
        const double production = static_cast<double>(lastEnd - firstStart) / 1000000000.0;
        const double consumption = static_cast<double>(receiver.m_finished.load() - firstStart) / 1000000000.0;
        bbCheck(production > 0 && consumption > 0, "invalid cross-process time interval");
        const int32_t finalBlocks = ipc.receiveBufferCount();
        bbCheck(finalBlocks == blocks, "fixed shared pool grew during measurement");
        ipc.uninitReceive();
        bbCheck(receiver.verified(), "benchmark final delivery ledger failed");
#ifdef PROCESS_WORK_BENCH_MEMORY_AUDIT
        std::cout << "AUDIT receiver " << bbRead(prefix + ".audit-receiver") << std::endl;
#endif
        std::cout << std::fixed << std::setprecision(6) << "{\"processes\":" << processes << ",\"threads_per_process\":" << threads
            << ",\"bytes\":" << bytes << ",\"blocks\":" << blocks << ",\"mode\":" << mode << ",\"messages\":" << total
            << ",\"production_seconds\":" << production << ",\"consumption_seconds\":" << consumption
            << ",\"produced_per_second\":" << total / production << ",\"consumed_per_second\":" << total / consumption
            << ",\"uncommitted_retries\":" << retries << ",\"final_blocks\":" << finalBlocks
            << ",\"mapping_prepare_total_ms\":" << preparationTotal / 1000000.0
            << ",\"mapping_prepare_max_ms\":" << preparationMaximum / 1000000.0
            << ",\"post_slot_bytes\":" << (mode != 2 ? 0 :
                (std::getenv("PROCESS_WORK_BENCH_POST_SLOT_BYTES") == nullptr ? ProcessWorkMemoryConfig().m_postBufferSlotBytes :
                    (std::string(std::getenv("PROCESS_WORK_BENCH_POST_SLOT_BYTES")) == "128" ? 128 : 64)))
            << ",\"producer_pool_growth\":" << producerGrowth << ",\"receiver_pool_growth\":" << s_storageGrowth.load(std::memory_order_relaxed)
            << ",\"unlimited_local\":" << (std::getenv("PROCESS_WORK_BENCH_UNLIMITED_LOCAL") != nullptr ? "true" : "false")
            << ",\"verified\":true}" << std::endl;
        return 0;
    }
    catch (...)
    {
        ipc.uninitReceive();
        for (int32_t i = 0; i < static_cast<int32_t>(children.size()); ++i)
        {
            std::cerr << bbRead(prefix + ".log" + bbNumber(i));
        }
        throw;
    }
}

int RunProcessWorkBenchmark(int argc, char** argv)
{
    try
    {
        if (argc == 11 && std::string(argv[1]) == "--producer")
        {
            return BenchmarkProducer(argv);
        }
        bbCheck(argc == 7, "usage: processes threads_per_process messages_per_thread bytes blocks mode(0pid/1name/2post)");
        return BenchmarkParent(argv);
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cerr << "unexpected benchmark exception" << std::endl;
        return 1;
    }
}
#ifndef PROCESS_WORK_BENCH_IN_SUITE
/** 独立性能程序入口；合入测试套件时由测试工程main分派
@param [in] argc 命令行参数数量
@param [in] argv 父进程工作量参数或驱动生成的子进程参数
@return 0表示账本验证完成，非0表示配置、执行或校验失败
*/
int main(int argc, char** argv)
{
    return PrepareProcessWorkTestDirectory() ? RunProcessWorkBenchmark(argc, argv) : 1;
}
#endif