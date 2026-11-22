#include "LibuvTcpPerformance.h"
#include <algorithm>
#include <cerrno>
#include <cfloat>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <sstream>
#include <signal.h>

LibuvTcpPerformance::LibuvTcpPerformance() :
m_loop(),
m_process(),
m_pipe(),
m_timer(),
m_exited(false),
m_eof(false),
m_error(0),
m_exitCode(0)
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
                state->cancel(UV_ENOBUFS);
                return;
            }
            state->m_output.append(buffer->base, static_cast<size_t>(count));
        }
        catch (...)
        {
            state->cancel(UV_ENOMEM);
        }
    }
    else if (count < 0)
    {
        if (count != UV_EOF && state->m_error == 0)
        {
            state->m_error = static_cast<int>(count);
        }
        state->m_eof = true;
        uv_close(reinterpret_cast<uv_handle_t*>(stream), nullptr);
        state->finish();
    }
}

void LibuvTcpPerformance::exited(uv_process_t* process, int64_t code, int signal)
{
    LibuvTcpPerformance* state = static_cast<LibuvTcpPerformance*>(process->data);
    state->m_exitCode = code;
    if (signal != 0 && state->m_error == 0)
    {
        state->m_error = UV_ECANCELED;
    }
    state->m_exited = true;
    uv_close(reinterpret_cast<uv_handle_t*>(process), nullptr);
    state->finish();
}

void LibuvTcpPerformance::timeout(uv_timer_t* timer)
{
    static_cast<LibuvTcpPerformance*>(timer->data)->cancel(UV_ETIMEDOUT);
}

void LibuvTcpPerformance::cancel(int error)
{
    if (m_error == 0)
    {
        m_error = error;
    }
    if (!m_exited)
    {
        uv_process_kill(&m_process, SIGKILL);
    }
    if (!uv_is_closing(reinterpret_cast<uv_handle_t*>(&m_pipe)))
    {
        uv_close(reinterpret_cast<uv_handle_t*>(&m_pipe), nullptr);
    }
    m_eof = true;
    finish();
}

int64_t LibuvTcpPerformance::execute(const char* executable, const std::vector<std::string>& arguments,
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
    output.clear();
    int result = uv_loop_init(&m_loop);
    if (result != 0)
    {
        return result;
    }
    result = uv_pipe_init(&m_loop, &m_pipe, 0);
    if (result != 0)
    {
        const int closed = uv_loop_close(&m_loop);
        return closed != 0 ? closed : result;
    }
    result = uv_timer_init(&m_loop, &m_timer);
    if (result != 0)
    {
        uv_close(reinterpret_cast<uv_handle_t*>(&m_pipe), nullptr);
        uv_run(&m_loop, UV_RUN_DEFAULT);
        const int closed = uv_loop_close(&m_loop);
        return closed != 0 ? closed : result;
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
    result = uv_spawn(&m_loop, &m_process, &options);
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
        const int closed = uv_loop_close(&m_loop);
        return closed != 0 ? closed : result;
    }
    result = uv_read_start(reinterpret_cast<uv_stream_t*>(&m_pipe), &allocate, &read);
    if (result == 0)
    {
        result = uv_timer_start(&m_timer, &timeout, milliseconds, 1000);
    }
    if (result != 0)
    {
        cancel(result);
    }
    uv_run(&m_loop, UV_RUN_DEFAULT);
    const int closed = uv_loop_close(&m_loop);
    output.swap(m_output);
    if (closed != 0 || !m_exited || !m_eof)
    {
        return closed != 0 ? closed : UV_EIO;
    }
    return m_error != 0 ? m_error : m_exitCode;
}

