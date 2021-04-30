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
	/** ���պ���
	@param [in] sendPid ���ͽ���ID
	@param [in] buffer �����ڴ�
	@param [in] length ���ݳ���
	@param [in] type ��������
	*/
	virtual void receive(int32_t sendPid, const char* buffer, int32_t length, MessageType type);

	void setServer(Server* server);

	uv_tcp_t* getClientPtr(const char* buffer, int32_t length);

protected:
	Server* m_server;
};