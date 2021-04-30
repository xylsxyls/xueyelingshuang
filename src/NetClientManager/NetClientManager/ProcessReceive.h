#pragma once
#include "ProcessWork/ProcessWorkAPI.h"
#include "ProtoMessage/ProtoMessageAPI.h"
#include "CorrespondParam/CorrespondParamAPI.h"

class Client;

class ProcessReceive : public ProcessReceiveCallback
{
public:
	ProcessReceive();

public:
	/** ���պ���
	@param [in] sendPid ���ͽ���ID
	@param [in] buffer �����ڴ�
	@param [in] length ���ݳ���
	@param [in] type ��������
	*/
	virtual void receive(int32_t sendPid, const char* buffer, int32_t length, MessageType type);

	/** ���ÿͻ���
	@param [in] client �ͻ���
	*/
	void setClient(Client* client);

protected:
	Client* m_client;
	std::string m_computerName;
};