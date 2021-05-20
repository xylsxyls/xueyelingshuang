#pragma once
#include "ProcessWork/ProcessWorkAPI.h"
#include "CorrespondParam/CorrespondParamAPI.h"
#include "ProtoMessage/ProtoMessageAPI.h"
#include <memory>

class CTaskThread;

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

	/** 获取发送进程名
	@param [in] sendPid 发送进程PID
	@return 返回发送进程名
	*/
	std::string getSenderName(int32_t sendPid);

	/** 设置发送线程
	@param [in] spScreenThread 屏幕展示线程
	@param [in] spLogThread 日志线程
	@param [in] spLogThread 日志线程
	*/
	void setThread(const std::shared_ptr<CTaskThread>& spScreenThread,
		const std::shared_ptr<CTaskThread>& spLogThread,
		const std::shared_ptr<CTaskThread>& spNetThread);

	/** 设置最后一次日志时间接收值
	@param [in] lastLogTime 最后一次日志时间接收值
	*/
	void setLastLogTime(std::atomic<int32_t>* lastLogTime);

	/** 设置登录名
	@param [in] loginName 登录名
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