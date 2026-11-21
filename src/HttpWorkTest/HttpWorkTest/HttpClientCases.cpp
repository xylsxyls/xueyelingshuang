#include "HttpClientCases.h"
#include "HttpRequest/HttpRequestAPI.h"
#include "HttpWorkTest.h"
#include "PlatformTest/PlatformTestAPI.h"
#include <cstdio>
#include <atomic>
#include <fstream>
#include <iterator>
#include <memory>
#include <vector>
#include <algorithm>
#include <chrono>
#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#else
#include <condition_variable>
#include <mutex>
#endif
#ifdef _WIN32
#include <windows.h>
#endif

/** 回调延迟采用单调时钟毫秒；Windows v120 使用 QPC 以避免低分辨率计时
*/
static double ClientClockMs()
{
#ifdef __EMSCRIPTEN__
    return emscripten_get_now();
#elif defined(_WIN32)
    LARGE_INTEGER ticks, frequency;
    QueryPerformanceCounter(&ticks);
    QueryPerformanceFrequency(&frequency);
    return 1000.0 * ticks.QuadPart / frequency.QuadPart;
#else
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count();
#endif
}

HttpClientCases::HttpClientCases() :
    m_reject(false),
    m_public(false),
    m_keepAlive(false),
    m_result(-1),
    m_finished(0),
    m_correct(true),
    m_selected(kClientCoalescing - kClientBinaryGet + 1, true)
{
}

void HttpClientCases::finish(bool success, int index)
{
#ifndef __EMSCRIPTEN__
    std::lock_guard<std::mutex> lock(m_mutex);
#endif
    std::printf("HttpWorkTest client case=%d result=%s\n", index, success ? "PASS" : "FAIL");
    std::fflush(stdout);
    m_result = success ? 0 : 1;
#ifndef __EMSCRIPTEN__
    m_done.notify_one();
#else
    if (!success)
    {
        emscripten_force_exit(1);
    }
#endif
}

void HttpClientCases::runCoalescing()
{
    std::shared_ptr<HttpClientCases> self = shared_from_this();
    // 服务在收到 release 前不会交付 held 响应，两个订阅共用一次实际请求。
    auto received = [self](bool correct) {
        if (!correct) { self->m_correct.store(false); }
        if (++self->m_finished == 3)
        {
            if (self->m_correct.load()) { std::printf("client scenario %u passed\n", unsigned(kClientCoalescing)); }
            self->finish(self->m_correct.load(), 9);
        }
    };
    auto held = [received](const HttpClientResponse& r) { received(r.m_status == 200 && r.m_body == "1"); };
    for (int i = 0; i < 2; ++i)
    {
        if (!HttpRequestClient::httpGet(m_origin + "/held", m_options, held)) { received(false); }
    }
    if (!HttpRequestClient::httpGet(m_origin + "/release", m_options,
        [received](const HttpClientResponse& r) { received(r.m_status == 204); })) { received(false); }
}

