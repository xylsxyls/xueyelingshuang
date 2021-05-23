#pragma once
#include "NetSender/NetSenderAPI.h"

class ServerReceive : public ServerReceiveCallback
{
public:
	ServerReceive();

public:
	/** �ͻ��˳�ʼ������
	@param [in] connectId ����ID
	@param [in] clientPid �ͻ��˽��̺�
	@param [in] buffer �����ڴ�
	@param [in] length ���ݳ���
	*/
	virtual void clientInit(int32_t connectId, int32_t clientPid, const char* buffer, int32_t length);

	/** ����˳�ʼ����Ӧ����
	@param [in] buffer �����ڴ�
	@param [in] length ���ݳ���
	*/
	virtual void serverInitResponse(const char* buffer, int32_t length);

	/** �ͻ�����Ϣ
	@param [in] connectId ����ID
	@param [in] clientPid �ͻ��˽��̺�
	@param [in] buffer �����ڴ�
	@param [in] length ���ݳ���
	*/
	virtual void ClientMessage(int32_t connectId, int32_t clientPid, const char* buffer, int32_t length);
};