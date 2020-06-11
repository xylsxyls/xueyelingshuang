#pragma once
#include "ProcessWork/ProcessWorkAPI.h"
#include "CorrespondParam/CorrespondParamAPI.h"

typedef struct uv_tcp_s uv_tcp_t;
class Server;

class ServerProcessReceive : public ProcessReceiveCallback
{
public:
	ServerProcessReceive();

public:
	/** 接收函数
	@param [in] sendPid 发送进程ID
	@param [in] buffer 数据内存
	@param [in] length 数据长度
	@param [in] protocolId 发送数据协议
	*/
	virtual void receive(int32_t sendPid, char* buffer, int32_t length, CorrespondParam::ProtocolId protocolId);

	void setServer(Server* server);

	uv_tcp_t* getClientPtr(const char* buffer, int32_t length);

protected:
	Server* m_server;
};