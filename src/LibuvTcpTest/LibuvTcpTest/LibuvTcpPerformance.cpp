#include "LibuvTcpPerformance.h"
#include <algorithm>
#include <cerrno>
#include <cfloat>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <signal.h>

LibuvTcpPerformance::LibuvTcpPerformance() :
m_loop(),
m_process(),
m_pipe(),
m_timer(),
m_exited(false),
m_eof(false),
m_failed(false)
{
}

void LibuvTcpPerformance::allocate(uv_handle_t* handle, size_t suggested, uv_buf_t* buffer)
{
    (void)suggested;
    LibuvTcpPerformance* state = static_cast<LibuvTcpPerformance*>(handle->data);
    *buffer = uv_buf_init(state->m_buffer, sizeof(state->m_buffer));
}

void LibuvTcpPerformance::finish()
{
    if (m_exited && m_eof && !uv_is_closing(reinterpret_cast<uv_handle_t*>(&m_timer)))
    {
        uv_timer_stop(&m_timer);
        uv_close(reinterpret_cast<uv_handle_t*>(&m_timer), nullptr);
    }
}

void LibuvTcpPerformance::read(uv_stream_t* stream, ssize_t count, const uv_buf_t* buffer)
{
    LibuvTcpPerformance* state = static_cast<LibuvTcpPerformance*>(stream->data);
    if (count > 0)
    {
        try
        {
            if (state->m_output.size() + static_cast<size_t>(count) > 65536)
            {
                timeout(&state->m_timer);
                return;
            }
            state->m_output.append(buffer->base, static_cast<size_t>(count));
        }
        catch (...)
        {
            timeout(&state->m_timer);
        }
    }
    else if (count < 0)
    {
        state->m_failed = state->m_failed || count != UV_EOF;
        state->m_eof = true;
        uv_close(reinterpret_cast<uv_handle_t*>(stream), nullptr);
        state->finish();
    }
}

void LibuvTcpPerformance::exited(uv_process_t* process, int64_t code, int signal)
{
    LibuvTcpPerformance* state = static_cast<LibuvTcpPerformance*>(process->data);
    state->m_failed = state->m_failed || code != 0 || signal != 0;
    state->m_exited = true;
    uv_close(reinterpret_cast<uv_handle_t*>(process), nullptr);
    state->finish();
}

void LibuvTcpPerformance::timeout(uv_timer_t* timer)
{
    LibuvTcpPerformance* state = static_cast<LibuvTcpPerformance*>(timer->data);
    state->m_failed = true;
    if (!state->m_exited)
    {
        uv_process_kill(&state->m_process, SIGKILL);
    }
    if (!uv_is_closing(reinterpret_cast<uv_handle_t*>(&state->m_pipe)))
    {
        uv_close(reinterpret_cast<uv_handle_t*>(&state->m_pipe), nullptr);
    }
    state->m_eof = true;
    state->finish();
}

