#ifndef __EMSCRIPTEN__
#include "HttpRequestWork.h"
#define CURL_STATICLIB
#include <curl/curl.h>
#include <cstring>
#include <memory>
#include <mutex>

#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "secur32.lib")
#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "normaliz.lib")
#pragma comment(lib, "iphlpapi.lib")
#ifdef _DEBUG
#pragma comment(lib, "libcurld.lib")
#else
#pragma comment(lib, "libcurl.lib")
#endif
#endif

HttpTransfer::HttpTransfer(size_t limit) :
    m_limit(limit)
{

}

HttpNativeConnection::HttpNativeConnection(const std::string& origin, const std::string& authority) :
    m_origin(origin),
    m_authority(authority),
    m_handle(curl_easy_init())
{

}

HttpNativeConnection::~HttpNativeConnection()
{
    if (m_handle)
    {
        curl_easy_cleanup(m_handle);
    }
}

/** 接收 libcurl 交付的正文，并检查累计长度。
@param [in] data 本次正文
@param [in] size 元素大小
@param [in] count 元素数量
@param [in,out] context 当前传输的响应与上限
@return 消费的字节数，达到上限或分配失败时返回零
*/
static size_t ReceiveBody(char* data, size_t size, size_t count, void* context)
{
    HttpTransfer& transfer = *static_cast<HttpTransfer*>(context);
    if (size && count > (transfer.m_limit - transfer.m_response.m_body.size()) / size)
    {
        return 0;
    }
    try
    {
        transfer.m_response.m_body.append(data, size * count);
    }
    catch (...)
    {
        return 0;
    }
    return size * count;
}

/** 从 libcurl 的响应头中提取 ETag 和 Content-Range。
@param [in] data 本次响应行
@param [in] size 元素大小
@param [in] count 元素数量
@param [in,out] context 当前响应的正文与头字段
@return 成功时消费完整行，失败返回零
*/
static size_t ReceiveHeader(char* data, size_t size, size_t count, void* context)
{
    HttpClientResponse& response = static_cast<HttpTransfer*>(context)->m_response;
    try
    {
        const size_t length = size * count;
        if (length >= 5 && std::memcmp(data, "HTTP/", 5) == 0)
        {
            response.m_body.clear();
            response.m_etag.clear();
            response.m_contentRange.clear();
        }
        size_t field = 0;
        std::string* target = nullptr;
        if (length >= 5 && curl_strnequal(data, "ETag:", 5))
        {
            field = 5;
            target = &response.m_etag;
        }
        else if (length >= 14 && curl_strnequal(data, "Content-Range:", 14))
        {
            field = 14;
            target = &response.m_contentRange;
        }
        if (target)
        {
            size_t end = length;
            while (field < end && (data[field] == ' ' || data[field] == '\t')) { ++field; }
            while (end > field && (data[end-1] == ' ' || data[end-1] == '\t' || data[end-1] == '\r' || data[end-1] == '\n')) { --end; }
            target->assign(data + field, end - field);
        }
    }
    catch (...)
    {
        return 0;
    }
    return size * count;
}

#if defined(__ANDROID__) || defined(__APPLE__)
/** 检查 Apple 或 Android 使用的 TLS 后端及本机信任实现。
@return 当前构建具有系统根证书支持时返回 true
*/
static bool HasNativeTrust()
{
    const curl_version_info_data* version = curl_version_info(CURLVERSION_NOW);
    if (!version || !version->ssl_version)
    {
        return false;
    }
    if (curl_strnequal(version->ssl_version, "rustls", 6))
    {
        return version->version_num >= 0x080d00;
    }
    if (std::strncmp(version->ssl_version, "wolfSSL/", 8) == 0)
    {
        return version->version_num >= 0x080300;
    }
#if defined(__APPLE__) && LIBCURL_VERSION_NUM >= 0x075700
    if (version->age >= CURLVERSION_ELEVENTH && version->feature_names)
    {
        for (const char* const* feature = version->feature_names; *feature; ++feature)
        {
            if (curl_strequal(*feature, "AppleSecTrust"))
            {
                return true;
            }
        }
    }
#endif
    return false;
}
#endif

