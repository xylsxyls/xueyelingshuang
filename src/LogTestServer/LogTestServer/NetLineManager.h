#pragma once
#include <map>
#include <stdint.h>
#include <mutex>

typedef struct uv_tcp_s uv_tcp_t;
/** 网络连接管理
*/
class NetLineManager
{
protected:
	/** 构造函数
	*/
	NetLineManager();

public:
	/** 单一实例
	@return 返回单一实例
	*/
	static NetLineManager& instance();

	/** 添加连接
	@param [in] serverName 服务端名
	@param [in] connect 连接指针
	*/
	void addConnect(const std::string& serverName, uv_tcp_t* connect);

	/** 查询连接
	@param [in] serverName 服务端名
	@return 返回连接指针
	*/
	uv_tcp_t* findConnect(const std::string& serverName);

private:
	std::map<std::string, uv_tcp_t*> m_connectedMap;
	std::mutex m_mutex;
};