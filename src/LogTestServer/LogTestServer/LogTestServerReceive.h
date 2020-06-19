#pragma once
#include "NetSender/NetSenderAPI.h"
#include "ProtoMessage/ProtoMessageAPI.h"
#include "CorrespondParam/CorrespondParamAPI.h"
#include <memory>

class CTaskThread;

class LogTestServerReceive : public ProcessReceiveCallback
{
public:
	LogTestServerReceive();

public:
	/** ���պ���
	@param [in] sendPid ���ͽ���ID
	@param [in] buffer �����ڴ�
	@param [in] length ���ݳ���
	@param [in] protocolId ��������Э��
	*/
	virtual void receive(int32_t sendPid, char* buffer, int32_t length, CorrespondParam::ProtocolId protocolId);

private:
	std::string m_iniPath;
};