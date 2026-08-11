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
    if (m_service == nullptr || m_handler.get() == nullptr)
    {
        return 0;
    }
    return m_service->handleContextConnection(conn, m_handler);
}