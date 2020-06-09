#pragma once
#include <stdint.h>
#include "CorrespondParam/CorrespondParamAPI.h"
#include <memory>
#include <map>
#include <atomic>
#include "ProcessWorkMacro.h"

class SharedMemory;
class ProcessReceiveCallback;
class Semaphore;

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
	@param [in] receiveSize ���������ڴ��������С��1M
	@param [in] areaCount ��������
	*/
	void initReceive(ProcessReceiveCallback* callback, int32_t receiveSize = 1 * 1024 * 1024, int32_t areaCount = 100);

	/** ���ٽ���ģ����Դ
	*/
	void uninitReceive();

	/** �����˷����ַ��������������򣬶�����������ȷ��󵽵����
	@param [in] destPid Ŀ�����pid
	@param [in] buffer �ַ�����ַ
	@param [in] length ����
	@param [in] protocolId Э���
	*/
	void send(int32_t destPid, const char* buffer, int32_t length, CorrespondParam::ProtocolId protocolId = CorrespondParam::PROTO_MESSAGE);

	/** �����˷����ַ��������������򣬶�����������ȷ��󵽵����
	@param [in] processName Ŀ�������������׺
	@param [in] buffer �ַ�����ַ
	@param [in] length ����
	@param [in] protocolId Э���
	*/
	void send(const std::string& processName, const char* buffer, int32_t length, CorrespondParam::ProtocolId protocolId = CorrespondParam::PROTO_MESSAGE);

protected:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::map<int32_t, std::pair<std::shared_ptr<SharedMemory>, std::shared_ptr<std::atomic<bool>>>> m_memoryMap;
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
	uint32_t m_sendThreadId;
};