/** 为一次请求设置 URL、TLS 验证、超时及读写回调。
@param [in,out] curl 本次传输独占的 easy 句柄
@param [in] work 不可变的请求和选项
@param [in,out] transfer 接收响应与选项错误的上下文
@param [in,out] headers 调用者释放的 libcurl 请求头链表
@return 成功返回 CURLE_OK，失败返回首个出错的状态
*/
static CURLcode ConfigureTransfer(CURL* curl, const HttpRequestWork& work, HttpTransfer& transfer, curl_slist*& headers)
{
    const HttpClientOptions& options = work.m_options;
    CURLcode result = CURLE_OK;
    // 任一安全或资源选项设置失败即结束，避免配置只生效一部分。
#define HTTP_SET(option, value) do { result = curl_easy_setopt(curl, option, value); if (result != CURLE_OK) { transfer.m_response.m_error = #option; return result; } } while (0)
    HTTP_SET(CURLOPT_URL, work.m_url.c_str());
    HTTP_SET(CURLOPT_PROTOCOLS_STR, "http,https");
    HTTP_SET(CURLOPT_REDIR_PROTOCOLS_STR, work.m_url.compare(0, 8, "https://") == 0 ? "https" : "http,https");
    HTTP_SET(CURLOPT_FOLLOWLOCATION, 1L);
    HTTP_SET(CURLOPT_MAXREDIRS, static_cast<long>(options.m_maxRedirects));
    HTTP_SET(CURLOPT_MAXCONNECTS, 1L);
    HTTP_SET(CURLOPT_NOSIGNAL, 1L);
    HTTP_SET(CURLOPT_TIMEOUT_MS, static_cast<long>(options.m_timeoutMs));
    HTTP_SET(CURLOPT_SSL_VERIFYPEER, 1L);
    HTTP_SET(CURLOPT_SSL_VERIFYHOST, 2L);
    const std::string& authority = options.m_authorityPem;

    if (!authority.empty())
    {
        curl_blob blob = {const_cast<char*>(authority.data()), authority.size(), CURL_BLOB_COPY};
        // Schannel没有CA目录；支持目录的后端先清除系统目录，再安装本请求的信任根。
        result = curl_easy_setopt(curl, CURLOPT_CAPATH, static_cast<const char*>(nullptr));
        if (result != CURLE_OK && result != CURLE_NOT_BUILT_IN)
        {
            return result;
        }
#ifdef _WIN32
        // 私有CA可能没有发布吊销信息；已经明确吊销的证书仍然拒绝。
        HTTP_SET(CURLOPT_SSL_OPTIONS, static_cast<long>(CURLSSLOPT_REVOKE_BEST_EFFORT));
#endif
        HTTP_SET(CURLOPT_CAINFO_BLOB, &blob);
    }
#if defined(__APPLE__) || defined(__ANDROID__)
    else
    {
        if (!HasNativeTrust())
        {
            transfer.m_response.m_error = "libcurl TLS backend lacks native certificate trust for this platform";
            return CURLE_NOT_BUILT_IN;
        }
        HTTP_SET(CURLOPT_SSL_OPTIONS, static_cast<long>(CURLSSLOPT_NATIVE_CA));
    }
#endif
    const std::string* values[] = {&options.m_range, &options.m_ifRange, &options.m_ifNoneMatch, &options.m_contentType};
    const char* names[] = {"Range: ", "If-Range: ", "If-None-Match: ", "Content-Type: "};
    for (size_t index = 0; index < 4; ++index)
    {
        if (values[index]->empty())
        {
            continue;
        }
        curl_slist* list = curl_slist_append(headers, (names[index] + *values[index]).c_str());
        if (!list)
        {
            return CURLE_OUT_OF_MEMORY;
        }
        headers = list;
    }
    HTTP_SET(CURLOPT_HTTPHEADER, headers);
    HTTP_SET(CURLOPT_WRITEFUNCTION, ReceiveBody);
    HTTP_SET(CURLOPT_WRITEDATA, &transfer);
    HTTP_SET(CURLOPT_HEADERFUNCTION, ReceiveHeader);
    HTTP_SET(CURLOPT_HEADERDATA, &transfer);
    if (work.m_post)
    {
        HTTP_SET(CURLOPT_POSTFIELDSIZE_LARGE, static_cast<curl_off_t>(work.m_body.size()));
        HTTP_SET(CURLOPT_POSTFIELDS, work.m_body.c_str());
    }
#undef HTTP_SET
    return CURLE_OK;
}

void HttpNativeWorkers::send(const HttpRequestWork& work)
{
    // 要求支持多线程全局初始化的libcurl。进程期间共用一次运行时。
    std::call_once(m_initOnce, [this]() { m_initialized = curl_global_init(CURL_GLOBAL_DEFAULT) == CURLE_OK; });
    HttpTransfer transfer(work.m_options.m_maxResponseBytes);
    const size_t authorityStart = work.m_url.compare(0, 8, "https://") == 0 ? 8 : 7;
    const std::string origin = work.m_url.substr(0, work.m_url.find_first_of("/?#", authorityStart));
    const bool cacheable = work.m_cacheAuthority;
    std::unique_ptr<HttpNativeConnection> connection;
    {
        std::lock_guard<std::mutex> lock(m_connectionMutex);
        if (cacheable && m_idle && m_idle->m_origin == origin && m_idle->m_authority == work.m_options.m_authorityPem)
        {
            connection.swap(m_idle);
        }
    }
    if (!connection && m_initialized)
    {
        connection.reset(new HttpNativeConnection(origin, cacheable ? work.m_options.m_authorityPem : std::string()));
    }
    CURL* curl = connection ? connection->m_handle : nullptr;
    curl_slist* headers = nullptr;
    char error[CURL_ERROR_SIZE] = {};
    CURLcode code = CURLE_FAILED_INIT;
    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error);
        try
        {
            code = ConfigureTransfer(curl, work, transfer, headers);
        }
        catch (...)
        {
            curl_slist_free_all(headers);
            throw;
        }
        if (code == CURLE_OK)
        {
            code = curl_easy_perform(curl);
        }
    }
    curl_slist_free_all(headers);
    if (code == CURLE_OK)
    {
        long status = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
        transfer.m_response.m_status = static_cast<int32_t>(status);
        // 先清除 POST、头、回调及借用的指针，再归还连接。下一次发送会完整配置 TLS。
        curl_easy_reset(curl);
        if (cacheable)
        {
            std::lock_guard<std::mutex> lock(m_connectionMutex);
            connection.swap(m_idle);
        }
    }
    else
    {
        const std::string option = transfer.m_response.m_error;
        transfer.m_response = HttpClientResponse();
        transfer.m_response.m_error = (option.empty() ? option : option + " ") + (error[0] ? error : curl_easy_strerror(code));
    }
    connection.reset();
    HttpRequestFinish(work.m_id, transfer.m_response);
}

void HttpNativeWorkers::closeTransport()
{
    m_idle.reset();
    if (m_initialized)
    {
        curl_global_cleanup();
        m_initialized = false;
    }
}
#endif