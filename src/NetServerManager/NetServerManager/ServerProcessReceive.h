#pragma once
#include "ProcessWork/ProcessWorkAPI.h"
#include "CorrespondParam/CorrespondParamAPI.h"

typedef struct uv_tcp_s uv_tcp_t;
class Server;

class ServerProcessReceive : public ProcessReceiveCallback
{
public:
	/** 构造函数
	*/
	ServerProcessReceive();

public:
	/** 接收函数
	@param [in] sendPid 发送进程ID
	@param [in] buffer 数据内存
	@param [in] length 数据长度
	@param [in] type 数据类型
	*/
	virtual void receive(int32_t sendPid, const char* buffer, int32_t length, MessageType type);

	/** 设置服务端指针
	@param [in] server 服务端指针
	*/
	void setServer(Server* server);

protected:
	Server* m_server;
};