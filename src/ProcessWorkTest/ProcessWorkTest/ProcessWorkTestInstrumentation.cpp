#include "ProcessWorkTestAutomation.h"

/** 在一个私有源文件中替换唯一锚点并保存
@param [in] path 私有源码路径
@param [in] before 唯一锚点
@param [in] after 替换内容
*/
static void Patch(const std::string& path, const std::string& before, const std::string& after)
{
    std::string text = ProcessWorkTestAutomation::plain(bbRead(path));
    ProcessWorkTestAutomation::replaceOnce(text, before, after);
    ProcessWorkTestAutomation::writeCode(path, text);
}

void ProcessWorkTestAutomation::instrument(const std::string& output, const std::string& mode, const std::string& benchmark)
{
    if (mode == "memory")
    {
        const char* patches[][3] = {
            { "ProcessWork/ProcessWorkChannel.cpp", "    if (block == nullptr)\n    {\n        ProcessWorkAtomic::replace(&m_control->m_demand", "    if (block == nullptr)\n    {\n        ProcessWorkBenchmarkAudit::hit(0);\n        ProcessWorkAtomic::replace(&m_control->m_demand" },
            { "ProcessWork/ProcessWorkChannel.cpp", "        if (failure == PROCESS_WORK_CAPACITY)\n        {", "        if (failure == PROCESS_WORK_CAPACITY)\n        {\n            ProcessWorkBenchmarkAudit::hit(7);" },
            { "ProcessWork/ProcessWorkSharedQueue.cpp", "        if (!assigned)\n        {", "        if (!assigned)\n        {\n            ProcessWorkBenchmarkAudit::hit(7);" },
            { "ProcessWork/ProcessWorkTime.cpp", "bool ProcessWorkTime::waitCapacity(uint32_t& attempts)\n{", "bool ProcessWorkTime::waitCapacity(uint32_t& attempts)\n{\n    ProcessWorkBenchmarkAudit::hit(1);" },
            { "ProcessWork/ProcessWorkReceiveQueue.cpp", "            if (m_reserved - m_knownReleased > maximum - charge)\n            {\n                return false;", "            if (m_reserved - m_knownReleased > maximum - charge)\n            {\n                ProcessWorkBenchmarkAudit::hit(9);\n                return false;" },
            { "ProcessWork/ProcessWorkReceiveQueue.cpp", "        batch = new (std::nothrow) ProcessWorkReceiveBatch(capacity);", "        ProcessWorkBenchmarkAudit::hit(2);\n        batch = new (std::nothrow) ProcessWorkReceiveBatch(capacity);" },
            { "ProcessWork/ProcessWorkLocalQueue.cpp", "            allocation = new (std::nothrow) char[bytes + prefix];", "            ProcessWorkBenchmarkAudit::hit(3);\n            allocation = new (std::nothrow) char[bytes + prefix];" },
            { "ProcessWork/ProcessWork.cpp", "\n        if (!m_postStarted || m_waitEndPost.load() || SendTask::currentStopFlag() != nullptr ||", "\n        ProcessWorkBenchmarkAudit::hit(8);\n        if (!m_postStarted || m_waitEndPost.load() || SendTask::currentStopFlag() != nullptr ||" },
            { "LockFreeQueue/LockFreeQueueArenaAllocation.inl", "LockFreeQueueResult LockFreeQueueArena<T>::grow(Record* record, bool required)\n{", "LockFreeQueueResult LockFreeQueueArena<T>::grow(Record* record, bool required)\n{\n    ProcessWorkBenchmarkAudit::hit(4);" },
            { "LockFreeMap/LockFreeMapPool.inl", "LockFreeMapStatus LockFreeMapPool<T>::grow(bool initial)\n{", "LockFreeMapStatus LockFreeMapPool<T>::grow(bool initial)\n{\n    if (!initial)\n    {\n        ProcessWorkBenchmarkAudit::hit(5);\n    }" },
            { "ProcessWork/ProcessWorkStorage.cpp", "        std::unique_ptr<ProcessWorkPage> candidate(new ProcessWorkPage);", "        ProcessWorkBenchmarkAudit::hit(6);\n        std::unique_ptr<ProcessWorkPage> candidate(new ProcessWorkPage);" }
        };
        for (size_t i = 0; i < sizeof(patches) / sizeof(patches[0]); ++i)
        {
            const std::string relative(patches[i][0]);
            const int32_t copies = relative.find(".inl") != std::string::npos ? 2 : 1;
            for (int32_t copy = 0; copy < copies; ++copy)
            {
                const std::string path = output + (copy == 0 ? "/source/" : "/include/") + relative;
                std::string text = plain(bbRead(path));
                replaceOnce(text, patches[i][1], patches[i][2]);
                const std::string include("#include \"ProcessWorkBenchmarkAudit.h\"\n");
                if (text.find(include) == std::string::npos)
                {
                    text = include + text;
                }
                writeCode(path, text);
            }
        }
        return;
    }
    bbCheck(mode == "profile", "unknown diagnostic mode");
    const std::string declaration = "\n#ifdef _MSC_VER\n__declspec(thread) uint64_t pw_profile[9] = {};\n#else\nthread_local uint64_t pw_profile[9] = {};\n#endif\n";
    const std::string external = "\n#ifdef _MSC_VER\nextern __declspec(thread) uint64_t pw_profile[9];\n#else\nextern thread_local uint64_t pw_profile[9];\n#endif\n";
    const std::string channel = output + "/source/ProcessWork/ProcessWorkChannel.cpp";
    Patch(channel, "#include <utility>", "#include <utility>" + declaration);
    Patch(channel, "    const uint32_t start = ChannelReservationStart(m_selfPid);", "    ++pw_profile[0];\n    const uint32_t start = ChannelReservationStart(m_selfPid);");
    Patch(channel, "        ProcessWorkSharedBlock* candidate = producer->m_bodyPage == nullptr ? nullptr :", "        ++pw_profile[1];\n        ProcessWorkSharedBlock* candidate = producer->m_bodyPage == nullptr ? nullptr :");
    Patch(channel, "    if (block == nullptr)\n    {\n        ProcessWorkAtomic::replace(&m_control->m_demand", "    if (block == nullptr)\n    {\n        ++pw_profile[2];\n        ProcessWorkAtomic::replace(&m_control->m_demand");
    Patch(channel, "bool ProcessWorkChannel::waitForSpaceImpl(int32_t timeout, bool probeSpace)\n{", "bool ProcessWorkChannel::waitForSpaceImpl(int32_t timeout, bool probeSpace)\n{\n    ++pw_profile[8];");
    Patch(channel, "            contended = true;", "            ++pw_profile[7];\n            contended = true;");
    const std::string queue = output + "/source/ProcessWork/ProcessWorkSharedQueue.cpp";
    Patch(queue, "#include \"ProcessWorkAtomic.h\"", "#include \"ProcessWorkAtomic.h\"" + external);
    Patch(queue, "        ProcessWorkSharedNode* value = node(candidate);", "        ++pw_profile[3];\n        ProcessWorkSharedNode* value = node(candidate);");
    const std::string anchor = "    if (index == UINT32_MAX && !reserve(ProcessWorkAtomic::load(&stream.m_owner), extent, hint, index, false, contended))\n    {";
    Patch(queue, anchor, anchor + "\n        ++pw_profile[4];");
    Patch(queue, "    ProcessWorkAtomic::store(&stream.m_writeOffset, int64_t(1));", "    ProcessWorkAtomic::store(&stream.m_writeOffset, int64_t(1));\n    ++pw_profile[6];");
    for (int32_t copy = 0; copy < 2; ++copy)
    {
        const std::string path = output + (copy == 0 ? "/source/" : "/include/") + "ProcessWork/ProcessWorkSharedQueue.inl";
        Patch(path, "#include \"ProcessWorkAtomic.h\"", "#include \"ProcessWorkAtomic.h\"" + external);
        Patch(path, "        ProcessWorkAtomic::store(&stream.m_writeOffset, int64_t(offset + 1));", "        ProcessWorkAtomic::store(&stream.m_writeOffset, int64_t(offset + 1));\n        ++pw_profile[6];");
        Patch(path, "        ProcessWorkReadState* reader = m_active[m_readCursor];", "        ++pw_profile[5];\n        ProcessWorkReadState* reader = m_active[m_readCursor];");
    }
    Patch(benchmark, "#include <thread>", "#include <thread>" + external);
    Patch(benchmark, "    std::vector<uint64_t> retries(threads, 0);", "    std::vector<uint64_t> retries(threads, 0);\n    std::vector<uint64_t> profile(static_cast<size_t>(threads) * 9, 0);");
    Patch(benchmark, "                starts[thread] = BenchmarkNow();", "                for (int i = 0; i < 9; ++i)\n                {\n                    pw_profile[i] = 0;\n                }\n                starts[thread] = BenchmarkNow();");
    Patch(benchmark, "                ends[thread] = BenchmarkNow();", "                ends[thread] = BenchmarkNow();\n                for (int i = 0; i < 9; ++i)\n                {\n                    profile[thread * 9 + i] = pw_profile[i];\n                }");
    Patch(benchmark, "        std::ostringstream report;", "        std::cout << \"PROFILE\";\n        for (int i = 0; i < 9; ++i)\n        {\n            uint64_t total = 0;\n            for (int t = 0; t < threads; ++t)\n            {\n                total += profile[t * 9 + i];\n            }\n            std::cout << ' ' << total;\n        }\n        std::cout << std::endl;\n        std::ostringstream report;");
    Patch(benchmark, "            std::istringstream input(bbRead(prefix + \".result\" + bbNumber(process)));", "            std::cout << bbRead(prefix + \".log\" + bbNumber(process));\n            std::istringstream input(bbRead(prefix + \".result\" + bbNumber(process)));");
}