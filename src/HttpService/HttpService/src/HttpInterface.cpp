#include "HttpInterface.h"
#include <stdio.h>

HttpInterface::HttpInterface()
{

}

HttpInterface::~HttpInterface()
{

}

bool HttpInterface::httpGetReceive(const std::string& request)
{
    (void)request;
    printf("HttpInterface httpGetReceive false\n");
    fflush(stdout);
    return false;
}

bool HttpInterface::httpPostReceive(const std::string& request)
{
    (void)request;
    printf("HttpInterface httpPostReceive false\n");
    fflush(stdout);
    return false;
}

HttpResponse HttpInterface::handle(const HttpRequest& request)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_currentRequest = request;
    m_currentResponse = HttpResponse::json("");

    bool ok = false;
    if (request.m_method == "GET")
    {
        ok = httpGetReceive(request.m_queryString);
    }
    else if (request.m_method == "POST")
    {
        ok = httpPostReceive(request.m_body);
    }
    else
    {
        m_currentResponse = HttpResponse::text("Method Not Allowed", kHttpStatusMethodNotAllowed);
        m_currentResponse.setHeader("Allow", "GET, POST, OPTIONS");
        return m_currentResponse;
    }

    if (!ok && m_currentResponse.m_body.empty())
    {
        m_currentResponse = HttpResponse::text("Not Found", kHttpStatusNotFound);
    }
    return m_currentResponse;
}

void HttpInterface::writeSuccess()
{
    m_currentResponse.setStatus(kHttpStatusOk);
    m_currentResponse.m_contentType = "application/json; charset=utf-8";
}

void HttpInterface::writeResult(const std::string& result)
{
    m_currentResponse.m_body += result;
}

void HttpInterface::setStatus(int32_t statusCode, const std::string& statusText)
{
    m_currentResponse.setStatus(statusCode);
    if (!statusText.empty())
    {
        m_currentResponse.m_statusText = statusText;
    }
}

void HttpInterface::setHeader(const std::string& name, const std::string& value)
{
    m_currentResponse.setHeader(name, value);
}

const HttpRequest& HttpInterface::currentRequest() const
{
    return m_currentRequest;
}