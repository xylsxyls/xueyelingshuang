#include "HandlerContext.h"
#include "HttpService.h"
#include <stddef.h>

HandlerContext::HandlerContext() :
m_service(nullptr),
m_handler(nullptr)
{

}

HandlerContext::HandlerContext(HttpService* service, const std::shared_ptr<HttpHandler>& handler) :
m_service(service),
m_handler(handler)
{

}

int32_t HandlerContext::operator()(void* conn) const
{
    std::shared_ptr<HttpHandler> handler;
    {
        std::lock_guard<std::mutex> lock(m_handlerMutex);
        handler = m_handler;
    }
    if (m_service == nullptr || !handler)
    {
        return 0;
    }
    return m_service->handleContextConnection(conn, handler);
}

void HandlerContext::setHandler(const std::shared_ptr<HttpHandler>& handler)
{
    std::lock_guard<std::mutex> lock(m_handlerMutex);
    m_handler = handler;
}