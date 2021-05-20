#pragma once
#include <stdint.h>
#include "ProcessWorkMacro.h"

/** ���ջص���
*/
class ProcessWorkAPI ProcessReceiveCallback
{
public:
	/** �����麯��
	@param [in] sendPid ���ͽ���ID
	@param [in] buffer �����ڴ�
	@param [in] length ���ݳ���
	@param [in] type ��������
	*/
	virtual void receive(int32_t sendPid, const char* buffer, int32_t length, MessageType type) = 0;
};