#pragma once
#include "NetSender/NetSenderAPI.h"
#include "CorrespondParam/CorrespondParamAPI.h"
#include <memory>

class Semaphore;
class CTaskThread;
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
	void setInitResponseSemaphore(Semaphore* semaphore);

	/** ���÷����߳�
	@param [in] spScreenThread ��Ļչʾ�߳�
	@param [in] spScreenThread ��־�߳�
	*/
	void setThread(const std::shared_ptr<CTaskThread>& spScreenThread, const std::shared_ptr<CTaskThread>& spLogThread);

	/** �������һ����־ʱ�����ֵ
	@param [in] lastLogTime ���һ����־ʱ�����ֵ
	*/
	void setLastLogTime(std::atomic<int32_t>* lastLogTime);

private:
	Semaphore* m_semaphore;
	std::atomic<int32_t>* m_lastLogTime;
	std::shared_ptr<CTaskThread> m_spScreenThread;
	std::shared_ptr<CTaskThread> m_spLogThread;
};