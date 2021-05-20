#pragma once
#include "ProcessWork/ProcessWorkAPI.h"
#include "NetSenderMacro.h"

class NetSenderAPI ClientReceiveCallback : public ProcessReceiveCallback
{
public:
	/** ��ʼ����Ӧ����
	@param [in] serverId ������̴���
	@param [in] buffer �����ڴ�
	@param [in] length ���ݳ���
	*/
	virtual void clientInitResponse(int32_t serverId, const char* buffer, int32_t length) = 0;

	/** �ͻ���������������Ϣ
	@param [in] buffer �����ڴ�
	@param [in] length ���ݳ���
	*/
	virtual void NetClientMessage(const char* buffer, int32_t length);

	/** �����������������Ϣ
	@param [in] buffer �����ڴ�
	@param [in] length ���ݳ���
	*/
	virtual void NetServerMessage(const char* buffer, int32_t length);

	/** ��������Ϣ
	@param [in] serverId ������̴���
	@param [in] buffer �����ڴ�
	@param [in] length ���ݳ���
	*/
	virtual void ServerMessage(int32_t serverId, const char* buffer, int32_t length) = 0;

protected:
	/** �����麯��
	@param [in] sendPid ���ͽ���ID
	@param [in] buffer �����ڴ�
	@param [in] length ���ݳ���
	@param [in] type ��������
	*/
	virtual void receive(int32_t sendPid, const char* buffer, int32_t length, MessageType type);
};