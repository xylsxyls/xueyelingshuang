#pragma once
#include <stdint.h>

/** ���ջص���
*/
class ProcessReceiveCallback
{
public:
	/** �����麯��
	@param [in] sendPid ���ͽ���ID
	@param [in] buffer �����ڴ�
	@param [in] length ���ݳ���
	@param [in] protocolId ��������Э��
	*/
	virtual void receive(int32_t sendPid, char* buffer, int32_t length, CorrespondParam::ProtocolId protocolId) = 0;
};