bool LibuvTcpPerformance::execute(const char* executable, const std::vector<std::string>& arguments,
    std::string& output, uint64_t milliseconds)
{
    // 在创建任何uv句柄之前完成可能分配的参数准备。
    std::vector<char*> args;
    args.push_back(const_cast<char*>(executable));
    for (size_t index = 0; index < arguments.size(); ++index)
    {
        args.push_back(const_cast<char*>(arguments[index].c_str()));
    }
    args.push_back(nullptr);
    if (uv_loop_init(&m_loop) != 0)
    {
        return false;
    }
    if (uv_pipe_init(&m_loop, &m_pipe, 0) != 0)
    {
        uv_loop_close(&m_loop);
        return false;
    }
    if (uv_timer_init(&m_loop, &m_timer) != 0)
    {
        uv_close(reinterpret_cast<uv_handle_t*>(&m_pipe), nullptr);
        uv_run(&m_loop, UV_RUN_DEFAULT);
        uv_loop_close(&m_loop);
        return false;
    }
    m_pipe.data = m_timer.data = m_process.data = this;
    uv_stdio_container_t streams[3] = {};
    streams[0].flags = UV_IGNORE;
    streams[1].flags = static_cast<uv_stdio_flags>(UV_CREATE_PIPE | UV_WRITABLE_PIPE);
    streams[1].data.stream = reinterpret_cast<uv_stream_t*>(&m_pipe);
    streams[2].flags = UV_INHERIT_FD;
    streams[2].data.fd = 2;
    uv_process_options_t options = {};
    options.file = executable;
    options.args = args.data();
    options.stdio_count = 3;
    options.stdio = streams;
    options.exit_cb = &exited;
#ifdef _WIN32
    options.flags = UV_PROCESS_WINDOWS_HIDE;
#endif
    const int result = uv_spawn(&m_loop, &m_process, &options);
    if (result != 0)
    {
        uv_close(reinterpret_cast<uv_handle_t*>(&m_pipe), nullptr);
        uv_close(reinterpret_cast<uv_handle_t*>(&m_timer), nullptr);
        // 仅关闭uv_spawn实际初始化过的句柄，没有有效子进程需要终止。
        if (m_process.type == UV_PROCESS)
        {
            uv_close(reinterpret_cast<uv_handle_t*>(&m_process), nullptr);
        }
        uv_run(&m_loop, UV_RUN_DEFAULT);
        uv_loop_close(&m_loop);
        return false;
    }
    if (uv_read_start(reinterpret_cast<uv_stream_t*>(&m_pipe), &allocate, &read) != 0 ||
        uv_timer_start(&m_timer, &timeout, milliseconds, 1000) != 0)
    {
        timeout(&m_timer);
    }
    uv_run(&m_loop, UV_RUN_DEFAULT);
    const bool closed = uv_loop_close(&m_loop) == 0;
    output.swap(m_output);
    return closed && m_exited && m_eof && !m_failed;
}

bool LibuvTcpPerformance::parse(const std::string& output, int kind, uint64_t expected, double* metrics)
{
    const size_t end = output.find_last_not_of("\r\n");
    if (end == std::string::npos)
    {
        return false;
    }
    const size_t start = output.rfind('\n', end);
    const std::string line = output.substr(start == std::string::npos ? 0 : start + 1,
        end + 1 - (start == std::string::npos ? 0 : start + 1));
    std::vector<std::string> fields;
    size_t begin = 0;
    for (;;)
    {
        const size_t comma = line.find(',', begin);
        fields.push_back(line.substr(begin, comma == std::string::npos ? comma : comma - begin));
        if (comma == std::string::npos)
        {
            break;
        }
        begin = comma + 1;
    }
    if ((kind == 0 && (fields.size() != 11 || (fields[0] != "TINY" && fields[0] != "TINYCOPY" &&
        fields[0] != "TINYPOST" && fields[0] != "TINYCOPYPOST"))) ||
        (kind == 1 && (fields.size() != 13 || fields[0] != "BENCH")) ||
        (kind == 2 && (fields.size() != 5 || fields[0] != "RAW")) || kind < 0 || kind > 2)
    {
        return false;
    }
    double values[13] = {};
    for (size_t index = 1; index < fields.size(); ++index)
    {
        char* tail = nullptr;
        errno = 0;
        values[index] = std::strtod(fields[index].c_str(), &tail);
        if (errno != 0 || tail == fields[index].c_str() || *tail != 0 ||
            !(values[index] >= 0 && values[index] <= DBL_MAX))
        {
            return false;
        }
    }
    if (kind == 0 && (values[9] != static_cast<double>(expected) || values[10] != 1))
    {
        return false;
    }
    std::fill(metrics, metrics + 8, -1.0);
    metrics[0] = values[kind == 2 ? 4 : 6];
    metrics[1] = values[kind == 2 ? 3 : 5];
    if (kind != 2)
    {
        metrics[2] = values[kind == 0 ? 7 : 11];
        metrics[3] = values[kind == 0 ? 8 : 12];
    }
    if (kind == 1)
    {
        metrics[4] = values[7];
        metrics[5] = values[8];
        metrics[6] = values[9];
        metrics[7] = values[10];
    }
    return metrics[0] > 0 && metrics[1] > 0;
}

