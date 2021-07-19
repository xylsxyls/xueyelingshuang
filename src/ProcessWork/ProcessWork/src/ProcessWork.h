#pragma once
#include "CorrespondParam/CorrespondParamAPI.h"
#include "ReadWriteMutex/ReadWriteMutexAPI.h"
#include "LockFreeQueue/LockFreeQueueAPI.h"
#include "Semaphore/SemaphoreAPI.h"
#include <memory>
#include <map>
#include <unordered_map>
#include <vector>
#include <atomic>
#include "ProcessWorkMacro.h"

class SharedMemory;
class ProcessReceiveCallback;

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
	/** ���ӽ��ջص��࣬��������ִ�лص�˳��
	@param [in] callback ���ջص���
	*/
	void addProcessReceiveCallback(ProcessReceiveCallback* callback);

	/** ��ʼ������ģ��
	@param [in] receiveSize ���������ڴ��������С��500K
	@param [in] areaCount ��������
	@return �����Ƿ��ʼ���ɹ�
	*/
	bool initReceive(int32_t receiveSize = 500 * 1024, int32_t areaCount = 100);

	/** ���ٽ���ģ����Դ
	*/
	void uninitReceive();

	/** ��շ���ģ��ָ��pid����Դ�����漰�洢pid�Ļ�����map
	@param [in] destPid ָ��pid
	*/
	void clearDestPid(int32_t destPid);

	/** ��շ���ģ�鲻�õĻ�����Դ
	*/
	void clearUseless();

	/** ��շ���ģ������л�����Դ
	*/
	void clear();

	/** ��ʼ�������̣߳����ɲ���
	*/
	void initPostThread();

	/** ���ٷ����߳�
	*/
	void uninitPostThread();
	
	/** �����˷����ַ��������������򣬶�����������ȷ��󵽵�������ú���ִ�������ʾ������д�빲���ڴ沢֪ͨ�Է�����ҵ��֤�Է����̴���
	windows�·����ٶ�71.0��/s�������ٶ�71.0��/s��linux�·����ٶ�45.9���/s�������ٶ�45.9���/s
	@param [in] destPid Ŀ�����pid
	@param [in] buffer �ַ�����ַ���հ���nullptr
	@param [in] length ����
	@param [in] type ������������
	*/
	void send(int32_t destPid, const char* buffer, int32_t length, MessageType type = MessageType::MESSAGE);

	/** �����˷�������
	@param [in] destPid Ŀ�����pid
	@param [in] message �������ݣ�����Ϊ���ַ�������ͬ��nullptr
	@param [in] type ������������
	*/
	void send(int32_t destPid, const std::string& message, MessageType type = MessageType::MESSAGE);

	/** �����˷����ַ��������������򣬶�����������ȷ��󵽵�������ú���ִ�������ʾ������д�빲���ڴ沢֪ͨ�Է�
	windows�·����ٶ�67.7���/s�������ٶ�67.7���/s��xp����Ϊ51���/s������̣�5����43.9���/s��10���̣�36.7���/s��20����57.2���/s��linux�·����ٶ�38.4���/s�������ٶ�38.4���/s
	@param [in] processName Ŀ���������windows�²�����׺����linux��ȫ��
	@param [in] buffer �ַ�����ַ
	@param [in] length ����
	@param [in] type ������������
	*/
	void send(const std::string& processName, const char* buffer, int32_t length, MessageType type = MessageType::MESSAGE);

	/** �����˷�������
	@param [in] processName Ŀ���������windows�²�����׺����linux��ȫ��
	@param [in] message ��������
	@param [in] type ������������
	*/
	void send(const std::string& processName, const std::string& message, MessageType type = MessageType::MESSAGE);

	/** �����˷����ַ��������������򣬶�������򣬵����̵��߳����򣬶���̻���߳̿��ܳ����ȷ���
	@param [in] destPid Ŀ�����pid
	@param [in] buffer �ַ�����ַ
	@param [in] length ����
	@param [in] type ������������
	*/
	void post(int32_t destPid, const char* buffer, int32_t length, MessageType type = MessageType::MESSAGE);

	/** �����˷����ַ��������������򣬶�������򣬵����̵��߳����򣬶���̻���߳̿��ܳ����ȷ���
	@param [in] destPid Ŀ�����pid
	@param [in] message ��������
	@param [in] type ������������
	*/
	void post(int32_t destPid, const std::string& message, MessageType type = MessageType::MESSAGE);

	/** �����˷����ַ��������������򣬶�������򣬵����̵��߳����򣬶���̻���߳̿��ܳ����ȷ���
	����11���߳�ͬʱ����ʱ����send��3���ٶȣ�12���߳���send��3��֮1�ٶȣ�����11�����ܴ���½���1���̷߳����ٶ�410.2���/s
	@param [in] processName Ŀ�������������׺
	@param [in] buffer �ַ�����ַ
	@param [in] length ����
	@param [in] type ������������
	*/
	void post(const std::string& processName, const char* buffer, int32_t length, MessageType type = MessageType::MESSAGE);

	/** �����˷����ַ��������������򣬶�������򣬵����̵��߳����򣬶���̻���߳̿��ܳ����ȷ���
	@param [in] processName Ŀ�������������׺
	@param [in] message ��������
	@param [in] type ������������
	*/
	void post(const std::string& processName, const std::string& message, MessageType type = MessageType::MESSAGE);

	/** �����������ȴ���������ȫ��ִ�к󷵻أ��ú����������ʾ�ѽ�����ȫ��д�빲���ڴ沢��֪ͨ�Է����̣������������̺߳�
	*/
	void waitForPostEnd();

protected:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::map<int32_t, std::shared_ptr<SharedMemory>> m_memoryMap;
	std::vector<ProcessReceiveCallback*> m_callback;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	int32_t m_thisProcessPid;
	Semaphore* m_readSemaphore;
	SharedMemory* m_areaAssign;
	SharedMemory* m_areaRead;
	SharedMemory* m_pid;
	uint32_t m_scanThreadId;
	uint32_t m_readThreadId;
	uint32_t m_copyThreadId;
	uint32_t m_receiveThreadId;
	uint32_t m_postThreadId;
	uint32_t m_postNameThreadId;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	ReadWriteMutex m_sendProcessAssignMapMutex;
	std::map<int32_t, std::shared_ptr<SharedMemory>> m_sendProcessAssignMap;
	ReadWriteMutex m_sendProcessReadMapMutex;
	std::map<int32_t, std::shared_ptr<SharedMemory>> m_sendProcessReadMap;
	ReadWriteMutex m_sendMemoryMapMutex;
	std::unordered_map<std::string, std::shared_ptr<SharedMemory>> m_sendMemoryMap;
	ReadWriteMutex m_sendSemaphoreMapMutex;
	std::map<int32_t, std::shared_ptr<Semaphore>> m_sendSemaphoreMap;
	ReadWriteMutex m_sendNameMemoryMapMutex;
	std::map<std::string, std::pair<int32_t, std::shared_ptr<SharedMemory>>> m_sendNameMemoryMap;

	LockFreeQueue<int32_t> m_assignQueue;
	Semaphore m_assignSemaphore;
	LockFreeQueue<char*> m_receiveQueue;
	Semaphore m_receiveSemaphore;

	ReadWriteMutex m_postMutex;
	std::atomic<bool> m_waitEndPost;
	LockFreeQueue<char*> m_postQueue;
	Semaphore m_postSemaphore;
	ReadWriteMutex m_postNameMutex;
	std::atomic<bool> m_waitEndPostName;
	LockFreeQueue<char*> m_postNameQueue;
	Semaphore m_postNameSemaphore;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};