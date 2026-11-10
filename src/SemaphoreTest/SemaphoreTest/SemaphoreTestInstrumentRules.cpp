#include "SemaphoreTestInstrument.h"

const SemaphoreTestInjection* SemaphoreTestInstrument::rules(size_t& count)
{
    // 测试注入只生成到测试构建目录；完整上下文必须唯一匹配当前正式源码。
    static const SemaphoreTestInjection kRules[] =
    {
        {
            "SemaphoreEventPool.cpp",
            "    const uintptr_t next = record->m_next.fetch_or(uintptr_t(1)) & ~uintptr_t(1);\n",
            82,
            "#ifdef SEMAPHORE_TEST_INSTRUMENTATION\n    SemaphoreTestPoolHook(9, this);\n#endif\n"
        },
        {
            "Semaphore.cpp",
            "SemaphoreResult Semaphore::createProcessResult(const std::string& name, int32_t signalCount)",
            0,
            "#ifdef SEMAPHORE_TEST_POSIX_NAME\n#undef _WIN32\n#endif\n"
        },
        {
            "Semaphore.cpp",
            "SemaphoreResult Semaphore::prepareEventPool(SemaphoreEventPool*& pool)",
            0,
            "#ifdef SEMAPHORE_TEST_POSIX_NAME\n#define _WIN32 1\n#endif\n"
        },
        {
            "SemaphoreEventPool.cpp",
            "        SemaphoreEventRecord* next = borrowed->m_retiredNext;",
            0,
            "#ifdef SEMAPHORE_TEST_INSTRUMENTATION\n        SemaphoreTestPoolHook(7, this);\n#endif\n"
        },
        {
            "SemaphoreEventPool.cpp",
            "        uint32_t references = head->m_retiredReferences.load();",
            0,
            "#ifdef SEMAPHORE_TEST_INSTRUMENTATION\n        SemaphoreTestPoolHook(8, this);\n#endif\n"
        },
        {
            "SemaphoreEventPool.cpp",
            "                const SemaphoreResult woke = wake->notify(&node->m_state);",
            0,
            "#ifdef SEMAPHORE_TEST_INSTRUMENTATION\n                SemaphoreTestPoolHook(6, this);\n#endif\n"
        },
        {
            "Semaphore.cpp",
            "    const bool direct = m_wake.canWaitAddress();\n    if (!direct)",
            0,
            "#ifdef SEMAPHORE_TEST_INSTRUMENTATION\n    SemaphoreTestHook(1, this);\n#endif\n"
        },
        {
            "Semaphore.cpp",
            "    DWORD waited = ::SignalObjectAndWait(\n        m_processSemaphore, waitOn.m_processSemaphore, timeout < 0 ? INFINITE : static_cast<DWORD>(timeout), FALSE);\n    DWORD error = waited == WAIT_FAILED ? ::GetLastError() : 0;\n    if (waited == WAIT_OBJECT_0)\n    {\n        return SemaphoreSignalWaitResult(SEMAPHORE_OK, SEMAPHORE_COMMIT_DONE, SEMAPHORE_COMMIT_DONE);",
            223,
            "#ifdef SEMAPHORE_TEST_INSTRUMENTATION\n    const int32_t ambiguous = SemaphoreTestFault(13);\n    if (ambiguous != 0)\n    {\n        // \346\250\241\346\213\237\347\263\273\347\273\237\345\234\250\351\230\266\346\256\265\347\273\223\346\236\234\344\270\215\346\230\216\346\227\266\345\244\261\350\264\245\357\274\233\345\267\262\344\272\247\347\224\237\347\232\204\347\234\237\345\256\236\345\211\257\344\275\234\347\224\250\344\270\215\350\203\275\350\242\253\346\265\213\350\257\225\345\233\236\346\273\232\343\200\202\n        waited = WAIT_FAILED;\n        error = static_cast<DWORD>(ambiguous);\n    }\n#endif\n"
        },
        {
            "Semaphore.cpp",
            "    {\n        return SemaphoreSignalWaitResult(SEMAPHORE_NOT_OPEN);\n    }\n#if defined(_WIN32) && !defined(SEMAPHORE_SHARED_COUNTER)\n    DWORD waited = ::SignalObjectAndWait(\n        m_processSemaphore, waitOn.m_processSemaphore, timeout < 0 ? INFINITE : static_cast<DWORD>(timeout), FALSE);",
            74,
            "#ifdef SEMAPHORE_TEST_INSTRUMENTATION\n    const int32_t beforeSignal = SemaphoreTestFault(11);\n    if (beforeSignal != 0)\n    {\n        return SemaphoreSignalWaitResult(\n            SEMAPHORE_SYSTEM_ERROR, SEMAPHORE_COMMIT_NONE, SEMAPHORE_COMMIT_NONE, beforeSignal);\n    }\n    const int32_t afterSignal = SemaphoreTestFault(12);\n    if (afterSignal != 0)\n    {\n        const SemaphoreResult sent = processSignalResult();\n        return SemaphoreSignalWaitResult(sent.status == SEMAPHORE_OK ? SEMAPHORE_SYSTEM_ERROR : sent.status,\n            sent.committed ? SEMAPHORE_COMMIT_DONE : SEMAPHORE_COMMIT_NONE, SEMAPHORE_COMMIT_NONE,\n            sent.status == SEMAPHORE_OK ? afterSignal : sent.nativeError);\n    }\n#endif\n"
        },
        {
            "SemaphoreWake.cpp",
            "SemaphoreResult SemaphoreWake::sleep(\n    uint32_t expected, uint32_t milliseconds, std::atomic<uint32_t>* address)\n{\n",
            118,
            "#ifdef SEMAPHORE_TEST_INSTRUMENTATION\n    const int32_t injected = SemaphoreTestFault(3);\n    if (injected != 0)\n    {\n        return SemaphoreResult(SEMAPHORE_SYSTEM_ERROR, false, injected);\n    }\n    SemaphoreTestWakeHook();\n#endif\n"
        },
        {
            "SemaphoreWake.cpp",
            "SemaphoreResult SemaphoreWake::signalAndSleep(SemaphoreWake& waitOn, uint32_t expected, uint32_t milliseconds)\n{\n#ifdef _WIN32\n",
            127,
            "#ifdef SEMAPHORE_TEST_INSTRUMENTATION\n    const int32_t injected = SemaphoreTestFault(2);\n    if (injected != 0)\n    {\n        return SemaphoreResult(SEMAPHORE_SYSTEM_ERROR, false, injected);\n    }\n    SemaphoreTestWakeHook();\n#endif\n"
        },
        {
            "SemaphoreWake.cpp",
            "    {\n        return SemaphoreResult(SEMAPHORE_UNSUPPORTED);\n    }\n#ifdef _WIN32\n    if (m_handle == nullptr)\n    {",
            67,
            "#ifdef SEMAPHORE_TEST_INSTRUMENTATION\n    const int32_t injected = SemaphoreTestFault(14);\n    if (injected != 0)\n    {\n        return SemaphoreResult(SEMAPHORE_SYSTEM_ERROR, false, injected);\n    }\n#endif\n"
        },
        {
            "SemaphoreWake.cpp",
            "SemaphoreResult SemaphoreWake::notify(std::atomic<uint32_t>* address)\n{\n",
            72,
            "#ifdef SEMAPHORE_TEST_INSTRUMENTATION\n    const int32_t injected = SemaphoreTestFault(2);\n    if (injected != 0)\n    {\n        return SemaphoreResult(SEMAPHORE_SYSTEM_ERROR, false, injected);\n    }\n#endif\n"
        },
        {
            "SemaphoreWake.cpp",
            "#elif defined(__APPLE__)\n    m_handle = dispatch_semaphore_create(0);",
            0,
            "#ifdef SEMAPHORE_TEST_INSTRUMENTATION\n    SemaphoreTestBackend(canWaitAddress());\n#endif\n"
        },
        {
            "SemaphoreWake.cpp",
            "void SemaphoreWake::loadAddressFunctions()\n{\n",
            45,
            "#ifdef SEMAPHORE_TEST_LEGACY_WAKE\n    return;\n#endif\n"
        },
        {
            "SemaphoreWake.cpp",
            "m_sleepers(0)\n{\n",
            16,
            "#ifdef SEMAPHORE_TEST_INSTRUMENTATION\n    if (SemaphoreTestFault(1) != 0)\n    {\n        throw std::system_error(std::make_error_code(std::errc::not_enough_memory));\n    }\n#endif\n"
        },
        {
            "SemaphoreEventPool.cpp",
            "    uintptr_t current = m_head.load();\n    bool delivered = false;\n    SemaphoreResult result;",
            39,
            "#ifdef SEMAPHORE_TEST_INSTRUMENTATION\n    SemaphoreTestPoolHook(1, this);\n#endif\n"
        },
        {
            "SemaphoreEventPool.cpp",
            "    do\n    {\n        record->m_next.store(head, std::memory_order_relaxed);\n    } while (!m_head.compare_exchange_weak(head, reinterpret_cast<uintptr_t>(record)));\n    leave();\n    return SemaphoreResult();",
            76,
            "#ifdef SEMAPHORE_TEST_INSTRUMENTATION\n        SemaphoreTestPoolHook(3, this);\n#endif\n"
        },
        {
            "SemaphoreEventPool.cpp",
            "        }\n    }\n    m_freeCount.fetch_add(reserved);\n    putFree(first, last);\n    // \346\211\271\346\254\241\346\216\245\345\205\245\345\217\257\347\224\250\351\223\276\344\271\213\345\220\216\346\211\215\350\275\254\344\270\272\345\267\262\345\217\221\345\270\203\345\256\271\351\207\217\357\274\214\346\232\202\345\201\234\345\217\221\345\270\203\350\200\205\344\270\215\350\203\275\346\217\220\345\211\215\350\200\227\345\260\275\345\217\257\347\224\250\351\242\235\345\272\246\n    // \350\277\220\350\241\214\346\234\237\350\260\203\347\224\250\350\200\205\346\214\201\346\234\211\350\257\273\350\200\205\344\277\235\346\212\244\357\274\214\346\226\260\345\242\236\345\256\271\351\207\217\345\205\245\350\264\246\345\211\215\344\270\215\344\274\232\347\211\251\347\220\206\347\274\251\345\256\271",
            53,
            "#ifdef SEMAPHORE_TEST_INSTRUMENTATION\n    SemaphoreTestPoolHook(5, this);\n#endif\n"
        },
        {
            "SemaphoreEventPool.cpp",
            "    for (size_t index = 0; index < reserved; ++index)\n    {\n        SemaphoreEventRecord* node = nullptr;\n        {\n            // \350\256\260\345\275\225\346\236\204\351\200\240\344\270\215\346\212\233\345\274\202\345\270\270\357\274\233\347\224\263\350\257\267\345\244\261\350\264\245\347\233\264\346\216\245\345\233\236\346\273\232\346\234\252\345\217\221\345\270\203\346\211\271\346\254\241\357\274\214\344\270\215\347\224\250\345\274\202\345\270\270\344\275\234\346\255\243\345\270\270\345\210\206\346\224\257\n            node = new (std::nothrow) SemaphoreEventRecord;",
            106,
            "#ifdef SEMAPHORE_TEST_INSTRUMENTATION\n        if (SemaphoreTestFault(4) == 0)\n#endif\n"
        },
        {
            "SemaphoreEventPool.cpp",
            "        }\n        reserved = (std::min)(count, limit - allocated);\n    } while (!m_allocated.compare_exchange_weak(allocated, allocated + reserved));\n    SemaphoreEventRecord* first = nullptr;\n    SemaphoreEventRecord* last = nullptr;\n    const uint64_t timestamp = m_config.shrink ? SemaphoreDeadline::now() : 0;",
            150,
            "#ifdef SEMAPHORE_TEST_INSTRUMENTATION\n    SemaphoreTestPoolHook(4, this);\n#endif\n"
        },
        {
            "Semaphore.cpp",
            "#include \"SemaphoreProcessMemory.h\"\n",
            0,
            "#if defined(_WIN32) && defined(PROCESS_WORK_PORTABLE_NOTIFY)\n#include \"SemaphoreTestProcessMemory.h\"\n#else\n"
        },
        {
            "Semaphore.cpp",
            "#include \"SemaphoreProcessMemory.h\"\n",
            36,
            "#endif\n"
        },
        {
            "Semaphore.cpp",
            "SemaphoreResult Semaphore::waitCount(bool infinite, uint32_t timeout)\n{\n",
            72,
            "#ifdef SEMAPHORE_TEST_INSTRUMENTATION\n    SemaphoreTestHook(1, this);\n#endif\n"
        },
        {
            "Semaphore.cpp",
            "SemaphoreResult Semaphore::waitCount(bool infinite, uint32_t timeout)\n{\n    if (consumeCount())\n    {\n        return SemaphoreResult(SEMAPHORE_OK, true);\n    }\n    if (!infinite && timeout == 0)\n    {\n",
            201,
            "#ifdef SEMAPHORE_TEST_INSTRUMENTATION\n    SemaphoreTestHook(5, this);\n#endif\n"
        },
        {
            "Semaphore.cpp",
            "bool Semaphore::incrementCount()\n{\n    uint64_t count = m_count.load();\n    for (;;)\n    {\n        const uint64_t next = count + 1;\n        // \350\256\260\345\275\225\345\234\260\345\235\200\351\235\236\347\251\272\344\270\224\350\207\263\345\260\2214\345\255\227\350\212\202\345\257\271\351\275\220\357\274\214\344\270\215\344\274\232\347\274\226\347\240\201\344\270\272UINT64_MAX\357\274\233\350\277\231\351\207\214\345\217\252\344\277\256\346\224\271\346\231\256\351\200\232\347\247\257\345\255\230\346\225\260\343\200\202\n        if (static_cast<int64_t>(next) < 0)\n        {\n            return false;\n        }\n        if (m_count.compare_exchange_weak(count, next))\n        {\n",
            404,
            "#ifdef SEMAPHORE_TEST_INSTRUMENTATION\n    SemaphoreTestHook(4, this);\n#endif\n"
        },
        {
            "Semaphore.cpp",
            "SemaphoreResult Semaphore::waitCountRecord(\n    SemaphoreEventRecord* record, const SemaphoreDeadline& deadline, bool entered)\n{\n    SemaphoreCount* pool = m_countPool.load();\n    SemaphoreResult result;\n    const bool direct = m_wake.canWaitAddress();\n    if (!direct && !entered)\n    {\n        result = m_wake.enterWait();\n        if (result.status != SEMAPHORE_OK)\n        {\n            result.committed = pool->finish(m_count, record);\n            return result;\n        }\n        entered = true;\n    }\n",
            507,
            "#ifdef SEMAPHORE_TEST_INSTRUMENTATION\n    SemaphoreTestHook(7, this);\n#endif\n"
        },
        {
            "Semaphore.cpp",
            "SemaphoreResult Semaphore::waitCountRecord(\n    SemaphoreEventRecord* record, const SemaphoreDeadline& deadline, bool entered)\n{\n    SemaphoreCount* pool = m_countPool.load();\n    SemaphoreResult result;\n    const bool direct = m_wake.canWaitAddress();\n    if (!direct && !entered)\n    {\n        result = m_wake.enterWait();\n        if (result.status != SEMAPHORE_OK)\n        {\n            result.committed = pool->finish(m_count, record);\n            return result;\n        }\n        entered = true;\n    }\n    for (;;)\n    {\n        const uint32_t expected = direct ? 0 : m_wake.snapshot();\n        if (record->m_state.load() == 1)\n        {\n            result = SemaphoreResult(SEMAPHORE_OK, true);\n            break;\n        }\n",
            730,
            "#ifdef SEMAPHORE_TEST_INSTRUMENTATION\n    SemaphoreTestHook(2, this);\n#endif\n"
        },
        {
            "Semaphore.cpp",
            "SemaphoreResult Semaphore::waitCountRecord(\n    SemaphoreEventRecord* record, const SemaphoreDeadline& deadline, bool entered)\n{\n    SemaphoreCount* pool = m_countPool.load();\n    SemaphoreResult result;\n    const bool direct = m_wake.canWaitAddress();\n    if (!direct && !entered)\n    {\n        result = m_wake.enterWait();\n        if (result.status != SEMAPHORE_OK)\n        {\n            result.committed = pool->finish(m_count, record);\n            return result;\n        }\n        entered = true;\n    }\n    for (;;)\n    {\n        const uint32_t expected = direct ? 0 : m_wake.snapshot();\n        if (record->m_state.load() == 1)\n        {\n            result = SemaphoreResult(SEMAPHORE_OK, true);\n            break;\n        }\n        const uint32_t slice = deadline.remaining(32);\n        if (slice == 0)\n        {\n            result = SemaphoreResult(SEMAPHORE_TIMEOUT);\n            break;\n        }\n",
            905,
            "#ifdef SEMAPHORE_TEST_INSTRUMENTATION\n    SemaphoreTestHook(3, this);\n#endif\n"
        },
        {
            "Semaphore.cpp",
            "SemaphoreResult Semaphore::waitCountRecord(\n    SemaphoreEventRecord* record, const SemaphoreDeadline& deadline, bool entered)\n{\n    SemaphoreCount* pool = m_countPool.load();\n    SemaphoreResult result;\n    const bool direct = m_wake.canWaitAddress();\n    if (!direct && !entered)\n    {\n        result = m_wake.enterWait();\n        if (result.status != SEMAPHORE_OK)\n        {\n            result.committed = pool->finish(m_count, record);\n            return result;\n        }\n        entered = true;\n    }\n    for (;;)\n    {\n        const uint32_t expected = direct ? 0 : m_wake.snapshot();\n        if (record->m_state.load() == 1)\n        {\n            result = SemaphoreResult(SEMAPHORE_OK, true);\n            break;\n        }\n        const uint32_t slice = deadline.remaining(32);\n        if (slice == 0)\n        {\n",
            819,
            "#ifdef SEMAPHORE_TEST_INSTRUMENTATION\n    SemaphoreTestHook(5, this);\n#endif\n"
        },
        {
            "Semaphore.cpp",
            "SemaphoreResult Semaphore::waitEvent(bool infinite, uint32_t timeout)\n{\n    const SemaphoreDeadline deadline(infinite, timeout);\n    if (!SemaphoreWake::canBlock())\n    {\n        return SemaphoreResult(SEMAPHORE_UNSUPPORTED);\n    }\n    SemaphoreEventPool* pool = nullptr;\n    SemaphoreResult result = prepareEventPool(pool);\n    if (result.status != SEMAPHORE_OK)\n    {\n        return result;\n    }\n    SemaphoreEventRecord* record = nullptr;\n    result = pool->begin(record);\n    if (result.status != SEMAPHORE_OK)\n    {\n        return result;\n    }\n    // \345\234\260\345\235\200\347\255\211\345\276\205\347\233\264\346\216\245\344\275\277\347\224\250\345\267\262\347\231\273\350\256\260\350\256\260\345\275\225\357\274\233\345\205\266\344\273\226\345\220\216\347\253\257\350\277\230\351\234\200\347\231\273\350\256\260\345\205\254\345\205\261\347\263\273\347\273\237\346\217\220\347\244\272\343\200\202\n    const bool direct = m_wake.canWaitAddress();\n    if (!direct)\n    {\n        result = m_wake.enterWait();\n        if (result.status != SEMAPHORE_OK)\n        {\n            result.committed = pool->finish(record);\n            return result;\n        }\n    }\n",
            904,
            "#ifdef SEMAPHORE_TEST_INSTRUMENTATION\n    SemaphoreTestHook(7, this);\n#endif\n"
        },
        {
            "Semaphore.cpp",
            "SemaphoreResult Semaphore::waitEvent(bool infinite, uint32_t timeout)\n{\n    const SemaphoreDeadline deadline(infinite, timeout);\n    if (!SemaphoreWake::canBlock())\n    {\n        return SemaphoreResult(SEMAPHORE_UNSUPPORTED);\n    }\n    SemaphoreEventPool* pool = nullptr;\n    SemaphoreResult result = prepareEventPool(pool);\n    if (result.status != SEMAPHORE_OK)\n    {\n        return result;\n    }\n    SemaphoreEventRecord* record = nullptr;\n    result = pool->begin(record);\n    if (result.status != SEMAPHORE_OK)\n    {\n        return result;\n    }\n    // \345\234\260\345\235\200\347\255\211\345\276\205\347\233\264\346\216\245\344\275\277\347\224\250\345\267\262\347\231\273\350\256\260\350\256\260\345\275\225\357\274\233\345\205\266\344\273\226\345\220\216\347\253\257\350\277\230\351\234\200\347\231\273\350\256\260\345\205\254\345\205\261\347\263\273\347\273\237\346\217\220\347\244\272\343\200\202\n    const bool direct = m_wake.canWaitAddress();\n    if (!direct)\n    {\n        result = m_wake.enterWait();\n        if (result.status != SEMAPHORE_OK)\n        {\n            result.committed = pool->finish(record);\n            return result;\n        }\n    }\n    for (;;)\n    {\n        const uint32_t expected = direct ? 0 : m_wake.snapshot();\n        if (record->m_state.load() == 1)\n        {\n            result = SemaphoreResult(SEMAPHORE_OK, true);\n            break;\n        }\n",
            1127,
            "#ifdef SEMAPHORE_TEST_INSTRUMENTATION\n    SemaphoreTestHook(2, this);\n#endif\n"
        },
        {
            "Semaphore.cpp",
            "SemaphoreResult Semaphore::waitEvent(bool infinite, uint32_t timeout)\n{\n    const SemaphoreDeadline deadline(infinite, timeout);\n    if (!SemaphoreWake::canBlock())\n    {\n        return SemaphoreResult(SEMAPHORE_UNSUPPORTED);\n    }\n    SemaphoreEventPool* pool = nullptr;\n    SemaphoreResult result = prepareEventPool(pool);\n    if (result.status != SEMAPHORE_OK)\n    {\n        return result;\n    }\n    SemaphoreEventRecord* record = nullptr;\n    result = pool->begin(record);\n    if (result.status != SEMAPHORE_OK)\n    {\n        return result;\n    }\n    // \345\234\260\345\235\200\347\255\211\345\276\205\347\233\264\346\216\245\344\275\277\347\224\250\345\267\262\347\231\273\350\256\260\350\256\260\345\275\225\357\274\233\345\205\266\344\273\226\345\220\216\347\253\257\350\277\230\351\234\200\347\231\273\350\256\260\345\205\254\345\205\261\347\263\273\347\273\237\346\217\220\347\244\272\343\200\202\n    const bool direct = m_wake.canWaitAddress();\n    if (!direct)\n    {\n        result = m_wake.enterWait();\n        if (result.status != SEMAPHORE_OK)\n        {\n            result.committed = pool->finish(record);\n            return result;\n        }\n    }\n    for (;;)\n    {\n        const uint32_t expected = direct ? 0 : m_wake.snapshot();\n        if (record->m_state.load() == 1)\n        {\n            result = SemaphoreResult(SEMAPHORE_OK, true);\n            break;\n        }\n        const uint32_t slice = deadline.remaining(32);\n        if (slice == 0)\n        {\n            result = SemaphoreResult(SEMAPHORE_TIMEOUT);\n            break;\n        }\n",
            1302,
            "#ifdef SEMAPHORE_TEST_INSTRUMENTATION\n    SemaphoreTestHook(3, this);\n#endif\n"
        },
        {
            "Semaphore.cpp",
            "SemaphoreResult Semaphore::waitEvent(bool infinite, uint32_t timeout)\n{\n    const SemaphoreDeadline deadline(infinite, timeout);\n    if (!SemaphoreWake::canBlock())\n    {\n        return SemaphoreResult(SEMAPHORE_UNSUPPORTED);\n    }\n    SemaphoreEventPool* pool = nullptr;\n    SemaphoreResult result = prepareEventPool(pool);\n    if (result.status != SEMAPHORE_OK)\n    {\n        return result;\n    }\n    SemaphoreEventRecord* record = nullptr;\n    result = pool->begin(record);\n    if (result.status != SEMAPHORE_OK)\n    {\n        return result;\n    }\n    // \345\234\260\345\235\200\347\255\211\345\276\205\347\233\264\346\216\245\344\275\277\347\224\250\345\267\262\347\231\273\350\256\260\350\256\260\345\275\225\357\274\233\345\205\266\344\273\226\345\220\216\347\253\257\350\277\230\351\234\200\347\231\273\350\256\260\345\205\254\345\205\261\347\263\273\347\273\237\346\217\220\347\244\272\343\200\202\n    const bool direct = m_wake.canWaitAddress();\n    if (!direct)\n    {\n        result = m_wake.enterWait();\n        if (result.status != SEMAPHORE_OK)\n        {\n            result.committed = pool->finish(record);\n            return result;\n        }\n    }\n    for (;;)\n    {\n        const uint32_t expected = direct ? 0 : m_wake.snapshot();\n        if (record->m_state.load() == 1)\n        {\n            result = SemaphoreResult(SEMAPHORE_OK, true);\n            break;\n        }\n        const uint32_t slice = deadline.remaining(32);\n        if (slice == 0)\n        {\n",
            1216,
            "#ifdef SEMAPHORE_TEST_INSTRUMENTATION\n    SemaphoreTestHook(5, this);\n#endif\n"
        },
        {
            "Semaphore.cpp",
            "SemaphoreResult Semaphore::waitCountCompetitive(const SemaphoreDeadline& deadline, bool& priority)\n{\n    priority = false;\n    SemaphoreResult result = m_wake.enterWait();\n    if (result.status != SEMAPHORE_OK)\n    {\n        return result;\n    }\n    // \346\231\256\351\200\232\347\255\211\345\276\205\345\205\210\345\244\215\347\224\250\345\205\261\344\272\253\347\263\273\347\273\237\346\217\220\347\244\272\357\274\233\345\215\225\346\254\241\347\255\211\345\276\205\346\214\201\347\273\25532ms\346\234\252\345\217\226\345\276\227\350\256\270\345\217\257\345\220\216\345\206\215\350\277\233\345\205\245\344\274\230\345\205\210\344\272\244\344\273\230\351\223\276\343\200\202\n    // \350\277\231\346\230\257\351\230\262\346\255\242\351\225\277\346\234\237\346\212\242\344\270\215\345\210\260\350\256\270\345\217\257\347\232\204\345\215\207\347\272\247\346\227\266\351\227\264\357\274\214\344\270\215\346\230\257\346\257\217\344\270\252\347\272\277\347\250\213\345\277\205\351\241\273\347\255\211\345\276\205\347\232\204\346\227\266\351\227\264\346\210\226\350\260\203\345\272\246\344\270\212\351\231\220\343\200\202\n    const SemaphoreDeadline promotion(false, 32);\n",
            529,
            "#ifdef SEMAPHORE_TEST_INSTRUMENTATION\n    SemaphoreTestHook(7, this);\n#endif\n"
        },
        {
            "Semaphore.cpp",
            "SemaphoreResult Semaphore::waitCountCompetitive(const SemaphoreDeadline& deadline, bool& priority)\n{\n    priority = false;\n    SemaphoreResult result = m_wake.enterWait();\n    if (result.status != SEMAPHORE_OK)\n    {\n        return result;\n    }\n    // \346\231\256\351\200\232\347\255\211\345\276\205\345\205\210\345\244\215\347\224\250\345\205\261\344\272\253\347\263\273\347\273\237\346\217\220\347\244\272\357\274\233\345\215\225\346\254\241\347\255\211\345\276\205\346\214\201\347\273\25532ms\346\234\252\345\217\226\345\276\227\350\256\270\345\217\257\345\220\216\345\206\215\350\277\233\345\205\245\344\274\230\345\205\210\344\272\244\344\273\230\351\223\276\343\200\202\n    // \350\277\231\346\230\257\351\230\262\346\255\242\351\225\277\346\234\237\346\212\242\344\270\215\345\210\260\350\256\270\345\217\257\347\232\204\345\215\207\347\272\247\346\227\266\351\227\264\357\274\214\344\270\215\346\230\257\346\257\217\344\270\252\347\272\277\347\250\213\345\277\205\351\241\273\347\255\211\345\276\205\347\232\204\346\227\266\351\227\264\346\210\226\350\260\203\345\272\246\344\270\212\351\231\220\343\200\202\n    const SemaphoreDeadline promotion(false, 32);\n    for (;;)\n    {\n        const uint32_t expected = m_wake.snapshot();\n        if (consumeCount())\n        {\n            result = SemaphoreResult(SEMAPHORE_OK, true);\n            break;\n        }\n",
            726,
            "#ifdef SEMAPHORE_TEST_INSTRUMENTATION\n    SemaphoreTestHook(2, this);\n#endif\n"
        },
        {
            "Semaphore.cpp",
            "SemaphoreResult Semaphore::waitCountCompetitive(const SemaphoreDeadline& deadline, bool& priority)\n{\n    priority = false;\n    SemaphoreResult result = m_wake.enterWait();\n    if (result.status != SEMAPHORE_OK)\n    {\n        return result;\n    }\n    // \346\231\256\351\200\232\347\255\211\345\276\205\345\205\210\345\244\215\347\224\250\345\205\261\344\272\253\347\263\273\347\273\237\346\217\220\347\244\272\357\274\233\345\215\225\346\254\241\347\255\211\345\276\205\346\214\201\347\273\25532ms\346\234\252\345\217\226\345\276\227\350\256\270\345\217\257\345\220\216\345\206\215\350\277\233\345\205\245\344\274\230\345\205\210\344\272\244\344\273\230\351\223\276\343\200\202\n    // \350\277\231\346\230\257\351\230\262\346\255\242\351\225\277\346\234\237\346\212\242\344\270\215\345\210\260\350\256\270\345\217\257\347\232\204\345\215\207\347\272\247\346\227\266\351\227\264\357\274\214\344\270\215\346\230\257\346\257\217\344\270\252\347\272\277\347\250\213\345\277\205\351\241\273\347\255\211\345\276\205\347\232\204\346\227\266\351\227\264\346\210\226\350\260\203\345\272\246\344\270\212\351\231\220\343\200\202\n    const SemaphoreDeadline promotion(false, 32);\n    for (;;)\n    {\n        const uint32_t expected = m_wake.snapshot();\n        if (consumeCount())\n        {\n            result = SemaphoreResult(SEMAPHORE_OK, true);\n            break;\n        }\n        const uint64_t state = m_count.load();\n        const uint32_t allowance = promotion.remaining(32);\n        if (allowance == 0 || (state > (UINT64_MAX >> 1) && (state & 1) != 0))\n        {\n            // \345\244\247\350\256\241\346\225\260\344\271\237\344\272\244\347\273\231\345\217\227\350\257\273\350\200\205\344\277\235\346\212\244\347\232\204\350\267\257\345\276\204\345\244\204\347\220\206\357\274\214\344\270\215\350\203\275\345\234\250\350\277\231\351\207\214\350\247\243\345\274\225\347\224\250\350\256\260\345\275\225\345\234\260\345\235\200\343\200\202\n            priority = true;\n            result = SemaphoreResult();\n            break;\n        }\n        const uint32_t slice = deadline.remaining(allowance);\n        if (slice == 0)\n        {\n            result = SemaphoreResult(SEMAPHORE_TIMEOUT);\n            break;\n        }\n",
            1308,
            "#ifdef SEMAPHORE_TEST_INSTRUMENTATION\n    SemaphoreTestHook(3, this);\n#endif\n"
        },
        {
            "Semaphore.cpp",
            "SemaphoreResult Semaphore::waitCountCompetitive(const SemaphoreDeadline& deadline, bool& priority)\n{\n    priority = false;\n    SemaphoreResult result = m_wake.enterWait();\n    if (result.status != SEMAPHORE_OK)\n    {\n        return result;\n    }\n    // \346\231\256\351\200\232\347\255\211\345\276\205\345\205\210\345\244\215\347\224\250\345\205\261\344\272\253\347\263\273\347\273\237\346\217\220\347\244\272\357\274\233\345\215\225\346\254\241\347\255\211\345\276\205\346\214\201\347\273\25532ms\346\234\252\345\217\226\345\276\227\350\256\270\345\217\257\345\220\216\345\206\215\350\277\233\345\205\245\344\274\230\345\205\210\344\272\244\344\273\230\351\223\276\343\200\202\n    // \350\277\231\346\230\257\351\230\262\346\255\242\351\225\277\346\234\237\346\212\242\344\270\215\345\210\260\350\256\270\345\217\257\347\232\204\345\215\207\347\272\247\346\227\266\351\227\264\357\274\214\344\270\215\346\230\257\346\257\217\344\270\252\347\272\277\347\250\213\345\277\205\351\241\273\347\255\211\345\276\205\347\232\204\346\227\266\351\227\264\346\210\226\350\260\203\345\272\246\344\270\212\351\231\220\343\200\202\n    const SemaphoreDeadline promotion(false, 32);\n    for (;;)\n    {\n        const uint32_t expected = m_wake.snapshot();\n        if (consumeCount())\n        {\n            result = SemaphoreResult(SEMAPHORE_OK, true);\n            break;\n        }\n        const uint64_t state = m_count.load();\n        const uint32_t allowance = promotion.remaining(32);\n        if (allowance == 0 || (state > (UINT64_MAX >> 1) && (state & 1) != 0))\n        {\n            // \345\244\247\350\256\241\346\225\260\344\271\237\344\272\244\347\273\231\345\217\227\350\257\273\350\200\205\344\277\235\346\212\244\347\232\204\350\267\257\345\276\204\345\244\204\347\220\206\357\274\214\344\270\215\350\203\275\345\234\250\350\277\231\351\207\214\350\247\243\345\274\225\347\224\250\350\256\260\345\275\225\345\234\260\345\235\200\343\200\202\n            priority = true;\n            result = SemaphoreResult();\n            break;\n        }\n        const uint32_t slice = deadline.remaining(allowance);\n        if (slice == 0)\n        {\n",
            1222,
            "#ifdef SEMAPHORE_TEST_INSTRUMENTATION\n    SemaphoreTestHook(5, this);\n#endif\n"
        },
        {
            "SemaphoreEventPool.cpp",
            "SemaphoreResult SemaphoreEventPool::notify(bool all, SemaphoreWake* wake)\n{\n    if (!enter())\n    {\n        return SemaphoreResult(SEMAPHORE_CAPACITY);\n    }\n    // \350\277\231\344\270\200head\345\277\253\347\205\247\345\233\272\345\256\232\345\271\277\346\222\255\351\233\206\345\220\210\357\274\233\345\271\266\345\217\221\346\226\260\347\231\273\350\256\260\350\200\205\345\217\252\350\203\275\351\223\276\346\216\245\345\234\250\345\256\203\344\271\213\345\211\215\343\200\202\n    uintptr_t current = m_head.load();\n    bool delivered = false;\n    SemaphoreResult result;\n    while (current != 0)\n    {\n        SemaphoreEventRecord* node = all ? reinterpret_cast<SemaphoreEventRecord*>(current) : oldest(current);\n        if (node == nullptr)\n        {\n            break;\n        }\n",
            553,
            "#ifdef SEMAPHORE_TEST_INSTRUMENTATION\n    SemaphoreTestPoolHook(2, this);\n#endif\n"
        },
        {
            "SemaphoreCount.cpp",
            "SemaphoreResult SemaphoreCount::signalProtected(std::atomic<uint64_t>& state, SemaphoreWake& wake, bool notify)\n{\n    for (;;)\n    {\n        uint64_t current = state.load();\n        if (current < directLimit())\n        {\n            if (state.compare_exchange_weak(current, current + 1))\n            {\n                // \347\231\273\350\256\260\351\223\276\345\217\257\350\203\275\345\210\232\350\242\253\345\205\266\344\273\226\347\272\277\347\250\213\346\270\205\347\251\272\357\274\214\346\231\256\351\200\232\347\253\236\344\272\211\347\255\211\345\276\205\350\200\205\344\273\215\351\234\200\346\224\266\345\210\260\345\205\254\345\205\261\346\217\220\347\244\272\343\200\202\n                SemaphoreResult result = notify ? wake.notify() : SemaphoreResult();\n                result.committed = true;\n                return result;\n            }\n            continue;\n        }\n        if (current == directLimit() || (current & 1) != 0)\n        {\n            const uint64_t count = current == directLimit() ? current : node(current)->m_idleSince;\n            if (count == UINT64_MAX)\n            {\n                return SemaphoreResult(SEMAPHORE_OVERFLOW);\n            }\n            const SemaphoreResult result = replaceLarge(state, current, count + 1);\n            if (result.status != SEMAPHORE_RETRY)\n            {\n                if (result.committed && notify)\n                {\n                    SemaphoreResult woke = wake.notify();\n                    woke.committed = true;\n                    return woke;\n                }\n                return result;\n            }\n            continue;\n        }\n        SemaphoreEventRecord* selected = SemaphoreEventPool::oldest(reinterpret_cast<uintptr_t>(node(current)));\n        if (selected == nullptr)\n        {\n            sweep(state);\n            continue;\n        }\n        uint32_t waiting = 0;\n        if (!selected->m_state.compare_exchange_strong(waiting, 1))\n        {\n            continue;\n        }\n        // \346\255\244CAS\345\267\262\347\273\217\344\272\244\344\273\230\346\234\254\346\254\241\350\256\270\345\217\257\343\200\202\345\215\263\344\275\277\346\216\245\346\224\266\350\200\205\346\232\202\345\201\234\346\210\226\347\263\273\347\273\237\346\217\220\347\244\272\345\244\261\350\264\245\357\274\214\345\220\216\346\235\245\350\200\205\344\271\237\344\270\215\350\203\275\345\244\272\350\265\260\343\200\202\n",
            1824,
            "#ifdef SEMAPHORE_TEST_INSTRUMENTATION\n    SemaphoreTestHook(4, nullptr);\n#endif\n"
        },
        {
            "SemaphoreCount.cpp",
            "SemaphoreResult SemaphoreCount::beginProtected(\n    std::atomic<uint64_t>& state, SemaphoreEventRecord*& record, bool probe)\n{\n    SemaphoreEventRecord* candidate = nullptr;\n    for (;;)\n    {\n        uint64_t current = state.load();\n        if (current != 0 && current <= directLimit())\n        {\n            if (!state.compare_exchange_weak(current, current - 1))\n            {\n                continue;\n            }\n            if (candidate != nullptr)\n            {\n                discard(candidate);\n            }\n            return SemaphoreResult(SEMAPHORE_OK, true);\n        }\n        if (current > directLimit() && (current & 1) != 0)\n        {\n            const SemaphoreResult result = replaceLarge(state, current, node(current)->m_idleSince - 1);\n            if (result.status == SEMAPHORE_RETRY)\n            {\n                continue;\n            }\n            if (candidate != nullptr)\n            {\n                discard(candidate);\n            }\n            return result;\n        }\n        if (probe)\n        {\n            return SemaphoreResult(SEMAPHORE_TIMEOUT);\n        }\n        if (candidate == nullptr)\n        {\n            const SemaphoreResult result = acquire(candidate);\n            if (result.status != SEMAPHORE_OK)\n            {\n                return result;\n            }\n        }\n        candidate->m_next.store(current == 0 ? 0 : reinterpret_cast<uintptr_t>(node(current)), std::memory_order_relaxed);\n        if (state.compare_exchange_weak(current, encode(candidate)))\n        {\n            record = candidate;\n",
            1556,
            "#ifdef SEMAPHORE_TEST_INSTRUMENTATION\n    SemaphoreTestHook(8, nullptr);\n#endif\n"
        },
        {
            "Semaphore.cpp",
            "void Semaphore::reset()\n{\n    // \345\244\226\351\203\250\351\235\231\346\255\242\357\274\233\347\263\273\347\273\237\344\270\255\345\215\263\344\275\277\346\234\211\346\227\247\346\217\220\347\244\272\357\274\214\344\271\237\344\270\215\345\206\215\344\273\243\350\241\250\350\256\270\345\217\257\346\210\226\344\272\213\344\273\266\343\200\202\n    SemaphoreCount* pool = m_countPool.load();\n    if (pool != nullptr)\n    {\n        pool->clear(m_count);\n    }\n    m_count.store(0);\n",
            248,
            "#ifdef SEMAPHORE_TEST_INSTRUMENTATION\n    uint64_t seed = 0;\n    if (SemaphoreTestCountSeed(this, seed))\n    {\n        if (seed <= (UINT64_MAX >> 1))\n        {\n            m_count.store(seed);\n        }\n        else\n        {\n            SemaphoreCount* prepared = nullptr;\n            checkLocalResult(prepareCountPool(prepared));\n            m_count.store(UINT64_MAX >> 1);\n            checkLocalResult(prepared->signal(m_count, m_wake));\n            SemaphoreEventRecord* large = reinterpret_cast<SemaphoreEventRecord*>(\n                static_cast<uintptr_t>(m_count.load() & (UINT64_MAX >> 1) & ~uint64_t(1)));\n            large->m_idleSince = seed;\n        }\n    }\n#endif\n"
        }
    };
    count = sizeof(kRules) / sizeof(kRules[0]);
    return kRules;
}