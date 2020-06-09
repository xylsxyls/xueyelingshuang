#pragma once
#include <stdint.h>
#include "CorrespondParam/CorrespondParamAPI.h"
#include <memory>
#include <map>
#include <atomic>
#include "ProcessWorkMacro.h"

class SharedMemory;
class ProcessReceiveCallback;
class Semaphore;

/** 进程通信类
*/
class ProcessWorkAPI ProcessWork
{
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
	/** 初始化接收模块
	@param [in] callback 接收回调类
	@param [in] receiveSize 单个共享内存接收区大小，1M
	@param [in] areaCount 缓存块个数
	*/
	void initReceive(ProcessReceiveCallback* callback, int32_t receiveSize = 1 * 1024 * 1024, int32_t areaCount = 100);

	/** 销毁接收模块资源
	*/
	void uninitReceive();

	/** 向服务端发送字符串，单进程有序，多进程无序，有先发后到的情况
	@param [in] destPid 目标进程pid
	@param [in] buffer 字符串地址
	@param [in] length 长度
	@param [in] protocolId 协议号
	*/
	void send(int32_t destPid, const char* buffer, int32_t length, CorrespondParam::ProtocolId protocolId = CorrespondParam::PROTO_MESSAGE);

	/** 向服务端发送字符串，单进程有序，多进程无序，有先发后到的情况
	@param [in] processName 目标进程名，带后缀
	@param [in] buffer 字符串地址
	@param [in] length 长度
	@param [in] protocolId 协议号
	*/
	void send(const std::string& processName, const char* buffer, int32_t length, CorrespondParam::ProtocolId protocolId = CorrespondParam::PROTO_MESSAGE);

protected:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::map<int32_t, std::pair<std::shared_ptr<SharedMemory>, std::shared_ptr<std::atomic<bool>>>> m_memoryMap;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	int32_t m_thisProcessPid;
	ProcessReceiveCallback* m_callback;
	Semaphore* m_assignSemaphore;
	Semaphore* m_assignEndSemaphore;
	Semaphore* m_readSemaphore;
	Semaphore* m_readEndSemaphore;
	SharedMemory* m_area;
	SharedMemory* m_pid;
	uint32_t m_assginThreadId;
	uint32_t m_readThreadId;
	uint32_t m_copyThreadId;
	uint32_t m_receiveThreadId;
	uint32_t m_sendThreadId;
};