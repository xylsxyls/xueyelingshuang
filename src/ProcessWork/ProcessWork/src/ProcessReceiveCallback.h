#pragma once
#include <stdint.h>

/** ���ջص���
*/
class ProcessReceiveCallback
{
public:
	/** �����麯��
	@param [in] buffer �����ڴ�
	@param [in] length ���ݳ���
	@param [in] sendPid ���ͽ���ID
	@param [in] protocolId ��������Э��
	*/
	virtual void receive(char* buffer, int32_t length, int32_t sendPid, CorrespondParam::ProtocolId protocolId) = 0;
};