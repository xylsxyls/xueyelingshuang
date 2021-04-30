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
	*/
	ReceiveTask();

	/** ��������
	*/
	~ReceiveTask();

public:
	/** ִ������
	*/
	void DoTask();

	/** ���ò���
	@param [in] buffer �����׵�ַ
	@param [in] length ����
	@param [in] sendPid �����ߵ�pid
	@param [in] callback ���ջص���
	@param [in] type ��������
	*/
	void setParam(const char* buffer,
		int32_t length,
		int32_t sendPid,
		std::vector<ProcessReceiveCallback*>* callback,
		MessageType type);

private:
	int32_t m_sendPid;
	MessageType m_type;
	std::vector<ProcessReceiveCallback*>* m_callback;
	char* m_buffer;
	int32_t m_length;
};