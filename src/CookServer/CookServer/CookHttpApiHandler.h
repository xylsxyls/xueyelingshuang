#pragma once
#include "CookApiService.h"
#include "HttpService/HttpServiceAPI.h"

/** CookServer HTTP API路由处理器，把HttpService请求转给CookApiService
*/
class CookHttpApiHandler : public HttpHandler
{
public:
	/** 构造函数
	@param [in] apiService 业务接口服务，生命周期必须长于处理器，可以为空
	*/
	explicit CookHttpApiHandler(CookApiService* apiService);

	/** 处理HTTP请求
	@param [in] request HTTP请求
	@return 返回HTTP响应
	*/
	virtual HttpResponse handle(const HttpRequest& request);

private:
	/** 禁止拷贝构造，避免处理器里的业务服务指针被意外复制
	@param [in] other 另一个HTTP API处理器
	*/
	CookHttpApiHandler(const CookHttpApiHandler& other);

	/** 禁止赋值，避免处理器绑定的业务服务指针被覆盖
	@param [in] other 另一个HTTP API处理器
	@return 返回当前对象引用
	*/
	CookHttpApiHandler& operator=(const CookHttpApiHandler& other);

private:
	// 业务接口服务指针，不负责释放
	CookApiService* m_apiService;
};