void HttpClientCases::run(int index)
{
    while (index < static_cast<int>(m_selected.size()) && !m_selected[index]) { ++index; }
    if (index == static_cast<int>(m_selected.size())) { finish(true, index); return; }
    if (index + kClientBinaryGet == kClientCoalescing)
    {
        std::shared_ptr<HttpClientCases> self = shared_from_this();
        // 每次独立运行都重置 held 端点。可复用同一个源站连续运行 --stress。
        if (!HttpRequestClient::httpGet(m_origin + "/video/sample", m_options, [self,index](const HttpClientResponse& r) {
            if (r.m_status == 200 && r.m_body == std::string("A\0BCD", 5)) { self->runCoalescing(); }
            else { self->finish(false, index); }
        })) { finish(false, index); }
        return;
    }
    HttpClientOptions options = m_options;
    const char* paths[] = {"/video/sample", "/audio/sample", "/etag", "/redirect", "/missing",
        "/echo", "/video/sample", "/video/sample", "/empty"};
    const std::string url = m_public ? m_origin : m_origin + paths[index];
    if (index == 0 && !options.setMaxResponseBytes(-1)) { finish(false, index); return; }
    if (index == 1)
    {
        options.m_range = "bytes=1-3"; options.m_ifRange = "\"one\"";
    }
    if (index == 2)
    {
        options.m_ifNoneMatch = "\"one\"";
    }
    if (index == 6)
    {
        options.m_range = "bytes=99-100";
    }
    if (index == 7)
    {
        if (!options.setMaxResponseBytes(2)) { finish(false, index); return; }
    }
    std::shared_ptr<HttpClientCases> self = shared_from_this();
    HttpRequestClient::Completion completed = [self,index](const HttpClientResponse& r) {
        if (self->m_public)
        {
            if (!r.m_error.empty())
            {
                std::printf("Public TLS error: %s\n", r.m_error.c_str());
            }
            self->finish(r.m_error.empty() && r.m_status >= 200 && r.m_status < 300, index);
            return;
        }
        if (self->m_reject)
        {
            self->finish(!r.m_error.empty() && r.m_status == 0 && r.m_body.empty(), index); return;
        }
        const int statuses[] = {200,206,304,200,404,200,416,0,204};
        const std::string full("A\0BCD",5);
        const std::string bodies[] = {full,full.substr(1,3),"",full,"missing",std::string("X\0Y",3),"","",""};
        bool ok = r.m_status == statuses[index] && r.m_body == bodies[index] && (r.m_error.empty() == (index != 7));
        if (index == 1)
        {
            ok = ok && r.m_contentRange == "bytes 1-3/5" && r.m_etag == "\"one\"";
        }
        if (index == 6)
        {
            ok = ok && r.m_contentRange == "bytes */5";
        }
        if (!ok)
        {
            std::printf("status=%d bytes=%u error=%s\n",r.m_status,static_cast<unsigned>(r.m_body.size()),r.m_error.c_str());
            self->finish(false,index);
        }
        else
        {
            std::printf("client scenario %u passed\n", unsigned(index + kClientBinaryGet));
            self->run(index + 1);
        }
    };
    const bool accepted = index == 5 || m_reject ?
        HttpRequestClient::httpPost(url, std::string("X\0Y",3), options, completed).size() == 32 :
        HttpRequestClient::httpGet(url, options, completed);
    if (!accepted)
    {
        finish(false,index);
    }
}

void HttpClientCases::benchmark(size_t index)
{
    if (index == 100)
    {
        std::sort(m_samples.begin(), m_samples.end());
        std::printf("[BENCH] HttpRequest.GET.5B%s.samples=90 median_us=%.3f q1_us=%.3f q3_us=%.3f p95_us=%.3f\n",
            m_keepAlive ? ".keepalive" : "",
            (m_samples[44] + m_samples[45]) * 0.5, m_samples[22], m_samples[67], m_samples[85]);
#ifndef __EMSCRIPTEN__
        if (!m_keepAlive)
        {
            m_keepAlive = true;
            m_samples.clear();
            benchmark(0);
            return;
        }
#endif
        finish(true, 10);
        return;
    }
    std::shared_ptr<HttpClientCases> self = shared_from_this();
    const double start = ClientClockMs();
    if (!HttpRequestClient::httpGet(m_origin + (m_keepAlive ? "/keep-media" : "/video/sample"), m_options, [self, index, start](const HttpClientResponse& response) {
        const double elapsed = (ClientClockMs() - start) * 1000.0;
        if (response.m_status != 200 || !response.m_error.empty() || response.m_body != std::string("A\0BCD", 5))
        {
            self->finish(false, 10);
            return;
        }
        if (index >= 10)
        {
            self->m_samples.push_back(elapsed);
        }
        self->benchmark(index + 1);
    })) { finish(false, 10); }
}

#ifdef __EMSCRIPTEN__
static std::shared_ptr<HttpClientCases> g_hostCases;
extern "C" EMSCRIPTEN_KEEPALIVE int httpWorkTestResult()
{
    return g_hostCases ? g_hostCases->m_result : -1;
}
#endif

