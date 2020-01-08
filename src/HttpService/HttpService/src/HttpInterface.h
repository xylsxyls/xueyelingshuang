#pragma once
#include "civetweb/CivetServer.h"
#include <string>
#include <mutex>
#include "HttpServiceMacro.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

struct mg_connection;

/** http接口类
*/
class HttpServiceAPI HttpInterface : public CivetHandler
{
public:
	/** get方法
	@param [in] result 请求字符串
	@return 处理是否正常
	*/
	virtual bool HttpGetReceive(const std::string& request);

	/** post方法
	@param [in] result 请求字符串
	@return 处理是否正常
	*/
	virtual bool HttpPostReceive(const std::string& request);

	/** 写入返回正确
	*/
	void WriteSuccess();

	/** 写入返回结果
	@param [in] result 返回字符串
	*/
	void WriteResult(const std::string& result);

protected:
	virtual bool handleGet(CivetServer *server, struct mg_connection *conn);
	virtual bool handlePost(CivetServer *server, struct mg_connection *conn);

protected:
	struct mg_connection* conn_;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::mutex mutex_;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};