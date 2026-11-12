#include "ProcessWorkBenchmarkAudit.h"
#include "BlackBoxTestSupport.h"
#include <atomic>
#include <sstream>

static std::atomic<bool> s_auditEnabled(false);
// 每个执行线程独占一组缓存行，避免审计自身成为多生产者共享CAS热点。
static std::atomic<uint64_t> s_auditEvents[256][16];
static std::atomic<uint32_t> s_auditNextRecord(0);
#ifdef _MSC_VER
static __declspec(thread) uint32_t s_auditRecord = UINT32_MAX;
#else
static thread_local uint32_t s_auditRecord = UINT32_MAX;
#endif

void ProcessWorkBenchmarkAudit::begin()
{
    s_auditEnabled.store(false);
    for (uint32_t record = 0; record < 256; ++record)
    {
        for (uint32_t i = 0; i < 10; ++i)
        {
            s_auditEvents[record][i].store(0);
        }
    }
    s_auditEnabled.store(true);
}

void ProcessWorkBenchmarkAudit::finish()
{
    s_auditEnabled.store(false);
}

void ProcessWorkBenchmarkAudit::hit(uint32_t event)
{
    if (event < 10 && s_auditEnabled.load(std::memory_order_relaxed))
    {
        if (s_auditRecord == UINT32_MAX)
        {
            const uint32_t record = s_auditNextRecord.fetch_add(1, std::memory_order_relaxed);
            s_auditRecord = record < 256 ? record : 255;
        }
        s_auditEvents[s_auditRecord][event].fetch_add(1, std::memory_order_relaxed);
    }
}

void ProcessWorkBenchmarkAudit::write(const std::string& path)
{
    const char* names[10] = { "reservation_miss", "capacity_wait", "receive_allocate", "post_allocate", "queue_growth", "map_growth", "shared_page", "shared_full", "post_capacity_wait", "receive_capacity" };
    std::ostringstream result;
    result << '{';
    for (uint32_t i = 0; i < 10; ++i)
    {
        if (i != 0)
        {
            result << ',';
        }
        uint64_t total = 0;
        for (uint32_t record = 0; record < 256; ++record)
        {
            total += s_auditEvents[record][i].load();
        }
        result << '"' << names[i] << "\":" << total;
    }
    result << '}';
    bbWrite(path, result.str());
}
