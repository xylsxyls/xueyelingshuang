#pragma once
#include "ProcessWork/ProcessWorkAPI.h"
#include "CorrespondParam/CorrespondParamAPI.h"
#include "ProtoMessage/ProtoMessageAPI.h"
#include "LockFreeQueue/LockFreeQueueAPI.h"
#include "Variant/VariantAPI.h"

class ReadWriteMutex;
class Semaphore;

class LogReceive : public ProcessReceiveCallback
{
public:
	/** ���캯��
	*/
	LogReceive();

public:
	/** �����麯��
	@param [in] sendPid ���ͽ���ID
	@param [in] buffer �����ڴ�
	@param [in] length ���ݳ���
	@param [in] type ��������
	*/
	virtual void receive(int32_t sendPid, const char* buffer, int32_t length, MessageType type);

	/** ���÷����߳�
	@param [in] spScreenThread ��Ļչʾ�߳�
	@param [in] spLogThread ��־�߳�
	@param [in] spLogThread ��־�߳�
	*/
	void setArea(ReadWriteMutex* screenMutex,
		Semaphore* screenSemaphore,
		LockFreeQueue<std::string>* screenQueue,
		ReadWriteMutex* logMutex,
		Semaphore* logSemaphore,
		LockFreeQueue<std::string>* logQueue,
		Semaphore* netSemaphore,
		LockFreeQueue<std::string>* netQueue);

	/** �������һ����־ʱ�����ֵ
	@param [in] lastLogTime ���һ����־ʱ�����ֵ
	*/
	void setLastLogTime(std::atomic<int32_t>* lastLogTime);

protected:
	ReadWriteMutex* m_screenMutex;
	Semaphore* m_screenSemaphore;
	LockFreeQueue<std::string>* m_screenQueue;
	ReadWriteMutex* m_logMutex;
	Semaphore* m_logSemaphore;
	LockFreeQueue<std::string>* m_logQueue;
	Semaphore* m_netSemaphore;
	LockFreeQueue<std::string>* m_netQueue;
	std::atomic<int32_t>* m_lastLogTime;
	ProtoMessage m_message;
	std::map<int32_t, Variant> m_messageMap;
};