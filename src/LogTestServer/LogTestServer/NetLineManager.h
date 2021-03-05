#pragma once
#include <map>
#include <stdint.h>
#include <mutex>
#include <vector>

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
	@param [in] loginName 客户端登录名
	@param [in] clientPid 客户端PID
	@param [in] connectId 服务器端的客户端ID
	*/
	void addConnect(const std::string& loginName, int32_t clientPid, int32_t connectId);

	/** 查询连接
	@param [in] loginName 客户端登录名
	@return 返回连接指针数组
	*/
	std::vector<std::pair<int32_t, int32_t>> findConnect(const std::string& loginName);

	/** 查询登陆名
	@param [in] clientPid 客户端PID
	@param [in] connectId 服务器端的客户端ID
	@return 返回登陆名
	*/
	std::string findLoginName(int32_t clientPid, int32_t connectId);

private:
	//clientPid, connectId, loginName
	std::map<std::pair<int32_t, int32_t>, std::string> m_connectedMap;
	//loginName, vecClient clientPid, connectId
	std::map<std::string, std::vector<std::pair<int32_t, int32_t>>> m_loginNameMap;
	std::mutex m_mutex;
};