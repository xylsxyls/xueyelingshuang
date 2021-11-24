#pragma once
#include "ProcessWork/ProcessWorkAPI.h"

class CameraCommand : public ProcessReceiveCallback
{
public:
	/** �����麯��
	@param [in] sendPid ���ͽ���ID
	@param [in] buffer �����ڴ棬�հ�Ϊnullptr
	@param [in] length ���ݳ���
	@param [in] type ��������
	*/
	virtual void receive(int32_t sendPid, const char* buffer, int32_t length, MessageType type);
};