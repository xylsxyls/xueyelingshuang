#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "LockFreeQueue/LockFreeQueueAPI.h"
#include "LogTestMessage.pb.h"
#include <atomic>
#include <string>

class Semaphore;

class LogTask : public CTask
{
public:
	LogTask();

public:
	virtual void DoTask();

	void StopTask();

	void setParam(Semaphore* logSemaphore, LockFreeQueue<std::string>* logQueue);

private:
	/** 把旧版logName转换为新版LogManager使用的日志目录
	@param [in] logName 业务侧传来的日志名，旧版本通常把它当作日志文件名使用
	@return 返回exe同级或绝对路径下的日志目录，空字符串表示使用默认日志
	*/
	std::string buildLogDir(const std::string& logName) const;

	/** 判断路径是否为当前系统可直接使用的绝对路径
	@param [in] path 待判断路径
	@return 返回是否为绝对路径
	*/
	bool isAbsolutePath(const std::string& path) const;

private:
	Semaphore* m_logSemaphore;
	LockFreeQueue<std::string>* m_logQueue;
	std::string m_buffer;
	logtest::LogTestMessage m_message;
	std::atomic<bool> m_exit;
};