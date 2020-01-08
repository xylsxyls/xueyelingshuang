#pragma once
#include <stdint.h>
#include <vector>
#include <string>
#include "HttpServiceMacro.h"

class HttpInterface;
class CivetServer;

/** http服务
*/
class HttpServiceAPI HttpService
{
public:
	/** 构造函数
	*/
	HttpService();

public:
	/** 运行
	@param [in] port 端口号
	@param [in] vec_interface 接口字符串和接口类
	*/
	void Run(uint16_t port, const std::vector<std::pair<std::string, HttpInterface*>>& vec_interface);

	/** 主线程循环
	*/
	void Loop();

protected:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	//所有接口，接口字符串和接口类
	std::vector<std::pair<std::string, HttpInterface*>> vec_interface_;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	//服务器
	CivetServer* server_;
};