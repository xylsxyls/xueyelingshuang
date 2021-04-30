#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "CorrespondParam/CorrespondParamAPI.h"

/** ��������
*/
class SendTask : public CTask
{
public:
	/** ���캯��
	*/
	SendTask();

	/** ��������
	*/
	~SendTask();

public:
	/** ִ������
	*/
	void DoTask();

	/** ���ò���
	@param [in] buffer �����׵�ַ
	@param [in] length ����
	@param [in] thisProcessId ��ǰ����pid
	@param [in] destPid Ŀ�����pid
	@param [in] processName Ŀ���������������׺
	@param [in] type ��������
	*/
	void setParam(const char* buffer,
		int32_t length,
		int32_t thisProcessId,
		int32_t destPid,
		const std::string& processName,
		MessageType type);

private:
	int32_t m_thisProcessPid;
	int32_t m_destPid;
	std::string m_processName;
	char* m_buffer;
	int32_t m_length;
	MessageType m_type;
};