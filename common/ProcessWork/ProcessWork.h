#pragma once
#include "ProcessWorkMacro.h"
#include <stdint.h>
#include <map>

class ReceiveCallback;
class SharedMemory;
class ProcessReadWriteMutex;

/** ����ͨ����
*/
class ProcessWorkAPI ProcessWork
{
	friend class ReceiveTask;
	friend class WorkTask;
	friend class DeleteTask;
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
	void initReceive(ReceiveCallback* callback);

	/** �����˷����ַ����������յ����̣߳������յ����߳�
	@param [in] buffer �ַ�����ַ
	@param [in] length ����
	@param [in] pid Ŀ�����id
	@param [in] protocolId Э���
	*/
	void send(const char* buffer, int32_t length, int32_t pid, int32_t protocolId = 1);

	/** �����˷����ַ����������յ����̣߳������յ����߳�
	@param [in] buffer �ַ�����ַ
	@param [in] length ����
	@param [in] processName Ŀ�������
	@param [in] protocolId Э���
	*/
	void send(const char* buffer, int32_t length, const std::string processName, int32_t protocolId = 1);

protected:
	int32_t m_processPid;
	ReceiveCallback* m_callback;
};