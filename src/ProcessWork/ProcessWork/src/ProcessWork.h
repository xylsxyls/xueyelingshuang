#pragma once
#include "ProcessWorkMacro.h"
#include <stdint.h>
#include <map>
#include "CorrespondParam/CorrespondParamAPI.h"

class ProcessReceiveCallback;
class SharedMemory;
class ProcessReadWriteMutex;

/** ����ͨ����
*/
class ProcessWorkAPI ProcessWork
{
	friend class ReceiveTask;
	friend class ProcessWorkTask;
	friend class CreateKeyTask;
	friend class CreateDataTask;
protected:
	/** ���캯��
	*/
	ProcessWork();

public:
	/** ��һʵ��
	@return ���ص�һʵ��
	*/
	static ProcessWork& instance();

public:
	/** ������Դ
	*/
	void uninit();

public:
	/** ��ʼ������ģ��
	@param [in] callback ���ջص���
	*/
	void initReceive(ProcessReceiveCallback* callback);

	/** �����˷����ַ����������յ����̣߳������յ����߳�
	@param [in] buffer �ַ�����ַ
	@param [in] length ����
	@param [in] pid Ŀ�����id
	@param [in] protocolId Э���
	*/
	void send(const char* buffer, int32_t length, int32_t pid, CorrespondParam::ProtocolId protocolId = CorrespondParam::PROTO_MESSAGE);

	/** �����˷����ַ����������յ����̣߳������յ����߳�
	@param [in] buffer �ַ�����ַ
	@param [in] length ����
	@param [in] processName Ŀ�������
	@param [in] protocolId Э���
	*/
	void send(const char* buffer, int32_t length, const std::string& processName, CorrespondParam::ProtocolId protocolId = CorrespondParam::PROTO_MESSAGE);

protected:
	int32_t m_processPid;
	ProcessReceiveCallback* m_callback;
};