bool LibuvTcpPerformance::parse(const std::string& output, int kind, uint64_t expected, double* metrics,
    const uint32_t* requested, const char* label)
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
        fields[0] != "TINYTRY" && fields[0] != "TINYTRYCOPY"))) ||
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
        const bool optionalCounter = (kind == 0 && (index == 7 || index == 8)) ||
            (kind == 1 && (index == 11 || index == 12));
        if (errno != 0 || tail == fields[index].c_str() || tail != fields[index].c_str() + fields[index].size() ||
            (!(values[index] >= 0 && values[index] <= DBL_MAX) && !(optionalCounter && values[index] == -1)))
        {
            return false;
        }
    }
    if (kind == 0 && (values[9] != static_cast<double>(expected) || values[10] != 1))
    {
        return false;
    }
    // 数量字段必须是实际整数，耗时和速率必须能由同一工作量相互推导。
    const size_t parameters = kind == 2 ? 2 : 4;
    for (size_t index = 1; index <= parameters; ++index)
    {
        if (std::floor(values[index]) != values[index] || values[index] > UINT32_MAX)
        {
            return false;
        }
    }
    if (label != nullptr && fields[0] != label)
    {
        return false;
    }
    for (size_t index = 0; requested != nullptr && index < parameters; ++index)
    {
        if (values[index + 1] != requested[kind == 2 ? index + 1 : index])
        {
            return false;
        }
    }
    const double count = kind == 2 ? values[2] : values[1] * values[3];
    const double milliseconds = values[kind == 2 ? 3 : 5];
    const double rate = values[kind == 2 ? 4 : 6];
    if (count <= 0 || milliseconds < 1e-6 || rate <= 0 ||
        (kind != 2 && (values[1] < 1 || values[1] > 16 || values[4] < 1)) ||
        (kind == 0 && (values[2] > 2 || count != values[9] || values[8] == 0)) ||
        (kind == 1 && values[12] == 0) ||
        (kind == 1 && (values[2] < 16 || values[8] > values[9] || values[9] > values[10])))
    {
        return false;
    }
    const double calculated = count * 1000.0 / milliseconds;
    if (std::fabs(rate - calculated) > 0.001 + calculated * 1e-6 / milliseconds)
    {
        return false;
    }
    if (kind == 1 && std::fabs(values[7] - calculated * values[2] / 1048576.0) >
        0.001 + calculated * values[2] / 1048576.0 * 1e-6 / milliseconds)
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

bool LibuvTcpPerformance::parseHost(const std::string& output, uint32_t producers, uint32_t payload,
    uint32_t count, const char* mode, double* metrics)
{
    if (producers == 0 || payload == 0 || count == 0 || mode == nullptr)
    {
        return false;
    }
    const bool framed = std::strcmp(mode, "framed") == 0 || std::strcmp(mode, "framedpool") == 0;
    if (!framed && std::strcmp(mode, "raw") != 0 && std::strcmp(mode, "ordered") != 0)
    {
        return false;
    }
    const size_t end = output.find_last_not_of("\r\n");
    if (end == std::string::npos)
    {
        return false;
    }
    const size_t start = output.rfind('\n', end);
    std::istringstream line(output.substr(start == std::string::npos ? 0 : start + 1,
        end + 1 - (start == std::string::npos ? 0 : start + 1)));
    std::string word;
    if (!(line >> word) || word != "HOST_POST" || !(line >> word) || word != std::string("mode=") + mode)
    {
        return false;
    }
    const char* keys[] = { "producers=", "payload=", "count=", "warmup=", "received=", "bytes=",
        "seconds=", "mps=", "submitSeconds=", "retries=" };
    double values[10] = {};
    for (size_t index = 0; index < 10; ++index)
    {
        if (!(line >> word) || word.compare(0, std::strlen(keys[index]), keys[index]) != 0)
        {
            return false;
        }
        const char* text = word.c_str() + std::strlen(keys[index]);
        char* tail = nullptr;
        errno = 0;
        values[index] = std::strtod(text, &tail);
        if (errno != 0 || tail == text || tail != word.c_str() + word.size() || !(values[index] >= 0 && values[index] <= DBL_MAX) ||
            ((index < 6 || index == 9) && std::floor(values[index]) != values[index]))
        {
            return false;
        }
    }
    if (!(line >> word) || word != "status=PASS" || (line >> word))
    {
        return false;
    }
    const uint32_t warmup = (std::min)(100000U, 8388608U / payload) / producers * producers;
    const uint64_t bytes = static_cast<uint64_t>(count) * (static_cast<uint64_t>(payload) + (framed ? 4 : 0));
    if (values[0] != producers || values[1] != payload || values[2] != count || values[3] != warmup ||
        values[4] != static_cast<uint64_t>(warmup) + count || values[5] != static_cast<double>(bytes) ||
        values[6] < 1e-9 || values[6] > DBL_MAX / 1000 || values[7] <= 0 || values[8] <= 0)
    {
        return false;
    }
    // 输出耗时保留纳秒、速率保留小数后三位，只容忍这两个字段的舍入误差。
    const double rate = static_cast<double>(count) / values[6];
    if (std::fabs(values[7] - rate) > 0.001 + rate * 1e-9 / values[6])
    {
        return false;
    }
    std::fill(metrics, metrics + 8, -1.0);
    metrics[0] = values[7];
    metrics[1] = values[6] * 1000;
    metrics[4] = bytes / values[6] / 1048576.0;
    return true;
}

