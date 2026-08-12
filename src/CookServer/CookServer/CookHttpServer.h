#pragma once
#include "CookApiService.h"
#include "HttpService/HttpServiceAPI.h"
#include <memory>

class CookHttpApiHandler;

/** CookServer的HTTP监听封装，负责注册API路由并管理HttpService生命周期
*/
class CookHttpServer
{
public:
	/** 构造函数
	@param [in] apiService 业务接口服务，生命周期必须长于CookHttpServer，可以为空
	*/
	explicit CookHttpServer(CookApiService* apiService);

	/** 析构函数，会停止HTTP服务并释放内部处理器
	*/
	~CookHttpServer();

	/** 启动HTTP服务并注册CookServer API路由
	@return 返回true表示启动成功或服务已经在运行
	*/
	bool start();

	/** 停止HTTP服务
	*/
	void stop();

private:
	/** 禁止拷贝构造，避免多个HTTP服务对象同时管理同一个监听和处理器
	@param [in] other 另一个HTTP服务对象
	*/
	CookHttpServer(const CookHttpServer& other);

	/** 禁止赋值，避免HTTP服务生命周期和底层监听状态被复制
	@param [in] other 另一个HTTP服务对象
	@return 返回当前对象引用
	*/
	CookHttpServer& operator=(const CookHttpServer& other);

	// 业务接口服务指针，不负责释放
	CookApiService* m_apiService;
	// HttpService路由处理器，由CookHttpServer创建并用智能指针管理生命周期
	std::shared_ptr<CookHttpApiHandler> m_handler;
	// 底层HTTP服务封装
	HttpService m_httpService;
};