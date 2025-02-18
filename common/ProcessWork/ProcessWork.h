#pragma once
#include "ProcessWorkMacro.h"
#include <stdint.h>
#include <map>

class ReceiveCallback;
class SharedMemory;
class ProcessReadWriteMutex;

/** 进程通信类
*/
class ProcessWorkAPI ProcessWork
{
	friend class ReceiveTask;
	friend class WorkTask;
	friend class DeleteTask;
	friend class CreateKeyTask;
	friend class CreateDataTask;
protected:
	/** 构造函数
	*/
	ProcessWork();

public:
	/** 单一实例
	@return 返回单一实例
	*/
	static ProcessWork& instance();

public:
	/** 销毁资源
	*/
	void uninit();

public:
	/** 初始化接收模块
	@param [in] callback 接收回调类
	*/
	void initReceive(ReceiveCallback* callback);

	/** 向服务端发送字符串，有序收到单线程，无序收到多线程
	@param [in] buffer 字符串地址
	@param [in] length 长度
	@param [in] pid 目标进程id
	@param [in] protocolId 协议号
	*/
	void send(const char* buffer, int32_t length, int32_t pid, int32_t protocolId = 1);

	/** 向服务端发送字符串，有序收到单线程，无序收到多线程
	@param [in] buffer 字符串地址
	@param [in] length 长度
	@param [in] processName 目标进程名
	@param [in] protocolId 协议号
	*/
	void send(const char* buffer, int32_t length, const std::string processName, int32_t protocolId = 1);

protected:
	int32_t m_processPid;
	ReceiveCallback* m_callback;
};