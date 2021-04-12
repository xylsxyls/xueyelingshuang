#pragma once
#include <stdint.h>
#include "CorrespondParam/CorrespondParamAPI.h"
#include <memory>
#include <map>
#include <atomic>
#include <mutex>
#include "ProcessWorkMacro.h"
#include "FiniteDeque/FiniteDequeAPI.h"

class SharedMemory;
class ProcessReceiveCallback;
class Semaphore;
class ProcessReadWriteMutex;

/** ����ͨ����
*/
class ProcessWorkAPI ProcessWork
{
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
	/** ��ʼ������ģ��
	@param [in] callback ���ջص���
	@param [in] receiveSize ���������ڴ��������С��500K
	@param [in] areaCount ��������
	@param [in] flow ������Դ����һ���򼸸����̷��͸�������Դ
	@return �����Ƿ��ʼ���ɹ�
	*/
	bool initReceive(ProcessReceiveCallback* callback, int32_t receiveSize = 500 * 1024, int32_t areaCount = 100, int32_t flow = 10);

	/** ���ٽ���ģ����Դ
	*/
	void uninitReceive();

	/** ��շ���ģ��Ļ�����Դ
	*/
	void clear();

	/** ��ʼ�������߳�
	*/
	void initPostThread();

	/** ���ٷ����߳�
	*/
	void uninitPostThread();
	
	/** �����˷����ַ��������������򣬶�����������ȷ��󵽵�������ú���ִ�������ʾ������д�빲���ڴ沢֪ͨ�Է�����ҵ��֤�Է����̴���
	windows�·����ٶ�9.6��/s�������ٶ�2.3��/s��linux�·����ٶ�1.6���/s�������ٶ�1.6���/s
	@param [in] destPid Ŀ�����pid
	@param [in] buffer �ַ�����ַ
	@param [in] length ����
	@param [in] protocolId Э���
	*/
	void send(int32_t destPid, const char* buffer, int32_t length, CorrespondParam::ProtocolId protocolId = CorrespondParam::PROTO_MESSAGE);

	/** �����˷����ַ��������������򣬶�����������ȷ��󵽵�������ú���ִ�������ʾ������д�빲���ڴ沢֪ͨ�Է�
	windows�·����ٶ�5���/s�������ٶ�2.5���/s��linux�·����ٶ�1.2���/s�������ٶ�1.2���/s
	@param [in] processName Ŀ�������������׺
	@param [in] buffer �ַ�����ַ
	@param [in] length ����
	@param [in] protocolId Э���
	*/
	void send(const std::string& processName, const char* buffer, int32_t length, CorrespondParam::ProtocolId protocolId = CorrespondParam::PROTO_MESSAGE);

	/** �����˷����ַ��������������򣬶�����������ȷ��󵽵����
	@param [in] destPid Ŀ�����pid
	@param [in] buffer �ַ�����ַ
	@param [in] length ����
	@param [in] protocolId Э���
	*/
	void post(int32_t destPid, const char* buffer, int32_t length, CorrespondParam::ProtocolId protocolId = CorrespondParam::PROTO_MESSAGE);

	/** �����˷����ַ��������������򣬶�����������ȷ��󵽵����
	@param [in] processName Ŀ�������������׺
	@param [in] buffer �ַ�����ַ
	@param [in] length ����
	@param [in] protocolId Э���
	*/
	void post(const std::string& processName, const char* buffer, int32_t length, CorrespondParam::ProtocolId protocolId = CorrespondParam::PROTO_MESSAGE);

	/** �����������ȴ���������ȫ��ִ�к󷵻أ��ú����������ʾ�ѽ�����ȫ��д�빲���ڴ沢��֪ͨ�Է����̣������������̺߳�
	*/
	void waitForPostEnd();

protected:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::map<int32_t, std::pair<std::shared_ptr<SharedMemory>, std::shared_ptr<std::atomic<bool>>>> m_memoryMap;
	std::mutex m_assignMutex;
	std::mutex m_readMutex;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	int32_t m_thisProcessPid;
	ProcessReceiveCallback* m_callback;
	Semaphore* m_assignSemaphore;
	Semaphore* m_assignEndSemaphore;
	Semaphore* m_readSemaphore;
	Semaphore* m_readEndSemaphore;
	SharedMemory* m_area;
	SharedMemory* m_pid;
	uint32_t m_assginThreadId;
	uint32_t m_readThreadId;
	uint32_t m_copyThreadId;
	uint32_t m_receiveThreadId;
	uint32_t m_postThreadId;

	int32_t m_flow;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	FiniteDeque<std::pair<int32_t, std::shared_ptr<SharedMemory>>> m_sendProcessDeque;
	std::mutex m_sendProcessDequeMutex;
	FiniteDeque<std::pair<std::string, std::shared_ptr<SharedMemory>>> m_sendMemoryDeque;
	std::mutex m_sendMemoryDequeMutex;
	FiniteDeque<std::pair<int32_t, std::shared_ptr<ProcessReadWriteMutex>[2]>> m_sendMutexDeque;
	std::mutex m_sendMutexDequeMutex;
	FiniteDeque<std::pair<int32_t, std::shared_ptr<Semaphore>[4]>> m_sendSemaphoreDeque;
	std::mutex m_sendSemaphoreDequeMutex;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};