#pragma once
#include "NetSender/NetSenderAPI.h"
#include "LockFreeQueue/LockFreeQueueAPI.h"

class ReadWriteMutex;
class Semaphore;

class NetReceive : public ClientReceiveCallback
{
public:
	/** ���캯��
	*/
	NetReceive();

public:
	/** ��ʼ����Ӧ����
	@param [in] serverId ������̴���
	@param [in] buffer �����ڴ�
	@param [in] length ���ݳ���
	*/
	virtual void clientInitResponse(int32_t serverId, const char* buffer, int32_t length);

	/** ��������Ϣ
	@param [in] serverId ������̴���
	@param [in] buffer �����ڴ�
	@param [in] length ���ݳ���
	*/
	virtual void ServerMessage(int32_t serverId, const char* buffer, int32_t length);

	/** ���ó�ʼ���ź���������
	@param [in] semaphore ��ʼ���ź���������
	*/
	void setInitResponseSemaphore(Semaphore* initResponseSemaphore);

	/** ���û�����
	@param [in] spScreenThread ��Ļչʾ�߳�
	@param [in] spScreenThread ��־�߳�
	*/
	void setArea(ReadWriteMutex* screenMutex,
		Semaphore* screenSemaphore,
		LockFreeQueue<std::string>* screenQueue,
		ReadWriteMutex* logMutex,
		Semaphore* logSemaphore,
		LockFreeQueue<std::string>* logQueue);

	/** �������һ����־ʱ�����ֵ
	@param [in] lastLogTime ���һ����־ʱ�����ֵ
	*/
	void setLastLogTime(std::atomic<int32_t>* lastLogTime);

private:
	Semaphore* m_initResponseSemaphore;
	ReadWriteMutex* m_screenMutex;
	Semaphore* m_screenSemaphore;
	LockFreeQueue<std::string>* m_screenQueue;
	ReadWriteMutex* m_logMutex;
	Semaphore* m_logSemaphore;
	LockFreeQueue<std::string>* m_logQueue;
	std::atomic<int32_t>* m_lastLogTime;
};