/** 选定本次运行的源站用例，拒绝本模式未登记的编号及多余参数。 */
static bool SelectClientCases(int argc, char* argv[], std::vector<bool>& selected)
{
    if (argc == 0 || (argc == 1 && std::strcmp(argv[0], "--all") == 0)) { return true; }
    if (argc == 1 && std::strcmp(argv[0], "--instant") == 0) { selected.back() = false; return true; }
    std::fill(selected.begin(), selected.end(), false);
    if (argc == 1 && std::strcmp(argv[0], "--stress") == 0) { selected.back() = true; return true; }
    if (argc != 2 || std::strcmp(argv[0], "--cases") != 0) { return false; }
    const char* cursor = argv[1];
    for (;;)
    {
        unsigned first = 0, last = 0;
        if (!LibraryCaseSelection::number(cursor, kClientCoalescing, first) || first < kClientBinaryGet) { return false; }
        last = first;
        if (*cursor == '-')
        {
            ++cursor;
            if (!LibraryCaseSelection::number(cursor, kClientCoalescing, last) || last < first) { return false; }
        }
        for (unsigned id = first; id <= last; ++id) { selected[id - kClientBinaryGet] = true; }
        if (*cursor == '\0') { return true; }
        if (*cursor++ != ',') { return false; }
    }
}

