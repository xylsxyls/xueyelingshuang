#include "HttpResponse.h"

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
    std::map<HttpStatus, std::string>::const_iterator it = kHttpStatusTextMap.find(static_cast<HttpStatus>(code));
    if (it != kHttpStatusTextMap.end())
    {
        return it->second;
    }
    return "OK";
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
    if (name.empty())
    {
        return;
    }
    m_headers[name] = value;
}