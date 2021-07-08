#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "LockFreeQueue/LockFreeQueueAPI.h"

class Semaphore;
/** 发送任务
*/
class SendTask : public CTask
{
public:
	/** 构造函数
	*/
	SendTask();

public:
	/** 执行任务
	*/
	void DoTask();

	/** 中断任务
	*/
	void StopTask();

	/** 设置参数
	@param [in] waitEndPost 发送结束后是否退出循环
	@param [in] postQueue 发送数据队列
	@param [in] postSemaphore 发送信号量
	*/
	void setParam(std::atomic<bool>* waitEndPost, LockFreeQueue<char*>* postQueue, Semaphore* postSemaphore);

private:
	std::atomic<bool>* m_waitEndPost;
	LockFreeQueue<char*>* m_postQueue;
	Semaphore* m_postSemaphore;
	std::atomic<bool> m_exit;
};