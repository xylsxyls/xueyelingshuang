#include "HttpResponse.h"
#include "HttpServiceHelper.h"
#include "civetweb.h"

HttpResponse::HttpResponse() :
m_statusCode(kHttpStatusOk),
m_statusText(defaultStatusText(kHttpStatusOk)),
m_contentType("application/json; charset=utf-8"),
m_closeConnection(true)
{

}

HttpResponse::HttpResponse(int32_t code) :
m_statusCode(code),
m_statusText(defaultStatusText(code)),
m_contentType("application/json; charset=utf-8"),
m_closeConnection(true)
{

}

std::string HttpResponse::defaultStatusText(int32_t code)
{
    const auto it = code >= kHttpStatusOk && code <= kHttpStatusServiceUnavailable ?
        kHttpStatusTextMap.find(static_cast<HttpStatus>(code)) : kHttpStatusTextMap.end();
    if (it != kHttpStatusTextMap.end())
    {
        return it->second;
    }
    return mg_get_response_code_text(nullptr, code);
}

HttpResponse HttpResponse::json(const std::string& bodyText, int32_t code)
{
    HttpResponse response(code);
    response.m_contentType = "application/json; charset=utf-8";
    response.m_body = bodyText;
    return response;
}

HttpResponse HttpResponse::text(const std::string& bodyText, int32_t code)
{
    HttpResponse response(code);
    response.m_contentType = "text/plain; charset=utf-8";
    response.m_body = bodyText;
    return response;
}

void HttpResponse::setStatus(int32_t code)
{
    m_statusCode = code;
    m_statusText = defaultStatusText(code);
}

void HttpResponse::setHeader(const std::string& name, const std::string& value)
{
    if (!HttpServiceHelper::validName(name) || !HttpServiceHelper::validValue(value))
    {
        return;
    }
    for (auto& field : m_headers)
    {
        if (HttpServiceHelper::sameName(field.first, name))
        {
            field.second = value;
            return;
        }
    }
    m_headers.emplace(name, value);
}