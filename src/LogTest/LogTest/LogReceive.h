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
	/** 构造函数
	*/
	LogReceive();

public:
	/** 接收虚函数
	@param [in] sendPid 发送进程ID
	@param [in] buffer 数据内存
	@param [in] length 数据长度
	@param [in] type 数据类型
	*/
	virtual void receive(int32_t sendPid, const char* buffer, int32_t length, MessageType type);

	/** 设置发送线程
	@param [in] spScreenThread 屏幕展示线程
	@param [in] spLogThread 日志线程
	@param [in] spLogThread 日志线程
	*/
	void setArea(ReadWriteMutex* screenMutex,
		Semaphore* screenSemaphore,
		LockFreeQueue<std::string>* screenQueue,
		ReadWriteMutex* logMutex,
		Semaphore* logSemaphore,
		LockFreeQueue<std::string>* logQueue,
		Semaphore* netSemaphore,
		LockFreeQueue<std::string>* netQueue);

	/** 设置最后一次日志时间接收值
	@param [in] lastLogTime 最后一次日志时间接收值
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