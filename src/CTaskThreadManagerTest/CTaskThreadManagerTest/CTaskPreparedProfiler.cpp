#include "CTaskPreparedProfiler.h"
#if defined(CTTM_BENCH_SAMPLER) && defined(_WIN32)
#include "CTaskPreparedBenchmark.h"
#include <DbgHelp.h>
#include <fstream>
#include <stdexcept>
#include <type_traits>

CTaskPreparedProfiler::CTaskPreparedProfiler(CTaskPreparedState& state, DWORD threadId) :
m_state(&state),
m_target(OpenThread(THREAD_GET_CONTEXT | THREAD_SUSPEND_RESUME, FALSE, threadId))
{
    if (m_target == nullptr)
    {
        throw std::runtime_error("profiler OpenThread failed");
    }
}

CTaskPreparedProfiler::~CTaskPreparedProfiler()
{
    CloseHandle(m_target);
}

void CTaskPreparedProfiler::sample()
{
    bool overlap = false;
    for (size_t i = 0; i < m_state->m_producers; ++i)
    {
        overlap = overlap || !m_state->m_progress[i].m_done.load(std::memory_order_acquire);
    }
    CONTEXT context = {};
    context.ContextFlags = CONTEXT_CONTROL;
    if (SuspendThread(m_target) == DWORD(-1))
    {
        throw std::runtime_error("profiler SuspendThread failed");
    }
    const BOOL obtained = GetThreadContext(m_target, &context);
    const DWORD resumed = ResumeThread(m_target);
    if (!obtained || resumed == DWORD(-1))
    {
        throw std::runtime_error("profiler context or resume failed");
    }
#ifdef _WIN64
    const uint64_t address = context.Rip;
#else
    const uint64_t address = context.Eip;
#endif
    ++(overlap ? m_overlap : m_drain)[address];
}

void CTaskPreparedProfiler::write(const std::string& path)
{
    std::ofstream output(path.c_str(), std::ios::binary | std::ios::trunc);
    if (!output)
    {
        throw std::runtime_error("profiler sampling or report failed");
    }
    HANDLE process = GetCurrentProcess();
    SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);
    if (!SymInitialize(process, nullptr, TRUE))
    {
        throw std::runtime_error("profiler symbol initialization failed");
    }
    output << "phase,address,samples,function,offset,line\n";
    for (size_t phase = 0; phase < 2; ++phase)
    {
        const std::map<uint64_t, size_t>& samples = phase == 0 ? m_overlap : m_drain;
        for (std::map<uint64_t, size_t>::const_iterator it = samples.begin(); it != samples.end(); ++it)
        {
            std::aligned_storage<sizeof(SYMBOL_INFO) + 1024, std::alignment_of<SYMBOL_INFO>::value>::type storage = {};
            SYMBOL_INFO* symbol = reinterpret_cast<SYMBOL_INFO*>(&storage);
            symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
            symbol->MaxNameLen = 1024;
            DWORD64 offset = 0;
            const BOOL found = SymFromAddr(process, it->first, &offset, symbol);
            IMAGEHLP_LINE64 line = {};
            line.SizeOfStruct = sizeof(line);
            DWORD displacement = 0;
            SymGetLineFromAddr64(process, it->first, &displacement, &line);
            output << (phase == 0 ? "overlap" : "drain") << ',' << it->first << ',' << it->second << ",\""
                   << (found ? symbol->Name : "unknown") << "\"," << offset << ',' << line.LineNumber << '\n';
        }
    }
    SymCleanup(process);
    output.close();
    if (!output)
    {
        throw std::runtime_error("profiler report write failed");
    }
}

#endif