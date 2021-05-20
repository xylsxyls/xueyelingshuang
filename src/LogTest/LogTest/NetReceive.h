#pragma once
#include "NetSender/NetSenderAPI.h"
#include "CorrespondParam/CorrespondParamAPI.h"
#include <memory>

class Semaphore;
class CTaskThread;
class NetReceive : public ClientReceiveCallback
{
public:
	/** 构造函数
	*/
	NetReceive();

public:
	/** 初始化响应函数
	@param [in] serverId 服务进程代号
	@param [in] buffer 数据内存
	@param [in] length 数据长度
	*/
	virtual void clientInitResponse(int32_t serverId, const char* buffer, int32_t length);

	/** 服务器消息
	@param [in] serverId 服务进程代号
	@param [in] buffer 数据内存
	@param [in] length 数据长度
	*/
	virtual void ServerMessage(int32_t serverId, const char* buffer, int32_t length);

	/** 设置初始化信号量接收类
	@param [in] semaphore 初始化信号量接收类
	*/
	void setInitResponseSemaphore(Semaphore* semaphore);

	/** 设置发送线程
	@param [in] spScreenThread 屏幕展示线程
	@param [in] spScreenThread 日志线程
	*/
	void setThread(const std::shared_ptr<CTaskThread>& spScreenThread, const std::shared_ptr<CTaskThread>& spLogThread);

	/** 设置最后一次日志时间接收值
	@param [in] lastLogTime 最后一次日志时间接收值
	*/
	void setLastLogTime(std::atomic<int32_t>* lastLogTime);

private:
	Semaphore* m_semaphore;
	std::atomic<int32_t>* m_lastLogTime;
	std::shared_ptr<CTaskThread> m_spScreenThread;
	std::shared_ptr<CTaskThread> m_spLogThread;
};