bool LibuvTcpPerformance::checkParser()
{
    double metrics[8];
    return parse("TINY,1,0,8,1,2,4000,1,100,8,1\n", 0, 8, metrics) && metrics[0] == 4000 &&
        !parse("TINY,1,0,8,1,2,4000,1,100,7,1\n", 0, 8, metrics) &&
        !parse("TINY,1,0,8,1,2,4000,1,100,8,0\n", 0, 8, metrics) &&
        !parse("RAW,6,10,1,nan\n", 2, 0, metrics) &&
        !parse("RAW,6,10,1,inf\n", 2, 0, metrics) &&
        !parse("WRONG,1,0,8,1,2,4000,1,100,8,1\n", 0, 8, metrics) &&
        !parse("RAW,6,10,1,10junk\n", 2, 0, metrics) &&
        !parse("RAW,6,10,1\n", 2, 0, metrics) &&
        !parse("", 2, 0, metrics) &&
        parse("RAW,6,10,1,10000\n", 2, 0, metrics) && metrics[2] == -1 &&
        parse("BENCH,1,16,100,128,2,50000,1,2,3,4,5,6\n", 1, 0, metrics) && metrics[7] == 4;
}

int LibuvTcpPerformance::runnerProbe(const char* mode)
{
    if (std::strcmp(mode, "0") == 0)
    {
        std::puts("runner-ok");
        return 0;
    }
    if (std::strcmp(mode, "1") == 0)
    {
        return 7;
    }
    if (std::strcmp(mode, "2") == 0)
    {
        char block[4096];
        std::memset(block, 'x', sizeof(block));
        for (uint32_t index = 0; index < 20; ++index)
        {
            std::fwrite(block, 1, sizeof(block), stdout);
        }
        std::fflush(stdout);
        return 0;
    }
    if (std::strcmp(mode, "3") == 0)
    {
        uv_sleep(5000);
        return 0;
    }
    return 2;
}

bool LibuvTcpPerformance::checkRunner()
{
    char self[32768];
    size_t size = sizeof(self);
    if (uv_exepath(self, &size) != 0 || size >= sizeof(self))
    {
        return false;
    }
    self[size] = 0;
    const char* modes[] = { "0", "1", "2", "3" };
    for (uint32_t index = 0; index < 4; ++index)
    {
        LibuvTcpPerformance child;
        const std::vector<std::string> args = { "--runner-probe", modes[index] };
        std::string output;
        const bool success = child.execute(self, args, output, index == 3 ? 200 : 5000);
        if (success != (index == 0) || (success && output.find("runner-ok") == std::string::npos))
        {
            return false;
        }
    }
    LibuvTcpPerformance missing;
    std::string output;
    const std::vector<std::string> empty;
    return !missing.execute((std::string(self) + ".missing").c_str(), empty, output);
}

