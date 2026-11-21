#ifdef __EMSCRIPTEN__
#include "HttpRequestWork.h"
#include <emscripten/emscripten.h>
#include <cstdlib>

/** 为宿主回传结果分配 Wasm 线性内存。
@param [in] bytes 请求的字节数
@return 成功返回由 HttpClientDeliver 接管的缓冲区，分配失败返回空指针
*/
extern "C" EMSCRIPTEN_KEEPALIVE uint8_t* HttpClientAllocate(uint32_t bytes)
{
    return static_cast<uint8_t*>(std::malloc(bytes));
}

/** 将宿主结果交付给请求回调并释放宿主填好的缓冲区。
@param [in] id 请求标识
@param [in] status HTTP 状态码，0 表示宿主错误
@param [in] payload 已分配的连续数据，依次为区间字段、实体标签和正文；分配失败时为空
@param [in] rangeSize Content-Range 字节数
@param [in] etagSize ETag 字节数
@param [in] bodySize 正文或错误文本的字节数
*/
extern "C" EMSCRIPTEN_KEEPALIVE void HttpClientDeliver(uint32_t id, int32_t status, uint8_t* payload,
    uint32_t rangeSize, uint32_t etagSize, uint32_t bodySize)
{
    std::unique_ptr<uint8_t, void(*)(void*)> memory(payload, std::free);
    HttpClientResponse response;
    try
    {
        if (!payload) { response.m_error = "HTTP host allocation failed"; }
        else
        {
            const char* data = reinterpret_cast<const char*>(payload);
            response.m_status = status;
            if (status == 0) { response.m_error.assign(data, bodySize); }
            else
            {
                response.m_contentRange.assign(data, rangeSize);
                response.m_etag.assign(data + rangeSize, etagSize);
                response.m_body.assign(data + rangeSize + etagSize, bodySize);
            }
        }
    }
    catch (...) { response = HttpClientResponse(); response.m_error = "HTTP response allocation failed"; }
    HttpRequestFinish(id, response);
}

EM_JS_DEPS(http_request_host, "$UTF8ToString,$runtimeKeepalivePush,$runtimeKeepalivePop");
/** 在宿主事件循环中发送请求，通过 HttpClientDeliver 完成一次交付。
@param [in] id 请求标识
@param [in] post 非零时发 POST，否则发 GET
@param [in] address UTF-8 URL
@param [in] data POST 正文首地址，进入异步阶段前复制
@param [in] size POST 正文字节数
@param [in] range Range 头值，空字符串表示省略
@param [in] ifRange If-Range 头值，空字符串表示省略
@param [in] ifNone If-None-Match 头值，空字符串表示省略
@param [in] contentType 请求的 Content-Type 头值，空字符串表示省略
@param [in] timeout 超时毫秒数
@param [in] limit 允许的最大响应正文字节数
*/
EM_JS(void, HttpClientStart, (uint32_t id, int post, const char* address, const char* data, uint32_t size,
    const char* range, const char* ifRange, const char* ifNone, const char* contentType, uint32_t timeout, uint32_t limit), {
    limit = Math.min(limit >>> 0, 2147483647);
    const str = ptr => UTF8ToString(Number(ptr));
    const url = str(address);
    const bytes = post ? HEAPU8.slice(Number(data), Number(data) + size) : undefined;
    const headers = {};
    for (const pair of [['Range',range],['If-Range',ifRange],['If-None-Match',ifNone],['Content-Type',contentType]]) {
        const value = str(pair[1]);
        if (value) headers[pair[0]] = value;
    }
    const method = post ? 'POST' : 'GET';
    const ascii = s => Uint8Array.from(s, c => c.charCodeAt(0) & 255);
    let settled = false, timer, abort;
    const finish = (status, body, cr, etag) => {
        if (settled) return;
        settled = true;
        clearTimeout(timer);
        try {
            const a = ascii(cr || ""), b = ascii(etag || "");
            const total = a.length + b.length + body.length;
            if (total > 2147483647) { _HttpClientDeliver(id,0,typeof address === 'bigint' ? BigInt(0) : 0,0,0,0); return; }
            const ptr = _HttpClientAllocate(Math.max(1,total));
            if (ptr) {
                const offset = Number(ptr);
                HEAPU8.set(a,offset);
                HEAPU8.set(b,offset+a.length);
                HEAPU8.set(body,offset+a.length+b.length);
            }
            _HttpClientDeliver(id,status,ptr,a.length,b.length,body.length);
        } finally { runtimeKeepalivePop(); }
    };
    const fail = e => finish(0,ascii(String(e || 'HTTP request failed').slice(0,512)));
    runtimeKeepalivePush();
    timer = setTimeout(() => { fail('HTTP timeout'); if (abort) abort(); }, timeout);
    Promise.resolve().then(async () => {
        if (typeof wx !== 'undefined' && wx.request) {
            const task = wx.request({url, method, header:headers, timeout, responseType:'arraybuffer',
                data:post ? bytes.buffer : undefined,
                success:r => {
                    if (!(r.data instanceof ArrayBuffer) || r.data.byteLength > limit) { fail('Invalid or oversized HTTP response'); return; }
                    const h = {};
                    for (const key of Object.keys(r.header || {})) h[key.toLowerCase()] = String(r.header[key]);
                    finish(r.statusCode,new Uint8Array(r.data),h['content-range'],h['etag']);
                }, fail:e => fail(e.errMsg)});
            abort = () => task.abort();
            if (task.onHeadersReceived) task.onHeadersReceived(r => {
                for (const key of Object.keys(r.header || {})) {
                    if (key.toLowerCase() === 'content-length' && Number(r.header[key]) > limit) { fail('HTTP response too large'); abort(); }
                }
            });
            return;
        }
        const controller = new AbortController();
        abort = () => controller.abort();
        const r = await fetch(url,{method,headers,body:bytes,signal:controller.signal});
        if (!r.status) throw Error('HTTP response unavailable');
        if (Number(r.headers.get('content-length')) > limit) { abort(); throw Error('HTTP response too large'); }
        const chunks = [];
        let total = 0;
        if (r.body) {
            const reader = r.body.getReader();
            while (true) {
                const part = await reader.read();
                if (part.done) break;
                total += part.value.length;
                if (total > limit) { abort(); throw Error('HTTP response too large'); }
                chunks.push(part.value);
            }
        }
        const out = new Uint8Array(total);
        let at = 0;
        for (const chunk of chunks) { out.set(chunk,at); at += chunk.length; }
        finish(r.status,out,r.headers.get('content-range'),r.headers.get('etag'));
    }).catch(fail);
});

void HttpRequestSend(const HttpRequestWork& work)
{
    const HttpClientOptions& options = work.m_options;
    HttpClientStart(work.m_id, work.m_post, work.m_url.c_str(), work.m_body.data(), static_cast<uint32_t>(work.m_body.size()),
        options.m_range.c_str(), options.m_ifRange.c_str(), options.m_ifNoneMatch.c_str(), options.m_contentType.c_str(),
        options.m_timeoutMs, options.m_maxResponseBytes);
}
#endif