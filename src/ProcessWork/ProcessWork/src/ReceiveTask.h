#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "CorrespondParam/CorrespondParamAPI.h"

class ProcessReceiveCallback;
class SharedMemory;

/** ��������
*/
class ReceiveTask : public CTask
{
public:
	/** ���캯��
	@param[in] buffer �����׵�ַ
	@param[in] length ����
	*/
	ReceiveTask(const char* buffer, int32_t length);

	/** ��������
	*/
	~ReceiveTask();

public:
	/** ִ������
	*/
	void DoTask();

	/** ���ò���
	@param [in] sendPid �����ߵ�pid
	@param [in] callback ���ջص���
	@param [in] protocolId ���ݱ��뷽ʽ
	*/
	void setParam(int32_t sendPid,
		ProcessReceiveCallback* callback,
		CorrespondParam::ProtocolId protocolId);

private:
	int32_t m_sendPid;
	CorrespondParam::ProtocolId m_protocolId;
	ProcessReceiveCallback* m_callback;
	char* m_buffer;
	int32_t m_length;
};