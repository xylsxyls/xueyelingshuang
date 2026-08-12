#include "CookHttpApiHandler.h"
#include "LogManager/LogManagerAPI.h"

CookHttpApiHandler::CookHttpApiHandler(CookApiService* apiService) :
m_apiService(apiService)
{

}

HttpResponse CookHttpApiHandler::handle(const HttpRequest& request)
{
	if (m_apiService == nullptr)
	{
		LOGERROR("CookHttpApiHandler handle failed, api service is null method=%s uri=%s remoteIp=%s remotePort=%d",
		         request.m_method.c_str(),
		         request.m_uri.c_str(),
		         request.m_remoteIp.c_str(),
		         request.m_remotePort);
		return HttpResponse::json("{\"ok\":false,\"message\":\"api service is null\"}", kHttpStatusInternalServerError);
	}
	return m_apiService->handle(request);
}