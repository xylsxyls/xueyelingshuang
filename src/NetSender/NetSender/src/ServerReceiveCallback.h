#pragma once
#include "ProcessWork/ProcessWorkAPI.h"
#include "NetSenderMacro.h"

class NetSenderAPI ServerReceiveCallback : public ProcessReceiveCallback
{
public:
	/** �ͻ��˳�ʼ������
	@param [in] connectId ����ID
	@param [in] clientPid �ͻ��˽��̺�
	@param [in] buffer �����ڴ�
	@param [in] length ���ݳ���
	*/
	virtual void clientInit(int32_t connectId, int32_t clientPid, const char* buffer, int32_t length) = 0;

	/** ����˳�ʼ����Ӧ����
	@param [in] buffer �����ڴ�
	@param [in] length ���ݳ���
	*/
	virtual void serverInitResponse(const char* buffer, int32_t length) = 0;

	/** �ͻ���������������Ϣ
	@param [in] buffer �����ڴ�
	@param [in] length ���ݳ���
	*/
	virtual void NetClientMessage(int32_t connectId, const char* buffer, int32_t length);

	/** �����������������Ϣ
	@param [in] buffer �����ڴ�
	@param [in] length ���ݳ���
	*/
	virtual void NetServerMessage(const char* buffer, int32_t length);

	/** �ͻ�����Ϣ
	@param [in] connectId ����ID
	@param [in] clientPid �ͻ��˽��̺�
	@param [in] buffer �����ڴ�
	@param [in] length ���ݳ���
	*/
	virtual void ClientMessage(int32_t connectId, int32_t clientPid, const char* buffer, int32_t length) = 0;

protected:
	/** �����麯��
	@param [in] sendPid ���ͽ���ID
	@param [in] buffer �����ڴ�
	@param [in] length ���ݳ���
	@param [in] type ��������
	*/
	virtual void receive(int32_t sendPid, const char* buffer, int32_t length, MessageType type);
};