bool LibuvTcpPerformance::checkParser()
{
    double metrics[8];
    const std::string host = "HOST_POST mode=raw producers=2 payload=6 count=8 warmup=100000 received=100008 bytes=48 seconds=0.002000000 mps=4000.000 submitSeconds=0.001900000 retries=3 status=PASS\n";
    if (!parseHost(host, 2, 6, 8, "raw", metrics) || metrics[0] != 4000 || metrics[2] != -1 ||
        parseHost(host, 1, 6, 8, "raw", metrics) || parseHost(host, 2, 7, 8, "raw", metrics) ||
        parseHost(host, 2, 6, 9, "raw", metrics) || parseHost(host, 2, 6, 8, "framed", metrics))
    {
        return false;
    }
    const char* valid[] = { "received=100008", "bytes=48", "seconds=0.002000000", "mps=4000.000",
        "retries=3", "status=PASS", "payload=6" };
    const char* invalid[] = { "received=100007", "bytes=47", "seconds=nan", "mps=3990.000",
        "retries=1.5", "status=FAIL", "payload=6junk" };
    for (size_t index = 0; index < 7; ++index)
    {
        std::string changed = host;
        changed.replace(changed.find(valid[index]), std::strlen(valid[index]), invalid[index]);
        if (parseHost(changed, 2, 6, 8, "raw", metrics))
        {
            return false;
        }
    }
    if (parseHost(std::string(host).insert(host.find("payload=6") + 9, 1, '\0'), 2, 6, 8, "raw", metrics) ||
        parseHost(host.substr(0, host.find(" status=")), 2, 6, 8, "raw", metrics) ||
        parseHost(host + "extra", 2, 6, 8, "raw", metrics) || parseHost("", 2, 6, 8, "raw", metrics))
    {
        return false;
    }
    const uint32_t expectedInput[4] = {1, 0, 8, 1};
    if (!parse("TINY,1,0,8,1,2,4000,1,100,8,1\n", 0, 8, metrics, expectedInput, "TINY") ||
        parse("TINY,2,0,4,1,2,4000,1,100,8,1\n", 0, 8, metrics, expectedInput, "TINY") ||
        parse("TINYTRY,1,0,8,1,2,4000,1,100,8,1\n", 0, 8, metrics, expectedInput, "TINY"))
    {
        return false;
    }
    return parse("TINY,1,0,8,1,2,4000,1,100,8,1\n", 0, 8, metrics) && metrics[0] == 4000 &&
        parse("TINY,1,0,8,1,2,4000,-1,-1,8,1\n", 0, 8, metrics) && metrics[2] == -1 && metrics[3] == -1 &&
        !parse("TINY,1,0,8,1,2,4000,1,0,8,1\n", 0, 8, metrics) &&
        !parse("TINY,1,0,8,1,2,4000,-2,100,8,1\n", 0, 8, metrics) &&
        !parse("TINY,1,0,8,1,2,4000,1,100,7,1\n", 0, 8, metrics) &&
        !parse("TINY,1,0,8,1,2,4000,1,100,8,0\n", 0, 8, metrics) &&
        !parse(std::string("RAW,6,10,1,10000") + '\0' + "junk", 2, 0, metrics) &&
        !parse("RAW,6,10,1e-290,1000\n", 2, 0, metrics) &&
        !parse("RAW,6,10,1,nan\n", 2, 0, metrics) &&
        !parse("RAW,6,10,1,inf\n", 2, 0, metrics) &&
        !parse("WRONG,1,0,8,1,2,4000,1,100,8,1\n", 0, 8, metrics) &&
        !parse("RAW,6,10,1,10junk\n", 2, 0, metrics) &&
        !parse("RAW,6,10,1\n", 2, 0, metrics) &&
        !parse("", 2, 0, metrics) &&
        parse("RAW,6,10,1,10000\n", 2, 0, metrics) && metrics[2] == -1 &&
        parse("BENCH,1,16,100,128,2,50000,0.763,2,3,4,5,6\n", 1, 0, metrics) && metrics[7] == 4 &&
        !parse("TINY,1,0,9,1,2,4000,1,100,8,1\n", 0, 8, metrics) &&
        !parse("TINY,1,0,8,1,2,40000,1,100,8,1\n", 0, 8, metrics) &&
        !parse("RAW,6,10,1,100000\n", 2, 0, metrics) &&
        !parse("RAW,6,10.5,1,10500\n", 2, 0, metrics) &&
        !parse("BENCH,1,16,100,128,2,50000,1,2,3,4,5,6\n", 1, 0, metrics) &&
        !parse("BENCH,1,16,100,128,2,50000,0.763,4,3,2,5,6\n", 1, 0, metrics);
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
    const int64_t expected[] = { 0, 7, UV_ENOBUFS, UV_ETIMEDOUT };
    for (uint32_t index = 0; index < 4; ++index)
    {
        LibuvTcpPerformance child;
        const std::vector<std::string> args = { "--runner-probe", modes[index] };
        std::string output;
        const int64_t result = child.execute(self, args, output, index == 3 ? 200 : 5000);
        if (result != expected[index] || (index == 0 && output.find("runner-ok") == std::string::npos))
        {
            return false;
        }
    }
    LibuvTcpPerformance missing;
    std::string output;
    const std::vector<std::string> empty;
    return missing.execute((std::string(self) + ".missing").c_str(), empty, output) == UV_ENOENT;
}