int LibuvTcpPerformance::run(int argc, char** argv)
{
    if (argc != 4 && argc != 5)
    {
        std::fputs("usage: --suite rounds baseline-exe-or-dash [case:0-28]\n", stderr);
        return 2;
    }
    uint32_t parsed[2] = {};
    for (size_t index = 0; index < (argc == 5 ? 2U : 1U); ++index)
    {
        const char* input = argv[index == 0 ? 2 : 4];
        char* tail = nullptr;
        errno = 0;
        const unsigned long value = std::strtoul(input, &tail, 10);
        if (*input < '0' || *input > '9' || tail == input || *tail != 0 || errno != 0 ||
            value > (index == 0 ? 100U : 28U) || (index == 0 && value == 0))
        {
            return 2;
        }
        parsed[index] = static_cast<uint32_t>(value);
    }
    char self[32768];
    size_t selfSize = sizeof(self);
    if (uv_exepath(self, &selfSize) != 0 || selfSize >= sizeof(self) || !checkParser())
    {
        return 2;
    }
    self[selfSize] = 0;
    const char* executables[2] = { argv[3], self };
    const bool compare = std::strcmp(argv[3], "-") != 0;
    const char* names[] = { "rate", "ms", "cpu_ms", "peak_bytes", "mib_s", "p50_us", "p95_us", "p99_us" };
    const char* modes[] = { "--tinycopybenchmark", "--tinycopypostbenchmark", "--tinybenchmark", "--tinypostbenchmark" };
    const uint32_t latency[10][4] = { {1,16,50000,1}, {1,16,100000,128}, {4,16,100000,128},
        {16,16,50000,128}, {1,64,100000,128}, {16,64,50000,128}, {1,1024,50000,128},
        {16,1024,10000,128}, {1,65536,4000,32}, {16,65536,1000,32} };
    const uint32_t raw[4][4] = { {1,6,100000,32}, {1,64,100000,32}, {1,1024,50000,32}, {1,32768,4000,32} };
    for (uint32_t test = 1; test <= 28; ++test)
    {
        if (parsed[1] != 0 && parsed[1] != test)
        {
            continue;
        }
        const uint32_t index = test - 1;
        const bool fixed = index < 12 && index % 3 == 0;
        const int kind = index < 12 || index >= 26 ? 0 : index < 22 ? 1 : 2;
        const char* mode = index < 12 ? modes[index / 3] : index < 22 ? "--benchmark" :
            index < 26 ? "--rawbenchmark" : "--tinybenchmark";
        uint32_t values[4] = { index % 3 == 2 ? 16U : 1U, 0, index % 3 == 2 ? 500000U : 8000000U, 1 };
        if (index >= 12 && index < 26)
        {
            std::copy(index < 22 ? latency[index - 12] : raw[index - 22],
                (index < 22 ? latency[index - 12] : raw[index - 22]) + 4, values);
        }
        else if (index >= 26)
        {
            values[0] = 4; values[2] = 2000000; values[3] = index == 26 ? 64 : 256;
        }
        std::vector<std::string> arguments(1, mode);
        char number[32];
        for (size_t arg = 0; arg < 4; ++arg)
        {
            std::sprintf(number, "%u", values[arg]);
            arguments.push_back(number);
        }
        // 此专用命令的环境仅由本线程修改；子进程继承测试器亲和性。
        const char* previous = std::getenv("LIBUV_TEST_FIXED_CPUS");
        const std::string saved = previous == nullptr ? "" : previous;
        const bool hadPrevious = previous != nullptr;
        if ((fixed ? uv_os_setenv("LIBUV_TEST_FIXED_CPUS", "1") : uv_os_unsetenv("LIBUV_TEST_FIXED_CPUS")) != 0)
        {
            return 2;
        }
        std::vector<double> samples[2][8];
        bool good = true;
        try
        {
            for (uint32_t round = 0; round <= parsed[0] && good; ++round)
            {
                for (uint32_t order = 0; order < (compare ? 2U : 1U); ++order)
                {
                    const uint32_t version = compare ? (order + round % 2) % 2 : 1;
                    LibuvTcpPerformance child;
                    std::string output;
                    double metrics[8];
                    good = child.execute(executables[version], arguments, output) &&
                        parse(output, kind, 8000000, metrics);
                    std::printf("SAMPLE,%u,%s,%u,%s,%s\n%s", test, version == 0 ? "before" : "after",
                        round, round == 0 ? "warmup" : "formal", good ? "PASS" : "FAIL", output.c_str());
                    std::fflush(stdout);
                    if (!good)
                    {
                        break;
                    }
                    if (round != 0)
                    {
                        for (size_t metric = 0; metric < 8; ++metric)
                        {
                            if (metrics[metric] >= 0)
                            {
                                samples[version][metric].push_back(metrics[metric]);
                            }
                        }
                    }
                }
            }
        }
        catch (...)
        {
            good = false;
        }
        const int restored = hadPrevious ? uv_os_setenv("LIBUV_TEST_FIXED_CPUS", saved.c_str()) :
            uv_os_unsetenv("LIBUV_TEST_FIXED_CPUS");
        if (!good || restored != 0)
        {
            return 1;
        }
        for (size_t metric = 0; metric < 8; ++metric)
        {
            if (samples[1][metric].empty())
            {
                continue;
            }
            double median[2] = {};
            for (size_t version = compare ? 0 : 1; version < 2; ++version)
            {
                std::vector<double>& data = samples[version][metric];
                std::sort(data.begin(), data.end());
                median[version] = (data[(data.size() - 1) / 2] + data[data.size() / 2]) / 2;
            }
            std::printf("SUMMARY,%u,%s,before=", test, names[metric]);
            if (compare)
            {
                std::printf("%.6f", median[0]);
            }
            std::printf(",after=%.6f,change_percent=", median[1]);
            if (compare && median[0] != 0)
            {
                std::printf("%.6f", 100 * (median[1] / median[0] - 1));
            }
            std::printf(",after_min=%.6f,after_max=%.6f\n", samples[1][metric].front(), samples[1][metric].back());
        }
        std::fflush(stdout);
    }
    return 0;
}