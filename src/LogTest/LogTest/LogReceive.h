#pragma once
#include "ProcessWork/ProcessWorkAPI.h"
#include "CorrespondParam/CorrespondParamAPI.h"
#include "ProtoMessage/ProtoMessageAPI.h"
#include <memory>

class CTaskThread;

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

	/** ��ȡ���ͽ�����
	@param [in] sendPid ���ͽ���PID
	@return ���ط��ͽ�����
	*/
	std::string getSenderName(int32_t sendPid);

	/** ���÷����߳�
	@param [in] spScreenThread ��Ļչʾ�߳�
	@param [in] spLogThread ��־�߳�
	@param [in] spLogThread ��־�߳�
	*/
	void setThread(const std::shared_ptr<CTaskThread>& spScreenThread,
		const std::shared_ptr<CTaskThread>& spLogThread,
		const std::shared_ptr<CTaskThread>& spNetThread);

	/** �������һ����־ʱ�����ֵ
	@param [in] lastLogTime ���һ����־ʱ�����ֵ
	*/
	void setLastLogTime(std::atomic<int32_t>* lastLogTime);

	/** ���õ�¼��
	@param [in] loginName ��¼��
	*/
	void setLoginName(const std::string& loginName);

protected:
	std::map<int32_t, std::string> m_sendMap;
	std::shared_ptr<CTaskThread> m_spScreenThread;
	std::shared_ptr<CTaskThread> m_spLogThread;
	std::shared_ptr<CTaskThread> m_spNetThread;
	ProtoMessage m_message;
	std::atomic<int32_t>* m_lastLogTime;
	std::string m_loginName;
};