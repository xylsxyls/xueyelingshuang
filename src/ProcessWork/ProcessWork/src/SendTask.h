#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "CorrespondParam/CorrespondParamAPI.h"

/** ��������
*/
class SendTask : public CTask
{
public:
	/** ���캯��
	@param[in] buffer �����׵�ַ
	@param[in] length ����
	*/
	SendTask(const char* buffer, int32_t length);

	/** ��������
	*/
	~SendTask();

public:
	/** ִ������
	*/
	void DoTask();

	/** ���ò���
	@param [in] thisProcessId ��ǰ����pid
	@param [in] destPid Ŀ�����pid
	@param [in] processName Ŀ���������������׺
	@param [in] protocolId ���뷽ʽ
	*/
	void setParam(int32_t thisProcessId,
		int32_t destPid,
		const std::string& processName,
		CorrespondParam::ProtocolId protocolId);

private:
	int32_t m_thisProcessPid;
	int32_t m_destPid;
	std::string m_processName;
	char* m_buffer;
	int32_t m_length;
	CorrespondParam::ProtocolId m_protocolId;
};