#pragma once
#include "ProcessWork/ProcessWorkAPI.h"
#include "CorrespondParam/CorrespondParamAPI.h"

typedef struct uv_tcp_s uv_tcp_t;
class Server;

class ServerProcessReceive : public ProcessReceiveCallback
{
public:
	/** ���캯��
	*/
	ServerProcessReceive();

public:
	/** ���պ���
	@param [in] sendPid ���ͽ���ID
	@param [in] buffer �����ڴ�
	@param [in] length ���ݳ���
	@param [in] type ��������
	*/
	virtual void receive(int32_t sendPid, const char* buffer, int32_t length, MessageType type);

	/** ���÷����ָ��
	@param [in] server �����ָ��
	*/
	void setServer(Server* server);

protected:
	Server* m_server;
};