int LibuvTcpPerformance::run(int argc, char** argv)
{
    if (argc != 3 && argc != 4)
    {
        std::fputs("usage: --suite rounds(9-100) [case:0-46]\n", stderr);
        return 2;
    }
    uint32_t parsed[2] = {};
    for (size_t index = 0; index < (argc == 4 ? 2U : 1U); ++index)
    {
        const char* input = argv[index + 2];
        char* tail = nullptr;
        errno = 0;
        const unsigned long value = std::strtoul(input, &tail, 10);
        if (*input < '0' || *input > '9' || tail == input || *tail != 0 || errno != 0 ||
            value > (index == 0 ? 100U : 46U) || (index == 0 && value < 9))
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
    const char* names[] = { "rate", "ms", "cpu_ms", "peak_bytes", "mib_s", "p50_us", "p95_us", "p99_us" };
    const char* modes[] = { "--tinycopybenchmark", "--tinytrycopybenchmark", "--tinybenchmark", "--tinytrybenchmark" };
    const uint32_t latency[10][4] = { {1,16,50000,1}, {1,16,100000,128}, {4,16,100000,128},
        {16,16,50000,128}, {1,64,100000,128}, {16,64,50000,128}, {1,1024,50000,128},
        {16,1024,10000,128}, {1,65536,4000,32}, {16,65536,1000,32} };
    const uint32_t raw[4][4] = { {1,6,100000,32}, {1,64,100000,32}, {1,1024,50000,32}, {1,32768,4000,32} };
    for (uint32_t test = 1; test <= 46; ++test)
    {
        if (parsed[1] != 0 && parsed[1] != test)
        {
            continue;
        }
        const uint32_t index = test - 1;
        const bool host = test >= 29;
        const char* hostMode = test <= 32 ? "raw" : test <= 36 ? "framed" :
            test <= 40 ? "framedpool" : test <= 44 ? "ordered" : "raw";
        #ifdef _WIN32
        const bool fixed = index < 12 && index % 3 == 0;
#else
        const bool fixed = false;
#endif
        const int kind = index < 12 || index >= 26 ? 0 : index < 22 ? 1 : 2;
        const char* mode = host ? "--hostbenchmark" : index < 12 ? modes[index / 3] : index < 22 ? "--benchmark" :
            index < 26 ? "--rawbenchmark" : "--tinybenchmark";
        uint32_t values[4] = { index % 3 == 2 ? 16U : 1U, 0, index % 3 == 2 ? 500000U : 8000000U, 1 };
        if (host)
        {
            values[0] = test <= 44 ? 1U << ((test - 29) % 4) : test == 45 ? 1 : 4;
            values[1] = test <= 44 ? 6 : 16384;
            values[2] = test <= 40 ? 10000000 : test <= 44 ? 1000000 : 65536;
        }
        else if (index >= 12 && index < 26)
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
            if (host && arg == 3)
            {
                arguments.push_back(hostMode);
            }
            else
            {
                std::sprintf(number, "%u", values[arg]);
                arguments.push_back(number);
            }
        }
        // 此专用命令的环境仅由本线程修改；子进程继承测试器亲和性。
        const char* previous = std::getenv("LIBUV_TEST_FIXED_CPUS");
        const std::string saved = previous == nullptr ? "" : previous;
        const bool hadPrevious = previous != nullptr;
        if ((fixed ? uv_os_setenv("LIBUV_TEST_FIXED_CPUS", "1") : uv_os_unsetenv("LIBUV_TEST_FIXED_CPUS")) != 0)
        {
            return 2;
        }
        std::vector<double> samples[8];
        bool good = true;
        try
        {
            for (uint32_t round = 0; round <= parsed[0] && good; ++round)
            {
                LibuvTcpPerformance child;
                std::string output;
                double metrics[8];
                good = child.execute(self, arguments, output) == 0 &&
                    (host ? parseHost(output, values[0], values[1], values[2], hostMode, metrics) :
                        parse(output, kind, 8000000, metrics, values,
                            kind == 2 ? "RAW" : kind == 1 ? "BENCH" : index >= 26 ? "TINY" :
                            index / 3 == 0 ? "TINYCOPY" : index / 3 == 1 ? "TINYTRYCOPY" :
                            index / 3 == 2 ? "TINY" : "TINYTRY"));
                std::printf("SAMPLE,%u,%u,%s,%s\n%s", test,
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
                            samples[metric].push_back(metrics[metric]);
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
            std::vector<double>& data = samples[metric];
            if (!host && kind != 2 && (metric == 2 || metric == 3) && data.size() != parsed[0])
            {
                std::printf("SUMMARY,%u,%s,unavailable\n", test, names[metric]);
            }
            else if (!data.empty())
            {
                std::sort(data.begin(), data.end());
                const size_t last = data.size() - 1;
                const double median = (data[last / 2] + data[data.size() / 2]) / 2;
                std::printf("SUMMARY,%u,%s,median=%.6f,q1=%.6f,q3=%.6f,min=%.6f,max=%.6f\n",
                    test, names[metric], median, data[last / 4], data[3 * last / 4], data.front(), data.back());
            }
        }
        std::fflush(stdout);
    }
    return 0;
}