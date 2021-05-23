#pragma once
#include "NetSender/NetSenderAPI.h"

class ClientReceive : public ClientReceiveCallback
{
public:
	ClientReceive();

public:
	/** ��ʼ����Ӧ����
	@param [in] serverId ������̴���
	@param [in] buffer �����ڴ�
	@param [in] length ���ݳ���
	*/
	virtual void clientInitResponse(int32_t serverId, const char* buffer, int32_t length);

	/** ��������Ϣ
	@param [in] serverId ������̴���
	@param [in] buffer �����ڴ�
	@param [in] length ���ݳ���
	*/
	virtual void ServerMessage(int32_t serverId, const char* buffer, int32_t length);
};