int HttpClientCasesMain(int argc, char* argv[])
{
#ifndef __EMSCRIPTEN__
    if (argc >= 2 && (std::string(argv[1]) == "--reuse" || std::string(argv[1]) == "--uncached-ca" ||
        std::string(argv[1]) == "--large-ca-default" || std::string(argv[1]) == "--large-ca-unlimited"))
    {
        return HttpClientReuseCases(argc, argv);
    }
#endif
    const std::string mode = argc >= 2 ? argv[1] : "";
    const bool capacity = mode == "--capacity-default" || mode == "--capacity-unlimited";
    if (argc < 3 || (mode != "--client" && argc > 4) || (mode != "--client" && mode != "--reject" && mode != "--public" &&
        mode != "--limits" && mode != "--benchmark" && !capacity))
    {
        std::puts("HttpWorkTest --client|--reject origin [CA.pem]"); return 2;
    }
    std::shared_ptr<HttpClientCases> tests(new HttpClientCases);
    tests->m_origin = argv[2];
    tests->m_reject = std::string(argv[1]) == "--reject";
    tests->m_public = std::string(argv[1]) == "--public";
    if (((mode == "--limits" || capacity) && argc != 3) || (tests->m_public && (argc != 3 || tests->m_origin.compare(0, 8, "https://") != 0)))
    {
        return 2;
    }
    if (!tests->m_options.setTimeoutMs(5000)) { return 1; }
    int nextArg = 3;
    if (nextArg < argc && (mode != "--client" || std::strncmp(argv[nextArg], "--", 2) != 0))
    {
        std::ifstream ca(argv[nextArg++], std::ios::binary);
        if (!ca)
        {
            return 2;
        }
        tests->m_options.m_authorityPem.assign(std::istreambuf_iterator<char>(ca), std::istreambuf_iterator<char>());
        if (tests->m_options.m_authorityPem.empty())
        {
            return 2;
        }
    }
    if (mode == "--client" && !SelectClientCases(argc - nextArg, argv + nextArg, tests->m_selected)) { return 2; }
#ifdef __EMSCRIPTEN__
    g_hostCases = tests;
#endif
    if (HttpRequestClient::httpGet("file:///unused", tests->m_options, [](const HttpClientResponse&) {}))
    {
        return 1;
    }
    HttpClientOptions defaults;
    const char* invalidAddresses[] = {"http://:80/media", "http://[]/media", "http://[::1/media",
        "http://[::1]extra/media", "http://127.0.0.1:12x/media", "http://127.0.0.1:65536/media"};
    for (const char* invalidAddress : invalidAddresses)
    {
        const HttpRequestClient::Completion unused = [](const HttpClientResponse&) {};
        if (HttpRequestClient::httpGet(invalidAddress, defaults, unused) ||
            !HttpRequestClient::httpPost(invalidAddress, "body", defaults, unused).empty())
        {
            std::fprintf(stderr, "accepted invalid HTTP authority: %s\n", invalidAddress);
            return 1;
        }
    }
    if (defaults.m_timeoutMs != 30000 || defaults.setTimeoutMs(0) || defaults.setTimeoutMs(UINT32_MAX) ||
        defaults.m_timeoutMs != 30000 || !defaults.setTimeoutMs(INT32_MAX) ||
        defaults.m_maxResponseBytes != 16U * 1024U * 1024U ||
        defaults.setMaxResponseBytes(0) || defaults.setMaxResponseBytes(-2) ||
        defaults.setMaxResponseBytes(INT64_C(4294967296)) || defaults.m_maxResponseBytes != 16U * 1024U * 1024U ||
        HttpRequestClient::setLimits(0) || HttpRequestClient::setLimits(-2) || HttpRequestClient::setLimits(1, -2))
    {
        return 1;
    }
    HttpClientOptions invalid = tests->m_options;
    invalid.m_contentType.assign(1, static_cast<char>(127));
    if (HttpRequestClient::httpGet(tests->m_origin, invalid, [](const HttpClientResponse&) {})) { return 1; }
    // 所有宿主应在登记请求前拒绝含 ASCII 控制字符、空格或反斜线的地址。
    for (unsigned int byte = 0; byte <= 127; ++byte)
    {
        if (byte > 32 && byte != 127 && byte != '\\') { continue; }
        const std::string url = tests->m_origin + "/a" + static_cast<char>(byte) + "b";
        if (HttpRequestClient::httpGet(url, tests->m_options, [](const HttpClientResponse&) {})) { return 1; }
    }
    if (capacity)
    {
        const bool unlimited = mode == "--capacity-unlimited";
        const int expected = unlimited ? 33 : 32;
        if ((unlimited && !HttpRequestClient::setLimits(-1)) || !tests->m_options.setTimeoutMs(2000))
        {
            return 1;
        }
        for (int i = 0; i < 33; ++i)
        {
            HttpClientOptions request = tests->m_options;
            request.m_ifNoneMatch = "\"" + std::to_string(i) + "\"";
            const bool accepted = HttpRequestClient::httpGet(tests->m_origin + "/held", request,
                [tests, expected](const HttpClientResponse& response) {
                    if (response.m_status != 0 || response.m_error.empty())
                    {
                        tests->m_correct.store(false);
                    }
                    if (++tests->m_finished == expected)
                    {
                        tests->finish(tests->m_correct.load(), 12);
                    }
                });
            if (accepted != (i < expected))
            {
                return 1;
            }
        }
    }
    else if (mode == "--limits")
    {
        // 此独立源站先阻塞首个请求。填满唯一槽时同一请求仍能订阅，不同请求必须立即拒绝。
        if (!HttpRequestClient::setLimits(-1, -1) || !HttpRequestClient::setLimits(1, 0)) { return 1; }
        if (!tests->m_options.setTimeoutMs(400)) { return 1; }
        HttpRequestClient::Completion complete = [tests](const HttpClientResponse& response) {
            if (response.m_status != 0 || response.m_error.empty()) { tests->m_correct.store(false); }
            if (++tests->m_finished == 2) { tests->finish(tests->m_correct.load(), 11); }
        };
        if (!HttpRequestClient::httpGet(tests->m_origin + "/held", tests->m_options, complete) ||
            !HttpRequestClient::httpGet(tests->m_origin + "/held", tests->m_options, complete) ||
            HttpRequestClient::setLimits() || HttpRequestClient::httpGet(tests->m_origin + "/empty", tests->m_options, complete))
        {
            return 1;
        }
    }
    else if (mode == "--benchmark")
    {
        tests->benchmark(0);
    }
    else
    {
        tests->run(0);
    }
#ifndef __EMSCRIPTEN__
    std::unique_lock<std::mutex> lock(tests->m_mutex);
    if (!tests->m_done.wait_for(lock, std::chrono::seconds(65), [tests]() { return tests->m_result >= 0; }))
    {
        return 1;
    }
    lock.unlock();
    HttpRequestClient::shutdown();
    if (HttpRequestClient::httpGet(tests->m_origin, tests->m_options, [](const HttpClientResponse&) {}))
    {
        return 1;
    }
    return tests->m_result;
#else
    return 0;
#endif
}